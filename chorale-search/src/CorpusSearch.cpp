#include "CorpusSearch.hpp"

#include <algorithm>
#include <atomic>
#include <memory>
#include <stdexcept>
#include <utility>

#include "AttributeMatcher.hpp"
#include "FileDispatcher.hpp"
#include "HumdrumChorale.hpp"
#include "HumdrumUtils.hpp"
#include "VoiceMap.hpp"

namespace fs = std::filesystem;

namespace choralesearch {

namespace {

MatcherOptions matcherOptions(const Query& query) {
    MatcherOptions options;
    options.mintStartAtPreviousToken = query.mintStartAtPreviousToken;
    options.mintAllowIntervalComplementation = query.mintAllowIntervalComplementation;
    options.fbCompareExactChord = query.fbCompareExactChord;
    options.kernIgnoreOctave = query.kernIgnoreOctave;
    options.hintReduceCompound = query.hintReduceCompound;
    options.durationAllowSplitNotes = query.durationAllowSplitNotes;
    options.durationAllowMergedNotes = query.durationAllowMergedNotes;
    options.metweightSkipUnclassified = query.metweightSkipUnclassified;
    return options;
}

// A group's own options are overrides -- nullopt falls back to the top-level query's own value
// of the same name, not an independent default.
MatcherOptions matcherOptions(const Query& query, const SimultaneousGroup& group) {
    MatcherOptions options;
    options.mintStartAtPreviousToken = group.mintStartAtPreviousToken.value_or(query.mintStartAtPreviousToken);
    options.mintAllowIntervalComplementation =
        group.mintAllowIntervalComplementation.value_or(query.mintAllowIntervalComplementation);
    options.fbCompareExactChord = group.fbCompareExactChord.value_or(query.fbCompareExactChord);
    options.kernIgnoreOctave = group.kernIgnoreOctave.value_or(query.kernIgnoreOctave);
    options.hintReduceCompound = group.hintReduceCompound.value_or(query.hintReduceCompound);
    options.durationAllowSplitNotes = group.durationAllowSplitNotes.value_or(query.durationAllowSplitNotes);
    options.durationAllowMergedNotes = group.durationAllowMergedNotes.value_or(query.durationAllowMergedNotes);
    options.metweightSkipUnclassified = group.metweightSkipUnclassified.value_or(query.metweightSkipUnclassified);
    return options;
}

// simultaneousAlignment is resolved the same way, but it isn't a MatcherOptions field: it
// decides which of a group's match positions must line up with the primary match, which is
// this file's own job rather than anything the matcher does.
struct ResolvedSimultaneousGroup {
    std::vector<AttributeMatch> matches;
    bool checkStart;
    bool checkEnd;
};

ResolvedSimultaneousGroup resolveSimultaneousGroup(const HumdrumChorale& chorale, const Query& query,
                                                    const SimultaneousGroup& group) {
    ResolvedSimultaneousGroup resolved;
    std::string alignment = group.simultaneousAlignment.value_or(query.simultaneousAlignment);
    resolved.checkStart = alignment != "end";
    resolved.checkEnd = alignment != "start";

    if (!chorale.hasFeature(group.feature)) return resolved;

    AttributeMatcher matcher(group.feature, group.pattern, matcherOptions(query, group));
    for (std::size_t voice : resolveVoices(group.voices)) {
        for (const auto& m : matcher.findAll(chorale, voice)) resolved.matches.push_back(m);
    }
    return resolved;
}

// The files a run has to open: those whose chorale id matches the filter of at least one query. Any other
// is skipped without being parsed. (What else a filter asks is only known once a chorale is loaded, see runOne.)
std::vector<fs::path> filesWithMatchingId(std::vector<fs::path> files, const std::vector<Query>& queries) {
    files.erase(std::remove_if(files.begin(), files.end(),
                               [&](const fs::path& file) {
                                   const std::string id = file.stem().string();
                                   return std::none_of(queries.begin(), queries.end(), [&](const Query& query) {
                                       return query.choraleFilter.matchesId(id);
                                   });
                               }),
                files.end());
    return files;
}

// The other voice a hint-<voice> key compares the voice searched with ("hint-2": 2), possibly negated ("!hint-2"),
// or 0 for any other key.
std::size_t otherVoiceOfHintKey(const std::string& key) {
    const std::string bare = key.substr(!key.empty() && key[0] == '!' ? 1 : 0);
    const bool voiceKey = bare.size() == 6 && bare.rfind("hint-", 0) == 0 && bare[5] >= '1' && bare[5] <= '4';
    return voiceKey ? static_cast<std::size_t>(bare[5] - '0') : 0;
}

// The two voices of the spine of a pair ("hint-24": 2 and 4), none for any other feature.
std::vector<std::size_t> voicesOfHintPair(const std::string& feature) {
    if (feature.size() != 7 || feature.rfind("hint-", 0) != 0) return {};
    return {static_cast<std::size_t>(feature[5] - '0'), static_cast<std::size_t>(feature[6] - '0')};
}

// The voices a match spans from its start to its end, by the feature it was found with: both voices of the spine of a
// pair, none for fb, whose spine is no voice's own, and otherwise the voice searched.
std::vector<std::size_t> voicesSpannedByMatch(const AttributeMatch& attributeMatch, const std::string& feature) {
    const std::vector<std::size_t> pair = voicesOfHintPair(feature);
    if (!pair.empty()) return pair;
    if (feature == "fb") return {};
    return {attributeMatch.voice};
}

// The sections of notes a match is made of, by the feature and the pattern it was found with: the voices it spans,
// except those in `coveredVoices`, which another match covers, and the voice of a hint-<voice> key at each position
// that names it.
std::vector<NoteSection> buildLinkedSections(const AttributeMatch& attributeMatch, const std::string& feature,
                                             const std::vector<AttributeMap>& pattern,
                                             const std::vector<std::size_t>& coveredVoices = {}) {
    std::vector<NoteSection> sections;
    // The voices the match spans, over the whole match.
    for (std::size_t voice : voicesSpannedByMatch(attributeMatch, feature)) {
        if (std::find(coveredVoices.begin(), coveredVoices.end(), voice) != coveredVoices.end()) continue;
        sections.push_back({feature, voice, humNumToString(attributeMatch.startPosition),
                            humNumToString(attributeMatch.endPosition), attributeMatch.startLineNumber,
                            attributeMatch.endLineNumber});
    }
    // The voices the keys of a position name, at that position only.
    for (std::size_t position = 0; position < pattern.size() && position < attributeMatch.positionOnsets.size(); ++position) {
        const AttributeMatch::PositionOnset& onset = attributeMatch.positionOnsets[position];
        const auto atPosition = [&](const std::string& key, std::size_t voice) {
            if (voice == attributeMatch.voice) return;
            sections.push_back({key, voice, humNumToString(onset.position), humNumToString(onset.position), onset.line,
                                onset.line});
        };
        for (const auto& entry : pattern[position]) {
            const std::string key = entry.first.substr(!entry.first.empty() && entry.first[0] == '!' ? 1 : 0);
            // A hint-<voice> key: the other voice it compares the voice searched with.
            const std::size_t other = otherVoiceOfHintKey(key);
            if (other != 0) atPosition(key, other);
        }
    }
    return sections;
}

// A chorale together with the results of searching it, one list for each query. A HumdrumChorale
// can be neither copied nor moved, which is why it is held by a pointer.
struct SearchedChorale {
    std::unique_ptr<HumdrumChorale> chorale;
    std::vector<Results> results; // per query
};

} // namespace

CorpusSearch::CorpusSearch(fs::path corpusRoot, bool applyAnalysis)
    : m_corpusRoot(std::move(corpusRoot)), m_applyAnalysis(applyAnalysis) {}

std::vector<fs::path> CorpusSearch::findChoraleFiles() const {
    if (!fs::exists(m_corpusRoot)) {
        throw std::runtime_error("Corpus root does not exist: " + m_corpusRoot.string());
    }
    std::vector<fs::path> files;
    if (fs::is_regular_file(m_corpusRoot)) {
        files.push_back(m_corpusRoot);
    } else {
        for (const auto& entry : fs::recursive_directory_iterator(m_corpusRoot)) {
            if (entry.is_regular_file() && entry.path().extension() == ".krn") {
                files.push_back(entry.path());
            }
        }
    }
    std::sort(files.begin(), files.end());
    return files;
}

void CorpusSearch::reportProgress(std::size_t choralesSearched, std::size_t choralesTotal,
                                  const std::string& choraleId, std::size_t matchesSoFar) const {
    if (m_onProgress) m_onProgress(SearchProgress{choralesSearched, choralesTotal, choraleId, matchesSoFar});
}

Results CorpusSearch::runOne(const HumdrumChorale& chorale, const Query& query) const {
    Results results;
    if (!chorale.hasFeature(query.feature)) return results;
    if (!query.choraleFilter.matchesId(chorale.id())) return results;
    if (query.choraleFilter.needsChoraleInfo() && !query.choraleFilter.matches(chorale.collectInfo())) return results;

    AttributeMatcher matcher(query.feature, query.pattern, matcherOptions(query));

    // Precompute each simultaneousWith group's own match positions (and resolved options)
    // once per chorale, rather than re-running its matcher for every one of the primary
    // pattern's own matches.
    std::vector<ResolvedSimultaneousGroup> resolvedGroups;
    for (const auto& group : query.simultaneousWith) {
        resolvedGroups.push_back(resolveSimultaneousGroup(chorale, query, group));
    }

    for (std::size_t voice : resolveVoices(query.voices)) {
        for (auto& m : matcher.findAll(chorale, voice)) {
            // Every match of each simultaneousWith entry that lines up with this one.
            std::vector<std::vector<const AttributeMatch*>> simultaneousMatches;
            for (const ResolvedSimultaneousGroup& g : resolvedGroups) {
                std::vector<const AttributeMatch*> lined;
                for (const AttributeMatch& candidate : g.matches) {
                    if (g.checkStart && candidate.startPosition != m.startPosition) continue;
                    if (g.checkEnd && candidate.endPosition != m.endPosition) continue;
                    lined.push_back(&candidate);
                }
                if (lined.empty()) break;
                simultaneousMatches.push_back(std::move(lined));
            }
            if (simultaneousMatches.size() != resolvedGroups.size()) continue;

            Result r;
            r.choraleId = chorale.id();
            r.feature = query.feature;
            r.voice = m.voice;
            r.startPosition = humNumToString(m.startPosition);
            r.endPosition = humNumToString(m.endPosition);
            r.startLineNumber = m.startLineNumber;
            r.endLineNumber = m.endLineNumber;

            // The sections of every aligned match of each group, by the feature and pattern of the group, noting the
            // voices a group covers over its whole match.
            std::vector<std::size_t> coveredBySimultaneous;
            for (std::size_t group = 0; group < simultaneousMatches.size(); ++group) {
                const SimultaneousGroup& simultaneousGroup = query.simultaneousWith[group];
                for (const AttributeMatch* simultaneousMatch : simultaneousMatches[group]) {
                    for (std::size_t voice : voicesSpannedByMatch(*simultaneousMatch, simultaneousGroup.feature)) {
                        coveredBySimultaneous.push_back(voice);
                    }
                    for (NoteSection& section : buildLinkedSections(*simultaneousMatch, simultaneousGroup.feature, simultaneousGroup.pattern)) {
                        r.linkedSections.push_back(std::move(section));
                    }
                }
            }
            // The sections of this match, but none over the whole match in a voice a group covers: the group's section
            // starts where the voice attacks its note, this match may only hold it.
            for (NoteSection& section : buildLinkedSections(m, query.feature, query.pattern, coveredBySimultaneous)) {
                r.linkedSections.push_back(std::move(section));
            }
            std::sort(r.linkedSections.begin(), r.linkedSections.end());
            r.linkedSections.erase(std::unique(r.linkedSections.begin(), r.linkedSections.end()), r.linkedSections.end());
            results.push_back(std::move(r));

            if (query.limit && results.size() >= *query.limit) return results;
        }
    }
    return results;
}

Results CorpusSearch::run(const Query& query) const {
    // The same search as for a list of one, which numbers its results; a lone query has no use for that.
    Results results = run(std::vector<Query>{query});
    for (Result& result : results) result.queryId.reset();
    return results;
}

Results CorpusSearch::run(const std::vector<Query>& queries) const {
    std::vector<std::string> queryIds;
    queryIds.reserve(queries.size());
    for (std::size_t i = 0; i < queries.size(); ++i) {
        queryIds.push_back(queries[i].id.value_or(std::to_string(i)));
    }

    // Each query gets its own result bucket (so its own `limit`, if any, caps only its own
    // matches) and its own `done` flag (so a query that already hit its limit is not added to
    // for the remaining files, without stopping the others still running).
    std::vector<Results> perQuery(queries.size());
    std::vector<std::atomic<bool>> done(queries.size()); // read while searching, set while collecting
    for (std::atomic<bool>& flag : done) flag = false;

    const auto files = filesWithMatchingId(findChoraleFiles(), queries);
    FileDispatcher<SearchedChorale>(
        files.size(),
        [&](std::size_t fileIndex) {
            // Parsed once per file, shared by every query.
            SearchedChorale searched{std::make_unique<HumdrumChorale>(files[fileIndex].string(), m_applyAnalysis),
                                     std::vector<Results>(queries.size())};
            for (std::size_t i = 0; i < queries.size(); ++i) {
                const Query& query = queries[i];
                if (done[i] || !searched.chorale->hasFeature(query.feature)) continue;
                searched.results[i] = runOne(*searched.chorale, query);
                for (auto& r : searched.results[i]) r.queryId = queryIds[i];
            }
            return searched;
        },
        [&](std::size_t fileIndex, SearchedChorale& searched) {
            for (std::size_t i = 0; i < queries.size(); ++i) {
                if (done[i]) continue;
                Results& bucket = perQuery[i];
                bucket.insert(bucket.end(), std::make_move_iterator(searched.results[i].begin()),
                              std::make_move_iterator(searched.results[i].end()));
                if (queries[i].limit && bucket.size() >= *queries[i].limit) {
                    bucket.resize(*queries[i].limit);
                    done[i] = true;
                }
            }
            std::size_t matchesSoFar = 0;
            for (const Results& bucket : perQuery) matchesSoFar += bucket.size();
            reportProgress(fileIndex + 1, files.size(), searched.chorale->id(), matchesSoFar);
            // Every query has its limit, what is left of the corpus is not needed.
            return !std::all_of(done.begin(), done.end(), [](const std::atomic<bool>& d) { return d.load(); });
        }).run();

    Results allResults;
    for (auto& bucket : perQuery) {
        allResults.insert(allResults.end(), std::make_move_iterator(bucket.begin()), std::make_move_iterator(bucket.end()));
    }
    return allResults;
}

void CorpusSearch::forEachMatch(const std::vector<Query>& queries, const MatchVisitor& visit) const {
    const auto files = filesWithMatchingId(findChoraleFiles(), queries);
    std::size_t matchesSoFar = 0;
    FileDispatcher<SearchedChorale>(
        files.size(),
        [&](std::size_t fileIndex) {
            SearchedChorale searched{std::make_unique<HumdrumChorale>(files[fileIndex].string(), m_applyAnalysis),
                                     std::vector<Results>(queries.size())};
            for (std::size_t i = 0; i < queries.size(); ++i) {
                if (!searched.chorale->hasFeature(queries[i].feature)) continue;
                searched.results[i] = runOne(*searched.chorale, queries[i]);
            }
            return searched;
        },
        [&](std::size_t fileIndex, SearchedChorale& searched) {
            for (std::size_t i = 0; i < queries.size(); ++i) {
                for (Result& result : searched.results[i]) {
                    result.queryId = queries[i].id.value_or(std::to_string(i));
                    ++matchesSoFar;
                    visit(*searched.chorale, result);
                }
            }
            reportProgress(fileIndex + 1, files.size(), searched.chorale->id(), matchesSoFar);
            return true;
        }).run();
}

} // namespace choralesearch

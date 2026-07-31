#include "CorpusSearch.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

#include "AttributeMatcher.hpp"
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
    std::vector<std::pair<hum::HumNum, hum::HumNum>> positions; // (startPosition, endPosition) per match
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
        for (const auto& m : matcher.findAll(chorale, voice)) {
            resolved.positions.emplace_back(m.startPosition, m.endPosition);
        }
    }
    return resolved;
}

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

Results CorpusSearch::runOne(const HumdrumChorale& chorale, const Query& query) const {
    Results results;
    if (!chorale.hasFeature(query.feature)) return results;

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
            bool alignsWithEverySimultaneousGroup =
                std::all_of(resolvedGroups.begin(), resolvedGroups.end(), [&](const ResolvedSimultaneousGroup& g) {
                    return std::any_of(g.positions.begin(), g.positions.end(), [&](const auto& p) {
                        if (g.checkStart && p.first != m.startPosition) return false;
                        if (g.checkEnd && p.second != m.endPosition) return false;
                        return true;
                    });
                });
            if (!alignsWithEverySimultaneousGroup) continue;

            Result r;
            r.choraleId = chorale.id();
            r.feature = query.feature;
            r.voiceLabel = voiceLabel(voice);
            r.voice = m.voice;
            r.startPosition = humNumToString(m.startPosition);
            r.endPosition = humNumToString(m.endPosition);
            r.startLineNumber = m.startLineNumber;
            r.endLineNumber = m.endLineNumber;
            results.push_back(std::move(r));

            if (query.limit && results.size() >= *query.limit) return results;
        }
    }
    return results;
}

Results CorpusSearch::run(const Query& query) const {
    Results allResults;
    for (const auto& file : findChoraleFiles()) {
        HumdrumChorale chorale(file.string(), m_applyAnalysis);
        if (!chorale.hasFeature(query.feature)) continue;
        auto results = runOne(chorale, query);
        allResults.insert(allResults.end(), std::make_move_iterator(results.begin()), std::make_move_iterator(results.end()));
        if (query.limit && allResults.size() >= *query.limit) {
            allResults.resize(*query.limit);
            break;
        }
    }
    return allResults;
}

Results CorpusSearch::run(const std::vector<Query>& queries) const {
    std::vector<std::string> queryIds;
    queryIds.reserve(queries.size());
    for (std::size_t i = 0; i < queries.size(); ++i) {
        queryIds.push_back(queries[i].id.value_or(std::to_string(i)));
    }

    // Each query gets its own result bucket (so its own `limit`, if any, caps only its own
    // matches) and its own `done` flag (so a query that already hit its limit is skipped
    // for every remaining file, without stopping the others still running).
    std::vector<Results> perQuery(queries.size());
    std::vector<bool> done(queries.size(), false);

    for (const auto& file : findChoraleFiles()) {
        // The loop only ever reaches this point if at least one query is still pending (see
        // the all_of(done) break below), so this is never wasted work -- no need to defer
        // construction.
        HumdrumChorale chorale(file.string(), m_applyAnalysis); // parsed once per file, shared by every query
        for (std::size_t i = 0; i < queries.size(); ++i) {
            if (done[i]) continue;
            const Query& query = queries[i];
            if (!chorale.hasFeature(query.feature)) continue;

            auto results = runOne(chorale, query);
            for (auto& r : results) r.queryId = queryIds[i];
            Results& bucket = perQuery[i];
            bucket.insert(bucket.end(), std::make_move_iterator(results.begin()), std::make_move_iterator(results.end()));
            if (query.limit && bucket.size() >= *query.limit) {
                bucket.resize(*query.limit);
                done[i] = true;
            }
        }
        if (std::all_of(done.begin(), done.end(), [](bool d) { return d; })) break;
    }

    Results allResults;
    for (auto& bucket : perQuery) {
        allResults.insert(allResults.end(), std::make_move_iterator(bucket.begin()), std::make_move_iterator(bucket.end()));
    }
    return allResults;
}

} // namespace choralesearch

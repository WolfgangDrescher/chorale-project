#include "Check.hpp"

#include <algorithm>
#include <tuple>
#include <utility>

#include "CorpusSearch.hpp"
#include "HumdrumUtils.hpp"
#include "Result.hpp"
#include "VoiceMap.hpp"

namespace choralesearch {

namespace {

// A kind of parallel: the type reported, and the perfect intervals it is made of. Compound ones
// are folded by the query's hintReduceCompound, so "P5" also stands for the twelfth. A unison
// counts as an octave -- the two voices sing the same note.
struct ParallelKind {
    const char* type;
    std::vector<std::string> intervals;
};

const std::vector<ParallelKind>& parallelKinds() {
    static const std::vector<ParallelKind> kinds = {
        {"parallelFifths", {"P5"}},
        {"parallelOctaves", {"P8", "P1"}},
    };
    return kinds;
}

struct ParallelQuery {
    Query query;
    const char* type;
    const char* direction;
    std::size_t lowerVoice;
    std::size_t upperVoice;
};

std::string queryId(const char* type, const char* direction, std::size_t lower, std::size_t upper) {
    return std::string(type) + ":" + direction + ":" + std::to_string(lower) + "-" + std::to_string(upper);
}

// Two consecutive notes of a voice: any first, a second reached by a move in the given direction
// ("+" up, "-" down, whatever the size or quality of the interval).
std::vector<AttributeMap> movePattern(const std::string& sign, const AttributeMap& firstExtra,
                                       const AttributeMap& secondExtra) {
    AttributeMap first = firstExtra;
    first["mint"] = {"*"};
    AttributeMap second = secondExtra;
    second["mint"] = {sign};
    return {first, second};
}

// One query for each pair of voices, direction and interval (6 x 2 x 2 = 24), with the id
// "<check>:<direction>:<lower voice>-<upper voice>". The upper voice is walked by its melodic
// intervals and has to form the interval to the lower one; the lower voice is a simultaneousWith
// group, so it attacks the same notes and moves the same way.
std::vector<ParallelQuery> buildParallelQueries() {
    std::vector<ParallelQuery> queries;
    for (const ParallelKind& kind : parallelKinds()) {
        for (const auto& [direction, sign] : std::vector<std::pair<const char*, std::string>>{{"up", "+"}, {"down", "-"}}) {
            for (std::size_t lower = 1; lower <= kVoiceCount; ++lower) {
                for (std::size_t upper = lower + 1; upper <= kVoiceCount; ++upper) {
                    // The interval to the lower voice, from the walked upper voice's point of view.
                    const std::string intervalKey = "hint-" + std::to_string(lower);
                    AttributeMap interval;
                    interval[intervalKey] = kind.intervals;

                    Query query;
                    query.id = queryId(kind.type, direction, lower, upper);
                    query.feature = "mint";
                    query.voices = std::to_string(upper);
                    query.pattern = movePattern(sign, interval, interval);
                    query.hintReduceCompound = true;
                    query.simultaneousAlignment = "start-end";

                    SimultaneousGroup lowerVoice;
                    lowerVoice.feature = "mint";
                    lowerVoice.voices = std::to_string(lower);
                    lowerVoice.pattern = movePattern(sign, {}, {});
                    query.simultaneousWith.push_back(std::move(lowerVoice));

                    queries.push_back({std::move(query), kind.type, direction, lower, upper});
                }
            }
        }
    }
    return queries;
}

} // namespace

std::vector<Finding> findParallelMotion(const HumdrumChorale& chorale) {
    // Only needed for runOne, which searches the chorale it is given and nothing on disk.
    const CorpusSearch search(chorale.path());

    std::vector<Finding> findings;
    for (const ParallelQuery& entry : buildParallelQueries()) {
        for (const Result& match : search.runOne(chorale, entry.query)) {
            Finding finding;
            finding.check = entry.type;
            finding.direction = entry.direction;
            finding.lowerVoice = entry.lowerVoice;
            finding.upperVoice = entry.upperVoice;
            finding.startLine = static_cast<int>(match.startLineNumber);
            finding.endLine = static_cast<int>(match.endLineNumber);
            finding.startPosition = match.startPosition;
            finding.endPosition = match.endPosition;
            findings.push_back(std::move(finding));
        }
    }

    // In the order of the score, the same place by voices and kind, so the result doesn't
    // depend on the order the queries run in.
    std::stable_sort(findings.begin(), findings.end(), [](const Finding& a, const Finding& b) {
        return std::tie(a.startLine, a.lowerVoice, a.upperVoice, a.check, a.direction) <
               std::tie(b.startLine, b.lowerVoice, b.upperVoice, b.check, b.direction);
    });
    return findings;
}

std::vector<Finding> findVoiceRangeViolations(const HumdrumChorale& chorale, VoiceRangeSet ranges) {
    const VoiceRanges& limits = voiceRanges(ranges);
    std::vector<Finding> findings;
    for (std::size_t voice = 1; voice <= kVoiceCount; ++voice) {
        const hum::HTp start = chorale.spine("kern", voice);
        if (!start) continue;
        const VoiceRange& range = limits[voice - 1];

        for (hum::HTp token = start->getNextToken(); token; token = token->getNextToken()) {
            if (!token->getOwner()->isData() || token->isNull() || token->isSecondaryTiedNote()) continue;

            // A rest has no pitch, which humlib reports as a number below the lowest note.
            const int midi = hum::Convert::kernToMidiNoteNumber(std::string(*token));
            if (midi <= 0 || (midi >= range.lower && midi <= range.upper)) continue;

            Finding finding;
            finding.check = "voiceRange";
            finding.severity = "warning";
            finding.direction = midi > range.upper ? "above" : "below";
            finding.lowerVoice = voice;
            finding.upperVoice = voice;
            finding.startLine = token->getLineNumber();
            finding.endLine = finding.startLine;
            finding.startPosition = humNumToString(token->getDurationFromStart());
            finding.endPosition = finding.startPosition;
            findings.push_back(std::move(finding));
        }
    }
    return findings;
}

std::vector<Finding> runChecks(const HumdrumChorale& chorale, VoiceRangeSet ranges) {
    std::vector<Finding> findings = findParallelMotion(chorale);
    for (Finding& finding : findVoiceRangeViolations(chorale, ranges)) findings.push_back(std::move(finding));
    std::stable_sort(findings.begin(), findings.end(), [](const Finding& a, const Finding& b) {
        return a.startLine < b.startLine;
    });
    return findings;
}

} // namespace choralesearch

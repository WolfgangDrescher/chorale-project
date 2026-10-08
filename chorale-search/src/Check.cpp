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

// A kind of perfect interval: the types reported for it as a parallel and as a hidden one, and the
// intervals it is made of. Compound ones are folded by the query's hintReduceCompound, so "P5" also
// stands for the twelfth. A unison counts as an octave -- the two voices sing the same note.
struct PerfectInterval {
    const char* parallelType;
    const char* hiddenType;
    std::vector<std::string> intervals;
};

const std::vector<PerfectInterval>& perfectIntervals() {
    static const std::vector<PerfectInterval> kinds = {
        {"parallelFifths", "hiddenFifths", {"P5"}},
        {"parallelOctaves", "hiddenOctaves", {"P8", "P1"}},
    };
    return kinds;
}

// A query for one kind of finding, and what a match of it says.
struct FindingQuery {
    Query query;
    const char* type;
    const char* severity;
    const char* direction;
    std::size_t lowerVoice;
    std::size_t upperVoice;
};

// The two directions of a move, with the sign mint writes them with.
const std::vector<std::pair<const char*, std::string>>& directions() {
    static const std::vector<std::pair<const char*, std::string>> all = {{"up", "+"}, {"down", "-"}};
    return all;
}

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
std::vector<FindingQuery> buildParallelQueries() {
    std::vector<FindingQuery> queries;
    for (const PerfectInterval& kind : perfectIntervals()) {
        for (const auto& [direction, sign] : directions()) {
            for (std::size_t lower = 1; lower <= kVoiceCount; ++lower) {
                for (std::size_t upper = lower + 1; upper <= kVoiceCount; ++upper) {
                    // The interval to the lower voice, from the walked upper voice's point of view.
                    const std::string intervalKey = "hint-" + std::to_string(lower);
                    AttributeMap interval;
                    interval[intervalKey] = kind.intervals;

                    Query query;
                    query.id = queryId(kind.parallelType, direction, lower, upper);
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

                    queries.push_back({std::move(query), kind.parallelType, "error", direction, lower, upper});
                }
            }
        }
    }
    return queries;
}

// A hidden fifth or octave: the soprano moves into a perfect interval to the bass that it was not
// already in, the bass moving the same way, as in the parallels. The soprano has to leap, unless
// `allowStepwiseSoprano` is off. Neither of the two notes before may be a rest.
std::vector<FindingQuery> buildHiddenMotionQueries(bool allowStepwiseSoprano) {
    constexpr std::size_t kBass = 1;
    constexpr std::size_t kSoprano = kVoiceCount;
    std::vector<FindingQuery> queries;
    for (const PerfectInterval& kind : perfectIntervals()) {
        for (const auto& [direction, sign] : directions()) {
            const AttributeMap noRest = {{"!kern", {"r"}}};

            AttributeMap before = noRest;
            before["!hint-1"] = kind.intervals;
            AttributeMap into;
            into["hint-1"] = kind.intervals;
            if (allowStepwiseSoprano) into["!mint"] = {sign + "2", sign + "1"};

            Query query;
            query.id = queryId(kind.hiddenType, direction, kBass, kSoprano);
            query.feature = "mint";
            query.voices = std::to_string(kSoprano);
            query.pattern = movePattern(sign, before, into);
            query.hintReduceCompound = true;
            query.simultaneousAlignment = "start-end";

            SimultaneousGroup bass;
            bass.feature = "mint";
            bass.voices = std::to_string(kBass);
            bass.pattern = movePattern(sign, noRest, {});
            query.simultaneousWith.push_back(std::move(bass));

            queries.push_back({std::move(query), kind.hiddenType, "warning", direction, kBass, kSoprano});
        }
    }
    return queries;
}

// The findings of the queries in the chorale, in the order of the score and, at the same place, by voices
// and kind, so the result doesn't depend on the order the queries run in.
std::vector<Finding> runFindingQueries(const HumdrumChorale& chorale, const std::vector<FindingQuery>& queries) {
    // Only needed for runOne, which searches the chorale it is given and nothing on disk.
    const CorpusSearch search(chorale.path());

    std::vector<Finding> findings;
    for (const FindingQuery& entry : queries) {
        for (const Result& match : search.runOne(chorale, entry.query)) {
            Finding finding;
            finding.check = entry.type;
            finding.severity = entry.severity;
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

    std::stable_sort(findings.begin(), findings.end(), [](const Finding& a, const Finding& b) {
        return std::tie(a.startLine, a.lowerVoice, a.upperVoice, a.check, a.direction) <
               std::tie(b.startLine, b.lowerVoice, b.upperVoice, b.check, b.direction);
    });
    return findings;
}

} // namespace

std::vector<Finding> findParallelMotion(const HumdrumChorale& chorale) {
    return runFindingQueries(chorale, buildParallelQueries());
}

std::vector<Finding> findHiddenMotion(const HumdrumChorale& chorale, bool allowStepwiseSoprano) {
    return runFindingQueries(chorale, buildHiddenMotionQueries(allowStepwiseSoprano));
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

std::vector<Finding> runChecks(const HumdrumChorale& chorale, VoiceRangeSet ranges, bool allowStepwiseHiddenMotion) {
    std::vector<Finding> findings = findParallelMotion(chorale);
    for (Finding& finding : findHiddenMotion(chorale, allowStepwiseHiddenMotion)) findings.push_back(std::move(finding));
    for (Finding& finding : findVoiceRangeViolations(chorale, ranges)) findings.push_back(std::move(finding));
    std::stable_sort(findings.begin(), findings.end(), [](const Finding& a, const Finding& b) {
        return a.startLine < b.startLine;
    });
    return findings;
}

} // namespace choralesearch

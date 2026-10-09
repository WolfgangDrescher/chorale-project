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
                    interval[intervalKey] = toPatternValues(kind.intervals);

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
            before["!hint-1"] = toPatternValues(kind.intervals);
            AttributeMap into;
            into["hint-1"] = toPatternValues(kind.intervals);
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

// The bass note under a fermata that is no cadence step from the note before it, one query for each way
// to get there: up, down, or on the same note. mint measures from the last sounding note, so a rest in
// between is looked through, and mintStartAtPreviousToken starts the finding on the onset before.
std::vector<FindingQuery> buildBassPhraseEndQueries() {
    constexpr std::size_t kBass = 1;
    // The perfect fourths and fifths, up and down. Compounds count (hintReduceCompound): a twelfth is a fifth.
    const std::vector<std::string> cadenceSteps = {"+P4", "+P5", "-P4", "-P5"};

    std::vector<FindingQuery> queries;
    for (const auto& [direction, move] : std::vector<std::pair<const char*, std::string>>{{"up", "+"}, {"down", "-"}, {"", "P1"}}) {
        AttributeMap position;
        position["mint"] = {move};
        position["fermata"] = {"true"};
        position["!mint"] = toPatternValues(cadenceSteps);

        Query query;
        query.id = queryId("bassPhraseEnd", direction, kBass, kBass);
        query.feature = "mint";
        query.voices = std::to_string(kBass);
        query.pattern = {position};
        query.mintStartAtPreviousToken = true;
        query.hintReduceCompound = true;
        queries.push_back({std::move(query), "bassPhraseEnd", "warning", direction, kBass, kBass});
    }
    return queries;
}

// The leaps that are not allowed, one query for each voice and direction: the note after a note that is
// neither a rest nor under a fermata (the next note opens a phrase) lies further away than a fifth. The
// octave is allowed, and so is the minor sixth upwards. mint looks from the last sounding note, but a rest
// in between ends the position before it, so no leap across a rest is found.
std::vector<FindingQuery> buildLargeLeapQueries() {
    std::vector<FindingQuery> queries;
    for (std::size_t voice = 1; voice <= kVoiceCount; ++voice) {
        for (const auto& [direction, sign] : directions()) {
            AttributeMap before;
            before["!kern"] = {"r"};
            before["!fermata"] = {"true"};

            AttributeMap leap;
            leap["mint"] = {PatternValue(ComparisonOperator::GreaterThan, sign + "P5")};
            leap["!mint"] = sign == "+" ? std::vector<PatternValue>{"+m6", "+P8"} : std::vector<PatternValue>{"-P8"};

            Query query;
            query.id = queryId("largeLeap", direction, voice, voice);
            query.feature = "mint";
            query.voices = std::to_string(voice);
            query.pattern = {before, leap};

            queries.push_back({std::move(query), "largeLeap", "warning", direction, voice, voice});
        }
    }
    return queries;
}

// The notes outside the range of their voice, one query for each voice and side: a note that is lower than
// the lowest note of the voice, or higher than its highest one. A rest has no pitch to compare.
std::vector<FindingQuery> buildVoiceRangeQueries(const VoiceRanges& limits) {
    std::vector<FindingQuery> queries;
    for (std::size_t voice = 1; voice <= kVoiceCount; ++voice) {
        const VoiceRange& range = limits[voice - 1];
        const std::vector<std::tuple<const char*, ComparisonOperator, int>> sides = {
            {"above", ComparisonOperator::GreaterThan, range.upper},
            {"below", ComparisonOperator::LessThan, range.lower},
        };
        for (const auto& [direction, comparisonOperator, limit] : sides) {
            Query query;
            query.id = queryId("voiceRange", direction, voice, voice);
            query.feature = "kern";
            query.voices = std::to_string(voice);
            query.pattern = {{{"kern", {PatternValue(comparisonOperator, hum::Convert::base12ToKern(limit))}}}};

            queries.push_back({std::move(query), "voiceRange", "warning", direction, voice, voice});
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

// The tokens of the four voices on every line of the score, [voice][line]; a voice that holds a note has
// a null token there. Empty if a voice is missing.
std::vector<std::vector<hum::HTp>> lineTokensOf(const HumdrumChorale& chorale) {
    std::vector<std::vector<hum::HTp>> lines(kVoiceCount + 1);
    for (std::size_t voice = 1; voice <= kVoiceCount; ++voice) {
        const hum::HTp start = chorale.spine("kern", voice);
        if (!start) return {};
        for (hum::HTp token = start->getNextToken(); token; token = token->getNextToken()) {
            if (token->getOwner()->isData()) lines[voice].push_back(token);
        }
        if (lines[voice].size() != lines[1].size()) return {};
    }
    return lines;
}

// The note a token stands for: itself, or for a null token the note held from before.
hum::HTp soundingNote(hum::HTp token) {
    return token->isNull() ? token->resolveNull() : token;
}

// From one note to another.
Finding makeFinding(const char* check, const char* severity, const char* direction, std::size_t lowerVoice,
                    std::size_t upperVoice, hum::HTp first, hum::HTp last) {
    Finding finding;
    finding.check = check;
    finding.severity = severity;
    finding.direction = direction;
    finding.lowerVoice = lowerVoice;
    finding.upperVoice = upperVoice;
    finding.startLine = first->getLineNumber();
    finding.endLine = last->getLineNumber();
    finding.startPosition = humNumToString(first->getDurationFromStart());
    finding.endPosition = humNumToString(last->getDurationFromStart());
    return finding;
}

// At one line: the token of any voice on it.
Finding makeFinding(const char* check, const char* severity, const char* direction, std::size_t lowerVoice,
                    std::size_t upperVoice, hum::HTp at) {
    return makeFinding(check, severity, direction, lowerVoice, upperVoice, at, at);
}

} // namespace

std::vector<Finding> findParallelMotion(const HumdrumChorale& chorale) {
    return runFindingQueries(chorale, buildParallelQueries());
}

std::vector<Finding> findHiddenMotion(const HumdrumChorale& chorale, bool allowStepwiseSoprano) {
    return runFindingQueries(chorale, buildHiddenMotionQueries(allowStepwiseSoprano));
}

std::vector<Finding> findVoiceCrossings(const HumdrumChorale& chorale) {
    constexpr std::size_t kBass = 1;
    const std::vector<std::vector<hum::HTp>> lines = lineTokensOf(chorale);

    std::vector<Finding> findings;
    if (lines.empty()) return findings;
    std::vector<bool> crossedBefore(kVoiceCount + 1, false);
    for (std::size_t line = 0; line < lines[kBass].size(); ++line) {
        const int bassPitch = hum::Convert::kernToBase40(soundingNote(lines[kBass][line]));
        for (std::size_t voice = kBass + 1; voice <= kVoiceCount; ++voice) {
            const int upperPitch = hum::Convert::kernToBase40(soundingNote(lines[voice][line]));
            const bool crossed = bassPitch > 0 && upperPitch > 0 && upperPitch < bassPitch;
            // Only where it begins: a voice that stays below the bass is one finding, not one for each note.
            if (crossed && !crossedBefore[voice]) {
                findings.push_back(makeFinding("voiceCrossing", "warning", "", kBass, voice, lines[kBass][line]));
            }
            crossedBefore[voice] = crossed;
        }
    }
    return findings;
}

std::vector<Finding> findLargeLeaps(const HumdrumChorale& chorale) {
    return runFindingQueries(chorale, buildLargeLeapQueries());
}

std::vector<Finding> findBassPhraseEndings(const HumdrumChorale& chorale) {
    return runFindingQueries(chorale, buildBassPhraseEndQueries());
}

std::vector<Finding> findVoiceRangeViolations(const HumdrumChorale& chorale, VoiceRangeSet ranges) {
    return runFindingQueries(chorale, buildVoiceRangeQueries(voiceRanges(ranges)));
}

std::vector<Finding> runChecks(const HumdrumChorale& chorale, VoiceRangeSet ranges, bool allowStepwiseHiddenMotion) {
    std::vector<Finding> findings = findParallelMotion(chorale);
    for (Finding& finding : findHiddenMotion(chorale, allowStepwiseHiddenMotion)) findings.push_back(std::move(finding));
    for (Finding& finding : findVoiceCrossings(chorale)) findings.push_back(std::move(finding));
    for (Finding& finding : findLargeLeaps(chorale)) findings.push_back(std::move(finding));
    for (Finding& finding : findBassPhraseEndings(chorale)) findings.push_back(std::move(finding));
    for (Finding& finding : findVoiceRangeViolations(chorale, ranges)) findings.push_back(std::move(finding));
    std::stable_sort(findings.begin(), findings.end(), [](const Finding& a, const Finding& b) {
        return a.startLine < b.startLine;
    });
    return findings;
}

} // namespace choralesearch

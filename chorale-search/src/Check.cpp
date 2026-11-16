#include "Check.hpp"

#include <algorithm>
#include <tuple>
#include <utility>

#include "CorpusSearch.hpp"
#include "HintPairs.hpp"
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

// A query for one kind of finding, and what a match of it says. The voices are those of the query, or 0 and 0 for
// a query over voice pairs, whose matches say which pair they are of.
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

std::string queryId(const char* type, const char* direction) {
    return std::string(type) + ":" + direction;
}

std::string queryId(const char* type, const char* direction, std::size_t lower, std::size_t upper) {
    return std::string(type) + ":" + direction + ":" + std::to_string(lower) + "-" + std::to_string(upper);
}

// The move of a voice into the second sonority of a finding: its note is not a rest, and the next onset of the
// voice, which has to be attacked on the line of the second sonority (a simultaneousWith group that ends there),
// is reached by a move in the given direction. A rest in between is no sonority, the hint spine has no token on
// it, so this is also what keeps a move out of a rest from being taken for one between two consecutive sonorities.
SimultaneousGroup movesInto(const std::string& voices, const std::string& sign, const AttributeMap& moveExtra = {}) {
    AttributeMap before;
    before["!kern"] = {"r"};
    AttributeMap move = moveExtra;
    move["mint"] = {sign};

    return SimultaneousGroup{
        .feature = "mint",
        .voices = voices,
        .pattern = {before, move},
        .simultaneousAlignment = "end",
    };
}

// One query for each direction and interval (2 x 2 = 4), with the id "<check>:<direction>", over all the pairs of
// voices: the hint spines of the pairs are walked one after the other, "hint-**", and a spine has a token on every
// line where both voices sound, so the two positions are two consecutive sonorities, whether the voices attack
// together or one of them holds its note into the second sonority. Both of them form the interval (hint-@pair, the
// interval of the pair searched); both voices of the pair move into the second one the same way, each of them a
// simultaneousWith group. The matches say which pair they are of.
std::vector<FindingQuery> buildParallelQueries() {
    const std::string pairs = "hint-**";
    std::vector<FindingQuery> queries;
    for (const PerfectInterval& kind : perfectIntervals()) {
        for (const auto& [direction, sign] : directions()) {
            AttributeMap sonority;
            sonority[kPairSearchedKey] = toPatternValues(kind.intervals);

            Query query{
                .id = queryId(kind.parallelType, direction),
                .feature = pairs,
                .voices = "@lower", // the pair has one spine, any voice reads it
                .pattern = {sonority, sonority},
                .simultaneousWith = {movesInto("@lower", sign), movesInto("@upper", sign)},
                .hintReduceCompound = true,
            };

            queries.push_back(FindingQuery{
                .query = std::move(query),
                .type = kind.parallelType,
                .severity = "error",
                .direction = direction,
                .lowerVoice = 0,
                .upperVoice = 0,
            });
        }
    }
    return queries;
}

// A hidden fifth or octave: the soprano moves into a perfect interval to the bass that the two were not
// already in, the bass moving the same way, as in the parallels. The soprano has to leap, unless
// `allowStepwiseSoprano` is off. Like the parallels it is found in the hint spine of the outer voices, whose
// token on every line is the sonority: the first position is the sonority before, and both voices move into the
// second one.
std::vector<FindingQuery> buildHiddenMotionQueries(bool allowStepwiseSoprano) {
    constexpr std::size_t kBass = 1;
    constexpr std::size_t kSoprano = kVoiceCount;
    const std::string pairKey = "hint-" + std::to_string(kBass) + std::to_string(kSoprano);
    std::vector<FindingQuery> queries;
    for (const PerfectInterval& kind : perfectIntervals()) {
        for (const auto& [direction, sign] : directions()) {
            AttributeMap before;
            before["!" + pairKey] = toPatternValues(kind.intervals);
            AttributeMap into;
            into[pairKey] = toPatternValues(kind.intervals);

            AttributeMap leap;
            if (allowStepwiseSoprano) leap["!mint"] = {sign + "2", sign + "1"};

            Query query{
                .id = queryId(kind.hiddenType, direction, kBass, kSoprano),
                .feature = pairKey,
                .voices = std::to_string(kSoprano), // the pair has one spine, any voice reads it
                .pattern = {before, into},
                .simultaneousWith = {movesInto(std::to_string(kBass), sign), movesInto(std::to_string(kSoprano), sign, leap)},
                .hintReduceCompound = true,
            };

            queries.push_back(FindingQuery{
                .query = std::move(query),
                .type = kind.hiddenType,
                .severity = "warning",
                .direction = direction,
                .lowerVoice = kBass,
                .upperVoice = kSoprano,
            });
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

        Query query{
            .id = queryId("bassPhraseEnd", direction, kBass, kBass),
            .feature = "mint",
            .voices = std::to_string(kBass),
            .pattern = {position},
            .mintStartAtPreviousToken = true,
            .hintReduceCompound = true,
        };
        queries.push_back(FindingQuery{
            .query = std::move(query),
            .type = "bassPhraseEnd",
            .severity = "warning",
            .direction = direction,
            .lowerVoice = kBass,
            .upperVoice = kBass,
        });
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

            Query query{
                .id = queryId("largeLeap", direction, voice, voice),
                .feature = "mint",
                .voices = std::to_string(voice),
                .pattern = {before, leap},
            };

            queries.push_back(FindingQuery{
                .query = std::move(query),
                .type = "largeLeap",
                .severity = "warning",
                .direction = direction,
                .lowerVoice = voice,
                .upperVoice = voice,
            });
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
            Query query{
                .id = queryId("voiceRange", direction, voice, voice),
                .feature = "kern",
                .voices = std::to_string(voice),
                .pattern = {{{"kern", {PatternValue(comparisonOperator, hum::Convert::base12ToKern(limit))}}}},
            };

            queries.push_back(FindingQuery{
                .query = std::move(query),
                .type = "voiceRange",
                .severity = "warning",
                .direction = direction,
                .lowerVoice = voice,
                .upperVoice = voice,
            });
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
            Finding finding{
                .check = entry.type,
                .severity = entry.severity,
                .direction = entry.direction,
                .lowerVoice = entry.lowerVoice,
                .upperVoice = entry.upperVoice,
                .startLine = static_cast<int>(match.startLineNumber),
                .endLine = static_cast<int>(match.endLineNumber),
                .startPosition = match.startPosition,
                .endPosition = match.endPosition,
            };
            // A query over voice pairs: the pair is the one whose spine the match was found in ("hint-24").
            if (entry.lowerVoice == 0 && match.feature.size() == 7 && match.feature.rfind("hint-", 0) == 0) {
                finding.lowerVoice = static_cast<std::size_t>(match.feature[5] - '0');
                finding.upperVoice = static_cast<std::size_t>(match.feature[6] - '0');
            }
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
    return Finding{
        .check = check,
        .severity = severity,
        .direction = direction,
        .lowerVoice = lowerVoice,
        .upperVoice = upperVoice,
        .startLine = first->getLineNumber(),
        .endLine = last->getLineNumber(),
        .startPosition = humNumToString(first->getDurationFromStart()),
        .endPosition = humNumToString(last->getDurationFromStart()),
    };
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

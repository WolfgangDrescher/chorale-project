#include "AttributeMatcher.hpp"
#include "ChoraleInfo.hpp"
#include "HintPairs.hpp"
#include "HumdrumUtils.hpp"
#include "Intervals.hpp"
#include "PatternKeys.hpp"
#include "QueryValidation.hpp"
#include "ValueScanner.hpp"
#include "VoiceReference.hpp"

#include <algorithm>
#include <cstdlib>
#include <iterator>
#include <map>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <tuple>
#include <utility>

namespace choralesearch {

namespace {

// How Tool_metweight abbreviates a metrically unclassified position -- an onset that falls
// between the beats the meter classifies, which in these chorales is where the diminution sits.
const std::string kUnclassifiedMetweight = "u";

// A concrete pairwise-interval key, e.g. "hint-14" -- exactly "hint-" followed by two characters.
bool isHintPairKey(const std::string& key) {
    return key.rfind("hint-", 0) == 0 && key.size() == 7;
}

// Unlike kern/deg/mint, fb (and each hint-<pair> spine) has a single spine per chorale, not
// one per voice -- so any lookup against it always uses voice 1, regardless of which voice
// is actually being searched.
std::size_t effectiveVoice(const std::string& feature, std::size_t voice) {
    return (feature == kFbKey || isHintPairKey(feature)) ? 1 : voice;
}

// Resolves "hint-<other>" against whichever voice is currently being walked into the
// concrete pair spine name, e.g. voice 3 + "hint-1" -> "hint-13". nullopt if other equals
// the walked voice itself (no such spine -- a voice's interval to itself is meaningless).
std::optional<std::string> resolveHintRelativeKey(const std::string& key, std::size_t voice) {
    std::size_t other = static_cast<std::size_t>(key[5] - '0');
    if (other == voice) return std::nullopt;
    std::size_t lower = std::min(voice, other);
    std::size_t upper = std::max(voice, other);
    return "hint-" + std::to_string(lower) + std::to_string(upper);
}

bool isWildcard(const std::vector<PatternValue>& allowed) {
    return std::any_of(allowed.begin(), allowed.end(), [](const PatternValue& v) { return v.text == "*"; });
}

bool inList(const std::vector<PatternValue>& allowed, const std::string& actual) {
    return std::any_of(allowed.begin(), allowed.end(), [&](const PatternValue& v) { return v.text == actual; });
}

// Whether a list of diatonic numbers (mintAllowIntervalComplementation) names `number`, or "*" for all.
bool containsNumberOrWildcard(const std::vector<std::string>& allowedNumbers, const std::string& number) {
    return std::find(allowedNumbers.begin(), allowedNumbers.end(), "*") != allowedNumbers.end() ||
           std::find(allowedNumbers.begin(), allowedNumbers.end(), number) != allowedNumbers.end();
}

// Whether `actual` stands in the relation `comparisonOperator` names to `threshold`:
// GreaterThan is actual > threshold.
template <typename T>
bool evaluateComparison(ComparisonOperator comparisonOperator, const T& actual, const T& threshold) {
    switch (comparisonOperator) {
        case ComparisonOperator::GreaterThan: return actual > threshold;
        case ComparisonOperator::GreaterThanOrEqual: return actual >= threshold;
        case ComparisonOperator::LessThan: return actual < threshold;
        case ComparisonOperator::LessThanOrEqual: return actual <= threshold;
        case ComparisonOperator::EqualTo: break;
    }
    return actual == threshold;
}

// A "duration" pattern's values against how long the onset sounds. An equal value is judged by
// its recip spelling like it always was; a comparison by the length that spelling stands for,
// so ">4" is "longer than a quarter" and ">4." "longer than a dotted quarter".
bool durationInList(const std::vector<PatternValue>& allowed, hum::HumNum duration) {
    std::string recip;
    for (const PatternValue& v : allowed) {
        if (v.isVoiceReference()) continue; // judged against the other voice, see voiceReferenceInList()
        if (v.hasComparisonOperator()) {
            if (evaluateComparison(v.comparisonOperator, duration, hum::Convert::recipToDuration(v.text))) return true;
            continue;
        }
        if (recip.empty()) recip = hum::Convert::durationToRecip(duration);
        if (v.text == recip) return true;
    }
    return false;
}

// Extracts the pitch+accidental from a **kern token (e.g. "f#" from "8f#L"), ignoring
// rhythm/ties/beams/decoration; returns "r" for a rest. Only the first subtoken of a
// chord is considered (getSubtoken(0) is a no-op when there's no space to split on).
std::string kernToPitch(const std::string& kerndata) {
    std::string subtoken = hum::HumdrumToken(kerndata).getSubtoken(0);
    if (hum::Convert::isKernRest(subtoken)) return "r";
    std::string pitch;
    for (char c : subtoken) {
        if ((c >= 'A' && c <= 'G') || (c >= 'a' && c <= 'g') || c == '#' || c == '-' || c == 'n') {
            pitch += c;
        }
    }
    return pitch;
}

// Whether `text` is one character repeated ("#", "##", "--", "+"). An accidental or alteration
// mixed of both directions ("#-", "+-") alters nothing as a whole, so it is none to search for.
bool isOneCharacterRepeated(const std::string& text) {
    return !text.empty() && text.find_first_not_of(text.front()) == std::string::npos;
}

// A **kern pattern value taken apart: rhythm, pitch-or-rest and fermata, each of them left out
// (empty, false) when the value does not state it. `accid` is an accidental standing alone, on no
// pitch letter, counted like hum::Convert::kernToAccidentalCount(): 1 for "#", -2 for "--". A
// pitch carries its own accidental in `pitch` ("f#"), so `accid` is left out there: not stated on
// its own, which is not the same as no accidental.
struct KernValue {
    std::string rhythm;
    std::string pitch;
    std::optional<int> accid;
    bool fermata;
};

// Splits a **kern pattern value into rhythm, pitch-or-rest or an accidental alone, and/or
// fermata (";") -- any subset, in any combination; an omitted component is a wildcard, not
// "absent" (no ";" doesn't mean "no fermata"). nullopt for anything else -- falls back to literal.
std::optional<KernValue> parseKernValue(const std::string& patternValue) {
    ValueScanner scan(patternValue);
    std::string rhythm = scan.digits();
    if (!rhythm.empty()) rhythm += scan.anyOf(".");

    std::string pitch = scan.anyOf("ABCDEFGabcdefg");
    if (!pitch.empty()) pitch += scan.anyOf("#n-");
    else if (scan.accept('r')) pitch = "r";

    std::optional<int> accid;
    if (pitch.empty()) {
        std::string accidental = scan.anyOf("#-");
        if (!accidental.empty() && !isOneCharacterRepeated(accidental)) return std::nullopt;
        if (!accidental.empty()) accid = hum::Convert::kernToAccidentalCount(accidental);
    }

    bool fermata = scan.accept(';');
    if (!scan.atEnd()) return std::nullopt;
    if (rhythm.empty() && pitch.empty() && !accid && !fermata) return std::nullopt; // e.g. "" -- nothing to match on

    return KernValue{std::move(rhythm), std::move(pitch), accid, fermata};
}

// A **kern pattern value independently checks rhythm, pitch-or-rest, and/or fermata,
// ignoring tie/beam/slur markup. Falls back to a raw literal comparison only for values
// with characters outside those three components (markup spelled out literally).
// `duration` is how long the note actually lasts -- the onset's own tied duration, except where
// a skipped ornament handed its own time over to it (see buildOnsets()). It is the same value
// the "duration" pattern key is judged against, so both stay in step.
bool kernValueMatches(const std::string& patternValue, hum::HTp actualTok, bool ignoreOctave,
                       hum::HumNum duration) {
    std::string actual = (std::string)*actualTok;
    auto parsed = parseKernValue(patternValue);
    if (!parsed) return patternValue == actual;

    const auto& [rhythm, pitch, accid, fermata] = *parsed;
    if (!rhythm.empty() && rhythm != hum::Convert::durationToRecip(duration)) return false;
    if (accid) {
        // An accidental alone: any pitch letter, in any register, altered by exactly as much.
        std::string actualPitch = kernToPitch(actual);
        if (actualPitch.empty() || actualPitch == "r") return false;
        if (hum::Convert::kernToAccidentalCount(actualPitch) != *accid) return false;
    }
    if (!pitch.empty()) {
        std::string actualPitch = kernToPitch(actual);
        bool pitchMatches = ignoreOctave ? hum::Convert::kernToBase40PC(pitch) == hum::Convert::kernToBase40PC(actualPitch)
                                          : pitch == actualPitch;
        if (!pitchMatches) return false;
    }
    if (fermata && !actualTok->hasFermata()) return false;
    return true;
}

// A comparison of the note's pitch (GreaterThan "g" is "higher than g"), judged by sound: the MIDI
// note number, which counts the octave and gives enharmonic spellings the same height, so c# is
// neither higher nor lower than d-. A rest has no pitch to compare, and kernIgnoreOctave has no
// say: a pitch class is neither higher nor lower than another.
bool kernComparisonMatches(const PatternValue& v, hum::HTp actualTok) {
    std::string pitch = kernToPitch(std::string(*actualTok));
    if (pitch.empty() || pitch == "r") return false;
    return evaluateComparison(v.comparisonOperator, hum::Convert::kernToMidiNoteNumber(pitch),
                                hum::Convert::kernToMidiNoteNumber(v.text));
}

bool kernInList(const std::vector<PatternValue>& allowed, hum::HTp actualTok, bool ignoreOctave,
                 hum::HumNum duration) {
    return std::any_of(allowed.begin(), allowed.end(), [&](const PatternValue& v) {
        if (v.isVoiceReference()) return false; // judged against the other voice, see voiceReferenceInList()
        if (v.hasComparisonOperator()) return kernComparisonMatches(v, actualTok);
        return kernValueMatches(v.text, actualTok, ignoreOctave, duration);
    });
}


// A pattern value may omit sign, quality, and/or number to match any of them -- "+2"
// matches "+M2"/"+m2"/..., a bare "2" any sign/quality, a bare "+" any interval at all
// as long as it goes up. Unparseable values (e.g. mint's bracketed "[G]" first-note
// marker) fall back to a literal comparison.
bool mintValueMatches(const std::string& patternValue, const std::string& actual) {
    auto pattern = parseMintValue(patternValue);
    auto value = parseMintValue(actual);
    if (!pattern || !value) return patternValue == actual;

    const auto& [patternSign, patternQuality, patternNumber] = *pattern;
    const auto& [valueSign, valueQuality, valueNumber] = *value;
    if (!patternNumber.empty() && patternNumber != valueNumber) return false;
    if (!patternSign.empty() && patternSign != valueSign) return false;
    if (!patternQuality.empty() && patternQuality != valueQuality) return false;
    return true;
}

// Inverts an interval quality the way complementation does: a major interval's complement is
// minor and vice versa, an augmented one's diminished and vice versa, a perfect one stays
// perfect. Letter by letter, so mint's doubled forms (AA, dd) invert as a whole.
std::string invertMintQuality(const std::string& quality) {
    std::string inverted;
    for (char c : quality) {
        switch (c) {
            case 'M': inverted += 'm'; break;
            case 'm': inverted += 'M'; break;
            case 'A': inverted += 'd'; break;
            case 'd': inverted += 'A'; break;
            default: inverted += c; break; // 'P' stays perfect
        }
    }
    return inverted;
}

} // namespace

// A value that doesn't pin down a direction keeps that openness -- its complement doesn't either.
std::optional<std::string> complementMintValue(const std::string& patternValue) {
    auto parsed = parseMintValue(patternValue);
    if (!parsed) return std::nullopt;
    const auto& [sign, quality, number] = *parsed;
    if (number.size() != 1 || number[0] < '1' || number[0] > '8') return std::nullopt;
    std::string complementSign = sign == "+" ? "-" : (sign == "-" ? "+" : "");
    return complementSign + invertMintQuality(quality) + std::to_string(9 - (number[0] - '0'));
}

namespace {

// True if `patternValue`'s own diatonic number is one the query opted into complementing.
// It's the number as *written in the pattern* that decides, not the complement's: opting in
// "5" lets a pattern's "-5" also match an actual "+4", but not the other way round (that's
// what opting in "4" does), so each direction of the shorthand stays something the query
// asks for explicitly. "*" opts in every number.
bool mintComplementationAllowedFor(const std::vector<std::string>& allowedNumbers, const std::string& patternValue) {
    auto parsed = parseMintValue(patternValue);
    if (!parsed) return false;
    const std::string& number = parsed->number;
    if (number.empty()) return false;
    return containsNumberOrWildcard(allowedNumbers, number);
}

} // namespace

std::string reduceMintValue(const std::string& value) {
    auto parsed = parseMintValue(value);
    if (!parsed) return value;
    const auto& [sign, quality, number] = *parsed;
    if (number.empty()) return value;
    return sign + reduceHintInterval(quality + number);
}

namespace {

// The size of an interval, as far as the pattern tells intervals apart. Without a quality in the
// pattern's value ("6") every interval of that number is as wide as any other, so the steps above
// the unison are compared; with one ("m6") the intervals are told apart by their size in semitones.
// nullopt for a quality the number cannot have, which no interval of the score has either.
std::optional<long> intervalSize(const IntervalParts& interval, bool inSemitones) {
    long diatonicNumber = std::stol(interval.number);
    if (!inSemitones) return diatonicNumber - 1;
    std::optional<int> semitones = intervalSizeInSemitones(interval.quality, static_cast<int>(diatonicNumber));
    if (!semitones) return std::nullopt;
    return *semitones;
}

// Evaluates "actual <operator> threshold" for two intervals (docs/features/mint#direction). The sizes
// are compared, and a direction in the threshold also requires the interval to go that way: GreaterThan
// "-m6" is a descent wider than a minor sixth, and never an ascent or a unison.
bool evaluateIntervalComparison(ComparisonOperator comparisonOperator, const IntervalParts& actual,
                                const IntervalParts& threshold) {
    if (actual.number.empty() || threshold.number.empty()) return false;
    if (!threshold.direction.empty() && actual.direction != threshold.direction) return false;

    bool inSemitones = !threshold.quality.empty();
    std::optional<long> actualSize = intervalSize(actual, inSemitones);
    std::optional<long> thresholdSize = intervalSize(threshold, inSemitones);
    if (!actualSize || !thresholdSize) return false;
    return evaluateComparison(comparisonOperator, *actualSize, *thresholdSize);
}

// The first note of a voice has no interval to compare, and neither does a token that is not one.
bool mintComparisonMatches(const PatternValue& v, const std::string& actual) {
    auto token = parseMintValue(actual);
    auto threshold = parseMintValue(v.text);
    if (!token || !threshold) return false;
    return evaluateIntervalComparison(v.comparisonOperator, *token, *threshold);
}

// Whether one value of a mint pattern, with its operator, holds for the interval token `actual`. `seen` is `actual` as
// hintReduceCompound would have it folded into the octave. An order comparison is about the
// interval as it is in the score: folding would make "wider than an octave" mean nothing, and a
// complement is another size by definition. Only an equal value can be complemented or folded.
bool mintPatternValueMatches(const PatternValue& entry, const std::string& actual, const std::string& seen,
                      const std::vector<std::string>& allowComplementationFor, bool reduceCompound) {
    if (entry.hasComparisonOperator()) return mintComparisonMatches(entry, actual);

    const std::string& v = entry.text;
    if (mintValueMatches(reduceCompound ? reduceMintValue(v) : v, seen)) return true;
    if (allowComplementationFor.empty()) return false; // complementation off (the default)
    if (!mintComplementationAllowedFor(allowComplementationFor, v)) return false;
    auto complement = complementMintValue(v);
    return complement && mintValueMatches(*complement, seen);
}

bool mintInList(const std::vector<PatternValue>& allowed, const std::string& actual,
                 const std::vector<std::string>& allowComplementationFor, bool reduceCompound) {
    const std::string seen = reduceCompound ? reduceMintValue(actual) : actual;
    return std::any_of(allowed.begin(), allowed.end(), [&](const PatternValue& entry) {
        return mintPatternValueMatches(entry, actual, seen, allowComplementationFor, reduceCompound);
    });
}

bool fbIntervalMatches(const std::string& patternValue, const std::string& actual) {
    auto pattern = parseFbValue(patternValue);
    auto value = parseFbValue(actual);
    if (!pattern || !value) return patternValue == actual;

    const auto& [patternQuality, patternFigure] = *pattern;
    const auto& [valueQuality, valueFigure] = *value;
    if (patternFigure != valueFigure) return false;
    if (!patternQuality.empty() && patternQuality != valueQuality) return false;
    return true;
}

std::vector<std::string> splitFbComponents(const std::string& s) {
    std::istringstream iss(s);
    return {std::istream_iterator<std::string>{iss}, std::istream_iterator<std::string>{}};
}

// fb's token is a chord of space-separated intervals (e.g. "m6 m3"); a pattern value
// lists figures (e.g. "6 3") that must all appear somewhere in it, order-independent.
// exactChord requires an exact figure count; otherwise it's a minimum ("2 4" in "2 4 6").
bool fbValueMatches(const std::string& patternValue, const std::string& actual, bool exactChord) {
    std::vector<std::string> patternComponents = splitFbComponents(patternValue);
    std::vector<std::string> actualComponents = splitFbComponents(actual);
    if (exactChord && patternComponents.size() != actualComponents.size()) return false;
    for (const std::string& patternComponent : patternComponents) {
        bool found = std::any_of(actualComponents.begin(), actualComponents.end(),
                                  [&](const std::string& a) { return fbIntervalMatches(patternComponent, a); });
        if (!found) return false;
    }
    return true;
}

bool fbInList(const std::vector<PatternValue>& allowed, const std::string& actual, bool exactChord) {
    return std::any_of(allowed.begin(), allowed.end(),
                        [&](const PatternValue& v) { return fbValueMatches(v.text, actual, exactChord); });
}

} // namespace

// Folds a hint interval's number back within an octave (10 -> 3, 15 -> 8, ...), leaving its
// quality letter untouched; a unison and an octave stay distinct from each other. Mirrors
// Tool_fb's own FiguredBassNumber::getNumberWithinOctave(), which isn't reusable standalone
// outside Tool_fb -- humlib's Convert class has no free-standing equivalent for a bare
// diatonic interval size. Unparsable input is returned as-is (so it simply won't match
// anything afterward, same as any other bad data in this file).
std::string reduceHintInterval(const std::string& value) {
    auto parsed = parseFbValue(value);
    if (!parsed) return value;
    const auto& [quality, figureStr] = *parsed;
    int figure = std::stoi(figureStr);
    int reduced;
    if (figure % 7 == 0) reduced = 7;
    else if (figure % 7 == 1) reduced = (figure == 1) ? 1 : 8;
    else reduced = figure % 7;
    return quality + std::to_string(reduced);
}

bool fbFigureMatchesInterval(const std::string& figure, const std::string& interval) {
    return fbIntervalMatches(figure, reduceHintInterval(interval));
}

namespace {

// A hint-<pair>/hint-<voice> token is always a single interval, never a chord, so this
// compares directly via fbIntervalMatches rather than fb's chord-aware fbValueMatches
// (exactChord doesn't apply to a lone value). reduceCompound folds both sides first, so e.g.
// a pattern of "3" also matches an actual tenth.
bool hintValueMatches(const std::string& patternValue, const std::string& actual, bool reduceCompound) {
    if (!reduceCompound) return fbIntervalMatches(patternValue, actual);
    return fbIntervalMatches(reduceHintInterval(patternValue), reduceHintInterval(actual));
}

// A hint interval is never signed, it is the distance between the two voices. Like a mint interval
// it can be compared by its number ("10") or, with a quality ("M10"), by its size in semitones, as
// it is in the score: hintReduceCompound does not fold it, it is meant for telling a tenth from a third.
bool hintComparisonMatches(const PatternValue& v, const std::string& actual) {
    auto token = parseFbValue(actual);
    auto threshold = parseFbValue(v.text);
    if (!token || !threshold) return false;
    return evaluateIntervalComparison(v.comparisonOperator, {"", token->quality, token->figure},
                                      {"", threshold->quality, threshold->figure});
}

} // namespace

bool hintInList(const std::vector<PatternValue>& allowed, const std::string& actual, bool reduceCompound) {
    return std::any_of(allowed.begin(), allowed.end(), [&](const PatternValue& v) {
        if (v.hasComparisonOperator()) return hintComparisonMatches(v, actual);
        return hintValueMatches(v.text, actual, reduceCompound);
    });
}

namespace {

// A **deg pattern value is a scale degree ("4+", "7", "r") compared literally, or an alteration
// alone ("+", "--"): every degree raised or lowered by exactly as much against the local key, the
// way Tool_deg reckons it (in minor against the natural minor scale, so a leading tone is "7+").
bool degValueMatches(const std::string& patternValue, const std::string& actual) {
    bool alterationAlone = isOneCharacterRepeated(patternValue) && (patternValue.front() == '+' || patternValue.front() == '-');
    if (!alterationAlone) return patternValue == actual;
    if (actual.empty() || actual.front() < '1' || actual.front() > '7') return false; // a rest has no degree
    return actual.substr(1) == patternValue;
}

bool degInList(const std::vector<PatternValue>& allowed, const std::string& actual) {
    return std::any_of(allowed.begin(), allowed.end(),
                        [&](const PatternValue& v) { return degValueMatches(v.text, actual); });
}

// What a note is to the phrases and to the chorale: whether it opens a phrase, ends one, and whether
// it is the first or the last sounding note of the whole chorale.
struct PhraseRoles {
    bool startsPhrase = false;
    bool endsPhrase = false;
    bool startsChorale = false;
    bool endsChorale = false;
};

// The roles of a note on its own: a written note starts where `first` does and ends where `last` does,
// which is the same token unless the note is split into several onsets.
PhraseRoles notePhraseRoles(hum::HTp first, hum::HTp last) {
    return {startsPhrase(first), last->hasFermata(), startsChorale(first), endsChorale(last)};
}

// Whether a "phrase" pattern value ("start"/"end", or "choraleStart"/"choraleEnd" for the first and the
// last note of the chorale) fits a note with these roles. A middle note has none of them: it's asked for
// as "!phrase".
bool phraseInList(const std::vector<PatternValue>& allowed, const PhraseRoles& roles) {
    for (const PatternValue& value : allowed) {
        if ((value.text == "start" && roles.startsPhrase) || (value.text == "end" && roles.endsPhrase) ||
            (value.text == "choraleStart" && roles.startsChorale) || (value.text == "choraleEnd" && roles.endsChorale)) {
            return true;
        }
    }
    return false;
}

// mode and key literally, keyDeg as a deg value.
bool keyContextInList(PatternKeyId id, const std::vector<PatternValue>& allowed, const KeyContext& context) {
    return std::any_of(allowed.begin(), allowed.end(), [&](const PatternValue& value) {
        if (id == PatternKeyId::Mode) return value.text == context.mode;
        if (id == PatternKeyId::Key) return value.text == context.key;
        return degValueMatches(value.text, context.keyDeg);
    });
}

hum::HTp lookupToken(const HumdrumChorale& chorale, std::size_t voice, int lineNumber, const std::string& feature) {
    return chorale.tokenAtLine(feature, voice, lineNumber);
}

// Whether `key` matches `syllable` at the soprano note on `lineNumber`. word and syl are compared normalized,
// their values as globs.
bool lyricsInList(PatternKeyId id, const std::vector<PatternValue>& allowed, const LyricsSyllable& syllable, int lineNumber) {
    if (id == PatternKeyId::Word || id == PatternKeyId::Syl) {
        const std::string& actual = id == PatternKeyId::Word ? syllable.word->normalizedText : syllable.normalizedText;
        return std::any_of(allowed.begin(), allowed.end(), [&](const PatternValue& v) { return matchesLyricsGlob(v.text, actual); });
    } else if (id == PatternKeyId::Sylpos) {
        return inList(allowed, syllablePositionName(syllable.position));
    } else if (id == PatternKeyId::Melisma) {
        // true is any note of a melisma, false a syllable with a note of its own.
        const MelismaRole role = syllable.melismaRoleAtLine(lineNumber);
        return inList(allowed, melismaRoleName(role)) || inList(allowed, role == MelismaRole::None ? "false" : "true");
    }
    return false;
}

// Adds the onset of a word or a syllable of `verse`, from `firstLine` to `lastLine`; on the same notes in
// another verse already, that onset gets the verse instead.
void addLyricsOnset(std::vector<AttributeMatcher::Onset>& onsets, const HumdrumChorale& chorale, std::size_t verse,
                    int firstLine, int lastLine, hum::HumNum duration) {
    const std::size_t soprano = chorale.voiceCount();
    hum::HTp firstTok = chorale.tokenAtLine(kKernKey, soprano, firstLine);
    hum::HTp lastTok = chorale.tokenAtLine(kKernKey, soprano, lastLine);
    if (!firstTok || !lastTok) return;
    for (AttributeMatcher::Onset& onset : onsets) {
        if (onset.token == firstTok && onset.lastToken == lastTok) {
            onset.verses |= VerseSet::of(verse);
            return;
        }
    }
    onsets.push_back({firstTok, duration, std::nullopt, lastTok, VerseSet::of(verse)});
}

void sortByLine(std::vector<AttributeMatcher::Onset>& onsets) {
    std::stable_sort(onsets.begin(), onsets.end(), [](const AttributeMatcher::Onset& a, const AttributeMatcher::Onset& b) {
        return a.token->getLineNumber() < b.token->getLineNumber();
    });
}

// One per syllable of every verse, lasting over all its notes (see Onset::lastToken). The lyrics belong to
// the soprano: another voice has none.
std::vector<AttributeMatcher::Onset> syllableOnsets(const HumdrumChorale& chorale, std::size_t voice) {
    std::vector<AttributeMatcher::Onset> onsets;
    if (voice != chorale.voiceCount()) return onsets;
    for (std::size_t verse = 1; verse <= chorale.verseCount(); ++verse) {
        for (const LyricsWord& word : chorale.lyrics().verse(verse)->words()) {
            for (const LyricsSyllable& syllable : word.syllables) {
                addLyricsOnset(onsets, chorale, verse, syllable.firstLine(), syllable.lastLine(), syllable.end - syllable.start);
            }
        }
    }
    sortByLine(onsets);
    return onsets;
}

// The same per word.
std::vector<AttributeMatcher::Onset> wordOnsets(const HumdrumChorale& chorale, std::size_t voice) {
    std::vector<AttributeMatcher::Onset> onsets;
    if (voice != chorale.voiceCount()) return onsets;
    for (std::size_t verse = 1; verse <= chorale.verseCount(); ++verse) {
        for (const LyricsWord& word : chorale.lyrics().verse(verse)->words()) {
            hum::HumNum duration(0);
            for (const LyricsSyllable& syllable : word.syllables) duration += syllable.end - syllable.start;
            addLyricsOnset(onsets, chorale, verse, word.firstLine(), word.lastLine(), duration);
        }
    }
    sortByLine(onsets);
    return onsets;
}

// The verses a lyrics key matches at the soprano note on `lineNumber`, negation included: "!word": "gott" matches
// a verse where the note has no gott, one without a syllable there too. A chorale without lyrics has no verse,
// so no key matches it, a negated one and the wildcard included.
VerseSet versesMatchingKey(const HumdrumChorale& chorale, const PositionKey& positionKey, int lineNumber) {
    const std::vector<PatternValue>& allowed = positionKey.values;
    VerseSet verses = VerseSet::none();
    for (std::size_t n = 1; n <= chorale.verseCount(); ++n) {
        const LyricsSyllable* syllable = chorale.lyrics().verse(n)->syllableAtLine(lineNumber);
        // The wildcard asks for a syllable to sound: none does at a rest or where the verse has run out of text.
        const bool holds = syllable && (isWildcard(allowed) || lyricsInList(positionKey.patternKey->id, allowed, *syllable, lineNumber));
        if (holds != positionKey.negated) verses |= VerseSet::of(n);
    }
    return verses;
}

// The key of `position` with that id, negated or not; nullptr where the position does not check it.
const PositionKey* findPositionKey(const PatternPosition& position, PatternKeyId id, bool negated) {
    for (const PositionKey& positionKey : position) {
        if (positionKey.is(id) && positionKey.negated == negated) return &positionKey;
    }
    return nullptr;
}

// Whether "duration" holds only exact lengths, which split and merged notes add up to. A comparison or a
// reference describes one whole note: such a position is judged on the single note it lands on.
bool hasOnlyExactDurations(const PatternPosition& position) {
    const PositionKey* duration = findPositionKey(position, PatternKeyId::Duration, false);
    return duration && !isWildcard(duration->values) &&
           std::none_of(duration->values.begin(), duration->values.end(), [](const PatternValue& v) {
               return v.hasComparisonOperator() || v.isVoiceReference();
           });
}

// The value as the score spells what it is compared with, so the comparison is literal: metweight and mode
// by their canonical name, sylpos and melisma by name ("i" is "initial"), word and syl like the lyrics
// (see normalizeLyricsText: "Jesu" finds "jesu"). A wildcard and a comparison stay as they are.
void normalizePatternValue(const PositionKey& positionKey, PatternValue& value) {
    if (value.text == "*" || value.hasComparisonOperator() || value.isVoiceReference()) return;
    if (positionKey.is(PatternKeyId::Metweight)) value.text = normalizeMetweightValue(value.text).value_or(value.text);
    else if (positionKey.is(PatternKeyId::Mode)) value.text = normalizeModeName(value.text).value_or(value.text);
    else if (positionKey.is(PatternKeyId::Sylpos)) value.text = normalizeSylposValue(value.text).value_or(value.text);
    else if (positionKey.is(PatternKeyId::Melisma)) value.text = normalizeMelismaValue(value.text).value_or(value.text);
    else if (positionKey.is(PatternKeyId::Word) || positionKey.is(PatternKeyId::Syl)) value.text = normalizeLyricsText(value.text);
}

// Whether a **mint octave leap may pass for a re-attack: only for a query that opted the
// unison-octave pair itself into complementation, P8 being P1's complement.
bool mintOctaveIsReAttack(const std::vector<std::string>& mintAllowComplementation) {
    return containsNumberOrWildcard(mintAllowComplementation, "1") || containsNumberOrWildcard(mintAllowComplementation, "8");
}

// Whether the onset whose token is `tok` (and whose interval into it is `mint`, already
// measured across any skipped ornament) re-articulates the note `first` started rather than
// moving to a new one: the same pitch for **kern (rhythm and beam/slur markup differ between
// the onsets, so only the pitch is compared), a unison -- or, opted in, an octave -- for
// **mint, which records the step *into* each note, and its own token repeated for every
// other spine.
bool continuesLogicalNote(std::optional<PatternKeyId> drivingFeature, hum::HTp first, hum::HTp tok,
                           const std::string& mint,
                           const std::vector<std::string>& mintAllowComplementation) {
    if (drivingFeature == PatternKeyId::Kern) return kernToPitch(std::string(*first)) == kernToPitch(std::string(*tok));
    if (drivingFeature == PatternKeyId::Mint) {
        if (mintValueMatches("P1", mint)) return true;
        return mintOctaveIsReAttack(mintAllowComplementation) && mintValueMatches("P8", mint);
    }
    return std::string(*first) == std::string(*tok);
}

// The mirror image of continuesLogicalNote's **mint case, for a note the score merged rather
// than split: whether an OR-list of **mint pattern values would be satisfied by a re-attack of
// the note a merged run started on. Such a re-attack is a unison by definition -- or an
// octave, with the same opt-in continuesLogicalNote asks for. Nothing in the score is
// consulted: the re-attack the values describe is precisely what isn't written there.
bool mintReAttackInList(const std::vector<PatternValue>& allowed,
                         const std::vector<std::string>& mintAllowComplementation) {
    return std::any_of(allowed.begin(), allowed.end(), [&](const PatternValue& value) {
        auto matches = [&](const char* interval) {
            return value.hasComparisonOperator() ? mintComparisonMatches(value, interval)
                                             : mintValueMatches(value.text, interval);
        };
        if (matches("P1")) return true;
        return mintOctaveIsReAttack(mintAllowComplementation) && matches("P8");
    });
}

// The same for **kern. A re-attack re-articulates the note the merged run started on, so its
// pitch *is* the merged onset's -- that part is judged exactly as anywhere else, and so is a
// fermata, which the onset either carries or doesn't. Only the rhythm component has to be
// dropped: the onset's duration is the whole merged note's, not this position's share of it.
// Constrain the share with "duration" instead, which is what the run divides the onset up by.
bool kernReAttackInList(const std::vector<PatternValue>& allowed, hum::HTp tok, bool ignoreOctave) {
    // Every value below has had its rhythm component dropped one way or another, so the
    // duration handed on is never actually consulted.
    hum::HumNum duration = tok->getTiedDuration();
    return std::any_of(allowed.begin(), allowed.end(), [&](const PatternValue& value) {
        if (value.isVoiceReference()) return false; // judged against the other voice, see voiceReferenceInList()
        // A comparison is of the pitch, which a re-attack has: the merged note's.
        if (value.hasComparisonOperator()) return kernComparisonMatches(value, tok);
        const std::string& v = value.text;
        auto parsed = parseKernValue(v);
        if (!parsed) return kernValueMatches(v, tok, ignoreOctave, duration); // literal markup -- no rhythm to drop
        const auto& [rhythm, pitch, accid, fermata] = *parsed;
        if (rhythm.empty()) return kernValueMatches(v, tok, ignoreOctave, duration);
        std::string accidental = accid ? std::string(std::abs(*accid), *accid > 0 ? '#' : '-') : "";
        std::string withoutRhythm = pitch + accidental + (fermata ? ";" : "");
        if (withoutRhythm.empty()) return true; // a rhythm and nothing else -- nothing left to judge
        return kernValueMatches(withoutRhythm, tok, ignoreOctave, duration);
    });
}

// The token `spineStart`'s spine has on the line `tok` sits on. The spines run side by side
// down the file, so this is a step sideways along one line -- the line `tok` already knows it
// belongs to -- rather than going by a line number as HumdrumChorale::tokenAtLine() does.
// buildOnsets() asks this of every onset it walks, which that one would make quadratic.
// A null token (the voice holds a note) is nullptr, or with `resolveNull` the note it stands for.
hum::HTp tokenBesideOnLine(hum::HTp tok, hum::HTp spineStart, bool resolveNull = false) {
    if (!tok || !spineStart) return nullptr;
    int track = spineStart->getTrack();
    hum::HLp line = tok->getOwner();
    for (int field = 0; field < line->getFieldCount(); ++field) {
        hum::HTp candidate = line->token(field);
        // The leftmost field of the track, which for a split spine is the branch
        // tokenAtLine() reaches too -- getNextToken() follows the left side of a split.
        if (candidate->getTrack() != track) continue;
        if (!candidate->isNull()) return candidate;
        hum::HTp note = resolveNull ? candidate->resolveNull() : nullptr;
        return note && note->isData() ? note : nullptr;
    }
    return nullptr;
}

// Whether a voice reference ("@2", "@anyBelow", ...) among the pattern values of kern or duration (`id`)
// holds at the line `lineToken` sits on. What the walked voice has there, its note
// `walkedNote` or the `walkedDuration` of its onset, is held against what the referenced voice sounds on
// that line, with the relation of the entry: equal for a plain value, higher/lower or longer/shorter for
// a comparison. A referenced voice that sounds no note there (a rest, no spine) satisfies nothing, and a
// reference that names no voice at all, @below of the bass, holds for neither "any" nor "all" of them.
// The entries that are no voice reference are not looked at, they are for the ...InList functions.
bool voiceReferenceInList(const HumdrumChorale& chorale, std::size_t voice, PatternKeyId id,
                          const std::vector<PatternValue>& allowed, hum::HTp lineToken, hum::HTp walkedNote,
                          hum::HumNum walkedDuration, bool ignoreOctave) {
    for (const PatternValue& entry : allowed) {
        if (!entry.isVoiceReference()) continue;
        auto reference = resolveVoiceReference(entry.text, voice);
        if (!reference || reference->voices.empty()) continue;

        auto holdsFor = [&](std::size_t other) {
            hum::HTp note = tokenBesideOnLine(lineToken, chorale.spine(kKernKey, other), true);
            if (!note) return false;
            if (id == PatternKeyId::Duration) {
                // A tied note is one note, and its duration is counted from where it was struck.
                hum::HTp start = note;
                while (start && start->isSecondaryTiedNote()) start = start->getPreviousNNDT();
                if (!start) return false;
                return evaluateComparison(entry.comparisonOperator, walkedDuration, start->getTiedDuration());
            }
            std::string pitch = kernToPitch(std::string(*note));
            if (pitch.empty() || pitch == "r") return false;
            return kernInList({PatternValue(entry.comparisonOperator, pitch)}, walkedNote, ignoreOctave, walkedDuration);
        };

        const std::vector<std::size_t>& voices = reference->voices;
        bool holds = reference->quantifier == ReferenceQuantifier::All
                         ? std::all_of(voices.begin(), voices.end(), holdsFor)
                         : std::any_of(voices.begin(), voices.end(), holdsFor);
        if (holds) return true;
    }
    return false;
}

} // namespace

// The **mint token Tool_mint would have written for the step from `from` to `to`, had the notes
// in between not been there. Tool_mint::getIntervalToken() spells an interval as quality plus
// diatonic size out of its own getIntervalQuality() table, and that table is
// Convert::base40ToIntervalAbbr()'s -- the same entries down to the "X" for the base-40 slots
// that name no interval at all -- so the public Convert function is used rather than a copy of
// the protected one. Two things about its output differ from **mint's and are fixed up here: it
// writes "p"/"a"/"aa" where **mint writes "P"/"A"/"AA", and it marks a descending interval but
// leaves an ascending one unsigned. (Tool_fb::getIntervalQuality() is a third table, not this
// one: it reads those unnamed slots as doubly diminished.) The chorales are generated with a
// plain Tool_mint, no -a/-c/-d/-l, so the highest note of a chord represents the voice. Empty
// when either side has no pitch to measure from, which leaves the spine's own token in charge.
std::string mintIntervalToken(hum::HTp from, hum::HTp to) {
    auto representativePitch = [](hum::HTp tok) {
        for (int pitch : tok->getBase40PitchesSortHL()) {
            if (pitch != 0) return std::abs(pitch);
        }
        return 0;
    };
    int fromPitch = representativePitch(from);
    int toPitch = representativePitch(to);
    if (fromPitch == 0 || toPitch == 0) return "";

    int interval = toPitch - fromPitch;
    std::string token = hum::Convert::base40ToIntervalAbbr(std::abs(interval));
    for (char& c : token) {
        if (c == 'p') c = 'P';
        else if (c == 'a') c = 'A';
    }
    return (interval > 0 ? "+" : interval < 0 ? "-" : "") + token;
}

namespace {

// A pattern read from JSON has been checked already, with the path to the value in the message; one built in
// code has not, and matching relies on it.
void checkValues(const PositionKey& positionKey, const MatcherOptions& options) {
    const std::string& key = positionKey.rawKey;
    for (const PatternValue& value : positionKey.values) {
        if (value.isVoiceReference()) {
            if (!supportsVoiceReference(key) || !isValidVoiceReference(value.text)) {
                throw std::invalid_argument("'" + key + "' cannot be held against '" + value.text + "'");
            }
            // Split and merged notes add up lengths, which a negated duration cannot be judged by.
            if (positionKey.negated && positionKey.is(PatternKeyId::Duration) &&
                (options.durationAllowSplitNotes || options.durationAllowMergedNotes)) {
                throw std::invalid_argument("'" + key + "' cannot be held against '" + value.text +
                                             "' with durationAllowSplitNotes or durationAllowMergedNotes");
            }
            continue;
        }
        if (!value.hasComparisonOperator()) continue;
        if (!supportsComparison(key) || !isValidComparisonValue(key, value.text)) {
            throw std::invalid_argument("'" + key + "' cannot be compared with '" + value.text + "'");
        }
    }
}

} // namespace

AttributeMatcher::AttributeMatcher(std::string drivingFeature, std::vector<AttributeMap> pattern, MatcherOptions options)
    : m_drivingFeature(std::move(drivingFeature)), m_options(std::move(options)) {
    // An unknown feature finds nothing.
    const PatternKeyLookup driving = findPatternKey(m_drivingFeature);
    if (driving.patternKey && driving.patternKey->driving && !driving.negated) m_drivingFeatureId = driving.patternKey->id;
    // A word or a syllable is matched as a whole, with all its notes: these options have nothing to work on.
    if (isDrivenByLyrics() &&
        (m_options.durationAllowSplitNotes || m_options.durationAllowMergedNotes || m_options.metweightSkipUnclassified)) {
        throw std::invalid_argument("'" + m_drivingFeature + "' cannot be searched with durationAllowSplitNotes, "
                                    "durationAllowMergedNotes or metweightSkipUnclassified");
    }
    m_readsLyrics = isDrivenByLyrics();
    for (const AttributeMap& position : pattern) {
        PatternPosition& patternPosition = m_pattern.emplace_back();
        for (const auto& [rawKey, values] : position) {
            PositionKey& positionKey = patternPosition.emplace_back(PositionKey{findPatternKey(rawKey), values});
            checkValues(positionKey, m_options);
            for (PatternValue& value : positionKey.values) normalizePatternValue(positionKey, value);
            if (positionKey.readsLyrics()) m_readsLyrics = true;
        }
    }
}


std::vector<AttributeMatcher::Onset> AttributeMatcher::buildOnsets(const HumdrumChorale& chorale,
                                                                    std::size_t voice) const {
    if (m_drivingFeatureId == PatternKeyId::Word) return wordOnsets(chorale, voice);
    if (m_drivingFeatureId == PatternKeyId::Syl) return syllableOnsets(chorale, voice);

    std::vector<Onset> onsets;
    hum::HTp drivingStart = chorale.spine(m_drivingFeature, effectiveVoice(m_drivingFeature, voice));
    if (!drivingStart) return onsets;

    std::vector<hum::HTp> tokens;
    for (hum::HTp t = drivingStart->getNextToken(); t; t = t->getNextToken()) {
        if (t->getOwner()->isData() && !t->isNull() && !t->isSecondaryTiedNote()) tokens.push_back(t);
    }
    onsets.reserve(tokens.size());

    if (!m_options.metweightSkipUnclassified) {
        for (hum::HTp tok : tokens) onsets.push_back(Onset{tok, tok->getTiedDuration(), std::nullopt, tok});
        return onsets;
    }

    // What makes an onset an ornament is where the *voice* attacks, so both spines are read for
    // the voice being walked -- the same one "fermata" is looked up in, not the (shared) spine
    // the driving feature may happen to live in.
    hum::HTp kernSpine = chorale.spine(kKernKey, voice);
    hum::HTp metweightSpine = chorale.spine(kMetweightKey, voice);

    // The last kept note that actually sounds: what a skipped ornament's successor measures its
    // interval from, rests being transparent to **mint the same way Tool_mint treats them.
    hum::HTp lastSoundingNote = nullptr;
    bool skippedSincePreviousOnset = false;

    for (hum::HTp tok : tokens) {
        hum::HTp kernTok = tokenBesideOnLine(tok, kernSpine);
        hum::HTp metweightTok = tokenBesideOnLine(tok, metweightSpine);
        bool isSoundingNote = kernTok && !kernTok->isRest();
        // A rest's **metweight is unclassified wherever it falls, so requiring a sounding note
        // is what keeps rests out; the very first onset has no note in front of it to decorate.
        bool ornamental = !onsets.empty() && isSoundingNote && metweightTok &&
                           std::string(*metweightTok) == kUnclassifiedMetweight;

        if (ornamental) {
            onsets.back().duration += tok->getTiedDuration();
            skippedSincePreviousOnset = true;
            continue;
        }

        Onset onset{tok, tok->getTiedDuration(), std::nullopt, tok};
        if (skippedSincePreviousOnset && lastSoundingNote && isSoundingNote) {
            std::string interval = mintIntervalToken(lastSoundingNote, kernTok);
            if (!interval.empty()) onset.mint = interval;
        }
        onsets.push_back(std::move(onset));
        skippedSincePreviousOnset = false;
        if (isSoundingNote) lastSoundingNote = kernTok;
    }
    return onsets;
}

std::optional<bool> AttributeMatcher::matchKey(const HumdrumChorale& chorale, std::size_t voice, const Onset& onset,
                                                const PositionKey& positionKey) const {
    // A "!" prefix negates the whole position's result (De Morgan's over the OR-list:
    // {"!deg": ["3","5"]} means "neither 3 nor 5"), not individual values -- negating single
    // OR-list entries doesn't compose sensibly.
    const bool negate = positionKey.negated;
    const std::string& key = positionKey.baseKey;
    const std::vector<PatternValue>& allowed = positionKey.values;
    hum::HTp tok = onset.token;
    int lineNumber = tok->getLineNumber();
    // fermata and the end of a phrase are judged on the last note of a word or a syllable.
    hum::HTp lastTok = onset.lastToken;

    // Some verse has to match the key here; that one verse matches every key of the pattern is checked once
    // the walk is through, see walkPattern.
    if (positionKey.readsLyrics()) return !versesMatchingKey(chorale, positionKey, lineNumber).empty();

    bool matched;
    if (isWildcard(allowed)) {
        // Any value, but a value: a pair's spine has no token where the other voice rests.
        const bool otherSpine = positionKey.patternKey && positionKey.patternKey->driving && key != m_drivingFeature;
        matched = !otherSpine || lookupToken(chorale, effectiveVoice(key, voice), lineNumber, key) != nullptr;
    } else if (positionKey.is(PatternKeyId::HintPairWildcard)) {
        // "hint-*4"/"hint-1*"/"hint-**": true if ANY pair matching the digit pattern
        // currently satisfies the value, e.g. "is any voice a 10th above the soprano
        // right now" -- not "are all of them".
        std::vector<std::string> pairs = expandHintPairKey(key);
        const std::vector<PatternValue>& allowedRef = allowed;
        matched = std::any_of(pairs.begin(), pairs.end(), [&](const std::string& pairFeature) {
            hum::HTp valTok = lookupToken(chorale, 1, lineNumber, pairFeature);
            return valTok && hintInList(allowedRef, std::string(*valTok), m_options.hintReduceCompound);
        });
    } else if (positionKey.is(PatternKeyId::HintVoice)) {
        // "hint-2": the interval between whichever voice is currently being searched and
        // voice 2 specifically -- both sides fixed, so this stays correct across several
        // pattern positions (unlike wildcarding this digit would, see isHintRelativeKey's
        // comment).
        auto pairFeature = resolveHintRelativeKey(key, voice);
        hum::HTp valTok = pairFeature ? lookupToken(chorale, 1, lineNumber, *pairFeature) : nullptr;
        matched = valTok && hintInList(allowed, std::string(*valTok), m_options.hintReduceCompound);
    } else if (positionKey.is(PatternKeyId::Phrase)) {
        hum::HTp noteTok = lookupToken(chorale, voice, lineNumber, kKernKey);
        hum::HTp lastNoteTok = lookupToken(chorale, voice, lastTok->getLineNumber(), kKernKey);
        if (!noteTok || !lastNoteTok) return std::nullopt;
        matched = phraseInList(allowed, notePhraseRoles(noteTok, lastNoteTok));
    } else if (positionKey.is(PatternKeyId::Mode) || positionKey.is(PatternKeyId::Key) || positionKey.is(PatternKeyId::KeyDeg)) {
        const KeyContext* context = chorale.keyContextAtLine(lineNumber);
        if (!context) return std::nullopt;
        matched = keyContextInList(positionKey.patternKey->id, allowed, *context);
    } else {
        std::string actual;
        hum::HTp kernTok = nullptr;
        if (positionKey.is(PatternKeyId::Duration)) {
            actual = hum::Convert::durationToRecip(onset.duration);
        } else if (positionKey.is(PatternKeyId::Fermata)) {
            hum::HTp fermataTok = lookupToken(chorale, voice, lastTok->getLineNumber(), kKernKey);
            if (!fermataTok) return std::nullopt;
            actual = fermataTok->hasFermata() ? "true" : "false";
        } else if (positionKey.is(PatternKeyId::Mint) && onset.mint) {
            // An ornament was skipped in front of this onset, so the spine's own token states
            // the interval out of that ornament rather than out of the note the pattern is
            // stepping off -- see buildOnsets().
            actual = *onset.mint;
        } else if (key == m_drivingFeature) {
            actual = std::string(*tok);
            kernTok = tok;
        } else {
            hum::HTp valTok = lookupToken(chorale, effectiveVoice(key, voice), lineNumber, key);
            if (!valTok) return std::nullopt;
            actual = std::string(*valTok);
            kernTok = valTok;
        }
        if (positionKey.is(PatternKeyId::Mint)) matched = mintInList(allowed, actual, m_options.mintAllowIntervalComplementation, m_options.hintReduceCompound);
        else if (positionKey.is(PatternKeyId::Fb)) matched = fbInList(allowed, actual, m_options.fbCompareExactChord);
        else if (positionKey.is(PatternKeyId::HintPair)) matched = hintInList(allowed, actual, m_options.hintReduceCompound);
        else if (positionKey.is(PatternKeyId::Kern)) {
            matched = kernInList(allowed, kernTok, m_options.kernIgnoreOctave, onset.duration) ||
                      voiceReferenceInList(chorale, voice, PatternKeyId::Kern, allowed, tok, kernTok, onset.duration, m_options.kernIgnoreOctave);
        } else if (positionKey.is(PatternKeyId::Duration)) {
            matched = durationInList(allowed, onset.duration) ||
                      voiceReferenceInList(chorale, voice, PatternKeyId::Duration, allowed, tok, nullptr, onset.duration, m_options.kernIgnoreOctave);
        }
        else if (positionKey.is(PatternKeyId::Metweight)) matched = inList(allowed, actual);
        else if (positionKey.is(PatternKeyId::Deg)) matched = degInList(allowed, actual);
        else matched = inList(allowed, actual);
    }
    return negate ? !matched : matched;
}

std::optional<std::size_t> AttributeMatcher::matchSplitPosition(const HumdrumChorale& chorale, std::size_t voice,
                                                                 const std::vector<Onset>& onsets,
                                                                 std::size_t onsetIndex,
                                                                 const PatternPosition& position) const {
    const std::vector<PatternValue>& allowedDurations = findPositionKey(position, PatternKeyId::Duration, false)->values;
    if (allowedDurations.empty()) return std::nullopt;

    std::vector<hum::HumNum> targets;
    targets.reserve(allowedDurations.size());
    for (const PatternValue& recip : allowedDurations) targets.push_back(hum::Convert::recipToDuration(recip.text));
    hum::HumNum maxTarget = *std::max_element(targets.begin(), targets.end());

    hum::HumNum sum(0);
    for (std::size_t idx = onsetIndex; idx < onsets.size(); ++idx) {
        const Onset& onset = onsets[idx];
        bool isContinuation = idx != onsetIndex;
        if (isContinuation &&
            !continuesLogicalNote(m_drivingFeatureId, onsets[onsetIndex].token, onset.token,
                                  onset.mint.value_or(std::string(*onset.token)),
                                  m_options.mintAllowIntervalComplementation)) {
            return std::nullopt;
        }

        for (const PositionKey& positionKey : position) {
            // duration is what the run as a whole is being summed towards, and fermata belongs
            // to the note's release -- both are judged once the run closes, below, and so is the
            // phrase, which opens with the run's first onset and ends with its last.
            if (positionKey.is(PatternKeyId::Duration) || positionKey.is(PatternKeyId::Fermata) ||
                positionKey.is(PatternKeyId::Phrase)) continue;
            // The driving feature describes the logical note itself, which continuesLogicalNote
            // has already vouched for; on a continuation onset it says something else entirely
            // (a **mint unison, a re-attack's own rhythm) and isn't re-checked. Every other
            // attribute has to hold for the whole run, not just its first onset.
            if (isContinuation && positionKey.baseKey == m_drivingFeature) continue;
            auto matched = matchKey(chorale, voice, onset, positionKey);
            if (!matched || !*matched) return std::nullopt;
        }

        sum += onset.duration;

        if (std::find(targets.begin(), targets.end(), sum) != targets.end()) {
            for (const PositionKey& positionKey : position) {
                const std::vector<PatternValue>& allowed = positionKey.values;
                if (positionKey.is(PatternKeyId::Fermata)) {
                    auto matched = matchKey(chorale, voice, onset, positionKey);
                    if (!matched || !*matched) return std::nullopt;
                } else if (positionKey.is(PatternKeyId::Phrase)) {
                    hum::HTp first = lookupToken(chorale, voice, onsets[onsetIndex].token->getLineNumber(), kKernKey);
                    hum::HTp last = lookupToken(chorale, voice, onset.token->getLineNumber(), kKernKey);
                    if (!first || !last) return std::nullopt;
                    bool matched = isWildcard(allowed) || phraseInList(allowed, notePhraseRoles(first, last));
                    if (matched == positionKey.negated) return std::nullopt;
                } else if (positionKey.is(PatternKeyId::Duration) && positionKey.negated) {
                    // A negated duration is judged against the summed duration too, so
                    // "!duration" keeps excluding exactly what "duration" would have matched.
                    if (isWildcard(allowed) || durationInList(allowed, sum)) return std::nullopt;
                }
            }
            return idx - onsetIndex + 1;
        }

        if (sum > maxTarget) return std::nullopt;
    }
    return std::nullopt;
}

std::optional<bool> AttributeMatcher::matchReAttackKey(const HumdrumChorale& chorale, std::size_t voice,
                                                        const Onset& onset, const PositionKey& positionKey) const {
    const bool negate = positionKey.negated;
    const std::vector<PatternValue>& allowed = positionKey.values;
    hum::HTp tok = onset.token;

    bool matched;
    if (isWildcard(allowed)) {
        matched = true;
    } else if (positionKey.is(PatternKeyId::Mint)) {
        matched = mintReAttackInList(allowed, m_options.mintAllowIntervalComplementation);
    } else if (positionKey.is(PatternKeyId::Phrase)) {
        // A re-attack inside a note can't open a phrase or the chorale, and closes either only with the note.
        hum::HTp noteTok = m_drivingFeatureId == PatternKeyId::Kern
                               ? tok
                               : lookupToken(chorale, voice, tok->getLineNumber(), kKernKey);
        if (!noteTok) return std::nullopt;
        PhraseRoles roles;
        roles.endsPhrase = noteTok->hasFermata();
        roles.endsChorale = endsChorale(noteTok);
        matched = phraseInList(allowed, roles);
    } else {
        hum::HTp kernTok = m_drivingFeatureId == PatternKeyId::Kern
                                ? tok
                                : lookupToken(chorale, voice, tok->getLineNumber(), kKernKey);
        if (!kernTok) return std::nullopt;
        matched = kernReAttackInList(allowed, kernTok, m_options.kernIgnoreOctave) ||
                  voiceReferenceInList(chorale, voice, PatternKeyId::Kern, allowed, tok, kernTok, kernTok->getTiedDuration(),
                                       m_options.kernIgnoreOctave);
    }
    return negate ? !matched : matched;
}

std::optional<std::size_t> AttributeMatcher::matchMergedPositions(const HumdrumChorale& chorale, std::size_t voice,
                                                                   const Onset& onset, std::size_t patternIndex,
                                                                   hum::HumNum remaining, bool isContinuation) const {
    if (patternIndex >= m_pattern.size()) return std::nullopt;
    const PatternPosition& position = m_pattern[patternIndex];
    // Without a concrete duration there is no share of the onset this position could claim,
    // so the run cannot reach across it.
    if (!hasOnlyExactDurations(position)) return std::nullopt;
    const PositionKey* duration = findPositionKey(position, PatternKeyId::Duration, false);

    for (const PositionKey& positionKey : position) {
        // duration is what the positions of the run are dividing the onset up by, judged below.
        if (positionKey.is(PatternKeyId::Duration)) continue;
        // A later position of the run describes a re-attack the merged onset doesn't have.
        // Two features say something about that re-attack which the onset's own token can't
        // answer, and each is judged against what the re-attack *would* have been instead --
        // see matchReAttackKey. Which feature drives the walk has nothing to do with it:
        // asking for the same repetition has to mean the same thing either way.
        if (isContinuation && (positionKey.is(PatternKeyId::Mint) || positionKey.is(PatternKeyId::Kern) ||
                               positionKey.is(PatternKeyId::Phrase))) {
            auto matched = matchReAttackKey(chorale, voice, onset, positionKey);
            if (!matched || !*matched) return std::nullopt;
            continue;
        }
        // Everything else is talking about this one onset, the run's own position in the
        // pattern included: a re-attack of a note has the note's scale degree, its figured
        // bass, its intervals to the other voices.
        auto matched = matchKey(chorale, voice, onset, positionKey);
        if (!matched || !*matched) return std::nullopt;
    }

    const PositionKey* negatedDuration = findPositionKey(position, PatternKeyId::Duration, true);
    for (const PatternValue& recip : duration->values) {
        hum::HumNum value = hum::Convert::recipToDuration(recip.text);
        if (value > remaining) continue;
        // A negated duration excludes the very share this position would be claiming, so
        // "!duration" keeps excluding exactly what "duration" would have matched.
        if (negatedDuration && (isWildcard(negatedDuration->values) || durationInList(negatedDuration->values, value))) continue;
        if (value == remaining) return 1;
        auto rest = matchMergedPositions(chorale, voice, onset, patternIndex + 1, remaining - value, true);
        if (rest) return *rest + 1;
    }
    return std::nullopt;
}

std::vector<AttributeMatch> AttributeMatcher::findAll(const HumdrumChorale& chorale, std::size_t voice) const {
    std::vector<AttributeMatch> matches;
    if (m_pattern.empty()) return matches;
    std::vector<Onset> onsets = buildOnsets(chorale, voice);
    if (!isDrivenByLyrics()) return walkPattern(chorale, voice, onsets);

    // The next word of a verse is not the next onset of the merged list, so the walk follows one verse at a
    // time; a match found in several verses is kept once, with all of them.
    for (std::size_t n = 1; n <= chorale.verseCount(); ++n) {
        std::vector<Onset> verseOnsets;
        for (const Onset& onset : onsets) {
            if (onset.verses.contains(n)) verseOnsets.push_back(onset);
        }
        for (AttributeMatch& m : walkPattern(chorale, voice, verseOnsets)) {
            if (!m.verses.contains(n)) continue;
            m.verses = VerseSet::of(n);
            const auto same = std::find_if(matches.begin(), matches.end(), [&](const AttributeMatch& found) {
                return found.startLineNumber == m.startLineNumber && found.endLineNumber == m.endLineNumber &&
                       std::equal(found.positionOnsets.begin(), found.positionOnsets.end(), m.positionOnsets.begin(), m.positionOnsets.end(),
                                  [](const PositionOnset& a, const PositionOnset& b) { return a.line == b.line; });
            });
            if (same != matches.end()) same->verses |= VerseSet::of(n);
            else matches.push_back(std::move(m));
        }
    }
    std::stable_sort(matches.begin(), matches.end(), [](const AttributeMatch& a, const AttributeMatch& b) {
        return std::tie(a.startLineNumber, a.endLineNumber) < std::tie(b.startLineNumber, b.endLineNumber);
    });
    return matches;
}

std::vector<AttributeMatch> AttributeMatcher::walkPattern(const HumdrumChorale& chorale, std::size_t voice,
                                                        const std::vector<Onset>& onsets) const {
    std::vector<AttributeMatch> matches;
    std::size_t n = m_pattern.size();
    if (onsets.empty()) return matches;

    // A pattern normally needs one onset per position, but a merged run covers several
    // positions with a single onset (see matchMergedPositions), so with that option on even a
    // single remaining onset can still carry the rest of the pattern.
    std::size_t minOnsets = m_options.durationAllowMergedNotes ? 1 : n;
    if (onsets.size() < minOnsets) return matches;

    bool shiftStartToPreviousToken = false;
    if (m_options.mintStartAtPreviousToken && m_drivingFeatureId == PatternKeyId::Mint) {
        bool firstPositionIsExplicitWildcard = std::any_of(m_pattern[0].begin(), m_pattern[0].end(), [&](const PositionKey& positionKey) {
            return positionKey.rawKey == m_drivingFeature && isWildcard(positionKey.values);
        });
        shiftStartToPreviousToken = !firstPositionIsExplicitWildcard;
    }

    for (std::size_t start = 0; start + minOnsets <= onsets.size(); ++start) {
        // A position may consume more than one onset (matchSplitPosition) and an onset may
        // cover more than one position (matchMergedPositions), so how far the pattern has
        // walked is tracked separately from how many positions it has checked.
        std::size_t idx = start;
        std::size_t offset = 0;
        bool ok = true;
        std::vector<PositionOnset> positionOnsets(n);
        while (offset < n) {
            const PatternPosition& position = m_pattern[offset];
            bool concreteDuration = hasOnlyExactDurations(position);

            if (concreteDuration && m_options.durationAllowSplitNotes) {
                auto consumed = matchSplitPosition(chorale, voice, onsets, idx, position);
                if (consumed) {
                    hum::HumNum duration(0);
                    for (std::size_t i = idx; i < idx + *consumed; ++i) duration += onsets[i].duration;
                    positionOnsets[offset] = {static_cast<std::size_t>(onsets[idx].token->getLineNumber()),
                                              onsets[idx].token->getDurationFromStart(), duration};
                    idx += *consumed;
                    ++offset;
                    continue;
                }
                // With both options on, a position the pattern splits notes for may still be
                // the start of a merged run instead -- the two describe opposite mismatches
                // between pattern and score and never the same one.
                if (!m_options.durationAllowMergedNotes) { ok = false; break; }
            }

            if (idx >= onsets.size()) { ok = false; break; }
            const Onset& onset = onsets[idx];

            if (concreteDuration && m_options.durationAllowMergedNotes) {
                auto consumed = matchMergedPositions(chorale, voice, onset, offset, onset.duration, false);
                if (!consumed) { ok = false; break; }
                for (std::size_t covered = offset; covered < offset + *consumed && covered < n; ++covered) {
                    positionOnsets[covered] = {static_cast<std::size_t>(onset.token->getLineNumber()), onset.token->getDurationFromStart(),
                                               covered == offset ? onset.duration : hum::HumNum(0)};
                }
                offset += *consumed;
                ++idx;
                continue;
            }

            for (const PositionKey& positionKey : position) {
                auto matched = matchKey(chorale, voice, onset, positionKey);
                if (!matched || !*matched) { ok = false; break; }
            }
            if (!ok) break;
            positionOnsets[offset] = {static_cast<std::size_t>(onset.token->getLineNumber()), onset.token->getDurationFromStart(),
                                      onset.duration};
            ++idx;
            ++offset;
        }
        if (!ok) continue;

        // One verse in which every lyrics key of every position holds, else the match is none.
        VerseSet verses = VerseSet::all();
        for (std::size_t p = 0; p < n; ++p) {
            for (const PositionKey& positionKey : m_pattern[p]) {
                if (positionKey.readsLyrics()) verses &= versesMatchingKey(chorale, positionKey, static_cast<int>(positionOnsets[p].line));
            }
        }
        if (verses.empty()) continue;

        // The note the first interval was measured from is the nearest preceding onset that
        // actually sounds -- rests carry a **mint token of their own but are transparent to
        // the interval calculation, so they must be skipped here too. If nothing sounds before
        // the match (the voice starts here, or only rests precede it) there is nothing to shift
        // back to and the match keeps its own first onset.
        hum::HTp startTok = onsets[start].token;
        if (shiftStartToPreviousToken) {
            for (std::size_t i = start; i-- > 0;) {
                if (std::string(*onsets[i].token) !=  "r") {
                    startTok = onsets[i].token;
                    break;
                }
            }
        }

        hum::HTp endTok = onsets[idx - 1].lastToken;
        AttributeMatch m;
        m.voice = voice;
        m.startLineNumber = static_cast<std::size_t>(startTok->getLineNumber());
        m.endLineNumber = static_cast<std::size_t>(endTok->getLineNumber());
        m.startPosition = startTok->getDurationFromStart();
        m.endPosition = endTok->getDurationFromStart();
        m.positionOnsets = std::move(positionOnsets);
        m.verses = verses;
        matches.push_back(std::move(m));
    }
    return matches;
}

} // namespace choralesearch

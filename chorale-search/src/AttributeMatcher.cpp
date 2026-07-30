#include "AttributeMatcher.hpp"

#include <algorithm>
#include <iterator>
#include <optional>
#include <regex>
#include <sstream>
#include <tuple>
#include <utility>

namespace choralesearch {

namespace {

const std::string kDurationKey = "duration";
const std::string kFermataKey = "fermata";
const std::string kKernFeature = "kern";
const std::string kMintFeature = "mint";
const std::string kFbFeature = "fb";
const std::string kMetweightFeature = "metweight";

// The 6 voice-pair suffixes addIntervalPairSpines() (HumdrumChorale.cpp) generates spines
// for: 1=bass, 2=tenor, 3=alto, 4=soprano, lower voice number first.
const std::vector<std::string> kHintPairSuffixes = {"12", "13", "14", "23", "24", "34"};

// A concrete pairwise-interval key, e.g. "hint-14" -- exactly "hint-" followed by two characters.
bool isHintPairKey(const std::string& key) {
    return key.rfind("hint-", 0) == 0 && key.size() == 7;
}

// A pairwise-interval key that uses "*" for one or both digits, e.g. "hint-*4", "hint-1*",
// "hint-**", to mean "any pair matching this digit pattern".
bool isHintPairWildcardKey(const std::string& key) {
    return isHintPairKey(key) && key.find('*') != std::string::npos;
}

// Expands a (possibly wildcarded) pairwise-interval key's two-character suffix against the
// 6 real pairs, e.g. "hint-*4" -> {"hint-14", "hint-24", "hint-34"}. A concrete,
// non-wildcarded key that happens not to be one of the 6 real pairs (e.g. a typo) simply
// expands to nothing.
std::vector<std::string> expandHintPairKey(const std::string& key) {
    std::string suffix = key.substr(5);
    std::vector<std::string> matches;
    for (const std::string& real : kHintPairSuffixes) {
        bool ok = true;
        for (std::size_t i = 0; i < 2; ++i) {
            if (suffix[i] != '*' && suffix[i] != real[i]) { ok = false; break; }
        }
        if (ok) matches.push_back("hint-" + real);
    }
    return matches;
}

// Unlike kern/deg/mint, fb (and each hint-<pair> spine) has a single spine per chorale, not
// one per voice -- so any lookup against it always uses voice 1, regardless of which voice
// is actually being searched.
std::size_t effectiveVoice(const std::string& feature, std::size_t voice) {
    return (feature == kFbFeature || isHintPairKey(feature)) ? 1 : voice;
}

// A single-digit interval key, e.g. "hint-2" -- the interval between whichever voice is
// currently being searched and this specific other voice. Unlike "hint-*4"/etc, the digit
// here is never a wildcard: both sides of the pair need to stay fixed for the whole pattern
// (the walked voice by construction, this digit by being a literal) for a match across
// several positions to actually mean "the same two voices," e.g. genuine parallel motion.
bool isHintRelativeKey(const std::string& key) {
    return key.rfind("hint-", 0) == 0 && key.size() == 6 && key[5] >= '1' && key[5] <= '4';
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

bool isWildcard(const std::vector<std::string>& allowed) {
    return std::find(allowed.begin(), allowed.end(), "*") != allowed.end();
}

bool inList(const std::vector<std::string>& allowed, const std::string& actual) {
    return std::find(allowed.begin(), allowed.end(), actual) != allowed.end();
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

// Splits a **kern pattern value into rhythm, pitch-or-rest, and/or fermata (";") --
// any subset, in any combination; an omitted component is a wildcard, not "absent"
// (no ";" doesn't mean "no fermata"). nullopt for anything else -- falls back to literal.
std::optional<std::tuple<std::string, std::string, bool>> parseKernValue(const std::string& patternValue) {
    static const std::regex re(R"(^(\d+\.*)?([A-Ga-g]+[#n-]*|r)?(;)?$)");
    std::smatch m;
    if (!std::regex_match(patternValue, m, re)) return std::nullopt;

    std::string recip = m[1].str();
    std::string pitch = m[2].str();
    bool fermata = m[3].matched;
    if (recip.empty() && pitch.empty() && !fermata) return std::nullopt; // e.g. "" -- nothing to match on

    return std::make_tuple(recip, pitch, fermata);
}

// A **kern pattern value independently checks rhythm, pitch-or-rest, and/or fermata,
// ignoring tie/beam/slur markup. Falls back to a raw literal comparison only for values
// with characters outside those three components (markup spelled out literally).
bool kernValueMatches(const std::string& patternValue, hum::HTp actualTok, bool ignoreOctave) {
    std::string actual = (std::string)*actualTok;
    auto parsed = parseKernValue(patternValue);
    if (!parsed) return patternValue == actual;

    const auto& [recip, pitch, fermata] = *parsed;
    if (!recip.empty() && recip != hum::Convert::durationToRecip(actualTok->getTiedDuration())) return false;
    if (!pitch.empty()) {
        std::string actualPitch = kernToPitch(actual);
        bool pitchMatches = ignoreOctave ? hum::Convert::kernToBase40PC(pitch) == hum::Convert::kernToBase40PC(actualPitch)
                                          : pitch == actualPitch;
        if (!pitchMatches) return false;
    }
    if (fermata && !actualTok->hasFermata()) return false;
    return true;
}

bool kernInList(const std::vector<std::string>& allowed, hum::HTp actualTok, bool ignoreOctave) {
    return std::any_of(allowed.begin(), allowed.end(), [&](const std::string& v) {
        return kernValueMatches(v, actualTok, ignoreOctave);
    });
}

// Splits a **mint interval token ("+M2", "-m3", ...) into (sign, quality, number).
// Sign, quality, and number each come back empty when the string has none (e.g. a
// bare "2", or a bare "+"), which mintValueMatches() below treats as "matches any".
std::optional<std::tuple<std::string, std::string, std::string>> parseMintValue(const std::string& s) {
    static const std::regex re(R"(^([+-]?)([A-Za-z]*)(\d*)$)");
    std::smatch m;
    if (!std::regex_match(s, m, re)) return std::nullopt;
    return std::make_tuple(m[1].str(), m[2].str(), m[3].str());
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

// The complementary interval of a **mint pattern value: the one filling the rest of the octave
// in the opposite direction, e.g. "-P5" -> "+P4", "+M2" -> "-m7", "5" -> "4". A value that
// doesn't pin down a direction keeps that openness -- its complement doesn't either.
// nullopt when there's nothing to complement: a value without a diatonic number ("+", "M"),
// a compound interval (a 9th and up -- only simple intervals invert within the octave), or an
// unparseable literal (mint's bracketed first-note marker, e.g. "[gg]").
std::optional<std::string> complementMintValue(const std::string& patternValue) {
    auto parsed = parseMintValue(patternValue);
    if (!parsed) return std::nullopt;
    const auto& [sign, quality, number] = *parsed;
    if (number.size() != 1 || number[0] < '1' || number[0] > '8') return std::nullopt;
    std::string complementSign = sign == "+" ? "-" : (sign == "-" ? "+" : "");
    return complementSign + invertMintQuality(quality) + std::to_string(9 - (number[0] - '0'));
}

// True if `patternValue`'s own diatonic number is one the query opted into complementing.
// It's the number as *written in the pattern* that decides, not the complement's: opting in
// "5" lets a pattern's "-5" also match an actual "+4", but not the other way round (that's
// what opting in "4" does), so each direction of the shorthand stays something the query
// asks for explicitly. "*" opts in every number.
bool mintComplementationAllowedFor(const std::vector<std::string>& allowedNumbers, const std::string& patternValue) {
    auto parsed = parseMintValue(patternValue);
    if (!parsed) return false;
    const std::string& number = std::get<2>(*parsed);
    if (number.empty()) return false;
    return isWildcard(allowedNumbers) || inList(allowedNumbers, number);
}

bool mintInList(const std::vector<std::string>& allowed, const std::string& actual,
                 const std::vector<std::string>& allowComplementationFor) {
    return std::any_of(allowed.begin(), allowed.end(), [&](const std::string& v) {
        if (mintValueMatches(v, actual)) return true;
        if (allowComplementationFor.empty()) return false; // complementation off (the default)
        if (!mintComplementationAllowedFor(allowComplementationFor, v)) return false;
        auto complement = complementMintValue(v);
        return complement && mintValueMatches(*complement, actual);
    });
}

// Splits a figured-bass interval ("m6", "P5", ...) into (quality, figure). A bare
// figure like "6" comes back with an empty quality, which fbValueMatches() treats
// as "any quality" for that figure.
std::optional<std::pair<std::string, std::string>> parseFbValue(const std::string& s) {
    static const std::regex re(R"(^([A-Za-z]*)(\d+)$)");
    std::smatch m;
    if (!std::regex_match(s, m, re)) return std::nullopt;
    return std::make_pair(m[1].str(), m[2].str());
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

bool fbInList(const std::vector<std::string>& allowed, const std::string& actual, bool exactChord) {
    return std::any_of(allowed.begin(), allowed.end(),
                        [&](const std::string& v) { return fbValueMatches(v, actual, exactChord); });
}

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

// A hint-<pair>/hint-<voice> token is always a single interval, never a chord, so this
// compares directly via fbIntervalMatches rather than fb's chord-aware fbValueMatches
// (exactChord doesn't apply to a lone value). reduceCompound folds both sides first, so e.g.
// a pattern of "3" also matches an actual tenth.
bool hintValueMatches(const std::string& patternValue, const std::string& actual, bool reduceCompound) {
    if (!reduceCompound) return fbIntervalMatches(patternValue, actual);
    return fbIntervalMatches(reduceHintInterval(patternValue), reduceHintInterval(actual));
}

bool hintInList(const std::vector<std::string>& allowed, const std::string& actual, bool reduceCompound) {
    return std::any_of(allowed.begin(), allowed.end(),
                        [&](const std::string& v) { return hintValueMatches(v, actual, reduceCompound); });
}

// Tool_metweight writes the **metweight spine in abbreviated form ("s"/"hs"/"w"/"u",
// but a pattern value may spell a weight class out as an abbreviation, a full word,
// or a numeric rank
std::string normalizeMetweightValue(const std::string& value) {
    if (value == "s" || value == "strong" || value == "1") return "s";
    if (value == "hs" || value == "half-strong" || value == "2") return "hs";
    if (value == "w" || value == "weak" || value == "3") return "w";
    if (value == "u" || value == "unclassified" || value == "4") return "u";
    return value;
}

bool metweightValueMatches(const std::string& patternValue, const std::string& actual) {
    return normalizeMetweightValue(patternValue) == actual;
}

bool metweightInList(const std::vector<std::string>& allowed, const std::string& actual) {
    return std::any_of(allowed.begin(), allowed.end(),
                        [&](const std::string& v) { return metweightValueMatches(v, actual); });
}

hum::HTp lookupToken(const HumdrumChorale& chorale, std::size_t voice, int lineNumber, const std::string& feature) {
    hum::HTp start = chorale.spine(feature, voice);
    if (!start) return nullptr;
    return findTokenAtLine(start, lineNumber);
}

// Strips the "!" negation prefix (see the pattern loop below). A lone "!" isn't a prefix --
// there'd be nothing left to negate.
bool isNegatedKey(const std::string& rawKey) {
    return rawKey.size() > 1 && rawKey[0] == '!';
}

std::string stripNegationPrefix(const std::string& rawKey) {
    return isNegatedKey(rawKey) ? rawKey.substr(1) : rawKey;
}

// How long the note at this onset actually sounds. tok isn't necessarily **kern (duration can
// be checked against any driving feature), so getTiedDuration() -- **kern-specific -- is only
// safe to call once we know it is one.
hum::HumNum soundingDuration(hum::HTp tok) {
    return tok->isKern() ? tok->getTiedDuration() : tok->getDuration();
}

// Whether a **mint octave leap may pass for a re-attack: only for a query that opted the
// unison-octave pair itself into complementation, P8 being P1's complement.
bool mintOctaveIsReAttack(const std::vector<std::string>& mintAllowComplementation) {
    return isWildcard(mintAllowComplementation) || inList(mintAllowComplementation, "1") ||
           inList(mintAllowComplementation, "8");
}

// Whether `tok` re-articulates the note `first` started rather than moving to a new one: the
// same pitch for **kern (rhythm and beam/slur markup differ between the onsets, so only the
// pitch is compared), a unison -- or, opted in, an octave -- for **mint, which records the
// step *into* each note, and its own token repeated for every other spine.
bool continuesLogicalNote(const std::string& drivingFeature, hum::HTp first, hum::HTp tok,
                           const std::vector<std::string>& mintAllowComplementation) {
    if (drivingFeature == kKernFeature) return kernToPitch(std::string(*first)) == kernToPitch(std::string(*tok));
    if (drivingFeature == kMintFeature) {
        std::string actual = std::string(*tok);
        if (mintValueMatches("P1", actual)) return true;
        return mintOctaveIsReAttack(mintAllowComplementation) && mintValueMatches("P8", actual);
    }
    return std::string(*first) == std::string(*tok);
}

} // namespace

AttributeMatcher::AttributeMatcher(std::string drivingFeature, std::vector<AttributeMap> pattern, MatcherOptions options)
    : m_drivingFeature(std::move(drivingFeature)), m_pattern(std::move(pattern)), m_options(std::move(options)) {}

std::optional<bool> AttributeMatcher::matchKey(const HumdrumChorale& chorale, std::size_t voice, hum::HTp tok,
                                                const std::string& rawKey,
                                                const std::vector<std::string>& allowed) const {
    // A "!" prefix negates the whole position's result (De Morgan's over the OR-list:
    // {"!deg": ["3","5"]} means "neither 3 nor 5"), not individual values -- negating single
    // OR-list entries doesn't compose sensibly.
    bool negate = isNegatedKey(rawKey);
    const std::string key = stripNegationPrefix(rawKey);
    int lineNumber = tok->getLineNumber();

    bool matched;
    if (isWildcard(allowed)) {
        matched = true;
    } else if (isHintPairWildcardKey(key)) {
        // "hint-*4"/"hint-1*"/"hint-**": true if ANY pair matching the digit pattern
        // currently satisfies the value, e.g. "is any voice a 10th above the soprano
        // right now" -- not "are all of them".
        std::vector<std::string> pairs = expandHintPairKey(key);
        const std::vector<std::string>& allowedRef = allowed;
        matched = std::any_of(pairs.begin(), pairs.end(), [&](const std::string& pairFeature) {
            hum::HTp valTok = lookupToken(chorale, 1, lineNumber, pairFeature);
            return valTok && hintInList(allowedRef, std::string(*valTok), m_options.hintReduceCompound);
        });
    } else if (isHintRelativeKey(key)) {
        // "hint-2": the interval between whichever voice is currently being searched and
        // voice 2 specifically -- both sides fixed, so this stays correct across several
        // pattern positions (unlike wildcarding this digit would, see isHintRelativeKey's
        // comment).
        auto pairFeature = resolveHintRelativeKey(key, voice);
        hum::HTp valTok = pairFeature ? lookupToken(chorale, 1, lineNumber, *pairFeature) : nullptr;
        matched = valTok && hintInList(allowed, std::string(*valTok), m_options.hintReduceCompound);
    } else {
        std::string actual;
        hum::HTp kernTok = nullptr;
        if (key == kDurationKey) {
            actual = hum::Convert::durationToRecip(soundingDuration(tok));
        } else if (key == kFermataKey) {
            hum::HTp fermataTok = lookupToken(chorale, voice, lineNumber, kKernFeature);
            if (!fermataTok) return std::nullopt;
            actual = fermataTok->hasFermata() ? "true" : "false";
        } else if (key == m_drivingFeature) {
            actual = std::string(*tok);
            kernTok = tok;
        } else {
            hum::HTp valTok = lookupToken(chorale, effectiveVoice(key, voice), lineNumber, key);
            if (!valTok) return std::nullopt;
            actual = std::string(*valTok);
            kernTok = valTok;
        }
        if (key == kMintFeature) matched = mintInList(allowed, actual, m_options.mintAllowIntervalComplementation);
        else if (key == kFbFeature) matched = fbInList(allowed, actual, m_options.fbCompareExactChord);
        else if (isHintPairKey(key)) matched = hintInList(allowed, actual, m_options.hintReduceCompound);
        else if (key == kKernFeature) matched = kernInList(allowed, kernTok, m_options.kernIgnoreOctave);
        else if (key == kMetweightFeature) matched = metweightInList(allowed, actual);
        else matched = inList(allowed, actual);
    }
    return negate ? !matched : matched;
}

std::optional<std::size_t> AttributeMatcher::matchSplitPosition(const HumdrumChorale& chorale, std::size_t voice,
                                                                 const std::vector<hum::HTp>& onsets,
                                                                 std::size_t onsetIndex,
                                                                 const AttributeMap& position) const {
    const std::vector<std::string>& allowedDurations = position.at(kDurationKey);
    if (allowedDurations.empty()) return std::nullopt;

    std::vector<hum::HumNum> targets;
    targets.reserve(allowedDurations.size());
    for (const std::string& recip : allowedDurations) targets.push_back(hum::Convert::recipToDuration(recip));
    hum::HumNum maxTarget = *std::max_element(targets.begin(), targets.end());

    hum::HumNum sum(0);
    for (std::size_t idx = onsetIndex; idx < onsets.size(); ++idx) {
        hum::HTp tok = onsets[idx];
        bool isContinuation = idx != onsetIndex;
        if (isContinuation && !continuesLogicalNote(m_drivingFeature, onsets[onsetIndex], tok,
                                                    m_options.mintAllowIntervalComplementation)) {
            return std::nullopt;
        }

        for (const auto& [rawKey, allowed] : position) {
            const std::string key = stripNegationPrefix(rawKey);
            // duration is what the run as a whole is being summed towards, and fermata belongs
            // to the note's release -- both are judged once the run closes, below.
            if (key == kDurationKey || key == kFermataKey) continue;
            // The driving feature describes the logical note itself, which continuesLogicalNote
            // has already vouched for; on a continuation onset it says something else entirely
            // (a **mint unison, a re-attack's own rhythm) and isn't re-checked. Every other
            // attribute has to hold for the whole run, not just its first onset.
            if (isContinuation && key == m_drivingFeature) continue;
            auto matched = matchKey(chorale, voice, tok, rawKey, allowed);
            if (!matched || !*matched) return std::nullopt;
        }

        sum += soundingDuration(tok);

        if (std::find(targets.begin(), targets.end(), sum) != targets.end()) {
            std::string sumRecip = hum::Convert::durationToRecip(sum);
            for (const auto& [rawKey, allowed] : position) {
                const std::string key = stripNegationPrefix(rawKey);
                if (key == kFermataKey) {
                    auto matched = matchKey(chorale, voice, tok, rawKey, allowed);
                    if (!matched || !*matched) return std::nullopt;
                } else if (key == kDurationKey && isNegatedKey(rawKey)) {
                    // A negated duration is judged against the summed duration too, so
                    // "!duration" keeps excluding exactly what "duration" would have matched.
                    if (isWildcard(allowed) || inList(allowed, sumRecip)) return std::nullopt;
                }
            }
            return idx - onsetIndex + 1;
        }

        if (sum > maxTarget) return std::nullopt;
    }
    return std::nullopt;
}

std::vector<AttributeMatch> AttributeMatcher::findAll(const HumdrumChorale& chorale, std::size_t voice) const {
    std::vector<AttributeMatch> matches;
    std::size_t n = m_pattern.size();
    if (n == 0) return matches;

    hum::HTp drivingStart = chorale.spine(m_drivingFeature, effectiveVoice(m_drivingFeature, voice));
    if (!drivingStart) return matches;

    std::vector<hum::HTp> onsets;
    hum::HTp t = drivingStart->getNextToken();
    while (t) {
        if (t->getOwner()->isData() && !t->isNull() && !t->isSecondaryTiedNote()) onsets.push_back(t);
        t = t->getNextToken();
    }

    if (onsets.size() < n) return matches;

    bool shiftStartToPreviousToken = false;
    if (m_options.mintStartAtPreviousToken && m_drivingFeature == "mint") {
        auto it = m_pattern[0].find(m_drivingFeature);
        bool firstPositionIsExplicitWildcard = it != m_pattern[0].end() && isWildcard(it->second);
        shiftStartToPreviousToken = !firstPositionIsExplicitWildcard;
    }

    for (std::size_t start = 0; start + n <= onsets.size(); ++start) {
        // A position may consume more than one onset (see matchSplitPosition), so how far the
        // pattern has walked is tracked separately from how many positions it has checked.
        std::size_t idx = start;
        bool ok = true;
        for (std::size_t offset = 0; ok && offset < n; ++offset) {
            const AttributeMap& position = m_pattern[offset];
            auto durationIt = position.find(kDurationKey);
            bool splitPosition = m_options.durationAllowSplitNotes && durationIt != position.end() &&
                                 !isWildcard(durationIt->second);

            if (splitPosition) {
                auto consumed = matchSplitPosition(chorale, voice, onsets, idx, position);
                if (!consumed) { ok = false; break; }
                idx += *consumed;
                continue;
            }

            if (idx >= onsets.size()) { ok = false; break; }
            hum::HTp tok = onsets[idx];
            for (const auto& [rawKey, allowed] : position) {
                auto matched = matchKey(chorale, voice, tok, rawKey, allowed);
                if (!matched || !*matched) { ok = false; break; }
            }
            ++idx;
        }
        if (!ok) continue;

        // The note the first interval was measured from is the nearest preceding onset that
        // actually sounds -- rests carry a **mint token of their own but are transparent to
        // the interval calculation, so they must be skipped here too. If nothing sounds before
        // the match (the voice starts here, or only rests precede it) there is nothing to shift
        // back to and the match keeps its own first onset.
        hum::HTp startTok = onsets[start];
        if (shiftStartToPreviousToken) {
            for (std::size_t i = start; i-- > 0;) {
                if (std::string(*onsets[i]) !=  "r") {
                    startTok = onsets[i];
                    break;
                }
            }
        }

        AttributeMatch m;
        m.voice = voice;
        m.startLineNumber = static_cast<std::size_t>(startTok->getLineNumber());
        m.endLineNumber = static_cast<std::size_t>(onsets[idx - 1]->getLineNumber());
        m.startPosition = startTok->getDurationFromStart();
        m.endPosition = onsets[idx - 1]->getDurationFromStart();
        matches.push_back(std::move(m));
    }
    return matches;
}

} // namespace choralesearch

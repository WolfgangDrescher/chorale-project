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

// Unlike kern/deg/mint, fb has a single spine per chorale (the bass's own figured-bass
// analysis), not one per voice -- so any lookup against it always uses voice 1,
// regardless of which voice is actually being searched.
std::size_t effectiveVoice(const std::string& feature, std::size_t voice) {
    return feature == kFbFeature ? 1 : voice;
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
// Sign and quality come back empty when the string has none (e.g. a bare "2"), which
// mintValueMatches() below treats as "matches any" for that part.
std::optional<std::tuple<std::string, std::string, std::string>> parseMintValue(const std::string& s) {
    static const std::regex re(R"(^([+-]?)([A-Za-z]*)(\d+)$)");
    std::smatch m;
    if (!std::regex_match(s, m, re)) return std::nullopt;
    return std::make_tuple(m[1].str(), m[2].str(), m[3].str());
}

// A pattern value may omit sign and/or quality to match any of them -- "+2" matches
// "+M2"/"+m2"/..., a bare "2" any sign/quality. Unparseable values (e.g. mint's
// bracketed "[G]" first-note marker) fall back to a literal comparison.
bool mintValueMatches(const std::string& patternValue, const std::string& actual) {
    auto pattern = parseMintValue(patternValue);
    auto value = parseMintValue(actual);
    if (!pattern || !value) return patternValue == actual;

    const auto& [patternSign, patternQuality, patternNumber] = *pattern;
    const auto& [valueSign, valueQuality, valueNumber] = *value;
    if (patternNumber != valueNumber) return false;
    if (!patternSign.empty() && patternSign != valueSign) return false;
    if (!patternQuality.empty() && patternQuality != valueQuality) return false;
    return true;
}

bool mintInList(const std::vector<std::string>& allowed, const std::string& actual) {
    return std::any_of(allowed.begin(), allowed.end(), [&](const std::string& v) {
        return mintValueMatches(v, actual);
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

} // namespace

AttributeMatcher::AttributeMatcher(std::string drivingFeature, std::vector<AttributeMap> pattern,
                                    bool mintStartAtPreviousToken, bool fbCompareExactChord,
                                    bool kernIgnoreOctave)
    : m_drivingFeature(std::move(drivingFeature)), m_pattern(std::move(pattern)),
      m_mintStartAtPreviousToken(mintStartAtPreviousToken), m_fbCompareExactChord(fbCompareExactChord),
      m_kernIgnoreOctave(kernIgnoreOctave) {}

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
    if (m_mintStartAtPreviousToken && m_drivingFeature == "mint") {
        auto it = m_pattern[0].find(m_drivingFeature);
        bool firstPositionIsExplicitWildcard = it != m_pattern[0].end() && isWildcard(it->second);
        shiftStartToPreviousToken = !firstPositionIsExplicitWildcard;
    }

    for (std::size_t start = 0; start + n <= onsets.size(); ++start) {
        bool ok = true;
        for (std::size_t offset = 0; ok && offset < n; ++offset) {
            hum::HTp tok = onsets[start + offset];
            int lineNumber = tok->getLineNumber();
            for (const auto& [rawKey, allowed] : m_pattern[offset]) {
                // A "!" prefix negates the whole position's result (De Morgan's over the
                // OR-list: {"!deg": ["3","5"]} means "neither 3 nor 5"), not individual
                // values -- negating single OR-list entries doesn't compose sensibly.
                bool negate = rawKey.size() > 1 && rawKey[0] == '!';
                const std::string key = negate ? rawKey.substr(1) : rawKey;

                bool matched;
                if (isWildcard(allowed)) {
                    matched = true;
                } else {
                    std::string actual;
                    hum::HTp kernTok = nullptr;
                    if (key == kDurationKey) {
                        // tok isn't necessarily **kern here (duration can be checked
                        // against any driving feature), so getTiedDuration() -- **kern-
                        // specific -- is only safe to call once we know it is one.
                        actual = hum::Convert::durationToRecip(tok->isKern() ? tok->getTiedDuration() : tok->getDuration());
                    } else if (key == kFermataKey) {
                        hum::HTp fermataTok = lookupToken(chorale, voice, lineNumber, kKernFeature);
                        if (!fermataTok) { ok = false; break; }
                        actual = fermataTok->hasFermata() ? "true" : "false";
                    } else if (key == m_drivingFeature) {
                        actual = std::string(*tok);
                        kernTok = tok;
                    } else {
                        hum::HTp valTok = lookupToken(chorale, effectiveVoice(key, voice), lineNumber, key);
                        if (!valTok) { ok = false; break; }
                        actual = std::string(*valTok);
                        kernTok = valTok;
                    }
                    if (key == kMintFeature) matched = mintInList(allowed, actual);
                    else if (key == kFbFeature) matched = fbInList(allowed, actual, m_fbCompareExactChord);
                    else if (key == kKernFeature) matched = kernInList(allowed, kernTok, m_kernIgnoreOctave);
                    else if (key == kMetweightFeature) matched = metweightInList(allowed, actual);
                    else matched = inList(allowed, actual);
                }
                if (negate) matched = !matched;
                if (!matched) { ok = false; break; }
            }
        }
        if (!ok) continue;

        hum::HTp startTok = (shiftStartToPreviousToken && start > 0) ? onsets[start - 1] : onsets[start];

        AttributeMatch m;
        m.voice = voice;
        m.startLineNumber = static_cast<std::size_t>(startTok->getLineNumber());
        m.endLineNumber = static_cast<std::size_t>(onsets[start + n - 1]->getLineNumber());
        m.startPosition = startTok->getDurationFromStart();
        m.endPosition = onsets[start + n - 1]->getDurationFromStart();
        matches.push_back(std::move(m));
    }
    return matches;
}

} // namespace choralesearch

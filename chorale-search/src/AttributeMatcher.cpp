#include "AttributeMatcher.hpp"

#include <algorithm>
#include <optional>
#include <regex>
#include <tuple>

namespace choralesearch {

namespace {

const std::string kDurationKey = "duration";
const std::string kFermataKey = "fermata";
const std::string kKernFeature = "kern";
const std::string kMintFeature = "mint";

bool isWildcard(const std::vector<std::string>& allowed) {
    return std::find(allowed.begin(), allowed.end(), "*") != allowed.end();
}

bool inList(const std::vector<std::string>& allowed, const std::string& actual) {
    return std::find(allowed.begin(), allowed.end(), actual) != allowed.end();
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
// "+M2"/"+m2"/"+A2"/..., and a bare "2" matches any sign and quality with that diatonic
// number. Values that aren't parseable as an interval (e.g. mint's own "[G]" bracketed
// pitch name for a voice's very first note) fall back to a literal string comparison.
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

hum::HTp lookupToken(const HumdrumChorale& chorale, std::size_t voice, int lineNumber, const std::string& feature) {
    hum::HTp start = chorale.spine(feature, voice);
    if (!start) return nullptr;
    return findTokenAtLine(start, lineNumber);
}

} // namespace

AttributeMatcher::AttributeMatcher(std::string drivingFeature, std::vector<AttributeMap> pattern,
                                    bool mintStartAtPreviousToken)
    : m_drivingFeature(std::move(drivingFeature)), m_pattern(std::move(pattern)),
      m_mintStartAtPreviousToken(mintStartAtPreviousToken) {}

std::vector<AttributeMatch> AttributeMatcher::findAll(const HumdrumChorale& chorale, std::size_t voice) const {
    std::vector<AttributeMatch> matches;
    std::size_t n = m_pattern.size();
    if (n == 0) return matches;

    hum::HTp drivingStart = chorale.spine(m_drivingFeature, voice);
    if (!drivingStart) return matches;

    std::vector<hum::HTp> onsets;
    hum::HTp t = drivingStart->getNextToken();
    while (t) {
        if (t->getOwner()->isData() && !t->isNull()) onsets.push_back(t);
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
            for (const auto& [key, allowed] : m_pattern[offset]) {
                if (isWildcard(allowed)) continue;

                std::string actual;
                if (key == kDurationKey) {
                    actual = hum::Convert::durationToRecip(tok->getDuration());
                } else if (key == kFermataKey) {
                    hum::HTp kernTok = lookupToken(chorale, voice, lineNumber, kKernFeature);
                    if (!kernTok) { ok = false; break; }
                    actual = kernTok->hasFermata() ? "true" : "false";
                } else if (key == m_drivingFeature) {
                    actual = std::string(*tok);
                } else {
                    hum::HTp valTok = lookupToken(chorale, voice, lineNumber, key);
                    if (!valTok) { ok = false; break; }
                    actual = std::string(*valTok);
                }
                bool matched = (key == kMintFeature) ? mintInList(allowed, actual) : inList(allowed, actual);
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

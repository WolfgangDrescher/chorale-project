#include "AttributeMatcher.hpp"

#include <algorithm>

namespace choralesearch {

namespace {

const std::string kDurationKey = "duration";
const std::string kFermataKey = "fermata";
const std::string kKernFeature = "kern";

bool isWildcard(const std::vector<std::string>& allowed) {
    return std::find(allowed.begin(), allowed.end(), "*") != allowed.end();
}

bool inList(const std::vector<std::string>& allowed, const std::string& actual) {
    return std::find(allowed.begin(), allowed.end(), actual) != allowed.end();
}

hum::HTp lookupToken(const HumdrumChorale& chorale, std::size_t voice, int lineNumber, const std::string& feature) {
    hum::HTp start = chorale.spine(feature, voice);
    if (!start) return nullptr;
    return findTokenAtLine(start, lineNumber);
}

} // namespace

AttributeMatcher::AttributeMatcher(std::string drivingFeature, std::vector<AttributeMap> pattern)
    : m_drivingFeature(std::move(drivingFeature)), m_pattern(std::move(pattern)) {}

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
                if (!inList(allowed, actual)) { ok = false; break; }
            }
        }
        if (!ok) continue;

        AttributeMatch m;
        m.voice = voice;
        m.startLineNumber = static_cast<std::size_t>(onsets[start]->getLineNumber());
        m.endLineNumber = static_cast<std::size_t>(onsets[start + n - 1]->getLineNumber());
        m.startPosition = onsets[start]->getDurationFromStart();
        m.endPosition = onsets[start + n - 1]->getDurationFromStart();
        matches.push_back(std::move(m));
    }
    return matches;
}

} // namespace choralesearch

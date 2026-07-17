#pragma once

#include <string>
#include <vector>

#include "HumdrumChorale.hpp"
#include "Query.hpp"

namespace choralesearch {

struct AttributeMatch {
    std::size_t voice;           // 1-indexed voice (spine) the match was found in
    std::size_t startLineNumber; // 1-indexed line in the compiled Humdrum file; 0 = not set
    std::size_t endLineNumber;   // same, for the last matched line number
    hum::HumNum startPosition;   // musical position (quarter notes from the start of the piece getDurationFromStart)
    hum::HumNum endPosition;     // same, for the last matched position without the duration of that slice
};

class AttributeMatcher {
public:
    AttributeMatcher(std::string drivingFeature, std::vector<AttributeMap> pattern);

    std::vector<AttributeMatch> findAll(const HumdrumChorale& chorale, std::size_t voice) const;

private:
    std::string m_drivingFeature;
    std::vector<AttributeMap> m_pattern;
};

} // namespace choralesearch

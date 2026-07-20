#pragma once

#include <ostream>
#include <string>
#include <vector>

namespace choralesearch {

struct Result {
    std::string choraleId;           // e.g. "chor029"
    std::string feature;             // e.g. "deg", "mint"
    std::string voiceLabel;          // e.g. "Soprano"
    std::size_t voice = 0;           // 1-indexed voice (spine); 0 = not set (voices are always 1-4; where 1 is Bass)
    std::string startPosition;       // musical position (quarter notes from the start of the piece getDurationFromStart)
    std::string endPosition;         // same, for the last matched position without the duration of that slice
    std::size_t startLineNumber = 0; // 1-indexed line in the compiled Humdrum file; 0 = not set
    std::size_t endLineNumber = 0;   // same, for the last matched line number
};

using Results = std::vector<Result>;

inline std::ostream& operator<<(std::ostream& os, const Result& r) {
    return os << r.choraleId << '\t' << r.feature << '\t' << r.voiceLabel << '\t' << r.startPosition << '\t' << r.endPosition;
}

inline std::ostream& operator<<(std::ostream& os, const Results& results) {
    for (const auto& r : results)
        os << r << '\n';
    return os;
}

} // namespace choralesearch

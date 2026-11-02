#include "KeyContext.hpp"

#include <array>
#include <cstdlib>
#include <map>
#include <regex>

#include "ChoraleInfo.hpp"
#include "humlib.h"

namespace choralesearch {

std::string keyScaleDegree(const std::string& tonic, const std::string& referenceKey, const std::string& referenceMode) {
    // Semitones of the seven degrees above the tonic.
    static const std::map<std::string, std::array<int, 7>> kScales = {
        {"major",      {0, 2, 4, 5, 7, 9, 11}},
        {"minor",      {0, 2, 3, 5, 7, 8, 10}},
        {"dorian",     {0, 2, 3, 5, 7, 9, 10}},
        {"phrygian",   {0, 1, 3, 5, 7, 8, 10}},
        {"lydian",     {0, 2, 4, 6, 7, 9, 11}},
        {"mixolydian", {0, 2, 4, 5, 7, 9, 10}},
        {"locrian",    {0, 1, 3, 5, 6, 8, 10}},
    };
    // The letters give the degree, so enharmonic spellings stay apart.
    const int degree = (hum::Convert::kernToDiatonicPC(tonic) - hum::Convert::kernToDiatonicPC(referenceKey) + 7) % 7;
    const int semitones = (hum::Convert::kernToBase12PC(tonic) - hum::Convert::kernToBase12PC(referenceKey) + 12) % 12;
    int alteration = (semitones - kScales.at(referenceMode)[degree] + 18) % 12 - 6;

    std::string text = std::to_string(degree + 1);
    text += std::string(static_cast<std::size_t>(std::abs(alteration)), alteration < 0 ? '-' : '+');
    return text;
}

std::optional<KeyContext> keyContextFromDesignation(const std::string& token) {
    static const std::regex designationRe(R"(^\*([a-gA-G][#-]*):([a-z]*)$)");
    std::smatch m;
    if (!std::regex_match(token, m, designationRe)) return std::nullopt;
    KeyContext context;
    context.key = m[1].str();
    context.mode = modeFromKeyDesignation(context.key, m[2].str());
    return context;
}

} // namespace choralesearch

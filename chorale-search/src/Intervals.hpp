#pragma once

#include <optional>
#include <string>

namespace choralesearch {

// A melodic interval as a mint token or a pattern value writes it: [direction][quality]number, each
// part possibly empty ("2", "+", "-M2").
struct IntervalParts {
    std::string direction; // "+", "-" or empty
    std::string quality;   // "M", "P", ... or empty
    std::string number;    // the diatonic number
};

std::optional<IntervalParts> parseMintValue(const std::string& s);

// One figure of fb or a hint interval: [quality]number, "m6", "6".
struct FbValue {
    std::string quality; // "M", "P", ... or empty
    std::string figure;  // the interval number
};

std::optional<FbValue> parseFbValue(const std::string& s);

} // namespace choralesearch

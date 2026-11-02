#pragma once

#include <optional>
#include <string>

namespace choralesearch {

// The key a passage is in, as its key designation states it.
struct KeyContext {
    // "G", "e", "B-"
    std::string key;
    // As normalizeModeName() spells it.
    std::string mode;
    // The degree of the tonic in the key of the first note, as **deg writes it: E minor in G major is "6".
    std::string keyDeg;
};

// The degree of `tonic` in `referenceKey` with `referenceMode`, as **deg writes it ("6", "3-").
std::string keyScaleDegree(const std::string& tonic, const std::string& referenceKey, const std::string& referenceMode);

// The key and the mode of a key designation ("*e:", "*f:dor"), nullopt for any other token. keyDeg stays empty.
std::optional<KeyContext> keyContextFromDesignation(const std::string& token);

} // namespace choralesearch

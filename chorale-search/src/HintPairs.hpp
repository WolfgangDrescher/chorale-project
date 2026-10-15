#pragma once

#include <string>
#include <vector>

namespace choralesearch {

// The 6 voice-pair spines addIntervalPairSpines() (HumdrumChorale.cpp) generates -- always the same set for every
// chorale, never discovered per file: 1=bass, 2=tenor, 3=alto, 4=soprano, the lower voice number first.
inline const std::vector<std::string>& hintPairFeatureNames() {
    static const std::vector<std::string> names = {"hint-12", "hint-13", "hint-14", "hint-23", "hint-24", "hint-34"};
    return names;
}

// A pairwise-interval key that uses "*" for one or both digits, e.g. "hint-*4", "hint-1*",
// "hint-**", to mean "any pair matching this digit pattern".
inline bool isHintPairWildcardKey(const std::string& key) {
    return key.rfind("hint-", 0) == 0 && key.size() == 7 && key.find('*') != std::string::npos;
}

// Expands a (possibly wildcarded) pairwise-interval key's two-character suffix against the
// 6 real pairs, e.g. "hint-*4" -> {"hint-14", "hint-24", "hint-34"}. A concrete,
// non-wildcarded key that happens not to be one of the 6 real pairs (e.g. a typo) simply
// expands to nothing, and so does a wildcard no pair matches, e.g. "hint-4*".
inline std::vector<std::string> expandHintPairKey(const std::string& key) {
    std::vector<std::string> matches;
    for (const std::string& real : hintPairFeatureNames()) {
        bool ok = true;
        for (std::size_t i = 5; i < 7; ++i) {
            if (key[i] != '*' && key[i] != real[i]) { ok = false; break; }
        }
        if (ok) matches.push_back(real);
    }
    return matches;
}

// The pattern key for the interval of the pair a query over voice pairs is searching ("hint-**" as feature), as
// "@lower" and "@upper" name its voices.
inline const std::string kPairSearchedKey = "hint-@pair";

// Whether `key`, possibly negated ("!hint-@pair"), is kPairSearchedKey.
inline bool isPairSearchedKey(const std::string& key) {
    return key.substr(!key.empty() && key[0] == '!' ? 1 : 0) == kPairSearchedKey;
}

} // namespace choralesearch

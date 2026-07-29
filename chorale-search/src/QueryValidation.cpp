#include "QueryValidation.hpp"

#include <algorithm>
#include <iterator>
#include <regex>
#include <sstream>

namespace choralesearch {

namespace {

// Fields shared by Query and SimultaneousGroup (see Query.hpp / JsonIO.cpp's
// parseSearchRequestFields). "id", "limit", and "simultaneousWith" are Query-only additions on
// top of this, handled by isKnownQueryKey below.
const std::vector<std::string> kSharedRequestFieldNames = {
    "feature", "voices", "pattern", "mintStartAtPreviousToken", "mintAllowIntervalComplementation",
    "fbCompareExactChord", "kernIgnoreOctave", "hintReduceCompound", "simultaneousAlignment",
};

// The 6 voice-pair spines addIntervalPairSpines() (HumdrumChorale.cpp) generates -- always the
// same set for every chorale, never discovered per file.
const std::vector<std::string> kHintPairFeatureNames = {
    "hint-12", "hint-13", "hint-14", "hint-23", "hint-24", "hint-34",
};

std::vector<std::string> buildDrivingFeatureNames() {
    std::vector<std::string> names = {"kern", "deg", "mint", "fb", "metweight"};
    names.insert(names.end(), kHintPairFeatureNames.begin(), kHintPairFeatureNames.end());
    return names;
}

// "hint-2" -- the interval between whichever voice is walked and voice 2 specifically. Mirrors
// AttributeMatcher.cpp's isHintRelativeKey (kept independent since that one operates on
// already-trusted, already-matched keys, this one is validating untrusted input up front).
bool isHintRelativeKey(const std::string& key) {
    return key.rfind("hint-", 0) == 0 && key.size() == 6 && key[5] >= '1' && key[5] <= '4';
}

// "hint-*4"/"hint-1*"/"hint-**" -- each of the 2 suffix characters is either '*' or a real
// voice number, with at least one '*' (a fully concrete pair, e.g. "hint-14", is checked
// against drivingFeatureNames() instead, since only the 6 real pairs are meaningful there).
bool isHintWildcardKey(const std::string& key) {
    if (key.rfind("hint-", 0) != 0 || key.size() != 7) return false;
    bool hasStar = false;
    for (std::size_t i = 5; i < key.size(); ++i) {
        if (key[i] == '*') { hasStar = true; continue; }
        if (key[i] < '1' || key[i] > '4') return false;
    }
    return hasStar;
}

bool isHintFlavoredKey(const std::string& key) {
    return isHintRelativeKey(key) || isHintWildcardKey(key) ||
           std::find(kHintPairFeatureNames.begin(), kHintPairFeatureNames.end(), key) != kHintPairFeatureNames.end();
}

bool isValidDegValue(const std::string& v) {
    static const std::regex re(R"(^([1-7][+-]*|r)$)");
    return std::regex_match(v, re);
}

bool isValidFermataValue(const std::string& v) {
    return v == "true" || v == "false";
}

bool isValidMetweightValue(const std::string& v) {
    static const std::vector<std::string> known = {
        "s", "hs", "w", "u", "strong", "half-strong", "weak", "unclassified", "1", "2", "3", "4",
    };
    return std::find(known.begin(), known.end(), v) != known.end();
}

// Mirrors AttributeMatcher.cpp's parseMintValue regex -- sign/quality/number each optional,
// but not all three empty at once -- plus the literal bracketed first-note-of-voice marker
// (e.g. "[gg]", see docs/features/mint#token-format), the only other value mint ever compares.
bool isValidMintValue(const std::string& v) {
    static const std::regex intervalRe(R"(^([+-]?)([A-Za-z]*)(\d*)$)");
    static const std::regex bracketRe(R"(^\[[A-Ga-g]+[#n-]*\]$)");
    std::smatch m;
    if (std::regex_match(v, m, intervalRe)) {
        return !(m[1].str().empty() && m[2].str().empty() && m[3].str().empty());
    }
    return std::regex_match(v, bracketRe);
}

// A single quality-letters+figure component, e.g. "m6" or "6". Mirrors AttributeMatcher.cpp's
// parseFbValue regex -- used both for a single hint-flavored value and, chord-split, for fb.
bool isValidFbComponentValue(const std::string& v) {
    static const std::regex re(R"(^[A-Za-z]*\d+$)");
    return std::regex_match(v, re);
}

bool isValidFbValue(const std::string& v) {
    std::istringstream iss(v);
    std::vector<std::string> components{std::istream_iterator<std::string>{iss}, std::istream_iterator<std::string>{}};
    if (components.empty()) return false;
    return std::all_of(components.begin(), components.end(), isValidFbComponentValue);
}

bool isValidDurationValue(const std::string& v) {
    static const std::regex re(R"(^\d+(%\d+)?\.*$)");
    return std::regex_match(v, re);
}

// Strips a leading '!' negation prefix (see AttributeMatcher.cpp / docs/patterns#negating-a-feature)
// if present. A lone "!" isn't treated as a prefix -- there'd be nothing left to negate --
// matching AttributeMatcher.cpp's own negate-detection.
std::string stripNegationPrefix(const std::string& key) {
    return (key.size() > 1 && key[0] == '!') ? key.substr(1) : key;
}

} // namespace

bool isKnownSimultaneousGroupKey(const std::string& key) {
    return std::find(kSharedRequestFieldNames.begin(), kSharedRequestFieldNames.end(), key) != kSharedRequestFieldNames.end();
}

bool isKnownQueryKey(const std::string& key) {
    return isKnownSimultaneousGroupKey(key) || key == "id" || key == "limit" || key == "simultaneousWith";
}

const std::vector<std::string>& drivingFeatureNames() {
    static const std::vector<std::string> names = buildDrivingFeatureNames();
    return names;
}

bool isKnownDrivingFeature(const std::string& feature) {
    const auto& names = drivingFeatureNames();
    return std::find(names.begin(), names.end(), feature) != names.end();
}

bool isKnownPatternKey(const std::string& rawKey) {
    std::string key = stripNegationPrefix(rawKey);
    return isKnownDrivingFeature(key) || key == "duration" || key == "fermata" ||
           isHintRelativeKey(key) || isHintWildcardKey(key);
}

bool isValidMintComplementationValue(const std::string& value) {
    // Only simple intervals have a complement within the octave (see AttributeMatcher.cpp's
    // complementMintValue), so anything beyond a single 1-8 digit is a mistake worth naming
    // rather than a silently ineffective entry.
    return value == "*" || (value.size() == 1 && value[0] >= '1' && value[0] <= '8');
}

bool isValidPatternValue(const std::string& rawKey, const std::string& value) {
    std::string key = stripNegationPrefix(rawKey);
    if (key == "kern") return true; // any string is legitimate -- see kern.md's literal fallback
    if (key == "deg") return isValidDegValue(value);
    if (key == "fermata") return isValidFermataValue(value);
    if (key == "metweight") return isValidMetweightValue(value);
    if (key == "mint") return isValidMintValue(value);
    if (key == "fb") return isValidFbValue(value);
    if (key == "duration") return isValidDurationValue(value);
    if (isHintFlavoredKey(key)) return isValidFbComponentValue(value); // a single interval, never a chord
    return true; // unreachable once isKnownPatternKey has already checked the key
}

} // namespace choralesearch

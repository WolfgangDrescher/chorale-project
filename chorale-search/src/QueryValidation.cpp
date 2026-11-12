#include "QueryValidation.hpp"
#include "HintPairs.hpp"
#include "PatternKeys.hpp"

#include <algorithm>
#include <optional>
#include <regex>

namespace choralesearch {

namespace {

// Fields shared by Query and SimultaneousGroup (see Query.hpp / JsonIO.cpp's
// parseSearchRequestFields). "id", "limit", "choraleFilter" and "simultaneousWith" are Query-only additions on
// top of this, handled by isKnownQueryKey below.
const std::vector<std::string> kSharedRequestFieldNames = {
    "feature", "voices", "pattern", "everyPosition", "mintStartAtPreviousToken", "mintAllowIntervalComplementation",
    "fbCompareExactChord", "kernIgnoreOctave", "hintReduceCompound", "durationAllowSplitNotes",
    "durationAllowMergedNotes", "metweightSkipUnclassified", "simultaneousAlignment",
};

std::vector<std::string> buildDrivingFeatureNames() {
    std::vector<std::string> names;
    for (const PatternKey& patternKey : patternKeys()) {
        if (!patternKey.driving) continue;
        if (patternKey.id == PatternKeyId::HintPair) names.insert(names.end(), hintPairFeatureNames().begin(), hintPairFeatureNames().end());
        else names.push_back(patternKey.name);
    }
    return names;
}

} // namespace

bool isKnownSimultaneousGroupKey(const std::string& key) {
    return std::find(kSharedRequestFieldNames.begin(), kSharedRequestFieldNames.end(), key) != kSharedRequestFieldNames.end();
}

bool isKnownQueryKey(const std::string& key) {
    return isKnownSimultaneousGroupKey(key) || key == "id" || key == "limit" || key == "choraleFilter" || key == "simultaneousWith";
}

const std::vector<std::string>& drivingFeatureNames() {
    static const std::vector<std::string> names = buildDrivingFeatureNames();
    return names;
}

bool isKnownDrivingFeature(const std::string& feature) {
    const PatternKeyLookup lookup = findPatternKey(feature);
    return lookup.patternKey && !lookup.patternKeyPart && lookup.patternKey->driving && !lookup.negated;
}

bool isLyricsDrivingFeature(const std::string& feature) {
    const PatternKeyLookup lookup = findPatternKey(feature);
    return lookup.patternKey && !lookup.patternKeyPart && lookup.patternKey->driving && lookup.patternKey->perVerse &&
           !lookup.negated;
}

bool isLyricsPatternKey(const std::string& rawKey) {
    const PatternKeyLookup lookup = findPatternKey(rawKey);
    return lookup.patternKey && lookup.patternKey->perVerse;
}

bool isPairWildcardFeature(const std::string& feature) {
    const PatternKeyLookup lookup = findPatternKey(feature);
    return lookup.patternKey && !lookup.patternKeyPart && lookup.patternKey->id == PatternKeyId::HintPairWildcard && !lookup.negated &&
           !expandHintPairKey(feature).empty();
}

bool isKnownPatternKey(const std::string& rawKey) {
    return rawKey == kPositionNameKey || findPatternKey(rawKey).patternKey != nullptr;
}

bool isValidPositionName(const std::string& text) {
    static const std::regex re(R"(^\$[A-Za-z]\w*$)");
    return std::regex_match(text, re);
}

std::optional<int> intervalSizeInSemitones(const std::string& quality, int number) {
    if (number < 1) return std::nullopt;
    static const int kMajorOrPerfectSemitones[7] = {0, 2, 4, 5, 7, 9, 11};
    const int degree = (number - 1) % 7;
    const int octaves = (number - 1) / 7;
    // A unison, fourth and fifth (and their octaves) are perfect; the rest are major or minor.
    const bool perfectClass = degree == 0 || degree == 3 || degree == 4;
    const int base = kMajorOrPerfectSemitones[degree] + 12 * octaves;

    if (quality == "P") return perfectClass ? std::optional<int>(base) : std::nullopt;
    if (quality == "M") return perfectClass ? std::nullopt : std::optional<int>(base);
    if (quality == "m") return perfectClass ? std::nullopt : std::optional<int>(base - 1);

    // Augmented raises either class by a semitone per letter; diminished lowers a perfect interval
    // by one per letter, and a major one by one more than that, past the minor.
    const auto allOf = [&](char c) { return !quality.empty() && quality.find_first_not_of(c) == std::string::npos; };
    const int letters = static_cast<int>(quality.size());
    if (letters > 3) return std::nullopt;
    if (allOf('A')) return base + letters;
    if (allOf('d')) return perfectClass ? base - letters : base - letters - 1;
    return std::nullopt;
}

bool isValidIntervalQuality(const std::string& quality, int number) {
    return intervalSizeInSemitones(quality, number).has_value();
}

bool supportsComparison(const std::string& rawKey) {
    const PatternKeyLookup lookup = findPatternKey(rawKey);
    return lookup.patternKey && lookup.valueSpec().comparable;
}

bool supportsVoiceReference(const std::string& rawKey) {
    const PatternKeyLookup lookup = findPatternKey(rawKey);
    return lookup.patternKey && lookup.valueSpec().voiceReference;
}

bool isValidComparisonValue(const std::string& rawKey, const std::string& value) {
    const PatternKeyLookup lookup = findPatternKey(rawKey);
    if (!lookup.patternKey || !lookup.valueSpec().isValidComparisonValue) return false;
    return lookup.valueSpec().isValidComparisonValue(value);
}

bool isValidMintComplementationValue(const std::string& value) {
    // Only simple intervals have a complement within the octave (see AttributeMatcher.cpp's
    // complementMintValue), so anything beyond a single 1-8 digit is a mistake worth naming
    // rather than a silently ineffective entry.
    return value == "*" || (value.size() == 1 && value[0] >= '1' && value[0] <= '8');
}

bool isValidPatternValue(const std::string& rawKey, const std::string& value) {
    if (rawKey == kPositionNameKey) return isValidPositionName(value);
    const PatternKeyLookup lookup = findPatternKey(rawKey);
    if (!lookup.patternKey) return true; // unreachable once isKnownPatternKey has already checked the key
    return lookup.valueSpec().isValidValue(value);
}

} // namespace choralesearch

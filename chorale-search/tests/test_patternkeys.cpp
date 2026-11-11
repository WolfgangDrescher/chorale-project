#include "test_framework.hpp"

#include <optional>
#include <string>
#include <vector>

#include "PatternKeys.hpp"
#include "QueryValidation.hpp"

using choralesearch::drivingFeatureNames;
using choralesearch::findPatternKey;
using choralesearch::PatternKey;
using choralesearch::PatternKeyId;
using choralesearch::patternKeys;

namespace {

// The PatternKey a key of the query finds, or nullptr.
const PatternKey* patternKeyOf(const std::string& rawKey) {
    return findPatternKey(rawKey).patternKey;
}

} // namespace

TEST_CASE(every_key_checks_its_values_and_is_comparable_exactly_with_a_check_for_its_comparison_value) {
    for (const PatternKey& patternKey : patternKeys()) {
        CHECK(patternKey.valueSpec.isValidValue != nullptr);
        CHECK_EQ(patternKey.valueSpec.comparable, patternKey.valueSpec.isValidComparisonValue != nullptr);
    }
}

TEST_CASE(find_pattern_key_finds_a_key_by_its_name_and_takes_its_negation_off) {
    REQUIRE(patternKeyOf("mint") != nullptr);
    CHECK(patternKeyOf("mint")->id == PatternKeyId::Mint);
    CHECK(!findPatternKey("mint").negated);
    CHECK(patternKeyOf("!mint") == patternKeyOf("mint"));
    CHECK(findPatternKey("!mint").negated);
    for (const char* unknown : {"", "!", "!!mint", "tempo", "Mint", "mint "}) CHECK(patternKeyOf(unknown) == nullptr);
}

TEST_CASE(find_pattern_key_finds_the_hint_keys_a_placeholder_covers) {
    auto idOf = [](const std::string& key) {
        const PatternKey* patternKey = patternKeyOf(key);
        REQUIRE(patternKey != nullptr);
        return patternKey->id;
    };
    for (const char* key : {"hint-12", "hint-14", "hint-34"}) CHECK(idOf(key) == PatternKeyId::HintPair);
    for (const char* key : {"hint-1", "hint-4"}) CHECK(idOf(key) == PatternKeyId::HintVoice);
    for (const char* key : {"hint-*4", "hint-1*", "hint-**", "hint-4*"}) CHECK(idOf(key) == PatternKeyId::HintPairWildcard);
    CHECK(idOf("hint-@pair") == PatternKeyId::HintPairCurrent);
    CHECK(idOf("!hint-@pair") == PatternKeyId::HintPairCurrent);
    for (const char* key : {"hint-", "hint-5", "hint-15", "hint-21", "hint-*5", "hint-123", "hint-@"}) CHECK(patternKeyOf(key) == nullptr);
}

TEST_CASE(the_driving_features_are_the_driving_keys_with_the_pairs_spelled_out) {
    CHECK_EQ(drivingFeatureNames(), (std::vector<std::string>{"kern", "deg", "mint", "fb", "metweight", "word", "syl",
                                                              "hint-12", "hint-13", "hint-14", "hint-23", "hint-24", "hint-34"}));
}

TEST_CASE(only_the_lyrics_are_read_verse_by_verse) {
    std::vector<std::string> perVerse;
    for (const PatternKey& patternKey : patternKeys()) {
        if (patternKey.perVerse) perVerse.push_back(patternKey.name);
    }
    CHECK_EQ(perVerse, (std::vector<std::string>{"word", "syl", "sylpos", "melisma"}));
}

TEST_CASE(an_interval_has_a_quality_its_number_can_have) {
    const auto isValid = [](const std::string& key, const std::string& value) {
        return findPatternKey(key).patternKey->valueSpec.isValidValue(value);
    };
    for (const char* value : {"m6", "6", "P5", "AA4", "ddd7", "M10"}) CHECK(isValid("hint-14", value));
    for (const char* value : {"x6", "P3", "M5", "AAAA4", "Ad4", "06", "1000", "m"}) CHECK(!isValid("hint-14", value));
    CHECK(isValid("fb", "m6 P5 m3"));
    CHECK(!isValid("fb", "m6 P3"));
    for (const char* value : {"+M2", "-2", "+", "M", "P1", "[gg]"}) CHECK(isValid("mint", value));
    for (const char* value : {"", "+P2", "x2", "+-2"}) CHECK(!isValid("mint", value));
}

TEST_CASE(a_value_is_normalized_to_how_the_score_spells_it) {
    CHECK_EQ(choralesearch::normalizeMetweightValue("strong"), std::optional<std::string>("s"));
    CHECK_EQ(choralesearch::normalizeMetweightValue("2"), std::optional<std::string>("hs"));
    CHECK(!choralesearch::normalizeMetweightValue("5"));
    CHECK_EQ(choralesearch::normalizeSylposValue("i"), std::optional<std::string>("initial"));
    CHECK(!choralesearch::normalizeSylposValue("n"));
    CHECK_EQ(choralesearch::normalizeMelismaValue("n"), std::optional<std::string>("none"));
    CHECK_EQ(choralesearch::normalizeMelismaValue("true"), std::optional<std::string>("true"));
    CHECK(!choralesearch::normalizeMelismaValue("s"));
}

TEST_MAIN()

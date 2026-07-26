#include "test_framework.hpp"

#include "QueryValidation.hpp"

using choralesearch::drivingFeatureNames;
using choralesearch::isKnownDrivingFeature;
using choralesearch::isKnownPatternKey;
using choralesearch::isKnownQueryKey;
using choralesearch::isKnownSimultaneousGroupKey;
using choralesearch::isValidPatternValue;

TEST_CASE(is_known_simultaneous_group_key_accepts_every_shared_field) {
    for (const std::string& key : {"feature", "voices", "pattern", "mintStartAtPreviousToken", "fbCompareExactChord",
                                    "kernIgnoreOctave", "hintReduceCompound", "simultaneousAlignment"}) {
        CHECK(isKnownSimultaneousGroupKey(key));
    }
}

TEST_CASE(is_known_simultaneous_group_key_rejects_query_only_fields) {
    CHECK(!isKnownSimultaneousGroupKey("id"));
    CHECK(!isKnownSimultaneousGroupKey("limit"));
    CHECK(!isKnownSimultaneousGroupKey("simultaneousWith"));
}

TEST_CASE(is_known_simultaneous_group_key_rejects_unknown_fields) {
    CHECK(!isKnownSimultaneousGroupKey("hintReduceCompund"));
    CHECK(!isKnownSimultaneousGroupKey(""));
}

TEST_CASE(is_known_query_key_accepts_every_shared_field_plus_the_query_only_ones) {
    for (const std::string& key : {"feature", "voices", "pattern", "mintStartAtPreviousToken", "fbCompareExactChord",
                                    "kernIgnoreOctave", "hintReduceCompound", "simultaneousAlignment", "id", "limit",
                                    "simultaneousWith"}) {
        CHECK(isKnownQueryKey(key));
    }
}

TEST_CASE(is_known_query_key_rejects_unknown_fields) {
    CHECK(!isKnownQueryKey("unknown"));
    CHECK(!isKnownQueryKey(""));
}

TEST_CASE(driving_feature_names_lists_exactly_the_known_spines) {
    std::vector<std::string> expected = {
        "kern", "deg", "mint", "fb", "metweight", "hint-12", "hint-13", "hint-14", "hint-23", "hint-24", "hint-34",
    };
    CHECK_EQ(drivingFeatureNames(), expected);
}

TEST_CASE(is_known_driving_feature_accepts_every_real_spine) {
    for (const std::string& feature : drivingFeatureNames()) {
        CHECK(isKnownDrivingFeature(feature));
    }
}

TEST_CASE(is_known_driving_feature_rejects_duration_and_fermata) {
    CHECK(!isKnownDrivingFeature("duration"));
    CHECK(!isKnownDrivingFeature("fermata"));
}

TEST_CASE(is_known_driving_feature_rejects_the_hint_voice_relative_form) {
    CHECK(!isKnownDrivingFeature("hint-1"));
    CHECK(!isKnownDrivingFeature("hint-2"));
}

TEST_CASE(is_known_driving_feature_rejects_a_non_real_hint_pair) {
    CHECK(!isKnownDrivingFeature("hint-11"));
    CHECK(!isKnownDrivingFeature("hint-99"));
}

TEST_CASE(is_known_driving_feature_rejects_unknown_names) {
    CHECK(!isKnownDrivingFeature("baritone"));
    CHECK(!isKnownDrivingFeature(""));
}

TEST_CASE(is_known_pattern_key_accepts_every_driving_feature) {
    for (const std::string& feature : drivingFeatureNames()) {
        CHECK(isKnownPatternKey(feature));
    }
}

TEST_CASE(is_known_pattern_key_accepts_duration_and_fermata) {
    CHECK(isKnownPatternKey("duration"));
    CHECK(isKnownPatternKey("fermata"));
}

TEST_CASE(is_known_pattern_key_accepts_hint_voice_relative_forms) {
    CHECK(isKnownPatternKey("hint-1"));
    CHECK(isKnownPatternKey("hint-2"));
    CHECK(isKnownPatternKey("hint-3"));
    CHECK(isKnownPatternKey("hint-4"));
}

TEST_CASE(is_known_pattern_key_rejects_hint_voice_relative_forms_out_of_range) {
    CHECK(!isKnownPatternKey("hint-0"));
    CHECK(!isKnownPatternKey("hint-5"));
}

TEST_CASE(is_known_pattern_key_accepts_hint_pair_wildcard_forms) {
    CHECK(isKnownPatternKey("hint-*4"));
    CHECK(isKnownPatternKey("hint-1*"));
    CHECK(isKnownPatternKey("hint-**"));
}

TEST_CASE(is_known_pattern_key_rejects_a_hint_wildcard_with_a_non_voice_digit) {
    CHECK(!isKnownPatternKey("hint-*9"));
    CHECK(!isKnownPatternKey("hint-59"));
}

TEST_CASE(is_known_pattern_key_rejects_a_non_real_concrete_hint_pair) {
    CHECK(!isKnownPatternKey("hint-11"));
    CHECK(!isKnownPatternKey("hint-99"));
}

TEST_CASE(is_known_pattern_key_rejects_unknown_keys) {
    CHECK(!isKnownPatternKey("durration"));
    CHECK(!isKnownPatternKey("baritone"));
    CHECK(!isKnownPatternKey(""));
    CHECK(!isKnownPatternKey("hint-"));
    CHECK(!isKnownPatternKey("hint-123"));
}

TEST_CASE(is_known_pattern_key_accepts_a_negated_key) {
    // Negation (see docs/patterns#negating-a-feature) doesn't change which keys are known --
    // "!deg" is exactly as known as "deg".
    CHECK(isKnownPatternKey("!deg"));
    CHECK(isKnownPatternKey("!fermata"));
    CHECK(isKnownPatternKey("!hint-2"));
    CHECK(isKnownPatternKey("!hint-*4"));
}

TEST_CASE(is_known_pattern_key_rejects_a_negated_unknown_key) {
    CHECK(!isKnownPatternKey("!durration"));
    CHECK(!isKnownPatternKey("!baritone"));
}

TEST_CASE(is_known_pattern_key_treats_a_lone_exclamation_mark_as_the_key_itself) {
    // Mirrors AttributeMatcher.cpp: a single "!" has nothing left to negate, so it's not
    // stripped -- and "!" alone isn't a known key either way.
    CHECK(!isKnownPatternKey("!"));
}

TEST_CASE(is_valid_pattern_value_for_kern_accepts_any_string) {
    // See kern.md's documented literal fallback: any string is a legitimate kern value,
    // structured ("4G") or a literal spelled-out token ("[4D").
    CHECK(isValidPatternValue("kern", "4G"));
    CHECK(isValidPatternValue("kern", "[4D"));
    CHECK(isValidPatternValue("kern", "anything at all"));
}

TEST_CASE(is_valid_pattern_value_for_deg_accepts_the_documented_grammar) {
    for (const std::string& v : {"1", "7", "4+", "6-", "4++", "7--", "r"}) {
        CHECK(isValidPatternValue("deg", v));
    }
}

TEST_CASE(is_valid_pattern_value_for_deg_rejects_out_of_range_or_garbage) {
    CHECK(!isValidPatternValue("deg", "0"));
    CHECK(!isValidPatternValue("deg", "8"));
    CHECK(!isValidPatternValue("deg", "nope"));
    CHECK(!isValidPatternValue("deg", ""));
}

TEST_CASE(is_valid_pattern_value_for_fermata_accepts_only_true_or_false) {
    CHECK(isValidPatternValue("fermata", "true"));
    CHECK(isValidPatternValue("fermata", "false"));
    CHECK(!isValidPatternValue("fermata", "yes"));
    CHECK(!isValidPatternValue("fermata", "1"));
}

TEST_CASE(is_valid_pattern_value_for_metweight_accepts_every_documented_spelling) {
    for (const std::string& v : {"s", "hs", "w", "u", "strong", "half-strong", "weak", "unclassified", "1", "2", "3", "4"}) {
        CHECK(isValidPatternValue("metweight", v));
    }
}

TEST_CASE(is_valid_pattern_value_for_metweight_rejects_unknown_spellings) {
    CHECK(!isValidPatternValue("metweight", "loud"));
    CHECK(!isValidPatternValue("metweight", "5"));
}

TEST_CASE(is_valid_pattern_value_for_mint_accepts_partial_and_full_intervals) {
    for (const std::string& v : {"+M2", "-m3", "P1", "+2", "-", "+", "M2", "2", "AA4", "dd5"}) {
        CHECK(isValidPatternValue("mint", v));
    }
}

TEST_CASE(is_valid_pattern_value_for_mint_accepts_the_first_note_bracket_literal) {
    CHECK(isValidPatternValue("mint", "[gg]"));
    CHECK(isValidPatternValue("mint", "[c#]"));
}

TEST_CASE(is_valid_pattern_value_for_mint_rejects_garbage) {
    CHECK(!isValidPatternValue("mint", "nope123$"));
    CHECK(!isValidPatternValue("mint", "[gg"));
    CHECK(!isValidPatternValue("mint", "gg]"));
}

TEST_CASE(is_valid_pattern_value_for_fb_accepts_single_and_chord_values) {
    CHECK(isValidPatternValue("fb", "6"));
    CHECK(isValidPatternValue("fb", "m6"));
    CHECK(isValidPatternValue("fb", "m6 m3"));
    CHECK(isValidPatternValue("fb", "A6 M3"));
}

TEST_CASE(is_valid_pattern_value_for_fb_rejects_garbage) {
    CHECK(!isValidPatternValue("fb", "six"));
    CHECK(!isValidPatternValue("fb", ""));
    CHECK(!isValidPatternValue("fb", "6 six"));
}

TEST_CASE(is_valid_pattern_value_for_a_hint_pair_key_accepts_a_single_interval) {
    CHECK(isValidPatternValue("hint-14", "M3"));
    CHECK(isValidPatternValue("hint-14", "P8"));
    CHECK(isValidPatternValue("hint-14", "6"));
}

TEST_CASE(is_valid_pattern_value_for_a_hint_pair_key_rejects_a_chord) {
    CHECK(!isValidPatternValue("hint-14", "M3 P5"));
}

TEST_CASE(is_valid_pattern_value_for_hint_relative_and_wildcard_keys_matches_hint_pair) {
    CHECK(isValidPatternValue("hint-2", "P5"));
    CHECK(isValidPatternValue("hint-*4", "M6"));
    CHECK(!isValidPatternValue("hint-2", "six"));
}

TEST_CASE(is_valid_pattern_value_accepts_a_negated_key_and_validates_against_its_own_grammar) {
    CHECK(isValidPatternValue("!deg", "3"));
    CHECK(!isValidPatternValue("!deg", "nope"));
    CHECK(isValidPatternValue("!fermata", "true"));
    CHECK(!isValidPatternValue("!fermata", "yes"));
}

TEST_CASE(is_valid_pattern_value_for_duration_accepts_recip_notation) {
    for (const std::string& v : {"4", "8", "4.", "4..", "0", "00", "3%2"}) {
        CHECK(isValidPatternValue("duration", v));
    }
}

TEST_CASE(is_valid_pattern_value_for_duration_rejects_garbage) {
    CHECK(!isValidPatternValue("duration", "abc"));
    CHECK(!isValidPatternValue("duration", "4x"));
}

TEST_MAIN()

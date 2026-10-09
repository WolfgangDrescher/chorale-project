#include "test_framework.hpp"

#include "QueryValidation.hpp"

using choralesearch::drivingFeatureNames;
using choralesearch::isKnownDrivingFeature;
using choralesearch::isKnownPatternKey;
using choralesearch::isKnownQueryKey;
using choralesearch::isKnownSimultaneousGroupKey;
using choralesearch::intervalSizeInSemitones;
using choralesearch::isValidIntervalQuality;
using choralesearch::isValidComparisonValue;
using choralesearch::isValidMintComplementationValue;
using choralesearch::isValidPatternValue;
using choralesearch::supportsComparison;

TEST_CASE(is_known_simultaneous_group_key_accepts_every_shared_field) {
    for (const std::string& key : {"feature", "voices", "pattern", "mintStartAtPreviousToken",
                                    "mintAllowIntervalComplementation", "fbCompareExactChord", "kernIgnoreOctave",
                                    "hintReduceCompound", "durationAllowSplitNotes",
                                    "durationAllowMergedNotes", "metweightSkipUnclassified",
                                    "simultaneousAlignment"}) {
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
    for (const std::string& key : {"feature", "voices", "pattern", "mintStartAtPreviousToken",
                                    "mintAllowIntervalComplementation", "fbCompareExactChord", "kernIgnoreOctave",
                                    "hintReduceCompound", "durationAllowSplitNotes",
                                    "durationAllowMergedNotes", "metweightSkipUnclassified",
                                    "simultaneousAlignment", "id", "limit", "simultaneousWith"}) {
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

TEST_CASE(is_known_pattern_key_accepts_phrase_but_it_cannot_drive_a_search) {
    CHECK(isKnownPatternKey("phrase"));
    CHECK(isKnownPatternKey("!phrase"));
    CHECK(!isKnownDrivingFeature("phrase"));
    CHECK(!isKnownPatternKey("phraseStart"));
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

TEST_CASE(is_valid_pattern_value_for_phrase_accepts_only_start_or_end) {
    CHECK(isValidPatternValue("phrase", "start"));
    CHECK(isValidPatternValue("phrase", "end"));
    CHECK(isValidPatternValue("!phrase", "start"));
    CHECK(!isValidPatternValue("phrase", "middle"));
    CHECK(!isValidPatternValue("phrase", "true"));
    CHECK(!isValidPatternValue("phrase", ""));
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

TEST_CASE(is_valid_mint_complementation_value_accepts_simple_interval_numbers_and_the_wildcard) {
    for (const std::string& v : {"1", "2", "3", "4", "5", "6", "7", "8", "*"}) {
        CHECK(isValidMintComplementationValue(v));
    }
}

TEST_CASE(is_valid_mint_complementation_value_rejects_compound_and_non_numeric_values) {
    for (const std::string& v : {"0", "9", "10", "", "P5", "+5", "5 4"}) {
        CHECK(!isValidMintComplementationValue(v));
    }
}

TEST_CASE(supports_comparison_for_pitch_duration_and_interval_size_only) {
    for (const std::string& key : {"kern", "duration", "mint", "hint-14", "hint-2", "hint-*4", "!kern", "!hint-14"}) {
        CHECK(supportsComparison(key));
    }
    for (const std::string& key : {"deg", "fb", "metweight", "fermata", "phrase", "!deg", "hint", "bogus"}) {
        CHECK(!supportsComparison(key));
    }
}

TEST_CASE(is_valid_comparison_value_for_kern_is_a_pitch) {
    for (const std::string& v : {"g", "GG", "f#", "bb-", "c##", "dn", "F--"}) CHECK(isValidComparisonValue("kern", v));
    // Rhythm, fermata and rests are not pitches; nor is a mix of cases or of letters, which humlib
    // already calls invalid (Convert::kernToOctaveNumber).
    for (const std::string& v : {"4", "4g", "r", "g;", "gG", "gf", "h", "", "*", "#", "c#-"}) {
        CHECK(!isValidComparisonValue("kern", v));
    }
}

TEST_CASE(is_valid_comparison_value_for_duration_is_a_recip_length) {
    for (const std::string& v : {"1", "4", "4.", "8..", "3%2"}) CHECK(isValidComparisonValue("duration", v));
    for (const std::string& v : {"g", "", "*", "-4", "4;"}) CHECK(!isValidComparisonValue("duration", v));
}

TEST_CASE(is_valid_comparison_value_for_mint_is_an_interval_size_with_an_optional_sign) {
    for (const std::string& v : {"3", "+3", "-3", "10", "M3", "+m6", "-P8", "A4", "+AA4", "dd5"}) {
        CHECK(isValidComparisonValue("mint", v));
    }
    // No bare sign or quality, no zero, and no quality the number cannot have.
    for (const std::string& v : {"+", "-", "M", "0", "", "*", "[gg]", "P3", "M5", "m4", "X3", "AAAA4", "1000"}) {
        CHECK(!isValidComparisonValue("mint", v));
    }
}

TEST_CASE(is_valid_comparison_value_for_hint_is_an_unsigned_interval_size) {
    for (const std::string& key : {"hint-14", "hint-2", "hint-**"}) {
        CHECK(isValidComparisonValue(key, "10"));
        CHECK(isValidComparisonValue(key, "M10"));
        CHECK(isValidComparisonValue(key, "A4"));
        CHECK(!isValidComparisonValue(key, "+3"));
        CHECK(!isValidComparisonValue(key, "P3"));
        CHECK(!isValidComparisonValue(key, "0"));
    }
}

TEST_CASE(is_valid_comparison_value_rejects_a_key_that_cannot_be_compared) {
    CHECK(!isValidComparisonValue("deg", "1"));
    CHECK(!isValidComparisonValue("fb", "3"));
}

TEST_CASE(interval_size_in_semitones_counts_simple_and_compound_intervals) {
    CHECK_EQ(*intervalSizeInSemitones("P", 1), 0);
    CHECK_EQ(*intervalSizeInSemitones("m", 2), 1);
    CHECK_EQ(*intervalSizeInSemitones("M", 3), 4);
    CHECK_EQ(*intervalSizeInSemitones("P", 5), 7);
    CHECK_EQ(*intervalSizeInSemitones("m", 6), 8);
    CHECK_EQ(*intervalSizeInSemitones("M", 7), 11);
    CHECK_EQ(*intervalSizeInSemitones("P", 8), 12);
    CHECK_EQ(*intervalSizeInSemitones("m", 10), 15);
    CHECK_EQ(*intervalSizeInSemitones("P", 15), 24);
}

TEST_CASE(interval_size_in_semitones_applies_augmented_and_diminished) {
    CHECK_EQ(*intervalSizeInSemitones("A", 4), 6);
    CHECK_EQ(*intervalSizeInSemitones("d", 5), 6); // the tritone either way
    CHECK_EQ(*intervalSizeInSemitones("A", 2), 3);
    CHECK_EQ(*intervalSizeInSemitones("d", 3), 2);
    CHECK_EQ(*intervalSizeInSemitones("AA", 4), 7);
    CHECK_EQ(*intervalSizeInSemitones("dd", 5), 5);
}

TEST_CASE(interval_size_in_semitones_rejects_a_quality_the_number_cannot_have) {
    CHECK(!intervalSizeInSemitones("P", 3));
    CHECK(!intervalSizeInSemitones("M", 5));
    CHECK(!intervalSizeInSemitones("m", 4));
    CHECK(!intervalSizeInSemitones("X", 3));
    CHECK(!intervalSizeInSemitones("", 3));
    CHECK(!intervalSizeInSemitones("Ad", 3));
    CHECK(!intervalSizeInSemitones("P", 0));
}

TEST_CASE(is_valid_interval_quality_pairs_the_quality_with_the_kind_of_interval) {
    CHECK(isValidIntervalQuality("P", 5));
    CHECK(isValidIntervalQuality("P", 11));
    CHECK(isValidIntervalQuality("M", 6));
    CHECK(isValidIntervalQuality("m", 7));
    CHECK(isValidIntervalQuality("A", 4));
    CHECK(isValidIntervalQuality("dd", 2));
    CHECK(!isValidIntervalQuality("P", 6));
    CHECK(!isValidIntervalQuality("M", 4));
    CHECK(!isValidIntervalQuality("", 4));
}

TEST_MAIN()

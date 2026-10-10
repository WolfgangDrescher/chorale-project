#include "test_framework.hpp"

#include <algorithm>
#include <sstream>
#include <string>
#include <vector>

#include "AttributeMatcher.hpp"
#include "QueryValidation.hpp"
#include "HumdrumChorale.hpp"

using choralesearch::AttributeMap;
using choralesearch::AttributeMatcher;
using choralesearch::HumdrumChorale;
using choralesearch::MatcherOptions;
using choralesearch::PatternValue;
using choralesearch::ComparisonOperator;

// General AttributeMatcher mechanics against chor029.krn: literal/duration/fermata/
// cross-spine matching, wildcards, multi-position patterns, voice scoping. Exhaustive
// exact-result coverage lives in test_fixture_results.cpp instead.

TEST_CASE(matcher_matches_literal_driving_feature_token) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    // Matching on the driving feature itself decomposes into rhythm+pitch+fermata like any
    // other kern value -- "4D;" requires a quarter-note D with a fermata specifically.
    AttributeMatcher matcher("kern", {AttributeMap{{"kern", {"4D;"}}}});
    auto matches = matcher.findAll(chorale, 1);
    CHECK(!matches.empty());
    for (const auto& m : matches) CHECK_EQ(m.voice, std::size_t{1});
}

TEST_CASE(matcher_matches_duration_key) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    AttributeMatcher matcher("kern", {AttributeMap{{"duration", {"4"}}}});
    auto matches = matcher.findAll(chorale, 1);
    CHECK(!matches.empty());
}

TEST_CASE(matcher_kern_rhythm_only_value_matches_any_pitch_with_that_rhythm) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    AttributeMatcher rhythmOnly("kern", {AttributeMap{{"kern", {"4"}}}});
    AttributeMatcher durationKey("kern", {AttributeMap{{"duration", {"4"}}}});
    auto rhythmOnlyMatches = rhythmOnly.findAll(chorale, 2);
    auto durationMatches = durationKey.findAll(chorale, 2);
    CHECK(!rhythmOnlyMatches.empty());
    CHECK_EQ(rhythmOnlyMatches.size(), durationMatches.size());
}

TEST_CASE(matcher_secondary_tied_kern_note_is_not_a_separate_onset) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    // Position 2, voice 2 is "[4G" (tie start); position 3 is its tied continuation
    // "8GL]" -- not a new attack, so it must not show up as its own onset.
    AttributeMatcher wildcard("kern", {AttributeMap{{"kern", {"*"}}}});
    auto matches = wildcard.findAll(chorale, 2);
    bool foundPosition3 = std::any_of(matches.begin(), matches.end(),
                                       [](const auto& m) { return m.startPosition == 3; });
    CHECK(!foundPosition3);
}

TEST_CASE(matcher_kern_rhythm_reflects_full_tied_duration) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    // "[4G" (position 2) is tied to an eighth note, so it actually sounds for a
    // dotted quarter ("4."), not the plain quarter its own written duration shows.
    AttributeMatcher dottedQuarter("kern", {AttributeMap{{"kern", {"4."}}}});
    AttributeMatcher plainQuarter("kern", {AttributeMap{{"kern", {"4"}}}});
    auto dottedMatches = dottedQuarter.findAll(chorale, 2);
    auto plainMatches = plainQuarter.findAll(chorale, 2);
    bool dottedFoundPosition2 = std::any_of(dottedMatches.begin(), dottedMatches.end(),
                                             [](const auto& m) { return m.startPosition == 2; });
    bool plainFoundPosition2 = std::any_of(plainMatches.begin(), plainMatches.end(),
                                            [](const auto& m) { return m.startPosition == 2; });
    CHECK(dottedFoundPosition2);
    CHECK(!plainFoundPosition2);
}

TEST_CASE(matcher_duration_key_reflects_full_tied_duration) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    AttributeMatcher dottedQuarter("kern", {AttributeMap{{"duration", {"4."}}}});
    AttributeMatcher plainQuarter("kern", {AttributeMap{{"duration", {"4"}}}});
    auto dottedMatches = dottedQuarter.findAll(chorale, 2);
    auto plainMatches = plainQuarter.findAll(chorale, 2);
    bool dottedFoundPosition2 = std::any_of(dottedMatches.begin(), dottedMatches.end(),
                                             [](const auto& m) { return m.startPosition == 2; });
    bool plainFoundPosition2 = std::any_of(plainMatches.begin(), plainMatches.end(),
                                            [](const auto& m) { return m.startPosition == 2; });
    CHECK(dottedFoundPosition2);
    CHECK(!plainFoundPosition2);
}

TEST_CASE(matcher_kern_rhythm_and_pitch_combination_ignores_fermata_unless_asked) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    // Posiion 9, voice 1 is "4D;" (fermata); A rhythm+pitch value with no ";"
    // doesn't care about fermata, so "4D" matches all six fermatas; adding ";"
    // narrows it down to just position 7.
    AttributeMatcher rhythmAndPitch("kern", {AttributeMap{{"kern", {"4D"}}}});
    AttributeMatcher rhythmAndPitchWithFermata("kern", {AttributeMap{{"kern", {"4D;"}}}});
    auto rhythmAndPitchMatches = rhythmAndPitch.findAll(chorale, 1);
    auto withFermataMatches = rhythmAndPitchWithFermata.findAll(chorale, 1);
    CHECK_EQ(withFermataMatches.size(), std::size_t{1});
    CHECK_EQ(withFermataMatches.front().startPosition, 7);
    CHECK(rhythmAndPitchMatches.size() > withFermataMatches.size());
    bool foundPosition7 = std::any_of(rhythmAndPitchMatches.begin(), rhythmAndPitchMatches.end(),
                                    [](const auto& m) { return m.startPosition == 7; });
    CHECK(foundPosition7);
}

TEST_CASE(matcher_kern_fermata_only_value_matches_any_pitch_and_rhythm_with_a_fermata) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    AttributeMatcher fermataOnly("kern", {AttributeMap{{"kern", {";"}}}});
    AttributeMatcher fermataKey("kern", {AttributeMap{{"fermata", {"true"}}}});
    auto fermataOnlyMatches = fermataOnly.findAll(chorale, 1);
    auto fermataKeyMatches = fermataKey.findAll(chorale, 1);
    CHECK_EQ(fermataOnlyMatches.size(), fermataKeyMatches.size()); // both see all 6 fermatas
    bool foundPosition7 = std::any_of(fermataOnlyMatches.begin(), fermataOnlyMatches.end(),
                                    [](const auto& m) { return m.startPosition == 7; });
    CHECK(foundPosition7);
}

TEST_CASE(matcher_kern_rhythm_and_fermata_combine_in_one_value) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    // "4;" requires a quarter note AND a fermata -- narrower than either alone.
    AttributeMatcher rhythmAndFermata("kern", {AttributeMap{{"kern", {"4;"}}}});
    AttributeMatcher rhythmOnly("kern", {AttributeMap{{"kern", {"4"}}}});
    AttributeMatcher fermataOnly("kern", {AttributeMap{{"kern", {";"}}}});
    auto combinedMatches = rhythmAndFermata.findAll(chorale, 1);
    CHECK(!combinedMatches.empty());
    CHECK(combinedMatches.size() < rhythmOnly.findAll(chorale, 1).size());
    CHECK(combinedMatches.size() < fermataOnly.findAll(chorale, 1).size());
    bool foundPosition7 = std::any_of(combinedMatches.begin(), combinedMatches.end(),
                                    [](const auto& m) { return m.startPosition == 7; });
    CHECK(foundPosition7);
}

TEST_CASE(matcher_kern_pitch_and_fermata_combine_in_one_value) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    // "D;" requires pitch D AND a fermata -- sees past the fermata marker to the pitch,
    // and past everything else to the fermata, at the same time.
    AttributeMatcher pitchAndFermata("kern", {AttributeMap{{"kern", {"D;"}}}});
    AttributeMatcher pitchOnly("kern", {AttributeMap{{"kern", {"D"}}}});
    auto combinedMatches = pitchAndFermata.findAll(chorale, 1);
    CHECK(!combinedMatches.empty());
    CHECK(combinedMatches.size() < pitchOnly.findAll(chorale, 1).size());
    bool foundPosition7 = std::any_of(combinedMatches.begin(), combinedMatches.end(),
                                    [](const auto& m) { return m.startPosition == 7; });
    CHECK(foundPosition7);
}

TEST_CASE(matcher_kern_rest_value_matches_any_rest_regardless_of_decoration) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor006"));
    // Position 15 is a full-measure rest: "4r" in bass/soprano, "4ry" in tenor/alto -- the
    // trailing "y" (an editorial/invisible marker) doesn't stop it from being a rest.
    AttributeMatcher restOnly("kern", {AttributeMap{{"kern", {"r"}}}});
    for (std::size_t voice = 1; voice <= 4; ++voice) {
        auto matches = restOnly.findAll(chorale, voice);
        bool foundPosition15 = std::any_of(matches.begin(), matches.end(),
                                        [](const auto& m) { return m.startPosition == 15; });
        CHECK(foundPosition15);
    }
}

TEST_CASE(matcher_kern_rhythm_and_rest_combine_in_one_value) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor006"));
    // "4r" requires a quarter rest specifically -- narrower than "4" (any quarter note or
    // rest) or "r" (any rest, any rhythm) alone.
    AttributeMatcher rhythmAndRest("kern", {AttributeMap{{"kern", {"4r"}}}});
    AttributeMatcher rhythmOnly("kern", {AttributeMap{{"kern", {"4"}}}});
    AttributeMatcher restOnly("kern", {AttributeMap{{"kern", {"r"}}}});
    auto combinedMatches = rhythmAndRest.findAll(chorale, 1);
    CHECK(!combinedMatches.empty());
    CHECK(combinedMatches.size() < rhythmOnly.findAll(chorale, 1).size());
    CHECK(combinedMatches.size() <= restOnly.findAll(chorale, 1).size());
    bool foundPosition15 = std::any_of(combinedMatches.begin(), combinedMatches.end(),
                                    [](const auto& m) { return m.startPosition == 15; });
    CHECK(foundPosition15);
}

TEST_CASE(matcher_kern_rest_value_never_overlaps_a_pitch_value) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor006"));
    AttributeMatcher restOnly("kern", {AttributeMap{{"kern", {"r"}}}});
    AttributeMatcher pitchOnly("kern", {AttributeMap{{"kern", {"g"}}}});
    auto restMatches = restOnly.findAll(chorale, 4);
    auto pitchMatches = pitchOnly.findAll(chorale, 4);
    CHECK(!restMatches.empty());
    CHECK(!pitchMatches.empty());
    for (const auto& m : restMatches) {
        bool overlap = std::any_of(pitchMatches.begin(), pitchMatches.end(),
                                    [&](const auto& other) { return other.startPosition == m.startPosition; });
        CHECK(!overlap);
    }
}

TEST_CASE(matcher_kern_fermata_only_value_does_not_match_a_rest) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor006"));
    // Fermatas fall on real notes in this piece (lines 30, 41, 58, 68); the "4r"/"4ry"
    // rests at line 42 have none.
    AttributeMatcher fermataOnly("kern", {AttributeMap{{"kern", {";"}}}});
    auto matches = fermataOnly.findAll(chorale, 1);
    CHECK(!matches.empty());
    bool foundPosition15 = std::any_of(matches.begin(), matches.end(),
                                    [](const auto& m) { return m.startPosition == 15; });
    CHECK(!foundPosition15);
}

TEST_CASE(matcher_duration_pattern_accepts_an_or_list_of_values) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    AttributeMatcher quarterOnly("kern", {AttributeMap{{"duration", {"4"}}}});
    AttributeMatcher quarterOrEighth("kern", {AttributeMap{{"duration", {"4", "8"}}}});
    auto quarterCount = quarterOnly.findAll(chorale, 1).size();
    auto eitherCount = quarterOrEighth.findAll(chorale, 1).size();
    CHECK(eitherCount > 0);
    CHECK(eitherCount >= quarterCount); // OR-list can only match more, never fewer
}

TEST_CASE(matcher_matches_fermata_key) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    AttributeMatcher matcher("kern", {AttributeMap{{"fermata", {"true"}}}});
    auto matches = matcher.findAll(chorale, 1);
    CHECK_EQ(matches.size(), std::size_t{6}); // one per phrase ending
    for (const auto& m : matches) CHECK_EQ(m.startLineNumber, m.endLineNumber);
}

TEST_CASE(matcher_wildcard_matches_every_onset_in_the_voice) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    AttributeMatcher wildcard("kern", {AttributeMap{{"kern", {"*"}}}});
    AttributeMatcher unfiltered("kern", {AttributeMap{}});
    auto wildcardCount = wildcard.findAll(chorale, 1).size();
    auto unfilteredCount = unfiltered.findAll(chorale, 1).size();
    CHECK(wildcardCount > 0);
    CHECK_EQ(wildcardCount, unfilteredCount);
}

TEST_CASE(matcher_looks_up_a_different_feature_spine) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    // Scale degree "1" (tonic) in the bass -- looked up via **deg, a
    // separate spine from the **kern the matcher is driven off of.
    AttributeMatcher matcher("kern", {AttributeMap{{"deg", {"1"}}}});
    auto matches = matcher.findAll(chorale, 1);
    CHECK(matches.size() > 1);
}

TEST_CASE(matcher_two_position_pattern_finds_a_real_fermata_cadence) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    // A fermata-marked bass note followed immediately by the next onset:
    // 5 of the 6 fermatas qualify -- the 6th is the piece's final note, so
    // there's no following onset for a 2-position window to end on.
    std::vector<AttributeMap> pattern = {
        AttributeMap{{"fermata", {"true"}}},
        AttributeMap{{"kern", {"*"}}},
    };
    AttributeMatcher matcher("kern", pattern);
    auto matches = matcher.findAll(chorale, 1);
    CHECK_EQ(matches.size(), std::size_t{5});
    for (const auto& m : matches) CHECK(m.endLineNumber > m.startLineNumber);
}

TEST_CASE(matcher_distinguishes_voices_by_literal_token) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    AttributeMatcher matcher("kern", {AttributeMap{{"kern", {"4G"}}}});
    auto bassMatches = matcher.findAll(chorale, 1);
    auto tenorMatches = matcher.findAll(chorale, 2);
    auto hasOnsetAtStart = [](const auto& matches) {
        return std::any_of(matches.begin(), matches.end(),
                            [](const auto& m) { return m.startPosition == 0; });
    };
    CHECK(hasOnsetAtStart(bassMatches));
    CHECK(!hasOnsetAtStart(tenorMatches));
}

TEST_CASE(matcher_pattern_position_referencing_an_unknown_feature_matches_nothing) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    AttributeMatcher matcher("kern", {AttributeMap{{"nonexistentfeature", {"anything"}}}});
    CHECK(matcher.findAll(chorale, 1).empty());
}

TEST_CASE(matcher_returns_empty_for_an_unknown_driving_feature) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    AttributeMatcher matcher("nonexistentfeature", {AttributeMap{{"kern", {"*"}}}});
    CHECK(matcher.findAll(chorale, 1).empty());
}

TEST_CASE(matcher_returns_empty_when_the_pattern_is_longer_than_the_voice_has_onsets) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    std::vector<AttributeMap> pattern(1000, AttributeMap{{"kern", {"*"}}});
    AttributeMatcher matcher("kern", pattern);
    CHECK(matcher.findAll(chorale, 1).empty());
}

TEST_CASE(matcher_mint_start_at_previous_token_has_no_effect_on_other_driving_features) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    std::vector<AttributeMap> pattern = {AttributeMap{{"kern", {"*"}}}, AttributeMap{{"kern", {"*"}}}};
    AttributeMatcher withoutFlag("kern", pattern);
    MatcherOptions withFlagOptions;
    withFlagOptions.mintStartAtPreviousToken = true;
    AttributeMatcher withFlag("kern", pattern, withFlagOptions);

    auto without = withoutFlag.findAll(chorale, 1);
    auto with = withFlag.findAll(chorale, 1);

    REQUIRE(!without.empty());
    REQUIRE(with.size() == without.size());
    for (std::size_t i = 0; i < with.size(); ++i) {
        CHECK_EQ(with[i].startLineNumber, without[i].startLineNumber);
    }
}

TEST_CASE(matcher_mint_start_at_previous_token_shifts_every_start_back_by_one_onset) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    // Positions left empty rather than an explicit {"mint": "*"} -- position 0 pinning
    // the driving feature (wildcard or not) is exactly the escape hatch that disables
    // the shift, which would make this test check nothing.
    std::vector<AttributeMap> pattern = {AttributeMap{}, AttributeMap{}};
    AttributeMatcher withoutShift("mint", pattern);
    MatcherOptions withShiftOptions;
    withShiftOptions.mintStartAtPreviousToken = true;
    AttributeMatcher withShift("mint", pattern, withShiftOptions);

    auto unshifted = withoutShift.findAll(chorale, 4);
    auto shifted = withShift.findAll(chorale, 4);

    // A wildcard-only 2-position pattern matches every consecutive onset pair, so
    // both matchers produce one match per window, in the same order.
    REQUIRE(unshifted.size() > 1u);
    REQUIRE(shifted.size() == unshifted.size());

    // The very first window has nothing earlier to shift to.
    CHECK_EQ(shifted[0].startLineNumber, unshifted[0].startLineNumber);

    // Every later window's shifted start lands exactly on the previous window's
    // (unshifted) start -- that's what "one onset earlier" means for a sliding window.
    for (std::size_t i = 1; i < shifted.size(); ++i) {
        CHECK_EQ(shifted[i].startLineNumber, unshifted[i - 1].startLineNumber);
        CHECK_EQ(shifted[i].endLineNumber, unshifted[i].endLineNumber); // end is never touched
    }
}

TEST_CASE(matcher_mint_start_at_previous_token_is_a_noop_after_an_explicit_wildcard) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    // The pattern's own first position already pins the driving feature (even though,
    // as a wildcard, it accepts any value) -- that position's onset is the intended
    // start already, so the flag must not shift past it.
    std::vector<AttributeMap> pattern = {
        AttributeMap{{"mint", {"*"}}},
        AttributeMap{{"mint", {"*"}}},
        AttributeMap{{"mint", {"*"}}},
    };
    AttributeMatcher withoutFlag("mint", pattern);
    MatcherOptions withFlagOptions;
    withFlagOptions.mintStartAtPreviousToken = true;
    AttributeMatcher withFlag("mint", pattern, withFlagOptions);

    auto without = withoutFlag.findAll(chorale, 4);
    auto with = withFlag.findAll(chorale, 4);

    REQUIRE(!without.empty());
    REQUIRE(with.size() == without.size());
    for (std::size_t i = 0; i < with.size(); ++i) {
        CHECK_EQ(with[i].startLineNumber, without[i].startLineNumber);
    }
}

TEST_CASE(matcher_mint_pattern_can_omit_quality_to_match_any_quality) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    AttributeMatcher exact("mint", {AttributeMap{{"mint", {"+M2"}}}});
    AttributeMatcher directionOnly("mint", {AttributeMap{{"mint", {"+2"}}}});

    auto exactMatches = exact.findAll(chorale, 4);
    auto directionOnlyMatches = directionOnly.findAll(chorale, 4);

    REQUIRE(!exactMatches.empty());
    // Every exact "+M2" match must also satisfy the quality-agnostic "+2".
    for (const auto& m : exactMatches) {
        bool found = std::any_of(directionOnlyMatches.begin(), directionOnlyMatches.end(),
                                  [&](const auto& d) { return d.startLineNumber == m.startLineNumber; });
        CHECK(found);
    }
    CHECK(directionOnlyMatches.size() >= exactMatches.size());
}

TEST_CASE(matcher_mint_pattern_can_omit_sign_and_quality_to_match_any_direction) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    AttributeMatcher ascending("mint", {AttributeMap{{"mint", {"+2"}}}});
    AttributeMatcher descending("mint", {AttributeMap{{"mint", {"-2"}}}});
    AttributeMatcher either("mint", {AttributeMap{{"mint", {"2"}}}});

    auto ascendingMatches = ascending.findAll(chorale, 4);
    auto descendingMatches = descending.findAll(chorale, 4);
    auto eitherMatches = either.findAll(chorale, 4);

    REQUIRE(!ascendingMatches.empty());
    REQUIRE(!descendingMatches.empty());
    CHECK_EQ(eitherMatches.size(), ascendingMatches.size() + descendingMatches.size());
}

TEST_CASE(matcher_mint_pattern_can_omit_quality_and_number_to_match_any_interval_in_that_direction) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    AttributeMatcher up("mint", {AttributeMap{{"mint", {"+"}}}});
    AttributeMatcher down("mint", {AttributeMap{{"mint", {"-"}}}});
    AttributeMatcher upSecond("mint", {AttributeMap{{"mint", {"+2"}}}});
    AttributeMatcher downThird("mint", {AttributeMap{{"mint", {"-3"}}}});

    auto upMatches = up.findAll(chorale, 4);
    auto downMatches = down.findAll(chorale, 4);
    auto upSecondMatches = upSecond.findAll(chorale, 4);
    auto downThirdMatches = downThird.findAll(chorale, 4);

    REQUIRE(!upMatches.empty());
    REQUIRE(!downMatches.empty());
    REQUIRE(!upSecondMatches.empty());
    REQUIRE(!downThirdMatches.empty());

    // Every specific-interval match in a given direction must also satisfy the
    // direction-only, any-interval-size, any-quality pattern for that same direction.
    for (const auto& m : upSecondMatches) {
        bool found = std::any_of(upMatches.begin(), upMatches.end(),
                                  [&](const auto& u) { return u.startLineNumber == m.startLineNumber; });
        CHECK(found);
    }
    for (const auto& m : downThirdMatches) {
        bool found = std::any_of(downMatches.begin(), downMatches.end(),
                                  [&](const auto& d) { return d.startLineNumber == m.startLineNumber; });
        CHECK(found);
    }
    CHECK(upMatches.size() >= upSecondMatches.size());
    CHECK(downMatches.size() >= downThirdMatches.size());

    // "+" and "-" never overlap -- no onset is both ascending and descending at once.
    for (const auto& m : upMatches) {
        bool alsoDown = std::any_of(downMatches.begin(), downMatches.end(),
                                     [&](const auto& d) { return d.startLineNumber == m.startLineNumber; });
        CHECK(!alsoDown);
    }
}

TEST_CASE(matcher_mint_pattern_mixes_partial_and_exact_values_across_positions) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    std::vector<AttributeMap> pattern = {
        AttributeMap{{"mint", {"+2"}}},
        AttributeMap{{"mint", {"+M2"}}},
    };
    AttributeMatcher matcher("mint", pattern);
    CHECK(!matcher.findAll(chorale, 4).empty());
}

TEST_CASE(matcher_mint_does_not_match_complementary_intervals_by_default) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    AttributeMatcher fifthDown("mint", {AttributeMap{{"mint", {"-5"}}}});
    AttributeMatcher fourthUp("mint", {AttributeMap{{"mint", {"+4"}}}});

    auto fifthDownMatches = fifthDown.findAll(chorale, 1);
    auto fourthUpMatches = fourthUp.findAll(chorale, 1);

    REQUIRE(!fifthDownMatches.empty());
    REQUIRE(!fourthUpMatches.empty());
    // Without the option the two are strictly separate result sets -- no onset is in both.
    for (const auto& m : fifthDownMatches) {
        bool alsoFourthUp = std::any_of(fourthUpMatches.begin(), fourthUpMatches.end(),
                                         [&](const auto& other) { return other.startLineNumber == m.startLineNumber; });
        CHECK(!alsoFourthUp);
    }
}

TEST_CASE(matcher_mint_allow_interval_complementation_also_matches_the_complementary_interval) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    AttributeMatcher fifthDown("mint", {AttributeMap{{"mint", {"-5"}}}});
    AttributeMatcher fourthUp("mint", {AttributeMap{{"mint", {"+4"}}}});
    MatcherOptions complementFifths;
    complementFifths.mintAllowIntervalComplementation = {"5"};
    AttributeMatcher fifthDownOrItsComplement("mint", {AttributeMap{{"mint", {"-5"}}}}, complementFifths);

    auto fifthDownMatches = fifthDown.findAll(chorale, 1);
    auto fourthUpMatches = fourthUp.findAll(chorale, 1);
    auto complementedMatches = fifthDownOrItsComplement.findAll(chorale, 1);

    REQUIRE(!fifthDownMatches.empty());
    REQUIRE(!fourthUpMatches.empty());
    CHECK_EQ(complementedMatches.size(), fifthDownMatches.size() + fourthUpMatches.size());
    for (const auto& m : fourthUpMatches) {
        bool found = std::any_of(complementedMatches.begin(), complementedMatches.end(),
                                  [&](const auto& c) { return c.startLineNumber == m.startLineNumber; });
        CHECK(found);
    }
}

TEST_CASE(matcher_mint_allow_interval_complementation_inverts_the_quality_too) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    // A major 2nd up complements to a *minor* 7th down, not a major one.
    AttributeMatcher majorSecondUp("mint", {AttributeMap{{"mint", {"+M2"}}}});
    AttributeMatcher minorSeventhDown("mint", {AttributeMap{{"mint", {"-m7"}}}});
    AttributeMatcher majorSeventhDown("mint", {AttributeMap{{"mint", {"-M7"}}}});
    MatcherOptions complementedOptions;
    complementedOptions.mintAllowIntervalComplementation = {"2"};
    AttributeMatcher complemented("mint", {AttributeMap{{"mint", {"+M2"}}}}, complementedOptions);

    auto majorSecondUpMatches = majorSecondUp.findAll(chorale, 1);
    auto minorSeventhDownMatches = minorSeventhDown.findAll(chorale, 1);
    auto complementedMatches = complemented.findAll(chorale, 1);

    REQUIRE(!majorSecondUpMatches.empty());
    REQUIRE(!minorSeventhDownMatches.empty());
    CHECK(majorSeventhDown.findAll(chorale, 1).empty()); // nothing a wrong inversion could pick up
    CHECK_EQ(complementedMatches.size(), majorSecondUpMatches.size() + minorSeventhDownMatches.size());
}

TEST_CASE(matcher_mint_allow_interval_complementation_only_opts_in_the_listed_numbers) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    AttributeMatcher fifthDown("mint", {AttributeMap{{"mint", {"-5"}}}});
    // The number as written in the pattern is what opts in, so a pattern of "-5" is untouched
    // by a list of {"4"} -- that one opts in patterns written as a 4th instead.
    MatcherOptions listedFourOptions;
    listedFourOptions.mintAllowIntervalComplementation = {"4"};
    AttributeMatcher listedFour("mint", {AttributeMap{{"mint", {"-5"}}}}, listedFourOptions);
    MatcherOptions listedThreeAndFiveOptions;
    listedThreeAndFiveOptions.mintAllowIntervalComplementation = {"3", "5"};
    AttributeMatcher listedThreeAndFive("mint", {AttributeMap{{"mint", {"-5"}}}}, listedThreeAndFiveOptions);

    auto fifthDownMatches = fifthDown.findAll(chorale, 1);
    auto listedFourMatches = listedFour.findAll(chorale, 1);
    auto listedThreeAndFiveMatches = listedThreeAndFive.findAll(chorale, 1);

    REQUIRE(!fifthDownMatches.empty());
    CHECK_EQ(listedFourMatches.size(), fifthDownMatches.size());
    CHECK(listedThreeAndFiveMatches.size() > fifthDownMatches.size()); // "5" is in the list here
}

TEST_CASE(matcher_mint_allow_interval_complementation_wildcard_opts_in_every_number) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    MatcherOptions listedFiveOptions;
    listedFiveOptions.mintAllowIntervalComplementation = {"5"};
    AttributeMatcher listedFive("mint", {AttributeMap{{"mint", {"-P5"}}}}, listedFiveOptions);
    MatcherOptions wildcardOptions;
    wildcardOptions.mintAllowIntervalComplementation = {"*"};
    AttributeMatcher wildcard("mint", {AttributeMap{{"mint", {"-P5"}}}}, wildcardOptions);

    auto listedFiveMatches = listedFive.findAll(chorale, 1);
    auto wildcardMatches = wildcard.findAll(chorale, 1);

    REQUIRE(!listedFiveMatches.empty());
    CHECK_EQ(wildcardMatches.size(), listedFiveMatches.size());
}

TEST_CASE(matcher_mint_allow_interval_complementation_ignores_values_without_an_interval_number) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    // "+" pins down a direction but no interval size, so there's nothing to complement --
    // complementing it would otherwise quietly turn it into "any motion at all".
    AttributeMatcher up("mint", {AttributeMap{{"mint", {"+"}}}});
    MatcherOptions complementEverything;
    complementEverything.mintAllowIntervalComplementation = {"*"};
    AttributeMatcher upWithComplementation("mint", {AttributeMap{{"mint", {"+"}}}}, complementEverything);

    auto upMatches = up.findAll(chorale, 1);
    auto upWithComplementationMatches = upWithComplementation.findAll(chorale, 1);

    REQUIRE(!upMatches.empty());
    CHECK_EQ(upWithComplementationMatches.size(), upMatches.size());
}

TEST_CASE(matcher_mint_allow_interval_complementation_applies_to_a_cross_referenced_mint_key) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    // Driven by kern, with mint only checked alongside it: the option is about the "mint" key,
    // not about mint being the driving feature.
    AttributeMatcher plain("kern", {AttributeMap{{"mint", {"-5"}}}});
    MatcherOptions complementedOptions;
    complementedOptions.mintAllowIntervalComplementation = {"5"};
    AttributeMatcher complemented("kern", {AttributeMap{{"mint", {"-5"}}}}, complementedOptions);

    auto plainMatches = plain.findAll(chorale, 1);
    auto complementedMatches = complemented.findAll(chorale, 1);

    REQUIRE(!plainMatches.empty());
    CHECK(complementedMatches.size() > plainMatches.size());
}

TEST_CASE(matcher_mint_allow_interval_complementation_composes_with_negation) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    // Negation still flips the whole position's result, so the negated set is the complement
    // (in the set sense) of the positive one, over the same total onset count.
    AttributeMatcher total("mint", {AttributeMap{}});
    MatcherOptions positiveOptions;
    positiveOptions.mintAllowIntervalComplementation = {"5"};
    AttributeMatcher positive("mint", {AttributeMap{{"mint", {"-5"}}}}, positiveOptions);
    MatcherOptions negatedOptions;
    negatedOptions.mintAllowIntervalComplementation = {"5"};
    AttributeMatcher negated("mint", {AttributeMap{{"!mint", {"-5"}}}}, negatedOptions);

    auto totalMatches = total.findAll(chorale, 1);
    auto positiveMatches = positive.findAll(chorale, 1);
    auto negatedMatches = negated.findAll(chorale, 1);

    REQUIRE(!positiveMatches.empty());
    CHECK_EQ(positiveMatches.size() + negatedMatches.size(), totalMatches.size());
}

TEST_CASE(matcher_fb_as_driving_feature_gives_identical_matches_for_every_voice) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    AttributeMatcher matcher("fb", {AttributeMap{}});
    auto bass = matcher.findAll(chorale, 1);
    auto tenor = matcher.findAll(chorale, 2);
    auto alto = matcher.findAll(chorale, 3);
    auto soprano = matcher.findAll(chorale, 4);

    REQUIRE(!bass.empty());
    REQUIRE(tenor.size() == bass.size());
    REQUIRE(alto.size() == bass.size());
    REQUIRE(soprano.size() == bass.size());
    for (std::size_t i = 0; i < bass.size(); ++i) {
        CHECK_EQ(tenor[i].startLineNumber, bass[i].startLineNumber);
        CHECK_EQ(alto[i].startLineNumber, bass[i].startLineNumber);
        CHECK_EQ(soprano[i].startLineNumber, bass[i].startLineNumber);
    }
}

TEST_CASE(matcher_fb_as_cross_referenced_key_resolves_the_same_regardless_of_voice) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    // Driving off kern in voice 4, constrained on a literal (non-wildcard) fb value so the
    // cross-spine lookup actually runs. fb's only spine is index 1 (bass); without voice
    // remapping, chorale.spine("fb", 4) would be out of range and this would fail.
    AttributeMatcher matcher("kern", {AttributeMap{{"fb", {"P5 M3"}}}});
    CHECK(!matcher.findAll(chorale, 4).empty());
}

// fb pattern values can omit quality per figure, and can list several figures that
// must all be present in the chord -- checked structurally (superset relationships),
// exact real-music results live in test_fixture_results.cpp.

TEST_CASE(matcher_fb_pattern_can_omit_quality_on_one_figure_but_not_another) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    AttributeMatcher exact("fb", {AttributeMap{{"fb", {"m6 m3"}}}});
    AttributeMatcher mixed("fb", {AttributeMap{{"fb", {"6 m3"}}}}); // 6th free, 3rd pinned
    AttributeMatcher any("fb", {AttributeMap{{"fb", {"6 3"}}}}); // both free

    auto exactMatches = exact.findAll(chorale, 1);
    auto mixedMatches = mixed.findAll(chorale, 1);
    auto anyMatches = any.findAll(chorale, 1);

    REQUIRE(!exactMatches.empty());
    // Every exact "m6 m3" match must also satisfy the looser patterns.
    for (const auto& m : exactMatches) {
        auto hasLine = [&](const auto& matches) {
            return std::any_of(matches.begin(), matches.end(),
                                [&](const auto& x) { return x.startPosition == m.startPosition; });
        };
        CHECK(hasLine(mixedMatches));
        CHECK(hasLine(anyMatches));
    }
    CHECK(mixedMatches.size() >= exactMatches.size());
    CHECK(anyMatches.size() >= mixedMatches.size());
}

TEST_CASE(matcher_fb_pattern_requires_every_listed_figure_present_in_the_chord) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    AttributeMatcher sixOnly("fb", {AttributeMap{{"fb", {"6"}}}});
    AttributeMatcher sixAndThree("fb", {AttributeMap{{"fb", {"6 3"}}}});

    auto sixOnlyMatches = sixOnly.findAll(chorale, 1);
    auto sixAndThreeMatches = sixAndThree.findAll(chorale, 1);

    REQUIRE(!sixOnlyMatches.empty());
    // Requiring an additional figure can only narrow the result set, never widen it.
    CHECK(sixAndThreeMatches.size() <= sixOnlyMatches.size());
    for (const auto& m : sixAndThreeMatches) {
        bool alsoInSixOnly = std::any_of(sixOnlyMatches.begin(), sixOnlyMatches.end(),
                                          [&](const auto& x) { return x.startLineNumber == m.startLineNumber; });
        CHECK(alsoInSixOnly);
    }
}

TEST_CASE(matcher_kern_ignore_octave_matches_every_register_of_a_pitch_class) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    // Soprano's G-pitch-class notes are all written lowercase ("g"); plain "G" (uppercase,
    // a different register) matches none of them without the flag.
    AttributeMatcher upperCaseOnly("kern", {AttributeMap{{"kern", {"G"}}}});
    AttributeMatcher lowerCaseOnly("kern", {AttributeMap{{"kern", {"g"}}}});
    MatcherOptions ignoreOctave;
    ignoreOctave.kernIgnoreOctave = true;
    AttributeMatcher upperCaseIgnoringOctave("kern", {AttributeMap{{"kern", {"G"}}}}, ignoreOctave);

    auto upperCaseMatches = upperCaseOnly.findAll(chorale, 4);
    auto lowerCaseMatches = lowerCaseOnly.findAll(chorale, 4);
    auto ignoringOctaveMatches = upperCaseIgnoringOctave.findAll(chorale, 4);

    CHECK(upperCaseMatches.empty());
    REQUIRE(!lowerCaseMatches.empty());
    REQUIRE(ignoringOctaveMatches.size() == lowerCaseMatches.size());
    for (std::size_t i = 0; i < ignoringOctaveMatches.size(); ++i) {
        CHECK_EQ(ignoringOctaveMatches[i].startLineNumber, lowerCaseMatches[i].startLineNumber);
    }
}

TEST_CASE(matcher_kern_ignore_octave_still_honors_rhythm_and_fermata_in_the_same_value) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    // "4D;" (bass, position 7) is a quarter-note D with a fermata -- ignoring octave must
    // still require the rhythm and fermata components, just not the D's own register.
    MatcherOptions matcherOptions;
    matcherOptions.kernIgnoreOctave = true;
    AttributeMatcher matcher("kern", {AttributeMap{{"kern", {"4D;"}}}}, matcherOptions);
    auto matches = matcher.findAll(chorale, 1);
    REQUIRE(matches.size() == 1u);
    CHECK_EQ(matches.front().startPosition, 7);
}

TEST_CASE(matcher_kern_ignore_octave_does_not_conflate_rests_with_pitches) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor006"));
    MatcherOptions restOnlyOptions;
    restOnlyOptions.kernIgnoreOctave = true;
    AttributeMatcher restOnly("kern", {AttributeMap{{"kern", {"r"}}}}, restOnlyOptions);
    MatcherOptions pitchOnlyOptions;
    pitchOnlyOptions.kernIgnoreOctave = true;
    AttributeMatcher pitchOnly("kern", {AttributeMap{{"kern", {"g"}}}}, pitchOnlyOptions);
    auto restMatches = restOnly.findAll(chorale, 4);
    auto pitchMatches = pitchOnly.findAll(chorale, 4);
    CHECK(!restMatches.empty());
    CHECK(!pitchMatches.empty());
    for (const auto& m : restMatches) {
        bool overlap = std::any_of(pitchMatches.begin(), pitchMatches.end(),
                                    [&](const auto& other) { return other.startPosition == m.startPosition; });
        CHECK(!overlap);
    }
}

TEST_CASE(matcher_fb_compare_exact_chord_rejects_chords_with_extra_figures) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    AttributeMatcher permissive("fb", {AttributeMap{{"fb", {"6 3"}}}});
    MatcherOptions exactOptions;
    exactOptions.fbCompareExactChord = true;
    AttributeMatcher exact("fb", {AttributeMap{{"fb", {"6 3"}}}}, exactOptions);

    auto permissiveMatches = permissive.findAll(chorale, 1);
    auto exactMatches = exact.findAll(chorale, 1);

    REQUIRE(!permissiveMatches.empty());
    // Exact-chord matching can only narrow the permissive (default) result set.
    CHECK(exactMatches.size() <= permissiveMatches.size());
    for (const auto& m : exactMatches) {
        bool alsoInPermissive = std::any_of(permissiveMatches.begin(), permissiveMatches.end(),
                                             [&](const auto& x) { return x.startLineNumber == m.startLineNumber; });
        CHECK(alsoInPermissive);
    }
}

TEST_CASE(matcher_metweight_accepts_abbreviation_full_word_or_number_for_the_same_value) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    AttributeMatcher abbreviation("metweight", {AttributeMap{{"metweight", {"s"}}}});
    AttributeMatcher fullWord("metweight", {AttributeMap{{"metweight", {"strong"}}}});
    AttributeMatcher number("metweight", {AttributeMap{{"metweight", {"1"}}}});

    auto abbreviationMatches = abbreviation.findAll(chorale, 4);
    auto fullWordMatches = fullWord.findAll(chorale, 4);
    auto numberMatches = number.findAll(chorale, 4);

    REQUIRE(!abbreviationMatches.empty());
    REQUIRE(fullWordMatches.size() == abbreviationMatches.size());
    REQUIRE(numberMatches.size() == abbreviationMatches.size());
    for (std::size_t i = 0; i < abbreviationMatches.size(); ++i) {
        CHECK_EQ(fullWordMatches[i].startPosition, abbreviationMatches[i].startPosition);
        CHECK_EQ(numberMatches[i].startPosition, abbreviationMatches[i].startPosition);
    }
}

TEST_CASE(matcher_metweight_half_strong_also_accepts_its_full_word_and_number) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    AttributeMatcher abbreviation("metweight", {AttributeMap{{"metweight", {"hs"}}}});
    AttributeMatcher fullWord("metweight", {AttributeMap{{"metweight", {"half-strong"}}}});
    AttributeMatcher number("metweight", {AttributeMap{{"metweight", {"2"}}}});

    auto abbreviationMatches = abbreviation.findAll(chorale, 4);
    REQUIRE(!abbreviationMatches.empty());
    CHECK_EQ(fullWord.findAll(chorale, 4).size(), abbreviationMatches.size());
    CHECK_EQ(number.findAll(chorale, 4).size(), abbreviationMatches.size());
}

TEST_CASE(matcher_metweight_as_driving_feature_partitions_into_the_weight_classes) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    AttributeMatcher total("metweight", {AttributeMap{{"metweight", {"*"}}}});
    AttributeMatcher strong("metweight", {AttributeMap{{"metweight", {"strong"}}}});
    AttributeMatcher halfStrong("metweight", {AttributeMap{{"metweight", {"half-strong"}}}});
    AttributeMatcher weak("metweight", {AttributeMap{{"metweight", {"weak"}}}});
    AttributeMatcher unclassified("metweight", {AttributeMap{{"metweight", {"unclassified"}}}});

    auto totalCount = total.findAll(chorale, 4).size();
    REQUIRE(totalCount > 0u);
    CHECK_EQ(strong.findAll(chorale, 4).size() + halfStrong.findAll(chorale, 4).size() +
                 weak.findAll(chorale, 4).size() + unclassified.findAll(chorale, 4).size(),
             totalCount);
}

TEST_CASE(matcher_metweight_works_as_a_cross_referenced_key) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    // Driving off kern, constrained on a literal metweight value -- exercises the
    // cross-spine lookup path, not metweight as the driving feature.
    AttributeMatcher matcher("kern", {AttributeMap{{"metweight", {"strong"}}}});
    CHECK(!matcher.findAll(chorale, 4).empty());
}

TEST_CASE(matcher_metweight_compare_negative_matches_with_positive) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    AttributeMatcher positive("metweight", {AttributeMap{{"metweight", {"strong", "half-strong", "weak"}}}});
    AttributeMatcher negative("metweight", {AttributeMap{{"!metweight", {"unclassified"}}}});

    auto positiveMatches = positive.findAll(chorale, 4);
    auto negativeMatches = negative.findAll(chorale, 4);
    REQUIRE(!positiveMatches.empty());
    CHECK_EQ(positiveMatches.size(), negativeMatches.size());
}

// A "!" prefix on a pattern key negates that whole position (De Morgan's over the
// OR-list) -- checked by partitioning: count("!key") + count("key") must equal the
// total onset count, for the plain, mint, and fb comparators alike.

TEST_CASE(matcher_negated_key_partitions_plain_comparator_matches) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    AttributeMatcher total("deg", {AttributeMap{}});
    AttributeMatcher positive("deg", {AttributeMap{{"deg", {"3", "5"}}}});
    AttributeMatcher negated("deg", {AttributeMap{{"!deg", {"3", "5"}}}});

    auto totalMatches = total.findAll(chorale, 1);
    auto positiveMatches = positive.findAll(chorale, 1);
    auto negatedMatches = negated.findAll(chorale, 1);

    REQUIRE(!totalMatches.empty());
    CHECK_EQ(positiveMatches.size() + negatedMatches.size(), totalMatches.size());

    // No overlap between the two partitions.
    for (const auto& m : negatedMatches) {
        bool alsoInPositive = std::any_of(positiveMatches.begin(), positiveMatches.end(),
                                           [&](const auto& x) { return x.startLineNumber == m.startLineNumber; });
        CHECK(!alsoInPositive);
    }
}

TEST_CASE(matcher_negated_key_partitions_mint_comparator_matches) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    AttributeMatcher total("mint", {AttributeMap{}});
    AttributeMatcher positive("mint", {AttributeMap{{"mint", {"+2"}}}});
    AttributeMatcher negated("mint", {AttributeMap{{"!mint", {"+2"}}}});

    auto totalMatches = total.findAll(chorale, 4);
    auto positiveMatches = positive.findAll(chorale, 4);
    auto negatedMatches = negated.findAll(chorale, 4);

    REQUIRE(!totalMatches.empty());
    CHECK_EQ(positiveMatches.size() + negatedMatches.size(), totalMatches.size());
}

TEST_CASE(matcher_negated_key_partitions_fb_comparator_matches) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    AttributeMatcher total("fb", {AttributeMap{}});
    AttributeMatcher positive("fb", {AttributeMap{{"fb", {"6"}}}});
    AttributeMatcher negated("fb", {AttributeMap{{"!fb", {"6"}}}});

    auto totalMatches = total.findAll(chorale, 1);
    auto positiveMatches = positive.findAll(chorale, 1);
    auto negatedMatches = negated.findAll(chorale, 1);

    REQUIRE(!totalMatches.empty());
    CHECK_EQ(positiveMatches.size() + negatedMatches.size(), totalMatches.size());
}

TEST_CASE(matcher_hint_pair_as_driving_feature_gives_identical_matches_for_every_voice) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    AttributeMatcher matcher("hint-14", {AttributeMap{}});
    auto bass = matcher.findAll(chorale, 1);
    auto tenor = matcher.findAll(chorale, 2);
    auto alto = matcher.findAll(chorale, 3);
    auto soprano = matcher.findAll(chorale, 4);

    REQUIRE(!bass.empty());
    REQUIRE(tenor.size() == bass.size());
    REQUIRE(alto.size() == bass.size());
    REQUIRE(soprano.size() == bass.size());
    for (std::size_t i = 0; i < bass.size(); ++i) {
        CHECK_EQ(tenor[i].startLineNumber, bass[i].startLineNumber);
        CHECK_EQ(alto[i].startLineNumber, bass[i].startLineNumber);
        CHECK_EQ(soprano[i].startLineNumber, bass[i].startLineNumber);
    }
}

TEST_CASE(matcher_hint_pair_as_cross_referenced_key_resolves_the_same_regardless_of_voice) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    AttributeMatcher matcher("kern", {AttributeMap{{"hint-14", {"P8"}}}});
    CHECK(!matcher.findAll(chorale, 2).empty());
}

TEST_CASE(matcher_hint_pair_pattern_can_omit_quality_to_match_any_quality) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    AttributeMatcher major("hint-12", {AttributeMap{{"hint-12", {"M3"}}}});
    AttributeMatcher minor("hint-12", {AttributeMap{{"hint-12", {"m3"}}}});
    AttributeMatcher either("hint-12", {AttributeMap{{"hint-12", {"3"}}}});

    auto majorMatches = major.findAll(chorale, 1);
    auto minorMatches = minor.findAll(chorale, 1);
    auto eitherMatches = either.findAll(chorale, 1);

    REQUIRE(!majorMatches.empty());
    REQUIRE(!minorMatches.empty());
    CHECK_EQ(eitherMatches.size(), majorMatches.size() + minorMatches.size());
}

TEST_CASE(matcher_hint_pair_wildcard_key_matches_if_any_pair_satisfies) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    AttributeMatcher wildcard("kern", {AttributeMap{{"hint-*4", {"M6"}}}});
    AttributeMatcher tenorSoprano("kern", {AttributeMap{{"hint-24", {"M6"}}}});
    AttributeMatcher altoSoprano("kern", {AttributeMap{{"hint-34", {"M6"}}}});

    auto wildcardMatches = wildcard.findAll(chorale, 1);
    auto tenorSopranoMatches = tenorSoprano.findAll(chorale, 1);
    auto altoSopranoMatches = altoSoprano.findAll(chorale, 1);

    REQUIRE(!tenorSopranoMatches.empty());
    REQUIRE(!altoSopranoMatches.empty());
    for (const auto& m : tenorSopranoMatches) {
        bool found = std::any_of(wildcardMatches.begin(), wildcardMatches.end(),
                                  [&](const auto& x) { return x.startLineNumber == m.startLineNumber; });
        CHECK(found);
    }
    for (const auto& m : altoSopranoMatches) {
        bool found = std::any_of(wildcardMatches.begin(), wildcardMatches.end(),
                                  [&](const auto& x) { return x.startLineNumber == m.startLineNumber; });
        CHECK(found);
    }
}

TEST_CASE(matcher_hint_relative_key_resolves_to_the_concrete_pair_relative_to_the_walked_voice) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    AttributeMatcher relative("kern", {AttributeMap{{"hint-2", {"P5"}}}});
    AttributeMatcher concrete("kern", {AttributeMap{{"hint-23", {"P5"}}}});

    auto relativeMatches = relative.findAll(chorale, 3);
    auto concreteMatches = concrete.findAll(chorale, 3);

    REQUIRE(!relativeMatches.empty());
    REQUIRE(relativeMatches.size() == concreteMatches.size());
    for (std::size_t i = 0; i < relativeMatches.size(); ++i) {
        CHECK_EQ(relativeMatches[i].startLineNumber, concreteMatches[i].startLineNumber);
    }
}

TEST_CASE(matcher_hint_relative_key_never_matches_when_it_equals_the_walked_voice) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    AttributeMatcher matcher("kern", {AttributeMap{{"hint-2", {"P1"}}}});
    CHECK(matcher.findAll(chorale, 2).empty());
}

TEST_CASE(matcher_hint_relative_key_wildcard_bypasses_the_self_reference_check) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    AttributeMatcher wildcard("kern", {AttributeMap{{"hint-2", {"*"}}}});
    AttributeMatcher total("kern", {AttributeMap{}});
    CHECK_EQ(wildcard.findAll(chorale, 2).size(), total.findAll(chorale, 2).size());
}

TEST_CASE(matcher_hint_reduce_compound_folds_both_pattern_and_actual_to_within_an_octave) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    AttributeMatcher withoutReduce("hint-14", {AttributeMap{{"hint-14", {"M3"}}}});
    AttributeMatcher m10("hint-14", {AttributeMap{{"hint-14", {"M10"}}}});
    AttributeMatcher m17("hint-14", {AttributeMap{{"hint-14", {"M17"}}}});
    MatcherOptions withReduceOptions;
    withReduceOptions.hintReduceCompound = true;
    AttributeMatcher withReduce("hint-14", {AttributeMap{{"hint-14", {"M3"}}}}, withReduceOptions);

    CHECK(withoutReduce.findAll(chorale, 1).empty());
    auto m10Matches = m10.findAll(chorale, 1);
    auto m17Matches = m17.findAll(chorale, 1);
    auto reducedMatches = withReduce.findAll(chorale, 1);

    REQUIRE(!m10Matches.empty());
    REQUIRE(!m17Matches.empty());
    CHECK_EQ(reducedMatches.size(), m10Matches.size() + m17Matches.size());
}

TEST_CASE(matcher_hint_reduce_compound_folds_a_compound_mint_interval_to_within_an_octave) {
    // The bass leaps a twelfth up, from C to the g above.
    std::istringstream contents(
        "**kern\t**kern\t**kern\t**kern\n"
        "*clefF4\t*clefGv2\t*clefG2\t*clefG2\n"
        "*M4/4\t*M4/4\t*M4/4\t*M4/4\n"
        "=1\t=1\t=1\t=1\n"
        "2C\t2g\t2e\t2cc\n"
        "2g\t2g\t2e\t2cc\n"
        "==\t==\t==\t==\n*-\t*-\t*-\t*-\n");
    HumdrumChorale chorale(contents, "mint-compound-test");
    MatcherOptions reduceOptions;
    reduceOptions.hintReduceCompound = true;
    AttributeMatcher twelfth("mint", {AttributeMap{{"mint", {"+P12"}}}});
    AttributeMatcher fifth("mint", {AttributeMap{{"mint", {"+P5"}}}});
    AttributeMatcher fifthReduced("mint", {AttributeMap{{"mint", {"+P5"}}}}, reduceOptions);

    CHECK_EQ(twelfth.findAll(chorale, 1).size(), std::size_t{1});
    CHECK(fifth.findAll(chorale, 1).empty());
    CHECK_EQ(fifthReduced.findAll(chorale, 1).size(), std::size_t{1});
}

TEST_CASE(matcher_negated_key_partitions_hint_comparator_matches) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    AttributeMatcher total("hint-14", {AttributeMap{}});
    AttributeMatcher positive("hint-14", {AttributeMap{{"hint-14", {"P8"}}}});
    AttributeMatcher negated("hint-14", {AttributeMap{{"!hint-14", {"P8"}}}});

    auto totalMatches = total.findAll(chorale, 1);
    auto positiveMatches = positive.findAll(chorale, 1);
    auto negatedMatches = negated.findAll(chorale, 1);

    REQUIRE(!totalMatches.empty());
    CHECK_EQ(positiveMatches.size() + negatedMatches.size(), totalMatches.size());
}

TEST_CASE(matcher_negated_key_as_a_cross_referenced_constraint) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    // Driving off kern, negating a cross-referenced deg constraint -- not just the
    // driving feature itself.
    AttributeMatcher total("kern", {AttributeMap{}});
    AttributeMatcher positive("kern", {AttributeMap{{"deg", {"1"}}}});
    AttributeMatcher negated("kern", {AttributeMap{{"!deg", {"1"}}}});

    auto totalMatches = total.findAll(chorale, 1);
    auto positiveMatches = positive.findAll(chorale, 1);
    auto negatedMatches = negated.findAll(chorale, 1);

    REQUIRE(!totalMatches.empty());
    CHECK_EQ(positiveMatches.size() + negatedMatches.size(), totalMatches.size());
}

TEST_CASE(matcher_duration_split_matches_a_note_written_as_repeated_shorter_notes) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor001"));
    // The alto opens on a quarter d (pickup) that is re-attacked as another quarter d in
    // bar 1 -- a half note's worth of d, but never written as one.
    AttributeMap position{{"kern", {"d"}}, {"duration", {"2"}}};
    MatcherOptions splitOptions;
    splitOptions.durationAllowSplitNotes = true;
    AttributeMatcher withoutSplits("kern", {position});
    AttributeMatcher withSplits("kern", {position}, splitOptions);

    auto plainMatches = withoutSplits.findAll(chorale, 3);
    auto splitMatches = withSplits.findAll(chorale, 3);

    REQUIRE(!plainMatches.empty());
    CHECK_EQ(splitMatches.size(), plainMatches.size() + 1);
    // The extra match is the split one, and it spans both onsets of the run.
    CHECK_EQ(splitMatches[0].startPosition, 0);
    CHECK_EQ(splitMatches[0].endPosition, 1);
}

TEST_CASE(matcher_duration_split_requires_the_onsets_to_be_the_same_note) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor001"));
    // Two quarters also add up to a half note when they're different notes (the alto's
    // d-e in bar 1) -- that isn't one logical note and must not match.
    MatcherOptions splitOptions;
    splitOptions.durationAllowSplitNotes = true;
    AttributeMatcher withSplits("kern", {AttributeMap{{"duration", {"2"}}}}, splitOptions);

    for (const auto& m : withSplits.findAll(chorale, 3)) {
        CHECK(m.endPosition - m.startPosition < 2); // no run of two quarters d + e
    }
}

TEST_CASE(matcher_duration_split_continues_the_pattern_after_the_run) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor001"));
    // The repeated d's are followed by a quarter e: the split position consumes two onsets,
    // the next position is checked against the third.
    std::vector<AttributeMap> pattern{AttributeMap{{"kern", {"d"}}, {"duration", {"2"}}},
                                       AttributeMap{{"kern", {"e"}}, {"duration", {"4"}}}};
    MatcherOptions splitOptions;
    splitOptions.durationAllowSplitNotes = true;

    CHECK(AttributeMatcher("kern", pattern).findAll(chorale, 3).empty());
    auto splitMatches = AttributeMatcher("kern", pattern, splitOptions).findAll(chorale, 3);
    REQUIRE(splitMatches.size() == 1u);
    CHECK_EQ(splitMatches[0].startPosition, 0);
    CHECK_EQ(splitMatches[0].endPosition, 2);
}

TEST_CASE(matcher_duration_split_leaves_a_wildcard_duration_alone) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor001"));
    // Nothing to sum towards, so the position stays a plain one-onset check.
    AttributeMap position{{"duration", {"*"}}};
    MatcherOptions splitOptions;
    splitOptions.durationAllowSplitNotes = true;

    CHECK_EQ(AttributeMatcher("kern", {position}, splitOptions).findAll(chorale, 3).size(),
              AttributeMatcher("kern", {position}).findAll(chorale, 3).size());
}

TEST_CASE(matcher_duration_split_works_off_a_mint_driving_feature) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor001"));
    // **mint writes a unison for a re-attacked note, which is how a split run is recognised
    // when mint is the feature being walked.
    AttributeMap position{{"mint", {"+2"}}, {"duration", {"2"}}};
    MatcherOptions splitOptions;
    splitOptions.durationAllowSplitNotes = true;

    auto plainMatches = AttributeMatcher("mint", {position}).findAll(chorale, 3);
    auto splitMatches = AttributeMatcher("mint", {position}, splitOptions).findAll(chorale, 3);
    REQUIRE(!plainMatches.empty());
    CHECK_EQ(splitMatches.size(), plainMatches.size() + 1);
}

TEST_CASE(matcher_duration_split_takes_an_octave_leap_for_a_re_attack_under_complementation) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor001"));
    // The bass leaps GG -> G over the first barline (**mint "+P8"): the same note an octave
    // up, twice a quarter. P8 is P1's complement, so the run only closes over it once the
    // query has opted into interval complementation.
    AttributeMap position{{"duration", {"2"}}};
    MatcherOptions splitOptions;
    splitOptions.durationAllowSplitNotes = true;
    MatcherOptions splitAndComplementOptions = splitOptions;
    splitAndComplementOptions.mintAllowIntervalComplementation = {"1"};

    auto splitMatches = AttributeMatcher("mint", {position}, splitOptions).findAll(chorale, 1);
    auto complementMatches = AttributeMatcher("mint", {position}, splitAndComplementOptions).findAll(chorale, 1);

    auto startsAtZero = [](const auto& matches) {
        return std::any_of(matches.begin(), matches.end(), [](const auto& m) {
            return m.startPosition == 0 && m.endPosition == 1;
        });
    };
    CHECK(!startsAtZero(splitMatches));
    CHECK(startsAtZero(complementMatches));
    CHECK_EQ(complementMatches.size(), splitMatches.size() + 1);
}

TEST_CASE(matcher_duration_split_octave_re_attack_needs_the_unison_octave_pair_opted_in) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor001"));
    // Only "1", "8" or "*" unlock the octave re-attack -- opting in an unrelated number says
    // nothing about octaves and leaves the run exactly as it was without the option.
    AttributeMap position{{"duration", {"2"}}};
    MatcherOptions options;
    options.durationAllowSplitNotes = true;
    auto matchesWith = [&](std::vector<std::string> complementation) {
        options.mintAllowIntervalComplementation = std::move(complementation);
        return AttributeMatcher("mint", {position}, options).findAll(chorale, 1).size();
    };

    std::size_t withoutOctaves = matchesWith({});
    CHECK_EQ(matchesWith({"5"}), withoutOctaves);
    CHECK_EQ(matchesWith({"2", "3"}), withoutOctaves);
    CHECK_EQ(matchesWith({"1"}), withoutOctaves + 1);
    CHECK_EQ(matchesWith({"8"}), withoutOctaves + 1);
    CHECK_EQ(matchesWith({"*"}), withoutOctaves + 1);
    CHECK_EQ(matchesWith({"5", "8"}), withoutOctaves + 1);
}

TEST_CASE(matcher_duration_split_octave_re_attack_does_not_apply_to_a_kern_driving_feature) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor001"));
    // The same GG -> G leap, walked as **kern: complementation is a mint notion, so the run
    // still needs the identical pitch and this stays two separate quarters.
    AttributeMap position{{"duration", {"2"}}};
    MatcherOptions options;
    options.durationAllowSplitNotes = true;
    options.mintAllowIntervalComplementation = {"*"};

    auto matches = AttributeMatcher("kern", {position}, options).findAll(chorale, 1);
    CHECK(std::none_of(matches.begin(), matches.end(),
                        [](const auto& m) { return m.startPosition == 0; }));
}

TEST_CASE(matcher_duration_split_keeps_every_plain_match_it_had_without_the_option) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    AttributeMap position{{"duration", {"2"}}};
    MatcherOptions splitOptions;
    splitOptions.durationAllowSplitNotes = true;

    for (std::size_t voice = 1; voice <= 4; ++voice) {
        auto plainMatches = AttributeMatcher("kern", {position}).findAll(chorale, voice);
        auto splitMatches = AttributeMatcher("kern", {position}, splitOptions).findAll(chorale, voice);
        for (const auto& plain : plainMatches) {
            bool kept = std::any_of(splitMatches.begin(), splitMatches.end(), [&](const auto& s) {
                return s.startPosition == plain.startPosition;
            });
            CHECK(kept);
        }
    }
}

TEST_CASE(matcher_duration_merge_matches_positions_written_as_one_longer_note) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor009"));
    // Bar 4's soprano is a half note a followed by the fermata g -- what the other three
    // voices spell out as a, a, g. Asked for as three quarters, only merging finds it.
    std::vector<AttributeMap> pattern{AttributeMap{{"kern", {"a"}}, {"duration", {"4"}}},
                                       AttributeMap{{"kern", {"a"}}, {"duration", {"4"}}},
                                       AttributeMap{{"kern", {"g"}}, {"duration", {"4"}}, {"fermata", {"true"}}}};
    MatcherOptions mergeOptions;
    mergeOptions.durationAllowMergedNotes = true;

    CHECK(AttributeMatcher("kern", pattern).findAll(chorale, 4).empty());
    auto mergedMatches = AttributeMatcher("kern", pattern, mergeOptions).findAll(chorale, 4);
    REQUIRE(mergedMatches.size() == 2u);
    // The first two positions share the half note, so the match spans two onsets, not three.
    CHECK_EQ(mergedMatches[0].startPosition, 13);
    CHECK_EQ(mergedMatches[0].endPosition, 15);
}

TEST_CASE(matcher_duration_merge_requires_the_positions_to_add_up_exactly) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor009"));
    // A quarter plus a half overshoots a half note, so no run of these two positions can
    // divide one up -- and no onset in the voice is a dotted whole either.
    MatcherOptions mergeOptions;
    mergeOptions.durationAllowMergedNotes = true;
    std::vector<AttributeMap> pattern{AttributeMap{{"duration", {"4"}}}, AttributeMap{{"duration", {"2"}}}};

    auto plainMatches = AttributeMatcher("kern", pattern).findAll(chorale, 4);
    auto mergedMatches = AttributeMatcher("kern", pattern, mergeOptions).findAll(chorale, 4);
    CHECK_EQ(mergedMatches.size(), plainMatches.size());
}

TEST_CASE(matcher_duration_merge_continues_the_pattern_after_the_run) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor009"));
    // The merged run consumes the half note alone; the position after it is checked against
    // the next onset, the fermata g.
    std::vector<AttributeMap> pattern{AttributeMap{{"duration", {"4"}}},
                                       AttributeMap{{"duration", {"4"}}},
                                       AttributeMap{{"kern", {"g"}}, {"fermata", {"true"}}}};
    MatcherOptions mergeOptions;
    mergeOptions.durationAllowMergedNotes = true;

    auto mergedMatches = AttributeMatcher("kern", pattern, mergeOptions).findAll(chorale, 4);
    bool foundBar4 = std::any_of(mergedMatches.begin(), mergedMatches.end(),
                                  [](const auto& m) { return m.startPosition == 13 && m.endPosition == 15; });
    CHECK(foundBar4);
}

TEST_CASE(matcher_duration_merge_leaves_a_wildcard_duration_alone) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor009"));
    // Nothing to divide the onset up by, so the position stays a plain one-onset check.
    AttributeMap position{{"duration", {"*"}}};
    MatcherOptions mergeOptions;
    mergeOptions.durationAllowMergedNotes = true;

    CHECK_EQ(AttributeMatcher("kern", {position}, mergeOptions).findAll(chorale, 4).size(),
              AttributeMatcher("kern", {position}).findAll(chorale, 4).size());
}

TEST_CASE(matcher_duration_merge_checks_every_other_key_against_the_merged_onset) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor009"));
    // Both positions of the run describe the same single onset, so a fermata asked for by
    // either of them is the one that onset carries -- bar 4's half note has none.
    MatcherOptions mergeOptions;
    mergeOptions.durationAllowMergedNotes = true;
    std::vector<AttributeMap> pattern{AttributeMap{{"kern", {"a"}}, {"duration", {"4"}}},
                                       AttributeMap{{"duration", {"4"}}, {"fermata", {"true"}}}};

    CHECK(AttributeMatcher("kern", pattern, mergeOptions).findAll(chorale, 4).empty());
}

TEST_CASE(matcher_duration_merge_holds_a_kern_continuation_to_the_merged_notes_pitch) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor009"));
    // A merged run re-articulates the note it started on, so a "kern" pitch on a later
    // position is that same pitch. Asking for a different one must not simply be waved
    // through -- the whole run would otherwise collapse onto one note of the wrong shape.
    MatcherOptions mergeOptions;
    mergeOptions.durationAllowMergedNotes = true;

    std::vector<AttributeMap> samePitch{AttributeMap{{"kern", {"a"}}, {"duration", {"4"}}},
                                         AttributeMap{{"kern", {"a"}}, {"duration", {"4"}}}};
    std::vector<AttributeMap> otherPitch{AttributeMap{{"kern", {"a"}}, {"duration", {"4"}}},
                                          AttributeMap{{"kern", {"b"}}, {"duration", {"4"}}}};

    // Both patterns also match plain pairs of written quarters, so it is the matches that
    // collapse onto a single onset -- the merged ones -- that this is about.
    auto onOneOnset = [](const auto& m) { return m.startPosition == m.endPosition; };
    auto samePitchMatches = AttributeMatcher("kern", samePitch, mergeOptions).findAll(chorale, 4);
    auto otherPitchMatches = AttributeMatcher("kern", otherPitch, mergeOptions).findAll(chorale, 4);
    CHECK_EQ(std::count_if(samePitchMatches.begin(), samePitchMatches.end(), onOneOnset), 3);
    CHECK_EQ(std::count_if(otherPitchMatches.begin(), otherPitchMatches.end(), onOneOnset), 0);
}

TEST_CASE(matcher_duration_merge_does_not_let_a_whole_pattern_collapse_onto_one_note) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor039"));
    // Every position of the pattern adding up to a single onset's duration is a legitimate
    // run, but only if the onset really answers all of them. A pattern whose last position
    // asks for a different pitch than its first cannot be one note, however neatly the
    // durations divide it.
    MatcherOptions mergeOptions;
    mergeOptions.durationAllowMergedNotes = true;
    std::vector<AttributeMap> pattern{AttributeMap{{"kern", {"a"}}, {"duration", {"4"}}},
                                       AttributeMap{{"kern", {"a"}}, {"duration", {"4"}}},
                                       AttributeMap{{"kern", {"g"}}, {"duration", {"4"}}, {"fermata", {"true"}}}};

    for (std::size_t voice = 1; voice <= 4; ++voice) {
        for (const auto& m : AttributeMatcher("kern", pattern, mergeOptions).findAll(chorale, voice)) {
            CHECK(m.endPosition > m.startPosition); // never all three positions on one onset
        }
    }
}

TEST_CASE(matcher_duration_merge_ignores_only_the_rhythm_of_a_kern_continuation) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor009"));
    // The rhythm spelled into a "kern" value is the one thing a merged onset cannot answer
    // for a later position: its duration is the whole note's, not that position's share of
    // it. That component is dropped -- "duration" is what constrains the share -- while the
    // pitch beside it still has to hold.
    MatcherOptions mergeOptions;
    mergeOptions.durationAllowMergedNotes = true;

    std::vector<AttributeMap> withRhythm{AttributeMap{{"kern", {"a"}}, {"duration", {"4"}}},
                                          AttributeMap{{"kern", {"4a"}}, {"duration", {"4"}}}};
    std::vector<AttributeMap> withRhythmWrongPitch{AttributeMap{{"kern", {"a"}}, {"duration", {"4"}}},
                                                    AttributeMap{{"kern", {"4b"}}, {"duration", {"4"}}}};

    // Same merged runs as the bare "a"/"b" above: the "4" changed nothing, the pitch did.
    auto onOneOnset = [](const auto& m) { return m.startPosition == m.endPosition; };
    auto rhythmMatches = AttributeMatcher("kern", withRhythm, mergeOptions).findAll(chorale, 4);
    auto wrongPitchMatches = AttributeMatcher("kern", withRhythmWrongPitch, mergeOptions).findAll(chorale, 4);
    CHECK_EQ(std::count_if(rhythmMatches.begin(), rhythmMatches.end(), onOneOnset), 3);
    CHECK_EQ(std::count_if(wrongPitchMatches.begin(), wrongPitchMatches.end(), onOneOnset), 0);
}

TEST_CASE(matcher_duration_merge_checks_a_non_driving_features_continuation_against_the_onset) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor009"));
    // Everything that isn't "mint" or "kern" describes the merged note itself, which a
    // re-attack of it shares -- so a later position's "deg" is judged against the onset like
    // any other key, whether or not "deg" happens to drive the walk.
    MatcherOptions mergeOptions;
    mergeOptions.durationAllowMergedNotes = true;

    auto onOneOnset = [](const auto& m) { return m.startPosition == m.endPosition; };
    for (const std::string& feature : {"kern", "deg"}) {
        auto degOf = [&](const std::string& deg) {
            return std::vector<AttributeMap>{AttributeMap{{"kern", {"a"}}, {"duration", {"4"}}},
                                              AttributeMap{{"deg", {deg}}, {"duration", {"4"}}}};
        };
        auto matching = AttributeMatcher(feature, degOf("2"), mergeOptions).findAll(chorale, 4);
        auto mismatching = AttributeMatcher(feature, degOf("7"), mergeOptions).findAll(chorale, 4);
        // the merged a's are scale degree 2 in G
        CHECK_EQ(std::count_if(matching.begin(), matching.end(), onOneOnset), 2);
        CHECK_EQ(std::count_if(mismatching.begin(), mismatching.end(), onOneOnset), 0);
    }
}

TEST_CASE(matcher_duration_merge_judges_a_mint_continuation_as_a_re_attack_under_any_feature) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    // The bass's half note D before the final fermata, asked for as a dotted quarter plus a
    // repeated eighth. Position 1's "mint" describes the step into a re-attack the merged
    // note doesn't have, so it is judged against the unison such a re-attack would be -- and
    // that has to hold no matter which feature drives the walk.
    std::vector<AttributeMap> pattern{AttributeMap{{"duration", {"4."}}},
                                       AttributeMap{{"mint", {"P1"}}, {"duration", {"8"}}},
                                       AttributeMap{{"mint", {"-P5"}}, {"fermata", {"true"}}}};
    MatcherOptions mergeOptions;
    mergeOptions.durationAllowMergedNotes = true;
    mergeOptions.mintAllowIntervalComplementation = {"*"};

    for (const std::string& feature : {"deg", "mint", "kern"}) {
        auto matches = AttributeMatcher(feature, pattern, mergeOptions).findAll(chorale, 1);
        REQUIRE(matches.size() == 1u);
        CHECK_EQ(matches[0].startPosition, 48);
        CHECK_EQ(matches[0].endPosition, 50);
    }
}

TEST_CASE(matcher_duration_merge_rejects_a_mint_continuation_that_is_no_re_attack) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    // Nothing inside a merged note can be a descending fifth -- there is no second attack for
    // the interval to be measured into.
    std::vector<AttributeMap> pattern{AttributeMap{{"duration", {"4."}}},
                                       AttributeMap{{"mint", {"-P5"}}, {"duration", {"8"}}},
                                       AttributeMap{{"mint", {"-P5"}}, {"fermata", {"true"}}}};
    MatcherOptions mergeOptions;
    mergeOptions.durationAllowMergedNotes = true;
    mergeOptions.mintAllowIntervalComplementation = {"*"};

    for (const std::string& feature : {"deg", "mint", "kern"}) {
        CHECK(AttributeMatcher(feature, pattern, mergeOptions).findAll(chorale, 1).empty());
    }
}

TEST_CASE(matcher_duration_merge_takes_an_octave_mint_continuation_only_when_opted_in) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    // An octave leap passes for a re-attack on the same terms durationAllowSplitNotes gives
    // it: the unison-octave pair has to be opted into complementation.
    std::vector<AttributeMap> pattern{AttributeMap{{"duration", {"4."}}},
                                       AttributeMap{{"mint", {"P8"}}, {"duration", {"8"}}},
                                       AttributeMap{{"mint", {"-P5"}}, {"fermata", {"true"}}}};
    MatcherOptions mergeOptions;
    mergeOptions.durationAllowMergedNotes = true;

    mergeOptions.mintAllowIntervalComplementation = {"5"}; // enough for position 2, not for the octave
    CHECK(AttributeMatcher("deg", pattern, mergeOptions).findAll(chorale, 1).empty());

    mergeOptions.mintAllowIntervalComplementation = {"5", "8"};
    CHECK_EQ(AttributeMatcher("deg", pattern, mergeOptions).findAll(chorale, 1).size(), std::size_t{1});
}

TEST_CASE(matcher_duration_merge_keeps_every_plain_match_it_had_without_the_option) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    AttributeMap position{{"duration", {"2"}}};
    MatcherOptions mergeOptions;
    mergeOptions.durationAllowMergedNotes = true;

    for (std::size_t voice = 1; voice <= 4; ++voice) {
        auto plainMatches = AttributeMatcher("kern", {position}).findAll(chorale, voice);
        auto mergedMatches = AttributeMatcher("kern", {position}, mergeOptions).findAll(chorale, voice);
        CHECK_EQ(plainMatches.size(), mergedMatches.size()); // a lone position has nothing to merge with
        for (const auto& plain : plainMatches) {
            bool kept = std::any_of(mergedMatches.begin(), mergedMatches.end(), [&](const auto& m) {
                return m.startPosition == plain.startPosition;
            });
            CHECK(kept);
        }
    }
}

TEST_CASE(matcher_duration_merge_and_split_can_be_combined) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor009"));
    // The two options describe opposite mismatches between pattern and score, so switching
    // both on keeps each of them working. In the soprano, bar 4's half note a answers a
    // pattern of two quarters (merged), and bar 5's repeated quarter cc answers a pattern of
    // one half note (split).
    MatcherOptions bothOptions;
    bothOptions.durationAllowSplitNotes = true;
    bothOptions.durationAllowMergedNotes = true;

    std::vector<AttributeMap> mergePattern{AttributeMap{{"kern", {"a"}}, {"duration", {"4"}}},
                                            AttributeMap{{"duration", {"4"}}},
                                            AttributeMap{{"kern", {"g"}}, {"fermata", {"true"}}}};
    std::vector<AttributeMap> splitPattern{AttributeMap{{"kern", {"cc"}}, {"duration", {"2"}}}};

    CHECK(AttributeMatcher("kern", mergePattern).findAll(chorale, 4).empty());
    CHECK(AttributeMatcher("kern", splitPattern).findAll(chorale, 4).empty());

    auto mergedMatches = AttributeMatcher("kern", mergePattern, bothOptions).findAll(chorale, 4);
    CHECK(std::any_of(mergedMatches.begin(), mergedMatches.end(),
                       [](const auto& m) { return m.startPosition == 13 && m.endPosition == 15; }));
    CHECK_EQ(AttributeMatcher("kern", splitPattern, bothOptions).findAll(chorale, 4).size(), std::size_t{1});
}

TEST_CASE(matcher_metweight_skip_takes_ornamental_onsets_out_of_the_walk) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor005"));
    // The soprano's bar 4 is 8a 8g 4a 4g; -- the offbeat g is a lower neighbour, unclassified
    // by **metweight. Skipped, it is no onset of its own any more, so the note right after
    // bar 4's downbeat is the second a rather than that g.
    MatcherOptions skipOptions;
    skipOptions.metweightSkipUnclassified = true;

    std::vector<AttributeMap> aThenG{AttributeMap{{"kern", {"a"}}}, AttributeMap{{"kern", {"g"}}}};
    std::vector<AttributeMap> aThenA{AttributeMap{{"kern", {"a"}}}, AttributeMap{{"kern", {"a"}}}};
    auto startsAt13 = [](const auto& m) { return m.startPosition == 13; };

    auto plainNeighbour = AttributeMatcher("kern", aThenG).findAll(chorale, 4);
    auto skippedNeighbour = AttributeMatcher("kern", aThenG, skipOptions).findAll(chorale, 4);
    CHECK(std::any_of(plainNeighbour.begin(), plainNeighbour.end(), startsAt13));
    CHECK(!std::any_of(skippedNeighbour.begin(), skippedNeighbour.end(), startsAt13));

    auto plainRepeat = AttributeMatcher("kern", aThenA).findAll(chorale, 4);
    auto skippedRepeat = AttributeMatcher("kern", aThenA, skipOptions).findAll(chorale, 4);
    CHECK(!std::any_of(plainRepeat.begin(), plainRepeat.end(), startsAt13));
    CHECK(std::any_of(skippedRepeat.begin(), skippedRepeat.end(), startsAt13));
}

TEST_CASE(matcher_metweight_skip_hands_the_ornaments_duration_to_the_note_it_decorates) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor008"));
    // The soprano's bar 4 is 4.b- 8a- 2a-; -- the offbeat a- anticipates the fermata note and
    // takes its time from the b- in front of it. With the ornament gone that b- is what it
    // musically is, a half note, for "duration" and for a "kern" value's own rhythm alike.
    MatcherOptions skipOptions;
    skipOptions.metweightSkipUnclassified = true;
    auto startsAt12 = [](const auto& m) { return m.startPosition == 12; };

    for (const AttributeMap& halfNoteBFlat :
         {AttributeMap{{"kern", {"b-"}}, {"duration", {"2"}}}, AttributeMap{{"kern", {"2b-"}}}}) {
        auto plain = AttributeMatcher("kern", {halfNoteBFlat}).findAll(chorale, 4);
        auto skipped = AttributeMatcher("kern", {halfNoteBFlat}, skipOptions).findAll(chorale, 4);
        CHECK(!std::any_of(plain.begin(), plain.end(), startsAt12));
        CHECK(std::any_of(skipped.begin(), skipped.end(), startsAt12));
    }
}

TEST_CASE(matcher_metweight_skip_measures_mint_across_the_skipped_ornament) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor005"));
    // Bar 3's last beat is 8cc 8b, bar 4 opens on 8a: **mint writes -M2 into that a, measured
    // from the passing b. With the b skipped the step is the one the phrase actually makes,
    // cc down to a -- a minor third.
    MatcherOptions skipOptions;
    skipOptions.metweightSkipUnclassified = true;
    auto startsAt13 = [](const auto& m) { return m.startPosition == 13; };

    AttributeMap writtenStep{{"kern", {"a"}}, {"mint", {"-M2"}}};
    AttributeMap stepAcrossTheOrnament{{"kern", {"a"}}, {"mint", {"-m3"}}};

    auto written = AttributeMatcher("kern", {writtenStep}, skipOptions).findAll(chorale, 4);
    auto across = AttributeMatcher("kern", {stepAcrossTheOrnament}, skipOptions).findAll(chorale, 4);
    CHECK(!std::any_of(written.begin(), written.end(), startsAt13));
    CHECK(std::any_of(across.begin(), across.end(), startsAt13));

    // Only the interval into the note *after* an ornament is recomputed. Bar 4's second a
    // follows the skipped neighbour g, so it reads as the repetition it is (P1) rather than
    // as the +M2 the spine wrote; the fermata g after it has no ornament in front of it and
    // keeps the spine's own -M2.
    std::vector<AttributeMap> reducedBar4{AttributeMap{{"kern", {"a"}}, {"mint", {"-m3"}}},
                                           AttributeMap{{"kern", {"a"}}, {"mint", {"P1"}}},
                                           AttributeMap{{"kern", {"g"}}, {"mint", {"-M2"}}}};
    auto matches = AttributeMatcher("kern", reducedBar4, skipOptions).findAll(chorale, 4);
    REQUIRE(matches.size() == 1u);
    CHECK_EQ(matches[0].startPosition, 13);
    CHECK_EQ(matches[0].endPosition, 15);
}

TEST_CASE(matcher_metweight_skip_never_skips_a_rest_or_a_voices_first_onset) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor006"));
    // **metweight calls every rest unclassified wherever it falls, and chor006's bar 5 is a
    // quarter rest in all four voices -- skipping those would silently swallow the rest and
    // hand its time to the note before it.
    MatcherOptions skipOptions;
    skipOptions.metweightSkipUnclassified = true;
    AttributeMap rest{{"kern", {"r"}}};

    for (std::size_t voice = 1; voice <= 4; ++voice) {
        CHECK_EQ(AttributeMatcher("kern", {rest}, skipOptions).findAll(chorale, voice).size(),
                  AttributeMatcher("kern", {rest}).findAll(chorale, voice).size());
    }

    // A voice's very first onset has no note in front of it to decorate, so it stays a note of
    // its own however its own metric position is classified.
    AttributeMap anyOnset{{"kern", {"*"}}};
    for (std::size_t voice = 1; voice <= 4; ++voice) {
        auto plain = AttributeMatcher("kern", {anyOnset}).findAll(chorale, voice);
        auto skipped = AttributeMatcher("kern", {anyOnset}, skipOptions).findAll(chorale, voice);
        REQUIRE(!plain.empty());
        REQUIRE(!skipped.empty());
        CHECK_EQ(skipped[0].startPosition, plain[0].startPosition);
    }
}

TEST_CASE(matcher_metweight_skip_keeps_an_ornament_free_voice_exactly_as_it_was) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor009"));
    // Nothing to fold away means nothing changes: same onsets, same durations, same intervals.
    MatcherOptions skipOptions;
    skipOptions.metweightSkipUnclassified = true;
    AttributeMap anyOnset{{"kern", {"*"}}};

    auto hasNoUnclassifiedOnset = [&](std::size_t voice) {
        return AttributeMatcher("kern", {AttributeMap{{"metweight", {"u"}}}}).findAll(chorale, voice).empty();
    };
    for (std::size_t voice = 1; voice <= 4; ++voice) {
        if (!hasNoUnclassifiedOnset(voice)) continue;
        auto plain = AttributeMatcher("kern", {anyOnset}).findAll(chorale, voice);
        auto skipped = AttributeMatcher("kern", {anyOnset}, skipOptions).findAll(chorale, voice);
        REQUIRE(plain.size() == skipped.size());
        for (std::size_t i = 0; i < plain.size(); ++i) CHECK_EQ(skipped[i].startPosition, plain[i].startPosition);
    }
}

TEST_CASE(matcher_metweight_skip_lets_a_split_run_close_over_an_ornament) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor005"));
    // Bar 4's soprano a is a half note's worth of a, written 8a 8g 4a with a neighbour note in
    // the middle. Skipping the neighbour makes the two a's adjacent, which is what lets a split
    // run join them -- neither option finds it alone.
    AttributeMap halfNoteA{{"kern", {"a"}}, {"duration", {"2"}}};
    MatcherOptions splitOnly;
    splitOnly.durationAllowSplitNotes = true;
    MatcherOptions skipOnly;
    skipOnly.metweightSkipUnclassified = true;
    MatcherOptions bothOptions;
    bothOptions.durationAllowSplitNotes = true;
    bothOptions.metweightSkipUnclassified = true;
    auto startsAt13 = [](const auto& m) { return m.startPosition == 13; };

    for (const MatcherOptions& tooLittle : {MatcherOptions{}, splitOnly, skipOnly}) {
        auto matches = AttributeMatcher("kern", {halfNoteA}, tooLittle).findAll(chorale, 4);
        CHECK(!std::any_of(matches.begin(), matches.end(), startsAt13));
    }
    auto matches = AttributeMatcher("kern", {halfNoteA}, bothOptions).findAll(chorale, 4);
    CHECK(std::any_of(matches.begin(), matches.end(), startsAt13));
}

TEST_MAIN()


// "phrase" pattern key against the fixtures. Every voice of a fixture ends its phrases on a fermata and
// the piece closes on one, so a phrase starts at the first note and after every fermata but the last,
// and ends on every fermata.
TEST_CASE(matcher_phrase_start_is_the_first_note_and_the_note_after_each_fermata) {
    AttributeMatcher starts("kern", {AttributeMap{{"phrase", {"start"}}}});
    AttributeMatcher fermata("kern", {AttributeMap{{"fermata", {"true"}}}});
    for (const char* id : {"chor001", "chor005", "chor006", "chor008", "chor009", "chor029", "chor039", "chor103"}) {
        HumdrumChorale chorale(FIXTURE_CHORALE(id));
        for (std::size_t voice = 1; voice <= 4; ++voice) {
            // One start for the first phrase and one for each fermata that has a note after it.
            CHECK_EQ(starts.findAll(chorale, voice).size(), fermata.findAll(chorale, voice).size());
        }
    }
}

TEST_CASE(matcher_phrase_end_is_every_note_with_a_fermata) {
    AttributeMatcher ends("kern", {AttributeMap{{"phrase", {"end"}}}});
    AttributeMatcher fermata("kern", {AttributeMap{{"fermata", {"true"}}}});
    for (const char* id : {"chor001", "chor005", "chor006", "chor008", "chor009", "chor029", "chor039", "chor103"}) {
        HumdrumChorale chorale(FIXTURE_CHORALE(id));
        for (std::size_t voice = 1; voice <= 4; ++voice) {
            auto fermataMatches = fermata.findAll(chorale, voice);
            REQUIRE(!fermataMatches.empty());
            CHECK_EQ(ends.findAll(chorale, voice).size(), fermataMatches.size());
        }
    }
}

TEST_CASE(matcher_phrase_start_middle_and_end_cover_every_note_once) {
    AttributeMatcher starts("kern", {AttributeMap{{"phrase", {"start"}}}});
    AttributeMatcher ends("kern", {AttributeMap{{"phrase", {"end"}}}});
    AttributeMatcher middle("kern", {AttributeMap{{"!phrase", {"start", "end"}}}});
    AttributeMatcher all("kern", {AttributeMap{{"kern", {"*"}}}});
    for (const char* id : {"chor001", "chor005", "chor006", "chor008", "chor009", "chor029", "chor039", "chor103"}) {
        HumdrumChorale chorale(FIXTURE_CHORALE(id));
        for (std::size_t voice = 1; voice <= 4; ++voice) {
            // No note is a start and an end at once in these chorales.
            CHECK_EQ(starts.findAll(chorale, voice).size() + ends.findAll(chorale, voice).size() +
                          middle.findAll(chorale, voice).size(),
                      all.findAll(chorale, voice).size());
        }
    }
}

TEST_CASE(matcher_phrase_values_are_an_or_list_and_a_negation_covers_the_rest) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    auto count = [&](const AttributeMap& position) {
        return AttributeMatcher("kern", {position}).findAll(chorale, 1).size();
    };
    std::size_t total = count({{"kern", {"*"}}});
    std::size_t starts = count({{"phrase", {"start"}}});
    std::size_t ends = count({{"phrase", {"end"}}});
    CHECK_EQ(count({{"phrase", {"start", "end"}}}), starts + ends);
    CHECK_EQ(count({{"!phrase", {"start"}}}), total - starts);
    CHECK_EQ(count({{"!phrase", {"end"}}}), total - ends);
    CHECK_EQ(count({{"phrase", {"*"}}}), total);
}

TEST_CASE(matcher_phrase_combines_with_other_keys_of_the_same_position) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    auto count = [&](const AttributeMap& position) {
        return AttributeMatcher("kern", {position}).findAll(chorale, 1).size();
    };
    CHECK_EQ(count({{"phrase", {"end"}}, {"fermata", {"true"}}}), count({{"fermata", {"true"}}}));
    CHECK_EQ(count({{"phrase", {"end"}}, {"duration", {"2"}}}), count({{"fermata", {"true"}}, {"duration", {"2"}}}));
    CHECK_EQ(count({{"phrase", {"start"}}, {"fermata", {"false"}}}), count({{"phrase", {"start"}}}));
    CHECK_EQ(count({{"phrase", {"start"}}, {"fermata", {"true"}}}), std::size_t{0});
}

TEST_CASE(matcher_phrase_works_across_the_positions_of_a_pattern) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    auto count = [&](const std::vector<AttributeMap>& pattern) {
        return AttributeMatcher("kern", pattern).findAll(chorale, 1).size();
    };
    AttributeMap start{{"phrase", {"start"}}};
    AttributeMap end{{"phrase", {"end"}}};
    AttributeMap middle{{"!phrase", {"start", "end"}}};
    // The last fermata has no note after it that could start a phrase.
    CHECK_EQ(count({end, start}), count({end}) - 1);
    // The first note of a phrase is followed by a note in the middle of it.
    CHECK_EQ(count({start, middle}), count({start}));
    // A phrase can't end twice in a row.
    CHECK_EQ(count({end, end}), std::size_t{0});
}

TEST_CASE(matcher_phrase_a_rest_after_a_fermata_does_not_start_the_phrase_but_the_note_after_it_does) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor006"));
    // Every voice of chor006 rests for a quarter after its second fermata, and the phrase starts with
    // the first sounding note after the rest, so the rest itself is in the middle.
    AttributeMatcher rests("kern", {AttributeMap{{"kern", {"r"}}}});
    AttributeMatcher restStarts("kern", {AttributeMap{{"kern", {"r"}}, {"phrase", {"start"}}}});
    AttributeMatcher restMiddles("kern", {AttributeMap{{"kern", {"r"}}, {"!phrase", {"start", "end"}}}});
    AttributeMatcher starts("kern", {AttributeMap{{"phrase", {"start"}}}});
    AttributeMatcher fermata("kern", {AttributeMap{{"fermata", {"true"}}}});
    for (std::size_t voice = 1; voice <= 4; ++voice) {
        REQUIRE(!rests.findAll(chorale, voice).empty());
        CHECK_EQ(restStarts.findAll(chorale, voice).size(), std::size_t{0});
        CHECK_EQ(restMiddles.findAll(chorale, voice).size(), rests.findAll(chorale, voice).size());
        // The rest doesn't take a phrase away: there is still one start per fermata.
        CHECK_EQ(starts.findAll(chorale, voice).size(), fermata.findAll(chorale, voice).size());
    }
}

// Split notes (durationAllowSplitNotes): a note written as repeated shorter notes is one logical
// note, which starts a phrase where its first part does and ends one where its last part does.
// The alto of chor001 opens with a quarter d that is repeated as a second quarter d.
TEST_CASE(matcher_phrase_split_run_starts_a_phrase_where_its_first_part_does) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor001"));
    MatcherOptions split;
    split.durationAllowSplitNotes = true;
    AttributeMap position{{"kern", {"d"}}, {"duration", {"2"}}, {"phrase", {"start"}}};

    auto plain = AttributeMatcher("kern", {position}).findAll(chorale, 3);
    auto withSplits = AttributeMatcher("kern", {position}, split).findAll(chorale, 3);
    // The two quarter d's that open the piece are a start together, which no half note d is.
    CHECK_EQ(withSplits.size(), plain.size() + 1);
}

TEST_CASE(matcher_phrase_split_run_does_not_start_a_phrase_with_its_second_part) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor001"));
    // Without the option the repeated quarter d's are two notes, and only the first is a start.
    auto quarters = AttributeMatcher("kern", {AttributeMap{{"kern", {"d"}}, {"duration", {"4"}}}}).findAll(chorale, 3);
    auto quarterStarts = AttributeMatcher("kern", {AttributeMap{{"kern", {"d"}}, {"duration", {"4"}},
                                                                {"phrase", {"start"}}}}).findAll(chorale, 3);
    CHECK_EQ(quarterStarts.size(), std::size_t{1});
    CHECK(quarters.size() > quarterStarts.size());
}

TEST_CASE(matcher_phrase_split_run_end_is_the_fermata_of_its_last_part) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor001"));
    MatcherOptions split;
    split.durationAllowSplitNotes = true;
    for (std::size_t voice = 1; voice <= 4; ++voice) {
        auto ends = AttributeMatcher("kern", {AttributeMap{{"duration", {"2"}}, {"phrase", {"end"}}}}, split)
                        .findAll(chorale, voice);
        auto fermatas = AttributeMatcher("kern", {AttributeMap{{"duration", {"2"}}, {"fermata", {"true"}}}}, split)
                            .findAll(chorale, voice);
        CHECK_EQ(ends.size(), fermatas.size());
    }
}

TEST_CASE(matcher_phrase_split_run_negation_judges_the_whole_run) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor001"));
    MatcherOptions split;
    split.durationAllowSplitNotes = true;
    auto count = [&](const AttributeMap& position) {
        return AttributeMatcher("kern", {position}, split).findAll(chorale, 3).size();
    };
    // Every run is either one or the other.
    for (const char* role : {"start", "end"}) {
        CHECK_EQ(count({{"kern", {"d"}}, {"duration", {"2"}}, {"phrase", {role}}}) +
                      count({{"kern", {"d"}}, {"duration", {"2"}}, {"!phrase", {role}}}),
                  count({{"kern", {"d"}}, {"duration", {"2"}}}));
    }
}

// Merged notes (durationAllowMergedNotes): two quarter positions share one longer written note, the
// half note a before the fermata g in the soprano of chor009 (quarters 13 to 15).
TEST_CASE(matcher_phrase_merged_second_position_is_never_a_start) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor009"));
    MatcherOptions merge;
    merge.durationAllowMergedNotes = true;
    AttributeMap first{{"kern", {"a"}}, {"duration", {"4"}}};
    auto count = [&](const AttributeMap& second) {
        return AttributeMatcher("kern", {first, second}, merge).findAll(chorale, 4).size();
    };
    REQUIRE(count(first) > 0);
    CHECK_EQ(count({{"kern", {"a"}}, {"duration", {"4"}}, {"phrase", {"start"}}}), std::size_t{0});
    CHECK_EQ(count({{"kern", {"a"}}, {"duration", {"4"}}, {"!phrase", {"start"}}}), count(first));
}

TEST_CASE(matcher_phrase_merged_second_position_ends_a_phrase_with_the_written_note) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor009"));
    MatcherOptions merge;
    merge.durationAllowMergedNotes = true;
    AttributeMap first{{"kern", {"a"}}, {"duration", {"4"}}};
    auto count = [&](const AttributeMap& second) {
        return AttributeMatcher("kern", {first, second}, merge).findAll(chorale, 4).size();
    };
    CHECK_EQ(count({{"kern", {"a"}}, {"duration", {"4"}}, {"phrase", {"end"}}}),
              count({{"kern", {"a"}}, {"duration", {"4"}}, {"fermata", {"true"}}}));
}

// Comparisons ({"operator": "gt", "value": ...}) against chor029.krn. They split the onsets of a voice into
// the ones beyond a threshold and the ones at or before it, so most of these check that the two halves add
// up to the whole instead of pinning down note numbers.

TEST_CASE(matcher_compares_duration_by_length) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    auto count = [&](const AttributeMap& position) {
        return AttributeMatcher("kern", {position}).findAll(chorale, 2).size();
    };
    PatternValue longerThanQuarter(ComparisonOperator::GreaterThan, "4");
    PatternValue quarterOrLonger(ComparisonOperator::GreaterThanOrEqual, "4");
    PatternValue quarterOrShorter(ComparisonOperator::LessThanOrEqual, "4");
    PatternValue shorterThanQuarter(ComparisonOperator::LessThan, "4");

    std::size_t all = count({{"kern", {"*"}}});
    std::size_t quarters = count({{"duration", {"4"}}});
    REQUIRE(quarters > 0);
    REQUIRE(count({{"duration", {longerThanQuarter}}}) > 0);
    CHECK_EQ(count({{"duration", {longerThanQuarter}}}) + count({{"duration", {quarterOrShorter}}}), all);
    CHECK_EQ(count({{"duration", {shorterThanQuarter}}}) + count({{"duration", {quarterOrLonger}}}), all);
    CHECK_EQ(count({{"duration", {quarterOrLonger}}}), count({{"duration", {longerThanQuarter}}}) + quarters);
}

TEST_CASE(matcher_compares_duration_by_the_sounding_length_of_tied_notes) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    auto count = [&](const AttributeMap& position) {
        return AttributeMatcher("kern", {position}).findAll(chorale, 2).size();
    };
    // "[4G" tied to "8GL]" sounds as a dotted quarter: longer than a quarter, but neither shorter nor
    // longer than a dotted one.
    std::size_t dotted = count({{"duration", {"4."}}});
    REQUIRE(dotted > 0);
    CHECK(count({{"duration", {{ComparisonOperator::GreaterThan, "4"}}}}) >= dotted);
    CHECK_EQ(count({{"!duration", {{ComparisonOperator::LessThan, "4."}, {ComparisonOperator::GreaterThan, "4."}}}}),
              dotted);
}

TEST_CASE(matcher_a_comparison_in_an_or_list_is_one_more_choice) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    auto count = [&](const AttributeMap& position) {
        return AttributeMatcher("kern", {position}).findAll(chorale, 2).size();
    };
    CHECK_EQ(count({{"duration", {{ComparisonOperator::GreaterThan, "4"}, "4"}}}),
              count({{"duration", {{ComparisonOperator::GreaterThanOrEqual, "4"}}}}));
}

TEST_CASE(matcher_a_range_is_a_key_and_its_negation_in_one_position) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    auto count = [&](const AttributeMap& position) {
        return AttributeMatcher("kern", {position}).findAll(chorale, 2).size();
    };
    // From an eighth up to a quarter: at least an eighth, and not longer than a quarter.
    AttributeMap eighthToQuarter{{"duration", {{ComparisonOperator::GreaterThanOrEqual, "8"}}},
                                 {"!duration", {{ComparisonOperator::GreaterThan, "4"}}}};
    std::size_t eighths = count({{"duration", {"8"}}});
    std::size_t quarters = count({{"duration", {"4"}}});
    REQUIRE(eighths > 0);
    REQUIRE(quarters > 0);
    CHECK_EQ(count(eighthToQuarter), eighths + quarters);
}

TEST_CASE(matcher_duration_comparisons_leave_split_and_merged_notes_alone) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    // The options divide notes up by or add them up to a length, which a comparison does not name.
    MatcherOptions options;
    options.durationAllowSplitNotes = true;
    options.durationAllowMergedNotes = true;
    AttributeMap longer{{"duration", {{ComparisonOperator::GreaterThan, "4"}}}};
    CHECK_EQ(AttributeMatcher("kern", {longer}, options).findAll(chorale, 2).size(),
              AttributeMatcher("kern", {longer}).findAll(chorale, 2).size());
}

TEST_CASE(matcher_compares_pitch_with_higher_and_lower_notes) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    auto count = [&](std::size_t voice, const AttributeMap& position) {
        return AttributeMatcher("kern", {position}).findAll(chorale, voice).size();
    };
    PatternValue higherThanG(ComparisonOperator::GreaterThan, "g");
    PatternValue gOrLower(ComparisonOperator::LessThanOrEqual, "g");
    // Rests have no pitch: they are neither higher nor lower than anything.
    for (std::size_t voice : {1u, 4u}) {
        CHECK_EQ(count(voice, {{"kern", {higherThanG}}}) + count(voice, {{"kern", {gOrLower}}}),
                  count(voice, {{"!kern", {"r"}}}));
    }
    // The bass stays below middle c, the soprano never goes down to it.
    PatternValue cOrHigher(ComparisonOperator::GreaterThanOrEqual, "c");
    PatternValue lowerThanC(ComparisonOperator::LessThan, "c");
    CHECK_EQ(count(1, {{"kern", {cOrHigher}}}), std::size_t{0});
    CHECK(count(1, {{"kern", {lowerThanC}}}) > 0);
    CHECK_EQ(count(4, {{"kern", {lowerThanC}}}), std::size_t{0});
}

TEST_CASE(matcher_pitch_comparison_counts_the_octave_and_equates_enharmonics) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    auto count = [&](const std::string& pitch, MatcherOptions options = {}) {
        AttributeMap position{{"kern", {{ComparisonOperator::GreaterThan, pitch}}}};
        return AttributeMatcher("kern", {position}, options).findAll(chorale, 4).size();
    };
    // "g" is the g above middle c, "G" the one an octave below it.
    CHECK(count("G") > count("g"));
    // c# and d- are the same key.
    CHECK_EQ(count("c#"), count("d-"));
    // kernIgnoreOctave has no effect on comparisons.
    MatcherOptions ignoreOctave;
    ignoreOctave.kernIgnoreOctave = true;
    CHECK_EQ(count("g", ignoreOctave), count("g"));
}

TEST_CASE(matcher_negated_pitch_comparison_includes_rests) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor006"));
    auto count = [&](const AttributeMap& position) {
        return AttributeMatcher("kern", {position}).findAll(chorale, 4).size();
    };
    PatternValue higherThanG(ComparisonOperator::GreaterThan, "g");
    PatternValue gOrLower(ComparisonOperator::LessThanOrEqual, "g");
    // "not higher than g" is true of a rest as well as of every note at g or below.
    std::size_t rests = count({{"kern", {"r"}}});
    REQUIRE(rests > 0);
    CHECK_EQ(count({{"!kern", {higherThanG}}}), count({{"kern", {gOrLower}}}) + rests);
}

TEST_CASE(matcher_compares_melodic_interval_by_size) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    auto count = [&](const AttributeMap& position) {
        return AttributeMatcher("mint", {position}).findAll(chorale, 4).size();
    };
    PatternValue widerThanThird(ComparisonOperator::GreaterThan, "3");
    PatternValue thirdOrNarrower(ComparisonOperator::LessThanOrEqual, "3");
    // Everything but the voice's first note (which has no interval, only a bracketed pitch) falls on one
    // side or the other of a third.
    std::size_t withInterval = count({{"mint", {"*"}}}) - 1;
    REQUIRE(count({{"mint", {widerThanThird}}}) > 0);
    REQUIRE(count({{"mint", {thirdOrNarrower}}}) > 0);
    CHECK_EQ(count({{"mint", {widerThanThird}}}) + count({{"mint", {thirdOrNarrower}}}), withInterval);
}

TEST_CASE(matcher_a_sign_restricts_an_interval_comparison_to_that_direction) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    auto count = [&](const AttributeMap& position) {
        return AttributeMatcher("mint", {position}).findAll(chorale, 1).size();
    };
    auto countOf = [&](ComparisonOperator comparisonOperator, const std::string& value) {
        return count({{"mint", {{comparisonOperator, value}}}});
    };
    const ComparisonOperator greaterThan = ComparisonOperator::GreaterThan;
    const ComparisonOperator lessThanOrEqual = ComparisonOperator::LessThanOrEqual;
    // Without a sign the size alone counts, with one the interval has to go that way as well.
    REQUIRE(countOf(greaterThan, "+3") > 0);
    REQUIRE(countOf(greaterThan, "-3") > 0);
    CHECK_EQ(countOf(greaterThan, "3"), countOf(greaterThan, "+3") + countOf(greaterThan, "-3"));
    // Every interval that goes somewhere is wider than the unison, and the unison goes nowhere.
    CHECK_EQ(countOf(greaterThan, "+1"), count({{"mint", {"+"}}}));
    CHECK_EQ(countOf(greaterThan, "-1"), count({{"mint", {"-"}}}));
    // Up, wider than a third or not.
    CHECK_EQ(countOf(greaterThan, "+3") + countOf(lessThanOrEqual, "+3"), count({{"mint", {"+"}}}));
}

TEST_CASE(matcher_a_signed_interval_comparison_with_a_quality_compares_in_semitones_in_that_direction) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    auto count = [&](const AttributeMap& position) {
        return AttributeMatcher("mint", {position}).findAll(chorale, 4).size();
    };
    auto countOf = [&](ComparisonOperator comparisonOperator, const std::string& value) {
        return count({{"mint", {{comparisonOperator, value}}}});
    };
    const ComparisonOperator greaterThan = ComparisonOperator::GreaterThan;
    // Wider than a minor sixth in either direction is the two directions together.
    CHECK_EQ(countOf(greaterThan, "m6"), countOf(greaterThan, "+m6") + countOf(greaterThan, "-m6"));
    // A descent wider than a minor sixth is a descent, and no ascent is one.
    CHECK(countOf(greaterThan, "-m6") <= count({{"mint", {"-"}}}));
    CHECK_EQ(countOf(ComparisonOperator::LessThanOrEqual, "-m6") + countOf(greaterThan, "-m6"), count({{"mint", {"-"}}}));
}

TEST_CASE(matcher_interval_comparison_without_a_quality_treats_all_intervals_of_a_number_alike) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    auto count = [&](const AttributeMap& position) {
        return AttributeMatcher("mint", {position}).findAll(chorale, 4).size();
    };
    CHECK_EQ(count({{"mint", {{ComparisonOperator::GreaterThanOrEqual, "3"}}}}),
              count({{"mint", {"3", {ComparisonOperator::GreaterThan, "3"}}}}));
    CHECK_EQ(count({{"mint", {{ComparisonOperator::LessThanOrEqual, "2"}}}}),
              count({{"mint", {"1", "2", "3"}}}) - count({{"mint", {"3"}}}));
}

TEST_CASE(matcher_compares_harmonic_interval_by_size) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    auto count = [&](const AttributeMap& position, MatcherOptions options = {}) {
        return AttributeMatcher("hint-14", {position}, options).findAll(chorale, 1).size();
    };
    AttributeMap widerThanTenth{{"hint-14", {{ComparisonOperator::GreaterThan, "10"}}}};
    AttributeMap tenthOrNarrower{{"hint-14", {{ComparisonOperator::LessThanOrEqual, "10"}}}};
    REQUIRE(count(widerThanTenth) > 0);
    REQUIRE(count(tenthOrNarrower) > 0);
    CHECK_EQ(count(widerThanTenth) + count(tenthOrNarrower), count({{"hint-14", {"*"}}}));

    // hintReduceCompound folds a tenth into a third for an equal value, but a comparison asks about the
    // size as it is in the score.
    MatcherOptions reduce;
    reduce.hintReduceCompound = true;
    CHECK_EQ(count(widerThanTenth, reduce), count(widerThanTenth));
}

TEST_CASE(matcher_compares_the_interval_to_a_given_voice) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    auto count = [&](const AttributeMap& position) {
        return AttributeMatcher("kern", {position}).findAll(chorale, 4).size();
    };
    // "hint-1" seen from the soprano is the same interval as hint-14.
    PatternValue widerThanTenth(ComparisonOperator::GreaterThan, "10");
    CHECK_EQ(count({{"hint-1", {widerThanTenth}}}), count({{"hint-14", {widerThanTenth}}}));
    CHECK(count({{"hint-*4", {{ComparisonOperator::GreaterThanOrEqual, "1"}}}}) > 0);
}

TEST_CASE(matcher_compares_intervals_with_a_quality_by_their_size_in_semitones) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    auto count = [&](const AttributeMap& position) {
        return AttributeMatcher("mint", {position}).findAll(chorale, 4).size();
    };
    auto countOf = [&](ComparisonOperator comparisonOperator, const std::string& value) {
        return count({{"mint", {{comparisonOperator, value}}}});
    };
    // Without a quality every interval of that number is equally wide: nothing is wider than a third
    // but the fourths and up. With one, the intervals are told apart: a major third is wider than a minor.
    std::size_t majorThirds = count({{"mint", {"M3"}}});
    REQUIRE(majorThirds > 0);
    CHECK_EQ(countOf(ComparisonOperator::GreaterThan, "m3"), countOf(ComparisonOperator::GreaterThan, "3") + majorThirds);
    CHECK_EQ(countOf(ComparisonOperator::GreaterThanOrEqual, "m3"),
              countOf(ComparisonOperator::GreaterThan, "m3") + count({{"mint", {"m3"}}}));
    // d5 and A4 are the same size, so neither is wider than the other.
    CHECK_EQ(countOf(ComparisonOperator::GreaterThan, "A4"), countOf(ComparisonOperator::GreaterThan, "d5"));
}

TEST_CASE(matcher_compares_the_harmonic_interval_with_a_quality_in_semitones) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    auto countOf = [&](ComparisonOperator comparisonOperator, const std::string& value) {
        AttributeMap position{{"hint-14", {{comparisonOperator, value}}}};
        return AttributeMatcher("hint-14", {position}).findAll(chorale, 1).size();
    };
    const ComparisonOperator greaterThan = ComparisonOperator::GreaterThan;
    std::size_t all = AttributeMatcher("hint-14", {AttributeMap{{"hint-14", {"*"}}}}).findAll(chorale, 1).size();
    CHECK_EQ(countOf(greaterThan, "M10") + countOf(ComparisonOperator::LessThanOrEqual, "M10"), all);
    // A minor tenth is narrower than a major one, but both are tenths.
    CHECK(countOf(greaterThan, "m10") >= countOf(greaterThan, "M10"));
    // Past a ninth is a tenth or more, whatever its quality; past a minor tenth leaves out the minor one.
    CHECK(countOf(greaterThan, "9") >= countOf(greaterThan, "m10"));
}

TEST_CASE(matcher_an_interval_wider_than_a_given_one_but_not_an_octave_takes_two_keys_in_one_position) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    auto count = [&](const AttributeMap& position) {
        return AttributeMatcher("mint", {position}).findAll(chorale, 4).size();
    };
    PatternValue widerThanMinorSixth(ComparisonOperator::GreaterThan, "m6");
    std::size_t octaves = count({{"mint", {"P8"}}});
    CHECK_EQ(count({{"mint", {widerThanMinorSixth}}, {"!mint", {"P8"}}}), count({{"mint", {widerThanMinorSixth}}}) - octaves);
}

TEST_CASE(matcher_refuses_an_order_comparison_it_cannot_judge) {
    // Built in code rather than read from JSON, so the matcher is the one to say no.
    auto build = [](const AttributeMap& position) { return AttributeMatcher("kern", {position}); };
    CHECK_NOTHROW(build({{"kern", {{ComparisonOperator::GreaterThan, "g"}}}}));
    CHECK_THROWS(build({{"deg", {{ComparisonOperator::GreaterThan, "1"}}}}));
    CHECK_THROWS(build({{"fb", {{ComparisonOperator::GreaterThan, "3"}}}}));
    CHECK_THROWS(build({{"kern", {{ComparisonOperator::GreaterThan, "4"}}}}));      // a rhythm is not a pitch
    CHECK_THROWS(build({{"mint", {{ComparisonOperator::GreaterThan, "P3"}}}}));     // no perfect third
    CHECK_THROWS(build({{"hint-14", {{ComparisonOperator::GreaterThan, "+3"}}}}));  // never signed
}

// Voice references ("@2", "@below", ...) against chor029.krn: a value held against what another voice sounds
// on the same line. Like the comparisons, most of these check that the halves add up to the whole.

TEST_CASE(matcher_orders_a_pitch_against_another_voice) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    auto count = [&](const AttributeMap& position, std::size_t voice) {
        return AttributeMatcher("kern", {position}).findAll(chorale, voice).size();
    };
    PatternValue lowerThanTenor(ComparisonOperator::LessThan, "@2");
    PatternValue notLowerThanTenor(ComparisonOperator::GreaterThanOrEqual, "@2");

    // The alto against the tenor: lower or not lower, and a rest is neither, so negating adds the rests.
    std::size_t all = count({{"kern", {"*"}}}, 3);
    std::size_t lower = count({{"kern", {lowerThanTenor}}}, 3);
    std::size_t notLower = count({{"kern", {notLowerThanTenor}}}, 3);
    REQUIRE(notLower > 0);
    CHECK(lower + notLower <= all);
    CHECK_EQ(count({{"!kern", {lowerThanTenor}}}, 3) + lower, all);

    // The neighbours are the voices next to the one walked.
    CHECK_EQ(count({{"kern", {{ComparisonOperator::LessThan, "@below"}}}}, 3), lower);
    CHECK_EQ(count({{"kern", {{ComparisonOperator::LessThan, "@above"}}}}, 3),
              count({{"kern", {{ComparisonOperator::LessThan, "@4"}}}}, 3));
}

TEST_CASE(matcher_reference_to_a_voice_that_does_not_exist_holds_for_nothing) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    auto count = [&](const AttributeMap& position, std::size_t voice) {
        return AttributeMatcher("kern", {position}).findAll(chorale, voice).size();
    };
    // Nothing is below the bass, nor above the soprano, nor is a voice its own reference: not even "all" of
    // no voices holds.
    for (const char* reference : {"@below", "@anyBelow", "@allBelow", "@1"}) {
        CHECK_EQ(count({{"kern", {{ComparisonOperator::LessThan, reference}}}}, 1), std::size_t{0});
    }
    for (const char* reference : {"@above", "@anyAbove", "@allAbove", "@4"}) {
        CHECK_EQ(count({{"kern", {{ComparisonOperator::LessThan, reference}}}}, 4), std::size_t{0});
    }
    // ...so its negation holds everywhere.
    CHECK_EQ(count({{"!kern", {{ComparisonOperator::LessThan, "@allBelow"}}}}, 1), count({{"kern", {"*"}}}, 1));
}

TEST_CASE(matcher_reference_to_a_group_of_voices_is_any_or_all_of_them) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    auto count = [&](const AttributeMap& position) {
        return AttributeMatcher("kern", {position}).findAll(chorale, 4).size();
    };
    auto higherThan = [&](const char* reference) {
        return count({{"kern", {{ComparisonOperator::GreaterThan, reference}}}});
    };
    // The soprano against the three voices below it.
    std::size_t any = higherThan("@anyBelow");
    std::size_t all = higherThan("@allBelow");
    for (const char* single : {"@1", "@2", "@3"}) {
        CHECK(higherThan(single) <= any);
        CHECK(higherThan(single) >= all);
    }
    // A list of values is any of them, so listing the voices is the same as naming the group.
    CHECK_EQ(count({{"kern", {{ComparisonOperator::GreaterThan, "@1"}, {ComparisonOperator::GreaterThan, "@2"},
                              {ComparisonOperator::GreaterThan, "@3"}}}}),
              any);
    // Negated, they hold wherever they did not hold before, the rests included.
    CHECK_EQ(count({{"!kern", {{ComparisonOperator::GreaterThan, "@anyBelow"}}}}),
              count({{"kern", {"*"}}}) - any);
    CHECK_EQ(count({{"!kern", {{ComparisonOperator::GreaterThan, "@allBelow"}}}}),
              count({{"kern", {"*"}}}) - all);
}

TEST_CASE(matcher_plain_reference_is_the_same_pitch_as_another_voice) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    MatcherOptions ignoreOctave;
    ignoreOctave.kernIgnoreOctave = true;
    auto count = [&](const AttributeMap& position, const MatcherOptions& options = MatcherOptions()) {
        return AttributeMatcher("kern", {position}, options).findAll(chorale, 4).size();
    };
    // The soprano sings what the tenor sings: the same note in the register it is written in, or with the
    // octave ignored the same pitch class.
    std::size_t sameNote = count({{"kern", {"@2"}}});
    std::size_t samePitchClass = count({{"kern", {"@2"}}}, ignoreOctave);
    CHECK(sameNote <= samePitchClass);
    // With four voices to a chord, some other voice nearly always sings the soprano's pitch class too.
    std::size_t doubled = count({{"kern", {"@anyOther"}}}, ignoreOctave);
    REQUIRE(doubled > 0);
    CHECK(doubled >= samePitchClass);
    CHECK(sameNote <= count({{"kern", {{ComparisonOperator::GreaterThanOrEqual, "@2"}}}}));
    CHECK(sameNote <= count({{"kern", {{ComparisonOperator::LessThanOrEqual, "@2"}}}}));
}

TEST_CASE(matcher_compares_duration_with_another_voice) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    auto count = [&](const AttributeMap& position) {
        return AttributeMatcher("kern", {position}).findAll(chorale, 3).size();
    };
    // A note of the alto is shorter, as long as or longer than the note the tenor sounds with it.
    std::size_t same = count({{"duration", {"@2"}}});
    std::size_t shorter = count({{"duration", {{ComparisonOperator::LessThan, "@2"}}}});
    std::size_t longer = count({{"duration", {{ComparisonOperator::GreaterThan, "@2"}}}});
    REQUIRE(same > 0);
    CHECK_EQ(same + shorter + longer, count({{"kern", {"*"}}}));
    CHECK_EQ(count({{"duration", {{ComparisonOperator::LessThanOrEqual, "@2"}}}}), same + shorter);
}

TEST_CASE(matcher_refuses_a_voice_reference_it_cannot_judge) {
    auto build = [](const AttributeMap& position, MatcherOptions options = MatcherOptions()) {
        return AttributeMatcher("kern", {position}, options);
    };
    CHECK_NOTHROW(build({{"kern", {"@2"}}}));
    CHECK_NOTHROW(build({{"!kern", {{ComparisonOperator::GreaterThan, "@anyBelow"}}}}));
    CHECK_NOTHROW(build({{"duration", {"@below"}}}));
    CHECK_THROWS(build({{"deg", {"@2"}}}));            // only kern and duration
    CHECK_THROWS(build({{"mint", {"@2"}}}));
    CHECK_THROWS(build({{"kern", {"@5"}}}));           // no such voice
    CHECK_THROWS(build({{"kern", {"@sideways"}}}));    // no such reference

    MatcherOptions splitNotes;
    splitNotes.durationAllowSplitNotes = true;
    CHECK_NOTHROW(build({{"duration", {"@2"}}}, splitNotes));
    CHECK_THROWS(build({{"!duration", {"@2"}}}, splitNotes));
}

// The semitone size of an interval comes from a small table of our own (intervalSizeInSemitones), as humlib
// only goes the other way, from a base-40 interval to its name. Every interval humlib can name has to be
// the size that table says, or a threshold would be ordered differently from the tokens the spines carry.
TEST_CASE(matcher_interval_sizes_agree_with_humlibs_base40_interval_names) {
    using choralesearch::intervalSizeInSemitones;
    const int c4 = hum::Convert::kernToBase40("c");
    int named = 0;
    for (int base40Interval = 0; base40Interval < 40 * 3; ++base40Interval) {
        std::string name = hum::Convert::base40ToIntervalAbbr(base40Interval);
        if (name.find('X') != std::string::npos) continue; // a slot that names no interval
        // The last two slots of an octave, C-- and C-, are the octave less two or one semitones, which
        // humlib names as if they were the unison ("dd1", "d1") and not "dd8"/"d8".
        if (base40Interval % 40 >= 38) continue;
        std::size_t digits = name.find_first_of("0123456789");
        REQUIRE(digits != std::string::npos);
        std::string quality = name.substr(0, digits);
        for (char& c : quality) {
            if (c == 'p') c = 'P';
            else if (c == 'a') c = 'A';
        }
        int number = std::stoi(name.substr(digits));
        int expected = hum::Convert::base40ToMidiNoteNumber(c4 + base40Interval) - hum::Convert::base40ToMidiNoteNumber(c4);
        auto size = intervalSizeInSemitones(quality, number);
        REQUIRE(size.has_value());
        CHECK_EQ(*size, expected);
        ++named;
    }
    CHECK(named > 60);
}

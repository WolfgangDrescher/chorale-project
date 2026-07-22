#include "test_framework.hpp"

#include <algorithm>

#include "AttributeMatcher.hpp"
#include "HumdrumChorale.hpp"

using choralesearch::AttributeMap;
using choralesearch::AttributeMatcher;
using choralesearch::HumdrumChorale;

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
    AttributeMatcher withoutFlag("kern", pattern, /*mintStartAtPreviousToken=*/false);
    AttributeMatcher withFlag("kern", pattern, /*mintStartAtPreviousToken=*/true);

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
    AttributeMatcher withShift("mint", pattern, /*mintStartAtPreviousToken=*/true);

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
    AttributeMatcher withoutFlag("mint", pattern, /*mintStartAtPreviousToken=*/false);
    AttributeMatcher withFlag("mint", pattern, /*mintStartAtPreviousToken=*/true);

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

TEST_CASE(matcher_mint_pattern_mixes_partial_and_exact_values_across_positions) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    std::vector<AttributeMap> pattern = {
        AttributeMap{{"mint", {"+2"}}},
        AttributeMap{{"mint", {"+M2"}}},
    };
    AttributeMatcher matcher("mint", pattern);
    CHECK(!matcher.findAll(chorale, 4).empty());
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

TEST_CASE(matcher_fb_compare_exact_chord_rejects_chords_with_extra_figures) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    AttributeMatcher permissive("fb", {AttributeMap{{"fb", {"6 3"}}}});
    AttributeMatcher exact("fb", {AttributeMap{{"fb", {"6 3"}}}}, /*mintStartAtPreviousToken=*/false,
                            /*fbCompareExactChord=*/true);

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

TEST_MAIN()

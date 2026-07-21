#include "test_framework.hpp"

#include <algorithm>

#include "AttributeMatcher.hpp"
#include "HumdrumChorale.hpp"

using choralesearch::AttributeMap;
using choralesearch::AttributeMatcher;
using choralesearch::HumdrumChorale;

// General AttributeMatcher mechanics against the real chor029.krn fixture:
// literal/duration/fermata/cross-spine matching, wildcards, multi-position
// patterns, voice scoping, and graceful-empty edge cases. Exhaustive
// exact-result coverage across all fixture chorales is a separate,
// later test file -- this one is about the matching *mechanism* itself.

TEST_CASE(matcher_matches_literal_driving_feature_token) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    // Matching on the driving feature itself compares the *raw* token
    // text, fermata marker included -- "4D" alone would not match "4D;".
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

TEST_MAIN()

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

TEST_MAIN()

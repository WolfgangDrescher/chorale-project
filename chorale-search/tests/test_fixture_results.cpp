#include "test_framework.hpp"

#include <filesystem>
#include <iostream>

#include "CorpusSearch.hpp"
#include "Query.hpp"

using choralesearch::AttributeMap;
using choralesearch::CorpusSearch;
using choralesearch::Query;

// A growing catalog of exact, hand-verified search results against the real
// chorales in tests/fixtures/. Unlike test_attributematcher.cpp (which checks
// the matching *mechanism* in the abstract) and test_corpussearch.cpp (which
// checks CorpusSearch's own plumbing), this file pins down concrete musical
// results: for pattern X, chorale Y's voice Z has a match starting/ending at
// exactly these beat positions.

TEST_CASE(deg_3_2_1) {
    Query q;
    q.feature = "deg";
    q.pattern = {
        AttributeMap{{"deg", {"3"}}},
        AttributeMap{{"deg", {"2"}}},
        AttributeMap{{"deg", {"1"}}},
    };
    q.voices = "all";

    CorpusSearch search(FIXTURE_CHORALE("chor029"));
    auto results = search.run(q);

    REQUIRE(results.size() == 8u);

    CHECK_RESULT(results[0], "chor029", 1, "25", "26");
    CHECK_RESULT(results[1], "chor029", 1, "36+1/2", "38");
    CHECK_RESULT(results[2], "chor029", 2, "0", "2");
    CHECK_RESULT(results[3], "chor029", 2, "8", "10");
    CHECK_RESULT(results[4], "chor029", 4, "5", "7");
    CHECK_RESULT(results[5], "chor029", 4, "12", "14");
    CHECK_RESULT(results[6], "chor029", 4, "35", "36");
    CHECK_RESULT(results[7], "chor029", 4, "47", "50");
}

TEST_CASE(deg_3_2_1_or_3_2_3_in_soprano) {
    Query q;
    q.feature = "deg";
    q.pattern = {
        AttributeMap{{"deg", {"3"}}},
        AttributeMap{{"deg", {"2"}}},
        AttributeMap{{"deg", {"1", "3"}}},
    };
    q.voices = "soprano";

    CorpusSearch search(FIXTURE_CHORALE("chor029"));
    auto results = search.run(q);

    REQUIRE(results.size() == 7u);

    CHECK_RESULT(results[0], "chor029", 4, "5", "7");
    CHECK_RESULT(results[1], "chor029", 4, "12", "14");
    CHECK_RESULT(results[2], "chor029", 4, "20", "22");
    CHECK_RESULT(results[3], "chor029", 4, "29", "32");
    CHECK_RESULT(results[4], "chor029", 4, "35", "36");
    CHECK_RESULT(results[5], "chor029", 4, "45", "47");
    CHECK_RESULT(results[6], "chor029", 4, "47", "50");
}

TEST_CASE(deg_3_2_1_or_3_2_3_in_soprano_fermata) {
    Query q;
    q.feature = "deg";
    q.pattern = {
        AttributeMap{{"deg", {"3"}}},
        AttributeMap{{"deg", {"2"}}},
        AttributeMap{{"deg", {"1", "3"}}, {"fermata", {"true"}}},
    };
    q.voices = "soprano";

    CorpusSearch search(FIXTURE_CHORALE("chor029"));
    auto results = search.run(q);

    REQUIRE(results.size() == 4u);

    CHECK_RESULT(results[0], "chor029", 4, "5", "7");
    CHECK_RESULT(results[1], "chor029", 4, "12", "14");
    CHECK_RESULT(results[2], "chor029", 4, "20", "22");
    CHECK_RESULT(results[3], "chor029", 4, "47", "50");
}

TEST_CASE(deg_3_2_1_or_3_2_3_duration) {
    Query q;
    q.feature = "deg";
    q.pattern = {
        AttributeMap{{"deg", {"3"}}, {"duration", {"4"}}},
        AttributeMap{{"deg", {"2"}}, {"duration", {"4"}}},
        AttributeMap{{"deg", {"1", "3"}}, {"duration", {"4"}}},
    };
    q.voices = "all";

    CorpusSearch search(FIXTURE_CHORALE("chor029"));
    auto results = search.run(q);

    REQUIRE(results.size() == 2u);

    CHECK_RESULT(results[0], "chor029", 4, "5", "7");
    CHECK_RESULT(results[1], "chor029", 4, "45", "47");
}

TEST_CASE(deg_3_2_1_or_3_2_3_duration_with_wildcard) {
    Query q;
    q.feature = "deg";
    q.pattern = {
        AttributeMap{{"deg", {"3"}}, {"duration", {"4"}}},
        AttributeMap{{"deg", {"2"}}, {"duration", {"4"}}},
        AttributeMap{{"deg", {"1", "3"}}, {"duration", {"*"}}},
    };
    q.voices = "all";

    CorpusSearch search(FIXTURE_CHORALE("chor029"));
    auto results = search.run(q);

    REQUIRE(results.size() == 6u);

    CHECK_RESULT(results[0], "chor029", 2, "0", "2");
    CHECK_RESULT(results[1], "chor029", 2, "8", "10");
    CHECK_RESULT(results[2], "chor029", 4, "5", "7");
    CHECK_RESULT(results[3], "chor029", 4, "12", "14");
    CHECK_RESULT(results[4], "chor029", 4, "20", "22");
    CHECK_RESULT(results[5], "chor029", 4, "45", "47");
}

TEST_CASE(deg_3_2_1_or_3_2_3_duration_with_wildcard_and_fermata) {
    Query q;
    q.feature = "deg";
    q.pattern = {
        AttributeMap{{"deg", {"3"}}, {"duration", {"4"}}},
        AttributeMap{{"deg", {"2"}}, {"duration", {"4"}}},
        AttributeMap{{"deg", {"1", "3"}}, {"duration", {"*"}}, {"fermata", {"true"}}},
    };
    q.voices = "all";

    CorpusSearch search(FIXTURE_CHORALE("chor029"));
    auto results = search.run(q);

    REQUIRE(results.size() == 3u);

    CHECK_RESULT(results[0], "chor029", 4, "5", "7");
    CHECK_RESULT(results[1], "chor029", 4, "12", "14");
    CHECK_RESULT(results[2], "chor029", 4, "20", "22");
}

TEST_CASE(mint_plusM2_plusM2_in_soprano) {
    Query q;
    q.feature = "mint";
    q.pattern = {
        AttributeMap{{"mint", {"+M2"}}},
        AttributeMap{{"mint", {"+M2"}}},
    };
    q.voices = "soprano";

    CorpusSearch search(FIXTURE_CHORALE("chor029"));
    auto results = search.run(q);

    REQUIRE(!results.empty());
    CHECK_RESULT(results[0], "chor029", 4, "1", "2");
}

TEST_CASE(mint_plusM2_plusM2_in_soprano_start_at_previous_token) {
    Query q;
    q.feature = "mint";
    q.pattern = {
        AttributeMap{{"mint", {"+M2"}}},
        AttributeMap{{"mint", {"+M2"}}},
    };
    q.voices = "soprano";
    q.mintStartAtPreviousToken = true;

    CorpusSearch search(FIXTURE_CHORALE("chor029"));
    auto results = search.run(q);

    REQUIRE(!results.empty());
    CHECK_RESULT(results[0], "chor029", 4, "0", "2");
}

TEST_CASE(mint_direction_only_plus2_plus2_in_soprano) {
    Query q;
    q.feature = "mint";
    q.pattern = {
        AttributeMap{{"mint", {"+2"}}},
        AttributeMap{{"mint", {"+2"}}},
        AttributeMap{{"mint", {"-2"}}},
    };
    q.voices = "soprano";
    q.mintStartAtPreviousToken = true;

    CorpusSearch search(FIXTURE_CHORALE("chor029"));
    auto results = search.run(q);

    REQUIRE(results.size() == 4u);
    CHECK_RESULT(results[0], "chor029", 4, "0", "3");
    CHECK_RESULT(results[1], "chor029", 4, "9", "12");
    CHECK_RESULT(results[2], "chor029", 4, "16", "19");
    CHECK_RESULT(results[3], "chor029", 4, "42", "44");
}

TEST_CASE(mint_direction_only_plus2_plus2_in_soprano_with_duration) {
    Query q;
    q.feature = "mint";
    q.pattern = {
        AttributeMap{{"mint", {"+2"}}, {"duration", {"4"}}},
        AttributeMap{{"mint", {"+2"}}},
        AttributeMap{{"mint", {"-2"}}},
    };
    q.voices = "soprano";
    q.mintStartAtPreviousToken = true;

    CorpusSearch search(FIXTURE_CHORALE("chor029"));
    auto results = search.run(q);


    REQUIRE(results.size() == 3u);
    CHECK_RESULT(results[0], "chor029", 4, "0", "3");
    CHECK_RESULT(results[1], "chor029", 4, "9", "12");
    CHECK_RESULT(results[2], "chor029", 4, "16", "19");
}

TEST_CASE(mint_bare_number_2_or_1_in_soprano) {
    Query q;
    q.feature = "mint";
    q.pattern = {
        AttributeMap{{"mint", {"2", "1"}}},
    };
    q.voices = "soprano";
    q.mintStartAtPreviousToken = true;

    CorpusSearch search(FIXTURE_CHORALE("chor029"));
    auto results = search.run(q);

    REQUIRE(results.size() == 40u);
}

TEST_CASE(fb_m6_3_in_bass) {
    // fb's 6th figure pinned to minor, 3rd left quality-free -- excludes e.g. line 31's
    // "M6 m3" (major 6th) while accepting every "m6 <any 3rd quality>" chord.
    Query q;
    q.feature = "fb";
    q.pattern = {
        AttributeMap{{"fb", {"m6 3"}}},
    };
    q.voices = "bass";

    CorpusSearch search(FIXTURE_CHORALE("chor029"));
    auto results = search.run(q);

    REQUIRE(results.size() == 13u);
    CHECK_RESULT(results[0], "chor029", 1, "1", "1");
    CHECK_RESULT(results[1], "chor029", 1, "5", "5");
    CHECK_RESULT(results[2], "chor029", 1, "9", "9");
    CHECK_RESULT(results[3], "chor029", 1, "12", "12");
    CHECK_RESULT(results[4], "chor029", 1, "17", "17");
    CHECK_RESULT(results[5], "chor029", 1, "19", "19");
    CHECK_RESULT(results[6], "chor029", 1, "28", "28");
    CHECK_RESULT(results[7], "chor029", 1, "35", "35");
    CHECK_RESULT(results[8], "chor029", 1, "36+1/2", "36+1/2");
    CHECK_RESULT(results[9], "chor029", 1, "39", "39");
    CHECK_RESULT(results[10], "chor029", 1, "43", "43");
    CHECK_RESULT(results[11], "chor029", 1, "44", "44");
    CHECK_RESULT(results[12], "chor029", 1, "47", "47");
}

TEST_CASE(fb_component_order_within_a_pattern_value_does_not_matter) {
    Query withOrder;
    withOrder.feature = "fb";
    withOrder.pattern = {AttributeMap{{"fb", {"m6 3"}}}};
    withOrder.voices = "bass";

    Query reversed;
    reversed.feature = "fb";
    reversed.pattern = {AttributeMap{{"fb", {"3 m6"}}}};
    reversed.voices = "bass";

    CorpusSearch search(FIXTURE_CHORALE("chor029"));
    auto a = search.run(withOrder);
    auto b = search.run(reversed);

    REQUIRE(a.size() == b.size());
    for (std::size_t i = 0; i < a.size(); ++i) {
        CHECK_EQ(a[i].startPosition, b[i].startPosition);
    }
}

TEST_CASE(mint_mixed_input) {
    Query q;
    q.feature = "mint";
    q.pattern = {
        AttributeMap{{"mint", {"m2"}}},
        AttributeMap{{"mint", {"6"}}},
        AttributeMap{{"mint", {"+2"}}},
        AttributeMap{{"mint", {"-m2"}}},
        AttributeMap{{"mint", {"-2"}}, {"duration", {"2"}}},
        AttributeMap{{"mint", {"P1"}}, {"fermata", {"true"}}},
    };
    q.voices = "soprano";
    q.mintStartAtPreviousToken = true;

    CorpusSearch search(FIXTURE_CHORALE("chor009"));
    auto results = search.run(q);

    REQUIRE(results.size() == 1u);
    CHECK_RESULT(results[0], "chor009", 4, "32", "39");
}

TEST_CASE(fb_6_3_exact_chord_in_bass) {
    // Same "6 3" pattern as a permissive search would use, but with fbCompareExactChord
    // set: chords voiced with an extra component beyond the 6th and 3rd (e.g. an added
    // 9th) no longer qualify, narrowing 19 permissive matches down to these 16.
    Query q;
    q.feature = "fb";
    q.pattern = {
        AttributeMap{{"fb", {"6 3"}}},
    };
    q.voices = "bass";
    q.fbCompareExactChord = true;

    CorpusSearch search(FIXTURE_CHORALE("chor029"));
    auto results = search.run(q);

    REQUIRE(results.size() == 16u);
    CHECK_RESULT(results[0], "chor029", 1, "1", "1");
    CHECK_RESULT(results[1], "chor029", 1, "4+1/2", "4+1/2");
    CHECK_RESULT(results[2], "chor029", 1, "5", "5");
    CHECK_RESULT(results[3], "chor029", 1, "9", "9");
    CHECK_RESULT(results[4], "chor029", 1, "11+1/2", "11+1/2");
    CHECK_RESULT(results[5], "chor029", 1, "12", "12");
    CHECK_RESULT(results[6], "chor029", 1, "17", "17");
    CHECK_RESULT(results[7], "chor029", 1, "19", "19");
    CHECK_RESULT(results[8], "chor029", 1, "28", "28");
    CHECK_RESULT(results[9], "chor029", 1, "34+1/2", "34+1/2");
    CHECK_RESULT(results[10], "chor029", 1, "35", "35");
    CHECK_RESULT(results[11], "chor029", 1, "37", "37");
    CHECK_RESULT(results[12], "chor029", 1, "39", "39");
    CHECK_RESULT(results[13], "chor029", 1, "43", "43");
    CHECK_RESULT(results[14], "chor029", 1, "44", "44");
    CHECK_RESULT(results[15], "chor029", 1, "47", "47");
}

TEST_CASE(fb_4_2_chord_in_bass) {
    Query q;
    q.feature = "fb";
    q.pattern = {
        AttributeMap{{"fb", {"M2 A4"}}, {"deg", {"4"}}},
        AttributeMap{{"mint", {"-2"}}},
    };
    q.voices = "bass";
    q.fbCompareExactChord = false;

    CorpusSearch search(FIXTURE_CHORALE("chor029"));
    auto results = search.run(q);

    REQUIRE(results.size() == 2u);
    CHECK_RESULT(results[0], "chor029", 1, "24+1/2", "25");
    CHECK_RESULT(results[1], "chor029", 1, "46", "47");
}

TEST_CASE(fb_4_2_exact_chord_in_bass) {
    Query q;
    q.feature = "fb";
    q.pattern = {
        AttributeMap{{"fb", {"M2 A4"}}, {"deg", {"4"}}},
        AttributeMap{{"mint", {"-2"}}},
    };
    q.voices = "bass";
    q.fbCompareExactChord = true;

    CorpusSearch search(FIXTURE_CHORALE("chor029"));
    auto results = search.run(q);

    REQUIRE(results.empty());
}

TEST_CASE(fb_not_rule_of_the_octave_chords_on_deg_4_in_bass) {
    Query q;
    q.feature = "fb";
    q.voices = "1";
    q.pattern = {
        AttributeMap{{"deg", {"4"}}, {"!fb", {"2 4 6", "6 5 3"}}},
    };

    CorpusSearch search(FIXTURE_CHORALE("chor029"));
    auto results = search.run(q);

    REQUIRE(results.size() == 5u);
    CHECK_RESULT(results[0], "chor029", 1, "5+1/2", "5+1/2");
    CHECK_RESULT(results[1], "chor029", 1, "12+1/2", "12+1/2");
    CHECK_RESULT(results[2], "chor029", 1, "36", "36");
    CHECK_RESULT(results[3], "chor029", 1, "40", "40");
    CHECK_RESULT(results[4], "chor029", 1, "47+1/2", "47+1/2");
}

TEST_CASE(fb_excluding_soprano_mint_2_or_1_or_bracketed_first_note) {
    Query q;
    q.feature = "fb";
    q.voices = "4";
    q.pattern = {
        AttributeMap{{"!mint", {"2", "1", "[g]"}}},
    };

    CorpusSearch search(FIXTURE_CHORALE("chor029"));
    auto results = search.run(q);

    REQUIRE(results.size() == 6u);

    CHECK_RESULT(results[0], "chor029", 4, "8", "8");
    CHECK_RESULT(results[1], "chor029", 4, "16", "16");
    CHECK_RESULT(results[2], "chor029", 4, "24", "24");
    CHECK_RESULT(results[3], "chor029", 4, "33", "33");
    CHECK_RESULT(results[4], "chor029", 4, "40", "40");
    CHECK_RESULT(results[5], "chor029", 4, "42", "42");
}

TEST_MAIN()

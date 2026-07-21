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

TEST_MAIN()

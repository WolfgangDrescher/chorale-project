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

TEST_MAIN()

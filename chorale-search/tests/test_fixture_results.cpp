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

TEST_CASE(deg_descending_3_2_1) {
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
    std::cout << results;

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

TEST_MAIN()

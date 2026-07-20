#include "test_framework.hpp"

#include <algorithm>
#include <filesystem>

#include "CorpusSearch.hpp"
#include "HumdrumChorale.hpp"
#include "Query.hpp"

using choralesearch::AttributeMap;
using choralesearch::CorpusSearch;
using choralesearch::HumdrumChorale;
using choralesearch::Query;
using choralesearch::Result;

// chor001.krn, chor009.krn and chor029.krn (tests/fixtures/) each have exactly
// 6 fermatas in the soprano -- used below to exercise behavior that's actually
// CorpusSearch's own job (file discovery, cross-file aggregation, Result-field
// mapping, limit truncation) rather than re-testing AttributeMatcher's matching
// logic, which test_attributematcher.cpp already covers.

TEST_CASE(run_one_populates_result_fields_from_a_real_match) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    CorpusSearch search(chorale.path()); // corpusRoot is unused by runOne, but the constructor needs one
    Query q;
    q.feature = "kern";
    q.pattern = {AttributeMap{{"fermata", {"true"}}}};
    q.voices = "soprano";
    auto results = search.runOne(chorale, q);

    REQUIRE(results.size() == 6u);
    const Result& r = results[0];
    CHECK_EQ(r.choraleId, std::string("chor029"));
    CHECK_EQ(r.feature, std::string("kern"));
    CHECK_EQ(r.voiceLabel, std::string("Soprano"));
    CHECK_EQ(r.voice, std::size_t{4});
    CHECK(r.startLineNumber > 0);
    CHECK_EQ(r.startLineNumber, r.endLineNumber); // single-position pattern
	CHECK_EQ(r.startPosition, std::string("7"));
    CHECK_EQ(r.endPosition, std::string("7"));
}

TEST_CASE(run_one_truncates_results_at_the_query_limit) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    CorpusSearch search(chorale.path());
    Query q;
    q.feature = "kern";
    q.pattern = {AttributeMap{{"fermata", {"true"}}}};
    q.voices = "soprano";
    q.limit = 2; // out of 6 available
    auto results = search.runOne(chorale, q);
    CHECK_EQ(results.size(), std::size_t{2});
}

TEST_CASE(run_searches_a_single_file_used_as_the_corpus_root) {
    CorpusSearch search(FIXTURE_CHORALE("chor029"));
    Query q;
    q.feature = "kern";
    q.pattern = {AttributeMap{{"fermata", {"true"}}}};
    q.voices = "soprano";
    auto results = search.run(q);
    REQUIRE(results.size() == 6u);
    CHECK_EQ(results[0].choraleId, std::string("chor029"));
}

TEST_CASE(run_aggregates_matches_across_every_file_in_a_directory_corpus_root) {
    auto fixturesDir = std::filesystem::path(FIXTURE_CHORALE("chor029")).parent_path();
    CorpusSearch search(fixturesDir);
    Query q;
    q.feature = "kern";
    q.pattern = {AttributeMap{{"fermata", {"true"}}}};
    q.voices = "soprano";
    auto results = search.run(q);

    CHECK_EQ(results.size(), std::size_t{18}); // 6 fermatas x 3 fixture chorales
    std::vector<std::string> ids;
    for (const auto& r : results) ids.push_back(r.choraleId);
    std::sort(ids.begin(), ids.end());
    ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
    CHECK_EQ(ids, (std::vector<std::string>{"chor001", "chor009", "chor029"}));
}

TEST_CASE(run_stops_at_the_limit_partway_through_a_later_file) {
    // findChoraleFiles() sorts, so chor001 < chor009 < chor029. chor001
    // alone has 6 soprano fermatas, so a limit of 8 must exhaust chor001 and
    // then take 2 more from chor009 -- exercising the cross-file
    // accumulate-then-truncate logic that's unique to CorpusSearch::run,
    // not something a single-file test (or AttributeMatcher) can show.
    auto fixturesDir = std::filesystem::path(FIXTURE_CHORALE("chor029")).parent_path();
    CorpusSearch search(fixturesDir);
    Query q;
    q.feature = "kern";
    q.pattern = {AttributeMap{{"fermata", {"true"}}}};
    q.voices = "soprano";
    q.limit = 8;
    auto results = search.run(q);

    REQUIRE(results.size() == 8u);
    for (std::size_t i = 0; i < 6; ++i) CHECK_EQ(results[i].choraleId, std::string("chor001"));
    for (std::size_t i = 6; i < 8; ++i) CHECK_EQ(results[i].choraleId, std::string("chor009"));
}

TEST_CASE(run_throws_when_the_corpus_root_does_not_exist) {
    CorpusSearch search("/nonexistent/corpus/root");
    Query q;
    q.feature = "kern";
    q.pattern = {AttributeMap{{"kern", {"*"}}}};
    CHECK_THROWS(search.run(q));
}

TEST_MAIN()

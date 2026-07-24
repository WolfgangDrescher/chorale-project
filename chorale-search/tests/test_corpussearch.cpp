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
using choralesearch::SimultaneousGroup;

// chor001.krn, chor009.krn, and chor029.krn each have exactly 6 soprano fermatas.
// chor006.krn (added for rest-matching coverage) has only 4 and sorts between
// chor001 and chor009. Tests below exercise CorpusSearch behaviour (file
// discovery, aggregation, result mapping, limit truncation), not AttributeMatcher.

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

    CHECK_EQ(results.size(), std::size_t{22}); // 6+6+6 fermatas x 3 fixture chorales, plus chor006's 4
    std::vector<std::string> ids;
    for (const auto& r : results) ids.push_back(r.choraleId);
    std::sort(ids.begin(), ids.end());
    ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
    CHECK_EQ(ids, (std::vector<std::string>{"chor001", "chor006", "chor009", "chor029"}));
}

TEST_CASE(run_stops_at_the_limit_partway_through_a_later_file) {
    // findChoraleFiles() sorts, so chor001 < chor006 < chor009 < chor029. chor001 alone has
    // 6 soprano fermatas, so a limit of 8 must exhaust chor001 and then take 2 more from
    // chor006 -- exercising the cross-file accumulate-then-truncate logic that's unique to
    // CorpusSearch::run, not something a single-file test (or AttributeMatcher) can show.
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
    for (std::size_t i = 6; i < 8; ++i) CHECK_EQ(results[i].choraleId, std::string("chor006"));
}

TEST_CASE(run_throws_when_the_corpus_root_does_not_exist) {
    CorpusSearch search("/nonexistent/corpus/root");
    Query q;
    q.feature = "kern";
    q.pattern = {AttributeMap{{"kern", {"*"}}}};
    CHECK_THROWS(search.run(q));
}

TEST_CASE(simultaneous_with_default_alignment_keeps_matches_that_align_with_the_group) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    CorpusSearch search(chorale.path());
    Query q;
    q.feature = "kern";
    q.pattern = {AttributeMap{{"fermata", {"true"}}}};
    q.voices = "soprano";

    SimultaneousGroup group;
    group.feature = "kern";
    group.voices = "bass";
    group.pattern = {AttributeMap{{"fermata", {"true"}}}};
    q.simultaneousWith = {group};

    auto results = search.runOne(chorale, q);
    CHECK_EQ(results.size(), std::size_t{6}); // same 6 as without the constraint
}

TEST_CASE(simultaneous_with_group_that_never_matches_filters_out_every_result) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    CorpusSearch search(chorale.path());
    Query q;
    q.feature = "kern";
    q.pattern = {AttributeMap{{"fermata", {"true"}}}};
    q.voices = "soprano";

    SimultaneousGroup group;
    group.feature = "kern";
    group.voices = "alto";
    group.pattern = {AttributeMap{{"kern", {"r"}}}}; // no rests anywhere in this chorale
    q.simultaneousWith = {group};

    auto results = search.runOne(chorale, q);
    CHECK_EQ(results.size(), std::size_t{0});
}

TEST_CASE(simultaneous_with_requires_every_group_to_have_a_match) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    CorpusSearch search(chorale.path());
    Query q;
    q.feature = "kern";
    q.pattern = {AttributeMap{{"fermata", {"true"}}}};
    q.voices = "soprano";

    SimultaneousGroup alwaysAligns;
    alwaysAligns.feature = "kern";
    alwaysAligns.voices = "bass";
    alwaysAligns.pattern = {AttributeMap{{"fermata", {"true"}}}};

    SimultaneousGroup neverMatches;
    neverMatches.feature = "kern";
    neverMatches.voices = "alto";
    neverMatches.pattern = {AttributeMap{{"kern", {"r"}}}};

    q.simultaneousWith = {alwaysAligns, neverMatches};
    CHECK_EQ(search.runOne(chorale, q).size(), std::size_t{0}); // the second group has no match

    q.simultaneousWith = {alwaysAligns};
    CHECK_EQ(search.runOne(chorale, q).size(), std::size_t{6}); // only the aligning group remains
}

TEST_CASE(simultaneous_alignment_start_only_checks_the_matchs_start_position) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    CorpusSearch search(chorale.path());
    Query q;
    q.feature = "kern";
    q.voices = "soprano";
    q.pattern = {AttributeMap{{"kern", {"g"}}}, AttributeMap{{"kern", {"a"}}}};
    q.simultaneousAlignment = "start"; // default, set explicitly for clarity

    SimultaneousGroup group;
    group.feature = "kern";
    group.voices = "bass";
    group.pattern = {AttributeMap{{"kern", {"G"}}}};
    q.simultaneousWith = {group};

    auto results = search.runOne(chorale, q);
    REQUIRE(results.size() == 1u);
    CHECK_EQ(results[0].startPosition, std::string("0"));
}

TEST_CASE(simultaneous_alignment_end_only_checks_the_matchs_end_position) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    CorpusSearch search(chorale.path());
    Query q;
    q.feature = "kern";
    q.voices = "soprano";
    q.pattern = {AttributeMap{{"kern", {"g"}}}, AttributeMap{{"kern", {"a"}}}};
    q.simultaneousAlignment = "end";

    SimultaneousGroup group;
    group.feature = "kern";
    group.voices = "bass";
    group.pattern = {AttributeMap{{"kern", {"F#"}}}};
    q.simultaneousWith = {group};

    auto results = search.runOne(chorale, q);
    REQUIRE(results.size() == 2u);
    CHECK_EQ(results[0].startPosition, std::string("0"));
    CHECK_EQ(results[1].startPosition, std::string("8"));
}

TEST_CASE(simultaneous_alignment_start_end_requires_both_positions_at_once) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    CorpusSearch search(chorale.path());
    Query q;
    q.feature = "kern";
    q.voices = "soprano";
    q.pattern = {AttributeMap{{"kern", {"g"}}}, AttributeMap{{"kern", {"a"}}}};
    q.simultaneousAlignment = "start-end";

    SimultaneousGroup group;
    group.feature = "kern";
    group.voices = "bass";
    group.pattern = {AttributeMap{{"kern", {"G"}}}};
    q.simultaneousWith = {group};
    CHECK_EQ(search.runOne(chorale, q).size(), std::size_t{0});

    group.pattern = {AttributeMap{{"kern", {"F#"}}}};
    q.simultaneousWith = {group};
    CHECK_EQ(search.runOne(chorale, q).size(), std::size_t{0});

    group.pattern = {AttributeMap{{"kern", {"G"}}}, AttributeMap{{"kern", {"F#"}}}};
    q.simultaneousWith = {group};
    CHECK_EQ(search.runOne(chorale, q).size(), std::size_t{1});
}

TEST_CASE(simultaneous_alignment_start_end_matches_when_both_patterns_are_single_position) {
    // Every fermata in this chorale is single-position, so its start and end position are
    // identical -- "start-end" behaves exactly like the "start" default here.
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    CorpusSearch search(chorale.path());
    Query q;
    q.feature = "kern";
    q.pattern = {AttributeMap{{"fermata", {"true"}}}};
    q.voices = "soprano";
    q.simultaneousAlignment = "start-end";

    SimultaneousGroup group;
    group.feature = "kern";
    group.voices = "bass";
    group.pattern = {AttributeMap{{"fermata", {"true"}}}};
    q.simultaneousWith = {group};

    CHECK_EQ(search.runOne(chorale, q).size(), std::size_t{6});
}

TEST_CASE(simultaneous_with_group_can_override_the_querys_simultaneous_alignment) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    CorpusSearch search(chorale.path());
    Query q;
    q.feature = "kern";
    q.voices = "soprano";
    q.pattern = {AttributeMap{{"kern", {"g"}}}, AttributeMap{{"kern", {"a"}}}};
    q.simultaneousAlignment = "end"; // would find nothing against bass "G" (see test above)

    SimultaneousGroup group;
    group.feature = "kern";
    group.voices = "bass";
    group.pattern = {AttributeMap{{"kern", {"G"}}}};
    group.simultaneousAlignment = "start"; // override back to "start" for this group only
    q.simultaneousWith = {group};

    auto results = search.runOne(chorale, q);
    REQUIRE(results.size() == 1u);
    CHECK_EQ(results[0].startPosition, std::string("0"));
}

TEST_CASE(simultaneous_with_group_can_override_kern_ignore_octave) {
    // The bass never has a literal lowercase "g" (specific octave) in this chorale, only
    // uppercase "G" -- so the group's pattern only matches once kernIgnoreOctave folds them
    // together, whether that comes from the group's own override or an inherited query value.
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    CorpusSearch search(chorale.path());
    Query q;
    q.feature = "kern";
    q.voices = "soprano";
    q.pattern = {AttributeMap{{"kern", {"g"}}}, AttributeMap{{"kern", {"a"}}}};

    SimultaneousGroup group;
    group.feature = "kern";
    group.voices = "bass";
    group.pattern = {AttributeMap{{"kern", {"g"}}}};
    q.simultaneousWith = {group};
    CHECK_EQ(search.runOne(chorale, q).size(), std::size_t{0}); // neither overridden nor inherited

    q.simultaneousWith[0].kernIgnoreOctave = true; // group-level override
    CHECK_EQ(search.runOne(chorale, q).size(), std::size_t{1});

    q.simultaneousWith[0].kernIgnoreOctave.reset();
    q.kernIgnoreOctave = true; // query-level, inherited by the group
    CHECK_EQ(search.runOne(chorale, q).size(), std::size_t{1});
}

TEST_MAIN()

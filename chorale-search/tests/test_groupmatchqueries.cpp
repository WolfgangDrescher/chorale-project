#include "test_framework.hpp"

#include <algorithm>

#include "CorpusSearch.hpp"
#include "GroupMatchQueries.hpp"
#include "Grouping.hpp"
#include "GroupingJson.hpp"

using choralesearch::GroupQuery;
using choralesearch::CorpusSearch;
using choralesearch::Group;

namespace {


} // namespace

TEST_CASE(the_query_of_a_group_finds_exactly_its_matches) {
    const GroupQuery groupQuery = choralesearch::groupQueryFromJson(nlohmann::json::parse(R"({
        "query": {
            "feature": "deg",
            "voices": "soprano",
            "pattern": [{ "deg": "*" }, { "deg": "*", "fermata": true }]
        },
        "groupBy": [
            { "reads": [{ "feature": "deg" }] },
            { "reads": [{ "feature": "hint-14" }] }
        ]
    })"));
    CorpusSearch search(FIXTURES_DIR);
    const Group root = choralesearch::groupMatchesInCorpus(search, groupQuery);
    REQUIRE(root.groups.front().query.has_value());
    CHECK(choralesearch::verifyGroupMatchQueries(search, root).empty());
}

TEST_CASE(an_interval_is_read_across_the_ornaments_the_search_leaves_out) {
    // chor005 and chor008 decorate the soprano before some of their fermatas.
    const GroupQuery groupQuery = choralesearch::groupQueryFromJson(nlohmann::json::parse(R"({
        "query": {
            "feature": "mint",
            "voices": "soprano",
            "metweightSkipUnclassified": true,
            "pattern": [{ "mint": "*", "fermata": true }]
        },
        "groupBy": [{ "reads": [{ "feature": "mint" }] }]
    })"));
    CorpusSearch search(FIXTURES_DIR);
    const Group root = choralesearch::groupMatchesInCorpus(search, groupQuery);
    REQUIRE(!root.groups.empty());
    CHECK(choralesearch::verifyGroupMatchQueries(search, root).empty());
}

TEST_CASE(a_note_written_as_repeated_notes_is_one_note_where_the_query_lets_it_be) {
    // Two quarters and the fermata: a half note before the fermata, or two quarters repeating a
    // pitch, are both a single note with durationAllowSplitNotes.
    const auto groupQuery = [](bool split) {
        return choralesearch::groupQueryFromJson(nlohmann::json::parse(std::string(R"({
            "query": {
                "feature": "mint",
                "voices": "soprano",
                "metweightSkipUnclassified": true,
                "durationAllowMergedNotes": true,
                "durationAllowSplitNotes": )") + (split ? "true" : "false") + R"(,
                "pattern": [{ "duration": "4" }, { "duration": "4" }, { "mint": "*", "fermata": true }]
            },
            "groupBy": [{ "reads": [{ "feature": "mint", "positions": [1, 2] }] }]
        })"));
    };
    CorpusSearch search(FIXTURES_DIR);
    const Group apart = choralesearch::groupMatchesInCorpus(search, groupQuery(false));
    const Group together = choralesearch::groupMatchesInCorpus(search, groupQuery(true));
    // A repeated quarter is read as a unison into the second quarter; the fermata, which the
    // window gives no duration, stays a note of its own even where it repeats the pitch.
    const auto repeatsTheQuarter = [](const Group& group) {
        return group.values[0].size() == 2 && group.values[0].front() == "P1";
    };
    CHECK(std::any_of(apart.groups.begin(), apart.groups.end(), repeatsTheQuarter));
    CHECK(std::none_of(together.groups.begin(), together.groups.end(), repeatsTheQuarter));
    CHECK(together.groups.size() < apart.groups.size());
    CHECK(choralesearch::verifyGroupMatchQueries(search, together).empty());
}

TEST_CASE(a_note_is_as_long_as_all_the_notes_its_positions_attack) {
    // Without ornaments left out, a quarter may be two repeated eighths: the duration of the note
    // is that of both, and the query of every group finds exactly its matches.
    const GroupQuery groupQuery = choralesearch::groupQueryFromJson(nlohmann::json::parse(R"({
        "query": {
            "feature": "mint",
            "voices": "soprano",
            "durationAllowMergedNotes": true,
            "durationAllowSplitNotes": true,
            "pattern": [{ "duration": "4" }, { "duration": "4" }, { "mint": "*", "fermata": true }]
        },
        "groupBy": [{ "reads": [{ "feature": "mint", "positions": [1, 2] }, { "feature": "duration", "positions": [0, 1] }] }],
        "examples": { "maxCount": 0 }
    })"));
    CorpusSearch search(FIXTURES_DIR);
    const Group root = choralesearch::groupMatchesInCorpus(search, groupQuery);
    REQUIRE(!root.groups.empty());
    CHECK(choralesearch::verifyGroupMatchQueries(search, root).empty());
}

TEST_CASE(a_read_of_durations_needs_them_in_the_pattern_only_where_it_reads) {
    const GroupQuery groupQuery = choralesearch::groupQueryFromJson(nlohmann::json::parse(R"({
        "query": {
            "feature": "mint",
            "voices": "soprano",
            "durationAllowSplitNotes": true,
            "pattern": [{ "duration": "4" }, { "duration": "4" }, { "mint": "*", "fermata": true }]
        },
        "groupBy": [{ "reads": [{ "feature": "mint", "positions": [1, 2] }, { "feature": "duration", "positions": [0, 1] }] }]
    })"));
    CorpusSearch search(FIXTURES_DIR);
    const Group root = choralesearch::groupMatchesInCorpus(search, groupQuery);
    REQUIRE(!root.groups.empty());
    // The note with the fermata has no duration in the pattern, but the read does not read it.
    for (const Group& group : root.groups) CHECK(group.query.has_value());
    CHECK(choralesearch::verifyGroupMatchQueries(search, root).empty());
}

TEST_CASE(the_bass_in_its_own_rhythm_is_asked_for_as_a_simultaneous_group) {
    const GroupQuery groupQuery = choralesearch::groupQueryFromJson(nlohmann::json::parse(R"({
        "query": {
            "feature": "mint",
            "voices": "soprano",
            "metweightSkipUnclassified": true,
            "pattern": [{ "mint": "*" }, { "mint": "*", "fermata": true }]
        },
        "groupBy": [
            { "reads": [{ "feature": "mint", "positions": [1] }] },
            { "reads": [{ "feature": "mint", "notesOfVoice": "bass" }] }
        ]
    })"));
    CorpusSearch search(FIXTURES_DIR);
    const Group root = choralesearch::groupMatchesInCorpus(search, groupQuery);
    std::size_t asked = 0;
    const auto holds = [](const std::vector<std::string>& notes) {
        return std::find(notes.begin(), notes.end(), choralesearch::kHeldNoteValue) != notes.end();
    };
    for (const Group& cantusFirmus : root.groups) {
        for (const Group& bass : cantusFirmus.groups) {
            // A group that is its own member has a query; one with members leaves it to them.
            CHECK_EQ(bass.query.has_value(), bass.members.empty());
            if (bass.query) {
                ++asked;
                REQUIRE(bass.query->simultaneousWith.size() == 1u);
                CHECK_EQ(bass.query->simultaneousWith[0].pattern.size(), bass.values[0].size() + 1);
            }
            for (const auto& member : bass.members) {
                // A note held into the match or into its last note has no onset the query could begin or end with.
                CHECK_EQ(member.query.has_value(), !holds(member.values[0]));
                if (member.query) CHECK_EQ(member.query->simultaneousWith[0].pattern.size(), member.values[0].size() + 1);
            }
        }
    }
    CHECK(asked > 0u);
    CHECK(choralesearch::verifyGroupMatchQueries(search, root).empty());
}

TEST_MAIN()

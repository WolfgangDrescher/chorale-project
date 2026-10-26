#include "test_framework.hpp"

#include <algorithm>
#include <string>

#include "CorpusSearch.hpp"
#include "Grouping.hpp"
#include "GroupingJson.hpp"

using choralesearch::GroupQuery;
using choralesearch::CorpusSearch;
using choralesearch::Group;

namespace {


} // namespace

TEST_CASE(categories_reject_what_they_cannot_tell_apart) {
    const auto parse = [](const char* categories) {
        return choralesearch::groupQueryFromJson(nlohmann::json::parse(std::string(R"({
            "query": { "feature": "kern", "voices": "bass", "pattern": [{ "kern": "*" }] },
            "groupBy": [{ "reads": [{ "feature": "hint-14", "categories": )") + categories + "}] }]}"));
    };
    CHECK_NOTHROW(parse(R"({ "perfect": ["P5", "P8"], "other": "remaining" })"));
    // A string other than "remaining" is no category.
    CHECK_THROWS(parse(R"({ "perfect": ["P5", "P8"], "other": "rest" })"));
    CHECK_THROWS(parse(R"({ "perfect": "P5" })"));
    CHECK_THROWS(parse(R"({ "a": "remaining", "b": "remaining" })"));
    // A value in two categories.
    CHECK_THROWS(parse(R"({ "perfect": ["P5", "P8"], "octave": ["P8"] })"));
    CHECK_THROWS(parse(R"({ "perfect": [] })"));
}

TEST_CASE(a_group_query_rejects_what_it_cannot_read) {
    const auto parse = [](const char* text) { return choralesearch::groupQueryFromJson(nlohmann::json::parse(text)); };
    CHECK_THROWS(parse(R"({"query": {"feature": "deg", "pattern": [{"deg": "1"}]}})"));
    CHECK_THROWS(parse(R"({"query": {"feature": "deg", "pattern": [{"deg": "1"}]},
        "groupBy": [{"reads": [{"feature": "fermata"}]}]})"));
    CHECK_THROWS(parse(R"({"query": {"feature": "deg", "pattern": [{"deg": "1"}]},
        "groupBy": [{"reads": [{"feature": "deg", "positions": [1]}]}]})"));
    CHECK_THROWS(parse(R"({"query": {"feature": "deg", "pattern": [{"deg": "1"}]},
        "groupBy": [{"reads": [{"feature": "deg"}], "sort": "matches"}]})"));
    // Another voice is read in its own rhythm only, as intervals.
    CHECK_THROWS(parse(R"({"query": {"feature": "deg", "pattern": [{"deg": "1"}]},
        "groupBy": [{"reads": [{"feature": "deg", "notesOfVoice": "bass"}]}]})"));
    CHECK_THROWS(parse(R"({"query": {"feature": "deg", "pattern": [{"deg": "1"}]},
        "groupBy": [{"reads": [{"feature": "mint", "notesOfVoice": "bass", "positions": [0]}]}]})"));
    // The complement of an interval only in the rhythm of another voice.
    CHECK_THROWS(parse(R"({"query": {"feature": "mint", "voices": "soprano", "pattern": [{"mint": "*"}]},
        "groupBy": [{"reads": [{"feature": "mint", "mintAllowIntervalComplementation": true}]}]})"));
    CHECK_THROWS(parse(R"({"query": {"feature": "deg", "pattern": [{"deg": "1"}]},
        "groupBy": [{"reads": [{"feature": "deg"}]}], "examples": {"maxCount": -1}})"));
}

TEST_CASE(a_variant_is_an_object_of_the_keys_it_knows) {
    CHECK_THROWS(choralesearch::groupQueryFromJson(nlohmann::json::parse(R"({
        "query": { "feature": "mint", "voices": "soprano", "pattern": [{ "mint": "*" }] },
        "groupBy": [{ "reads": [{ "feature": "mint" }] }],
        "variants": { "ignoreRhythm": { "ignoreRhythm": true } }
    })")));
    // A variant is an object, whatever its name.
    CHECK_THROWS(choralesearch::groupQueryFromJson(nlohmann::json::parse(R"({
        "query": { "feature": "mint", "voices": "soprano", "pattern": [{ "mint": "*" }] },
        "groupBy": [{ "reads": [{ "feature": "mint" }] }],
        "variants": { "ignoreQuality": true }
    })")));
}

TEST_CASE(the_pattern_of_a_variant_has_a_position_for_each_position_of_the_query_at_most) {
    // The keys of a position, not a list of positions.
    CHECK_THROWS(choralesearch::groupQueryFromJson(nlohmann::json::parse(R"({
        "query": { "feature": "kern", "voices": "bass", "pattern": [{ "kern": "*" }] },
        "groupBy": [{ "reads": [{ "feature": "hint-14" }] }],
        "variants": { "phraseStart": { "pattern": { "phrase": "start" } } }
    })")));
    // One position more than the pattern of the query has.
    CHECK_THROWS(choralesearch::groupQueryFromJson(nlohmann::json::parse(R"({
        "query": { "feature": "kern", "voices": "bass", "pattern": [{ "kern": "*" }] },
        "groupBy": [{ "reads": [{ "feature": "hint-14" }] }],
        "variants": { "phraseStart": { "pattern": [{}, { "phrase": "start" }] } }
    })")));
}

TEST_CASE(a_group_is_written_with_the_keys_the_webapp_reads) {
    const GroupQuery groupQuery = choralesearch::groupQueryFromJson(nlohmann::json::parse(R"({
        "query": { "feature": "kern", "voices": "bass", "hintReduceCompound": true, "pattern": [{ "kern": "*" }] },
        "groupBy": [{ "reads": [{ "feature": "hint-14", "categories": { "perfect": ["P1", "P5", "P8"], "other": "remaining" } }] }],
        "examples": { "maxCount": 1 }
    })"));
    CorpusSearch search(FIXTURES_DIR);
    const nlohmann::json root = choralesearch::groupToJson(choralesearch::groupMatchesInCorpus(search, groupQuery));
    // The root has no values of its own.
    CHECK(!root.contains("values"));
    CHECK(root["query"].is_object());
    CHECK_EQ(root["omittedMatchCount"], 0);
    const nlohmann::json& groups = root["groups"];
    const auto perfect = std::find_if(groups.begin(), groups.end(), [](const nlohmann::json& group) { return group["label"] == "perfect"; });
    REQUIRE(perfect != groups.end());
    // A group of several values is found by the queries of its members together.
    REQUIRE((*perfect)["members"].size() > 1u);
    CHECK((*perfect)["query"].is_array());
    CHECK_EQ((*perfect)["query"].size(), (*perfect)["members"].size());
    REQUIRE((*perfect)["examples"].size() == 1u);
    CHECK((*perfect)["examples"][0].contains("groupLabel"));
    CHECK_EQ((*perfect)["examples"][0]["measures"].size(), std::size_t{2});
}

TEST_MAIN()

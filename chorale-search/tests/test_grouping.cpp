#include "test_framework.hpp"

#include <algorithm>
#include <set>
#include <stdexcept>

#include "CorpusSearch.hpp"
#include "GroupMatchQueries.hpp"
#include "Grouping.hpp"
#include "GroupingJson.hpp"

using choralesearch::GroupQuery;
using choralesearch::CorpusSearch;
using choralesearch::GroupExample;
using choralesearch::Group;
using choralesearch::HumdrumChorale;
using choralesearch::Query;
using choralesearch::Result;

namespace {


// The last two notes of the soprano before and at every fermata, by their scale degrees, and
// below them the intervals to the bass at the same two notes.
GroupQuery cadenceGroupQuery() {
    return choralesearch::groupQueryFromJson(nlohmann::json::parse(R"({
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
}

std::size_t sumOfGroupsBelow(const Group& group) {
    std::size_t sum = group.omittedMatchCount;
    for (const Group& groupBelow : group.groups) sum += groupBelow.matchCount;
    return sum;
}

} // namespace

TEST_CASE(group_corpus_counts_every_match_in_one_group_of_each_level) {
    CorpusSearch search(FIXTURES_DIR);
    const Group root = choralesearch::groupMatchesInCorpus(search, cadenceGroupQuery());
    REQUIRE(!root.groups.empty());
    CHECK_EQ(sumOfGroupsBelow(root), root.matchCount);
    for (const Group& group : root.groups) {
        CHECK_EQ(sumOfGroupsBelow(group), group.matchCount);
        REQUIRE(group.values.size() == 1u);
        CHECK_EQ(group.values[0].size(), std::size_t{2});
    }
    // The most frequent group first.
    for (std::size_t i = 1; i < root.groups.size(); ++i) CHECK(root.groups[i - 1].matchCount >= root.groups[i].matchCount);
}

TEST_CASE(intervals_are_read_without_their_quality_where_asked) {
    const GroupQuery groupQuery = choralesearch::groupQueryFromJson(nlohmann::json::parse(R"({
        "query": {
            "feature": "mint",
            "voices": "soprano",
            "pattern": [{ "mint": "*", "fermata": true }]
        },
        "groupBy": [{ "reads": [{ "feature": "mint", "ignoreQuality": true }] }]
    })"));
    CorpusSearch search(FIXTURES_DIR);
    const Group root = choralesearch::groupMatchesInCorpus(search, groupQuery);
    for (const Group& group : root.groups) {
        const std::string& interval = group.values[0][0];
        // A unison keeps its quality, every other interval is a direction and a number.
        if (interval != "P1") CHECK(interval.find_first_of("PMmAd") == std::string::npos);
    }
    CHECK(choralesearch::verifyGroupMatchQueries(search, root).empty());
}

TEST_CASE(a_variant_reads_every_interval_of_the_group_query_without_its_quality) {
    const GroupQuery groupQuery = choralesearch::groupQueryFromJson(nlohmann::json::parse(R"({
        "query": {
            "feature": "mint",
            "voices": "soprano",
            "pattern": [{ "mint": "*" }, { "mint": "*", "fermata": true }]
        },
        "groupBy": [
            { "reads": [{ "feature": "mint" }] },
            { "reads": [{ "feature": "mint", "notesOfVoice": "bass" }] }
        ],
        "variants": { "ignoreQuality": { "ignoreQuality": true } }
    })"));
    REQUIRE(groupQuery.variants.size() == 1u);
    const GroupQuery variant = choralesearch::applyVariant(groupQuery, groupQuery.variants.front());
    CHECK(variant.variants.empty());
    for (const auto& level : variant.levels) {
        for (const auto& read : level.reads) CHECK(read.ignoreQuality);
    }
}

TEST_CASE(a_variant_adds_keys_to_a_position_of_the_pattern) {
    const GroupQuery groupQuery = choralesearch::groupQueryFromJson(nlohmann::json::parse(R"({
        "query": { "feature": "kern", "voices": "bass", "hintReduceCompound": true, "pattern": [{ "kern": "*" }] },
        "groupBy": [{ "reads": [{ "feature": "hint-14" }] }],
        "variants": {
            "phraseStart": { "pattern": [{ "phrase": "start" }] },
            "phraseMiddle": { "pattern": [{ "!phrase": ["start", "end"] }] },
            "phraseEnd": { "pattern": [{ "phrase": "end" }] }
        }
    })"));
    REQUIRE(groupQuery.variants.size() == 3u);
    CorpusSearch search(FIXTURES_DIR);
    const Group all = choralesearch::groupMatchesInCorpus(search, groupQuery);
    std::size_t matches = 0;
    for (const auto& variant : groupQuery.variants) {
        const GroupQuery read = choralesearch::applyVariant(groupQuery, variant);
        CHECK(read.query.pattern[0].count("kern") == 1u);
        const Group root = choralesearch::groupMatchesInCorpus(search, read);
        CHECK(root.matchCount > 0u);
        CHECK(root.matchCount < all.matchCount);
        CHECK(choralesearch::verifyGroupMatchQueries(search, root).empty());
        matches += root.matchCount;
    }
    // Every note opens a phrase, ends one or stands in the middle of one.
    CHECK_EQ(matches, all.matchCount);
}

TEST_CASE(values_are_read_by_their_categories) {
    const GroupQuery groupQuery = choralesearch::groupQueryFromJson(nlohmann::json::parse(R"({
        "query": { "feature": "kern", "voices": "bass", "hintReduceCompound": true, "pattern": [{ "kern": "*" }] },
        "groupBy": [{ "reads": [{ "feature": "hint-14", "categories": {
            "perfect": ["P1", "P5", "P8"],
            "imperfect": ["m3", "M3", "m6", "M6"],
            "dissonant": "remaining"
        } }] }]
    })"));
    CorpusSearch search(FIXTURES_DIR);
    const Group root = choralesearch::groupMatchesInCorpus(search, groupQuery);
    std::set<std::string> classes;
    for (const Group& group : root.groups) {
        classes.insert(group.values[0][0]);
        // A class written one way only is that way, with its query.
        if (group.members.empty()) continue;
        CHECK(!group.query);
        std::size_t matches = 0;
        for (const auto& member : group.members) {
            matches += member.matchCount;
            const std::string& interval = member.values[0][0];
            if (group.values[0][0] == "perfect") CHECK(interval == "P1" || interval == "P5" || interval == "P8");
            if (group.values[0][0] == "imperfect") CHECK(interval.find_first_of("36") != std::string::npos);
        }
        CHECK_EQ(matches, group.matchCount);
    }
    CHECK(classes.count("perfect") == 1u);
    CHECK(classes.count("imperfect") == 1u);
    CHECK(classes.count("dissonant") == 1u);
    CHECK(choralesearch::verifyGroupMatchQueries(search, root).empty());
}

TEST_CASE(a_value_in_no_category_stays_itself_without_a_remaining_one) {
    const GroupQuery groupQuery = choralesearch::groupQueryFromJson(nlohmann::json::parse(R"({
        "query": { "feature": "kern", "voices": "bass", "hintReduceCompound": true, "pattern": [{ "kern": "*" }] },
        "groupBy": [{ "reads": [{ "feature": "hint-14", "categories": { "perfect": ["P1", "P5", "P8"] } }] }]
    })"));
    CorpusSearch search(FIXTURES_DIR);
    const Group root = choralesearch::groupMatchesInCorpus(search, groupQuery);
    REQUIRE(root.groups.size() > 1u);
    CHECK(std::any_of(root.groups.begin(), root.groups.end(), [](const Group& group) { return group.label == "perfect"; }));
    CHECK(std::any_of(root.groups.begin(), root.groups.end(), [](const Group& group) { return group.label == "m3"; }));
}

TEST_CASE(a_voice_read_by_interval_classes_lists_how_each_group_is_written) {
    const GroupQuery groupQuery = choralesearch::groupQueryFromJson(nlohmann::json::parse(R"({
        "query": {
            "feature": "mint",
            "voices": "soprano",
            "metweightSkipUnclassified": true,
            "pattern": [{ "mint": "*" }, { "mint": "*", "fermata": true }]
        },
        "groupBy": [{ "reads": [{ "feature": "mint", "notesOfVoice": "bass", "mintAllowIntervalComplementation": true }] }]
    })"));
    CorpusSearch search(FIXTURES_DIR);
    const Group root = choralesearch::groupMatchesInCorpus(search, groupQuery);
    std::size_t written = 0;
    for (const Group& group : root.groups) {
        for (const std::string& interval : group.values[0]) {
            // No repeated note, and every interval within a fourth.
            CHECK(interval != "P1" && interval.find("P8") == std::string::npos);
            if (interval != choralesearch::kHeldNoteValue) CHECK(interval.back() >= '1' && interval.back() <= '4');
        }
        if (group.members.empty()) continue;
        ++written;
        CHECK(!group.query);
        // A lone member that reads like its group would say nothing.
        CHECK(group.members.size() > 1 || group.members.front().label != group.label);
        // Named after its most frequent spelling, without the ties.
        std::string spelling = group.members.front().label;
        for (auto tie = spelling.find("\u203F"); tie != std::string::npos; tie = spelling.find("\u203F")) spelling.erase(tie, std::string("\u203F").size());
        CHECK_EQ(group.label, spelling);
        std::size_t matches = 0;
        for (const auto& member : group.members) matches += member.matchCount;
        CHECK_EQ(matches, group.matchCount);
    }
    CHECK(written > 0u);
    CHECK(choralesearch::verifyGroupMatchQueries(search, root).empty());
}

TEST_CASE(a_held_note_of_a_voice_is_a_member_of_its_group) {
    const GroupQuery groupQuery = choralesearch::groupQueryFromJson(nlohmann::json::parse(R"({
        "query": {
            "feature": "mint",
            "voices": "soprano",
            "metweightSkipUnclassified": true,
            "pattern": [{ "mint": "*" }, { "mint": "*", "fermata": true }]
        },
        "groupBy": [{ "reads": [{ "feature": "mint", "notesOfVoice": "bass" }] }]
    })"));
    CorpusSearch search(FIXTURES_DIR);
    const Group root = choralesearch::groupMatchesInCorpus(search, groupQuery);
    bool held = false;
    for (const Group& group : root.groups) {
        for (const std::string& interval : group.values[0]) CHECK(interval != choralesearch::kHeldNoteValue);
        CHECK(group.label.find("\u203F") == std::string::npos);
        for (const auto& member : group.members) {
            const auto& intervals = member.values[0];
            if (std::find(intervals.begin(), intervals.end(), choralesearch::kHeldNoteValue) != intervals.end()) {
                held = true;
                CHECK(member.label.find("\u203F") != std::string::npos);
                CHECK(!member.query);
            }
        }
    }
    CHECK(held);
    CHECK(choralesearch::verifyGroupMatchQueries(search, root).empty());
}

TEST_CASE(a_duration_is_labelled_as_its_note) {
    const GroupQuery groupQuery = choralesearch::groupQueryFromJson(nlohmann::json::parse(R"({
        "query": {
            "feature": "mint",
            "voices": "soprano",
            "durationAllowMergedNotes": true,
            "durationAllowSplitNotes": true,
            "pattern": [{ "duration": "4" }, { "mint": "*", "fermata": true }]
        },
        "groupBy": [{ "reads": [{ "feature": "mint", "positions": [1] }, { "feature": "duration", "positions": [0] }] }]
    })"));
    CorpusSearch search(FIXTURES_DIR);
    const Group root = choralesearch::groupMatchesInCorpus(search, groupQuery);
    REQUIRE(!root.groups.empty());
    for (const Group& group : root.groups) {
        // The note before the interval into the note with the fermata: "\U0001D15F -M2".
        const std::string note = "\U0001D15F ";
        CHECK_EQ(group.label.substr(0, note.size()), note);
        CHECK_EQ(group.label.substr(note.size()), group.values[0][0]);
    }
}

TEST_CASE(a_group_shows_the_unornamented_example_first_then_one_for_each_set_of_ornamented_voices) {
    const GroupQuery groupQuery = choralesearch::groupQueryFromJson(nlohmann::json::parse(R"({
        "query": {
            "feature": "mint",
            "voices": "soprano",
            "metweightSkipUnclassified": true,
            "pattern": [{ "mint": "*" }, { "mint": "*", "fermata": true }]
        },
        "groupBy": [{ "reads": [{ "feature": "mint", "notesOfVoice": "bass" }] }]
    })"));
    CorpusSearch search(FIXTURES_DIR);
    const Group root = choralesearch::groupMatchesInCorpus(search, groupQuery);
    for (const Group& group : root.groups) {
        REQUIRE(!group.examples.empty());
        CHECK(group.examples.size() <= 5);
        std::set<std::vector<std::size_t>> ornamentations;
        std::set<std::pair<std::string, std::size_t>> passages;
        std::size_t ornamentedVoices = 0;
        for (const auto& example : group.examples) {
            CHECK(example.firstMeasure <= example.lastMeasure);
            CHECK(passages.emplace(example.choraleId, example.startLine).second);
            // A set of ornamented voices first shown after one with more voices would be out of order.
            if (ornamentations.insert(example.ornamentedVoices).second) {
                CHECK(ornamentedVoices <= example.ornamentedVoices.size());
                ornamentedVoices = example.ornamentedVoices.size();
            }
        }
    }
}

TEST_CASE(max_count_sets_how_many_examples_each_group_shows) {
    CorpusSearch search(FIXTURES_DIR);
    for (const std::size_t maxCount : {std::size_t{0}, std::size_t{2}}) {
        GroupQuery groupQuery = cadenceGroupQuery();
        groupQuery.examples.maxCount = maxCount;
        const Group root = choralesearch::groupMatchesInCorpus(search, groupQuery);
        CHECK(root.examples.size() <= maxCount);
        for (const Group& group : root.groups) {
            CHECK(group.examples.size() <= maxCount);
            if (maxCount > 0) CHECK(!group.examples.empty());
        }
    }
}

TEST_CASE(a_group_with_groups_below_it_shows_an_example_of_each_of_the_most_frequent) {
    CorpusSearch search(FIXTURES_DIR);
    const Group root = choralesearch::groupMatchesInCorpus(search, cadenceGroupQuery());
    for (const Group& cantusFirmus : root.groups) {
        REQUIRE(!cantusFirmus.groups.empty());
        CHECK_EQ(cantusFirmus.examples.size(), std::min<std::size_t>(cantusFirmus.groups.size(), 5));
        for (std::size_t i = 0; i < cantusFirmus.examples.size(); ++i) {
            const GroupExample& example = cantusFirmus.examples[i];
            const Group& bass = cantusFirmus.groups[i];
            CHECK_EQ(example.choraleId, bass.examples.front().choraleId);
            CHECK_EQ(example.startLine, bass.examples.front().startLine);
            CHECK(!example.groupLabel.empty());
        }
    }
}

TEST_CASE(max_group_count_keeps_the_most_frequent_groups_and_counts_the_rest) {
    GroupQuery groupQuery = cadenceGroupQuery();
    groupQuery.levels.resize(1);
    groupQuery.levels[0].maxGroupCount = 2;
    CorpusSearch search(FIXTURES_DIR);
    const Group root = choralesearch::groupMatchesInCorpus(search, groupQuery);
    CHECK_EQ(root.groups.size(), std::size_t{2});
    CHECK(root.omittedMatchCount > 0u);
    CHECK_EQ(sumOfGroupsBelow(root), root.matchCount);
}

TEST_MAIN()

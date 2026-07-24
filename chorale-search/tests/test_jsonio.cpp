#include "test_framework.hpp"

#include "JsonIO.hpp"

using choralesearch::queryFromJson;
using choralesearch::Result;
using choralesearch::Query;
using choralesearch::resultsToJson;
using choralesearch::resultToJson;
using choralesearch::resultsGroupedByChoraleToJson;
using nlohmann::json;

TEST_CASE(query_from_json_parses_minimal_query) {
    Query q = queryFromJson(json::parse(R"({"feature":"deg","pattern":[{"deg":"1"}]})"));
    CHECK_EQ(q.feature, std::string("deg"));
    CHECK_EQ(q.voices, std::string("all")); // default when omitted
    REQUIRE(q.pattern.size() == 1u);
    REQUIRE(q.pattern[0].count("deg") == 1u);
    CHECK_EQ(q.pattern[0]["deg"], (std::vector<std::string>{"1"}));
    CHECK(!q.limit.has_value());
    CHECK(!q.mintStartAtPreviousToken); // default when omitted
}

TEST_CASE(query_from_json_reads_voices_and_limit) {
    Query q = queryFromJson(json::parse(R"({"feature":"mint","voices":"1234","pattern":[{"mint":"+2"}],"limit":5})"));
    CHECK_EQ(q.voices, std::string("1234"));
    REQUIRE(q.limit.has_value());
    CHECK_EQ(*q.limit, 5u);
}

TEST_CASE(query_from_json_reads_mint_start_at_previous_token) {
    Query q = queryFromJson(json::parse(
        R"({"feature":"mint","pattern":[{"mint":"-M2"}],"mintStartAtPreviousToken":true})"));
    CHECK(q.mintStartAtPreviousToken);
}

TEST_CASE(query_from_json_reads_fb_compare_exact_chord) {
    Query q = queryFromJson(json::parse(
        R"({"feature":"fb","pattern":[{"fb":"6 3"}],"fbCompareExactChord":true})"));
    CHECK(q.fbCompareExactChord);
}

TEST_CASE(query_from_json_reads_kern_ignore_octave) {
    Query q = queryFromJson(json::parse(
        R"({"feature":"kern","pattern":[{"kern":"G"}],"kernIgnoreOctave":true})"));
    CHECK(q.kernIgnoreOctave);
}

TEST_CASE(query_from_json_defaults_kern_ignore_octave_to_false) {
    Query q = queryFromJson(json::parse(R"({"feature":"kern","pattern":[{"kern":"G"}]})"));
    CHECK(!q.kernIgnoreOctave);
}

TEST_CASE(query_from_json_defaults_simultaneous_alignment_to_start) {
    Query q = queryFromJson(json::parse(R"({"feature":"kern","pattern":[{"kern":"G"}]})"));
    CHECK_EQ(q.simultaneousAlignment, std::string("start"));
}

TEST_CASE(query_from_json_reads_simultaneous_alignment) {
    Query q = queryFromJson(json::parse(
        R"({"feature":"kern","pattern":[{"kern":"G"}],"simultaneousAlignment":"end"})"));
    CHECK_EQ(q.simultaneousAlignment, std::string("end"));
}

TEST_CASE(query_from_json_accepts_start_end_as_simultaneous_alignment) {
    Query q = queryFromJson(json::parse(
        R"({"feature":"kern","pattern":[{"kern":"G"}],"simultaneousAlignment":"start-end"})"));
    CHECK_EQ(q.simultaneousAlignment, std::string("start-end"));
}

TEST_CASE(query_from_json_rejects_unknown_simultaneous_alignment) {
    CHECK_THROWS(queryFromJson(json::parse(
        R"({"feature":"kern","pattern":[{"kern":"G"}],"simultaneousAlignment":"middle"})")));
}

TEST_CASE(query_from_json_defaults_simultaneous_with_to_empty) {
    Query q = queryFromJson(json::parse(R"({"feature":"kern","pattern":[{"kern":"G"}]})"));
    CHECK(q.simultaneousWith.empty());
}

TEST_CASE(query_from_json_reads_a_minimal_simultaneous_with_group) {
    Query q = queryFromJson(json::parse(R"({
        "feature":"kern",
        "pattern":[{"kern":"G"}],
        "simultaneousWith":[{"feature":"kern","pattern":[{"fermata":true}]}]
    })"));
    REQUIRE(q.simultaneousWith.size() == 1u);
    const auto& group = q.simultaneousWith[0];
    CHECK_EQ(group.feature, std::string("kern"));
    CHECK_EQ(group.voices, std::string("all")); // default when omitted
    REQUIRE(group.pattern.size() == 1u);

    // nullopt everywhere: a group with no options of its own inherits the query's.
    CHECK(!group.mintStartAtPreviousToken.has_value());
    CHECK(!group.fbCompareExactChord.has_value());
    CHECK(!group.kernIgnoreOctave.has_value());
    CHECK(!group.simultaneousAlignment.has_value());
}

TEST_CASE(query_from_json_reads_simultaneous_with_group_options_as_overrides) {
    Query q = queryFromJson(json::parse(R"({
        "feature":"kern",
        "pattern":[{"kern":"G"}],
        "simultaneousWith":[{
            "feature":"mint",
            "voices":"bass",
            "pattern":[{"mint":"-2"}],
            "mintStartAtPreviousToken":true,
            "fbCompareExactChord":true,
            "kernIgnoreOctave":true,
            "simultaneousAlignment":"end"
        }]
    })"));
    REQUIRE(q.simultaneousWith.size() == 1u);
    const auto& group = q.simultaneousWith[0];
    CHECK_EQ(group.voices, std::string("bass"));
    REQUIRE(group.mintStartAtPreviousToken.has_value());
    CHECK(*group.mintStartAtPreviousToken);
    REQUIRE(group.fbCompareExactChord.has_value());
    CHECK(*group.fbCompareExactChord);
    REQUIRE(group.kernIgnoreOctave.has_value());
    CHECK(*group.kernIgnoreOctave);
    REQUIRE(group.simultaneousAlignment.has_value());
    CHECK_EQ(*group.simultaneousAlignment, std::string("end"));
}

TEST_CASE(query_from_json_reads_several_simultaneous_with_groups_in_order) {
    Query q = queryFromJson(json::parse(R"({
        "feature":"kern",
        "pattern":[{"kern":"G"}],
        "simultaneousWith":[
            {"feature":"kern","voices":"bass","pattern":[{"fermata":true}]},
            {"feature":"kern","voices":"tenor","pattern":[{"fermata":true}]}
        ]
    })"));
    REQUIRE(q.simultaneousWith.size() == 2u);
    CHECK_EQ(q.simultaneousWith[0].voices, std::string("bass"));
    CHECK_EQ(q.simultaneousWith[1].voices, std::string("tenor"));
}

TEST_CASE(query_from_json_rejects_non_array_simultaneous_with) {
    CHECK_THROWS(queryFromJson(json::parse(
        R"({"feature":"kern","pattern":[{"kern":"G"}],"simultaneousWith":"nope"})")));
}

TEST_CASE(query_from_json_rejects_non_object_simultaneous_with_entry) {
    CHECK_THROWS(queryFromJson(json::parse(
        R"({"feature":"kern","pattern":[{"kern":"G"}],"simultaneousWith":["nope"]})")));
}

TEST_CASE(query_from_json_rejects_simultaneous_with_entry_missing_feature) {
    CHECK_THROWS(queryFromJson(json::parse(
        R"({"feature":"kern","pattern":[{"kern":"G"}],"simultaneousWith":[{"pattern":[{"fermata":true}]}]})")));
}

TEST_CASE(query_from_json_rejects_simultaneous_with_entry_missing_pattern) {
    CHECK_THROWS(queryFromJson(json::parse(
        R"({"feature":"kern","pattern":[{"kern":"G"}],"simultaneousWith":[{"feature":"kern"}]})")));
}

TEST_CASE(query_from_json_error_message_references_the_failing_simultaneous_with_index) {
    try {
        queryFromJson(json::parse(R"({
            "feature":"kern",
            "pattern":[{"kern":"G"}],
            "simultaneousWith":[
                {"feature":"kern","pattern":[{"fermata":true}]},
                {"feature":"kern"}
            ]
        })"));
        REQUIRE(false); // must throw
    } catch (const std::invalid_argument& e) {
        std::string msg = e.what();
        CHECK(msg.find("simultaneousWith[1]") != std::string::npos);
    }
}

TEST_CASE(query_from_json_accepts_boolean_attribute_value) {
    Query q = queryFromJson(json::parse(R"({"feature":"kern","pattern":[{"fermata":true}]})"));
    CHECK_EQ(q.pattern[0]["fermata"], (std::vector<std::string>{"true"}));
}

TEST_CASE(query_from_json_accepts_array_or_list_for_attribute_value) {
    Query q = queryFromJson(json::parse(R"({"feature":"deg","pattern":[{"deg":["1","3","5"]}]})"));
    CHECK_EQ(q.pattern[0]["deg"], (std::vector<std::string>{"1", "3", "5"}));
}

TEST_CASE(query_from_json_preserves_a_negated_key_as_is) {
    Query q = queryFromJson(json::parse(R"({"feature":"deg","pattern":[{"!deg":["1","3"]}]})"));
    REQUIRE(q.pattern[0].count("!deg") == 1u);
    CHECK_EQ(q.pattern[0]["!deg"], (std::vector<std::string>{"1", "3"}));
}

TEST_CASE(query_from_json_requires_feature_field) {
    CHECK_THROWS(queryFromJson(json::parse(R"({"pattern":[{"deg":"1"}]})")));
}

TEST_CASE(query_from_json_requires_feature_to_be_string) {
    CHECK_THROWS(queryFromJson(json::parse(R"({"feature":42,"pattern":[{"deg":"1"}]})")));
}

TEST_CASE(query_from_json_requires_pattern_field) {
    CHECK_THROWS(queryFromJson(json::parse(R"({"feature":"deg"})")));
}

TEST_CASE(query_from_json_rejects_non_array_pattern) {
    CHECK_THROWS(queryFromJson(json::parse(R"({"feature":"deg","pattern":"not-an-array"})")));
}

TEST_CASE(query_from_json_rejects_empty_pattern) {
    CHECK_THROWS(queryFromJson(json::parse(R"({"feature":"deg","pattern":[]})")));
}

TEST_CASE(query_from_json_rejects_non_object_pattern_entry) {
    CHECK_THROWS(queryFromJson(json::parse(R"({"feature":"deg","pattern":["nope"]})")));
}

TEST_CASE(query_from_json_rejects_empty_or_list) {
    CHECK_THROWS(queryFromJson(json::parse(R"({"feature":"deg","pattern":[{"deg":[]}]})")));
}

TEST_CASE(query_from_json_rejects_non_string_array_entries) {
    CHECK_THROWS(queryFromJson(json::parse(R"({"feature":"deg","pattern":[{"deg":[1,2]}]})")));
}

TEST_CASE(query_from_json_rejects_wrong_typed_attribute_value) {
    CHECK_THROWS(queryFromJson(json::parse(R"({"feature":"deg","pattern":[{"deg":3.14}]})")));
}

TEST_CASE(result_to_json_round_trips_fields) {
    Result r;
    r.choraleId = "chor029";
    r.feature = "deg";
    r.voiceLabel = "Soprano";
    r.voice = 4;
    r.startPosition = "4";
    r.endPosition = "5";
    r.startLineNumber = 12;
    r.endLineNumber = 13;

    json j = resultToJson(r);
    CHECK_EQ(j.at("chorale").get<std::string>(), std::string("chor029"));
    CHECK_EQ(j.at("feature").get<std::string>(), std::string("deg"));
    CHECK_EQ(j.at("voiceLabel").get<std::string>(), std::string("Soprano"));
    CHECK_EQ(j.at("voice").get<std::size_t>(), 4u);
    CHECK_EQ(j.at("startPosition").get<std::string>(), std::string("4"));
    CHECK_EQ(j.at("endPosition").get<std::string>(), std::string("5"));
    CHECK_EQ(j.at("startLine").get<std::size_t>(), 12u);
    CHECK_EQ(j.at("endLine").get<std::size_t>(), 13u);
}

TEST_CASE(results_to_json_produces_array_in_order) {
    Result a;
    a.choraleId = "chor001";
    Result b;
    b.choraleId = "chor002";

    json arr = resultsToJson({a, b});
    REQUIRE(arr.is_array());
    REQUIRE(arr.size() == 2u);
    CHECK_EQ(arr[0].at("chorale").get<std::string>(), std::string("chor001"));
    CHECK_EQ(arr[1].at("chorale").get<std::string>(), std::string("chor002"));
}

TEST_CASE(results_to_json_handles_empty_list) {
    json arr = resultsToJson({});
    REQUIRE(arr.is_array());
    CHECK_EQ(arr.size(), 0u);
}

TEST_CASE(results_grouped_by_chorale_to_json) {
    Result a;
    a.choraleId = "chor001";
    a.startPosition = "1";
    Result b;
    b.choraleId = "chor002";
    b.startPosition = "2";
    Result c;
    c.choraleId = "chor001";
    c.startPosition = "3";

    json obj = resultsGroupedByChoraleToJson({a, b, c});
    REQUIRE(obj.is_object());
    REQUIRE(obj.size() == 2u);

    REQUIRE(obj.at("chor001").is_array());
    REQUIRE(obj.at("chor001").size() == 2u);
    CHECK_EQ(obj.at("chor001")[0].at("startPosition").get<std::string>(), std::string("1"));
    CHECK_EQ(obj.at("chor001")[1].at("startPosition").get<std::string>(), std::string("3"));

    REQUIRE(obj.at("chor002").is_array());
    REQUIRE(obj.at("chor002").size() == 1u);
    CHECK_EQ(obj.at("chor002")[0].at("startPosition").get<std::string>(), std::string("2"));
}

TEST_CASE(results_grouped_by_chorale_to_json_handles_empty_list) {
    json obj = resultsGroupedByChoraleToJson({});
    REQUIRE(obj.is_object());
    CHECK_EQ(obj.size(), 0u);
}

TEST_MAIN()

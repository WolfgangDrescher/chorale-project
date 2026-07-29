#include "test_framework.hpp"

#include "JsonIO.hpp"

using choralesearch::queryArrayFromJson;
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

TEST_CASE(query_from_json_rejects_mint_start_at_previous_token_for_another_driving_feature) {
    // The shift only means anything while walking mint's own onsets.
    CHECK_THROWS(queryFromJson(json::parse(
        R"({"feature":"kern","pattern":[{"kern":"G"}],"mintStartAtPreviousToken":true})")));
    CHECK_THROWS(queryFromJson(json::parse(
        R"({"feature":"hint-14","pattern":[{"hint-14":"3"}],"mintStartAtPreviousToken":true})")));
}

TEST_CASE(query_from_json_accepts_mint_start_at_previous_token_set_to_false_for_any_feature) {
    Query q = queryFromJson(json::parse(
        R"({"feature":"kern","pattern":[{"kern":"G"}],"mintStartAtPreviousToken":false})"));
    CHECK(!q.mintStartAtPreviousToken);
}

TEST_CASE(query_from_json_rejects_mint_start_at_previous_token_for_a_non_mint_simultaneous_group) {
    // Each group carries its own driving feature, so the check follows that one, not the
    // top-level query's.
    CHECK_THROWS(queryFromJson(json::parse(R"({
        "feature":"mint",
        "pattern":[{"mint":"-2"}],
        "simultaneousWith":[{
            "feature":"kern",
            "pattern":[{"kern":"G"}],
            "mintStartAtPreviousToken":true
        }]
    })")));
}

TEST_CASE(query_from_json_reads_mint_allow_interval_complementation_array) {
    Query q = queryFromJson(json::parse(
        R"({"feature":"mint","pattern":[{"mint":"-5"}],"mintAllowIntervalComplementation":["4","5"]})"));
    CHECK_EQ(q.mintAllowIntervalComplementation, (std::vector<std::string>{"4", "5"}));
}

TEST_CASE(query_from_json_reads_a_single_mint_allow_interval_complementation_value) {
    Query q = queryFromJson(json::parse(
        R"({"feature":"mint","pattern":[{"mint":"-5"}],"mintAllowIntervalComplementation":"5"})"));
    CHECK_EQ(q.mintAllowIntervalComplementation, (std::vector<std::string>{"5"}));
}

TEST_CASE(query_from_json_reads_mint_allow_interval_complementation_written_as_numbers) {
    Query q = queryFromJson(json::parse(
        R"({"feature":"mint","pattern":[{"mint":"-5"}],"mintAllowIntervalComplementation":[4,5]})"));
    CHECK_EQ(q.mintAllowIntervalComplementation, (std::vector<std::string>{"4", "5"}));

    Query bare = queryFromJson(json::parse(
        R"({"feature":"mint","pattern":[{"mint":"-5"}],"mintAllowIntervalComplementation":5})"));
    CHECK_EQ(bare.mintAllowIntervalComplementation, (std::vector<std::string>{"5"}));
}

TEST_CASE(query_from_json_reads_the_mint_allow_interval_complementation_wildcard) {
    Query q = queryFromJson(json::parse(
        R"({"feature":"mint","pattern":[{"mint":"-5"}],"mintAllowIntervalComplementation":["*"]})"));
    CHECK_EQ(q.mintAllowIntervalComplementation, (std::vector<std::string>{"*"}));

    Query q2 = queryFromJson(json::parse(
        R"({"feature":"mint","pattern":[{"mint":"-5"}],"mintAllowIntervalComplementation":"*"})"));
    CHECK_EQ(q2.mintAllowIntervalComplementation, (std::vector<std::string>{"*"}));
}

TEST_CASE(query_from_json_defaults_mint_allow_interval_complementation_to_empty) {
    Query q = queryFromJson(json::parse(R"({"feature":"mint","pattern":[{"mint":"-5"}]})"));
    CHECK(q.mintAllowIntervalComplementation.empty());
}

TEST_CASE(query_from_json_reads_simultaneous_with_group_mint_allow_interval_complementation_override) {
    Query q = queryFromJson(json::parse(R"({
        "feature":"kern",
        "pattern":[{"kern":"G"}],
        "simultaneousWith":[{
            "feature":"mint",
            "pattern":[{"mint":"-5"}],
            "mintAllowIntervalComplementation":["5"]
        }]
    })"));
    REQUIRE(q.simultaneousWith.size() == 1u);
    REQUIRE(q.simultaneousWith[0].mintAllowIntervalComplementation.has_value());
    CHECK_EQ(*q.simultaneousWith[0].mintAllowIntervalComplementation, (std::vector<std::string>{"5"}));
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

TEST_CASE(query_from_json_reads_hint_reduce_compound) {
    Query q = queryFromJson(json::parse(
        R"({"feature":"hint-14","pattern":[{"hint-14":"3"}],"hintReduceCompound":true})"));
    CHECK(q.hintReduceCompound);
}

TEST_CASE(query_from_json_defaults_hint_reduce_compound_to_false) {
    Query q = queryFromJson(json::parse(R"({"feature":"hint-14","pattern":[{"hint-14":"3"}]})"));
    CHECK(!q.hintReduceCompound);
}

TEST_CASE(query_from_json_reads_simultaneous_with_group_hint_reduce_compound_override) {
    Query q = queryFromJson(json::parse(R"({
        "feature":"kern",
        "pattern":[{"kern":"G"}],
        "simultaneousWith":[{
            "feature":"hint-14",
            "pattern":[{"hint-14":"3"}],
            "hintReduceCompound":true
        }]
    })"));
    REQUIRE(q.simultaneousWith.size() == 1u);
    REQUIRE(q.simultaneousWith[0].hintReduceCompound.has_value());
    CHECK(*q.simultaneousWith[0].hintReduceCompound);
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
    CHECK(!group.mintAllowIntervalComplementation.has_value());
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

TEST_CASE(query_from_json_reads_an_optional_id) {
    Query q = queryFromJson(json::parse(R"({"id":"my-id","feature":"kern","pattern":[{"kern":"G"}]})"));
    REQUIRE(q.id.has_value());
    CHECK_EQ(*q.id, std::string("my-id"));
}

TEST_CASE(query_from_json_defaults_id_to_nullopt) {
    Query q = queryFromJson(json::parse(R"({"feature":"kern","pattern":[{"kern":"G"}]})"));
    CHECK(!q.id.has_value());
}

TEST_CASE(query_from_json_rejects_non_string_id) {
    CHECK_THROWS(queryFromJson(json::parse(R"({"id":5,"feature":"kern","pattern":[{"kern":"G"}]})")));
}

TEST_CASE(query_array_from_json_parses_each_element) {
    auto queries = queryArrayFromJson(json::parse(R"([
        {"feature":"kern","pattern":[{"kern":"G"}]},
        {"id":"second","feature":"deg","pattern":[{"deg":"1"}]}
    ])"));
    REQUIRE(queries.size() == 2u);
    CHECK_EQ(queries[0].feature, std::string("kern"));
    CHECK(!queries[0].id.has_value());
    CHECK_EQ(queries[1].feature, std::string("deg"));
    REQUIRE(queries[1].id.has_value());
    CHECK_EQ(*queries[1].id, std::string("second"));
}

TEST_CASE(query_array_from_json_rejects_a_plain_object) {
    CHECK_THROWS(queryArrayFromJson(json::parse(R"({"feature":"kern","pattern":[{"kern":"G"}]})")));
}

TEST_CASE(query_array_from_json_rejects_an_empty_array) {
    CHECK_THROWS(queryArrayFromJson(json::parse("[]")));
}

TEST_CASE(query_array_from_json_error_message_references_the_failing_element_index) {
    try {
        queryArrayFromJson(json::parse(R"([
            {"feature":"kern","pattern":[{"kern":"G"}]},
            {"feature":"kern"}
        ])"));
        REQUIRE(false); // must throw
    } catch (const std::invalid_argument& e) {
        std::string msg = e.what();
        CHECK(msg.find("queries[1]") != std::string::npos);
    }
}

TEST_CASE(result_to_json_omits_query_id_when_not_set) {
    Result r;
    r.choraleId = "chor029";
    json j = resultToJson(r);
    CHECK(!j.contains("queryId"));
}

TEST_CASE(result_to_json_includes_query_id_when_set) {
    Result r;
    r.choraleId = "chor029";
    r.queryId = "my-id";
    json j = resultToJson(r);
    REQUIRE(j.contains("queryId"));
    CHECK_EQ(j.at("queryId").get<std::string>(), std::string("my-id"));
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

TEST_CASE(query_from_json_accepts_every_known_driving_feature) {
    for (const std::string& feature :
         {"kern", "deg", "mint", "fb", "metweight", "hint-12", "hint-13", "hint-14", "hint-23", "hint-24", "hint-34"}) {
        Query q = queryFromJson(json::parse(R"({"feature":")" + feature + R"(","pattern":[{"kern":"*"}]})"));
        CHECK_EQ(q.feature, feature);
    }
}

TEST_CASE(query_from_json_rejects_an_unknown_feature) {
    CHECK_THROWS(queryFromJson(json::parse(R"({"feature":"unknown","pattern":[{"kern":"*"}]})")));
}

TEST_CASE(query_from_json_rejects_duration_or_fermata_as_the_driving_feature) {
    CHECK_THROWS(queryFromJson(json::parse(R"({"feature":"duration","pattern":[{"kern":"*"}]})")));
    CHECK_THROWS(queryFromJson(json::parse(R"({"feature":"fermata","pattern":[{"kern":"*"}]})")));
}

TEST_CASE(query_from_json_rejects_hint_voice_relative_form_as_the_driving_feature) {
    CHECK_THROWS(queryFromJson(json::parse(R"({"feature":"hint-2","pattern":[{"kern":"*"}]})")));
}

TEST_CASE(query_from_json_rejects_a_non_real_hint_pair_as_the_driving_feature) {
    CHECK_THROWS(queryFromJson(json::parse(R"({"feature":"hint-11","pattern":[{"kern":"*"}]})")));
}

TEST_CASE(query_from_json_rejects_an_unresolvable_voices_selector) {
    CHECK_THROWS(queryFromJson(json::parse(R"({"feature":"kern","voices":"baritone","pattern":[{"kern":"*"}]})")));
    CHECK_THROWS(queryFromJson(json::parse(R"({"feature":"kern","voices":"5","pattern":[{"kern":"*"}]})")));
}

TEST_CASE(query_from_json_accepts_a_resolvable_voices_selector) {
    Query q = queryFromJson(json::parse(R"({"feature":"kern","voices":"soprano,bass","pattern":[{"kern":"*"}]})"));
    CHECK_EQ(q.voices, std::string("soprano,bass"));
}

TEST_CASE(query_from_json_rejects_an_unknown_pattern_key) {
    CHECK_THROWS(queryFromJson(json::parse(R"({"feature":"kern","pattern":[{"unknown":"4"}]})")));
    CHECK_THROWS(queryFromJson(json::parse(R"({"feature":"kern","pattern":[{"!unknown":"4"}]})")));
}

TEST_CASE(query_from_json_accepts_every_known_pattern_key) {
    for (const std::string& key : {"kern", "deg", "mint", "fb", "metweight", "duration", "fermata", "hint-12", "hint-1",
                                    "hint-2", "hint-3", "hint-4", "hint-*4", "hint-1*", "hint-**"}) {
        Query q = queryFromJson(json::parse(R"({"feature":"kern","pattern":[{")" + key + R"(":"*"}]})"));
        REQUIRE(q.pattern[0].count(key) == 1u);
    }
}

TEST_CASE(query_from_json_rejects_a_non_real_hint_pair_pattern_key) {
    CHECK_THROWS(queryFromJson(json::parse(R"({"feature":"kern","pattern":[{"hint-11":"3"}]})")));
}

TEST_CASE(query_from_json_rejects_a_hint_wildcard_key_with_a_non_voice_digit) {
    CHECK_THROWS(queryFromJson(json::parse(R"({"feature":"kern","pattern":[{"hint-*9":"3"}]})")));
}

TEST_CASE(query_from_json_accepts_valid_deg_values) {
    Query q = queryFromJson(json::parse(R"({"feature":"deg","pattern":[{"deg":["1","4+","6--","r"]}]})"));
    CHECK_EQ(q.pattern[0]["deg"], (std::vector<std::string>{"1", "4+", "6--", "r"}));
}

TEST_CASE(query_from_json_rejects_invalid_deg_values) {
    CHECK_THROWS(queryFromJson(json::parse(R"({"feature":"deg","pattern":[{"deg":"8"}]})")));
    CHECK_THROWS(queryFromJson(json::parse(R"({"feature":"deg","pattern":[{"deg":"nope"}]})")));
}

TEST_CASE(query_from_json_rejects_invalid_fermata_values) {
    CHECK_THROWS(queryFromJson(json::parse(R"({"feature":"kern","pattern":[{"fermata":"yes"}]})")));
}

TEST_CASE(query_from_json_accepts_fermata_bool_value) {
    Query q = queryFromJson(json::parse(R"({"feature":"kern","pattern":[{"fermata":true}]})"));
    CHECK_EQ(q.pattern[0]["fermata"], std::vector<std::string>{"true"});
}

TEST_CASE(query_from_json_accepts_every_way_to_write_a_metweight_value) {
    for (const std::string& v : {"s", "hs", "w", "u", "strong", "half-strong", "weak", "unclassified", "1", "2", "3", "4"}) {
        Query q = queryFromJson(json::parse(R"({"feature":"metweight","pattern":[{"metweight":")" + v + R"("}]})"));
        CHECK_EQ(q.pattern[0]["metweight"], (std::vector<std::string>{v}));
    }
}

TEST_CASE(query_from_json_rejects_an_invalid_metweight_value) {
    CHECK_THROWS(queryFromJson(json::parse(R"({"feature":"metweight","pattern":[{"metweight":"invalid"}]})")));
}

TEST_CASE(query_from_json_accepts_the_mint_first_note_bracket_literal) {
    Query q = queryFromJson(json::parse(R"({"feature":"mint","pattern":[{"mint":"[gg]"}]})"));
    CHECK_EQ(q.pattern[0]["mint"], (std::vector<std::string>{"[gg]"}));
}

TEST_CASE(query_from_json_rejects_an_invalid_mint_value) {
    CHECK_THROWS(queryFromJson(json::parse(R"({"feature":"mint","pattern":[{"mint":"nope123$"}]})")));
}

TEST_CASE(query_from_json_rejects_an_invalid_fb_value) {
    CHECK_THROWS(queryFromJson(json::parse(R"({"feature":"fb","pattern":[{"fb":"six"}]})")));
    CHECK_THROWS(queryFromJson(json::parse(R"({"feature":"fb","pattern":[{"fb":""}]})")));
}

TEST_CASE(query_from_json_rejects_an_invalid_hint_value) {
    CHECK_THROWS(queryFromJson(json::parse(R"({"feature":"hint-14","pattern":[{"hint-14":"six"}]})")));
}

TEST_CASE(query_from_json_rejects_an_invalid_duration_value) {
    CHECK_THROWS(queryFromJson(json::parse(R"({"feature":"kern","pattern":[{"duration":"invalid"}]})")));
}

TEST_CASE(query_from_json_accepts_wildcard_for_any_key) {
    Query q = queryFromJson(json::parse(
        R"({"feature":"kern","pattern":[{"kern":"*","deg":"*","mint":"*","fb":"*","hint-14":"*","metweight":"*","duration":"*","fermata":"*"}]})"));
    CHECK_EQ(q.pattern[0]["deg"], (std::vector<std::string>{"*"}));
}

TEST_CASE(query_from_json_rejects_an_empty_pattern_value) {
    CHECK_THROWS(queryFromJson(json::parse(R"({"feature":"kern","pattern":[{"deg":""}]})")));
}

TEST_CASE(query_from_json_applies_all_the_same_validation_inside_simultaneous_with) {
    CHECK_THROWS(queryFromJson(json::parse(R"({
        "feature":"kern",
        "pattern":[{"kern":"*"}],
        "simultaneousWith":[{"feature":"baritone","pattern":[{"kern":"*"}]}]
    })")));
    CHECK_THROWS(queryFromJson(json::parse(R"({
        "feature":"kern",
        "pattern":[{"kern":"*"}],
        "simultaneousWith":[{"feature":"kern","voices":"baritone","pattern":[{"kern":"*"}]}]
    })")));
    CHECK_THROWS(queryFromJson(json::parse(R"({
        "feature":"kern",
        "pattern":[{"kern":"*"}],
        "simultaneousWith":[{"feature":"kern","pattern":[{"durration":"4"}]}]
    })")));
    CHECK_THROWS(queryFromJson(json::parse(R"({
        "feature":"kern",
        "pattern":[{"kern":"*"}],
        "simultaneousWith":[{"feature":"deg","pattern":[{"deg":"8"}]}]
    })")));
}

TEST_CASE(query_from_json_rejects_an_unknown_top_level_field) {
    CHECK_THROWS(queryFromJson(json::parse(
        R"({"feature":"kern","pattern":[{"kern":"G"}],"unknown":true})")));
}

TEST_CASE(query_from_json_rejects_an_unknown_simultaneous_with_field) {
    CHECK_THROWS(queryFromJson(json::parse(R"({
        "feature":"kern",
        "pattern":[{"kern":"G"}],
        "simultaneousWith":[{"feature":"kern","pattern":[{"fermata":true}],"limit":5}]
    })")));
}

TEST_CASE(query_from_json_rejects_non_string_voices) {
    CHECK_THROWS(queryFromJson(json::parse(R"({"feature":"kern","voices":4,"pattern":[{"kern":"G"}]})")));
}

TEST_CASE(query_from_json_rejects_non_boolean_mint_start_at_previous_token) {
    CHECK_THROWS(queryFromJson(json::parse(
        R"({"feature":"mint","pattern":[{"mint":"-M2"}],"mintStartAtPreviousToken":"true"})")));
}

TEST_CASE(query_from_json_rejects_non_boolean_fb_compare_exact_chord) {
    CHECK_THROWS(queryFromJson(json::parse(
        R"({"feature":"fb","pattern":[{"fb":"6 3"}],"fbCompareExactChord":1})")));
}

TEST_CASE(query_from_json_rejects_non_boolean_kern_ignore_octave) {
    CHECK_THROWS(queryFromJson(json::parse(
        R"({"feature":"kern","pattern":[{"kern":"G"}],"kernIgnoreOctave":"yes"})")));
}

TEST_CASE(query_from_json_rejects_non_boolean_hint_reduce_compound) {
    CHECK_THROWS(queryFromJson(json::parse(
        R"({"feature":"hint-14","pattern":[{"hint-14":"3"}],"hintReduceCompound":null})")));
}

TEST_CASE(query_from_json_rejects_a_compound_mint_allow_interval_complementation_value) {
    // Only simple intervals (1-8) have a complement inside the octave.
    CHECK_THROWS(queryFromJson(json::parse(
        R"({"feature":"mint","pattern":[{"mint":"-5"}],"mintAllowIntervalComplementation":["10"]})")));
    CHECK_THROWS(queryFromJson(json::parse(
        R"({"feature":"mint","pattern":[{"mint":"-5"}],"mintAllowIntervalComplementation":9})")));
}

TEST_CASE(query_from_json_rejects_a_non_numeric_mint_allow_interval_complementation_value) {
    CHECK_THROWS(queryFromJson(json::parse(
        R"({"feature":"mint","pattern":[{"mint":"-5"}],"mintAllowIntervalComplementation":["P5"]})")));
    CHECK_THROWS(queryFromJson(json::parse(
        R"({"feature":"mint","pattern":[{"mint":"-5"}],"mintAllowIntervalComplementation":[true]})")));
}

TEST_CASE(query_from_json_rejects_an_empty_mint_allow_interval_complementation_array) {
    CHECK_THROWS(queryFromJson(json::parse(
        R"({"feature":"mint","pattern":[{"mint":"-5"}],"mintAllowIntervalComplementation":[]})")));
}

TEST_CASE(query_from_json_rejects_negative_limit) {
    CHECK_THROWS(queryFromJson(json::parse(R"({"feature":"kern","pattern":[{"kern":"G"}],"limit":-1})")));
}

TEST_CASE(query_from_json_rejects_non_integer_limit) {
    CHECK_THROWS(queryFromJson(json::parse(R"({"feature":"kern","pattern":[{"kern":"G"}],"limit":"5"})")));
    CHECK_THROWS(queryFromJson(json::parse(R"({"feature":"kern","pattern":[{"kern":"G"}],"limit":5.5})")));
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

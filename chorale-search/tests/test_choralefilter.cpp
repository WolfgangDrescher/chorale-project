#include "test_framework.hpp"

#include "ChoraleInfo.hpp"
#include "ChoraleFilter.hpp"
#include "JsonIO.hpp"

using choralesearch::normalizeModeName;
using choralesearch::ChoraleInfo;
using choralesearch::decodeHtmlEntities;
using choralesearch::modeFromKeyDesignation;
using choralesearch::integerFromChoraleId;
using choralesearch::lowercaseLatin;
using choralesearch::ChoraleFilter;
using choralesearch::choraleFilterFromJson;
using choralesearch::choraleFilterToJson;
using nlohmann::json;

namespace {

ChoraleFilter filterOf(const char* text) {
    return choraleFilterFromJson(json::parse(text), "Query JSON");
}

ChoraleInfo someChorale() {
    ChoraleInfo info;
    info.id = "chor029";
    info.titles = {"Freu dich sehr, o meine Seele", "Rejoice greatly, O my soul"};
    info.meters = {"4/4"};
    info.measureCount = 13;
    info.phraseCount = 6;
    info.key = "G";
    info.mode = "major";
    return info;
}

} // namespace

TEST_CASE(chorale_number_is_the_number_at_the_end_of_the_id) {
    CHECK_EQ(*integerFromChoraleId("chor029"), std::size_t{29});
    CHECK_EQ(*integerFromChoraleId("chor001"), std::size_t{1});
    CHECK_EQ(*integerFromChoraleId("chor371"), std::size_t{371});
    CHECK(!integerFromChoraleId("score").has_value());
    CHECK(!integerFromChoraleId("").has_value());
}

TEST_CASE(canonical_mode_folds_ionian_into_major_and_aeolian_into_minor) {
    CHECK_EQ(*normalizeModeName("major"), std::string("major"));
    CHECK_EQ(*normalizeModeName("ionian"), std::string("major"));
    CHECK_EQ(*normalizeModeName("minor"), std::string("minor"));
    CHECK_EQ(*normalizeModeName("Aeolian"), std::string("minor"));
    CHECK_EQ(*normalizeModeName("PHRYGIAN"), std::string("phrygian"));
    CHECK(!normalizeModeName("blues").has_value());
}

TEST_CASE(lowercase_utf8_lowers_ascii_and_the_umlauts) {
    CHECK_EQ(lowercaseLatin("Herz"), std::string("herz"));
    CHECK_EQ(lowercaseLatin("\xC3\x9C" "BER"), std::string("\xC3\xBC" "ber")); // Ü -> ü
    CHECK_EQ(lowercaseLatin("\xC3\x84" "\xC3\x96"), std::string("\xC3\xA4\xC3\xB6")); // Ä Ö -> ä ö
    CHECK_EQ(lowercaseLatin("Gro\xC3\x9F"), std::string("gro\xC3\x9F")); // ß stays
}

TEST_CASE(html_entities_of_a_title_are_decoded) {
    CHECK_EQ(decodeHtmlEntities("An Wasserfl&uuml;ssen Babylon"), std::string("An Wasserfl\xC3\xBC" "ssen Babylon"));
    CHECK_EQ(decodeHtmlEntities("&Auml;&ouml;&szlig;&amp;"), std::string("\xC3\x84\xC3\xB6\xC3\x9F&"));
    CHECK_EQ(decodeHtmlEntities("&#252; &#xFC;"), std::string("\xC3\xBC \xC3\xBC"));
    CHECK_EQ(decodeHtmlEntities("Tom & Jerry &unknown; &#zz;"), std::string("Tom & Jerry &unknown; &#zz;"));
    CHECK_EQ(decodeHtmlEntities("plain"), std::string("plain"));
}

TEST_CASE(a_key_designation_states_its_mode_by_a_label_or_else_by_the_case_of_the_key) {
    CHECK_EQ(modeFromKeyDesignation("e", ""), std::string("minor"));
    CHECK_EQ(modeFromKeyDesignation("E", ""), std::string("major"));
    CHECK_EQ(modeFromKeyDesignation("f", "dor"), std::string("dorian"));
    CHECK_EQ(modeFromKeyDesignation("e", "phr"), std::string("phrygian"));
    CHECK_EQ(modeFromKeyDesignation("G", "mix"), std::string("mixolydian"));
    CHECK_EQ(modeFromKeyDesignation("g", "aeo"), std::string("minor"));
    CHECK_EQ(modeFromKeyDesignation("C", "ion"), std::string("major"));
    CHECK_EQ(modeFromKeyDesignation("a", "xyz"), std::string("minor")); // an unknown label is none
}

TEST_CASE(filter_is_empty_by_default_and_lets_everything_in) {
    ChoraleFilter filter;
    CHECK(filter.empty());
    CHECK(!filter.needsChoraleInfo());
    CHECK(filter.matchesId("chor029"));
    CHECK(filter.matchesId("score"));
    CHECK(filter.matches(someChorale()));
}

TEST_CASE(chorale_ids_take_numbers_strings_with_or_without_zeros_and_the_chor_prefix) {
    for (const char* ids : {"[29]", "[\"29\"]", "[\"029\"]", "[\"chor029\"]", "[\"CHOR029\"]", "29", "\"029\""}) {
        ChoraleFilter filter = filterOf((std::string("{\"ids\":") + ids + "}").c_str());
        CHECK(filter.matchesId("chor029"));
        CHECK(!filter.matchesId("chor028"));
        CHECK(!filter.matchesId("chor290"));
    }
}

TEST_CASE(chorale_ids_take_ranges_and_mixed_lists) {
    ChoraleFilter filter = filterOf(R"({"ids":[1, "005", "chor010", "20-30", "chor100-chor102"]})");
    for (const char* id : {"chor001", "chor005", "chor010", "chor020", "chor025", "chor030", "chor100", "chor102"}) {
        CHECK(filter.matchesId(id));
    }
    for (const char* id : {"chor002", "chor011", "chor019", "chor031", "chor099", "chor103"}) {
        CHECK(!filter.matchesId(id));
    }
}

TEST_CASE(exclude_chorale_ids_leave_chorales_out_of_the_ones_asked_for) {
    ChoraleFilter filter = filterOf(R"({"ids":"1-10","excludeIds":[5, "7-8"]})");
    CHECK(filter.matchesId("chor004"));
    CHECK(!filter.matchesId("chor005"));
    CHECK(filter.matchesId("chor006"));
    CHECK(!filter.matchesId("chor007"));
    CHECK(!filter.matchesId("chor008"));
    CHECK(filter.matchesId("chor009"));
    CHECK(!filter.matchesId("chor011"));

    ChoraleFilter onlyExcluding = filterOf(R"({"excludeIds":"chor005"})");
    CHECK(!onlyExcluding.matchesId("chor005"));
    CHECK(onlyExcluding.matchesId("chor006"));
}

TEST_CASE(an_id_without_a_number_is_never_named_by_one) {
    CHECK(!filterOf(R"({"ids":[1]})").matchesId("score"));
    CHECK(filterOf(R"({"excludeIds":[1]})").matchesId("score"));
}

TEST_CASE(chorale_ids_that_cannot_be_read_are_rejected) {
    for (const char* ids : {"1.5", "-3", "\"-3\"", "\"abc\"", "\"\"", "\"5-\"", "\"-5\"", "\"20-5\"", "\"1-2-3\"", "[]",
                            "true", "null", "\"chor\"", "[[1]]", "1234567890123"}) {
        CHECK_THROWS(filterOf((std::string("{\"ids\":") + ids + "}").c_str()));
        CHECK_THROWS(filterOf((std::string("{\"excludeIds\":") + ids + "}").c_str()));
    }
}

TEST_CASE(a_title_is_searched_in_every_title_without_regard_to_case) {
    ChoraleInfo info = someChorale();
    CHECK(filterOf(R"({"title":"sehr"})").matches(info));
    CHECK(filterOf(R"({"title":"FREU DICH"})").matches(info));
    CHECK(filterOf(R"({"title":"rejoice"})").matches(info)); // the English title counts too
    CHECK(!filterOf(R"({"title":"Herz"})").matches(info));
    CHECK(filterOf(R"({"title":["Herz", "Seele"]})").matches(info)); // any of them

    info.titles = {"An Wasserfl\xC3\xBC" "ssen Babylon"};
    CHECK(filterOf("{\"title\":\"WASSERFL\xC3\x9C" "SSEN\"}").matches(info));
}

TEST_CASE(an_entry_that_cannot_be_read_is_named_in_the_error) {
    auto messageOf = [](const char* text) {
        try {
            filterOf(text);
        } catch (const std::invalid_argument& e) {
            return std::string(e.what());
        }
        return std::string();
    };
    CHECK(messageOf(R"({"title":[1,"Herz"]})").find("'choraleFilter.title'") != std::string::npos);
    CHECK(messageOf(R"({"title":[1,"Herz"]})").find("1 is not a non-empty string") != std::string::npos);
    CHECK(messageOf(R"({"meter":["4/4","four"]})").find("'four' is not a meter") != std::string::npos);
    CHECK(messageOf(R"({"startKey":["G","H"]})").find("'H' is not a key") != std::string::npos);
    CHECK(messageOf(R"({"startMode":["minor","blues"]})").find("'blues' is not a mode") != std::string::npos);
    CHECK(messageOf(R"({"ids":[1,"abc"]})").find("'abc' is not a chorale id") != std::string::npos);
}

TEST_CASE(a_single_entry_is_the_same_as_a_list_of_one) {
    CHECK(filterOf(R"({"title":"Herz"})") == filterOf(R"({"title":["Herz"]})"));
    CHECK(filterOf(R"({"startMode":"ionian"})") == filterOf(R"({"startMode":["major"]})"));
    CHECK(filterOf(R"({"ids":"5-20"})") == filterOf(R"({"ids":["5-20"]})"));
}

TEST_CASE(a_space_in_an_id_or_a_text_is_not_removed) {
    CHECK_THROWS(filterOf(R"({"ids":" chor029"})"));
    CHECK_THROWS(filterOf(R"({"ids":"5 - 20"})"));
    CHECK_THROWS(filterOf(R"({"startKey":"G "})"));
    CHECK_THROWS(filterOf(R"({"startMode":" minor"})"));

    // A space at the edge of a text is part of what is looked for: "Herz" is found inside of "Scherz",
    // " Herz" only where a word begins with it.
    ChoraleInfo info = someChorale();
    info.titles = {"Scherz"};
    CHECK(filterOf(R"({"title":"Herz"})").matches(info));
    CHECK(!filterOf(R"({"title":" Herz"})").matches(info));
    info.titles = {"Mein Herz"};
    CHECK(filterOf(R"({"title":"Herz"})").matches(info));
    CHECK(filterOf(R"({"title":" Herz"})").matches(info));
}

TEST_CASE(a_meter_matches_when_the_chorale_is_in_it_at_some_point) {
    ChoraleInfo info = someChorale();
    info.meters = {"4/4", "3/4"};
    CHECK(filterOf(R"({"meter":"3/4"})").matches(info));
    CHECK(filterOf(R"({"meter":["2/2", "4/4"]})").matches(info));
    CHECK(!filterOf(R"({"meter":["2/2", "6/8"]})").matches(info));
}

TEST_CASE(counts_take_a_number_or_a_range) {
    ChoraleInfo info = someChorale(); // 13 measures, 6 phrases
    CHECK(filterOf(R"({"measureCount":13})").matches(info));
    CHECK(!filterOf(R"({"measureCount":12})").matches(info));
    CHECK(filterOf(R"({"measureCount":{"min":8,"max":16}})").matches(info));
    CHECK(filterOf(R"({"measureCount":{"min":13}})").matches(info));
    CHECK(filterOf(R"({"measureCount":{"max":13}})").matches(info));
    CHECK(!filterOf(R"({"measureCount":{"min":14}})").matches(info));
    CHECK(!filterOf(R"({"measureCount":{"max":12}})").matches(info));
    CHECK(filterOf(R"({"phraseCount":{"min":4}})").matches(info));
    CHECK(!filterOf(R"({"phraseCount":{"min":7}})").matches(info));
    CHECK(filterOf(R"({"phraseCount":6})").matches(info));
}

TEST_CASE(key_and_mode_are_those_the_chorale_starts_in) {
    ChoraleInfo info = someChorale(); // G major
    CHECK(filterOf(R"({"startKey":"G"})").matches(info));
    CHECK(filterOf(R"({"startKey":["e", "G"]})").matches(info));
    CHECK(!filterOf(R"({"startKey":"g"})").matches(info)); // g is the minor key
    CHECK(filterOf(R"({"startMode":"major"})").matches(info));
    CHECK(filterOf(R"({"startMode":"ionian"})").matches(info));
    CHECK(filterOf(R"({"startMode":["minor", "major"]})").matches(info));
    CHECK(!filterOf(R"({"startMode":["minor", "phrygian"]})").matches(info));

    info.mode = "phrygian";
    CHECK(filterOf(R"({"startMode":"Phrygian"})").matches(info));
    CHECK(!filterOf(R"({"startMode":"major"})").matches(info));

    info.key.reset();
    info.mode.reset();
    CHECK(!filterOf(R"({"startKey":"G"})").matches(info));
    CHECK(!filterOf(R"({"startMode":"major"})").matches(info));
}

TEST_CASE(every_filter_that_is_set_has_to_hold) {
    ChoraleInfo info = someChorale();
    CHECK(filterOf(R"({"ids":"20-30","startMode":"major","meter":"4/4","measureCount":{"min":10}})").matches(info));
    CHECK(!filterOf(R"({"ids":"20-30","startMode":"minor","meter":"4/4","measureCount":{"min":10}})").matches(info));
    CHECK(!filterOf(R"({"ids":"1-10","startMode":"major"})").matches(info));
}

TEST_CASE(filter_rejects_what_it_does_not_know_or_cannot_read) {
    CHECK_THROWS(choraleFilterFromJson(json::parse("[]"), "Query JSON"));
    CHECK_THROWS(filterOf(R"({"choraleId":[1]})")); // a typo is not silently ignored
    CHECK_THROWS(filterOf(R"({"activeMode":"minor"})"));
    CHECK_THROWS(filterOf(R"({"title":""})"));
    CHECK_THROWS(filterOf(R"({"title":[]})"));
    CHECK_THROWS(filterOf(R"({"title":5})"));
    CHECK_THROWS(filterOf(R"({"meter":"four four"})"));
    CHECK_THROWS(filterOf(R"({"meter":"4"})"));
    CHECK_THROWS(filterOf(R"({"measureCount":"8"})"));
    CHECK_THROWS(filterOf(R"({"measureCount":-1})"));
    CHECK_THROWS(filterOf(R"({"measureCount":{}})"));
    CHECK_THROWS(filterOf(R"({"measureCount":{"min":9,"max":8}})"));
    CHECK_THROWS(filterOf(R"({"measureCount":{"least":8}})"));
    CHECK_THROWS(filterOf(R"({"measureCount":{"min":"8"}})"));
    CHECK_THROWS(filterOf(R"({"phraseCount":{"min":1.5}})"));
    CHECK_THROWS(filterOf(R"({"startKey":"H"})"));
    CHECK_THROWS(filterOf(R"({"startKey":"e:dor"})"));
    CHECK_THROWS(filterOf(R"({"startMode":"blues"})"));
    CHECK_THROWS(filterOf(R"({"startMode":[]})"));
}

TEST_CASE(filter_error_names_where_it_was_found) {
    try {
        choraleFilterFromJson(json::parse(R"({"measureCount":{"min":9,"max":8}})"), "'queries[2]'");
        CHECK(false);
    } catch (const std::invalid_argument& e) {
        const std::string message = e.what();
        CHECK(message.find("choraleFilter.measureCount") != std::string::npos);
        CHECK(message.find("queries[2]") != std::string::npos);
    }
}

TEST_CASE(filter_survives_a_trip_through_json) {
    ChoraleFilter filter = filterOf(R"({
        "ids": [1, "chor005-chor020"], "excludeIds": "9", "title": ["Herz"],
        "meter": ["4/4", "3/4"], "measureCount": {"min": 8, "max": 16}, "phraseCount": 6,
        "startKey": ["e"], "startMode": ["ionian", "Phrygian"]
    })");
    ChoraleFilter again = choraleFilterFromJson(choraleFilterToJson(filter), "Query JSON");
    CHECK(filter == again);
    CHECK_EQ(filter.startMode, (std::vector<std::string>{"major", "phrygian"}));
    CHECK(choraleFilterToJson(ChoraleFilter{}).empty());
}

TEST_MAIN()

#include "test_framework.hpp"

#include <filesystem>
#include <iostream>

#include "CorpusSearch.hpp"
#include "Query.hpp"

using choralesearch::AttributeMap;
using choralesearch::CorpusSearch;
using choralesearch::Query;
using choralesearch::SimultaneousGroup;

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

TEST_CASE(deg_3_2_1_or_3_2_3_duration) {
    Query q;
    q.feature = "deg";
    q.pattern = {
        AttributeMap{{"deg", {"3"}}, {"duration", {"4"}}},
        AttributeMap{{"deg", {"2"}}, {"duration", {"4"}}},
        AttributeMap{{"deg", {"1", "3"}}, {"duration", {"4"}}},
    };
    q.voices = "all";

    CorpusSearch search(FIXTURE_CHORALE("chor029"));
    auto results = search.run(q);

    REQUIRE(results.size() == 2u);

    CHECK_RESULT(results[0], "chor029", 4, "5", "7");
    CHECK_RESULT(results[1], "chor029", 4, "45", "47");
}

TEST_CASE(deg_3_2_1_or_3_2_3_duration_with_wildcard) {
    Query q;
    q.feature = "deg";
    q.pattern = {
        AttributeMap{{"deg", {"3"}}, {"duration", {"4"}}},
        AttributeMap{{"deg", {"2"}}, {"duration", {"4"}}},
        AttributeMap{{"deg", {"1", "3"}}, {"duration", {"*"}}},
    };
    q.voices = "all";

    CorpusSearch search(FIXTURE_CHORALE("chor029"));
    auto results = search.run(q);

    REQUIRE(results.size() == 6u);

    CHECK_RESULT(results[0], "chor029", 2, "0", "2");
    CHECK_RESULT(results[1], "chor029", 2, "8", "10");
    CHECK_RESULT(results[2], "chor029", 4, "5", "7");
    CHECK_RESULT(results[3], "chor029", 4, "12", "14");
    CHECK_RESULT(results[4], "chor029", 4, "20", "22");
    CHECK_RESULT(results[5], "chor029", 4, "45", "47");
}

TEST_CASE(deg_3_2_1_or_3_2_3_duration_with_wildcard_and_fermata) {
    Query q;
    q.feature = "deg";
    q.pattern = {
        AttributeMap{{"deg", {"3"}}, {"duration", {"4"}}},
        AttributeMap{{"deg", {"2"}}, {"duration", {"4"}}},
        AttributeMap{{"deg", {"1", "3"}}, {"duration", {"*"}}, {"fermata", {"true"}}},
    };
    q.voices = "all";

    CorpusSearch search(FIXTURE_CHORALE("chor029"));
    auto results = search.run(q);

    REQUIRE(results.size() == 3u);

    CHECK_RESULT(results[0], "chor029", 4, "5", "7");
    CHECK_RESULT(results[1], "chor029", 4, "12", "14");
    CHECK_RESULT(results[2], "chor029", 4, "20", "22");
}

TEST_CASE(mint_plusM2_plusM2_in_soprano) {
    Query q;
    q.feature = "mint";
    q.pattern = {
        AttributeMap{{"mint", {"+M2"}}},
        AttributeMap{{"mint", {"+M2"}}},
    };
    q.voices = "soprano";

    CorpusSearch search(FIXTURE_CHORALE("chor029"));
    auto results = search.run(q);

    REQUIRE(!results.empty());
    CHECK_RESULT(results[0], "chor029", 4, "1", "2");
}

TEST_CASE(mint_plusM2_plusM2_in_soprano_start_at_previous_token) {
    Query q;
    q.feature = "mint";
    q.pattern = {
        AttributeMap{{"mint", {"+M2"}}},
        AttributeMap{{"mint", {"+M2"}}},
    };
    q.voices = "soprano";
    q.mintStartAtPreviousToken = true;

    CorpusSearch search(FIXTURE_CHORALE("chor029"));
    auto results = search.run(q);

    REQUIRE(!results.empty());
    CHECK_RESULT(results[0], "chor029", 4, "0", "2");
}

TEST_CASE(mint_direction_only_plus2_plus2_in_soprano) {
    Query q;
    q.feature = "mint";
    q.pattern = {
        AttributeMap{{"mint", {"+2"}}},
        AttributeMap{{"mint", {"+2"}}},
        AttributeMap{{"mint", {"-2"}}},
    };
    q.voices = "soprano";
    q.mintStartAtPreviousToken = true;

    CorpusSearch search(FIXTURE_CHORALE("chor029"));
    auto results = search.run(q);

    REQUIRE(results.size() == 4u);
    CHECK_RESULT(results[0], "chor029", 4, "0", "3");
    CHECK_RESULT(results[1], "chor029", 4, "9", "12");
    CHECK_RESULT(results[2], "chor029", 4, "16", "19");
    CHECK_RESULT(results[3], "chor029", 4, "42", "44");
}

TEST_CASE(mint_direction_only_plus2_plus2_in_soprano_with_duration) {
    Query q;
    q.feature = "mint";
    q.pattern = {
        AttributeMap{{"mint", {"+2"}}, {"duration", {"4"}}},
        AttributeMap{{"mint", {"+2"}}},
        AttributeMap{{"mint", {"-2"}}},
    };
    q.voices = "soprano";
    q.mintStartAtPreviousToken = true;

    CorpusSearch search(FIXTURE_CHORALE("chor029"));
    auto results = search.run(q);


    REQUIRE(results.size() == 3u);
    CHECK_RESULT(results[0], "chor029", 4, "0", "3");
    CHECK_RESULT(results[1], "chor029", 4, "9", "12");
    CHECK_RESULT(results[2], "chor029", 4, "16", "19");
}

TEST_CASE(mint_bare_number_2_or_1_in_soprano) {
    Query q;
    q.feature = "mint";
    q.pattern = {
        AttributeMap{{"mint", {"2", "1"}}},
    };
    q.voices = "soprano";
    q.mintStartAtPreviousToken = true;

    CorpusSearch search(FIXTURE_CHORALE("chor029"));
    auto results = search.run(q);

    REQUIRE(results.size() == 40u);
}

TEST_CASE(fb_m6_3_in_bass) {
    // fb's 6th figure pinned to minor, 3rd left quality-free -- excludes e.g. line 31's
    // "M6 m3" (major 6th) while accepting every "m6 <any 3rd quality>" chord.
    Query q;
    q.feature = "fb";
    q.pattern = {
        AttributeMap{{"fb", {"m6 3"}}},
    };
    q.voices = "bass";

    CorpusSearch search(FIXTURE_CHORALE("chor029"));
    auto results = search.run(q);

    REQUIRE(results.size() == 13u);
    CHECK_RESULT(results[0], "chor029", 1, "1", "1");
    CHECK_RESULT(results[1], "chor029", 1, "5", "5");
    CHECK_RESULT(results[2], "chor029", 1, "9", "9");
    CHECK_RESULT(results[3], "chor029", 1, "12", "12");
    CHECK_RESULT(results[4], "chor029", 1, "17", "17");
    CHECK_RESULT(results[5], "chor029", 1, "19", "19");
    CHECK_RESULT(results[6], "chor029", 1, "28", "28");
    CHECK_RESULT(results[7], "chor029", 1, "35", "35");
    CHECK_RESULT(results[8], "chor029", 1, "36+1/2", "36+1/2");
    CHECK_RESULT(results[9], "chor029", 1, "39", "39");
    CHECK_RESULT(results[10], "chor029", 1, "43", "43");
    CHECK_RESULT(results[11], "chor029", 1, "44", "44");
    CHECK_RESULT(results[12], "chor029", 1, "47", "47");
}

TEST_CASE(fb_component_order_within_a_pattern_value_does_not_matter) {
    Query withOrder;
    withOrder.feature = "fb";
    withOrder.pattern = {AttributeMap{{"fb", {"m6 3"}}}};
    withOrder.voices = "bass";

    Query reversed;
    reversed.feature = "fb";
    reversed.pattern = {AttributeMap{{"fb", {"3 m6"}}}};
    reversed.voices = "bass";

    CorpusSearch search(FIXTURE_CHORALE("chor029"));
    auto a = search.run(withOrder);
    auto b = search.run(reversed);

    REQUIRE(a.size() == b.size());
    for (std::size_t i = 0; i < a.size(); ++i) {
        CHECK_EQ(a[i].startPosition, b[i].startPosition);
    }
}

TEST_CASE(mint_mixed_input) {
    Query q;
    q.feature = "mint";
    q.pattern = {
        AttributeMap{{"mint", {"m2"}}},
        AttributeMap{{"mint", {"6"}}},
        AttributeMap{{"mint", {"+2"}}},
        AttributeMap{{"mint", {"-m2"}}},
        AttributeMap{{"mint", {"-2"}}, {"duration", {"2"}}},
        AttributeMap{{"mint", {"P1"}}, {"fermata", {"true"}}},
    };
    q.voices = "soprano";
    q.mintStartAtPreviousToken = true;

    CorpusSearch search(FIXTURE_CHORALE("chor009"));
    auto results = search.run(q);

    REQUIRE(results.size() == 1u);
    CHECK_RESULT(results[0], "chor009", 4, "32", "39");
}

TEST_CASE(mint_start_at_previous_token_skips_a_rest_back_to_the_last_sounding_note) {
    // chor006's alto has f (whole note, position 13), a quarter rest (position 15), then
    // a (position 16). **mint reports the a as +M3 because the interval is measured across
    // the rest from the f -- so the shifted start must be the f at 13, not the rest at 15.
    Query q;
    q.feature = "mint";
    q.pattern = {
        AttributeMap{{"mint", {"+M3"}}},
    };
    q.voices = "alto";
    q.mintStartAtPreviousToken = true;

    CorpusSearch search(FIXTURE_CHORALE("chor006"));
    auto results = search.run(q);

    REQUIRE(results.size() == 1u);
    CHECK_RESULT(results[0], "chor006", 3, "13", "16");
}

TEST_CASE(fb_6_3_exact_chord_in_bass) {
    // Same "6 3" pattern as a permissive search would use, but with fbCompareExactChord
    // set: chords voiced with an extra component beyond the 6th and 3rd (e.g. an added
    // 9th) no longer qualify, narrowing 19 permissive matches down to these 16.
    Query q;
    q.feature = "fb";
    q.pattern = {
        AttributeMap{{"fb", {"6 3"}}},
    };
    q.voices = "bass";
    q.fbCompareExactChord = true;

    CorpusSearch search(FIXTURE_CHORALE("chor029"));
    auto results = search.run(q);

    REQUIRE(results.size() == 16u);
    CHECK_RESULT(results[0], "chor029", 1, "1", "1");
    CHECK_RESULT(results[1], "chor029", 1, "4+1/2", "4+1/2");
    CHECK_RESULT(results[2], "chor029", 1, "5", "5");
    CHECK_RESULT(results[3], "chor029", 1, "9", "9");
    CHECK_RESULT(results[4], "chor029", 1, "11+1/2", "11+1/2");
    CHECK_RESULT(results[5], "chor029", 1, "12", "12");
    CHECK_RESULT(results[6], "chor029", 1, "17", "17");
    CHECK_RESULT(results[7], "chor029", 1, "19", "19");
    CHECK_RESULT(results[8], "chor029", 1, "28", "28");
    CHECK_RESULT(results[9], "chor029", 1, "34+1/2", "34+1/2");
    CHECK_RESULT(results[10], "chor029", 1, "35", "35");
    CHECK_RESULT(results[11], "chor029", 1, "37", "37");
    CHECK_RESULT(results[12], "chor029", 1, "39", "39");
    CHECK_RESULT(results[13], "chor029", 1, "43", "43");
    CHECK_RESULT(results[14], "chor029", 1, "44", "44");
    CHECK_RESULT(results[15], "chor029", 1, "47", "47");
}

TEST_CASE(fb_4_2_chord_in_bass) {
    Query q;
    q.feature = "fb";
    q.pattern = {
        AttributeMap{{"fb", {"M2 A4"}}, {"deg", {"4"}}},
        AttributeMap{{"mint", {"-2"}}},
    };
    q.voices = "bass";
    q.fbCompareExactChord = false;

    CorpusSearch search(FIXTURE_CHORALE("chor029"));
    auto results = search.run(q);

    REQUIRE(results.size() == 2u);
    CHECK_RESULT(results[0], "chor029", 1, "24+1/2", "25");
    CHECK_RESULT(results[1], "chor029", 1, "46", "47");
}

TEST_CASE(fb_4_2_exact_chord_in_bass) {
    Query q;
    q.feature = "fb";
    q.pattern = {
        AttributeMap{{"fb", {"M2 A4"}}, {"deg", {"4"}}},
        AttributeMap{{"mint", {"-2"}}},
    };
    q.voices = "bass";
    q.fbCompareExactChord = true;

    CorpusSearch search(FIXTURE_CHORALE("chor029"));
    auto results = search.run(q);

    REQUIRE(results.empty());
}

TEST_CASE(fb_not_rule_of_the_octave_chords_on_deg_4_in_bass) {
    Query q;
    q.feature = "fb";
    q.voices = "1";
    q.pattern = {
        AttributeMap{{"deg", {"4"}}, {"!fb", {"2 4 6", "6 5 3"}}},
    };

    CorpusSearch search(FIXTURE_CHORALE("chor029"));
    auto results = search.run(q);

    REQUIRE(results.size() == 5u);
    CHECK_RESULT(results[0], "chor029", 1, "5+1/2", "5+1/2");
    CHECK_RESULT(results[1], "chor029", 1, "12+1/2", "12+1/2");
    CHECK_RESULT(results[2], "chor029", 1, "36", "36");
    CHECK_RESULT(results[3], "chor029", 1, "40", "40");
    CHECK_RESULT(results[4], "chor029", 1, "47+1/2", "47+1/2");
}

TEST_CASE(fb_excluding_soprano_mint_2_or_1_or_bracketed_first_note) {
    Query q;
    q.feature = "fb";
    q.voices = "4";
    q.pattern = {
        AttributeMap{{"!mint", {"2", "1", "[g]"}}},
    };

    CorpusSearch search(FIXTURE_CHORALE("chor029"));
    auto results = search.run(q);

    REQUIRE(results.size() == 6u);

    CHECK_RESULT(results[0], "chor029", 4, "8", "8");
    CHECK_RESULT(results[1], "chor029", 4, "16", "16");
    CHECK_RESULT(results[2], "chor029", 4, "24", "24");
    CHECK_RESULT(results[3], "chor029", 4, "33", "33");
    CHECK_RESULT(results[4], "chor029", 4, "40", "40");
    CHECK_RESULT(results[5], "chor029", 4, "42", "42");
}

TEST_CASE(deg_cadential_soprano_descent_simultaneous_with_bass_3_4_5_1) {
    Query q;
    q.feature = "deg";
    q.voices = "soprano";
    q.pattern = {
        AttributeMap{{"deg", {"3"}}, {"duration", {"4"}}},
        AttributeMap{{"deg", {"2"}}, {"duration", {"4"}}},
        AttributeMap{{"deg", {"1", "3"}}, {"duration", {"*"}}, {"fermata", {"true"}}},
    };
    
    SimultaneousGroup bass;
    bass.feature = "deg";
    bass.voices = "bass";
    bass.pattern = {
        AttributeMap{{"deg", {"3"}}, {"duration", {"8"}}},
        AttributeMap{{"deg", {"4"}}, {"duration", {"8"}}},
        AttributeMap{{"deg", {"5"}}, {"duration", {"4"}}},
        AttributeMap{{"deg", {"1"}}, {"duration", {"*"}}, {"fermata", {"true"}}},
    };
    q.simultaneousWith = {bass};
    q.simultaneousAlignment = "start-end";

    CorpusSearch search(FIXTURE_CHORALE("chor029"));
    auto results = search.run(q);

    REQUIRE(results.size() == 2u);
    CHECK_RESULT(results[0], "chor029", 4, "5", "7");
    CHECK_RESULT(results[1], "chor029", 4, "12", "14");
}

TEST_CASE(mint_half_note_then_descending_M2_onto_fermata_without_split_notes) {
    Query q;
    q.feature = "mint";
    q.pattern = {
        AttributeMap{{"duration", {"2"}}},
        AttributeMap{{"mint", {"-M2"}}, {"fermata", {"true"}}},
    };
    q.voices = "4";

    CorpusSearch search(FIXTURE_CHORALE("chor039"));
    auto results = search.run(q);

    CHECK(results.empty());

    Query q2;
    q2.feature = "mint";
    q2.pattern = {
        AttributeMap{{"duration", {"*"}}},
        AttributeMap{{"mint", {"-M2"}}, {"fermata", {"true"}}},
    };
    q2.voices = "4";
    
    CorpusSearch search2(FIXTURE_CHORALE("chor039"));
    auto results2 = search2.run(q2);
    
    CHECK_EQ(results2.size(), 4u);
}

TEST_CASE(mint_half_note_then_descending_M2_onto_fermata_with_split_notes) {
    Query q;
    q.feature = "mint";
    q.pattern = {
        AttributeMap{{"duration", {"2"}}},
        AttributeMap{{"mint", {"-M2"}}, {"fermata", {"true"}}},
    };
    q.voices = "4";
    q.durationAllowSplitNotes = true;

    CorpusSearch search(FIXTURE_CHORALE("chor039"));
    auto results = search.run(q);

    REQUIRE(results.size() == 4u);
    CHECK_RESULT(results[0], "chor039", 4, "12", "14");
    CHECK_RESULT(results[1], "chor039", 4, "20", "22");
    CHECK_RESULT(results[2], "chor039", 4, "36", "38");
    CHECK_RESULT(results[3], "chor039", 4, "44", "46");
}

TEST_CASE(mint_dominant_split_by_an_octave_onto_the_fermata_needs_both_options) {
    Query q;
    q.feature = "mint";
    q.pattern = {
        AttributeMap{{"duration", {"2"}}},
        AttributeMap{{"mint", {"-P5"}}, {"fermata", {"true"}}},
    };
    q.voices = "bass";

    CorpusSearch search(FIXTURE_CHORALE("chor009"));
    CHECK(search.run(q).empty()); // neither option

    q.mintAllowIntervalComplementation = {"*"};
    CHECK(search.run(q).empty()); // the half note still isn't written as one

    q.mintAllowIntervalComplementation.clear();
    q.durationAllowSplitNotes = true;
    CHECK(search.run(q).empty()); // the octave leap still isn't a re-attack

    // ...and the two things complementation does here need their own numbers: "8" (or "1")
    // unlocks the octave re-attack, "5" lets position 1's "-P5" match the written "+P4".
    q.mintAllowIntervalComplementation = {"5"};
    CHECK(search.run(q).empty()); // no octave re-attack, so no half note to begin with
    q.mintAllowIntervalComplementation = {"8"};
    CHECK(search.run(q).empty()); // the run closes now, but "-P5" doesn't reach the "+P4"
    q.mintAllowIntervalComplementation = {"5", "8"};
    CHECK_EQ(search.run(q).size(), 4u); // both, same as the "*" below
}

TEST_CASE(mint_dominant_split_by_an_octave_onto_the_fermata) {
    Query q;
    q.feature = "mint";
    q.pattern = {
        AttributeMap{{"duration", {"2"}}},
        AttributeMap{{"mint", {"-P5"}}, {"fermata", {"true"}}},
    };
    q.voices = "bass";
    q.durationAllowSplitNotes = true;
    q.mintAllowIntervalComplementation = {"*"};

    CorpusSearch search(FIXTURE_CHORALE("chor009"));
    auto results = search.run(q);

    REQUIRE(results.size() == 4u);
    CHECK_RESULT(results[0], "chor009", 1, "5", "7");
    CHECK_RESULT(results[1], "chor009", 1, "13", "15");
    CHECK_RESULT(results[2], "chor009", 1, "21", "23");
    CHECK_RESULT(results[3], "chor009", 1, "29", "31");
}

TEST_CASE(kern_three_quarters_onto_a_fermata_without_merged_notes) {
    Query q;
    q.feature = "kern";
    q.pattern = {
        AttributeMap{{"kern", {"a"}}, {"duration", {"4"}}},
        AttributeMap{{"kern", {"a"}}, {"duration", {"4"}}},
        AttributeMap{{"kern", {"g"}}, {"duration", {"4"}}, {"fermata", {"true"}}},
    };
    q.voices = "4";

    CorpusSearch search(FIXTURE_CHORALE("chor009"));
    CHECK(search.run(q).empty());
}

TEST_CASE(kern_three_quarters_onto_a_fermata_with_merged_notes) {
    // chor009's soprano writes both of these cadences as a half note a plus the fermata g,
    // where the lower voices spell the a out as two quarters.
    Query q;
    q.feature = "kern";
    q.pattern = {
        AttributeMap{{"kern", {"a"}}, {"duration", {"4"}}},
        AttributeMap{{"kern", {"a"}}, {"duration", {"4"}}},
        AttributeMap{{"kern", {"g"}}, {"duration", {"4"}}, {"fermata", {"true"}}},
    };
    q.voices = "4";
    q.durationAllowMergedNotes = true;

    CorpusSearch search(FIXTURE_CHORALE("chor009"));
    auto results = search.run(q);

    REQUIRE(results.size() == 2u);
    CHECK_RESULT(results[0], "chor009", 4, "13", "15");
    CHECK_RESULT(results[1], "chor009", 4, "45", "47");
}

TEST_CASE(mint_dotted_quarter_plus_repeated_eighth_merged_into_a_half_note) {
    // chor029's bass walks into the final fermata as a half note D plus the cadential fifth
    // down to GG. Asked for as a dotted quarter plus a repeated eighth, only merging finds
    // it -- and it has to be found identically whichever feature drives the walk, since the
    // pattern says nothing about the driving feature either way.
    for (const std::string& feature : {"mint", "deg", "kern"}) {
        Query q;
        q.feature = feature;
        q.pattern = {
            AttributeMap{{"mint", {"*"}}, {"duration", {"4."}}},
            AttributeMap{{"mint", {"P1"}}, {"duration", {"8"}}},
            AttributeMap{{"mint", {"-P5"}}, {"duration", {"*"}}, {"fermata", {"true"}}},
        };
        q.voices = "all";
        q.mintAllowIntervalComplementation = {"*"};

        CorpusSearch search(FIXTURE_CHORALE("chor029"));
        CHECK(search.run(q).empty()); // without the option the half note is just a half note

        q.durationAllowMergedNotes = true;
        auto results = search.run(q);

        REQUIRE(results.size() == 1u);
        CHECK_RESULT(results[0], "chor029", 1, "48", "50");
    }
}

TEST_CASE(kern_cadence_needing_a_split_run_and_a_merged_run_in_the_same_match) {
    // chor103's alto walks into the bar-4 fermata as: a a | g g | c, where the score writes
    // the two a's out as repeated quarters but the two g's as a single half note. One pattern
    // spelling both pairs out therefore needs each option for a different position of itself:
    //
    //   position 0  "duration": "2"   <- split:  the two written quarter a's, summed
    //   position 1  "mint": "-M2"     <- merged: the written half note g, shared with...
    //   position 2  "mint": "P1"      <- ...this position, the repetition the score merged away
    //   position 3  "mint": "-P5"     <- the fermata c, a fifth below, on its own onset
    //
    // Position 2 is also the case that has to stay independent of the driving feature: "kern"
    // drives here, so its "mint" is a cross-referenced key, and it is judged against the
    // unison the merged-away re-attack would have been rather than against the half note's
    // own token.
    Query q;
    q.feature = "kern";
    q.pattern = {
        AttributeMap{{"mint", {"*"}}, {"duration", {"2"}}},
        AttributeMap{{"mint", {"-M2"}}, {"duration", {"4"}}},
        AttributeMap{{"mint", {"P1"}}, {"duration", {"4"}}},
        AttributeMap{{"mint", {"-P5"}}, {"duration", {"*"}}, {"fermata", {"true"}}},
    };
    q.voices = "all";
    q.mintAllowIntervalComplementation = {"*"};

    CorpusSearch search(FIXTURE_CHORALE("chor103"));

    CHECK(search.run(q).empty()); // neither option

    q.durationAllowSplitNotes = true;
    CHECK(search.run(q).empty()); // the half note g still answers only one position

    q.durationAllowSplitNotes = false;
    q.durationAllowMergedNotes = true;
    CHECK(search.run(q).empty()); // the repeated a's still aren't a half note

    q.durationAllowSplitNotes = true;
    auto results = search.run(q);

    REQUIRE(results.size() == 1u);
    CHECK_RESULT(results[0], "chor103", 3, "11", "15");
}

TEST_CASE(kern_cadence_ornamented_by_a_neighbour_note) {
    // chor005's soprano walks into the bar-4 fermata as 8a 8g 4a 4g;: a half note's worth of
    // a decorated by a lower neighbour on the offbeat, then the step down to the fermata. The
    // plain skeleton of that -- a quarter, its repetition, the step down -- only reaches it
    // once the neighbour stops counting as a note of its own.
    Query q;
    q.feature = "kern";
    q.pattern = {
        AttributeMap{{"mint", {"*"}}, {"duration", {"4"}}},
        AttributeMap{{"mint", {"P1"}}, {"duration", {"4"}}},
        AttributeMap{{"mint", {"-M2"}}, {"duration", {"*"}}, {"fermata", {"true"}}},
    };
    q.voices = "all";

    CorpusSearch search(FIXTURE_CHORALE("chor005"));
    CHECK(search.run(q).empty());

    q.metweightSkipUnclassified = true;
    auto results = search.run(q);

    REQUIRE(results.size() == 1u);
    CHECK_RESULT(results[0], "chor005", 4, "13", "15");
}

TEST_CASE(kern_cadence_ornamented_by_a_neighbour_note_asked_for_as_one_half_note) {
    // The same bar of chor005, asked for as the half note it really is instead of as two
    // quarters. That needs both options at once: skipping the neighbour is what makes the two
    // written a's adjacent, and only then can a split run sum them into a half note. Bar 17's
    // soprano writes that half note out for real (position 65) and is found either way.
    Query q;
    q.feature = "kern";
    q.pattern = {
        AttributeMap{{"mint", {"*"}}, {"duration", {"2"}}},
        AttributeMap{{"mint", {"-M2"}}, {"duration", {"*"}}, {"fermata", {"true"}}},
    };
    q.voices = "all";
    q.durationAllowSplitNotes = true;

    CorpusSearch search(FIXTURE_CHORALE("chor005"));
    auto withoutSkipping = search.run(q);
    REQUIRE(withoutSkipping.size() == 1u);
    CHECK_RESULT(withoutSkipping[0], "chor005", 4, "65", "67");

    q.metweightSkipUnclassified = true;
    auto results = search.run(q);

    REQUIRE(results.size() == 2u);
    CHECK_RESULT(results[0], "chor005", 4, "13", "15");
    CHECK_RESULT(results[1], "chor005", 4, "65", "67");
}

TEST_CASE(kern_cadence_ornamented_by_an_anticipation) {
    // chor008's soprano reaches both of its bar-4 and bar-6 fermatas through an anticipation:
    // 4.b- 8a- 2a-; and 4.g 8f 2f;, where the offbeat eighth is already the fermata note,
    // taking its time from the note before it. Skipping it hands that time back, which turns
    // the dotted quarter into the half note the phrase is really made of.
    Query q;
    q.feature = "kern";
    q.pattern = {
        AttributeMap{{"mint", {"*"}}, {"duration", {"2"}}},
        AttributeMap{{"mint", {"-M2"}}, {"duration", {"*"}}, {"fermata", {"true"}}},
    };
    q.voices = "all";

    CorpusSearch search(FIXTURE_CHORALE("chor008"));
    CHECK(search.run(q).empty());

    q.metweightSkipUnclassified = true;
    auto results = search.run(q);

    REQUIRE(results.size() == 2u);
    CHECK_RESULT(results[0], "chor008", 4, "12", "14");
    CHECK_RESULT(results[1], "chor008", 4, "20", "22");
}

TEST_CASE(kern_cadence_ornamented_by_a_leap_away_and_back) {
    // A deg 4 5 1 bass cadence, written out twice in chor006: bar 8 plainly (position 27), and
    // bar 6 diminished into 4 2 5 1 (position 21), where an offbeat eighth drops a third away
    // from the 4 and leaps a fourth back up to the 5. The step from 4 to 5 the skeleton asks
    // for is only there once that eighth is out of the way -- the spine's own **mint reads the
    // fourth out of the ornament instead.
    Query q;
    q.feature = "kern";
    q.pattern = {
        AttributeMap{{"mint", {"*"}}, {"duration", {"4"}}},
        AttributeMap{{"mint", {"+2"}}, {"duration", {"4"}}},
        AttributeMap{{"mint", {"-P5"}}, {"duration", {"*"}}, {"fermata", {"true"}}},
    };
    q.voices = "all";
    q.mintAllowIntervalComplementation = {"5"};

    CorpusSearch search(FIXTURE_CHORALE("chor006"));
    auto withoutSkipping = search.run(q);
    REQUIRE(withoutSkipping.size() == 1u);
    CHECK_RESULT(withoutSkipping[0], "chor006", 1, "27", "29");

    q.metweightSkipUnclassified = true;
    auto results = search.run(q);

    REQUIRE(results.size() == 2u);
    CHECK_RESULT(results[0], "chor006", 1, "21", "23");
    CHECK_RESULT(results[1], "chor006", 1, "27", "29");
}

TEST_MAIN()

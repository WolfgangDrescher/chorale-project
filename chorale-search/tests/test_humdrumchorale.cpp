#include "test_framework.hpp"

#include <algorithm>
#include <sstream>

#include "HumdrumChorale.hpp"

using choralesearch::HumdrumChorale;

namespace {

// The line a spine's data token sits on `position` quarter notes into the piece. The tests name
// where in the music they mean and look the line up, because a line number moves whenever
// anything above it does -- a header record dropped from the fixtures moved all of them once.
int dataLineAtPosition(hum::HTp spineStart, hum::HumNum position) {
    for (hum::HTp token = spineStart; token; token = token->getNextToken()) {
        if (token->isData() && token->getDurationFromStart() == position) return token->getLineNumber();
    }
    return -1;
}

// The same for the barline a spine reaches at that position.
int barlineAtPosition(hum::HTp spineStart, hum::HumNum position) {
    for (hum::HTp token = spineStart; token; token = token->getNextToken()) {
        if (token->isBarline() && token->getDurationFromStart() == position) return token->getLineNumber();
    }
    return -1;
}

} // namespace

TEST_CASE(constructor_sets_path_and_id_from_the_filename_stem) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    CHECK_EQ(chorale.id(), std::string("chor029"));
    CHECK_EQ(chorale.path(), FIXTURE_CHORALE("chor029"));
}

TEST_CASE(constructor_throws_on_a_missing_file) {
    minitest::SilenceStderr silence;
    CHECK_THROWS(HumdrumChorale("/nonexistent/path/to/chorale.krn"));
}

TEST_CASE(available_features_lists_kern_and_the_derived_analysis_spines) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    auto features = chorale.availableFeatures();
    // std::map-backed storage, so this comes back key-sorted rather than in
    // file order -- "deg" < "fb" < "hint-12" < ... < "hint-34" < "kern" <
    // "metweight" < "mint".
    CHECK_EQ(features, (std::vector<std::string>{"deg", "fb", "hint-12", "hint-13", "hint-14", "hint-23", "hint-24",
                                                  "hint-34", "kern", "metweight", "mint"}));
}

TEST_CASE(has_feature_is_true_for_present_and_false_for_absent) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    CHECK(chorale.hasFeature("kern"));
    CHECK(chorale.hasFeature("deg"));
    CHECK(chorale.hasFeature("mint"));
    CHECK(chorale.hasFeature("fb"));
    CHECK(chorale.hasFeature("metweight"));
    CHECK(!chorale.hasFeature("hint"));
    CHECK(!chorale.hasFeature("nonexistent"));
}

TEST_CASE(spine_returns_null_for_an_unknown_feature) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    CHECK(chorale.spine("nonexistent", 1) == nullptr);
}

TEST_CASE(spine_returns_null_for_an_out_of_range_voice) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    CHECK(chorale.spine("kern", 0) == nullptr);
    CHECK(chorale.spine("kern", 5) == nullptr); // chor029 only has 4 **kern voices
}

TEST_CASE(spine_voice_order_matches_bass_tenor_alto_soprano_columns) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    // The downbeat of measure 1, the chorale's first full onset, with a different pitch in every
    // column: 4G(bass) 4B(tenor) 4d(alto) 4g(soprano).
    const int line = dataLineAtPosition(chorale.spine("kern", 1), 0);
    REQUIRE(line > 0);
    auto tokenTextAt = [&](std::size_t voice) {
        auto tok = chorale.tokenAtLine("kern", voice, line);
        REQUIRE(tok != nullptr);
        return std::string(*tok);
    };
    CHECK_EQ(tokenTextAt(1), std::string("4G"));
    CHECK_EQ(tokenTextAt(2), std::string("4B"));
    CHECK_EQ(tokenTextAt(3), std::string("4d"));
    CHECK_EQ(tokenTextAt(4), std::string("4g"));
}

TEST_CASE(token_at_line_returns_null_for_a_null_token) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    // The second half of beat 4 in measure 1, where the tenor moves and the bass does not: the
    // bass column is "." there, still sounding its previous note, so it has no onset and must
    // not be read as a match. The line is looked up through the tenor, the voice that moves.
    const int line = dataLineAtPosition(chorale.spine("kern", 2), hum::HumNum(7, 2));
    REQUIRE(line > 0);
    CHECK(chorale.tokenAtLine("kern", 1, line) == nullptr);
    // Same line, tenor column has a real (if tied) onset: "8F#J".
    auto tenorTok = chorale.tokenAtLine("kern", 2, line);
    REQUIRE(tenorTok != nullptr);
    CHECK_EQ(std::string(*tenorTok), std::string("8F#J"));
}

TEST_CASE(token_at_line_returns_null_for_a_non_data_line) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    // The barline opening measure 1 ("=1-") stands at the same position as the first onset and is
    // not a data line.
    const int line = barlineAtPosition(chorale.spine("kern", 1), 0);
    REQUIRE(line > 0);
    CHECK(chorale.tokenAtLine("kern", 1, line) == nullptr);
}

TEST_CASE(token_at_line_returns_null_for_a_missing_spine) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    CHECK(chorale.tokenAtLine("nonexistent", 1, 10) == nullptr);
    CHECK(chorale.tokenAtLine("kern", 5, 10) == nullptr);
}

TEST_CASE(token_at_line_returns_null_for_a_line_outside_the_file) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    CHECK(chorale.tokenAtLine("kern", 1, 0) == nullptr);
    CHECK(chorale.tokenAtLine("kern", 1, 1000000) == nullptr);
}

TEST_CASE(has_feature_is_true_for_all_six_hint_pair_spines) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    for (const std::string& pair : {"hint-12", "hint-13", "hint-14", "hint-23", "hint-24", "hint-34"}) {
        CHECK(chorale.hasFeature(pair));
    }
    CHECK(!chorale.hasFeature("hint"));
}

TEST_CASE(hint_pair_spine_has_exactly_one_entry_regardless_of_the_pair) {
    // Unlike kern/deg/mint (4 spines, one per voice), each hint-<pair> name has exactly
    // one spine in the whole file -- voice 1 resolves it, any other voice is out of range.
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    CHECK(chorale.spine("hint-14", 1) != nullptr);
    CHECK(chorale.spine("hint-14", 2) == nullptr);
    CHECK(chorale.spine("hint-14", 0) == nullptr);
}

TEST_CASE(voice_count_is_the_number_of_kern_spines) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    CHECK_EQ(chorale.voiceCount(), std::size_t{4});
}

TEST_CASE(phrase_endings_are_where_the_fermatas_close_a_phrase) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    std::vector<hum::HumNum> endings = chorale.phraseEndings();
    CHECK_EQ(endings.size(), std::size_t{6});
    CHECK(std::is_sorted(endings.begin(), endings.end()));
    // Every chorale closes on a fermata: the last ending is where the piece ends.
    CHECK(endings.back() > endings.front());
}

TEST_CASE(collect_info_reads_the_meters_the_key_and_the_measures_off_a_small_score) {
    std::istringstream score(
        "!!!OTL@@DE: Ein Titel mit Umlaut: &auml;\n"
        "!!!OTL@EN: A title\n"
        "**kern\t**kern\n"
        "*clefF4\t*clefG2\n"
        "*e:phr\t*e:phr\n"
        "*M4/4\t*M4/4\n"
        "4E\t4e\n"
        "=1\t=1\n"
        "4F\t4f\n"
        "=:|!\t=:|!\n"
        "*M3/4\t*M3/4\n"
        "*G:\t*G:\n"
        "=3a\t=3a\n"
        "4G\t4g\n"
        "==\t==\n"
        "*-\t*-\n");
    choralesearch::ChoraleInfo info = HumdrumChorale(score, "score", false).collectInfo();

    CHECK_EQ(info.id, std::string("score"));
    REQUIRE(info.titles.size() == 2u);
    CHECK_EQ(info.titles[0], std::string("Ein Titel mit Umlaut: \xC3\xA4"));
    CHECK_EQ(info.titles[1], std::string("A title"));
    CHECK_EQ(info.meters, (std::vector<std::string>{"4/4", "3/4"}));
    REQUIRE(info.key.has_value());
    CHECK_EQ(*info.key, std::string("e")); // the first key, not the one it changes to
    CHECK_EQ(*info.mode, std::string("phrygian"));
    CHECK_EQ(info.measureCount, std::size_t{3}); // the highest number a barline carries, "3a" included
    CHECK_EQ(info.duration, 3.0); // three quarter notes
}

TEST_CASE(collect_info_has_no_key_and_no_measures_where_the_score_states_none) {
    std::istringstream score("**kern\n4c\n4d\n*-\n");
    choralesearch::ChoraleInfo info = HumdrumChorale(score, "bare", false).collectInfo();
    CHECK(info.titles.empty());
    CHECK(info.meters.empty());
    CHECK(!info.key.has_value());
    CHECK(!info.mode.has_value());
    CHECK_EQ(info.measureCount, std::size_t{0});
    CHECK_EQ(info.duration, 2.0);
    CHECK_EQ(info.phraseCount, std::size_t{0});
}

TEST_CASE(collect_info_reads_what_the_score_states_about_itself) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor001"));
    choralesearch::ChoraleInfo info = chorale.collectInfo();
    CHECK_EQ(info.id, std::string("chor001"));
    REQUIRE(info.titles.size() == 2u);
    CHECK_EQ(info.titles[0], std::string("Aus meines Herzens Grunde"));
    CHECK_EQ(info.meters, (std::vector<std::string>{"3/4"}));
    CHECK_EQ(info.measureCount, std::size_t{21});
    CHECK_EQ(info.phraseCount, std::size_t{6});
    REQUIRE(info.key.has_value());
    CHECK_EQ(*info.key, std::string("G")); // it modulates to e minor later, but starts in G
    CHECK_EQ(*info.mode, std::string("major"));
}

TEST_CASE(collect_info_decodes_the_html_entities_of_a_title_and_reads_a_church_mode) {
    CHECK_EQ(HumdrumChorale(FIXTURE_CHORALE("chor005")).collectInfo().titles.front(),
             std::string("An Wasserfl\xC3\xBC" "ssen Babylon"));
    choralesearch::ChoraleInfo dorian = HumdrumChorale(FIXTURE_CHORALE("chor008")).collectInfo();
    CHECK_EQ(*dorian.key, std::string("f"));
    CHECK_EQ(*dorian.mode, std::string("dorian"));
    CHECK_EQ(dorian.phraseCount, std::size_t{9});
}

TEST_MAIN()

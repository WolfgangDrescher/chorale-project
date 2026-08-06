#include "test_framework.hpp"

#include "HumdrumChorale.hpp"

using choralesearch::findTokenAtLine;
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
    // The downbeat of bar 1, the chorale's first full onset, with a different pitch in every
    // column: 4G(bass) 4B(tenor) 4d(alto) 4g(soprano).
    const int line = dataLineAtPosition(chorale.spine("kern", 1), 0);
    REQUIRE(line > 0);
    auto tokenTextAt = [&](std::size_t voice) {
        auto tok = findTokenAtLine(chorale.spine("kern", voice), line);
        REQUIRE(tok != nullptr);
        return std::string(*tok);
    };
    CHECK_EQ(tokenTextAt(1), std::string("4G"));
    CHECK_EQ(tokenTextAt(2), std::string("4B"));
    CHECK_EQ(tokenTextAt(3), std::string("4d"));
    CHECK_EQ(tokenTextAt(4), std::string("4g"));
}

TEST_CASE(find_token_at_line_returns_null_for_a_null_token) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    // The second half of beat 4 in bar 1, where the tenor moves and the bass does not: the
    // bass column is "." there, still sounding its previous note, so it has no onset and must
    // not be read as a match. The line is looked up through the tenor, the voice that moves.
    const int line = dataLineAtPosition(chorale.spine("kern", 2), hum::HumNum(7, 2));
    REQUIRE(line > 0);
    CHECK(findTokenAtLine(chorale.spine("kern", 1), line) == nullptr);
    // Same line, tenor column has a real (if tied) onset: "8F#J".
    auto tenorTok = findTokenAtLine(chorale.spine("kern", 2), line);
    REQUIRE(tenorTok != nullptr);
    CHECK_EQ(std::string(*tenorTok), std::string("8F#J"));
}

TEST_CASE(find_token_at_line_returns_null_for_a_non_data_line) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    // The barline opening bar 1 ("=1-") stands at the same position as the first onset and is
    // not a data line.
    const int line = barlineAtPosition(chorale.spine("kern", 1), 0);
    REQUIRE(line > 0);
    CHECK(findTokenAtLine(chorale.spine("kern", 1), line) == nullptr);
}

TEST_CASE(find_token_at_line_returns_null_for_a_null_spine) {
    CHECK(findTokenAtLine(nullptr, 1) == nullptr);
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

TEST_MAIN()

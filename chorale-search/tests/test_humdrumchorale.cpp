#include "test_framework.hpp"

#include "HumdrumChorale.hpp"

using choralesearch::findTokenAtLine;
using choralesearch::HumdrumChorale;

TEST_CASE(constructor_sets_path_and_id_from_the_filename_stem) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    CHECK_EQ(chorale.id(), std::string("chor029"));
    CHECK_EQ(chorale.path(), FIXTURE_CHORALE("chor029"));
}

TEST_CASE(constructor_throws_on_a_missing_file) {
    CHECK_THROWS(HumdrumChorale("/nonexistent/path/to/chorale.krn"));
}

TEST_CASE(available_features_lists_kern_and_the_derived_analysis_spines) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    auto features = chorale.availableFeatures();
    // std::map-backed storage, so this comes back key-sorted rather than in
    // file order -- "deg" < "kern" < "mint".
    CHECK_EQ(features, (std::vector<std::string>{"deg", "kern", "mint"}));
}

TEST_CASE(has_feature_is_true_for_present_and_false_for_absent) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    CHECK(chorale.hasFeature("kern"));
    CHECK(chorale.hasFeature("deg"));
    CHECK(chorale.hasFeature("mint"));
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
    // Line 23 is the chorale's first full onset in every voice, with a
    // different pitch in each column: 4G(bass) 4B(tenor) 4d(alto) 4g(soprano).
    auto tokenTextAt = [&](std::size_t voice) {
        auto tok = findTokenAtLine(chorale.spine("kern", voice), 23);
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
    // Line 27's bass column is "." (still sounding the previous note) --
    // no onset there, so this must not be treated as a match.
    CHECK(findTokenAtLine(chorale.spine("kern", 1), 27) == nullptr);
    // Same line, tenor column has a real (if tied) onset: "8F#J".
    auto tenorTok = findTokenAtLine(chorale.spine("kern", 2), 27);
    REQUIRE(tenorTok != nullptr);
    CHECK_EQ(std::string(*tenorTok), std::string("8F#J"));
}

TEST_CASE(find_token_at_line_returns_null_for_a_non_data_line) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    // Line 22 is a barline ("=1-"), not a data line.
    CHECK(findTokenAtLine(chorale.spine("kern", 1), 22) == nullptr);
}

TEST_CASE(find_token_at_line_returns_null_for_a_null_spine) {
    CHECK(findTokenAtLine(nullptr, 23) == nullptr);
}

TEST_MAIN()

#include "test_framework.hpp"

#include <stdexcept>
#include <string>
#include <vector>

#include "ScoreImport.hpp"

using choralesearch::prepareScore;

namespace {

// The rows of a score as one text, a line feed after each.
std::string lines(const std::vector<std::string>& rows) {
    std::string text;
    for (const std::string& row : rows) text += row + "\n";
    return text;
}

} // namespace

TEST_CASE(prepare_accepts_a_fermata_in_every_voice) {
    CHECK_NOTHROW(prepareScore(lines({
        "**kern\t**kern\t**kern\t**kern",
        "*M4/4\t*M4/4\t*M4/4\t*M4/4",
        "=1\t=1\t=1\t=1",
        "4G\t4d\t4g\t4b",
        "4C\t4e\t4g\t4cc",
        "2G;\t2d;\t2g;\t2b;",
        "==\t==\t==\t==",
        "*-\t*-\t*-\t*-",
    })));
}

TEST_CASE(prepare_rejects_fermatas_missing_in_some_voices) {
    // A MusicXML export with the fermata on the upper layer of each staff only.
    CHECK_THROWS(prepareScore(lines({
        "**kern\t**kern\t**kern\t**kern",
        "*M4/4\t*M4/4\t*M4/4\t*M4/4",
        "=1\t=1\t=1\t=1",
        "4G\t4d\t4g\t4b",
        "4C\t4e\t4g\t4cc",
        "2G\t2d;\t2g\t2b;",
        "==\t==\t==\t==",
        "*-\t*-\t*-\t*-",
    })));
}

TEST_CASE(prepare_rejects_fermatas_that_never_sound_together) {
    CHECK_THROWS(prepareScore(lines({
        "**kern\t**kern\t**kern\t**kern",
        "*M4/4\t*M4/4\t*M4/4\t*M4/4",
        "=1\t=1\t=1\t=1",
        "4G;\t4d;\t4g\t4b",
        "4C\t4e\t4g;\t4cc;",
        "2G\t2d\t2g\t2b",
        "==\t==\t==\t==",
        "*-\t*-\t*-\t*-",
    })));
}

TEST_CASE(prepare_rejects_one_fermata_of_the_soprano_that_the_other_voices_miss) {
    // The first phrase ends in all voices, the second in the soprano only.
    CHECK_THROWS(prepareScore(lines({
        "**kern\t**kern\t**kern\t**kern",
        "*M4/4\t*M4/4\t*M4/4\t*M4/4",
        "=1\t=1\t=1\t=1",
        "2G;\t2d;\t2g;\t2b;",
        "4C\t4e\t4g\t4cc",
        "4G\t4d\t4g\t4b;",
        "==\t==\t==\t==",
        "*-\t*-\t*-\t*-",
    })));
}

TEST_CASE(prepare_rejects_a_soprano_without_fermata) {
    CHECK_THROWS(prepareScore(lines({
        "**kern\t**kern\t**kern\t**kern",
        "*M4/4\t*M4/4\t*M4/4\t*M4/4",
        "=1\t=1\t=1\t=1",
        "4G\t4d\t4g\t4b",
        "4C\t4e\t4g\t4cc",
        "2G;\t2d;\t2g;\t2b",
        "==\t==\t==\t==",
        "*-\t*-\t*-\t*-",
    })));
}

TEST_CASE(prepare_accepts_fermatas_that_overlap_without_starting_together) {
    // The bass holds its fermata while the upper voices move into theirs.
    CHECK_NOTHROW(prepareScore(lines({
        "**kern\t**kern\t**kern\t**kern",
        "*M4/4\t*M4/4\t*M4/4\t*M4/4",
        "=1\t=1\t=1\t=1",
        "2G;\t8dL\t4g\t4b",
        ".\t8cJ\t.\t.",
        ".\t4B;\t4g;\t4b;",
        "==\t==\t==\t==",
        "*-\t*-\t*-\t*-",
    })));
}

TEST_CASE(prepare_accepts_a_fermata_at_the_end_of_a_tie) {
    CHECK_NOTHROW(prepareScore(lines({
        "**kern\t**kern\t**kern\t**kern",
        "*M4/4\t*M4/4\t*M4/4\t*M4/4",
        "=1\t=1\t=1\t=1",
        "2G\t2d\t2g\t[2b",
        "=2\t=2\t=2\t=2",
        "2G;\t2d;\t2g;\t2b];",
        "==\t==\t==\t==",
        "*-\t*-\t*-\t*-",
    })));
}

TEST_CASE(prepare_accepts_a_fermata_after_the_tie_it_ends) {
    // The tenor ties over the barline and marks the fermata on the tie's end only.
    CHECK_NOTHROW(prepareScore(lines({
        "**kern\t**kern\t**kern\t**kern",
        "*M4/4\t*M4/4\t*M4/4\t*M4/4",
        "=1\t=1\t=1\t=1",
        "2G;\t[2d\t2g;\t2b;",
        "=2\t=2\t=2\t=2",
        "4G\t4d;]\t4g\t4b",
        "==\t==\t==\t==",
        "*-\t*-\t*-\t*-",
    })));
}

TEST_CASE(prepare_counts_the_measures_of_an_unnumbered_score_after_its_pickup) {
    try {
        prepareScore(lines({
            "**kern\t**kern\t**kern\t**kern",
            "*M4/4\t*M4/4\t*M4/4\t*M4/4",
            "4G\t4d\t4g\t4b",
            "=\t=\t=\t=",
            "2C\t2e\t2g\t2cc",
            "2G\t2d\t2g\t2b;",
            "==\t==\t==\t==",
            "*-\t*-\t*-\t*-",
        }));
        CHECK(false);
    } catch (const std::invalid_argument& e) {
        CHECK(std::string(e.what()).find("in measure 1 ") != std::string::npos);
    }
}

TEST_CASE(prepare_keeps_the_measure_numbers_a_score_has) {
    try {
        prepareScore(lines({
            "**kern\t**kern\t**kern\t**kern",
            "*M4/4\t*M4/4\t*M4/4\t*M4/4",
            "=5\t=5\t=5\t=5",
            "2C\t2e\t2g\t2cc",
            "2G\t2d\t2g\t2b;",
            "=6\t=6\t=6\t=6",
            "1C;\t1e;\t1g;\t1cc;",
            "==\t==\t==\t==",
            "*-\t*-\t*-\t*-",
        }));
        CHECK(false);
    } catch (const std::invalid_argument& e) {
        CHECK(std::string(e.what()).find("in measure 5 ") != std::string::npos);
    }
}

TEST_CASE(prepare_leaves_the_silent_voices_of_a_two_voice_setting_out) {
    // Split into bass and soprano, the inner voices only rest.
    CHECK_NOTHROW(prepareScore(lines({
        "**kern\t**kern",
        "*M4/4\t*M4/4",
        "=1\t=1",
        "4G\t4g",
        "4F#\t4a",
        "2D;\t2b;",
        "==\t==",
        "*-\t*-",
    })));
    CHECK_THROWS(prepareScore(lines({
        "**kern\t**kern",
        "*M4/4\t*M4/4",
        "=1\t=1",
        "4G\t4g",
        "4F#\t4a",
        "2D\t2b;",
        "==\t==",
        "*-\t*-",
    })));
}

TEST_CASE(prepare_names_a_compressed_musicxml_upload) {
    try {
        prepareScore(std::string("PK\x03\x04", 4) + "META-INF/container.xml");
        CHECK(false);
    } catch (const std::invalid_argument& e) {
        CHECK(std::string(e.what()).find("compressed MusicXML (.mxl)") != std::string::npos);
    }
}

TEST_MAIN()

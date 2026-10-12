#include "test_framework.hpp"

#include <string>
#include <vector>

#include "SplitScoreIntoVoices.hpp"
#include "humlib.h"

using choralesearch::splitScoreIntoVoices;

namespace {

// The rows of a score as one text, a line feed after each.
std::string lines(const std::vector<std::string>& rows) {
    std::string text;
    for (const std::string& row : rows) text += row + "\n";
    return text;
}

std::string split(const std::vector<std::string>& rows) {
    hum::HumdrumFile infile;
    REQUIRE(infile.readString(lines(rows)));
    return splitScoreIntoVoices(infile);
}

} // namespace

TEST_CASE(split_gives_a_lone_voice_per_staff_to_bass_and_soprano) {
    // Two staves with one voice each: the inner voices rest, nothing is doubled.
    const std::string result = split({
        "**kern\t**kern",
        "*M4/4\t*M4/4",
        "=1\t=1",
        "4G\t4g",
        "4F#\t4a",
        "2D\t2b",
        "==\t==",
        "*-\t*-",
    });
    CHECK_EQ(result, lines({
        "**kern\t**kern\t**kern\t**kern",
        "*M4/4\t*M4/4\t*M4/4\t*M4/4",
        "=1\t=1\t=1\t=1",
        "4G\t1r\t1r\t4g",
        "4F#\t.\t.\t4a",
        "2D\t.\t.\t2b",
        "==\t==\t==\t==",
        "*-\t*-\t*-\t*-",
    }));
}

TEST_CASE(split_ignores_a_layer_that_only_rests) {
    // MusicXML exports write the unused voice of a staff as a layer of rests.
    const std::string result = split({
        "**kern\t**kern",
        "*^\t*^",
        "*\t*\t*\t*",
        "=1\t=1\t=1\t=1",
        "4r\t4G\t4r\t4g",
        "4r\t4F#\t4r\t4a",
        "2r\t2D\t2r\t2b",
        "==\t==\t==\t==",
        "*-\t*-\t*-\t*-",
    });
    CHECK_EQ(result, lines({
        "**kern\t**kern\t**kern\t**kern",
        "*\t*\t*\t*",
        "=1\t=1\t=1\t=1",
        "4G\t1r\t1r\t4g",
        "4F#\t.\t.\t4a",
        "2D\t.\t.\t2b",
        "==\t==\t==\t==",
        "*-\t*-\t*-\t*-",
    }));
}

TEST_CASE(split_lets_a_held_note_outlast_a_rest_in_the_other_layer) {
    const std::string result = split({
        "**kern\t**kern",
        "*^\t*",
        "=1\t=1\t=1",
        "2G\t4r\t2g",
        ".\t4r\t.",
        "2D\t2r\t2b",
        "==\t==\t==",
        "*-\t*-\t*-",
    });
    CHECK_EQ(result, lines({
        "**kern\t**kern\t**kern\t**kern",
        "=1\t=1\t=1\t=1",
        "2G\t1r\t1r\t2g",
        ".\t.\t.\t.",
        "2D\t.\t.\t2b",
        "==\t==\t==\t==",
        "*-\t*-\t*-\t*-",
    }));
}

TEST_CASE(split_rests_for_the_length_of_each_bar_including_a_pickup) {
    const std::string result = split({
        "**kern\t**kern",
        "*M3/4\t*M3/4",
        "4D\t4a",
        "=1\t=1",
        "2G\t2b",
        "4F#\t4a",
        "=2\t=2",
        "2.G\t2.g",
        "==\t==",
        "*-\t*-",
    });
    CHECK_EQ(result, lines({
        "**kern\t**kern\t**kern\t**kern",
        "*M3/4\t*M3/4\t*M3/4\t*M3/4",
        "4D\t4r\t4r\t4a",
        "=1\t=1\t=1\t=1",
        "2G\t2.r\t2.r\t2b",
        "4F#\t.\t.\t4a",
        "=2\t=2\t=2\t=2",
        "2.G\t2.r\t2.r\t2.g",
        "==\t==\t==\t==",
        "*-\t*-\t*-\t*-",
    }));
}

TEST_CASE(split_still_splits_a_staff_with_two_voices_by_register) {
    // Chords on each staff: bass-tenor and alto-soprano, the way a piano reduction writes them.
    const std::string result = split({
        "**kern\t**kern",
        "=1\t=1",
        "4G 4D\t4b 4g",
        "4C\t4cc",
        "2F 4c\t2a 2f",
        "==\t==",
        "*-\t*-",
    });
    CHECK_EQ(result, lines({
        "**kern\t**kern\t**kern\t**kern",
        "=1\t=1\t=1\t=1",
        "4D\t4G\t4g\t4b",
        "4C\t4C\t4cc\t4cc",
        "2F\t4c\t2f\t2a",
        "==\t==\t==\t==",
        "*-\t*-\t*-\t*-",
    }));
}

TEST_CASE(split_rejects_a_score_that_is_not_two_staves) {
    hum::HumdrumFile infile;
    REQUIRE(infile.readString("**kern\n=1\n4c\n==\n*-\n"));
    CHECK_THROWS(splitScoreIntoVoices(infile));
}

TEST_MAIN()

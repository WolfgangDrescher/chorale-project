#include "test_framework.hpp"

#include <sstream>
#include <string>
#include <vector>

#include "LyricsSpines.hpp"

using choralesearch::applyLyrics;
using choralesearch::Verse;

namespace {

// The rows of a score as one text, a line feed after each.
std::string lines(const std::vector<std::string>& rows) {
    std::string text;
    for (const std::string& row : rows) text += row + "\n";
    return text;
}

std::string withLyrics(const std::vector<Verse>& verses, const std::vector<std::string>& rows) {
    hum::HumdrumFile infile;
    REQUIRE(infile.readString(lines(rows)));
    applyLyrics(infile, "test", verses);
    std::stringstream out;
    out << infile;
    return out.str();
}

} // namespace

TEST_CASE(syllables_go_onto_the_attacks_of_the_soprano) {
    const std::string result = withLyrics({{"Aus", "mei-", "-nes", std::nullopt}}, {
        "!!!OTL: Aus meines Herzens Grunde",
        "**kern\t**kern",
        "*M3/4\t*M3/4",
        "!\t!LO:TX:t=x",
        "4G\t4g",
        "=1\t=1",
        "4G\t2g",
        "4E\t.",
        "4F#\t4dd",
        "=2\t=2",
        "2G;\t2b;",
        "==\t==",
        "*-\t*-",
    });
    CHECK_EQ(result, lines({
        "!!!OTL: Aus meines Herzens Grunde",
        "**kern\t**kern\t**text",
        "*M3/4\t*M3/4\t*",
        "!\t!LO:TX:t=x\t!",
        "4G\t4g\tAus",
        "=1\t=1\t=1",
        "4G\t2g\tmei-",
        "4E\t.\t.",
        "4F#\t4dd\t-nes",
        "=2\t=2\t=2",
        "2G;\t2b;\t.",
        "==\t==\t==",
        "*-\t*-\t*-",
    }));
}

TEST_CASE(rests_and_tied_notes_take_no_syllable) {
    const std::string result = withLyrics({{"Gott", "sei"}, {"Herr", std::nullopt}}, {
        "**kern\t**kern",
        "4G\t[4g",
        "4A\t4g]",
        "4B\t4r",
        "4c\t4a",
        "*-\t*-",
    });
    CHECK_EQ(result, lines({
        "**kern\t**kern\t**text\t**text",
        "4G\t[4g\tGott\tHerr",
        "4A\t4g]\t.\t.",
        "4B\t4r\t.\t.",
        "4c\t4a\tsei\t.",
        "*-\t*-\t*-\t*-",
    }));
}

TEST_CASE(lyrics_that_do_not_fit_the_soprano_are_left_out) {
    const std::vector<std::string> rows = {
        "**kern\t**kern",
        "4G\t4g",
        "4c\t4a",
        "*-\t*-",
    };
    CHECK_EQ(withLyrics({{"Aus", "mei-", "-nes"}}, rows), lines(rows));
}

TEST_CASE(stanzas_are_numbered_above_the_first_note) {
    const std::string result = withLyrics({{"Gott", "sei."}, {"Wir", "Dank."}}, {
        "**kern\t**kern",
        "4G\t4g",
        "4c\t4a",
        "*-\t*-",
    });
    CHECK_EQ(result, lines({
        "**kern\t**kern\t**text\t**text",
        "*\t*\t*v:1.\t*v:2.",
        "4G\t4g\tGott\tWir",
        "4c\t4a\tsei.\tDank.",
        "*-\t*-\t*-\t*-",
    }));
}

TEST_CASE(a_repeat_makes_two_lines_one_stanza) {
    const std::string result = withLyrics({{"Gott", "sei."}, {"Herr", "dir."}}, {
        "**kern\t**kern",
        "4G\t4g",
        "4c\t4a",
        "=:|!\t=:|!",
        "*-\t*-",
    });
    CHECK(result.find("*v:") == std::string::npos);
}

TEST_CASE(lines_that_continue_a_sentence_are_no_stanzas) {
    const std::string result = withLyrics({{"Gott", "sei,"}, {"Wir", "Dank."}}, {
        "**kern\t**kern",
        "4G\t4g",
        "4c\t4a",
        "*-\t*-",
    });
    CHECK(result.find("*v:") == std::string::npos);
}

TEST_CASE(a_syllable_that_continues_a_word_starts_with_a_hyphen) {
    // The syllable before tells: "ren" goes on after "Eh-", "-durch" starts a word after "ren".
    const std::string result = withLyrics({{"Eh-", "ren", "-durch", std::nullopt, "mich", "-hast"}}, {
        "**kern\t**kern",
        "4G\t4g",
        "4A\t4a",
        "4B\t4b",
        "4c\t4cc",
        "4d\t4dd",
        "4e\t4ee",
        "*-\t*-",
    });
    CHECK_EQ(result, lines({
        "**kern\t**kern\t**text",
        "4G\t4g\tEh-",
        "4A\t4a\t-ren",
        "4B\t4b\tdurch",
        "4c\t4cc\t.",
        "4d\t4dd\tmich",
        "4e\t4ee\thast",
        "*-\t*-\t*-",
    }));
}

TEST_MAIN()

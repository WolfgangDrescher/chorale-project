#include "test_framework.hpp"

#include <algorithm>
#include <sstream>
#include <string>
#include <vector>

#include "ChoraleLyrics.hpp"
#include "HumdrumChorale.hpp"

using choralesearch::ChoraleLyrics;
using choralesearch::HumdrumChorale;
using choralesearch::LyricsSyllable;
using choralesearch::LyricsVerse;
using choralesearch::LyricsWord;
using choralesearch::SyllablePosition;
using choralesearch::matchesLyricsGlob;
using choralesearch::normalizeLyricsText;

// The fixtures with lyrics: chor001 (two verses, the second only under the repeat), chor005 (four
// verses) and chor029 (two verses); chor006 has none.
namespace {

std::vector<std::string> wordTexts(const LyricsVerse& verse) {
    std::vector<std::string> texts;
    for (const LyricsWord& word : verse.words()) texts.push_back(word.normalizedText);
    return texts;
}

// The syllables of the verse in the order of the score, out of their words.
std::vector<const LyricsSyllable*> syllablesOf(const LyricsVerse& verse) {
    std::vector<const LyricsSyllable*> syllables;
    for (const LyricsWord& word : verse.words()) {
        for (const LyricsSyllable& syllable : word.syllables) syllables.push_back(&syllable);
    }
    return syllables;
}

std::vector<std::string> syllableTexts(const LyricsVerse& verse) {
    std::vector<std::string> texts;
    for (const LyricsSyllable* syllable : syllablesOf(verse)) texts.push_back(syllable->normalizedText);
    return texts;
}

std::vector<int> linesOf(const LyricsVerse& verse, std::size_t syllable) {
    return syllablesOf(verse).at(syllable)->lines;
}

std::string sungAt(const LyricsVerse& verse, int lineNumber) {
    const choralesearch::LyricsSyllable* syllable = verse.syllableAtLine(lineNumber);
    return syllable ? syllable->normalizedText : "";
}

// Four voices with two verses: a syllable over a tied note, a rest and a fermata. The first row of notes is line 7.
HumdrumChorale tiedAndRestingChorale() {
    std::istringstream contents(
        "**kern\t**kern\t**kern\t**kern\t**text\t**text\n"
        "*clefF4\t*clefGv2\t*clefG2\t*clefG2\t*\t*\n"
        "*k[]\t*k[]\t*k[]\t*k[]\t*\t*\n"
        "*C:\t*C:\t*C:\t*C:\t*\t*\n"
        "*M4/4\t*M4/4\t*M4/4\t*M4/4\t*\t*\n"
        "=1\t=1\t=1\t=1\t=1\t=1\n"
        "4C\t4e\t4g\t[2cc\tHer-\tGott,\n" // 7: the cc is tied over to line 9
        "4D\t4f\t4a\t.\t.\t.\n"           // 8: the other voices move under the held cc
        "4E\t4g\t4g\t4cc]\t.\t.\n"        // 9: the tied continuation: no attack
        "4F\t4a\t4a\t4dd\t-zens\t.\n"     // 10
        "=2\t=2\t=2\t=2\t=2\t=2\n"
        "4G\t4g\t4b\t4r\t.\t.\n"          // 12
        "4A\t4a\t4cc\t4ee\t.\tJe-\n"      // 13: no text in verse 1 after the rest
        "4B;\t4b;\t4dd;\t4ff;\tGrun-\t-su\n" // 14
        "4c\t4e\t4g\t4gg\t.\t.\n"         // 15: after the fermata
        "==\t==\t==\t==\t==\t==\n"
        "*-\t*-\t*-\t*-\t*-\t*-\n");
    return HumdrumChorale(contents, "tied");
}

} // namespace

TEST_CASE(a_chorale_with_text_spines_has_a_verse_for_each_and_the_lyrics_features) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor001"));
    CHECK_EQ(chorale.verseCount(), std::size_t{2});
    CHECK(chorale.hasFeature("word"));
    CHECK(chorale.hasFeature("syl"));
    CHECK(chorale.hasFeature("kern"));
    CHECK(chorale.lyrics().verse(1) != nullptr);
    CHECK(chorale.lyrics().verse(2) != nullptr);
    CHECK(chorale.lyrics().verse(0) == nullptr);
    CHECK(chorale.lyrics().verse(3) == nullptr);
    const auto features = chorale.availableFeatures();
    CHECK(std::find(features.begin(), features.end(), "word") != features.end());
    CHECK(std::find(features.begin(), features.end(), "syl") != features.end());
    HumdrumChorale four(FIXTURE_CHORALE("chor005"));
    CHECK_EQ(four.verseCount(), std::size_t{4});
    CHECK(four.lyrics().verse(4) != nullptr);
    CHECK(four.lyrics().verse(5) == nullptr);
}

TEST_CASE(a_chorale_without_text_spines_has_no_lyrics) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor006"));
    CHECK_EQ(chorale.verseCount(), std::size_t{0});
    CHECK(!chorale.hasFeature("word"));
    CHECK(!chorale.hasFeature("syl"));
    CHECK(chorale.lyrics().verse(1) == nullptr);
    const auto features = chorale.availableFeatures();
    CHECK(std::find(features.begin(), features.end(), "word") == features.end());
}

TEST_CASE(a_syllable_continues_the_word_only_after_a_hyphen_at_the_end_of_the_syllable_before) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor001"));
    const LyricsVerse& verse = *chorale.lyrics().verse(1);
    // "Eh-", "ren", "-durch": "ren" has no hyphen at its end, so "durch" is a word of its own, hyphen or not.
    REQUIRE(verse.words().size() == 28u);
    REQUIRE(syllablesOf(verse).size() == 39u);
    CHECK_EQ(verse.words()[20].normalizedText, std::string("ehren"));
    CHECK_EQ(verse.words()[20].writtenText, std::string("Ehren"));
    REQUIRE(verse.words()[20].syllables.size() == 2u);
    CHECK_EQ(verse.words()[20].syllables[0].normalizedText, std::string("eh"));
    CHECK_EQ(verse.words()[20].syllables[1].normalizedText, std::string("ren"));
    CHECK_EQ(verse.words()[20].firstLine(), 86);
    CHECK_EQ(verse.words()[20].lastLine(), 93);
    CHECK_EQ(verse.words()[21].normalizedText, std::string("durch"));
    REQUIRE(verse.words()[21].syllables.size() == 1u);
    CHECK_EQ(verse.words()[21].firstLine(), 94);
    CHECK_EQ(syllableTexts(verse)[24], std::string("eh"));
    CHECK_EQ(syllableTexts(verse)[25], std::string("ren"));
    CHECK_EQ(syllableTexts(verse)[26], std::string("durch"));
    CHECK_EQ(verse.syllableAtLine(93)->word->normalizedText, std::string("ehren"));
    // The start of the verse, and a word of four syllables.
    CHECK_EQ(wordTexts(verse)[0], std::string("aus"));
    CHECK_EQ(wordTexts(verse)[1], std::string("meines"));
    CHECK_EQ(wordTexts(verse)[2], std::string("herzens"));
    CHECK_EQ(wordTexts(verse)[26], std::string("eingebornen"));
    CHECK_EQ(verse.words()[26].syllables.size(), std::size_t{4});
    CHECK_EQ(wordTexts(verse)[27], std::string("sohn"));
    CHECK_EQ(verse.words()[27].writtenText, std::string("Sohn."));
    // The second verse has text under the repeat only.
    const LyricsVerse& second = *chorale.lyrics().verse(2);
    CHECK_EQ(wordTexts(second), (std::vector<std::string>{"in", "dieser", "morgenstunde", "darzu", "mein", "lebelang"}));
}

TEST_CASE(the_position_of_a_syllable_in_its_word) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor001"));
    const LyricsVerse& verse = *chorale.lyrics().verse(1);
    // "Aus", "mei-", "-nes", and "ein-", "-ge-", "-bor-", "-nen".
    const auto syllables = syllablesOf(verse);
    CHECK(syllables[0]->position == SyllablePosition::Single);
    CHECK(syllables[1]->position == SyllablePosition::Initial);
    CHECK(syllables[2]->position == SyllablePosition::Terminal);
    CHECK(syllables[34]->position == SyllablePosition::Initial);
    CHECK(syllables[35]->position == SyllablePosition::Medial);
    CHECK(syllables[36]->position == SyllablePosition::Medial);
    CHECK(syllables[37]->position == SyllablePosition::Terminal);
    CHECK(syllables[38]->position == SyllablePosition::Single);
    CHECK_EQ(std::string(choralesearch::syllablePositionName(SyllablePosition::Single)), std::string("single"));
    CHECK_EQ(std::string(choralesearch::syllablePositionName(SyllablePosition::Initial)), std::string("initial"));
    CHECK_EQ(std::string(choralesearch::syllablePositionName(SyllablePosition::Medial)), std::string("medial"));
    CHECK_EQ(std::string(choralesearch::syllablePositionName(SyllablePosition::Terminal)), std::string("terminal"));
}

TEST_CASE(a_syllable_is_sung_over_the_notes_without_a_syllable_that_follow_it) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor001"));
    const LyricsVerse& verse = *chorale.lyrics().verse(1);
    CHECK_EQ(linesOf(verse, 0), (std::vector<int>{20}));
    // "Her-" on the dotted b and the a after it, "Grun-" over three notes up to the "-de" with the fermata.
    CHECK_EQ(linesOf(verse, 3), (std::vector<int>{27, 29}));
    CHECK_EQ(linesOf(verse, 5), (std::vector<int>{32, 35, 36}));
    CHECK_EQ(syllablesOf(verse)[5]->start, hum::HumNum(7));
    CHECK_EQ(syllablesOf(verse)[5]->end, hum::HumNum(10));
    CHECK_EQ(linesOf(verse, 6), (std::vector<int>{38}));
    CHECK_EQ(syllablesOf(verse)[6]->end, hum::HumNum(12));
    // The word "Grunde" spans both.
    CHECK_EQ(verse.words()[3].firstLine(), 32);
    CHECK_EQ(verse.words()[3].lastLine(), 38);
    CHECK_EQ(verse.words()[3].start(), hum::HumNum(7));
    CHECK_EQ(verse.words()[3].end(), hum::HumNum(12));
    // The second verse has the same notes under other syllables.
    const LyricsVerse& second = *chorale.lyrics().verse(2);
    CHECK_EQ(syllableTexts(second)[5], std::string("stun"));
    CHECK_EQ(linesOf(second, 5), (std::vector<int>{32, 35, 36}));
}

TEST_CASE(a_melisma_stops_at_a_fermata_and_the_verse_ends_where_its_text_does) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor001"));
    // Verse 2 ends with "-lang," on the fermata of the repeat; the notes after it have no text in that verse.
    const LyricsVerse& second = *chorale.lyrics().verse(2);
    REQUIRE(syllablesOf(second).size() == 13u);
    CHECK_EQ(syllableTexts(second)[12], std::string("lang"));
    CHECK_EQ(linesOf(second, 12), (std::vector<int>{50}));
    CHECK(!second.syllableAtLine(54));
    CHECK(!second.syllableAtLine(128));
    // Verse 1 goes on: "o" on the note after the fermata, "Gott" over two.
    const LyricsVerse& first = *chorale.lyrics().verse(1);
    CHECK_EQ(linesOf(first, 12), (std::vector<int>{50}));
    CHECK_EQ(linesOf(first, 13), (std::vector<int>{54}));
    CHECK_EQ(linesOf(first, 14), (std::vector<int>{56, 58}));
}

TEST_CASE(a_tied_note_is_sung_once_and_a_rest_stops_a_melisma) {
    const HumdrumChorale chorale = tiedAndRestingChorale();
    REQUIRE(chorale.verseCount() == 2u);
    const LyricsVerse& first = *chorale.lyrics().verse(1);
    CHECK_EQ(syllableTexts(first), (std::vector<std::string>{"her", "zens", "grun"}));
    CHECK_EQ(wordTexts(first), (std::vector<std::string>{"herzens", "grun"}));
    // The tied continuation is no attack, and the syllable is held over it while the other voices move.
    CHECK_EQ(linesOf(first, 0), (std::vector<int>{7}));
    CHECK_EQ(sungAt(first, 8), std::string("her"));
    CHECK_EQ(sungAt(first, 9), std::string("her"));
    CHECK_EQ(syllablesOf(first)[0]->end, hum::HumNum(3));
    // The ee after the rest has no text in verse 1 and is silent; so is the gg after the fermata.
    CHECK(!first.syllableAtLine(12));
    CHECK(!first.syllableAtLine(13));
    CHECK_EQ(linesOf(first, 2), (std::vector<int>{14}));
    CHECK(!first.syllableAtLine(15));
    // "Gott," over the tied cc and the dd: two attacks; the rest ends it.
    const LyricsVerse& second = *chorale.lyrics().verse(2);
    CHECK_EQ(wordTexts(second), (std::vector<std::string>{"gott", "jesu"}));
    CHECK_EQ(linesOf(second, 0), (std::vector<int>{7, 10}));
    CHECK_EQ(second.words()[0].end(), hum::HumNum(4));
    CHECK_EQ(linesOf(second, 1), (std::vector<int>{13}));
    CHECK_EQ(linesOf(second, 2), (std::vector<int>{14}));
}

TEST_CASE(the_syllable_sounding_on_a_line_lasts_until_its_last_note_is_over) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor001"));
    const LyricsVerse& verse = *chorale.lyrics().verse(1);
    CHECK_EQ(sungAt(verse, 20), std::string("aus"));
    CHECK_EQ(verse.syllableAtLine(20)->word->normalizedText, std::string("aus"));
    // "Grun-" while the alto and the tenor move under its dotted g, and on its own later notes.
    for (int line : {32, 33, 34, 35, 36}) {
        CHECK_EQ(sungAt(verse, line), std::string("grun"));
        CHECK_EQ(verse.syllableAtLine(line)->word->normalizedText, std::string("grunde"));
    }
    CHECK_EQ(sungAt(verse, 38), std::string("de"));
    // "ich" on a half note while the bass moves.
    CHECK_EQ(sungAt(verse, 41), std::string("ich"));
    CHECK_EQ(sungAt(verse, 42), std::string("ich"));
    CHECK_EQ(sungAt(verse, 43), std::string("dir"));
    // A barline, and a line the score does not have.
    CHECK(!verse.syllableAtLine(37));
    CHECK(!verse.syllableAtLine(0));
    CHECK(!verse.syllableAtLine(10000));
}

TEST_CASE(a_word_or_a_syllable_is_normalized_to_small_letters_without_the_punctuation_around_it) {
    CHECK_EQ(normalizeLyricsText("Gott,"), std::string("gott"));
    CHECK_EQ(normalizeLyricsText("Gott!"), std::string("gott"));
    CHECK_EQ(normalizeLyricsText("\"Herr\""), std::string("herr"));
    CHECK_EQ(normalizeLyricsText("(Jesu)"), std::string("jesu"));
    CHECK_EQ(normalizeLyricsText("sag'"), std::string("sag"));
    CHECK_EQ(normalizeLyricsText("dein’"), std::string("dein"));
    CHECK_EQ(normalizeLyricsText("Thron,"), std::string("thron"));
    CHECK_EQ(normalizeLyricsText("Preis;"), std::string("preis"));
    CHECK_EQ(normalizeLyricsText("Gott:"), std::string("gott"));
    CHECK_EQ(normalizeLyricsText("Gott?"), std::string("gott"));
    CHECK_EQ(normalizeLyricsText("Sohn."), std::string("sohn"));
    CHECK_EQ(normalizeLyricsText("-zens"), std::string("zens"));
    // The umlauts and the sharp s.
    CHECK_EQ(normalizeLyricsText("Ängsten"), std::string("ängsten"));
    CHECK_EQ(normalizeLyricsText("Öl"), std::string("öl"));
    CHECK_EQ(normalizeLyricsText("Über"), std::string("über"));
    CHECK_EQ(normalizeLyricsText("große"), std::string("grosse"));
    CHECK_EQ(normalizeLyricsText("daß"), std::string("dass"));
    CHECK_EQ(normalizeLyricsText("für"), std::string("für"));
    // Two words elided onto one note stay one syllable, with a plain space between them.
    CHECK_EQ(normalizeLyricsText("-i – son."), std::string("i – son"));
    CHECK_EQ(normalizeLyricsText("1. An"), std::string("1. an"));
    // Already normalized text stays as it is, a star too, and nothing is left of punctuation alone.
    CHECK_EQ(normalizeLyricsText("jesu"), std::string("jesu"));
    CHECK_EQ(normalizeLyricsText("Her*"), std::string("her*"));
    CHECK_EQ(normalizeLyricsText("*zen"), std::string("*zen"));
    CHECK_EQ(normalizeLyricsText("..."), std::string(""));
    // The words of the fixtures are normalized the same way.
    HumdrumChorale chorale(FIXTURE_CHORALE("chor005"));
    CHECK_EQ(wordTexts(*chorale.lyrics().verse(1))[1], std::string("wasserflüssen"));
    CHECK_EQ(wordTexts(*chorale.lyrics().verse(1))[22], std::string("bäum"));
    CHECK_EQ(chorale.lyrics().verse(1)->words()[22].writtenText, std::string("Bäum'"));
}

TEST_CASE(a_star_in_a_word_or_a_syllable_stands_for_any_characters) {
    CHECK(matchesLyricsGlob("herzens", "herzens"));
    CHECK(!matchesLyricsGlob("herz", "herzens"));
    CHECK(!matchesLyricsGlob("herzens", "herz"));
    CHECK(matchesLyricsGlob("her*", "herzens"));
    CHECK(matchesLyricsGlob("her*", "her"));
    CHECK(!matchesLyricsGlob("her*", "ehren"));
    CHECK(matchesLyricsGlob("*zen", "herzen"));
    CHECK(!matchesLyricsGlob("*zen", "herzens"));
    CHECK(matchesLyricsGlob("*erz*", "herzens"));
    CHECK(matchesLyricsGlob("*erz*", "erz"));
    CHECK(matchesLyricsGlob("h*s", "herzens"));
    CHECK(matchesLyricsGlob("h**s", "herzens"));
    CHECK(matchesLyricsGlob("*e*e*", "herzen"));
    CHECK(!matchesLyricsGlob("*z*z*", "herzen"));
    CHECK(matchesLyricsGlob("*", "herzen"));
    CHECK(matchesLyricsGlob("*", ""));
    CHECK(!matchesLyricsGlob("h*", ""));
    // A letter beyond ASCII is more than one byte, which a star takes like any other.
    CHECK(matchesLyricsGlob("*ber", "über"));
    CHECK(matchesLyricsGlob("ü*", "über"));
    CHECK(!matchesLyricsGlob("u*", "über"));
}

TEST_MAIN()

#include "test_framework.hpp"

#include <algorithm>
#include <string>
#include <vector>

#include "RegolaChords.hpp"

using choralesearch::allowedRegolaChords;
using choralesearch::isValidRegolaTag;
using choralesearch::regolaChords;

namespace {

std::vector<std::string> degreesAndFigures(const std::vector<std::size_t>& indices) {
    std::vector<std::string> chords;
    for (std::size_t index : indices) chords.push_back(regolaChords()[index].scaleDegree + " " + regolaChords()[index].figuredBass);
    return chords;
}

bool contains(const std::vector<std::string>& values, const std::string& value) {
    return std::find(values.begin(), values.end(), value) != values.end();
}

} // namespace

TEST_CASE(regola_chords_are_the_thirty_three_of_kartimento_with_the_octave_as_chord_tone) {
    CHECK_EQ(regolaChords().size(), std::size_t{33});
    for (const auto& chord : regolaChords()) {
        CHECK(chord.chordTones.front() == (choralesearch::RegolaFigure{1, 0}));
        for (const auto& figure : chord.chordTones) CHECK(figure.number >= 1 && figure.number <= 7);
    }
}

TEST_CASE(regola_chords_of_one_mode_allow_only_that_mode) {
    const auto count = [](choralesearch::ChordMode mode) {
        return std::count_if(regolaChords().begin(), regolaChords().end(), [&](const auto& chord) { return chord.mode == mode; });
    };
    CHECK_EQ(count(choralesearch::ChordMode::Major), 3);
    CHECK_EQ(count(choralesearch::ChordMode::Minor), 3);
    for (const auto& chord : regolaChords()) {
        CHECK_EQ(chord.allowsMode(true), chord.mode != choralesearch::ChordMode::Major);
        CHECK_EQ(chord.allowsMode(false), chord.mode != choralesearch::ChordMode::Minor);
    }
}

TEST_CASE(regola_chords_put_the_core_first_and_the_plain_chord_before_the_enriched_one) {
    const auto& chords = regolaChords();
    const auto firstExtended = std::find_if(chords.begin(), chords.end(), [](const auto& chord) { return chord.hasTag("extended"); });
    CHECK(std::all_of(chords.begin(), firstExtended, [](const auto& chord) { return chord.hasTag("core"); }));
    CHECK(std::none_of(firstExtended, chords.end(), [](const auto& chord) { return chord.hasTag("core"); }));
    const auto firstEnriched = std::find_if(chords.begin(), firstExtended, [](const auto& chord) { return chord.hasTag("enriched"); });
    CHECK(std::all_of(firstEnriched, firstExtended, [](const auto& chord) { return chord.hasTag("enriched"); }));
}

TEST_CASE(regola_tags_are_those_of_the_chords_also_negated) {
    CHECK(isValidRegolaTag("core"));
    CHECK(isValidRegolaTag("!affect"));
    CHECK(isValidRegolaTag("leap"));
    CHECK(!isValidRegolaTag("green"));
    CHECK(!isValidRegolaTag("!"));
    CHECK_THROWS(allowedRegolaChords({"green"}));
}

TEST_CASE(regola_allows_the_core_without_tags) {
    const auto allowed = allowedRegolaChords({});
    CHECK_EQ(allowed.size(), std::size_t{12});
    for (std::size_t index : allowed) CHECK(regolaChords()[index].hasTag("core"));
}

TEST_CASE(regola_allows_a_chord_with_one_of_the_tags_and_none_negated) {
    const auto coreWithLeaps = degreesAndFigures(allowedRegolaChords({"core", "leap", "!enriched"}));
    CHECK_EQ(coreWithLeaps.size(), std::size_t{10 + 3});
    CHECK(contains(coreWithLeaps, "4 5 3"));
    CHECK(!contains(coreWithLeaps, "2 4 3"));
    CHECK_EQ(allowedRegolaChords({"core", "extended"}).size(), std::size_t{33});
    // Without a positive tag every chord but those negated.
    CHECK_EQ(allowedRegolaChords({"!affect"}).size(), std::size_t{33 - 11});
}

TEST_MAIN()

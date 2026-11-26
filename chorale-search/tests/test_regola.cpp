#include "test_framework.hpp"

#include <algorithm>
#include <sstream>
#include <string>
#include <vector>

#include "AttributeMatcher.hpp"
#include "HumdrumChorale.hpp"
#include "RegolaAnalysis.hpp"
#include "RegolaChords.hpp"

using choralesearch::allowedRegolaChords;
using choralesearch::AttributeMatcher;
using choralesearch::BassMotion;
using choralesearch::bassMotions;
using choralesearch::HumdrumChorale;
using choralesearch::isValidRegolaTag;
using choralesearch::MatcherOptions;
using choralesearch::PatternPosition;
using choralesearch::regolaChords;
using choralesearch::regolaMode;

namespace {

// Bass, tenor, alto and soprano in C major. Beat 2: the bass D goes up a step, so the sixth chord of the core holds
// and the c of the tenor is no chord tone; its B after it sounds over the E of the bass, which no chord explains.
// Beat 3: the bass F leaps to C, which no chord of the core on the fourth degree allows. Measure 2: the bass D leaps
// to G in a diminution and steps up to E on the next beat.
const char* const kSteps =
    "**kern\t**kern\t**kern\t**kern\n"
    "*C:\t*C:\t*C:\t*C:\n"
    "*M4/4\t*M4/4\t*M4/4\t*M4/4\n"
    "*met(c)\t*met(c)\t*met(c)\t*met(c)\n"
    "=1\t=1\t=1\t=1\n"
    "4C\t4c\t4g\t4ee\n"
    "8DL\t8cL\t4f\t4dd\n"
    "8EJ\t8BJ\t.\t.\n"
    "4F\t4c\t4a\t4cc\n"
    "4C\t8BL\t4g\t4cc\n"
    ".\t8cJ\t.\t.\n"
    "=2\t=2\t=2\t=2\n"
    "8DL\t4B\t4f\t4dd\n"
    "8GJ\t.\t.\t.\n"
    "2E\t2c\t2g\t2cc\n"
    "==\t==\t==\t==\n"
    "*-\t*-\t*-\t*-\n";

// C major. The sixth chord on E with the g# of the alto, and on D with its chromatic passing f#; the major sixth
// f# over the bass A going down.
const char* const kQualitiesInMajor =
    "**kern\t**kern\t**kern\t**kern\n"
    "*C:\t*C:\t*C:\t*C:\n"
    "*M4/4\t*M4/4\t*M4/4\t*M4/4\n"
    "*met(c)\t*met(c)\t*met(c)\t*met(c)\n"
    "=1\t=1\t=1\t=1\n"
    "4E\t4c\t4g\t4cc\n"
    "4E\t4c\t4g#\t4cc\n"
    "4D\t4B\t8fL\t4dd\n"
    ".\t.\t8f#J\t.\n"
    "4C\t4c\t4g\t4ee\n"
    "=2\t=2\t=2\t=2\n"
    "4A\t4c\t8fL\t4ee\n"
    ".\t.\t8f#J\t.\n"
    "4G\t4B\t4g\t4dd\n"
    "2C\t2c\t2e\t2cc\n"
    "==\t==\t==\t==\n"
    "*-\t*-\t*-\t*-\n";

// A minor. The leading tone g# over the fifth degree and the g that is none; the bass f# goes up to g#, then down to
// e; the bass f goes up to g#.
const char* const kMelodicMinor =
    "**kern\t**kern\t**kern\t**kern\n"
    "*a:\t*a:\t*a:\t*a:\n"
    "*M4/4\t*M4/4\t*M4/4\t*M4/4\n"
    "*met(c)\t*met(c)\t*met(c)\t*met(c)\n"
    "=1\t=1\t=1\t=1\n"
    "4E\t4B\t4g#\t4ee\n"
    "4E\t4B\t4g\t4ee\n"
    "4F#\t4A\t4d\t4dd\n"
    "4G#\t4B\t4e\t4dd\n"
    "=2\t=2\t=2\t=2\n"
    "4A\t4c\t4e\t4cc\n"
    "4F#\t4A\t4d\t4dd\n"
    "4E\t4B\t4e\t4b\n"
    "4F\t4A\t4d\t4dd\n"
    "=3\t=3\t=3\t=3\n"
    "4G#\t4B\t4e\t4b\n"
    "2A\t2c\t2e\t2cc\n"
    "==\t==\t==\t==\n"
    "*-\t*-\t*-\t*-\n";

// F major turns to C major after the beat, while the bass D is held: the b of the alto is measured in C.
const char* const kKeyChangeInTheMeasure =
    "**kern\t**kern\t**kern\t**kern\n"
    "*F:\t*F:\t*F:\t*F:\n"
    "*M4/4\t*M4/4\t*M4/4\t*M4/4\n"
    "*met(c)\t*met(c)\t*met(c)\t*met(c)\n"
    "=1\t=1\t=1\t=1\n"
    "4D\t4F\t8aL\t4ff\n"
    "*C:\t*C:\t*C:\t*C:\n"
    ".\t.\t8bnJ\t.\n"
    "4E\t4G\t4cc\t4ee\n"
    "2F\t2A\t2cc\t2ff\n"
    "==\t==\t==\t==\n"
    "*-\t*-\t*-\t*-\n";

// C major. The same 4 2 over F twice: the bass goes down to E, then up to G.
const char* const kThirdInversion =
    "**kern\t**kern\t**kern\t**kern\n"
    "*C:\t*C:\t*C:\t*C:\n"
    "*M4/4\t*M4/4\t*M4/4\t*M4/4\n"
    "*met(c)\t*met(c)\t*met(c)\t*met(c)\n"
    "=1\t=1\t=1\t=1\n"
    "4F\t4B\t4d\t4g\n"
    "4E\t4c\t4c\t4g\n"
    "4F\t4B\t4d\t4g\n"
    "4G\t4B\t4d\t4g\n"
    "==\t==\t==\t==\n"
    "*-\t*-\t*-\t*-\n";

// The regola up and down in C major and twice in C minor, the second time without the enriched chord on the sixth
// degree going down.
const char* const kRegolaDellOttava =
    "!!!OTL: Regola dell'ottava in C major and C minor\n"
    "**kern\t**kern\t**kern\t**kern\n"
    "*clefF4\t*clefGv2\t*clefG2\t*clefG2\n"
    "*k[]\t*k[]\t*k[]\t*k[]\n"
    "*C:\t*C:\t*C:\t*C:\n"
    "*M4/4\t*M4/4\t*M4/4\t*M4/4\n"
    "*met(c)\t*met(c)\t*met(c)\t*met(c)\n"
    "=1\t=1\t=1\t=1\n"
    "4C\t4e\t4g\t4cc\n"
    "4D\t4f\t4g\t4b\n"
    "4E\t4e\t4g\t4cc\n"
    "4F\t4d\t4a\t4cc\n"
    "=\t=\t=\t=\n"
    "4G\t4d\t4g\t4b\n"
    "4G\t4b\t4dd\t4gg\n"
    "4A\t4a\t4cc\t4ff\n"
    "4B\t4g\t4dd\t4ff\n"
    "=\t=\t=\t=\n"
    "2c\t2g\t2cc\t2ee\n"
    "2r\t2r\t2r\t2r\n"
    "=\t=\t=\t=\n"
    "4c\t4g\t4cc\t4ee\n"
    "4B\t4g\t4dd\t4gg\n"
    "4A\t4cc\t4dd\t4ff#X\n"
    "4G\t4b\t4dd\t4gg\n"
    "=\t=\t=\t=\n"
    "4G\t4d\t4g\t4b\n"
    "4F\t4d\t4g\t4b\n"
    "4E\t4e\t4g\t4cc\n"
    "4D\t4f\t4g\t4b\n"
    "=||\t=||\t=||\t=||\n"
    "2C\t2e\t2g\t2cc\n"
    "2r\t2r\t2r\t2r\n"
    "=||\t=||\t=||\t=||\n"
    "*k[b-e-a-]\t*k[b-e-a-]\t*k[b-e-a-]\t*k[b-e-a-]\n"
    "*c:\t*c:\t*c:\t*c:\n"
    "4C\t4e-\t4g\t4cc\n"
    "4D\t4f\t4g\t4bn\n"
    "4E-\t4e-\t4g\t4cc\n"
    "4F\t4d\t4a-\t4cc\n"
    "=\t=\t=\t=\n"
    "4G\t4d\t4g\t4bn\n"
    "4G\t4bn\t4dd\t4gg\n"
    "4An\t4an\t4cc\t4ff\n"
    "4Bn\t4g\t4dd\t4ff\n"
    "=\t=\t=\t=\n"
    "2c\t2g\t2cc\t2ee-\n"
    "2r\t2r\t2r\t2r\n"
    "=\t=\t=\t=\n"
    "4c\t4g\t4cc\t4ee-\n"
    "4B-X\t4g\t4dd\t4gg\n"
    "4A-X\t4cc\t4dd\t4ff\n"
    "4G\t4bn\t4dd\t4gg\n"
    "=\t=\t=\t=\n"
    "4G\t4d\t4g\t4bn\n"
    "4F\t4d\t4g\t4bn\n"
    "4E-\t4e-\t4g\t4cc\n"
    "4D\t4f\t4g\t4bn\n"
    "=\t=\t=\t=\n"
    "2C\t2e-\t2g\t2cc\n"
    "2r\t2r\t2r\t2r\n"
    "=||\t=||\t=||\t=||\n"
    "!! The minor once more, the sixth degree going down without the enriched chord\n"
    "4C\t4e-\t4g\t4cc\n"
    "4D\t4f\t4g\t4bn\n"
    "4E-\t4e-\t4g\t4cc\n"
    "4F\t4d\t4a-\t4cc\n"
    "=\t=\t=\t=\n"
    "4G\t4d\t4g\t4bn\n"
    "4G\t4bn\t4dd\t4gg\n"
    "4An\t4an\t4cc\t4ff\n"
    "4Bn\t4g\t4dd\t4ff\n"
    "=\t=\t=\t=\n"
    "2c\t2g\t2cc\t2ee-\n"
    "2r\t2r\t2r\t2r\n"
    "=\t=\t=\t=\n"
    "4c\t4g\t4cc\t4ee-\n"
    "4B-X\t4g\t4dd\t4gg\n"
    "4A-X\t4cc\t4cc\t4ff\n"
    "4G\t4bn\t4dd\t4gg\n"
    "=\t=\t=\t=\n"
    "4G\t4d\t4g\t4bn\n"
    "4F\t4d\t4g\t4bn\n"
    "4E-\t4e-\t4g\t4cc\n"
    "4D\t4f\t4g\t4bn\n"
    "=\t=\t=\t=\n"
    "2C\t2e-\t2g\t2cc\n"
    "2r\t2r\t2r\t2r\n"
    "==\t==\t==\t==\n"
    "*-\t*-\t*-\t*-\n";

// G major, as chor001 begins its second measure: the b of the soprano is held over the bass D.
const char* const kHeldSixth =
    "**kern\t**kern\t**kern\t**kern\n"
    "*G:\t*G:\t*G:\t*G:\n"
    "*M3/4\t*M3/4\t*M3/4\t*M3/4\n"
    "*met(3)\t*met(3)\t*met(3)\t*met(3)\n"
    "=1\t=1\t=1\t=1\n"
    "4G\t4G\t2d\t4.b\n"
    "4D\t4F#\t.\t.\n"
    ".\t.\t.\t8a\n"
    "4E\t4G\t4B\t4g\n"
    "==\t==\t==\t==\n"
    "*-\t*-\t*-\t*-\n";

// C major. The soprano repeats its g over the bass C and over the F that goes up: a chord tone, then none.
const char* const kRepeatedNote =
    "**kern\t**kern\t**kern\t**kern\n"
    "*C:\t*C:\t*C:\t*C:\n"
    "*M4/4\t*M4/4\t*M4/4\t*M4/4\n"
    "*met(c)\t*met(c)\t*met(c)\t*met(c)\n"
    "=1\t=1\t=1\t=1\n"
    "4C\t4c\t4e\t4g\n"
    "4F\t4c\t4a\t4g\n"
    "2G\t2B\t2g\t2dd\n"
    "==\t==\t==\t==\n"
    "*-\t*-\t*-\t*-\n";

// A minor and C major: the diminished seventh chord over the sixth degree going down, its 2 the leading tone.
const char* const kSeventhOnTheSixthDegreeInMinor =
    "**kern\t**kern\t**kern\t**kern\n"
    "*a:\t*a:\t*a:\t*a:\n"
    "*M4/4\t*M4/4\t*M4/4\t*M4/4\n"
    "*met(c)\t*met(c)\t*met(c)\t*met(c)\n"
    "=1\t=1\t=1\t=1\n"
    "4F\t4G#\t4B\t4d\n"
    "4E\t4G#\t4B\t4e\n"
    "2A\t2A\t2c\t2e\n"
    "==\t==\t==\t==\n"
    "*-\t*-\t*-\t*-\n";

const char* const kSeventhOnTheSixthDegreeInMajor =
    "**kern\t**kern\t**kern\t**kern\n"
    "*C:\t*C:\t*C:\t*C:\n"
    "*M4/4\t*M4/4\t*M4/4\t*M4/4\n"
    "*met(c)\t*met(c)\t*met(c)\t*met(c)\n"
    "=1\t=1\t=1\t=1\n"
    "4A\t4B\t4d\t4f\n"
    "4G\t4B\t4d\t4g\n"
    "2C\t2c\t2e\t2g\n"
    "==\t==\t==\t==\n"
    "*-\t*-\t*-\t*-\n";

// F lydian. The bass b-flat is the fourth degree as in major, the b of the mode the raised fourth.
const char* const kLydianFourths =
    "**kern\t**kern\t**kern\t**kern\n"
    "*F:lyd\t*F:lyd\t*F:lyd\t*F:lyd\n"
    "*M4/4\t*M4/4\t*M4/4\t*M4/4\n"
    "*met(c)\t*met(c)\t*met(c)\t*met(c)\n"
    "=1\t=1\t=1\t=1\n"
    "4B-\t4d\t4f\t4g\n"
    "4c\t4e\t4g\t4cc\n"
    "4Bn\t4d\t4f\t4g\n"
    "4c\t4e\t4g\t4cc\n"
    "==\t==\t==\t==\n"
    "*-\t*-\t*-\t*-\n";

HumdrumChorale readScore(const char* score) {
    std::istringstream contents(score);
    return HumdrumChorale(contents, "regola-test");
}

// The `index`th note or rest of `voice`, counted from 0.
hum::HTp nthNote(const HumdrumChorale& chorale, std::size_t voice, std::size_t index) {
    for (hum::HTp token = chorale.spine("kern", voice)->getNextToken(); token; token = token->getNextToken()) {
        if (!token->isData() || token->isNull()) continue;
        if (index-- == 0) return token;
    }
    return nullptr;
}

std::optional<bool> isChordTone(const HumdrumChorale& chorale, std::size_t voice, std::size_t index,
                                const std::vector<std::string>& tags = {}) {
    return chorale.regolaAnalysis(tags).isChordTone(chorale, voice, nthNote(chorale, voice, index)->getLineNumber());
}

const choralesearch::RegolaChord* chordOnNote(const HumdrumChorale& chorale, std::size_t index) {
    return chorale.regolaAnalysis({}).chordAtLine(nthNote(chorale, 1, index)->getLineNumber());
}

// The degree and the figures of the chord on each note of the bass, "-" where it has none.
std::vector<std::string> chordsOverTheBass(const HumdrumChorale& chorale, const std::vector<std::string>& tags) {
    const auto& analysis = chorale.regolaAnalysis(tags);
    std::vector<std::string> chords;
    for (hum::HTp token = chorale.spine("kern", 1)->getNextToken(); token; token = token->getNextToken()) {
        if (!token->isData() || token->isNull() || token->isRest()) continue;
        const choralesearch::RegolaChord* chord = analysis.chordAtLine(token->getLineNumber());
        chords.push_back(chord ? chord->scaleDegree + " " + chord->figuredBass : "-");
    }
    return chords;
}

std::vector<std::string> degreesAndFigures(const std::vector<std::size_t>& indices) {
    std::vector<std::string> chords;
    for (std::size_t index : indices) chords.push_back(regolaChords()[index].scaleDegree + " " + regolaChords()[index].figuredBass);
    return chords;
}

bool contains(const std::vector<std::string>& values, const std::string& value) {
    return std::find(values.begin(), values.end(), value) != values.end();
}

constexpr std::size_t kTenor = 2;
constexpr std::size_t kAlto = 3;

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

TEST_CASE(regola_reads_the_church_modes_by_their_third_and_leaves_out_phrygian) {
    CHECK(!regolaMode("major")->minor);
    CHECK(regolaMode("minor")->minor);
    CHECK(regolaMode("dorian")->minor);
    CHECK(!regolaMode("mixolydian")->minor);
    CHECK(!regolaMode("phrygian"));
    CHECK(!regolaMode("locrian"));
}

TEST_CASE(regola_bass_motion_is_a_step_to_the_next_note_or_to_the_next_one_with_a_metric_weight) {
    const HumdrumChorale chorale = readScore(kSteps);
    const auto motionsFrom = [&](std::size_t index) { return bassMotions(chorale, nthNote(chorale, 1, index)); };
    using enum BassMotion;
    CHECK(motionsFrom(1) == std::vector<BassMotion>{Ascending});
    CHECK(motionsFrom(3) == std::vector<BassMotion>{Leap});
    CHECK(motionsFrom(4) == std::vector<BassMotion>{Ascending});
    CHECK(motionsFrom(5) == std::vector<BassMotion>{Ascending});
    CHECK(motionsFrom(7).empty());
}

TEST_CASE(regola_takes_the_four_two_chord_only_for_a_bass_that_goes_down_by_step) {
    const HumdrumChorale chorale = readScore(kThirdInversion);
    REQUIRE(chordOnNote(chorale, 0) != nullptr);
    CHECK_EQ(chordOnNote(chorale, 0)->figuredBass, std::string("2"));
    REQUIRE(chordOnNote(chorale, 2) != nullptr);
    CHECK_EQ(chordOnNote(chorale, 2)->figuredBass, std::string("6 5"));
}

TEST_CASE(regola_finds_its_own_chords_in_the_rule_up_and_down_in_major_and_minor) {
    const HumdrumChorale chorale = readScore(kRegolaDellOttava);
    const std::vector<std::string> up = {"1 5 3", "2 4 3", "3 6", "4 6 5", "5 5 3", "5 5 3", "6 6", "7 6 5", "1 5 3"};
    const std::vector<std::string> down = {"1 5 3", "7 6", "6 4 3", "5 5 3", "5 5 3", "4 2", "3 6", "2 4 3", "1 5 3"};
    const std::vector<std::string> downWithoutEnriched = {"1 5 3", "7 6", "6 6", "5 5 3", "5 5 3", "4 2", "3 6", "2 4 3", "1 5 3"};
    std::vector<std::string> expected;
    for (const auto* part : {&up, &down, &up, &down, &up, &downWithoutEnriched}) expected.insert(expected.end(), part->begin(), part->end());

    CHECK(chordsOverTheBass(chorale, {}) == expected);
    // The chords of further contexts take none of their places.
    CHECK(chordsOverTheBass(chorale, {"core", "extended"}) == expected);

    // Every note above the bass is a chord tone, the fifths of the 6 5 included.
    const auto& analysis = chorale.regolaAnalysis({});
    for (std::size_t voice = 2; voice <= 4; ++voice) {
        for (hum::HTp token = chorale.spine("kern", voice)->getNextToken(); token; token = token->getNextToken()) {
            if (!token->isData() || token->isNull() || token->isRest()) continue;
            CHECK_EQ(analysis.isChordTone(chorale, voice, token->getLineNumber()), std::optional<bool>(true));
        }
    }
}

TEST_CASE(regola_judges_a_note_against_the_chord_over_the_bass_of_its_line) {
    const HumdrumChorale chorale = readScore(kSteps);
    REQUIRE(chordOnNote(chorale, 1) != nullptr);
    CHECK_EQ(chordOnNote(chorale, 1)->figuredBass, std::string("6"));
    CHECK_EQ(isChordTone(chorale, kTenor, 1), std::optional<bool>(false));
    CHECK(chordOnNote(chorale, 2) == nullptr);
    CHECK(!isChordTone(chorale, kTenor, 2));
    // Over the bass D held under the B in measure 2.
    CHECK_EQ(isChordTone(chorale, kTenor, 6), std::optional<bool>(true));

    // The bass is no voice above it, and a line without a chord judges no note.
    CHECK(!isChordTone(chorale, 1, 1));
    CHECK(chordOnNote(chorale, 3) == nullptr);
    CHECK(!isChordTone(chorale, kTenor, 3));
}

TEST_CASE(regola_chord_tone_judges_a_held_note_on_every_line_fb_steps_on) {
    const HumdrumChorale chorale = readScore(kHeldSixth);
    constexpr std::size_t kSoprano = 4;
    const int bassD = nthNote(chorale, 1, 1)->getLineNumber();
    // The b over D is a sixth, which the fifth degree of the core has not.
    CHECK_EQ(chorale.regolaAnalysis({}).isChordTone(chorale, kSoprano, bassD), std::optional<bool>(false));
    const auto matchedLines = [&](const char* feature) {
        std::vector<std::size_t> lines;
        const AttributeMatcher matcher(feature, {PatternPosition{.keys = {{"regolaChordTone", {"false"}}}}});
        for (const auto& match : matcher.findAll(chorale, kSoprano)) lines.push_back(match.startLineNumber);
        return lines;
    };
    CHECK(matchedLines("fb") == std::vector<std::size_t>{static_cast<std::size_t>(bassD)});
    // Driven by kern the soprano has no position there, it begins no note.
    CHECK(matchedLines("kern").empty());
}

TEST_CASE(regola_chord_tone_is_judged_at_every_repetition_of_a_split_note) {
    const HumdrumChorale chorale = readScore(kRepeatedNote);
    constexpr std::size_t kSoprano = 4;
    const AttributeMatcher halfNotes("kern", {PatternPosition{.keys = {{"duration", {"2"}}, {"regolaChordTone", {"true"}}}}},
                                     MatcherOptions{.durationAllowSplitNotes = true});
    // The dd over the bass G. The g written as two quarters is no chord tone over the bass F.
    CHECK_EQ(halfNotes.findAll(chorale, kSoprano).size(), std::size_t{1});
}

TEST_CASE(regola_chord_tone_takes_the_quality_the_key_gives_a_figure) {
    const HumdrumChorale chorale = readScore(kQualitiesInMajor);
    // The minor third g over E, not g#; the f over D, not its chromatic passing f#.
    CHECK_EQ(isChordTone(chorale, kAlto, 0), std::optional<bool>(true));
    CHECK_EQ(isChordTone(chorale, kAlto, 1), std::optional<bool>(false));
    CHECK_EQ(isChordTone(chorale, kAlto, 2), std::optional<bool>(true));
    CHECK_EQ(isChordTone(chorale, kAlto, 3), std::optional<bool>(false));
    // major-sixth: f# over the bass A going down, not f.
    CHECK_EQ(isChordTone(chorale, kAlto, 5), std::optional<bool>(false));
    CHECK_EQ(isChordTone(chorale, kAlto, 6), std::optional<bool>(true));
}

TEST_CASE(regola_raises_the_leading_tone_and_reads_the_sixth_and_seventh_of_the_bass_in_minor_as_melodic) {
    const HumdrumChorale chorale = readScore(kMelodicMinor);
    // semitonium-modi: g# over E, not g.
    CHECK_EQ(isChordTone(chorale, kAlto, 0), std::optional<bool>(true));
    CHECK_EQ(isChordTone(chorale, kAlto, 1), std::optional<bool>(false));
    // f# going up has a chord, f# going down and f going up none.
    CHECK(chordOnNote(chorale, 2) != nullptr);
    CHECK(chordOnNote(chorale, 3) != nullptr);
    CHECK(chordOnNote(chorale, 5) == nullptr);
    CHECK(chordOnNote(chorale, 7) == nullptr);
}

TEST_CASE(regola_takes_a_chord_of_minor_only_in_a_mode_read_as_minor) {
    const auto figuredBassOnTheFirstNote = [](const char* score) {
        const HumdrumChorale chorale = readScore(score);
        const choralesearch::RegolaChord* chord = chorale.regolaAnalysis({"affect"}).chordAtLine(nthNote(chorale, 1, 0)->getLineNumber());
        return chord ? chord->figuredBass : std::string("-");
    };
    CHECK_EQ(figuredBassOnTheFirstNote(kSeventhOnTheSixthDegreeInMinor), std::string("2"));
    CHECK(figuredBassOnTheFirstNote(kSeventhOnTheSixthDegreeInMajor) != "2");
}

TEST_CASE(regola_reads_the_fourths_of_lydian_as_in_major) {
    const HumdrumChorale chorale = readScore(kLydianFourths);
    const auto degreeOnNote = [&](std::size_t index) {
        const choralesearch::RegolaChord* chord = chorale.regolaAnalysis({"core", "extended"}).chordAtLine(nthNote(chorale, 1, index)->getLineNumber());
        return chord ? chord->scaleDegree : std::string("-");
    };
    CHECK_EQ(degreeOnNote(0), std::string("4"));
    CHECK_EQ(degreeOnNote(2), std::string("4+"));
}

TEST_CASE(regola_measures_the_notes_after_a_change_of_key_in_the_new_key) {
    const HumdrumChorale chorale = readScore(kKeyChangeInTheMeasure);
    // The sixth chord on the second degree of C, its sixth the leading tone b.
    CHECK_EQ(isChordTone(chorale, kAlto, 1), std::optional<bool>(true));
}

TEST_CASE(regola_chord_tone_key_judges_the_notes_with_the_chords_of_the_tags) {
    const HumdrumChorale chorale = readScore(kSteps);
    const auto count = [&](const char* value, std::vector<std::string> tags = {}) {
        return AttributeMatcher("kern", {PatternPosition{.keys = {{"regolaChordTone", {value}}}}},
                                MatcherOptions{.regolaTags = std::move(tags)})
            .findAll(chorale, kTenor)
            .size();
    };
    // The core: the c of beat 1 and after beat 4, the B and the c of measure 2; the c on beat 2 and the B on beat 4.
    CHECK_EQ(count("true"), std::size_t{2 + 2});
    CHECK_EQ(count("false"), std::size_t{2});
    // The passing six-four chord on the first degree: the c on beat 1 and after beat 4, not the B.
    CHECK_EQ(count("true", {"passing"}), std::size_t{2});
    CHECK_EQ(count("false", {"passing"}), std::size_t{1});
}

TEST_MAIN()

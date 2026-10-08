#include "test_framework.hpp"

#include <sstream>
#include <string>
#include <vector>

#include "Check.hpp"
#include "HumdrumChorale.hpp"

using choralesearch::HumdrumChorale;
using choralesearch::Finding;
using choralesearch::findHiddenMotion;
using choralesearch::findParallelMotion;
using choralesearch::findVoiceRangeViolations;
using choralesearch::runChecks;

namespace {

// Four voices, bass to soprano, in C major: the header takes five lines and the barline a sixth,
// so the first row of notes is line 7.
constexpr int kFirstRow = 7;

HumdrumChorale score(const std::vector<std::string>& rows) {
    std::string text =
        "**kern\t**kern\t**kern\t**kern\n"
        "*clefF4\t*clefGv2\t*clefG2\t*clefG2\n"
        "*k[]\t*k[]\t*k[]\t*k[]\n"
        "*C:\t*C:\t*C:\t*C:\n"
        "*M4/4\t*M4/4\t*M4/4\t*M4/4\n"
        "=1\t=1\t=1\t=1\n";
    for (const std::string& row : rows) text += row + "\n";
    text += "==\t==\t==\t==\n*-\t*-\t*-\t*-\n";
    std::istringstream contents(text);
    return HumdrumChorale(contents, "check-test");
}

} // namespace

TEST_CASE(parallel_fifths_are_found_upwards_and_downwards) {
    const HumdrumChorale chorale = score({
        "4C\t4g\t4e\t4cc",
        "4D\t4a\t4f\t4cc",
        "4C\t4g\t4e\t4cc",
        "4C\t4g\t4e\t4cc",
    });
    const std::vector<Finding> findings = findParallelMotion(chorale);
    REQUIRE(findings.size() == 2);

    CHECK_EQ(findings[0].check, std::string("parallelFifths"));
    CHECK_EQ(findings[0].direction, std::string("up"));
    CHECK_EQ(findings[0].lowerVoice, std::size_t{1});
    CHECK_EQ(findings[0].upperVoice, std::size_t{2});
    CHECK_EQ(findings[0].startLine, kFirstRow);
    CHECK_EQ(findings[0].endLine, kFirstRow + 1);

    CHECK_EQ(findings[1].check, std::string("parallelFifths"));
    CHECK_EQ(findings[1].direction, std::string("down"));
    CHECK_EQ(findings[1].startLine, kFirstRow + 1);
    CHECK_EQ(findings[1].endLine, kFirstRow + 2);
}

TEST_CASE(parallel_octaves_are_found_between_outer_voices) {
    const HumdrumChorale chorale = score({
        "4C\t4g\t4e\t4c",
        "4D\t4g\t4f\t4d",
        "4D\t4g\t4f\t4d",
        "4D\t4g\t4f\t4d",
    });
    const std::vector<Finding> findings = findParallelMotion(chorale);
    REQUIRE(findings.size() == 1);
    CHECK_EQ(findings[0].check, std::string("parallelOctaves"));
    CHECK_EQ(findings[0].direction, std::string("up"));
    CHECK_EQ(findings[0].lowerVoice, std::size_t{1});
    CHECK_EQ(findings[0].upperVoice, std::size_t{4});
    CHECK_EQ(findings[0].startLine, kFirstRow);
    CHECK_EQ(findings[0].endLine, kFirstRow + 1);
}

TEST_CASE(oblique_and_contrary_motion_and_a_diminished_fifth_are_no_parallels) {
    const HumdrumChorale chorale = score({
        "4C\t4g\t4e\t4cc",
        "4C\t4a\t4f\t4cc", // the bass holds: oblique
        "4D\t4g\t4f\t4cc", // bass up, tenor down: contrary
        "4C\t4g\t4e\t4cc",
        "4B\t4f\t4d\t4g",  // a fifth into a diminished one
        "4c\t4g\t4e\t4cc", // and out of it again
    });
    CHECK_EQ(findParallelMotion(chorale).size(), std::size_t{0});
}

TEST_CASE(a_hidden_fifth_is_found_when_the_soprano_leaps_into_it_in_the_same_direction_as_the_bass) {
    const HumdrumChorale chorale = score({
        "4C\t4g\t4e\t4c",  // an octave
        "4D\t4g\t4f\t4a",  // the bass a step up, the soprano a sixth up into a twelfth
        "4D\t4g\t4f\t4a",
        "4D\t4g\t4f\t4a",
    });
    const std::vector<Finding> findings = findHiddenMotion(chorale);
    REQUIRE(findings.size() == 1);
    CHECK_EQ(findings[0].check, std::string("hiddenFifths"));
    CHECK_EQ(findings[0].severity, std::string("warning"));
    CHECK_EQ(findings[0].direction, std::string("up"));
    CHECK_EQ(findings[0].lowerVoice, std::size_t{1});
    CHECK_EQ(findings[0].upperVoice, std::size_t{4});
    CHECK_EQ(findings[0].startLine, kFirstRow);
    CHECK_EQ(findings[0].endLine, kFirstRow + 1);
}

TEST_CASE(a_hidden_octave_is_found_downwards_too) {
    const HumdrumChorale chorale = score({
        "4G\t4g\t4d\t4dd",  // a twelfth
        "4D\t4g\t4d\t4d",   // both down, the soprano a fifth, into an octave
        "4D\t4g\t4d\t4d",
        "4D\t4g\t4d\t4d",
    });
    const std::vector<Finding> findings = findHiddenMotion(chorale);
    REQUIRE(findings.size() == 1);
    CHECK_EQ(findings[0].check, std::string("hiddenOctaves"));
    CHECK_EQ(findings[0].direction, std::string("down"));
}

TEST_CASE(a_step_of_the_soprano_contrary_motion_and_a_held_bass_hide_nothing) {
    const HumdrumChorale chorale = score({
        "4G\t4g\t4e\t4b",
        "4c\t4g\t4e\t4cc", // both up, but the soprano by a step: excused
        "4G\t4g\t4e\t4g",  // both down, octave to octave: a parallel, no hidden one
        "4C\t4g\t4e\t4cc", // bass down, soprano up: contrary
        "4C\t4g\t4e\t4gg", // the bass holds: oblique
    });
    CHECK_EQ(findHiddenMotion(chorale).size(), std::size_t{0});
}

TEST_CASE(a_fifth_or_octave_that_follows_the_same_one_is_a_parallel_and_no_hidden_one) {
    const HumdrumChorale chorale = score({
        "4C\t4g\t4e\t4c",
        "4D\t4g\t4f\t4d", // octave to octave, even though the soprano is a step
        "4C\t4g\t4e\t4g",
        "4F\t4g\t4f\t4cc", // a twelfth up to a twelfth by a leap up: the same fifth
    });
    CHECK_EQ(findHiddenMotion(chorale).size(), std::size_t{0});
}

TEST_CASE(a_step_of_the_soprano_into_a_perfect_interval_is_found_when_steps_are_not_allowed) {
    const HumdrumChorale chorale = score({
        "4G\t4g\t4e\t4b",
        "4c\t4g\t4e\t4cc", // both up, the soprano by a step, into an octave
        "4c\t4g\t4e\t4cc",
        "4c\t4g\t4e\t4cc",
    });
    CHECK_EQ(findHiddenMotion(chorale).size(), std::size_t{0});
    CHECK_EQ(findHiddenMotion(chorale, true).size(), std::size_t{0});

    const std::vector<Finding> findings = findHiddenMotion(chorale, false);
    REQUIRE(findings.size() == 1);
    CHECK_EQ(findings[0].check, std::string("hiddenOctaves"));
    CHECK_EQ(findings[0].startLine, kFirstRow);
}

TEST_CASE(run_checks_passes_on_whether_steps_are_allowed_in_hidden_motion) {
    const HumdrumChorale chorale = score({
        "4G\t4g\t4e\t4b",
        "4c\t4g\t4e\t4cc",
        "4c\t4g\t4e\t4cc",
        "4c\t4g\t4e\t4cc",
    });
    const auto ranges = choralesearch::VoiceRangeSet::StraussBerlioz;
    CHECK_EQ(runChecks(chorale, ranges).size(), std::size_t{0});
    CHECK_EQ(runChecks(chorale, ranges, true).size(), std::size_t{0});
    CHECK_EQ(runChecks(chorale, ranges, false).size(), std::size_t{1});
}

TEST_CASE(hidden_motion_needs_both_voices_to_attack_the_same_notes) {
    const HumdrumChorale chorale = score({
        "4C\t4g\t4e\t4c",
        "4D\t4g\t4f\t4a",
    });
    CHECK_EQ(findHiddenMotion(chorale).size(), std::size_t{1});

    const HumdrumChorale offset = score({
        "2C\t4g\t4e\t4c",
        ".\t4g\t4f\t4a",  // the bass holds its note under the soprano's move
        "4D\t4g\t4f\t4a",
    });
    CHECK_EQ(findHiddenMotion(offset).size(), std::size_t{0});
}

TEST_CASE(notes_outside_the_range_of_their_voice_are_found_with_the_side_they_lie_on) {
    const HumdrumChorale chorale = score({
        "4g\t4BB\t4e\t4cc", // the bass on g' and the tenor on B, both out of their range
        "4C\t4g\t4e\t4cc",
    });
    const std::vector<Finding> findings = findVoiceRangeViolations(chorale);
    REQUIRE(findings.size() == 2);

    CHECK_EQ(findings[0].check, std::string("voiceRange"));
    CHECK_EQ(findings[0].severity, std::string("warning"));
    CHECK_EQ(findings[0].direction, std::string("above"));
    CHECK_EQ(findings[0].lowerVoice, std::size_t{1});
    CHECK_EQ(findings[0].upperVoice, std::size_t{1});
    CHECK_EQ(findings[0].startLine, kFirstRow);
    CHECK_EQ(findings[0].endLine, kFirstRow);

    CHECK_EQ(findings[1].direction, std::string("below"));
    CHECK_EQ(findings[1].lowerVoice, std::size_t{2});
}

TEST_CASE(notes_at_the_edges_of_the_range_and_rests_are_no_findings) {
    const HumdrumChorale chorale = score({
        "4FF\t4C\t4F\t4c",     // the lowest note of every voice: F, c, f and c'
        "4e-\t4b-\t4ee-\t4bb-", // the highest note of every voice: es', b', es'' and b''
        "4r\t4r\t4r\t4r",
    });
    CHECK_EQ(findVoiceRangeViolations(chorale).size(), std::size_t{0});
}

TEST_CASE(the_ranges_decide_which_notes_are_out_of_range) {
    // A tenor on b flat': inside the range of Berlioz and Strauss, above the one of Bach.
    const HumdrumChorale chorale = score({
        "4C\t4b-\t4g\t4c",
        "4C\t4g\t4g\t4c",
    });
    CHECK_EQ(findVoiceRangeViolations(chorale, choralesearch::VoiceRangeSet::StraussBerlioz).size(), std::size_t{0});

    const auto bach = findVoiceRangeViolations(chorale, choralesearch::VoiceRangeSet::Bach);
    REQUIRE(bach.size() == 1);
    CHECK_EQ(bach[0].lowerVoice, std::size_t{2});
    CHECK_EQ(bach[0].direction, std::string("above"));
}

TEST_CASE(every_set_of_ranges_has_a_lower_limit_below_its_upper_one_for_each_voice) {
    for (const auto set : {choralesearch::VoiceRangeSet::StraussBerlioz, choralesearch::VoiceRangeSet::Bach}) {
        for (const auto& range : choralesearch::voiceRanges(set)) CHECK(range.lower < range.upper);
    }
}

TEST_CASE(run_checks_gathers_the_findings_of_every_check_in_the_order_of_the_score) {
    const HumdrumChorale chorale = score({
        "4C\t4g\t4e\t4cc",
        "4D\t4a\t4f\t4cc",
        "4C\t4g\t4e\t4cc",
        "4C\t4g\t4e\t4cc",
    });
    const std::vector<Finding> findings = runChecks(chorale);
    REQUIRE(findings.size() == 2);
    CHECK_EQ(findings[0].startLine, kFirstRow);
    CHECK_EQ(findings[1].startLine, kFirstRow + 1);
    CHECK_EQ(findings[0].severity, std::string("error"));
}

TEST_MAIN()

#include "test_framework.hpp"

#include <sstream>
#include <string>
#include <vector>

#include "Check.hpp"
#include "HumdrumChorale.hpp"

using choralesearch::HumdrumChorale;
using choralesearch::Finding;
using choralesearch::findParallelMotion;
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

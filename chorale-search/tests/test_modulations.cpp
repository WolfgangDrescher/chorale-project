#include "test_framework.hpp"

#include <sstream>
#include <string>
#include <vector>

#include "Modulations.hpp"

using choralesearch::applyModulations;
using choralesearch::Modulation;

namespace {

// The rows of a score as one text, a line feed after each.
std::string lines(const std::vector<std::string>& rows) {
    std::string text;
    for (const std::string& row : rows) text += row + "\n";
    return text;
}

std::string startIn(const std::string& key, const std::vector<std::string>& rows) {
    hum::HumdrumFile infile;
    REQUIRE(infile.readString(lines(rows)));
    applyModulations(infile, "test", {Modulation{"0/1", 0, hum::HumNum(1), key}});
    std::stringstream out;
    out << infile;
    return out.str();
}

} // namespace

TEST_CASE(starting_key_replaces_a_major_or_minor_key_of_the_source) {
    const std::string result = startIn("G", {
        "**kern\t**kern",
        "*k[f#]\t*k[f#]",
        "*e:\t*e:",
        "4e\t4g",
        "*-\t*-",
    });
    CHECK_EQ(result, lines({
        "**kern\t**kern",
        "*k[f#]\t*k[f#]",
        "*G:\t*G:",
        "4e\t4g",
        "*-\t*-",
    }));
}

TEST_CASE(starting_key_is_annotated_below_a_church_mode_of_the_source) {
    const std::string result = startIn("e", {
        "**kern\t**kern",
        "*k[]\t*k[]",
        "*e:phr\t*e:phr",
        "4e\t4g",
        "*-\t*-",
    });
    CHECK_EQ(result, lines({
        "**kern\t**kern",
        "*k[]\t*k[]",
        "*e:phr\t*e:phr",
        "*e:\t*e:",
        "4e\t4g",
        "*-\t*-",
    }));
}

TEST_CASE(starting_key_is_inserted_where_the_source_has_none) {
    const std::string result = startIn("C", {
        "**kern\t**kern",
        "*k[]\t*k[]",
        "4c\t4e",
        "*-\t*-",
    });
    CHECK_EQ(result, lines({
        "**kern\t**kern",
        "*k[]\t*k[]",
        "*C:\t*C:",
        "4c\t4e",
        "*-\t*-",
    }));
}

TEST_MAIN()

TEST_CASE(key_left_open_is_written_as_the_unknown_key_of_humdrum) {
    hum::HumdrumFile infile;
    REQUIRE(infile.readString(lines({"**kern", "*k[]", "*C:", "=1", "4c", "4d", "*-"})));
    applyModulations(infile, "test", {Modulation{"1/2", 1, hum::HumNum(2), "?"}});
    std::stringstream out;
    out << infile;
    CHECK_EQ(out.str(), lines({"**kern", "*k[]", "*C:", "=1", "4c", "*?:", "4d", "*-"}));
}

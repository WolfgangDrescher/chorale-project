#include "test_framework.hpp"

#include <string>
#include <vector>

#include "LinePositions.hpp"

using choralesearch::LinePosition;
using choralesearch::linePositions;

namespace {

// The positions of the data lines of a score, in order.
std::vector<std::string> positionsOf(const std::string& score) {
    hum::HumdrumFile infile;
    REQUIRE(infile.readString(score));
    std::vector<std::string> positions;
    for (const LinePosition& position : linePositions(infile)) positions.push_back(position.position);
    return positions;
}

} // namespace

TEST_CASE(the_beats_count_quarters_from_the_last_numbered_barline) {
    CHECK_EQ(positionsOf("**kern\n*M3/4\n=1\n4c\n4d\n4e\n=2\n2f\n4g\n*-\n"),
             (std::vector<std::string>{"1/1", "1/2", "1/3", "2/1", "2/3"}));
}

TEST_CASE(the_pickup_counts_back_from_the_first_barline) {
    CHECK_EQ(positionsOf("**kern\n*M3/4\n4c\n=1\n2d\n4e\n*-\n"), (std::vector<std::string>{"0/3", "1/1", "1/3"}));
    CHECK_EQ(positionsOf("**kern\n*M4/4\n8c\n8d\n=1\n1e\n*-\n"), (std::vector<std::string>{"0/4", "0/4+1/2", "1/1"}));
    // Against the meter, not the first measure, which a repeat sign may split.
    CHECK_EQ(positionsOf("**kern\n*M4/4\n4c\n=1\n2d\n=:|!\n2e\n=2\n1f\n*-\n"), (std::vector<std::string>{"0/4", "1/1", "1/3", "2/1"}));
}

TEST_CASE(an_unnumbered_barline_continues_the_measure_before_it) {
    // The quarter after the repeat sign is beat 3 of measure 1, as it is written.
    CHECK_EQ(positionsOf("**kern\n*M3/4\n=1\n2c\n=:|!\n4d\n=2\n4e\n*-\n"), (std::vector<std::string>{"1/1", "1/3", "2/1"}));
}

TEST_CASE(the_part_of_a_beat_is_a_fraction) {
    CHECK_EQ(positionsOf("**kern\n*M4/4\n=1\n8c\n8d\n12e\n12f\n12g\n16a\n16b\n8cc\n4dd\n*-\n"),
             (std::vector<std::string>{"1/1", "1/1+1/2", "1/2", "1/2+1/3", "1/2+2/3", "1/3", "1/3+1/4", "1/3+1/2", "1/4"}));
}

TEST_MAIN()

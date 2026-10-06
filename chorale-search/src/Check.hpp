#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "HumdrumChorale.hpp"
#include "Query.hpp"

namespace choralesearch {

// What a check found in a score.
struct Finding {
    // Which check found it: "parallelFifths" or "parallelOctaves".
    std::string check;

    // "error" for a rule that is broken, "warning" for what only deserves a second look.
    std::string severity = "error";

    // The way both voices of a parallel go: "up" or "down".
    std::string direction;

    // The voices involved, 1 is the bass and 4 the soprano: lowerVoice < upperVoice.
    std::size_t lowerVoice = 0;
    std::size_t upperVoice = 0;

    // Where it is: the two sonorities of a parallel (their intervals are attacked on the same
    // line in both voices). The lines are those of the file the chorale was loaded from.
    int startLine = 0;
    int endLine = 0;
    std::string startPosition;
    std::string endPosition;
};

// The parallel fifths and octaves between any two voices, upwards and downwards: both voices attack
// the same two notes, move the same way and form the same perfect interval each time. A twelfth
// counts as a fifth and a unison as an octave. Found with the search's own queries.
std::vector<Finding> findParallelMotion(const HumdrumChorale& chorale);

// Every check of the checker, run on the chorale: the findings of all of them, in the order of the
// score. A new check is a function like findParallelMotion, added here.
std::vector<Finding> runChecks(const HumdrumChorale& chorale);

} // namespace choralesearch

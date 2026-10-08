#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "HumdrumChorale.hpp"
#include "Query.hpp"
#include "VoiceRanges.hpp"

namespace choralesearch {

// What a check found in a score.
struct Finding {
    // Which check found it: "parallelFifths", "parallelOctaves", "hiddenFifths", "hiddenOctaves" or
    // "voiceRange".
    std::string check;

    // "error" for a rule that is broken, "warning" for what only deserves a second look.
    std::string severity = "error";

    // The way it goes: "up" or "down" for the voices of a parallel or a hidden one, "above" or
    // "below" for the side of its range a note lies on.
    std::string direction;

    // The voices involved, 1 is the bass and 4 the soprano. A finding about a single voice names
    // it twice, a parallel has lowerVoice < upperVoice.
    std::size_t lowerVoice = 0;
    std::size_t upperVoice = 0;

    // Where it is: the two sonorities of a parallel (their intervals are attacked on the same
    // line in both voices), or the one line of a note out of its range, which is both the start
    // and the end. The lines are those of the file the chorale was loaded from.
    int startLine = 0;
    int endLine = 0;
    std::string startPosition;
    std::string endPosition;
};

// The parallel fifths and octaves between any two voices, upwards and downwards: both voices attack
// the same two notes, move the same way and form the same perfect interval each time. A twelfth
// counts as a fifth and a unison as an octave. Found with the search's own queries.
std::vector<Finding> findParallelMotion(const HumdrumChorale& chorale);

// Hidden fifths and octaves between the outer voices, as warnings: bass and soprano move the same way
// into a perfect fifth or octave, the soprano by a leap (a step too, if `allowStepwiseSoprano` is off).
// A fifth or octave that follows the same one is a parallel, reported by findParallelMotion.
std::vector<Finding> findHiddenMotion(const HumdrumChorale& chorale, bool allowStepwiseSoprano = true);

// The notes outside the range of their voice (see VoiceRanges.hpp), as warnings: a range is a
// custom of the voices, not a rule.
std::vector<Finding> findVoiceRangeViolations(const HumdrumChorale& chorale,
                                               VoiceRangeSet ranges = VoiceRangeSet::StraussBerlioz);

// Every check of the checker, run on the chorale: the findings of all of them, in the order of the
// score. A new check is a function like findParallelMotion, added here.
std::vector<Finding> runChecks(const HumdrumChorale& chorale, VoiceRangeSet ranges = VoiceRangeSet::StraussBerlioz,
                               bool allowStepwiseHiddenMotion = true);

} // namespace choralesearch

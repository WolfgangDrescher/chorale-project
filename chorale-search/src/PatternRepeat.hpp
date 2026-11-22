#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include "Query.hpp"
#include "humlib.h"

namespace choralesearch {

// The fewest onsets `pattern` matches, each repeat as rarely as it may; at most the largest size_t, which no voice reaches.
std::size_t minimumOnsets(const std::vector<PatternPosition>& pattern);

// Whether the repeat may take no onset: a gap, where a run that has to last takes one.
bool mayBeEmpty(const PatternRepeat& repeat);

// Whether a repeat of `pattern` may take more or fewer onsets: one without a fixed number of times.
bool hasRepeatWithoutFixedCount(const std::vector<PatternPosition>& pattern);

// The shortest and the longest the onsets of all repeats may last together, in quarter notes: the narrower of
// minDuration and minQuarterNotes, of maxDuration and maxQuarterNotes. nullopt without such a limit.
std::optional<hum::HumNum> shortestDuration(const PatternRepeat& repeat);
std::optional<hum::HumNum> longestDuration(const PatternRepeat& repeat);

// Throws, naming the position, for a repeat that cannot be matched: a max of 0 or below its min, a longest
// duration below its shortest, a subPattern without a repeat or with repeats that match no onset, a pattern
// that matches no onset, and with `notesMaySplitOrMerge` (durationAllowSplitNotes, durationAllowMergedNotes) a
// duration of a position that repeats.
void validateRepeats(const std::vector<PatternPosition>& pattern, bool notesMaySplitOrMerge);

// The path of a repeat that nothing has to follow and that neither is greedy nor has to last: where nothing
// says where a match ends, it would never take more than its min. nullopt without one.
std::optional<std::string> lastRepeatThatStops(const std::vector<PatternPosition>& pattern, const std::string& path = "pattern");

// Throws for the repeat lastRepeatThatStops finds.
void validateLastRepeatGoesOn(const std::vector<PatternPosition>& pattern);

} // namespace choralesearch

#pragma once

#include <cstddef>
#include <vector>

#include "Query.hpp"

namespace choralesearch {

// The fewest onsets `pattern` matches, each repeat as rarely as it may.
std::size_t minimumOnsets(const std::vector<PatternPosition>& pattern);

// Throws, naming the position, for a repeat that cannot be matched: a max of 0 or below its min, a subPattern
// without a repeat or with repeats that match no onset, a pattern that matches no onset, and with
// `notesMaySplitOrMerge` (durationAllowSplitNotes, durationAllowMergedNotes) a duration of a position that repeats.
void validateRepeats(const std::vector<PatternPosition>& pattern, bool notesMaySplitOrMerge);

// Throws for a repeat that nothing has to follow and that is not greedy: where nothing says where a match ends,
// it would never take more than its min.
void validateLastRepeatGoesOn(const std::vector<PatternPosition>& pattern);

} // namespace choralesearch

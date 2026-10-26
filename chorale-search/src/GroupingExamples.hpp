#pragma once

#include <cstddef>
#include <set>
#include <vector>

#include "Grouping.hpp"

namespace choralesearch {

// The passage of a match, and how the voice searched and `voicesReadInOwnRhythm` ornament it there.
GroupExample exampleOf(const FoundMatch& match, const std::set<std::size_t>& voicesReadInOwnRhythm);

// The examples of a group: the unornamented passage first, then one for each set of ornamented
// voices, then passages of further chorales; a chorale at most once.
std::vector<GroupExample> chooseExamples(const std::vector<const MatchReading*>& matches, const ExampleSettings& settings);

// The example of `matches` ornamented in the fewest voices, the first found of them.
const GroupExample& preferredExampleOf(const std::vector<const MatchReading*>& matches);

} // namespace choralesearch

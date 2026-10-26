#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include "Query.hpp"

namespace choralesearch {

// What the reads of a level found at a match, one list per read: {{"-M2", "+m2"}, {"s", "w"}}.
using LevelValues = std::vector<std::vector<std::string>>;

// A passage of the score that shows a group, and which voices it reads are ornamented there.
struct GroupExample {
    std::string choraleId;
    std::size_t startLine = 0;
    std::size_t endLine = 0;
    int firstMeasure = 0;
    int lastMeasure = 0;
    std::vector<std::size_t> ornamentedVoices;
    std::size_t ornamentCount = 0;
    std::string groupLabel; // of the group or member below it stands for
};

// One of the values of a group of several values: "P5" of "perfect", "-P5" of "+P4".
struct GroupMember {
    LevelValues values;
    std::string label;
    std::size_t matchCount = 0;
    std::size_t choraleCount = 0;
    std::optional<Query> query;
};

// The matches that share the values of every level down to this one.
struct Group {
    LevelValues values;
    std::string label; // the values as shown
    std::size_t matchCount = 0;
    std::size_t choraleCount = 0;
    std::vector<GroupMember> members; // where it stands for several values
    std::optional<Query> query;       // finds exactly these matches, where it can be asked for
    std::vector<Group> groups;
    std::size_t omittedMatchCount = 0; // of the groups the next level leaves out
    std::vector<GroupExample> examples;
};

} // namespace choralesearch

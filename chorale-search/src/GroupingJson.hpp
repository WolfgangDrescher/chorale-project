#pragma once

#include <nlohmann/json.hpp>

#include "GroupQuery.hpp"
#include "Group.hpp"

namespace choralesearch {

// Throws std::invalid_argument on malformed input, like queryFromJson.
GroupQuery groupQueryFromJson(const nlohmann::json& j);

// With `withMatches` a group or member no query finds lists its matches, grouped by chorale as the search does.
nlohmann::json groupToJson(const Group& group, bool withMatches = false);

} // namespace choralesearch

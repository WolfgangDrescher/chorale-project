#pragma once

#include <nlohmann/json.hpp>

#include "GroupQuery.hpp"
#include "Group.hpp"

namespace choralesearch {

// Throws std::invalid_argument on malformed input, like queryFromJson.
GroupQuery groupQueryFromJson(const nlohmann::json& j);

nlohmann::json groupToJson(const Group& group);

} // namespace choralesearch

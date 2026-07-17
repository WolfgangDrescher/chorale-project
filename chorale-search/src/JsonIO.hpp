#pragma once

#include <nlohmann/json.hpp>

#include "Query.hpp"
#include "Result.hpp"

namespace choralesearch {

// Throws std::invalid_argument with a human-readable message on malformed input
Query queryFromJson(const nlohmann::json& j);

nlohmann::json resultToJson(const Result& r);
nlohmann::json resultsToJson(const std::vector<Result>& results);

} // namespace choralesearch

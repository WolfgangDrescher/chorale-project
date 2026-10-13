#pragma once

#include <nlohmann/json.hpp>

#include "Query.hpp"
#include "Result.hpp"

namespace choralesearch {

// Throws std::invalid_argument with a human-readable message on malformed input
Query queryFromJson(const nlohmann::json& j);

// The "choraleFilter" object of a query. `context` names where it was found ("Query JSON", "'queries[2]'") for
// the message of the std::invalid_argument it throws on anything it does not accept: an unknown field, a value
// of the wrong type, an unreadable id, an empty list.
ChoraleFilter choraleFilterFromJson(const nlohmann::json& j, const std::string& context);

// j must be a JSON array; each element is parsed the same way as queryFromJson, plus an
// optional "id" field (a string) tagging that query's own results -- see Query::id.
// Throws std::invalid_argument with a human-readable message on malformed input.
std::vector<Query> queryArrayFromJson(const nlohmann::json& j);

nlohmann::json resultToJson(const Result& r);
nlohmann::json resultsToJson(const Results& results);
nlohmann::json resultsGroupedByChoraleToJson(const Results& results);

} // namespace choralesearch

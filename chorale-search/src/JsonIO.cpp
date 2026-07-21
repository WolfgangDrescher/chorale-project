#include "JsonIO.hpp"

#include <stdexcept>

namespace choralesearch {

namespace {

std::vector<std::string> attributeValueFromJson(const nlohmann::json& v, const std::string& context) {
    if (v.is_string()) return {v.get<std::string>()};
    if (v.is_boolean()) return {v.get<bool>() ? "true" : "false"};
    if (v.is_array()) {
        std::vector<std::string> out;
        for (const auto& entry : v) {
            if (!entry.is_string()) throw std::invalid_argument(context + ": array entries must be strings");
            out.push_back(entry.get<std::string>());
        }
        if (out.empty()) throw std::invalid_argument(context + ": OR-list must not be empty");
        return out;
    }
    throw std::invalid_argument(context + ": must be a string, boolean, or an array of strings");
}

std::vector<AttributeMap> patternFromJson(const nlohmann::json& j) {
    if (!j.is_array()) throw std::invalid_argument("'pattern' must be an array");
    std::vector<AttributeMap> result;
    for (const auto& posJson : j) {
        if (!posJson.is_object()) throw std::invalid_argument("'pattern' entries must be objects");
        AttributeMap pos;
        for (auto it = posJson.begin(); it != posJson.end(); ++it) {
            pos[it.key()] = attributeValueFromJson(it.value(), "'pattern[...][" + it.key() + "]'");
        }
        result.push_back(std::move(pos));
    }
    if (result.empty()) throw std::invalid_argument("'pattern' must not be empty");
    return result;
}

} // namespace

Query queryFromJson(const nlohmann::json& j) {
    Query q;

    if (!j.contains("feature") || !j["feature"].is_string()) {
        throw std::invalid_argument("Query JSON must contain a string field 'feature'");
    }
    q.feature = j["feature"].get<std::string>();

    if (j.contains("voices") && j["voices"].is_string()) {
        q.voices = j["voices"].get<std::string>();
    }

    if (!j.contains("pattern")) {
        throw std::invalid_argument("Query JSON must contain a 'pattern' array");
    }
    q.pattern = patternFromJson(j["pattern"]);

    if (j.contains("limit") && j["limit"].is_number_unsigned()) {
        q.limit = j["limit"].get<std::size_t>();
    }

    return q;
}

nlohmann::json resultToJson(const Result& r) {
    nlohmann::json j;
    j["chorale"] = r.choraleId;
    j["feature"] = r.feature;
    j["voiceLabel"] = r.voiceLabel;
    j["voice"] = r.voice;
    j["startPosition"] = r.startPosition;
    j["endPosition"] = r.endPosition;
    j["startLine"] = r.startLineNumber;
    j["endLine"] = r.endLineNumber;
    return j;
}

nlohmann::json resultsToJson(const Results& results) {
    nlohmann::json arr = nlohmann::json::array();
    for (const auto& r : results) arr.push_back(resultToJson(r));
    return arr;
}

nlohmann::json resultsGroupedByChoraleToJson(const Results& results) {
    nlohmann::json obj = nlohmann::json::object();
    for (const auto& r : results) obj[r.choraleId].push_back(resultToJson(r));
    return obj;
}

} // namespace choralesearch

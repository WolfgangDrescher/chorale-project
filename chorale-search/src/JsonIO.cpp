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

std::string simultaneousAlignmentFromJson(const nlohmann::json& j, const std::string& context) {
    if (!j.is_string()) throw std::invalid_argument("'" + context + "' must be a string");
    std::string alignment = j.get<std::string>();
    if (alignment != "start" && alignment != "end" && alignment != "start-end") {
        throw std::invalid_argument("'" + context + "' must be one of 'start', 'end', 'start-end' (got '" +
                                     alignment + "')");
    }
    return alignment;
}

// Fields shared by Query and SimultaneousGroup, parsed once for both. Assignment works
// whether target's members are plain values (Query) or std::optional (SimultaneousGroup,
// nullopt = inherit from the top-level query -- see Query.hpp).
template <typename T>
void parseSearchRequestFields(const nlohmann::json& j, T& target, const std::string& context) {
    if (!j.contains("feature") || !j["feature"].is_string()) {
        throw std::invalid_argument(context + " must contain a string field 'feature'");
    }
    target.feature = j["feature"].get<std::string>();

    if (j.contains("voices") && j["voices"].is_string()) {
        target.voices = j["voices"].get<std::string>();
    }

    if (!j.contains("pattern")) {
        throw std::invalid_argument(context + " must contain a 'pattern' array");
    }
    target.pattern = patternFromJson(j["pattern"]);

    if (j.contains("mintStartAtPreviousToken") && j["mintStartAtPreviousToken"].is_boolean()) {
        target.mintStartAtPreviousToken = j["mintStartAtPreviousToken"].get<bool>();
    }

    if (j.contains("fbCompareExactChord") && j["fbCompareExactChord"].is_boolean()) {
        target.fbCompareExactChord = j["fbCompareExactChord"].get<bool>();
    }

    if (j.contains("kernIgnoreOctave") && j["kernIgnoreOctave"].is_boolean()) {
        target.kernIgnoreOctave = j["kernIgnoreOctave"].get<bool>();
    }

    if (j.contains("hintReduceCompound") && j["hintReduceCompound"].is_boolean()) {
        target.hintReduceCompound = j["hintReduceCompound"].get<bool>();
    }

    if (j.contains("simultaneousAlignment")) {
        target.simultaneousAlignment = simultaneousAlignmentFromJson(j["simultaneousAlignment"], "simultaneousAlignment");
    }
}

SimultaneousGroup simultaneousGroupFromJson(const nlohmann::json& j, std::size_t index) {
    std::string context = "'simultaneousWith[" + std::to_string(index) + "]'";
    if (!j.is_object()) throw std::invalid_argument(context + " must be an object");

    SimultaneousGroup group;
    parseSearchRequestFields(j, group, context);
    return group;
}

std::vector<SimultaneousGroup> simultaneousWithFromJson(const nlohmann::json& j) {
    if (!j.is_array()) throw std::invalid_argument("'simultaneousWith' must be an array");
    std::vector<SimultaneousGroup> groups;
    for (std::size_t i = 0; i < j.size(); ++i) groups.push_back(simultaneousGroupFromJson(j[i], i));
    return groups;
}

} // namespace

Query queryFromJson(const nlohmann::json& j) {
    Query q;
    parseSearchRequestFields(j, q, "Query JSON");

    if (j.contains("limit") && j["limit"].is_number_unsigned()) {
        q.limit = j["limit"].get<std::size_t>();
    }

    if (j.contains("simultaneousWith")) {
        q.simultaneousWith = simultaneousWithFromJson(j["simultaneousWith"]);
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

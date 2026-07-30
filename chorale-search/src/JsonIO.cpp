#include "JsonIO.hpp"
#include "QueryValidation.hpp"
#include "VoiceMap.hpp"

#include <stdexcept>

namespace choralesearch {

namespace {

// A typo'd field name (e.g. "hintReduceCompund") would otherwise be silently ignored, quietly
// falling back to that option's default and making a malformed query look identical to a
// correct one that simply matched nothing. Rejecting anything outside the known set up front
// turns that into an immediate, specific error instead.
void rejectUnknownKeys(const nlohmann::json& j, bool (*isKnown)(const std::string&), const std::string& context) {
    for (auto it = j.begin(); it != j.end(); ++it) {
        if (!isKnown(it.key())) {
            throw std::invalid_argument(context + " has an unknown field '" + it.key() + "'");
        }
    }
}

std::vector<std::string> attributeValueFromJson(const nlohmann::json& v, const std::string& key, const std::string& context) {
    std::vector<std::string> values;
    if (v.is_string()) {
        values = {v.get<std::string>()};
    } else if (v.is_boolean()) {
        values = {v.get<bool>() ? "true" : "false"};
    } else if (v.is_array()) {
        for (const auto& entry : v) {
            if (!entry.is_string()) throw std::invalid_argument(context + ": array entries must be strings");
            values.push_back(entry.get<std::string>());
        }
        if (values.empty()) throw std::invalid_argument(context + ": OR-list must not be empty");
    } else {
        throw std::invalid_argument(context + ": must be a string, boolean, or an array of strings");
    }

    for (const std::string& value : values) {
        if (value == "*") continue; // universal wildcard, valid for any key
        if (value.empty() || !isValidPatternValue(key, value)) {
            throw std::invalid_argument(context + ": '" + value + "' is not a valid '" + key + "' value");
        }
    }
    return values;
}

std::vector<AttributeMap> patternFromJson(const nlohmann::json& j) {
    if (!j.is_array()) throw std::invalid_argument("'pattern' must be an array");
    std::vector<AttributeMap> result;
    for (const auto& posJson : j) {
        if (!posJson.is_object()) throw std::invalid_argument("'pattern' entries must be objects");
        AttributeMap pos;
        for (auto it = posJson.begin(); it != posJson.end(); ++it) {
            const std::string& rawKey = it.key();
            // isKnownPatternKey/isValidPatternValue both accept a "!"-negated key directly (see
            // docs/patterns#negating-a-feature) -- negation doesn't change what's known or valid.
            if (!isKnownPatternKey(rawKey)) {
                throw std::invalid_argument("'pattern' has an unknown key '" + rawKey + "'");
            }
            pos[rawKey] = attributeValueFromJson(it.value(), rawKey, "'pattern[...][" + rawKey + "]'");
        }
        result.push_back(std::move(pos));
    }
    if (result.empty()) throw std::invalid_argument("'pattern' must not be empty");
    return result;
}

// The diatonic numbers "mintAllowIntervalComplementation" opts in: a single value or an array
// of them, each either a string ("5") or a plain JSON number (5), since writing an interval
// size as a number is the obvious thing to reach for and stringifying it here costs nothing.
std::vector<std::string> mintComplementationFromJson(const nlohmann::json& j, const std::string& context) {
    auto entryToString = [&](const nlohmann::json& entry) {
        if (entry.is_string()) return entry.get<std::string>();
        if (entry.is_number_unsigned()) return std::to_string(entry.get<std::size_t>());
        throw std::invalid_argument("'mintAllowIntervalComplementation' in " + context +
                                     " must hold strings or numbers");
    };

    std::vector<std::string> values;
    if (j.is_array()) {
        for (const auto& entry : j) values.push_back(entryToString(entry));
        if (values.empty()) {
            throw std::invalid_argument("'mintAllowIntervalComplementation' in " + context +
                                         " must not be an empty array (omit it to switch complementation off)");
        }
    } else {
        values.push_back(entryToString(j));
    }

    for (const std::string& value : values) {
        if (!isValidMintComplementationValue(value)) {
            throw std::invalid_argument("'mintAllowIntervalComplementation' in " + context + ": '" + value +
                                         "' is not a diatonic number 1-8 (or '*')");
        }
    }
    return values;
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
    if (!isKnownDrivingFeature(target.feature)) {
        const auto& names = drivingFeatureNames();
        std::string joined;
        for (std::size_t i = 0; i < names.size(); ++i) joined += (i ? ", " : "") + names[i];
        throw std::invalid_argument("'feature' in " + context + " must be one of " + joined +
                                     " (got '" + target.feature + "')");
    }

    if (j.contains("voices")) {
        if (!j["voices"].is_string()) throw std::invalid_argument("'voices' in " + context + " must be a string");
        target.voices = j["voices"].get<std::string>();
        try {
            resolveVoices(target.voices);
        } catch (const std::exception& e) {
            throw std::invalid_argument("'voices' in " + context + " is invalid: " + e.what());
        }
    }

    if (!j.contains("pattern")) {
        throw std::invalid_argument(context + " must contain a 'pattern' array");
    }
    target.pattern = patternFromJson(j["pattern"]);

    if (j.contains("mintStartAtPreviousToken")) {
        if (!j["mintStartAtPreviousToken"].is_boolean()) {
            throw std::invalid_argument("'mintStartAtPreviousToken' in " + context + " must be a boolean");
        }
        // The shift is only meaningful while walking mint's own onsets, so switching it on for
        // any other driving feature would silently do nothing -- a mistake worth naming instead
        // of a query that quietly reports the starts the author didn't ask for. Explicitly
        // switching it *off* stays legal everywhere.
        if (j["mintStartAtPreviousToken"].get<bool>() && target.feature != "mint") {
            throw std::invalid_argument("'mintStartAtPreviousToken' in " + context +
                                         " can only be true when 'feature' is 'mint' (got '" + target.feature + "')");
        }
        target.mintStartAtPreviousToken = j["mintStartAtPreviousToken"].get<bool>();
    }

    if (j.contains("mintAllowIntervalComplementation")) {
        target.mintAllowIntervalComplementation =
            mintComplementationFromJson(j["mintAllowIntervalComplementation"], context);
    }

    if (j.contains("fbCompareExactChord")) {
        if (!j["fbCompareExactChord"].is_boolean()) {
            throw std::invalid_argument("'fbCompareExactChord' in " + context + " must be a boolean");
        }
        target.fbCompareExactChord = j["fbCompareExactChord"].get<bool>();
    }

    if (j.contains("kernIgnoreOctave")) {
        if (!j["kernIgnoreOctave"].is_boolean()) {
            throw std::invalid_argument("'kernIgnoreOctave' in " + context + " must be a boolean");
        }
        target.kernIgnoreOctave = j["kernIgnoreOctave"].get<bool>();
    }

    if (j.contains("hintReduceCompound")) {
        if (!j["hintReduceCompound"].is_boolean()) {
            throw std::invalid_argument("'hintReduceCompound' in " + context + " must be a boolean");
        }
        target.hintReduceCompound = j["hintReduceCompound"].get<bool>();
    }

    if (j.contains("durationAllowSplitNotes")) {
        if (!j["durationAllowSplitNotes"].is_boolean()) {
            throw std::invalid_argument("'durationAllowSplitNotes' in " + context + " must be a boolean");
        }
        target.durationAllowSplitNotes = j["durationAllowSplitNotes"].get<bool>();
    }

    if (j.contains("simultaneousAlignment")) {
        target.simultaneousAlignment = simultaneousAlignmentFromJson(j["simultaneousAlignment"], "simultaneousAlignment");
    }
}

SimultaneousGroup simultaneousGroupFromJson(const nlohmann::json& j, std::size_t index) {
    std::string context = "'simultaneousWith[" + std::to_string(index) + "]'";
    if (!j.is_object()) throw std::invalid_argument(context + " must be an object");
    rejectUnknownKeys(j, isKnownSimultaneousGroupKey, context);

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

// Internal overload (not declared in the header) shared by the single-query and
// array-of-queries entrypoints below, so the error message can name which array element
// failed ("'queries[2]'") instead of always saying "Query JSON".
Query queryFromJson(const nlohmann::json& j, const std::string& context) {
    if (!j.is_object()) throw std::invalid_argument(context + " must be an object");
    rejectUnknownKeys(j, isKnownQueryKey, context);

    Query q;
    parseSearchRequestFields(j, q, context);

    if (j.contains("id")) {
        if (!j["id"].is_string()) throw std::invalid_argument("'id' in " + context + " must be a string");
        q.id = j["id"].get<std::string>();
    }

    if (j.contains("limit")) {
        if (!j["limit"].is_number_unsigned()) {
            throw std::invalid_argument("'limit' in " + context + " must be a non-negative integer");
        }
        q.limit = j["limit"].get<std::size_t>();
    }

    if (j.contains("simultaneousWith")) {
        q.simultaneousWith = simultaneousWithFromJson(j["simultaneousWith"]);
    }

    return q;
}

} // namespace

Query queryFromJson(const nlohmann::json& j) {
    return queryFromJson(j, "Query JSON");
}

std::vector<Query> queryArrayFromJson(const nlohmann::json& j) {
    if (!j.is_array()) throw std::invalid_argument("Query JSON array must be an array");
    if (j.empty()) throw std::invalid_argument("Query JSON array must not be empty");

    std::vector<Query> queries;
    for (std::size_t i = 0; i < j.size(); ++i) {
        queries.push_back(queryFromJson(j[i], "'queries[" + std::to_string(i) + "]'"));
    }
    return queries;
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
    if (r.queryId) j["queryId"] = *r.queryId;
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

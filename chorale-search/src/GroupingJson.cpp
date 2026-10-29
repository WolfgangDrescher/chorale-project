#include "GroupingJson.hpp"

#include <algorithm>
#include <map>
#include <set>
#include <stdexcept>
#include <utility>

#include "Grouping.hpp"
#include "HintPairs.hpp"
#include "JsonIO.hpp"
#include "VoiceMap.hpp"

namespace choralesearch {

namespace {

// What a field is called in an error message: "'reads' in 'groupBy[0]'".
std::string fieldLocation(const std::string& field, const std::string& context) {
    return "'" + field + "' in " + context;
}

// False where the field is left out.
bool booleanFromJson(const nlohmann::json& j, const char* field, const std::string& context) {
    if (!j.contains(field)) return false;
    if (!j[field].is_boolean()) throw std::invalid_argument(fieldLocation(field, context) + " must be a boolean");
    return j[field].get<bool>();
}

std::size_t countFromJson(const nlohmann::json& value, const std::string& where) {
    if (!value.is_number_unsigned()) throw std::invalid_argument(where + " must be a non-negative integer");
    return value.get<std::size_t>();
}

std::vector<std::size_t> positionsFromJson(const nlohmann::json& j, std::size_t positionCount, const std::string& where) {
    if (!j.is_array() || j.empty()) throw std::invalid_argument(where + " must be a non-empty array");
    std::vector<std::size_t> positions;
    for (const auto& position : j) {
        positions.push_back(countFromJson(position, where));
        if (positions.back() >= positionCount) {
            throw std::invalid_argument(where + " must be positions of the pattern, from 0 to " + std::to_string(positionCount - 1));
        }
    }
    return positions;
}

// {"perfect": ["P1", "P5", "P8"], "dissonant": "remaining"}
Categories categoriesFromJson(const nlohmann::json& j, const std::string& context) {
    if (!j.is_object() || j.empty()) throw std::invalid_argument(context + " must be a non-empty object");
    Categories categories;
    std::set<std::string> listed;
    for (const auto& category : j.items()) {
        const std::string& name = category.key();
        const nlohmann::json& values = category.value();
        if (values.is_string()) {
            if (values.get<std::string>() != "remaining") {
                throw std::invalid_argument("category '" + name + "' in " + context + " must be a list of values, or the string " +
                                            "\"remaining\" for every value in no list, got \"" + values.get<std::string>() + "\"");
            }
            if (categories.remainingName) {
                throw std::invalid_argument(context + " has two remaining categories, '" + *categories.remainingName + "' and '" + name + "'");
            }
            categories.remainingName = name;
            continue;
        }
        const bool list = values.is_array() && !values.empty() &&
                          std::all_of(values.begin(), values.end(), [](const auto& value) { return value.is_string(); });
        if (!list) throw std::invalid_argument("category '" + name + "' in " + context + " must be a non-empty list of values");
        for (const auto& value : values) {
            if (!listed.insert(value.get<std::string>()).second) {
                throw std::invalid_argument("'" + value.get<std::string>() + "' in " + context + " is in two categories");
            }
            categories.valuesByName[name].push_back(value.get<std::string>());
        }
    }
    return categories;
}

FeatureRead featureReadFromJson(const nlohmann::json& j, std::size_t positionCount, const std::string& context) {
    rejectUnknownKeys(j, {"feature", "positions", "notesOfVoice", "ignoreQuality", "mintAllowIntervalComplementation", "categories"}, context);
    FeatureRead read;
    if (!j.contains("feature") || !j["feature"].is_string()) throw std::invalid_argument(fieldLocation("feature", context) + " must be a string");
    read.feature = j["feature"].get<std::string>();
    const bool knownFeature = read.feature == "deg" || read.feature == "mint" || read.feature == "metweight" ||
                              read.feature == "duration" || isHintPairFeature(read.feature);
    if (!knownFeature) {
        throw std::invalid_argument(fieldLocation("feature", context) + " must be deg, mint, metweight, duration or a hint pair, got '" +
                                    read.feature + "'");
    }
    if (j.contains("positions")) read.positions = positionsFromJson(j["positions"], positionCount, fieldLocation("positions", context));
    if (j.contains("notesOfVoice")) {
        const std::string where = fieldLocation("notesOfVoice", context);
        const std::vector<std::size_t> voices =
            j["notesOfVoice"].is_string() ? resolveVoices(j["notesOfVoice"].get<std::string>()) : std::vector<std::size_t>{};
        if (voices.size() != 1) throw std::invalid_argument(where + " must name a single voice");
        if (read.feature != "mint" || !read.positions.empty()) throw std::invalid_argument(where + " reads the feature mint, and takes no 'positions'");
        read.notesOfVoice = voices.front();
    }
    read.ignoreQuality = booleanFromJson(j, "ignoreQuality", context);
    read.mintAllowIntervalComplementation = booleanFromJson(j, "mintAllowIntervalComplementation", context);
    if (read.mintAllowIntervalComplementation && !read.notesOfVoice) {
        throw std::invalid_argument(fieldLocation("mintAllowIntervalComplementation", context) + " needs 'notesOfVoice'");
    }
    if (j.contains("categories")) read.categories = categoriesFromJson(j["categories"], fieldLocation("categories", context));
    return read;
}

GroupingLevel groupingLevelFromJson(const nlohmann::json& j, std::size_t positionCount, const std::string& context) {
    rejectUnknownKeys(j, {"reads", "maxGroupCount", "minMatchCount"}, context);
    if (!j.contains("reads") || !j["reads"].is_array() || j["reads"].empty()) {
        throw std::invalid_argument(fieldLocation("reads", context) + " must be a non-empty array");
    }
    GroupingLevel level;
    for (std::size_t i = 0; i < j["reads"].size(); ++i) {
        level.reads.push_back(featureReadFromJson(j["reads"][i], positionCount, "'reads[" + std::to_string(i) + "]' of " + context));
    }
    if (j.contains("maxGroupCount")) level.maxGroupCount = countFromJson(j["maxGroupCount"], fieldLocation("maxGroupCount", context));
    if (j.contains("minMatchCount")) level.minMatchCount = countFromJson(j["minMatchCount"], fieldLocation("minMatchCount", context));
    return level;
}

// {"maxCount": 5}, or {} for one example for each group or member below
ExampleSettings exampleSettingsFromJson(const nlohmann::json& j, const std::string& context) {
    rejectUnknownKeys(j, {"maxCount"}, context);
    ExampleSettings settings;
    if (j.contains("maxCount")) settings.maxCount = countFromJson(j["maxCount"], fieldLocation("maxCount", context));
    return settings;
}

// {"ignoreQuality": true, "pattern": [{...}, ...]}
GroupQueryVariant groupQueryVariantFromJson(const std::string& name, const nlohmann::json& j, std::size_t positionCount) {
    const std::string context = "variant '" + name + "' in GroupQuery JSON";
    rejectUnknownKeys(j, {"ignoreQuality", "pattern"}, context);
    GroupQueryVariant variant;
    variant.name = name;
    variant.ignoreQuality = booleanFromJson(j, "ignoreQuality", context);
    if (j.contains("pattern")) {
        const nlohmann::json& pattern = j["pattern"];
        const bool positions = pattern.is_array() && std::all_of(pattern.begin(), pattern.end(), [](const auto& p) { return p.is_object(); });
        if (!positions || pattern.size() > positionCount) {
            throw std::invalid_argument(fieldLocation("pattern", context) + " must be an array of objects, one for each position of the " +
                                        "pattern of the query at most (" + std::to_string(positionCount) + ")");
        }
        variant.pattern = pattern;
    }
    return variant;
}

} // namespace

GroupQuery groupQueryFromJson(const nlohmann::json& j) {
    const std::string context = "group query JSON";
    rejectUnknownKeys(j, {"query", "groupBy", "examples", "variants"}, context);
    if (!j.contains("query")) throw std::invalid_argument(context + " needs a 'query'");
    if (!j.contains("groupBy") || !j["groupBy"].is_array() || j["groupBy"].empty()) {
        throw std::invalid_argument(fieldLocation("groupBy", context) + " must be a non-empty array");
    }
    GroupQuery groupQuery;
    groupQuery.query = queryFromJson(j["query"]);
    if (isHintPairWildcardKey(groupQuery.query.feature)) {
        throw std::invalid_argument("the query of a group query cannot be driven by a set of voice pairs");
    }
    const std::size_t positionCount = groupQuery.query.pattern.size();
    for (std::size_t i = 0; i < j["groupBy"].size(); ++i) {
        groupQuery.levels.push_back(groupingLevelFromJson(j["groupBy"][i], positionCount, "'groupBy[" + std::to_string(i) + "]'"));
    }
    if (j.contains("examples")) groupQuery.examples = exampleSettingsFromJson(j["examples"], fieldLocation("examples", context));
    if (j.contains("variants")) {
        if (!j["variants"].is_object()) throw std::invalid_argument(fieldLocation("variants", context) + " must be an object");
        for (const auto& [name, value] : j["variants"].items()) {
            GroupQueryVariant variant = groupQueryVariantFromJson(name, value, positionCount);
            applyVariant(groupQuery, variant); // rejects a pattern that makes no valid query
            groupQuery.variants.push_back(std::move(variant));
        }
    }
    return groupQuery;
}

// The keys are those the webapp reads.
nlohmann::json groupToJson(const Group& group) {
    nlohmann::json j;
    if (!group.values.empty()) {
        j["values"] = group.values;
        j["label"] = group.label;
    }
    j["matchCount"] = group.matchCount;
    j["choraleCount"] = group.choraleCount;
    j["query"] = group.query ? queryToJson(*group.query) : nlohmann::json(nullptr);
    if (!group.members.empty()) {
        // The queries of the members together find the matches of the group.
        nlohmann::json members = nlohmann::json::array();
        nlohmann::json queries = nlohmann::json::array();
        for (const GroupMember& member : group.members) {
            const nlohmann::json query = member.query ? queryToJson(*member.query) : nlohmann::json(nullptr);
            members.push_back({{"values", member.values},
                                 {"label", member.label},
                                 {"matchCount", member.matchCount},
                                 {"choraleCount", member.choraleCount},
                                 {"query", query}});
            queries.push_back(query);
        }
        j["members"] = std::move(members);
        const bool everyMemberAsked = std::none_of(queries.begin(), queries.end(), [](const nlohmann::json& query) { return query.is_null(); });
        if (everyMemberAsked) j["query"] = std::move(queries);
    }
    if (!group.examples.empty()) {
        nlohmann::json examples = nlohmann::json::array();
        for (const GroupExample& example : group.examples) {
            examples.push_back({{"choraleId", example.choraleId},
                                {"startLine", example.startLine},
                                {"endLine", example.endLine},
                                {"measures", {example.firstMeasure, example.lastMeasure}},
                                {"ornamentedVoices", example.ornamentedVoices},
                                {"ornamentCount", example.ornamentCount}});
            if (!example.groupLabel.empty()) examples.back()["groupLabel"] = example.groupLabel;
        }
        j["examples"] = std::move(examples);
    }
    if (!group.groups.empty() || group.omittedMatchCount) {
        nlohmann::json groups = nlohmann::json::array();
        for (const Group& groupBelow : group.groups) groups.push_back(groupToJson(groupBelow));
        j["groups"] = std::move(groups);
        j["omittedMatchCount"] = group.omittedMatchCount;
    }
    return j;
}

} // namespace choralesearch

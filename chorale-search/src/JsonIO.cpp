#include "JsonIO.hpp"
#include "QueryValidation.hpp"
#include "VoiceMap.hpp"
#include "VoiceReference.hpp"

#include <algorithm>
#include <cctype>
#include <optional>
#include <regex>
#include <stdexcept>
#include <utility>
#include <vector>

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

// Every way a query may spell an order comparison: the name of the constraint, its abbreviation, or
// the symbol. "equalTo" is not among them, an equal value is the plain string.
const std::vector<std::pair<std::vector<std::string>, ComparisonOperator>> kComparisonOperatorSpellings = {
    {{"greaterThan", "gt", ">"}, ComparisonOperator::GreaterThan},
    {{"greaterThanOrEqual", "gte", ">="}, ComparisonOperator::GreaterThanOrEqual},
    {{"lessThan", "lt", "<"}, ComparisonOperator::LessThan},
    {{"lessThanOrEqual", "lte", "<="}, ComparisonOperator::LessThanOrEqual},
};

std::optional<ComparisonOperator> parseComparisonOperator(const std::string& spelling) {
    for (const auto& [spellings, comparisonOperator] : kComparisonOperatorSpellings) {
        if (std::find(spellings.begin(), spellings.end(), spelling) != spellings.end()) return comparisonOperator;
    }
    return std::nullopt;
}

// A value that names another voice ("@2", "@below") instead of stating one. Only the keys whose value
// can be read off the other voice's note take one, and the name has to be one of the known spellings.
void checkVoiceReference(const std::string& key, const std::string& text, const std::string& context) {
    if (!supportsVoiceReference(key)) {
        throw std::invalid_argument(context + ": '" + key + "' cannot be held against another voice with '" + text +
                                     "', only kern and duration can");
    }
    if (!isValidVoiceReference(text)) {
        throw std::invalid_argument(context + ": '" + text + "' is not a voice reference (@1 to @4, @below, @above, "
                                              "@anyBelow, @allBelow, @anyAbove, @allAbove, @anyOther, @allOther)");
    }
}

// {"operator": "gt", "value": "4"}: one order comparison, which is one entry of the OR-list like
// any string. Anything else in the object, or a value that is not a plain string (a list of them
// would only ever mean its smallest or largest, write one entry per comparison instead), is an
// error rather than something to guess at.
PatternValue comparisonFromJson(const nlohmann::json& v, const std::string& key, const std::string& context) {
    for (auto it = v.begin(); it != v.end(); ++it) {
        if (it.key() != "operator" && it.key() != "value") {
            throw std::invalid_argument(context + ": a comparison has an unknown field '" + it.key() +
                                         "' (only 'operator' and 'value')");
        }
    }
    if (!v.contains("operator") || !v["operator"].is_string()) {
        throw std::invalid_argument(context + ": a comparison needs a string field 'operator'");
    }
    if (!v.contains("value")) throw std::invalid_argument(context + ": a comparison needs a field 'value'");
    if (v["value"].is_array()) {
        throw std::invalid_argument(context + ": the 'value' of a comparison must be a single string, not an array "
                                              "(list one comparison per entry instead)");
    }
    if (!v["value"].is_string()) throw std::invalid_argument(context + ": the 'value' of a comparison must be a string");

    const std::string spelling = v["operator"].get<std::string>();
    const std::string text = v["value"].get<std::string>();
    std::optional<ComparisonOperator> comparisonOperator = parseComparisonOperator(spelling);
    if (!comparisonOperator) {
        throw std::invalid_argument(context + ": 'operator' must be one of greaterThan (gt, >), greaterThanOrEqual "
                                              "(gte, >=), lessThan (lt, <), lessThanOrEqual (lte, <=), got '" + spelling + "'");
    }

    if (!supportsComparison(key)) {
        throw std::invalid_argument(context + ": '" + key + "' cannot be compared with '" + spelling +
                                     "', only kern, duration, mint and the hint keys can");
    }
    if (PatternValue(text).isVoiceReference()) {
        checkVoiceReference(key, text, context);
        return PatternValue(*comparisonOperator, text);
    }
    if (!isValidComparisonValue(key, text)) {
        throw std::invalid_argument(context + ": '" + text + "' is not a valid value to compare '" + key + "' with");
    }
    return PatternValue(*comparisonOperator, text);
}

std::vector<PatternValue> attributeValueFromJson(const nlohmann::json& v, const std::string& key, const std::string& context) {
    std::vector<PatternValue> values;
    if (v.is_string()) {
        values = {PatternValue(v.get<std::string>())};
    } else if (v.is_boolean()) {
        values = {PatternValue(v.get<bool>() ? "true" : "false")};
    } else if (v.is_object()) {
        values.push_back(comparisonFromJson(v, key, context));
    } else if (v.is_array()) {
        for (const auto& entry : v) {
            if (entry.is_string()) values.emplace_back(entry.get<std::string>());
            else if (entry.is_object()) values.push_back(comparisonFromJson(entry, key, context));
            else throw std::invalid_argument(context + ": array entries must be strings or comparison objects");
        }
        if (values.empty()) throw std::invalid_argument(context + ": OR-list must not be empty");
    } else {
        throw std::invalid_argument(context + ": must be a string, boolean, comparison object, or an array of them");
    }

    for (const PatternValue& value : values) {
        if (value.isVoiceReference()) {
            checkVoiceReference(key, value.text, context);
            continue;
        }
        if (value.hasRelationalOperator()) continue; // already checked above
        if (value.text == "*") continue; // universal wildcard, valid for any key
        if (value.text.empty() || !isValidPatternValue(key, value.text)) {
            throw std::invalid_argument(context + ": '" + value.text + "' is not a valid '" + key + "' value");
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

    if (j.contains("durationAllowMergedNotes")) {
        if (!j["durationAllowMergedNotes"].is_boolean()) {
            throw std::invalid_argument("'durationAllowMergedNotes' in " + context + " must be a boolean");
        }
        target.durationAllowMergedNotes = j["durationAllowMergedNotes"].get<bool>();
    }

    if (j.contains("metweightSkipUnclassified")) {
        if (!j["metweightSkipUnclassified"].is_boolean()) {
            throw std::invalid_argument("'metweightSkipUnclassified' in " + context + " must be a boolean");
        }
        target.metweightSkipUnclassified = j["metweightSkipUnclassified"].get<bool>();
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

// What a field is called in an error message: "'choraleFilter.title' in Query JSON".
std::string choraleFilterFieldLocation(const std::string& field, const std::string& context) {
    return "'choraleFilter." + field + "' in " + context;
}

// The number one end of an id range names: "29", "029" or "chor029". At most nine digits, so the number
// always fits in a size_t. Anything else, a space included, is no number.
std::optional<std::size_t> integerFromIdText(const std::string& text) {
    std::string digits = text;
    if (digits.size() >= 4) {
        std::string prefix = digits.substr(0, 4);
        std::transform(prefix.begin(), prefix.end(), prefix.begin(), [](unsigned char c) { return std::tolower(c); });
        if (prefix == "chor") digits = digits.substr(4);
    }
    if (digits.empty() || digits.size() > 9 || digits.find_first_not_of("0123456789") != std::string::npos) {
        return std::nullopt;
    }
    return static_cast<std::size_t>(std::stoul(digits));
}

ChoraleIdRange idRangeFromJson(const nlohmann::json& entry, const std::string& field, const std::string& context) {
    const std::string where = choraleFilterFieldLocation(field, context);
    ChoraleIdRange range;

    if (entry.is_number_unsigned()) {
        range.first = range.last = entry.get<std::size_t>();
        range.text = std::to_string(range.first);
        if (range.text.size() > 9) throw std::invalid_argument(where + ": " + range.text + " is not a chorale number");
        return range;
    }
    if (!entry.is_string()) {
        throw std::invalid_argument(where + " must hold chorale numbers (29) or strings (\"029\", \"chor029\", \"5-20\"), "
                                            "got " + entry.dump());
    }

    range.text = entry.get<std::string>();
    const std::size_t dash = range.text.find('-');
    const std::optional<std::size_t> first = integerFromIdText(range.text.substr(0, dash));
    const std::optional<std::size_t> last = dash == std::string::npos ? first : integerFromIdText(range.text.substr(dash + 1));
    if (!first || !last || (dash != std::string::npos && range.text.find('-', dash + 1) != std::string::npos)) {
        throw std::invalid_argument(where + ": '" + range.text + "' is not a chorale id (29, \"029\", \"chor029\") or a range "
                                            "of them (\"5-20\", \"chor005-chor020\")");
    }
    if (*first > *last) {
        throw std::invalid_argument(where + ": the range '" + range.text + "' ends before it starts");
    }
    range.first = *first;
    range.last = *last;
    return range;
}

// A field that takes a single entry or an array of them, read into a vector either way. `readEntry` reads and
// checks one entry. An empty array is an error, so a field that is set always holds at least one entry.
template <typename T, typename ReadEntry>
std::vector<T> listFromJson(const nlohmann::json& j, const std::string& field, const std::string& context,
                            ReadEntry readEntry) {
    std::vector<T> entries;
    if (j.is_array()) {
        for (const auto& entry : j) entries.push_back(readEntry(entry, field, context));
        if (entries.empty()) {
            throw std::invalid_argument(choraleFilterFieldLocation(field, context) +
                                        " must not be an empty array (omit it to leave the chorales unrestricted)");
        }
    } else {
        entries.push_back(readEntry(j, field, context));
    }
    return entries;
}

std::string nonEmptyStringFromJson(const nlohmann::json& entry, const std::string& field, const std::string& context) {
    if (!entry.is_string() || entry.get<std::string>().empty()) {
        throw std::invalid_argument(choraleFilterFieldLocation(field, context) + ": " + entry.dump() + " is not a non-empty string");
    }
    return entry.get<std::string>();
}

std::string meterFromJson(const nlohmann::json& entry, const std::string& field, const std::string& context) {
    static const std::regex meterRe(R"(^\d+/\d+$)");
    const std::string meter = nonEmptyStringFromJson(entry, field, context);
    if (!std::regex_match(meter, meterRe)) {
        throw std::invalid_argument(choraleFilterFieldLocation(field, context) + ": '" + meter + "' is not a meter like \"4/4\"");
    }
    return meter;
}

std::string keyFromJson(const nlohmann::json& entry, const std::string& field, const std::string& context) {
    static const std::regex keyRe(R"(^[a-gA-G][#-]*$)");
    const std::string key = nonEmptyStringFromJson(entry, field, context);
    if (!std::regex_match(key, keyRe)) {
        throw std::invalid_argument(choraleFilterFieldLocation(field, context) + ": '" + key + "' is not a key like \"G\", \"e\" "
                                    "or \"B-\" (a capital letter is a major key, a small one a minor key)");
    }
    return key;
}

std::string modeFromJson(const nlohmann::json& entry, const std::string& field, const std::string& context) {
    const std::string name = nonEmptyStringFromJson(entry, field, context);
    const std::optional<std::string> mode = normalizeModeName(name);
    if (!mode) {
        std::string joined;
        for (const std::string& known : modeNames()) joined += (joined.empty() ? "" : ", ") + known;
        throw std::invalid_argument(choraleFilterFieldLocation(field, context) + ": '" + name + "' is not a mode (" + joined + ")");
    }
    return *mode;
}

CountRange countRangeFromJson(const nlohmann::json& j, const std::string& field, const std::string& context) {
    const std::string where = choraleFilterFieldLocation(field, context);
    CountRange range;
    if (j.is_number_unsigned()) {
        range.min = range.max = j.get<std::size_t>();
        return range;
    }
    if (!j.is_object()) {
        throw std::invalid_argument(where + " must be a number or an object with 'min' and/or 'max'");
    }
    for (auto it = j.begin(); it != j.end(); ++it) {
        if (it.key() != "min" && it.key() != "max") {
            throw std::invalid_argument(where + " has an unknown field '" + it.key() + "' (only 'min' and 'max')");
        }
        if (!it.value().is_number_unsigned()) {
            throw std::invalid_argument(where + ": '" + it.key() + "' must be a non-negative integer");
        }
    }
    if (j.contains("min")) range.min = j["min"].get<std::size_t>();
    if (j.contains("max")) range.max = j["max"].get<std::size_t>();
    if (!range.min && !range.max) throw std::invalid_argument(where + " needs 'min' and/or 'max'");
    if (range.min && range.max && *range.min > *range.max) {
        throw std::invalid_argument(where + ": 'min' is greater than 'max'");
    }
    return range;
}

bool isKnownChoraleFilterField(const std::string& key) {
    static const std::vector<std::string> known = {
        "ids", "excludeIds", "title", "meter", "measureCount", "phraseCount", "startKey", "startMode",
    };
    return std::find(known.begin(), known.end(), key) != known.end();
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

    if (j.contains("choraleFilter")) q.choraleFilter = choraleFilterFromJson(j["choraleFilter"], context);

    if (j.contains("simultaneousWith")) {
        q.simultaneousWith = simultaneousWithFromJson(j["simultaneousWith"]);
    }

    return q;
}

} // namespace

ChoraleFilter choraleFilterFromJson(const nlohmann::json& j, const std::string& context) {
    if (!j.is_object()) throw std::invalid_argument("'choraleFilter' in " + context + " must be an object");
    rejectUnknownKeys(j, isKnownChoraleFilterField, "'choraleFilter' in " + context);

    ChoraleFilter filter;
    if (j.contains("ids")) filter.ids = listFromJson<ChoraleIdRange>(j["ids"], "ids", context, idRangeFromJson);
    if (j.contains("excludeIds")) {
        filter.excludeIds = listFromJson<ChoraleIdRange>(j["excludeIds"], "excludeIds", context, idRangeFromJson);
    }
    if (j.contains("title")) filter.title = listFromJson<std::string>(j["title"], "title", context, nonEmptyStringFromJson);
    if (j.contains("meter")) filter.meter = listFromJson<std::string>(j["meter"], "meter", context, meterFromJson);
    if (j.contains("measureCount")) filter.measureCount = countRangeFromJson(j["measureCount"], "measureCount", context);
    if (j.contains("phraseCount")) filter.phraseCount = countRangeFromJson(j["phraseCount"], "phraseCount", context);
    if (j.contains("startKey")) filter.startKey = listFromJson<std::string>(j["startKey"], "startKey", context, keyFromJson);
    if (j.contains("startMode")) filter.startMode = listFromJson<std::string>(j["startMode"], "startMode", context, modeFromJson);
    return filter;
}

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
    j["voice"] = r.voice;
    j["startPosition"] = r.startPosition;
    j["endPosition"] = r.endPosition;
    j["startLine"] = r.startLineNumber;
    j["endLine"] = r.endLineNumber;
    if (!r.linkedSections.empty()) {
        j["linkedSections"] = nlohmann::json::array();
        for (const NoteSection& section : r.linkedSections) {
            j["linkedSections"].push_back({
                {"feature", section.feature},
                {"voice", section.voice},
                {"startLine", section.startLineNumber},
                {"endLine", section.endLineNumber},
                {"startPosition", section.startPosition},
                {"endPosition", section.endPosition},
            });
        }
    }
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

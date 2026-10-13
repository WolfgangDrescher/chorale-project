#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "ChoraleInfo.hpp"

namespace choralesearch {

// A count a chorale has to have: exactly one number, or anything from `min` to `max` where a bound
// that is left out is open.
struct CountRange {
    std::optional<std::size_t> min;
    std::optional<std::size_t> max;

    bool contains(std::size_t count) const { return (!min || count >= *min) && (!max || count <= *max); }
    bool operator==(const CountRange& other) const { return min == other.min && max == other.max; }
};

// One entry of a list of chorale ids: the numbers of the chorales it names, from `first` to `last`
// (the same for a single one). `text` is how the query spelled it.
struct ChoraleIdRange {
    std::string text;
    std::size_t first = 0;
    std::size_t last = 0;

    bool contains(std::size_t number) const { return number >= first && number <= last; }
    bool operator==(const ChoraleIdRange& other) const {
        return text == other.text && first == other.first && last == other.last;
    }
};

// Which chorales a query searches. Every filter that is set has to hold for a chorale to be searched
// at all; a list holds when any of its entries does. An empty filter (the default) lets every chorale in.
struct ChoraleFilter {
    // Chorales by the number in their id (29 for chor029): only these, or all but these.
    std::vector<ChoraleIdRange> ids;
    std::vector<ChoraleIdRange> excludeIds;

    // Text a title contains, without regard to case. Both the German and the English title count.
    std::vector<std::string> title;

    // The meters ("4/4") the chorale is in at some point.
    std::vector<std::string> meter;

    std::optional<CountRange> measureCount;
    std::optional<CountRange> phraseCount;

    // The key ("G", "e", "B-") and the mode (see normalizeModeName) the chorale starts in.
    std::vector<std::string> startKey;
    std::vector<std::string> startMode;

    bool empty() const {
        return ids.empty() && excludeIds.empty() && !needsChoraleInfo();
    }

    // Whether anything beyond the id lists is asked, i.e. whether a ChoraleInfo has to be read from the
    // score to answer. The id lists can be judged from the file name alone.
    bool needsChoraleInfo() const {
        return !title.empty() || !meter.empty() || measureCount || phraseCount || !startKey.empty() || !startMode.empty();
    }

    // Whether a chorale with this id passes the id lists.
    bool matchesId(const std::string& id) const;

    // Whether the chorale passes every filter, the id lists included.
    bool matches(const ChoraleInfo& info) const;

    bool operator==(const ChoraleFilter& other) const {
        return ids == other.ids && excludeIds == other.excludeIds && title == other.title &&
               meter == other.meter && measureCount == other.measureCount && phraseCount == other.phraseCount &&
               startKey == other.startKey && startMode == other.startMode;
    }
};

inline nlohmann::json countRangeToJson(const CountRange& range) {
    if (range.min && range.max && *range.min == *range.max) return *range.min;
    nlohmann::json j = nlohmann::json::object();
    if (range.min) j["min"] = *range.min;
    if (range.max) j["max"] = *range.max;
    return j;
}

// The inverse of JsonIO's choraleFilterFromJson, omitting every filter that is not set.
inline nlohmann::json choraleFilterToJson(const ChoraleFilter& filter) {
    nlohmann::json j = nlohmann::json::object();
    auto idsToJson = [](const std::vector<ChoraleIdRange>& ranges) {
        nlohmann::json list = nlohmann::json::array();
        for (const ChoraleIdRange& range : ranges) list.push_back(range.text);
        return list;
    };
    if (!filter.ids.empty()) j["ids"] = idsToJson(filter.ids);
    if (!filter.excludeIds.empty()) j["excludeIds"] = idsToJson(filter.excludeIds);
    if (!filter.title.empty()) j["title"] = filter.title;
    if (!filter.meter.empty()) j["meter"] = filter.meter;
    if (filter.measureCount) j["measureCount"] = countRangeToJson(*filter.measureCount);
    if (filter.phraseCount) j["phraseCount"] = countRangeToJson(*filter.phraseCount);
    if (!filter.startKey.empty()) j["startKey"] = filter.startKey;
    if (!filter.startMode.empty()) j["startMode"] = filter.startMode;
    return j;
}

} // namespace choralesearch

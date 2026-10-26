#pragma once

#include <algorithm>
#include <cstddef>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "Query.hpp"

namespace choralesearch {

// Named lists of values, and the category of every other value, if any.
struct Categories {
    std::map<std::string, std::vector<std::string>> valuesByName;
    std::optional<std::string> remainingName;

    bool empty() const { return valuesByName.empty() && !remainingName; }

    // The category of `value`, or the value itself.
    std::string categoryNameFor(const std::string& value) const {
        for (const auto& [name, values] : valuesByName) {
            if (std::find(values.begin(), values.end(), value) != values.end()) return name;
        }
        return remainingName.value_or(value);
    }
};

// One feature a level reads at every match, named as in the JSON of a group query.
struct FeatureRead {
    // deg, mint, metweight, duration or a hint pair such as hint-14.
    std::string feature;

    // The positions of the pattern it is read at, from 0; empty for all.
    std::vector<std::size_t> positions;
    // Instead the intervals this voice (1 = bass) sings across the match, in its own rhythm.
    std::optional<std::size_t> notesOfVoice;

    bool ignoreQuality = false;                    // "-M2" counts as "-2"
    bool mintAllowIntervalComplementation = false; // "-P5" counts as "+P4", only with notesOfVoice

    Categories categories;

    // Whether its groups stand for several values, listed as members: the values of a category,
    // an interval and its complement, or the notes of a voice with and without a held note.
    bool groupsSeveralValues() const { return mintAllowIntervalComplementation || !categories.empty() || notesOfVoice.has_value(); }
};

// One level of a grouping: the matches are grouped by what its reads find.
struct GroupingLevel {
    std::vector<FeatureRead> reads;
    // Which groups are kept; the matches of the others are counted as omitted.
    std::optional<std::size_t> maxGroupCount;
    std::size_t minMatchCount = 1;

    // Whether a group is kept after `keptGroupCount` more frequent ones.
    bool keeps(std::size_t matchCount, std::size_t keptGroupCount) const {
        return matchCount >= minMatchCount && (!maxGroupCount || keptGroupCount < *maxGroupCount);
    }
};

// The group query read another way, or of fewer matches.
struct GroupQueryVariant {
    std::string name;
    bool ignoreQuality = false; // for every read
    // Keys added to the positions of the pattern: [{}, {"phrase": "end"}].
    nlohmann::json pattern = nlohmann::json::array();
};

// How many examples each group shows, 0 for none: the unornamented passage first, then one for
// each set of ornamented voices, then passages of further chorales.
struct ExampleSettings {
    std::size_t maxCount = 5;
};

// A query and how its matches are grouped.
struct GroupQuery {
    Query query;
    std::vector<GroupingLevel> levels;
    ExampleSettings examples;
    std::vector<GroupQueryVariant> variants;
};

} // namespace choralesearch

#pragma once

#include <map>
#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace choralesearch {

// feature name -> OR-list of acceptable values ("*" anywhere in the list = wildcard)
using AttributeMap = std::map<std::string, std::vector<std::string>>;

struct Query {
    std::string feature;
    std::string voices = "all";

    std::vector<AttributeMap> pattern;

    std::optional<std::size_t> limit;

    // Only relevant when feature == "mint": a mint token records the interval *into* a
    // note, so the note the first matched interval starts from is one token earlier than
    // the match itself. When set, that earlier token becomes startLine/startPosition --
    // unless the pattern's first position already pins down the driving feature itself
    // (e.g. an explicit {"mint": "*"}), which signals the caller included that lead-in
    // token in the pattern themselves.
    bool mintStartAtPreviousToken = false;

    // Only relevant when feature == "fb": by default an fb pattern value's figures are a
    // minimum requirement, so "2 4" also matches a chord actually voiced as "2 4 6". When
    // set, the chord must have exactly as many figures as the pattern value -- no extras.
    bool fbCompareExactChord = false;
};

inline std::ostream& operator<<(std::ostream& os, const Query& q) {
    nlohmann::json j;
    j["feature"] = q.feature;
    j["voices"] = q.voices;

    nlohmann::json pattern = nlohmann::json::array();
    for (const auto& position : q.pattern) {
        nlohmann::json posJson = nlohmann::json::object();
        for (const auto& [key, values] : position) {
            posJson[key] = values;
        }
        pattern.push_back(posJson);
    }
    j["pattern"] = pattern;

    if (q.limit) j["limit"] = *q.limit;
    if (q.mintStartAtPreviousToken) j["mintStartAtPreviousToken"] = true;
    if (q.fbCompareExactChord) j["fbCompareExactChord"] = true;

    return os << j.dump(1, '\t') << std::endl;
}

} // namespace choralesearch

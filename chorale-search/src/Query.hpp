#pragma once

#include <map>
#include <optional>
#include <string>
#include <vector>

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
};

} // namespace choralesearch

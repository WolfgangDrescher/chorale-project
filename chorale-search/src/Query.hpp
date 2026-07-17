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
};

} // namespace choralesearch

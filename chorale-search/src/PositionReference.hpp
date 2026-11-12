#pragma once

#include <optional>
#include <string>
#include <vector>

#include "Query.hpp"

namespace choralesearch {

// Per position, its name under "as", or nullopt. Throws for an invalid or duplicate name.
std::vector<std::optional<std::string>> positionNames(const std::vector<AttributeMap>& pattern);

// `pattern` with every PatternValue::positionReference set. Throws, naming position and key, for a reference
// that cannot be read: a later or unknown position, a key without references, a value of another kind.
std::vector<AttributeMap> resolvePositionReferences(const std::vector<AttributeMap>& pattern);

} // namespace choralesearch

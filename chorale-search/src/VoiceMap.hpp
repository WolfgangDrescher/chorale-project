#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace choralesearch {

// A chorale has four voices, numbered 1 (bass) to 4 (soprano).
inline constexpr std::size_t kVoiceCount = 4;

// Resolves a comma-separated selector string into 1-indexed voice
// numbers. Accepts, case-insensitively:
//   - "all" / ""    -> every voice (1,2,3,4)
//   - "1".."4"      -> that voice
//   - a multi-digit token, e.g. "124" -> Bass,Tenor,Soprano; "1234" -> all four
//   - names: bass/tenor/alto/soprano (and b/t/a/s)
// Throws std::invalid_argument on an unresolvable token.
std::vector<std::size_t> resolveVoices(const std::string& selector);

} // namespace choralesearch

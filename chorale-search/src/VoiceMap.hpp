#pragma once

#include <string>
#include <vector>

namespace choralesearch {

// Human-readable label for a voice, e.g. "Bass".
// Out-of-range (not 1-4) returns "?".
std::string voiceLabel(std::size_t voice);

// Resolves a comma-separated selector string into 1-indexed voice
// numbers. Accepts, case-insensitively:
//   - "all" / ""    -> every voice (1,2,3,4)
//   - "1".."4"      -> that voice
//   - a multi-digit token, e.g. "124" -> Bass,Tenor,Soprano; "1234" -> all four
//   - names: bass/tenor/alto/soprano (and b/t/a/s)
// Throws std::invalid_argument on an unresolvable token.
std::vector<std::size_t> resolveVoices(const std::string& selector);

} // namespace choralesearch

#pragma once

#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "humlib.h"

namespace choralesearch {

// One syllable (or none) per soprano attack, spelled as in **text: "Her-", "-zens".
using Verse = std::vector<std::optional<std::string>>;

// {"chor001": {"source": "...", "verses": [["Aus", "mei-", null, ...]]}}
std::map<std::string, std::vector<Verse>> readLyrics(const std::filesystem::path& path);

// Adds a **text spine per verse after the soprano, the last **kern spine, and the spines computed
// from it. Numbered by stanza where there are several.
// If the number of soprano attacks doesn't match, the lyrics are left out and reported on stderr.
// Re-parses `infile`.
void applyLyrics(hum::HumdrumFile& infile, const std::string& choraleId, const std::vector<Verse>& verses);

bool isSyllableAttack(hum::HTp token);

} // namespace choralesearch

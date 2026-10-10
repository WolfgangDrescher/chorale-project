#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace choralesearch {

// Which of the voices a reference points to count, and how they are combined. Nearest is only the
// voice next to the one searched, Any holds when one of the voices satisfies the value, All only when
// every one of them does.
enum class ReferenceQuantifier { Nearest, Any, All };

// The voices a reference stands for while one voice is being walked, and how they are combined: Any or
// All, as Nearest has already been resolved to the one voice it stands for.
struct ResolvedVoiceReference {
    ReferenceQuantifier quantifier = ReferenceQuantifier::Any;
    std::vector<std::size_t> voices;
};

// True if `text` is the spelling of a voice reference: "@1" to "@4" for one fixed voice, "@below" and
// "@above" for the neighbour voice, "@anyBelow"/"@allBelow", "@anyAbove"/"@allAbove" and
// "@anyOther"/"@allOther" for a group of voices. Voice 1 is the bass, so below means a lower number.
bool isValidVoiceReference(const std::string& text);

// The voices `text` stands for while `walkedVoice` is the voice being searched. The walked voice is
// never one of them, and a group that has no voice in it (@below of the bass) is empty. nullopt if
// `text` is no voice reference at all.
std::optional<ResolvedVoiceReference> resolveVoiceReference(const std::string& text, std::size_t walkedVoice);

} // namespace choralesearch

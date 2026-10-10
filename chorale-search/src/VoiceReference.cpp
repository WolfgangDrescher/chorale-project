#include "VoiceReference.hpp"

#include "VoiceMap.hpp"

namespace choralesearch {

namespace {

// In which direction, counted from the voice being searched, a reference looks for voices. Voice 1 is
// the bass, so below means a lower number.
enum class ReferenceDirection { Below, Above, Both };

struct ReferenceKeyword {
    const char* text;
    ReferenceDirection direction;
    ReferenceQuantifier quantifier;
};

const ReferenceKeyword kReferenceKeywords[] = {
    {"@below", ReferenceDirection::Below, ReferenceQuantifier::Nearest},
    {"@above", ReferenceDirection::Above, ReferenceQuantifier::Nearest},
    {"@anyBelow", ReferenceDirection::Below, ReferenceQuantifier::Any},
    {"@allBelow", ReferenceDirection::Below, ReferenceQuantifier::All},
    {"@anyAbove", ReferenceDirection::Above, ReferenceQuantifier::Any},
    {"@allAbove", ReferenceDirection::Above, ReferenceQuantifier::All},
    {"@anyOther", ReferenceDirection::Both, ReferenceQuantifier::Any},
    {"@allOther", ReferenceDirection::Both, ReferenceQuantifier::All},
};

const ReferenceKeyword* findKeyword(const std::string& text) {
    for (const ReferenceKeyword& keyword : kReferenceKeywords) {
        if (text == keyword.text) return &keyword;
    }
    return nullptr;
}

// "@1" to "@4": the voice with that number.
std::optional<std::size_t> findFixedVoice(const std::string& text) {
    if (text.size() == 2 && text[0] == '@' && text[1] >= '1' && text[1] <= '0' + static_cast<int>(kVoiceCount)) {
        return static_cast<std::size_t>(text[1] - '0');
    }
    return std::nullopt;
}

} // namespace

bool isValidVoiceReference(const std::string& text) {
    return findKeyword(text) || findFixedVoice(text);
}

std::optional<ResolvedVoiceReference> resolveVoiceReference(const std::string& text, std::size_t walkedVoice) {
    const ReferenceKeyword* keyword = findKeyword(text);
    std::optional<std::size_t> fixedVoice = findFixedVoice(text);
    if (!keyword && !fixedVoice) return std::nullopt;

    ResolvedVoiceReference resolved;
    // No voice is walked: nothing is above, below or beside it.
    if (walkedVoice < 1 || walkedVoice > kVoiceCount) return resolved;

    if (fixedVoice) {
        if (*fixedVoice != walkedVoice) resolved.voices.push_back(*fixedVoice);
        return resolved;
    }

    // Nearest is resolved to its one voice below, for which any and all say the same.
    const bool nearest = keyword->quantifier == ReferenceQuantifier::Nearest;
    resolved.quantifier = nearest ? ReferenceQuantifier::Any : keyword->quantifier;
    if (keyword->direction != ReferenceDirection::Above) {
        for (std::size_t voice = 1; voice < walkedVoice; ++voice) resolved.voices.push_back(voice);
    }
    if (keyword->direction != ReferenceDirection::Below) {
        for (std::size_t voice = walkedVoice + 1; voice <= kVoiceCount; ++voice) resolved.voices.push_back(voice);
    }
    if (nearest && !resolved.voices.empty()) {
        std::size_t nearest = keyword->direction == ReferenceDirection::Below ? resolved.voices.back() : resolved.voices.front();
        resolved.voices = {nearest};
    }
    return resolved;
}

} // namespace choralesearch

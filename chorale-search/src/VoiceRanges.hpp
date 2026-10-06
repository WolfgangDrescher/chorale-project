#pragma once

#include <array>

#include "VoiceMap.hpp"

namespace choralesearch {

// The range of a voice as the lower and the upper limit of the notes it takes, in MIDI note numbers.
struct VoiceRange {
    int lower;
    int upper;
};

// The ranges of the four voices: bass, tenor, alto, soprano.
using VoiceRanges = std::array<VoiceRange, kVoiceCount>;

// The ranges of Berlioz's treatise on instrumentation, as Strauss revised it.
inline constexpr VoiceRanges kStraussBerliozVoiceRanges = {{
    {41, 63}, // F to es'
    {48, 70}, // c to b'
    {53, 75}, // f to es''
    {60, 82}, // c' to b''
}};

// The ranges Bach's chorales use: the lowest and the highest note of every voice over all 370.
inline constexpr VoiceRanges kBachVoiceRanges = {{
    {36, 64}, // C to e'
    {48, 69}, // c to a'
    {52, 74}, // e to d''
    {57, 81}, // a to a''
}};

// Which of the ranges a check measures the voices against.
enum class VoiceRangeSet {
    StraussBerlioz,
    Bach,
};

inline const VoiceRanges& voiceRanges(VoiceRangeSet set) {
    switch (set) {
        case VoiceRangeSet::Bach: return kBachVoiceRanges;
        case VoiceRangeSet::StraussBerlioz: break;
    }
    return kStraussBerliozVoiceRanges;
}

} // namespace choralesearch

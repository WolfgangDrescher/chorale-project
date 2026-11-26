#pragma once

#include <array>
#include <cstddef>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "RegolaChords.hpp"
#include "humlib.h"

namespace choralesearch {

class HumdrumChorale;

// How the regola, which knows major and minor, reads a mode: the semitones of its scale, and whether it is read as
// minor, its sixth and seventh degree melodic.
struct RegolaMode {
    std::array<int, 7> scale;
    bool minor = false;
};

// The mode as normalizeModeName() spells it, nullopt for one the regola leaves out.
std::optional<RegolaMode> regolaMode(const std::string& mode);

// Where the bass goes on from `bass`: up or down by step to its next note on another degree, or to the next such
// note with a metric weight (strong, half-strong or weak), so that a diminution may leap in between. A leap where
// neither is a step, none at the last note.
std::vector<BassMotion> bassMotions(const HumdrumChorale& chorale, hum::HTp bass);

// The chord of the regola on each line of a chorale, over the bass that sounds there.
class RegolaAnalysis {
public:
    // `tags` choose the chords as allowedRegolaChords() reads them.
    RegolaAnalysis(const HumdrumChorale& chorale, const std::vector<std::string>& tags);

    // nullptr where the bass rests, or no allowed chord fits key, degree and motion of the bass and explains a note
    // above it.
    const RegolaChord* chordAtLine(int lineNumber) const;

    // Whether the note `voice` sounds on `lineNumber`, begun there or held, is a chord tone of the chord of the line.
    // nullopt for the bass, a rest, or a line without a chord.
    std::optional<bool> isChordTone(const HumdrumChorale& chorale, std::size_t voice, int lineNumber) const;

    // A scale degree: its number and its semitones above the key.
    struct Degree {
        int number = 1;
        int semitones = 0;
    };

    // What a chord stands on: the bass in its key.
    struct Ground {
        Degree bass;
        std::string key;
        RegolaMode mode;
    };

private:
    struct Sound {
        Ground ground;
        const RegolaChord* chord = nullptr;
    };
    const Sound* soundAtLine(int lineNumber) const;

    std::map<int, Sound> m_soundsByLine;
};

} // namespace choralesearch

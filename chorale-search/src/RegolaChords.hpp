#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace choralesearch {

// Where the bass goes on, see bassMotions().
enum class BassMotion { Ascending, Descending, Leap };

// The modes a chord is for, as regolaMode() reads a mode as major or as minor.
enum class ChordMode { Any, Major, Minor };

// A figure over the bass: its interval number within an octave, 1 for the octave, and its accidental relative to
// the key, as a figured bass writes it: "#6" is {6, +1}.
struct RegolaFigure {
    int number = 1;
    int alteration = 0;

    bool operator==(const RegolaFigure&) const = default;
};

// A chord of the regola dell'ottava as Kartimento has it: the figures on a degree of the bass, for where the bass
// goes on.
struct RegolaChord {
    std::string scaleDegree;                  // "1" to "7", "4+", as **deg writes it
    std::string figuredBass;                  // as Kartimento writes it: "6 5"
    std::vector<BassMotion> bassMotions;      // empty for any
    std::vector<std::string> tags;            // "core", "affect", ...
    // Every figure of the chord, a dissonance as the 4 of "5 4" included.
    std::vector<RegolaFigure> chordTones;
    ChordMode mode = ChordMode::Any;

    // Allows a bass that goes on in one of `motions`.
    bool allowsBassMotion(const std::vector<BassMotion>& motions) const;
    bool allowsMode(bool minor) const;
    bool hasTag(const std::string& tag) const;
};

// In the order a tie between two chords is decided in: the core before the extended ones, the plain chord before
// the enriched one.
const std::vector<RegolaChord>& regolaChords();

// Every tag of a chord, in the order of its first chord.
const std::vector<std::string>& regolaTagNames();

// A tag of a chord, or one negated: "!affect".
bool isValidRegolaTag(const std::string& tag);

// The chords `tags` allow, as indices into regolaChords(): a chord with one of the tags and none of the negated
// ones; any chord without a negated one where no tag is positive. Without tags the core. Throws for a tag no chord
// has.
std::vector<std::size_t> allowedRegolaChords(const std::vector<std::string>& tags);

} // namespace choralesearch

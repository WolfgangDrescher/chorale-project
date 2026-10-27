#pragma once

#include <string>

#include "humlib.h"

namespace choralesearch {

// The inverse of humlib's satb2gs, structurally: a grand staff score's two staves with two voices
// each become the four spines bass-tenor-alto-soprano. Which layer of a staff is which voice
// is decided by register (mean pitch), not by spine order, so both satb2gs output and MusicXML
// exports come out right regardless of their conventions. Works line by line, so the output
// has exactly one line per input line (the staff-internal spine manipulators aside) and line
// numbers keep meaning the same music -- which is what lets segments found in the split score
// be highlighted on the rendered original.
//
// A staff that never sounds two notes at once (no chord, no second layer that plays) carries a
// lone voice, however it is written -- a layer of rests next to the melody included. That voice
// is the bass of the lower staff or the soprano of the upper one, and the staff's other voice
// rests: a one-measure rest at each measure's first line, null tokens after it. Tenor and alto are
// never filled with a copy of the outer voices. A pair of lone-voice staves thus ends up as
// the same bass-and-soprano setting a four-staff score with silent inner staves does.
//
// Throws std::invalid_argument for a score this reading doesn't fit: not exactly two **kern
// spines, more than two simultaneous notes on one staff, or a spine manipulation (*x, *+)
// the split can't carry over.
std::string splitScoreIntoVoices(hum::HumdrumFile& infile);

} // namespace choralesearch

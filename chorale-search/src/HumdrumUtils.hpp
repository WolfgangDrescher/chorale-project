#pragma once

#include <sstream>
#include <string>

#include "humlib.h"

// The handful of things about humlib's tokens and positions that every module ends up needing
// and humlib itself doesn't offer. Nothing here knows what a chorale, a query or a segment is.

namespace choralesearch {

// How long the note at this onset actually sounds. tok isn't necessarily **kern (a duration is
// read off whichever spine is being walked), so getTiedDuration() -- **kern-specific, and what
// keeps a tied note one note rather than several -- is only safe to call once we know it is one.
inline hum::HumNum soundingDuration(hum::HTp tok) {
    return tok->isKern() ? tok->getTiedDuration() : tok->getDuration();
}

// A musical position (quarter notes from the start of the piece) as text, e.g. "35+1/2" for a
// position halfway through the 36th quarter. Positions are fractions, so they're carried around
// as strings rather than lossily flattened into a double.
inline std::string humNumToString(const hum::HumNum& value) {
    std::ostringstream oss;
    value.printTwoPart(oss);
    return oss.str();
}

} // namespace choralesearch

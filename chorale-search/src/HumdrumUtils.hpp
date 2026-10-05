#pragma once

#include <sstream>
#include <string>
#include <vector>

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

// The track numbers of the **kern spines, in spine order. How many voices (or staves) a score
// has is this list's size; everything else about a spine is asked through the track number.
inline std::vector<int> kernTracks(hum::HumdrumFile& infile) {
    std::vector<hum::HTp> starts;
    infile.getSpineStartList(starts);
    std::vector<int> tracks;
    for (hum::HTp start : starts) {
        if (start->isKern()) tracks.push_back(start->getTrack());
    }
    return tracks;
}

} // namespace choralesearch

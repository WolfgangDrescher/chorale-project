#pragma once

#include <string>

namespace choralesearch {

// Whether an uploaded score's text is MusicXML rather than **kern -- decided by the content
// (an XML document starts with '<'), never by a file name, which an upload doesn't have.
bool looksLikeMusicXml(const std::string& input);

// The MusicXML text as **kern, via humlib's musicxml2hum. What comes out mirrors the input's
// layout: a four-part score stays four spines, a grand staff score two -- pulling a grand staff
// apart into four voices is splitScoreIntoVoices() (see SplitScoreIntoVoices.hpp), not this one's.
//
// Throws std::invalid_argument when the input can't be converted (a compressed .mxl archive
// included -- only plain MusicXML is supported).
std::string musicXmlToKern(const std::string& input);

// Reshapes a four-voice **kern text's header the way the corpus's own scores spell it:
// whatever the input carried about parts, staves and instruments (*part, *staff, *I" names,
// *I' abbreviations, *I codes, *IC classes) is dropped, along with the !!!system-decoration
// record a MusicXML conversion writes. Every score the pipeline hands on is the same four
// voices, so none of that bookkeeping says anything.
//
// Throws std::invalid_argument when the text does not parse as Humdrum.
std::string normalizeChoraleHeader(const std::string& kern);

} // namespace choralesearch

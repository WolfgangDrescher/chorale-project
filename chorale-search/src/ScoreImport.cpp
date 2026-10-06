#include "ScoreImport.hpp"

#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "HumdrumUtils.hpp"
#include "ProgressReport.hpp"
#include "SplitScoreIntoVoices.hpp"
#include "humlib.h"

namespace choralesearch {

namespace {

// Whether a header line states part, staff or instrument bookkeeping -- judged by its first
// token, since these interpretations always fill the whole line. "*I" covers the codes, the
// classes (*IC...), the names (*I"...) and the abbreviations (*I'...) alike; nothing else in
// **kern starts that way.
bool isPartStaffOrInstrumentLine(const std::string& line) {
    return line.rfind("*part", 0) == 0 || line.rfind("*staff", 0) == 0 || line.rfind("*I", 0) == 0;
}

} // namespace

bool looksLikeMusicXml(const std::string& input) {
    const std::size_t start = input.find_first_not_of(" \t\r\n\xEF\xBB\xBF"); // skips a BOM too
    return start != std::string::npos && input[start] == '<';
}

std::string musicXmlToKern(const std::string& input) {
    if (input.rfind("PK\x03\x04", 0) == 0) {
        throw std::invalid_argument("compressed MusicXML (.mxl) is not supported -- export uncompressed MusicXML");
    }
    hum::Tool_musicxml2hum converter;
    std::stringstream out;
    if (!converter.convert(out, input.c_str()) || out.str().empty()) {
        throw std::invalid_argument("could not convert the MusicXML input");
    }
    return out.str();
}

std::string normalizeChoraleHeader(const std::string& kern) {
    hum::HumdrumFile infile;
    if (!infile.readString(kern)) {
        throw std::invalid_argument("could not parse the score as Humdrum **kern");
    }

    // Backwards, since deleteLine shifts everything after it.
    for (int i = infile.getLineCount() - 1; i >= 0; --i) {
        const std::string& line = infile[i];
        if (isPartStaffOrInstrumentLine(line) || line.rfind("!!!system-decoration", 0) == 0) {
            infile.deleteLine(i);
            continue;
        }
        if (line.rfind("*clef", 0) == 0) {
            // The voices' canonical clefs, whatever the input wrote them in: bass clef,
            // octave-down treble for the tenor, treble for alto and soprano. Replaced
            // rather than dropped, so the line count stays.
            infile[i].setText("*clefF4\t*clefGv2\t*clefG2\t*clefG2");
        }
    }

    std::ostringstream out;
    out << infile;
    return out.str();
}

PreparedScore prepareScore(const std::string& input, bool reportProgress) {
    PreparedScore score;
    score.inputFormat = looksLikeMusicXml(input) ? "musicxml" : "kern";
    if (reportProgress && score.inputFormat == "musicxml") reportPhase("convert-musicxml");
    const std::string kernText = score.inputFormat == "musicxml" ? musicXmlToKern(input) : input;

    hum::HumdrumFile infile;
    if (!infile.readString(kernText)) {
        throw std::invalid_argument("could not parse the score as Humdrum **kern");
    }

    const std::size_t voices = kernTracks(infile).size();
    if (voices == 4) {
        score.layout = "satb";
        score.kern = kernText;
    } else if (voices == 2) {
        score.layout = "grand-staff";
        if (reportProgress) reportPhase("split-score-into-voices");
        score.kern = splitScoreIntoVoices(infile);
    } else {
        throw std::invalid_argument("expected a score with 4 voices or a two-staff grand staff score, got " +
                                     std::to_string(voices) + " **kern spine(s)");
    }

    // The corpus's own header shape, whatever the input carried: canonical voice interpretations
    // instead of part/staff/instrument bookkeeping. Whatever runs on the score runs on this same
    // text, so the line numbers it reports mean lines of the kern returned here.
    score.kern = normalizeChoraleHeader(score.kern);
    return score;
}

} // namespace choralesearch

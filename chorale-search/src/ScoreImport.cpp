#include "ScoreImport.hpp"

#include <algorithm>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
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

// Numbers the barlines with humlib's barnum, unless the first one after the first note has a
// number already (as in the corpus). barnum only recognizes a pickup if a barline stands before
// it, otherwise it counts the pickup as measure 1. So a barline is added in front of the first
// note if there is none.
void addMissingMeasureNumbers(hum::HumdrumFile& infile) {
    int firstNote = -1;
    for (int i = 0; i < infile.getLineCount(); ++i) {
        if (firstNote < 0 && infile[i].isData()) firstNote = i;
        if (firstNote >= 0 && infile[i].isBarline()) {
            if (infile[i].getBarNumber() >= 0) return;
            break;
        }
    }
    if (firstNote < 0) return;

    bool barlineBefore = false;
    std::ostringstream text;
    for (int i = 0; i < infile.getLineCount(); ++i) {
        if (i < firstNote && infile[i].isBarline()) barlineBefore = true;
        if (i == firstNote && !barlineBefore) {
            text << "=";
            for (int j = 1; j < infile[i].getFieldCount(); ++j) text << "\t=";
            text << "\n";
        }
        text << infile[i] << "\n";
    }
    hum::Tool_barnum barnum;
    hum::HumdrumFile withLeadingBarline;
    withLeadingBarline.readString(text.str());
    barnum.run(withLeadingBarline);
    if (!barnum.getAllText().empty()) infile.readString(barnum.getAllText());
}

// A note with a fermata: where it is attacked, where it stops sounding and in which measure it starts.
struct FermataNote {
    hum::HumNum start;
    hum::HumNum end;
    int measure;
};

// Throws unless the soprano has a fermata and every voice with notes holds a fermata during each
// of them. The phrases are read off each voice's fermatas, so missing ones would end a phrase in
// some voices only. Silent voices (of a two-voice setting) are left out.
void requireSimultaneousFermatas(const std::string& kern) {
    hum::HumdrumFile infile;
    if (!infile.readString(kern)) {
        throw std::invalid_argument("could not parse the score as Humdrum **kern");
    }
    addMissingMeasureNumbers(infile);

    // Per voice (track), its fermata notes.
    std::map<int, std::vector<FermataNote>> fermataNotes;
    int measure = 0; // a pickup comes before the first numbered barline
    for (int i = 0; i < infile.getLineCount(); ++i) {
        if (infile[i].isBarline() && infile[i].getBarNumber() >= 0) measure = infile[i].getBarNumber();
        if (!infile[i].isData()) continue;
        for (int j = 0; j < infile[i].getFieldCount(); ++j) {
            hum::HTp token = infile.token(i, j);
            if (!token->isKern() || token->isNull() || token->isRest()) continue;
            auto& notes = fermataNotes[token->getTrack()]; // a voice with notes, fermatas or not
            if (!token->hasFermata()) continue;
            // A fermata on the end of a tie belongs to the whole tied note.
            hum::HTp attack = token;
            while (attack->isSecondaryTiedNote()) {
                hum::HTp previous = attack->getPreviousToken();
                while (previous && (!previous->getOwner()->isData() || previous->isNull())) {
                    previous = previous->getPreviousToken();
                }
                if (!previous) break;
                attack = previous;
            }
            hum::HumNum start = attack->getDurationFromStart();
            notes.push_back({start, start + attack->getTiedDuration(), measure});
        }
    }

    const std::vector<int> tracks = kernTracks(infile);
    const auto cantusFirmus = fermataNotes.find(tracks.empty() ? 0 : tracks.back());
    if (cantusFirmus == fermataNotes.end() || cantusFirmus->second.empty()) {
        throw std::invalid_argument("the soprano has no fermata, mark the phrase endings with fermatas in all voices");
    }

    auto allVoicesHoldAFermataAt = [&](hum::HumNum time) {
        return std::all_of(fermataNotes.begin(), fermataNotes.end(), [&](const auto& voice) {
            return std::any_of(voice.second.begin(), voice.second.end(), [&](const FermataNote& note) {
                return note.start <= time && time < note.end;
            });
        });
    };
    for (const FermataNote& held : cantusFirmus->second) {
        // Where the fermata notes overlap at all, they do at the latest of their onsets, so
        // trying the onsets while the soprano's fermata sounds is enough.
        bool simultaneous = false;
        for (const auto& [track, notes] : fermataNotes) {
            simultaneous = std::any_of(notes.begin(), notes.end(), [&](const FermataNote& note) {
                return held.start <= note.start && note.start < held.end && allVoicesHoldAFermataAt(note.start);
            });
            if (simultaneous) break;
        }
        if (!simultaneous) {
            throw std::invalid_argument("the soprano's fermata in measure " + std::to_string(held.measure) +
                                        " is not held by all voices, put the fermata on every voice");
        }
    }
}

} // namespace

bool looksLikeMusicXml(const std::string& input) {
    const std::size_t start = input.find_first_not_of(" \t\r\n\xEF\xBB\xBF"); // skips a BOM too
    return start != std::string::npos && input[start] == '<';
}

std::string musicXmlToKern(const std::string& input) {
    if (input.rfind("PK\x03\x04", 0) == 0) {
        throw std::invalid_argument("compressed MusicXML (.mxl) is not supported, export it uncompressed");
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
    requireSimultaneousFermatas(score.kern);
    return score;
}

} // namespace choralesearch

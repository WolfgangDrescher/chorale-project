#include "SplitScoreIntoVoices.hpp"

#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <vector>

#include "HumdrumUtils.hpp"

namespace choralesearch {

namespace {

// A staff of the grand-staff input: its track number and which of its two layers sounds the
// upper voice. Everything else about it is read off the file when a line is printed.
struct Staff {
    int track = 0;
    int upperLayer = 0; // index into the track's tokens on a line (0 = leftmost subspine)
};

std::vector<std::string> splitChordNotes(const std::string& token) {
    std::vector<std::string> notes;
    std::istringstream iss(token);
    std::string note;
    while (iss >> note) notes.push_back(note);
    return notes;
}

bool isRestToken(const std::string& token) {
    return token.find('r') != std::string::npos;
}

// Mean MIDI note number of everything a track (or one of its layers) plays. What tells the
// bass staff from the treble and, within a staff, the upper layer from the lower -- registers
// overlap note by note (voices cross), but not on average over a whole chorale.
double meanPitch(hum::HumdrumFile& infile, int track, int onlyLayer = -1) {
    double sum = 0;
    int count = 0;
    for (int i = 0; i < infile.getLineCount(); ++i) {
        if (!infile[i].isData()) continue;
        int layer = 0;
        for (int j = 0; j < infile[i].getFieldCount(); ++j) {
            hum::HTp token = infile.token(i, j);
            if (token->getTrack() != track) continue;
            const int thisLayer = layer++;
            if (onlyLayer >= 0 && thisLayer != onlyLayer) continue;
            if (token->isNull()) continue;
            for (const std::string& note : splitChordNotes(*token)) {
                if (isRestToken(note)) continue;
                const int midi = hum::Convert::kernToMidiNoteNumber(note);
                if (midi <= 0) continue;
                sum += midi;
                ++count;
            }
        }
    }
    return count ? sum / count : 0;
}

std::vector<hum::HTp> tokensOfTrack(hum::HumdrumFile& infile, int line, int track) {
    std::vector<hum::HTp> tokens;
    for (int j = 0; j < infile[line].getFieldCount(); ++j) {
        hum::HTp token = infile.token(line, j);
        if (token->getTrack() == track) tokens.push_back(token);
    }
    return tokens;
}

// The two voices a staff contributes to one data line, lower first. A staff says one of three
// things there: two layers (each voice speaks for itself), one chord (the notes split by
// register), or one lone token (a doubling -- both voices sing it; rests and nulls the same).
std::pair<std::string, std::string> voicesOnDataLine(const std::vector<hum::HTp>& tokens, const Staff& staff,
                                                      int lineNumber) {
    if (tokens.size() > 2) {
        throw std::invalid_argument("more than two voices on one staff (line " + std::to_string(lineNumber) + ")");
    }

    if (tokens.size() == 2) {
        const std::string upper = *tokens[staff.upperLayer];
        const std::string lower = *tokens[1 - staff.upperLayer];
        if (upper.find(' ') != std::string::npos || lower.find(' ') != std::string::npos) {
            throw std::invalid_argument("a chord inside a two-voice staff layer (line " + std::to_string(lineNumber) +
                                         ") -- that is more than two voices on one staff");
        }
        return {lower, upper};
    }

    const std::string token = *tokens[0];
    if (token == "." || isRestToken(token)) return {token, token};

    const std::vector<std::string> notes = splitChordNotes(token);
    if (notes.size() == 1) return {token, token}; // both voices double the one note
    if (notes.size() == 2) {
        const int firstMidi = hum::Convert::kernToMidiNoteNumber(notes[0]);
        const int secondMidi = hum::Convert::kernToMidiNoteNumber(notes[1]);
        const bool firstIsLower = firstMidi <= secondMidi;
        return {notes[firstIsLower ? 0 : 1], notes[firstIsLower ? 1 : 0]};
    }
    throw std::invalid_argument("a chord with more than two notes (line " + std::to_string(lineNumber) +
                                 ") -- not a four-voice setting");
}

} // namespace

std::string splitScoreIntoVoices(hum::HumdrumFile& infile) {
    const std::vector<int> tracks = kernTracks(infile);
    if (tracks.size() != 2) {
        throw std::invalid_argument("a grand-staff split needs exactly 2 **kern spines, got " +
                                     std::to_string(tracks.size()));
    }

    Staff low{tracks[0], 0};
    Staff high{tracks[1], 0};
    if (meanPitch(infile, low.track) > meanPitch(infile, high.track)) std::swap(low.track, high.track);
    for (Staff* staff : {&low, &high}) {
        if (meanPitch(infile, staff->track, 0) < meanPitch(infile, staff->track, 1)) staff->upperLayer = 1;
    }

    std::ostringstream out;
    for (int i = 0; i < infile.getLineCount(); ++i) {
        hum::HumdrumLine& line = infile[i];
        if (!line.hasSpines()) {
            out << line << '\n';
            continue;
        }

        hum::HTp first = line.token(0);
        if (first->isExclusiveInterpretation()) {
            out << "**kern\t**kern\t**kern\t**kern\n";
            continue;
        }
        if (*first == "*-") {
            out << "*-\t*-\t*-\t*-\n";
            continue;
        }
        if (line.isManipulator()) {
            for (int j = 0; j < line.getFieldCount(); ++j) {
                hum::HTp token = line.token(j);
                if (token->isExchangeInterpretation() || token->isAddInterpretation()) {
                    throw std::invalid_argument("unsupported spine manipulation " + std::string(*token) + " (line " +
                                                 std::to_string(i + 1) + ")");
                }
            }
            continue; // the staff-internal splits and merges -- gone, four voices are four spines now
        }
        if (line.isLocalComment()) continue; // spine-bound layout hints whose spines no longer exist

        if (line.isInterpretation() || line.isBarline()) {
            // Whatever a staff states -- clef, key, meter, a barline -- both of its voices
            // inherit. Subspines rarely disagree here; the leftmost speaks for the staff.
            std::string lowToken = *tokensOfTrack(infile, i, low.track).front();
            std::string highToken = *tokensOfTrack(infile, i, high.track).front();
            out << lowToken << '\t' << lowToken << '\t' << highToken << '\t' << highToken << '\n';
            continue;
        }

        if (line.isData()) {
            const auto [bass, tenor] = voicesOnDataLine(tokensOfTrack(infile, i, low.track), low, i + 1);
            const auto [alto, soprano] = voicesOnDataLine(tokensOfTrack(infile, i, high.track), high, i + 1);
            out << bass << '\t' << tenor << '\t' << alto << '\t' << soprano << '\n';
            continue;
        }

        out << line << '\n';
    }
    return out.str();
}

} // namespace choralesearch

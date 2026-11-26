#include "RegolaAnalysis.hpp"

#include "HumdrumChorale.hpp"
#include "PatternKeys.hpp"

#include <algorithm>

namespace choralesearch {

namespace {

constexpr std::size_t kBassVoice = 1;
const std::string kSemitoniumModi = "semitonium-modi";
const std::string kMajorSixth = "major-sixth";

using Degree = RegolaAnalysis::Degree;
using Ground = RegolaAnalysis::Ground;

bool isNote(hum::HTp token) {
    return token && token->isData() && !token->isNull() && !token->isRest();
}

int positiveModulo(int value, int modulus) {
    return (value % modulus + modulus) % modulus;
}

// The degree of `note` in `key` ("B-"). Measured from the pitch, not read from **deg: a note held
// into a new key keeps the degree of the old one there.
Degree noteDegree(hum::HTp note, const std::string& key) {
    const int steps = hum::Convert::kernToBase7(note) - hum::Convert::kernToBase7(key);
    const int semitones = hum::Convert::kernToBase12PC(note) - hum::Convert::kernToBase12PC(key);
    return Degree{positiveModulo(steps, 7) + 1, positiveModulo(semitones, 12)};
}

// The degrees of the notes the voices above the bass sound on `line`, begun there or held.
std::vector<Degree> upperNotes(const HumdrumChorale& chorale, int line, const std::string& key) {
    std::vector<Degree> notes;
    for (std::size_t voice = kBassVoice + 1; voice <= chorale.voiceCount(); ++voice) {
        hum::HTp note = chorale.soundingTokenAtLine(kKernKey, voice, line);
        if (isNote(note)) notes.push_back(noteDegree(note, key));
    }
    return notes;
}

bool isMelodic(const RegolaMode& mode, int number) {
    return mode.minor && (number == 6 || number == 7);
}

// The semitones of the raised sixth and seventh of a melodic mode, one more than the natural ones.
int raisedSemitones(int number) {
    return number == 6 ? 9 : 11;
}

// Whether `degree` has the semitones of the scale of `mode`, in a melodic mode the natural or the raised ones of
// its sixth and seventh.
bool fitsScale(const RegolaMode& mode, const Degree& degree) {
    if (!isMelodic(mode, degree.number)) return degree.semitones == mode.scale[degree.number - 1];
    const int raised = raisedSemitones(degree.number);
    return degree.semitones == raised || degree.semitones == raised - 1;
}

// The degree of a chord the bass stands on, nullopt for an alteration. The fourth by its interval, in lydian as in
// major: "4" for the perfect fourth, "4+" for the raised one.
std::optional<std::string> chordDegree(const Ground& ground) {
    if (ground.bass.number == 4 && ground.bass.semitones == 5) return "4";
    if (ground.bass.number == 4 && ground.bass.semitones == 6) return "4+";
    if (!fitsScale(ground.mode, ground.bass)) return std::nullopt;
    return std::to_string(ground.bass.number);
}

// In a melodic mode a raised sixth or seventh of the bass goes up and a natural one does not.
bool followsMelodicMode(const Ground& ground, BassMotion motion) {
    if (!isMelodic(ground.mode, ground.bass.number)) return true;
    const bool raised = ground.bass.semitones == raisedSemitones(ground.bass.number);
    return (motion == BassMotion::Ascending) == raised;
}

// Whether `note` is `figure` of `chord`: on the degree of the figure, with the semitones the key gives it, its
// accidental, or the chord: the leading tone of semitonium-modi, the major sixth of major-sixth.
bool isFigure(const Degree& note, const RegolaFigure& figure, const RegolaChord& chord, const Ground& ground) {
    const Degree& bass = ground.bass;
    if (figure.number == 1) return note.number == bass.number && note.semitones == bass.semitones;
    const int number = (bass.number + figure.number - 2) % 7 + 1;
    if (note.number != number) return false;
    if (figure.alteration != 0) return note.semitones == positiveModulo(ground.mode.scale[number - 1] + figure.alteration, 12);
    if (chord.hasTag(kSemitoniumModi) && number == 7) return note.semitones == raisedSemitones(7);
    if (chord.hasTag(kMajorSixth) && figure.number == 6) return note.semitones == positiveModulo(bass.semitones + 9, 12);
    return fitsScale(ground.mode, note);
}

bool belongsToChord(const RegolaChord& chord, const Degree& note, const Ground& ground) {
    return std::any_of(chord.chordTones.begin(), chord.chordTones.end(),
                       [&](const RegolaFigure& figure) { return isFigure(note, figure, chord, ground); });
}

// Of the allowed chords on `scaleDegree` that allow the mode and one of `motions`, the first that explains the most
// of `notes`.
const RegolaChord* chooseChord(const std::vector<std::size_t>& allowedChords, const std::string& scaleDegree,
                               const std::vector<BassMotion>& motions, const Ground& ground, const std::vector<Degree>& notes) {
    const RegolaChord* chosen = nullptr;
    std::size_t mostExplained = 0;
    for (std::size_t index : allowedChords) {
        const RegolaChord& chord = regolaChords()[index];
        if (chord.scaleDegree != scaleDegree || !chord.allowsBassMotion(motions) || !chord.allowsMode(ground.mode.minor)) continue;
        const auto explained = static_cast<std::size_t>(
            std::count_if(notes.begin(), notes.end(), [&](const Degree& note) { return belongsToChord(chord, note, ground); }));
        if (chosen && explained <= mostExplained) continue;
        if (explained == 0 && !notes.empty()) continue;
        chosen = &chord;
        mostExplained = explained;
    }
    return chosen;
}

// A step to the next degree as a motion, nullopt for anything wider.
std::optional<BassMotion> stepMotion(int degrees) {
    if (degrees == 1) return BassMotion::Ascending;
    if (degrees == -1) return BassMotion::Descending;
    return std::nullopt;
}

} // namespace

std::optional<RegolaMode> regolaMode(const std::string& mode) {
    // The church modes are read as major or as minor by their third; phrygian, with its semitone above the final
    // instead of below, not at all.
    static const std::map<std::string, RegolaMode> modes = {
        {"major", {.scale = {0, 2, 4, 5, 7, 9, 11}}},
        {"minor", {.scale = {0, 2, 3, 5, 7, 8, 10}, .minor = true}},
        {"dorian", {.scale = {0, 2, 3, 5, 7, 9, 10}, .minor = true}},
        {"lydian", {.scale = {0, 2, 4, 6, 7, 9, 11}}},
        {"mixolydian", {.scale = {0, 2, 4, 5, 7, 9, 10}}},
    };
    const auto found = modes.find(mode);
    if (found == modes.end()) return std::nullopt;
    return found->second;
}

std::vector<BassMotion> bassMotions(const HumdrumChorale& chorale, hum::HTp bass) {
    const int from = hum::Convert::kernToBase7(bass);
    std::optional<int> toNext;
    std::optional<int> toNextWeighted;
    for (hum::HTp next = bass->getNextToken(); next && !toNextWeighted; next = next->getNextToken()) {
        if (!isNote(next) || next->isSecondaryTiedNote()) continue;
        const int step = hum::Convert::kernToBase7(next) - from;
        if (step % 7 == 0) continue;
        if (!toNext) toNext = step;
        hum::HTp weight = chorale.tokenAtLine(kMetweightKey, kBassVoice, next->getLineNumber());
        if (weight && *weight != "u") toNextWeighted = step;
    }
    if (!toNext) return {};
    std::vector<BassMotion> motions;
    for (const std::optional<int>& step : {toNext, toNextWeighted}) {
        const std::optional<BassMotion> motion = step ? stepMotion(*step) : std::nullopt;
        if (motion && std::find(motions.begin(), motions.end(), *motion) == motions.end()) motions.push_back(*motion);
    }
    if (motions.empty()) motions.push_back(BassMotion::Leap);
    return motions;
}

RegolaAnalysis::RegolaAnalysis(const HumdrumChorale& chorale, const std::vector<std::string>& tags) {
    const std::vector<std::size_t> allowedChords = allowedRegolaChords(tags);
    hum::HTp bassStart = chorale.spine(kKernKey, kBassVoice);
    for (hum::HTp token = bassStart ? bassStart->getNextToken() : nullptr; token; token = token->getNextToken()) {
        if (!token->isData()) continue;
        const int line = token->getLineNumber();
        const KeyContext* key = chorale.keyContextAtLine(line);
        const std::optional<RegolaMode> mode = key ? regolaMode(key->mode) : std::nullopt;
        hum::HTp bass = chorale.soundingTokenAtLine(kKernKey, kBassVoice, line);
        if (!mode || !isNote(bass)) continue;
        const Ground ground{.bass = noteDegree(bass, key->key), .key = key->key, .mode = *mode};
        const std::optional<std::string> scaleDegree = chordDegree(ground);
        if (!scaleDegree) continue;

        std::vector<BassMotion> motions = bassMotions(chorale, bass);
        const bool goesOn = !motions.empty();
        std::erase_if(motions, [&](BassMotion motion) { return !followsMelodicMode(ground, motion); });
        if (goesOn && motions.empty()) continue;

        const RegolaChord* chord = chooseChord(allowedChords, *scaleDegree, motions, ground, upperNotes(chorale, line, ground.key));
        if (chord) m_soundsByLine[line] = Sound{.ground = ground, .chord = chord};
    }
}

const RegolaAnalysis::Sound* RegolaAnalysis::soundAtLine(int lineNumber) const {
    const auto sound = m_soundsByLine.find(lineNumber);
    return sound == m_soundsByLine.end() ? nullptr : &sound->second;
}

const RegolaChord* RegolaAnalysis::chordAtLine(int lineNumber) const {
    const Sound* sound = soundAtLine(lineNumber);
    return sound ? sound->chord : nullptr;
}

std::optional<bool> RegolaAnalysis::isChordTone(const HumdrumChorale& chorale, std::size_t voice, int lineNumber) const {
    hum::HTp note = chorale.soundingTokenAtLine(kKernKey, voice, lineNumber);
    if (voice == kBassVoice || !isNote(note)) return std::nullopt;
    const Sound* sound = soundAtLine(lineNumber);
    if (!sound) return std::nullopt;
    return belongsToChord(*sound->chord, noteDegree(note, sound->ground.key), sound->ground);
}

std::optional<bool> RegolaAnalysis::isRegolaChord(const HumdrumChorale& chorale, int lineNumber) const {
    const Sound* sound = soundAtLine(lineNumber);
    if (!sound) return std::nullopt;
    const std::vector<Degree> notes = upperNotes(chorale, lineNumber, sound->ground.key);
    return std::all_of(notes.begin(), notes.end(), [&](const Degree& note) { return belongsToChord(*sound->chord, note, sound->ground); });
}

} // namespace choralesearch

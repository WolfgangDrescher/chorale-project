#pragma once

#include <cstddef>
#include <optional>
#include <ostream>
#include <string>
#include <vector>

namespace choralesearch {

// A stretch of the notes of one voice that a result is made of, from one note to another or a single note. A line
// names the note the voice sounds there, which is not attacked there when the voice holds it from before, so the
// voice alone does not tell the note and the line alone does not tell the voice.
struct NoteSection {
    std::string feature;             // e.g. "deg", "mint", or for a linked section the key that names it, e.g. "hint-2"
    std::size_t voice = 0;           // 1-indexed voice (spine); 0 = not set (voices are always 1-4; where 1 is Bass)
    std::string startPosition;       // musical position (quarter notes from the start of the piece getDurationFromStart)
    std::string endPosition;         // same, for the last matched position without the duration of that slice
    std::size_t startLineNumber = 0; // 1-indexed line in the compiled Humdrum file; 0 = not set
    std::size_t endLineNumber = 0;   // same, for the last matched line number

    bool operator==(const NoteSection& other) const {
        return feature == other.feature && voice == other.voice && startLineNumber == other.startLineNumber &&
               endLineNumber == other.endLineNumber;
    }
    bool operator<(const NoteSection& other) const {
        if (startLineNumber != other.startLineNumber) return startLineNumber < other.startLineNumber;
        if (voice != other.voice) return voice < other.voice;
        if (endLineNumber != other.endLineNumber) return endLineNumber < other.endLineNumber;
        return feature < other.feature;
    }
};

// A match: where it was found, as a section of the notes of the voice searched, and what it is about besides.
struct Result : NoteSection {
    std::string choraleId; // e.g. "chor029"

    // The sections of notes this match is made of, in the order of the score: the voice searched, the voices of a
    // hint pair or hint-<voice> key, and the sections of every simultaneousWith match lining up with this one.
    std::vector<NoteSection> linkedSections;

    // Which of a combined array of queries produced this result (see
    // CorpusSearch::run(const std::vector<Query>&)) -- the query's own "id" if it set one,
    // otherwise its position in the array. nullopt when this came from a single, non-array
    // query, where there's nothing to disambiguate.
    std::optional<std::string> queryId;
};

using Results = std::vector<Result>;

inline std::ostream& operator<<(std::ostream& os, const Result& r) {
    return os << r.choraleId << '\t' << r.feature << '\t' << r.voice << '\t' << r.startPosition << '\t' << r.endPosition;
}

inline std::ostream& operator<<(std::ostream& os, const Results& results) {
    for (const auto& r : results)
        os << r << '\n';
    return os;
}

} // namespace choralesearch

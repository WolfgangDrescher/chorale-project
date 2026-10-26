#pragma once

#include <cstddef>
#include <string>
#include <tuple>
#include <vector>

#include "GroupQuery.hpp"
#include "CorpusSearch.hpp"
#include "Group.hpp"

namespace choralesearch {

// What a read of notesOfVoice lists first where the voice holds a note into the match, and last
// where it holds one into the last note of the match. It does not tell groups apart: the members
// of a group show where its voice holds, as a tie.
inline const std::string kHeldNoteValue = "held";

// Which note of the score answers each position of the pattern, and how long each note is where
// the query asks. A group has a query only where all its matches share this.
struct MatchNotes {
    // [0, 0, 1] where one note answers the first two positions: on one line
    // (durationAllowMergedNotes), or as repeated notes whose durations the query asks for
    // (durationAllowSplitNotes).
    std::vector<std::size_t> noteIndexOfPosition;
    std::vector<std::string> noteDurations; // one per note; empty where the query asks for no duration

    bool operator==(const MatchNotes& other) const {
        return std::tie(noteIndexOfPosition, noteDurations) == std::tie(other.noteIndexOfPosition, other.noteDurations);
    }
};

// A match as the search found it, with the score around it.
struct FoundMatch {
    const HumdrumChorale& chorale;
    const Query& query;
    const Result& result;
    MatchNotes notes;

    FoundMatch(const HumdrumChorale& chorale, const Query& query, const Result& result);

    std::size_t lineAt(std::size_t position) const { return result.positionOnsets.at(position).line; }
    std::size_t firstLine() const { return lineAt(0); }
    std::size_t lastLine() const { return result.positionOnsets.back().line; }

    // The duration of the note at `position`, over all the positions it answers.
    std::string durationOfNoteAt(std::size_t position) const;
};

// A match as the group query read it: what it is grouped by.
struct MatchReading {
    std::string choraleId;
    std::vector<LevelValues> valuesByLevel;        // what the reads of each level found, in their categories
    std::vector<LevelValues> writtenValuesByLevel; // the same as written, for the members
    MatchNotes notes;
    GroupExample example;
};

// The positions `read` reads that begin a note, so a note is read once.
std::vector<std::size_t> positionsStartingANote(const FeatureRead& read, const std::vector<std::size_t>& noteIndexOfPosition);

// `group query` as `variant` reads it.
GroupQuery applyVariant(const GroupQuery& groupQuery, const GroupQueryVariant& variant);

// Searches the corpus once and groups the matches; the group returned holds all of them.
Group groupMatchesInCorpus(const CorpusSearch& search, const GroupQuery& groupQuery);

} // namespace choralesearch

#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include "CorpusSearch.hpp"
#include "Grouping.hpp"
#include "Query.hpp"

namespace choralesearch {

// Builds the query that finds exactly the matches of a group: its search query with its
// notes as the matches answer the pattern, asking for what the group was read with. There is none
// where that cannot be asked for.
class GroupMatchQueryBuilder {
public:
    GroupMatchQueryBuilder(const Query& searchQuery, const MatchNotes& notes);

    // Asks for what `read` found: for a group, or as written for a member of one.
    void askFor(const FeatureRead& read, const std::vector<std::string>& values, bool asWritten);
    // Asks for what the reads of a level found.
    void askForLevel(const std::vector<FeatureRead>& reads, const LevelValues& values, bool asWritten);

    std::optional<Query> query() const { return m_askable ? std::optional<Query>(m_query) : std::nullopt; }

private:
    const Query& m_searchQuery;
    const MatchNotes& m_notes;
    Query m_query;
    // Whether the positions stay, or become one per note.
    bool m_keepsPositions;
    bool m_askable = true;

    std::size_t queryPositionOf(std::size_t position) const;
    void askForNotesAtPositions();
    void askForOneNotePerPosition();
    void askForNotesOfVoice(const FeatureRead& read, const std::vector<std::string>& intervals);
    void askForPatternPositions(const FeatureRead& read, const std::vector<std::string>& values);
};

// A message for every group or member below `root` whose query finds other matches than it counts.
std::vector<std::string> verifyGroupMatchQueries(const CorpusSearch& search, const Group& root);

} // namespace choralesearch

#include "GroupMatchQueries.hpp"

#include <algorithm>
#include <map>
#include <utility>

namespace choralesearch {

GroupMatchQueryBuilder::GroupMatchQueryBuilder(const Query& searchQuery, const MatchNotes& notes)
    : m_searchQuery(searchQuery), m_notes(notes), m_query(searchQuery), m_keepsPositions(searchQuery.durationAllowSplitNotes) {
    m_query.id.reset();
    if (m_keepsPositions) {
        askForNotesAtPositions();
    } else {
        askForOneNotePerPosition();
    }
}

void GroupMatchQueryBuilder::askFor(const FeatureRead& read, const std::vector<std::string>& values, bool asWritten) {
    if (!m_askable) return;
    if (read.groupsSeveralValues() && !asWritten) {
        m_askable = false; // asked for by the queries of its members
    } else if (read.notesOfVoice) {
        askForNotesOfVoice(read, values);
    } else {
        askForPatternPositions(read, values);
    }
}

void GroupMatchQueryBuilder::askForLevel(const std::vector<FeatureRead>& reads, const LevelValues& values, bool asWritten) {
    for (std::size_t r = 0; r < reads.size(); ++r) askFor(reads[r], values[r], asWritten);
}

std::size_t GroupMatchQueryBuilder::queryPositionOf(std::size_t position) const {
    return m_keepsPositions ? position : m_notes.noteIndexOfPosition[position];
}

// With durationAllowSplitNotes the positions stay, each continuing the note before it ("mint":
// "P1") or starting a new one ("!mint": "P1").
void GroupMatchQueryBuilder::askForNotesAtPositions() {
    for (const auto& position : m_searchQuery.pattern) {
        const auto duration = position.find("duration");
        if (duration != position.end() && duration->second.size() != 1) m_askable = false;
    }
    for (std::size_t p = 1; p < m_query.pattern.size(); ++p) {
        if (!m_searchQuery.pattern[p - 1].count("duration") || !m_searchQuery.pattern[p].count("duration")) continue;
        const bool sameNote = m_notes.noteIndexOfPosition[p] == m_notes.noteIndexOfPosition[p - 1];
        m_query.pattern[p][sameNote ? "mint" : "!mint"] = {PatternValue("P1")};
    }
}

// One position per note, with the duration of its note where the group query asks for one.
void GroupMatchQueryBuilder::askForOneNotePerPosition() {
    m_query.durationAllowMergedNotes = false;
    m_query.pattern.assign(m_notes.noteDurations.size(), AttributeMap{});
    for (std::size_t p = 0; p < m_searchQuery.pattern.size(); ++p) {
        const std::size_t note = m_notes.noteIndexOfPosition[p];
        for (const auto& [key, patternValues] : m_searchQuery.pattern[p]) {
            if (key != "duration") {
                m_query.pattern[note].emplace(key, patternValues);
            } else if (m_notes.noteDurations[note].empty()) {
                m_askable = false;
            } else {
                m_query.pattern[note]["duration"] = {PatternValue(m_notes.noteDurations[note])};
            }
        }
    }
}

// The intervals of the other voice, as a simultaneous group spanning the match. A note held into
// the match or into its last note has no onset the group could begin or end with.
void GroupMatchQueryBuilder::askForNotesOfVoice(const FeatureRead& read, const std::vector<std::string>& intervals) {
    if (std::find(intervals.begin(), intervals.end(), kHeldNoteValue) != intervals.end()) {
        m_askable = false;
        return;
    }
    SimultaneousGroup voiceLine;
    voiceLine.feature = read.feature;
    voiceLine.voices = std::to_string(*read.notesOfVoice);
    voiceLine.pattern.push_back({{read.feature, {PatternValue("*")}}});
    for (const std::string& interval : intervals) voiceLine.pattern.push_back({{read.feature, {PatternValue(interval)}}});
    voiceLine.simultaneousAlignment = "start-end";
    m_query.simultaneousWith.push_back(std::move(voiceLine));
}

void GroupMatchQueryBuilder::askForPatternPositions(const FeatureRead& read, const std::vector<std::string>& values) {
    const std::vector<std::size_t> positions = positionsStartingANote(read, m_notes.noteIndexOfPosition);
    // With the positions kept, the pattern already asks for the durations where it has them.
    if (m_keepsPositions && read.feature == "duration") {
        for (std::size_t position : positions) {
            if (!m_searchQuery.pattern[position].count("duration")) m_askable = false;
        }
        return;
    }
    for (std::size_t i = 0; i < positions.size() && m_askable; ++i) {
        if (values[i].empty()) {
            m_askable = false; // a held note
        } else {
            m_query.pattern[queryPositionOf(positions[i])][read.feature] = {PatternValue(values[i])};
        }
    }
}

namespace {

// The query of a group or a member, and how many matches it should find.
struct QueryCheck {
    Query query;
    std::size_t expectedMatchCount = 0;
    std::string label;
};

// The checks of the groups and members below `group`, each query named by its place: "3.2" is
// the second group below the third, "1~2" the second member of the first.
void addQueryChecksBelow(const Group& group, const std::string& place, std::vector<QueryCheck>& checks) {
    const auto add = [&](Query query, std::size_t expectedMatchCount, const std::string& label, const std::string& at) {
        query.id = at;
        checks.push_back({std::move(query), expectedMatchCount, label});
    };
    for (std::size_t i = 0; i < group.groups.size(); ++i) {
        const Group& groupBelow = group.groups[i];
        const std::string placeBelow = place.empty() ? std::to_string(i + 1) : place + "." + std::to_string(i + 1);
        if (groupBelow.query) add(*groupBelow.query, groupBelow.matchCount, groupBelow.label, placeBelow);
        for (std::size_t k = 0; k < groupBelow.members.size(); ++k) {
            const GroupMember& member = groupBelow.members[k];
            if (member.query) add(*member.query, member.matchCount, member.label, placeBelow + "~" + std::to_string(k + 1));
        }
        addQueryChecksBelow(groupBelow, placeBelow, checks);
    }
}

} // namespace

std::vector<std::string> verifyGroupMatchQueries(const CorpusSearch& search, const Group& root) {
    std::vector<QueryCheck> checks;
    addQueryChecksBelow(root, "", checks);
    if (checks.empty()) return {};
    // All queries in one search, told apart by their place.
    std::vector<Query> queries;
    for (const QueryCheck& check : checks) queries.push_back(check.query);
    std::map<std::string, std::size_t> foundMatchCount;
    search.forEachMatch(queries, [&](const HumdrumChorale&, const Result& match) { foundMatchCount[match.queryId.value_or("")]++; });

    std::vector<std::string> messages;
    for (const QueryCheck& check : checks) {
        const std::string& place = *check.query.id;
        if (foundMatchCount[place] == check.expectedMatchCount) continue;
        messages.push_back("group " + place + " (" + check.label + ") counts " + std::to_string(check.expectedMatchCount) +
                           " matches, its query finds " + std::to_string(foundMatchCount[place]));
    }
    return messages;
}

} // namespace choralesearch

#include "Grouping.hpp"

#include <algorithm>
#include <map>
#include <set>
#include <utility>

#include "AttributeMatcher.hpp"
#include "GroupMatchQueries.hpp"
#include "GroupingExamples.hpp"
#include "HintPairs.hpp"
#include "HumdrumChorale.hpp"
#include "JsonIO.hpp"
#include "Segmentation.hpp"

namespace choralesearch {

namespace {

using Matches = std::vector<const MatchReading*>;

// ---- Intervals and durations as they are grouped and shown ----

// "-P5", "+P4" and "-P12" as "+P4"; empty for a unison or an octave.
std::string intervalIgnoringOctave(const std::string& value) {
    const std::optional<IntervalParts> interval = parseMintValue(reduceMintValue(value));
    if (!interval || interval->quality.empty() || interval->number.empty()) return value;
    const int number = std::stoi(interval->number); // 1 to 8
    if ((number == 1 || number == 8) && interval->quality == "P") return "";
    if (number == 8) return interval->direction + interval->quality + "1";
    if (number < 5) return interval->direction + interval->quality + interval->number;
    return complementMintValue(interval->direction + interval->quality + interval->number).value_or(value);
}

// A duration as its note symbol: "4" as a quarter note, "2." as a dotted half note.
std::string noteSymbolOfDuration(const std::string& recip) {
    static const std::map<std::string, std::string> notes = {
        {"1", "\U0001D15D"}, {"2", "\U0001D15E"}, {"4", "\U0001D15F"}, {"8", "\U0001D160"}, {"16", "\U0001D161"}};
    const std::size_t dots = recip.find('.');
    const auto note = notes.find(recip.substr(0, dots));
    if (note == notes.end()) return recip;
    std::string symbol = note->second;
    if (dots != std::string::npos) {
        for (std::size_t i = dots; i < recip.size(); ++i) symbol += "\U0001D16D";
    }
    return symbol;
}

// A held note as shown: a tie (U+203F) hugging the interval next to it, "‿P1" or "+P5‿".
const std::string kHeldNoteSymbol = "‿";

// ---- Reading a match ----

// The notes a voice attacks as the search sees them, without the ornaments it skips.
std::vector<AttributeMatcher::Onset> onsetsSeenBySearch(const HumdrumChorale& chorale, const std::string& feature, std::size_t voice,
                                                        const Query& query) {
    MatcherOptions options;
    options.metweightSkipUnclassified = query.metweightSkipUnclassified;
    return AttributeMatcher(feature, {}, options).buildOnsets(chorale, voice);
}

// Whether the note on `line` repeats the pitch of the note on `previousLine`.
bool repeatsPitch(const HumdrumChorale& chorale, std::size_t voice, std::size_t previousLine, std::size_t line) {
    hum::HTp previous = chorale.tokenAtLine("kern", voice, static_cast<int>(previousLine));
    hum::HTp current = chorale.tokenAtLine("kern", voice, static_cast<int>(line));
    if (!previous || !current || previous->isRest() || current->isRest()) return false;
    return hum::Convert::kernToBase40(std::string(*previous)) == hum::Convert::kernToBase40(std::string(*current));
}

// The intervals of the other voice across the match; kHeldNoteValue where it holds a note into it.
// As written, an interval is not replaced by its complement.
std::vector<std::string> intervalsInOwnRhythm(const FeatureRead& read, const FoundMatch& match, bool asWritten) {
    std::vector<std::string> intervals;
    bool attacksFirstNote = false;
    bool attacksLastNote = false;
    for (const auto& onset : onsetsSeenBySearch(match.chorale, read.feature, *read.notesOfVoice, match.query)) {
        const auto line = static_cast<std::size_t>(onset.token->getLineNumber());
        if (line < match.firstLine()) continue;
        if (line > match.lastLine()) break;
        if (line == match.firstLine()) {
            attacksFirstNote = true;
            if (match.firstLine() != match.lastLine()) continue;
        }
        std::string interval = onset.mint ? *onset.mint : std::string(*onset.token);
        if (read.mintAllowIntervalComplementation && !asWritten) {
            interval = intervalIgnoringOctave(interval);
            if (interval.empty()) continue; // a repeated note
        }
        attacksLastNote = line == match.lastLine();
        intervals.push_back(std::move(interval));
    }
    if (!attacksFirstNote) intervals.insert(intervals.begin(), kHeldNoteValue);
    if (!attacksLastNote) intervals.push_back(kHeldNoteValue);
    return intervals;
}

// The interval into the note on `line` as the search measured it, across the ornaments it skipped.
std::string intervalAsSearched(const FoundMatch& match, std::size_t line) {
    for (const auto& onset : onsetsSeenBySearch(match.chorale, "mint", match.result.voice, match.query)) {
        if (static_cast<std::size_t>(onset.token->getLineNumber()) != line) continue;
        return onset.mint ? *onset.mint : std::string(*onset.token);
    }
    return "";
}

// The value of `read` at `position`; empty for a note the voice holds, as the search finds none.
std::string valueAt(const FeatureRead& read, const FoundMatch& match, std::size_t position) {
    if (read.feature == "duration") return match.durationOfNoteAt(position);
    const std::size_t line = match.lineAt(position);
    if (read.feature == "mint" && match.query.metweightSkipUnclassified) return intervalAsSearched(match, line);
    // The hint pair spines belong to the bass.
    const std::size_t spineVoice = isHintPairFeature(read.feature) ? 1 : match.result.voice;
    hum::HTp token = match.chorale.tokenAtLine(read.feature, spineVoice, static_cast<int>(line));
    return token ? std::string(*token) : "";
}

std::vector<std::string> withoutHeldNotes(std::vector<std::string> values) {
    values.erase(std::remove(values.begin(), values.end(), kHeldNoteValue), values.end());
    return values;
}

// The values of `read` at the match, compared as it asks and put into its categories; as written,
// without the categories and complements and with the held notes.
std::vector<std::string> valuesOf(const FeatureRead& read, const FoundMatch& match, bool asWritten) {
    std::vector<std::string> values;
    if (read.notesOfVoice) {
        values = intervalsInOwnRhythm(read, match, asWritten);
        if (!asWritten) values = withoutHeldNotes(values);
    } else {
        for (std::size_t position : positionsStartingANote(read, match.notes.noteIndexOfPosition)) values.push_back(valueAt(read, match, position));
    }
    for (std::string& value : values) {
        if (value.empty() || value == kHeldNoteValue) continue; // a held note has no value
        if (isHintPairFeature(read.feature) && match.query.hintReduceCompound) value = reduceHintInterval(value);
        if (read.ignoreQuality && (read.feature == "mint" || isHintPairFeature(read.feature))) value = withoutQuality(value);
        if (!read.categories.empty() && !asWritten) value = read.categories.categoryNameFor(value);
    }
    return values;
}

LevelValues valuesOfReads(const std::vector<FeatureRead>& reads, const FoundMatch& match, bool asWritten) {
    LevelValues values;
    for (const FeatureRead& read : reads) values.push_back(valuesOf(read, match, asWritten));
    return values;
}

bool anyReadGroupsSeveralValues(const std::vector<FeatureRead>& reads) {
    return std::any_of(reads.begin(), reads.end(), [](const FeatureRead& read) { return read.groupsSeveralValues(); });
}

MatchReading readMatch(const FoundMatch& match, const GroupQuery& groupQuery, const std::set<std::size_t>& voicesReadInOwnRhythm) {
    MatchReading reading;
    reading.choraleId = match.result.choraleId;
    for (const GroupingLevel& level : groupQuery.levels) {
        LevelValues values = valuesOfReads(level.reads, match, false);
        reading.writtenValuesByLevel.push_back(anyReadGroupsSeveralValues(level.reads) ? valuesOfReads(level.reads, match, true) : values);
        reading.valuesByLevel.push_back(std::move(values));
    }
    reading.notes = match.notes;
    // Only where examples are shown, as it walks the score.
    if (groupQuery.examples.maxCount > 0) reading.example = exampleOf(match, voicesReadInOwnRhythm);
    return reading;
}

// ---- Labels ----

std::string valueAsShown(const FeatureRead& read, const std::string& value) {
    if (value.empty()) return ".";
    if (value == kHeldNoteValue) return kHeldNoteSymbol;
    return read.feature == "duration" ? noteSymbolOfDuration(value) : value;
}

// The values of a read in one string: "-M2 +m2", a tie hugging the interval next to it.
std::string joinedValues(const std::vector<std::string>& shownValues) {
    std::string text;
    for (const std::string& value : shownValues) {
        const bool tiedBefore = text.size() >= kHeldNoteSymbol.size() && text.compare(text.size() - kHeldNoteSymbol.size(), std::string::npos, kHeldNoteSymbol) == 0;
        const bool ties = value == kHeldNoteSymbol || tiedBefore;
        if (!text.empty() && !ties) text += " ";
        text += value;
    }
    return text;
}

// The values of each read in turn: "-M2 +m2 · s w".
std::string labelByRead(const std::vector<FeatureRead>& reads, const LevelValues& values) {
    std::string label;
    for (std::size_t r = 0; r < values.size(); ++r) {
        std::vector<std::string> shownValues;
        for (const std::string& value : values[r]) shownValues.push_back(valueAsShown(reads[r], value));
        label += (label.empty() ? "" : " · ") + joinedValues(shownValues);
    }
    return label;
}

// The values note by note as a melody reads, the interval into a note before its duration:
// "(quarter note) -M2 (half note)".
std::string labelFollowingNotes(const std::vector<FeatureRead>& reads, const LevelValues& values,
                                const std::vector<std::size_t>& noteIndexOfPosition) {
    std::map<std::size_t, std::vector<std::string>> shownByNote;
    for (const bool durations : {false, true}) {
        for (std::size_t r = 0; r < reads.size(); ++r) {
            if ((reads[r].feature == "duration") != durations) continue;
            const std::vector<std::size_t> positions = positionsStartingANote(reads[r], noteIndexOfPosition);
            for (std::size_t i = 0; i < values[r].size() && i < positions.size(); ++i) {
                shownByNote[noteIndexOfPosition[positions[i]]].push_back(valueAsShown(reads[r], values[r][i]));
            }
        }
    }
    std::vector<std::string> shownValues;
    for (const auto& [note, shown] : shownByNote) shownValues.insert(shownValues.end(), shown.begin(), shown.end());
    return joinedValues(shownValues);
}

// The label of a group: its values as shown, following the notes of `noteIndexOfPosition` where a
// read of the durations lets them.
std::string labelOf(const std::vector<FeatureRead>& reads, const LevelValues& values, const std::vector<std::size_t>& noteIndexOfPosition) {
    const auto readsDurations = [](const FeatureRead& read) { return read.feature == "duration"; };
    const auto readsNotesOfVoice = [](const FeatureRead& read) { return read.notesOfVoice.has_value(); };
    const bool followsNotes = !noteIndexOfPosition.empty() && std::any_of(reads.begin(), reads.end(), readsDurations) &&
                              std::none_of(reads.begin(), reads.end(), readsNotesOfVoice);
    return followsNotes ? labelFollowingNotes(reads, values, noteIndexOfPosition) : labelByRead(reads, values);
}

// ---- Building the groups ----

// The matches by the values `valuesOf` picks of each, in the order of the values.
template <typename ValuesOf>
std::map<LevelValues, Matches> matchesByValues(const Matches& matches, ValuesOf valuesOf) {
    std::map<LevelValues, Matches> byValues;
    for (const MatchReading* match : matches) byValues[valuesOf(*match)].push_back(match);
    return byValues;
}

std::size_t choraleCountOf(const Matches& matches) {
    std::set<std::string> choraleIds;
    for (const MatchReading* match : matches) choraleIds.insert(match->choraleId);
    return choraleIds.size();
}

// The most matches first, then the most chorales; ties keep the order of the values.
std::vector<const Matches*> mostFrequentFirst(const std::map<LevelValues, Matches>& byValues) {
    std::vector<std::pair<const Matches*, std::size_t>> ranked; // with the chorale count
    for (const auto& [values, matches] : byValues) ranked.emplace_back(&matches, choraleCountOf(matches));
    std::stable_sort(ranked.begin(), ranked.end(), [](const auto& a, const auto& b) {
        if (a.first->size() != b.first->size()) return a.first->size() > b.first->size();
        return a.second > b.second;
    });
    std::vector<const Matches*> order;
    for (const auto& [matches, choraleCount] : ranked) order.push_back(matches);
    return order;
}

bool answerWithTheSameNotes(const Matches& matches) {
    return std::all_of(matches.begin(), matches.end(), [&](const MatchReading* match) { return match->notes == matches.front()->notes; });
}

// The query that finds exactly the matches of a group of `level`, which share the values of the
// levels down to it.
std::optional<Query> queryOfGroup(const GroupQuery& groupQuery, const Matches& matches, std::size_t level) {
    if (!answerWithTheSameNotes(matches)) return std::nullopt;
    const MatchReading& first = *matches.front();
    GroupMatchQueryBuilder builder(groupQuery.query, first.notes);
    for (std::size_t l = 0; l < level; ++l) builder.askForLevel(groupQuery.levels[l].reads, first.valuesByLevel[l], false);
    return builder.query();
}

// The same for a member of a group of `level`: read as written at that level.
std::optional<Query> queryOfMember(const GroupQuery& groupQuery, const Matches& matches, std::size_t level) {
    if (!answerWithTheSameNotes(matches)) return std::nullopt;
    const MatchReading& first = *matches.front();
    GroupMatchQueryBuilder builder(groupQuery.query, first.notes);
    for (std::size_t l = 0; l + 1 < level; ++l) builder.askForLevel(groupQuery.levels[l].reads, first.valuesByLevel[l], false);
    builder.askForLevel(groupQuery.levels[level - 1].reads, first.writtenValuesByLevel[level - 1], true);
    return builder.query();
}

Group buildGroup(const GroupQuery& groupQuery, const Matches& matches, std::size_t level);

// The members of a group of several values, the most frequent first. A category keeps its name,
// any other group is named after its most frequent member, without its held notes, and the group
// shows the preferred example of each member.
void addMembers(Group& group, const GroupQuery& groupQuery, const Matches& matches, std::size_t level) {
    const GroupingLevel& ownLevel = groupQuery.levels[level - 1];
    const std::map<LevelValues, Matches> byWrittenValues =
        matchesByValues(matches, [&](const MatchReading& match) { return match.writtenValuesByLevel[level - 1]; });
    const std::vector<const Matches*> members = mostFrequentFirst(byWrittenValues);
    for (const Matches* member : members) {
        const LevelValues& values = member->front()->writtenValuesByLevel[level - 1];
        group.members.push_back({values, labelOf(ownLevel.reads, values, {}), member->size(), choraleCountOf(*member),
                                 queryOfMember(groupQuery, *member, level)});
    }
    const bool readsCategories = std::any_of(ownLevel.reads.begin(), ownLevel.reads.end(),
                                             [](const FeatureRead& read) { return !read.categories.empty(); });
    if (!readsCategories) {
        LevelValues spelling = group.members.front().values;
        for (std::vector<std::string>& values : spelling) values = withoutHeldNotes(values);
        group.label = labelOf(ownLevel.reads, spelling, {});
    }
    // A group of one value is its own member, unless the member tells something the group does not.
    if (group.members.size() == 1 && group.members.front().label == group.label) {
        group.query = group.members.front().query;
        group.members.clear();
        return;
    }
    group.examples.clear();
    for (std::size_t i = 0; i < members.size() && group.examples.size() < groupQuery.examples.maxCount; ++i) {
        group.examples.push_back(preferredExampleOf(*members[i]));
        group.examples.back().groupLabel = group.members[i].label;
    }
}

// The groups of the next level, the most frequent first, as many as the level keeps. A group
// with groups below it shows the first example of each.
void addGroupsBelow(Group& group, const GroupQuery& groupQuery, const Matches& matches, std::size_t level) {
    const GroupingLevel& nextLevel = groupQuery.levels[level];
    const std::map<LevelValues, Matches> byValues =
        matchesByValues(matches, [&](const MatchReading& match) { return match.valuesByLevel[level]; });
    for (const Matches* below : mostFrequentFirst(byValues)) {
        if (!nextLevel.keeps(below->size(), group.groups.size())) {
            group.omittedMatchCount += below->size();
            continue;
        }
        group.groups.push_back(buildGroup(groupQuery, *below, level + 1));
    }
    if (group.groups.empty()) return;
    group.examples.clear();
    for (const Group& groupBelow : group.groups) {
        if (groupBelow.examples.empty()) continue;
        group.examples.push_back(groupBelow.examples.front());
        group.examples.back().groupLabel = groupBelow.label;
        if (group.examples.size() == groupQuery.examples.maxCount) break;
    }
}

// The group of `matches`, which share the values of the levels down to `level` (0 for the root),
// with its groups down to the last level.
Group buildGroup(const GroupQuery& groupQuery, const Matches& matches, std::size_t level) {
    Group group;
    group.matchCount = matches.size();
    group.choraleCount = choraleCountOf(matches);
    group.examples = chooseExamples(matches, groupQuery.examples);
    if (level == 0) {
        group.query = groupQuery.query;
        group.query->id.reset();
    } else {
        const GroupingLevel& ownLevel = groupQuery.levels[level - 1];
        const MatchReading& first = *matches.front();
        group.values = first.valuesByLevel[level - 1];
        group.label = labelOf(ownLevel.reads, group.values, first.notes.noteIndexOfPosition);
        group.query = queryOfGroup(groupQuery, matches, level);
        if (anyReadGroupsSeveralValues(ownLevel.reads)) addMembers(group, groupQuery, matches, level);
    }
    if (level < groupQuery.levels.size()) addGroupsBelow(group, groupQuery, matches, level);
    return group;
}

} // namespace

FoundMatch::FoundMatch(const HumdrumChorale& chorale, const Query& query, const Result& result)
    : chorale(chorale), query(query), result(result) {
    std::vector<std::size_t>& noteIndex = notes.noteIndexOfPosition;
    for (std::size_t p = 0; p < result.positionOnsets.size(); ++p) {
        if (p == 0) {
            noteIndex.push_back(0);
            continue;
        }
        const std::size_t previousLine = lineAt(p - 1);
        const std::size_t line = lineAt(p);
        const bool splitNote = query.durationAllowSplitNotes && query.pattern[p - 1].count("duration") &&
                               query.pattern[p].count("duration") && repeatsPitch(chorale, result.voice, previousLine, line);
        const bool sameNote = line == previousLine || splitNote;
        noteIndex.push_back(noteIndex.back() + (sameNote ? 0 : 1));
    }
    for (std::size_t p = 0; p < noteIndex.size(); ++p) {
        if (p > 0 && noteIndex[p] == noteIndex[p - 1]) continue;
        bool durationAsked = false;
        for (std::size_t q = p; q < noteIndex.size() && noteIndex[q] == noteIndex[p]; ++q) {
            durationAsked = durationAsked || query.pattern[q].count("duration");
        }
        notes.noteDurations.push_back(durationAsked ? durationOfNoteAt(p) : "");
    }
}

std::string FoundMatch::durationOfNoteAt(std::size_t position) const {
    const std::vector<std::size_t>& noteIndex = notes.noteIndexOfPosition;
    hum::HumNum duration(0);
    for (std::size_t p = position; p < noteIndex.size() && noteIndex[p] == noteIndex[position]; ++p) {
        duration += result.positionOnsets[p].duration;
    }
    return duration > 0 ? hum::Convert::durationToRecip(duration) : "";
}

std::vector<std::size_t> positionsStartingANote(const FeatureRead& read, const std::vector<std::size_t>& noteIndexOfPosition) {
    std::vector<std::size_t> positions;
    for (std::size_t position = 0; position < noteIndexOfPosition.size(); ++position) {
        const bool readHere = read.positions.empty() ||
                              std::find(read.positions.begin(), read.positions.end(), position) != read.positions.end();
        const bool startsNote = position == 0 || noteIndexOfPosition[position] != noteIndexOfPosition[position - 1];
        if (readHere && startsNote) positions.push_back(position);
    }
    return positions;
}

GroupQuery applyVariant(const GroupQuery& groupQuery, const GroupQueryVariant& variant) {
    GroupQuery varied = groupQuery;
    varied.variants.clear();
    if (!variant.pattern.empty()) {
        nlohmann::json query = queryToJson(groupQuery.query);
        for (std::size_t position = 0; position < variant.pattern.size(); ++position) {
            for (const auto& [key, value] : variant.pattern[position].items()) query["pattern"][position][key] = value;
        }
        varied.query = queryFromJson(query);
    }
    if (variant.ignoreQuality) {
        for (GroupingLevel& level : varied.levels) {
            for (FeatureRead& read : level.reads) read.ignoreQuality = true;
        }
    }
    return varied;
}

Group groupMatchesInCorpus(const CorpusSearch& search, const GroupQuery& groupQuery) {
    std::set<std::size_t> voicesReadInOwnRhythm;
    for (const GroupingLevel& level : groupQuery.levels) {
        for (const FeatureRead& read : level.reads) {
            if (read.notesOfVoice) voicesReadInOwnRhythm.insert(*read.notesOfVoice);
        }
    }
    std::vector<MatchReading> readings;
    search.forEachMatch({groupQuery.query}, [&](const HumdrumChorale& chorale, const Result& result) {
        readings.push_back(readMatch(FoundMatch(chorale, groupQuery.query, result), groupQuery, voicesReadInOwnRhythm));
    });
    Matches all;
    for (const MatchReading& reading : readings) all.push_back(&reading);
    return buildGroup(groupQuery, all, 0);
}

} // namespace choralesearch

#pragma once

#include <map>
#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace choralesearch {

// feature name -> OR-list of acceptable values ("*" anywhere in the list = wildcard)
using AttributeMap = std::map<std::string, std::vector<std::string>>;

// What AttributeMatcher is allowed to treat as equal beyond a literal comparison
struct MatcherOptions {
    bool mintStartAtPreviousToken = false;
    std::vector<std::string> mintAllowIntervalComplementation;
    bool fbCompareExactChord = false;
    bool kernIgnoreOctave = false;
    bool hintReduceCompound = false;
    bool durationAllowSplitNotes = false;
    bool durationAllowMergedNotes = false;
    bool metweightSkipUnclassified = false;
};

// An additional pattern that must have a match starting at the exact same musical position
// (hum::HumNum, quarter notes from the start of the piece) as each match of the query's own
// top-level pattern -- e.g. "whatever the bass is doing when this soprano line happens".
// Only whether a match exists at that position matters; the group's own matches aren't
// themselves returned as results.
struct SimultaneousGroup {
    std::string feature;
    std::string voices = "all";
    std::vector<AttributeMap> pattern;

    // nullopt means "inherit the top-level Query's own value of the same name" -- these
    // are only ever overrides, not independent defaults.
    std::optional<bool> mintStartAtPreviousToken;
    std::optional<std::vector<std::string>> mintAllowIntervalComplementation;
    std::optional<bool> fbCompareExactChord;
    std::optional<bool> kernIgnoreOctave;
    std::optional<bool> hintReduceCompound;
    std::optional<bool> durationAllowSplitNotes;
    std::optional<bool> durationAllowMergedNotes;
    std::optional<bool> metweightSkipUnclassified;
    std::optional<std::string> simultaneousAlignment;
};

struct Query {
    // Only meaningful when this query is run as part of a combined array of queries (see
    // CorpusSearch::run(const std::vector<Query>&)): echoed onto every Result's queryId so
    // the caller can tell which of the combined queries produced it. nullopt means "use this
    // query's own position in the array instead" -- resolved to a string once up front, not
    // re-derived per result. Ignored entirely for a single, non-array query.
    std::optional<std::string> id;

    std::string feature;
    std::string voices = "all";

    std::vector<AttributeMap> pattern;

    // Additional patterns (typically in other voices) that must each have a match starting
    // at the same musical position as a match of the query's own pattern, for that match to
    // be kept. Empty means no such constraint. See SimultaneousGroup.
    std::vector<SimultaneousGroup> simultaneousWith;

    std::optional<std::size_t> limit;

    // Only relevant when feature == "mint": a mint token records the interval *into* a
    // note, so the note the first matched interval starts from is one token earlier than
    // the match itself. When set, that earlier token becomes startLine/startPosition --
    // unless the pattern's first position already pins down the driving feature itself
    // (e.g. an explicit {"mint": "*"}), which signals the caller included that lead-in
    // token in the pattern themselves.
    bool mintStartAtPreviousToken = false;

    // Affects any "mint" key comparison (driving or cross-referenced): the diatonic numbers
    // whose pattern values may also be satisfied by their complementary interval, i.e. the
    // interval filling the rest of the octave in the opposite direction ("-P5" <-> "+P4",
    // "+M2" <-> "-m7"). A number listed here opts in every pattern value *written* with that
    // number, so {"mint": "-5"} with {"5"} also matches "+4"; "*" opts in every number.
    // Empty (the default) means no complementation at all. Listing the unison-octave pair
    // itself ("1", "8" or "*") additionally lets an octave leap continue a
    // durationAllowSplitNotes run (see there).
    std::vector<std::string> mintAllowIntervalComplementation;

    // Only relevant when feature == "fb": by default an fb pattern value's figures are a
    // minimum requirement, so "2 4" also matches a chord actually voiced as "2 4 6". When
    // set, the chord must have exactly as many figures as the pattern value -- no extras.
    bool fbCompareExactChord = false;

    // Affects any "kern" key comparison (driving or cross-referenced): by default, a pitch
    // pattern value's register must match exactly, so "G" only matches "G", not "g"/"GG"/etc.
    // When set, register is ignored -- "G" matches every octave of that pitch class.
    bool kernIgnoreOctave = false;

    // Affects any "hint-<pair>"/"hint-<voice>" key comparison: by default a pattern value's
    // interval size must match exactly, so "3" only matches a genuine third, not a compound
    // third (a tenth, seventeenth, ...). When set, both the actual interval and the pattern
    // value are reduced to their simple (within-octave) equivalent before comparing -- a
    // unison and an octave stay distinct from each other.
    bool hintReduceCompound = false;

    // Affects any "duration" pattern key: by default a position's duration must be the sounding
    // duration of the single onset it is checked against. When set, a position may instead be
    // satisfied by a run of consecutive onsets that re-articulate one and the same note (the
    // same pitch for "kern", a unison step -- or an octave leap, with "1"/"8"/"*" in
    // mintAllowIntervalComplementation -- for "mint", the same token otherwise) and whose
    // durations add up to one of the position's allowed values -- so a pattern asking for a
    // half note also finds it written as two repeated quarters. Every other key of that
    // position must hold for the whole run; "fermata" is checked on its closing onset.
    bool durationAllowSplitNotes = false;

    // The converse of durationAllowSplitNotes, and independent of it: when set, a *single*
    // onset may satisfy several consecutive pattern positions at once, as long as their
    // durations add up to exactly that onset's sounding duration -- so a pattern spelling a
    // note out as two quarters also finds it written as one half note. Every position of such
    // a run is checked against that one onset; the driving feature's own key is only checked
    // on the run's first position, since the later ones describe re-attacks that aren't there.
    bool durationAllowMergedNotes = false;

    // When set, an onset the walked voice attacks on a metrically unclassified position (its
    // **metweight is "u") counts as an ornament rather than as a note of its own: the pattern
    // never sees it, its duration falls to the note it decorates, and the next note's "mint" is
    // measured across it, from the note before it. So one pattern spelling out the plain
    // skeleton of a phrase finds both the plain reading and the diminished one -- a neighbour
    // note, an anticipation, a leap away and back. Rests are never ornaments, and neither is a
    // voice's very first onset (there is no note in front of it to decorate).
    bool metweightSkipUnclassified = false;

    // Only relevant with simultaneousWith: which of a group's own match's positions must
    // line up with the primary match's. One of "start" (default -- just start together),
    // "end" (just end together), or "start-end" (both -- runs for the same duration).
    std::string simultaneousAlignment = "start";
};

inline nlohmann::json patternToJson(const std::vector<AttributeMap>& pattern) {
    nlohmann::json arr = nlohmann::json::array();
    for (const auto& position : pattern) {
        nlohmann::json posJson = nlohmann::json::object();
        for (const auto& [key, values] : position) {
            posJson[key] = values;
        }
        arr.push_back(posJson);
    }
    return arr;
}

inline nlohmann::json simultaneousGroupToJson(const SimultaneousGroup& g) {
    nlohmann::json j;
    j["feature"] = g.feature;
    j["voices"] = g.voices;
    j["pattern"] = patternToJson(g.pattern);
    if (g.mintStartAtPreviousToken) j["mintStartAtPreviousToken"] = *g.mintStartAtPreviousToken;
    if (g.mintAllowIntervalComplementation) j["mintAllowIntervalComplementation"] = *g.mintAllowIntervalComplementation;
    if (g.fbCompareExactChord) j["fbCompareExactChord"] = *g.fbCompareExactChord;
    if (g.kernIgnoreOctave) j["kernIgnoreOctave"] = *g.kernIgnoreOctave;
    if (g.hintReduceCompound) j["hintReduceCompound"] = *g.hintReduceCompound;
    if (g.durationAllowSplitNotes) j["durationAllowSplitNotes"] = *g.durationAllowSplitNotes;
    if (g.durationAllowMergedNotes) j["durationAllowMergedNotes"] = *g.durationAllowMergedNotes;
    if (g.metweightSkipUnclassified) j["metweightSkipUnclassified"] = *g.metweightSkipUnclassified;
    if (g.simultaneousAlignment) j["simultaneousAlignment"] = *g.simultaneousAlignment;
    return j;
}

inline std::ostream& operator<<(std::ostream& os, const Query& q) {
    nlohmann::json j;
    if (q.id) j["id"] = *q.id;
    j["feature"] = q.feature;
    j["voices"] = q.voices;
    j["pattern"] = patternToJson(q.pattern);

    if (q.limit) j["limit"] = *q.limit;
    if (q.mintStartAtPreviousToken) j["mintStartAtPreviousToken"] = true;
    if (!q.mintAllowIntervalComplementation.empty()) j["mintAllowIntervalComplementation"] = q.mintAllowIntervalComplementation;
    if (q.fbCompareExactChord) j["fbCompareExactChord"] = true;
    if (q.kernIgnoreOctave) j["kernIgnoreOctave"] = true;
    if (q.hintReduceCompound) j["hintReduceCompound"] = true;
    if (q.durationAllowSplitNotes) j["durationAllowSplitNotes"] = true;
    if (q.durationAllowMergedNotes) j["durationAllowMergedNotes"] = true;
    if (q.metweightSkipUnclassified) j["metweightSkipUnclassified"] = true;
    if (q.simultaneousAlignment != "start") j["simultaneousAlignment"] = q.simultaneousAlignment;

    if (!q.simultaneousWith.empty()) {
        nlohmann::json groups = nlohmann::json::array();
        for (const auto& g : q.simultaneousWith) groups.push_back(simultaneousGroupToJson(g));
        j["simultaneousWith"] = groups;
    }

    return os << j.dump(1, '\t') << std::endl;
}

} // namespace choralesearch

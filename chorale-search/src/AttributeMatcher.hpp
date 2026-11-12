#pragma once

#include <cstddef>
#include <functional>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "HumdrumChorale.hpp"
#include "Intervals.hpp"
#include "PatternKeys.hpp"
#include "Query.hpp"
#include "Result.hpp"
#include "VerseSet.hpp"

namespace choralesearch {

// A key of a position of the pattern with its values, looked up in the table of keys once.
struct PositionKey : PatternKeyLookup {
    std::vector<PatternValue> values;
    // Another voice under its voice reference ("@1"), checked for all its keys at once.
    std::string otherVoice;
    std::vector<PositionKey> otherVoiceKeys;

    // A key of another voice ("@1") holds while the note sounds, as the harmony does.
    bool checksEveryOnset() const { return !otherVoice.empty() || PatternKeyLookup::checksEveryOnset(); }
};

using PositionKeys = std::vector<PositionKey>;

// A position of the pattern as the matcher holds it.
struct CompiledPosition {
    PositionKeys keys;
    bool hasReferences = false;
    std::optional<PatternRepeat> repeat;
    std::vector<CompiledPosition> subPattern;
    // Where it stands in the written pattern, as RepeatedOnset has it.
    std::vector<std::size_t> path;
};

struct AttributeMatch {
    std::size_t voice;           // 1-indexed voice (spine) the match was found in
    std::size_t startLineNumber; // 1-indexed line in the compiled Humdrum file; 0 = not set
    std::size_t endLineNumber;   // same, for the last matched line number
    hum::HumNum startPosition;   // musical position (quarter notes from the start of the piece getDurationFromStart)
    hum::HumNum endPosition;     // same, for the last matched position without the duration of that slice
    // Where each position of the pattern matched, see PositionOnset.
    std::vector<PositionOnset> positionOnsets;
    // The onsets of the positions that repeat, in order.
    std::vector<RepeatedOnset> repeatedOnsets;
    VerseSet verses = VerseSet::all(); // the verses the match holds in
};

// The **mint token for the step from the note `from` to the note `to` ("+M2", "-m3", "P1"), the
// highest note standing for a chord. Empty where either has no pitch, a rest for one.
std::string mintIntervalToken(hum::HTp from, hum::HTp to);

// A hint interval ("M10") folded back within an octave ("M3"); a unison and an octave stay distinct.
std::string reduceHintInterval(const std::string& value);

// The complement within the octave in the opposite direction ("-P5" -> "+P4"); nullopt for a value
// without a simple interval.
std::optional<std::string> complementMintValue(const std::string& patternValue);

// A **mint interval folded within an octave ("+P12" -> "+P5"); a unison and an octave stay distinct.
std::string reduceMintValue(const std::string& value);

// Whether a figure of a figured-bass pattern value ("4", "P4") is the interval of a voice above the bass as its hint
// spine has it ("P11"): folded within an octave, as the fb spine has it, and of the quality only if the figure names one.
bool fbFigureMatchesInterval(const std::string& figure, const std::string& interval);

// Whether a hint interval ("P11") matches one of the values of a hint key.
bool hintInList(const std::vector<PatternValue>& allowed, const std::string& actual, bool reduceCompound);

// Where a match would begin and end, in quarter notes from the start of the piece, and the verses it would hold in.
struct MatchCandidate {
    hum::HumNum startPosition;
    hum::HumNum endPosition;
    VerseSet verses;
};

// What a match has to meet: where it begins and ends, and whether it is accepted there. Refused, a repeat of the
// pattern goes on to the next place the match could end.
struct MatchConditions {
    std::optional<hum::HumNum> start;
    std::optional<hum::HumNum> end;
    std::function<bool(const MatchCandidate&)> accepts;
};

class AttributeMatcher {
public:
    AttributeMatcher(std::string drivingFeature, std::vector<PatternPosition> pattern,
                      MatcherOptions options = {});

    // The matches in `voice` under `conditions`. With skipOverlappingMatches, none begins inside the one before it.
    std::vector<AttributeMatch> findAll(const HumdrumChorale& chorale, std::size_t voice, const MatchConditions& conditions = {}) const;

    // Whether a position of the pattern repeats: then where a match ends depends on its conditions.
    bool hasRepeats() const { return m_hasRepeats; }

    // Driven by word or syl, or with a lyrics key in the pattern: the matches say which verses they hold in.
    bool readsLyrics() const { return m_readsLyrics; }

    // One onset of the driving spine as the pattern walk sees it. Without
    // MatcherOptions::metweightSkipUnclassified that is just the spine's own token and its own
    // sounding duration. With it, ornamental onsets are gone from the walk entirely and survive
    // only in what they changed about their neighbours: the duration of the note they decorate,
    // and the interval into the note that follows them -- see buildOnsets().
    // Driven by word or syl, an onset is a whole word or syllable: its duration is that of all its notes.
    struct Onset {
        hum::HTp token;
        hum::HumNum duration;            // own sounding duration, plus that of any ornament skipped after it
        std::optional<std::string> mint; // interval measured across skipped ornaments; nullopt = ask the spine
        hum::HTp lastToken = nullptr;    // the last note of a word or a syllable on several, else the token itself
        VerseSet verses = VerseSet::all(); // driven by word or syl, the verses with this word or syllable here
    };

    // The driving spine's onsets in order, with ornaments already folded away when
    // MatcherOptions::metweightSkipUnclassified asks for it. Rests are never ornaments (their
    // **metweight is unclassified regardless of where they fall), and neither is a voice's very
    // first onset -- there is no note in front of it whose duration it could belong to. Driven by
    // word or syl, the words or syllables of every verse of the soprano, each span once; another voice has none.
    std::vector<Onset> buildOnsets(const HumdrumChorale& chorale, std::size_t voice) const;

    // What a position of the pattern was matched on, for the position references that read it: a copy of the
    // onset, with the duration the position took (the sum of a split run, the share of a merged one) and, for
    // a later position of a merged run, the unison of the re-attack it describes as its mint.
    using MatchedOnsets = std::vector<std::optional<Onset>>;

private:
    // Walks the pattern along `onsets`, the matches in their order.
    std::vector<AttributeMatch> walkPattern(const HumdrumChorale& chorale, std::size_t voice, const std::vector<Onset>& onsets,
                                            const MatchConditions& conditions) const;

    // The pattern matched from one start onset on, position by position. A repeat tries its counts one after the
    // other, and the states the rest of the pattern failed from are kept for the starts after it.
    struct Walk {
        const HumdrumChorale& chorale;
        std::size_t voice;
        const std::vector<Onset>& onsets;
        const MatchConditions& conditions;
        hum::HumNum startPosition = 0;
        std::vector<PositionOnset> positionOnsets;
        // The onsets the positions that repeat took so far, each with the position that took it.
        std::vector<std::pair<const CompiledPosition*, std::size_t>> repeatedOnsets;
        MatchedOnsets matched;
        VerseSet verses = VerseSet::all();
        std::size_t end = 0; // the index after the last onset of the match
        std::set<std::vector<std::size_t>> failedStates;
    };
    using Continuation = std::function<bool(std::size_t onsetIndex)>;

    // Whether the positions from `offset` on match from the onset `onsetIndex` on.
    bool matchFrom(Walk& walk, std::size_t offset, std::size_t onsetIndex) const;
    // The same for a position without repeat, with durationAllowSplitNotes and durationAllowMergedNotes.
    bool matchSinglePosition(Walk& walk, std::size_t offset, std::size_t onsetIndex) const;
    // A repeat that has matched `count` times from the onset `runStart` on: stops and hands the onset after its
    // run to `next`, or matches once more. `offset` is its place in the pattern, none in a subPattern.
    bool matchRepeat(Walk& walk, const CompiledPosition& position, std::optional<std::size_t> offset, std::size_t count,
                     std::size_t runStart, std::size_t onsetIndex, const Continuation& next) const;
    // Matches `position` once, its keys on one onset or its subPattern, then `next`.
    bool matchOnce(Walk& walk, const CompiledPosition& position, std::optional<std::size_t> offset, std::size_t onsetIndex,
                   const Continuation& next) const;
    // The positions of a subPattern from `index` on, then `next`.
    bool matchSubPattern(Walk& walk, const std::vector<CompiledPosition>& positions, std::size_t index, std::size_t onsetIndex,
                         const Continuation& next) const;
    bool matchesAllKeys(const HumdrumChorale& chorale, std::size_t voice, const Onset& onset, const PositionKeys& position) const;
    // Whether the lyrics keys of `position` hold in some verse at `line`, narrowing the verses of the walk.
    bool narrowVerses(Walk& walk, const PositionKeys& position, int line) const;
    // What the walk from `offset` on depends on besides the onset: the onsets later positions read, the verses.
    std::vector<std::size_t> walkState(const Walk& walk, std::size_t offset, std::size_t count, std::size_t onsetIndex) const;

    CompiledPosition compilePosition(const PatternPosition& position, std::vector<std::size_t> path);

    // Judges a single pattern key (possibly "!"-negated) against a single onset, with the
    // negation already applied to the returned value. nullopt means the key couldn't be
    // judged at all because the spine or token it needs isn't there -- that fails the
    // position outright, negation included.
    std::optional<bool> matchKey(const HumdrumChorale& chorale, std::size_t voice, const Onset& onset,
                                  const PositionKey& positionKey) const;

    // What the score has for a key at `onset`: the text a value is held against, and the token it is read from
    // where there is one. nullopt where the spine has no token there.
    struct KeyValue {
        std::string text;
        hum::HTp token = nullptr;
    };
    std::optional<KeyValue> valueOfKey(const HumdrumChorale& chorale, std::size_t voice, const Onset& onset, PatternKeyId id,
                                       const std::string& key) const;

    // What a position reference reads at the onset its position was matched on, spelled as a value of the key
    // it reads ("4f#", "-M2", "+", "6 3"). nullopt where there is nothing to read: a rest, the first note of a
    // voice for mint, a missing spine.
    std::optional<std::string> readPositionReference(const HumdrumChorale& chorale, std::size_t voice,
                                                     const PositionReference& reference, const MatchedOnsets& matched) const;

    // `position` with each position reference replaced by what it reads, or dropped where it reads nothing. The
    // lyrics are left as they are: they are read verse by verse, see walkPattern.
    PositionKeys withReferencesRead(const HumdrumChorale& chorale, std::size_t voice, const PositionKeys& position,
                                    const MatchedOnsets& matched) const;

    // Consumes one or more consecutive onsets starting at onsets[onsetIndex] that together
    // make up one "logical note" for this pattern position, until their summed duration hits
    // one of the position's allowed "duration" values. Returns how many onsets were consumed,
    // or nullopt if no prefix of the run lands exactly on an allowed duration before
    // overshooting all of them or leaving the logical note. Only used when
    // MatcherOptions::durationAllowSplitNotes is set.
    std::optional<std::size_t> matchSplitPosition(const HumdrumChorale& chorale, std::size_t voice,
                                                   const std::vector<Onset>& onsets, std::size_t onsetIndex,
                                                   const PositionKeys& position) const;

    // Judges a "mint" or "kern" key (possibly "!"-negated) that a merged run's later position
    // states about the re-attack the merged onset doesn't have, against what that re-attack
    // *would* have been rather than against the onset's own token -- see the two ...ReAttackInList
    // helpers. Same contract as matchKey otherwise: negation applied, nullopt when the key
    // can't be judged at all.
    std::optional<bool> matchReAttackKey(const HumdrumChorale& chorale, std::size_t voice, const Onset& onset,
                                          const PositionKey& positionKey) const;

    // The converse of matchSplitPosition: consumes one or more consecutive pattern positions
    // starting at m_pattern[patternIndex] that together describe the single onset `tok`,
    // until their durations have used up exactly `remaining` (the onset's sounding duration,
    // minus whatever earlier positions of the run already claimed). Returns how many positions
    // were consumed, or nullopt if no prefix of them divides the onset up exactly before
    // overshooting or running into a position without a concrete duration. isContinuation says
    // whether m_pattern[patternIndex] is itself already part of a started run. Only used when
    // MatcherOptions::durationAllowMergedNotes is set.
    std::optional<std::size_t> matchMergedPositions(const HumdrumChorale& chorale, std::size_t voice,
                                                     const Onset& onset, std::size_t patternIndex,
                                                     hum::HumNum remaining, bool isContinuation, MatchedOnsets& matched) const;

    bool isDrivenByLyrics() const { return m_drivingFeatureId == PatternKeyId::Word || m_drivingFeatureId == PatternKeyId::Syl; }

    std::string m_drivingFeature;
    std::optional<PatternKeyId> m_drivingFeatureId; // nullopt for an unknown feature
    std::vector<CompiledPosition> m_pattern;
    std::vector<std::vector<std::size_t>> m_earlierPositionsStillRead; // per position: those before it that it or a later one reads
    std::size_t m_minimumOnsets = 0;
    bool m_hasRepeats = false; // a position of the pattern repeats
    MatcherOptions m_options;
    bool m_readsLyrics = false; // driven by word or syl, or with a lyrics key in the pattern
};

} // namespace choralesearch

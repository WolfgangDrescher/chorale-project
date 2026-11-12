#pragma once

#include <cstddef>
#include <optional>
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
};

using PatternPosition = std::vector<PositionKey>;

struct AttributeMatch {
    std::size_t voice;           // 1-indexed voice (spine) the match was found in
    std::size_t startLineNumber; // 1-indexed line in the compiled Humdrum file; 0 = not set
    std::size_t endLineNumber;   // same, for the last matched line number
    hum::HumNum startPosition;   // musical position (quarter notes from the start of the piece getDurationFromStart)
    hum::HumNum endPosition;     // same, for the last matched position without the duration of that slice
    // Where each position of the pattern matched, see PositionOnset.
    std::vector<PositionOnset> positionOnsets;
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

class AttributeMatcher {
public:
    AttributeMatcher(std::string drivingFeature, std::vector<AttributeMap> pattern,
                      MatcherOptions options = {});

    std::vector<AttributeMatch> findAll(const HumdrumChorale& chorale, std::size_t voice) const;

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
    std::vector<AttributeMatch> walkPattern(const HumdrumChorale& chorale, std::size_t voice, const std::vector<Onset>& onsets) const;

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
    PatternPosition withReferencesRead(const HumdrumChorale& chorale, std::size_t voice, const PatternPosition& position,
                                       const MatchedOnsets& matched) const;

    // Consumes one or more consecutive onsets starting at onsets[onsetIndex] that together
    // make up one "logical note" for this pattern position, until their summed duration hits
    // one of the position's allowed "duration" values. Returns how many onsets were consumed,
    // or nullopt if no prefix of the run lands exactly on an allowed duration before
    // overshooting all of them or leaving the logical note. Only used when
    // MatcherOptions::durationAllowSplitNotes is set.
    std::optional<std::size_t> matchSplitPosition(const HumdrumChorale& chorale, std::size_t voice,
                                                   const std::vector<Onset>& onsets, std::size_t onsetIndex,
                                                   const PatternPosition& position) const;

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
    std::vector<PatternPosition> m_pattern;
    std::vector<bool> m_hasReferences; // per position: whether it has a position reference
    MatcherOptions m_options;
    bool m_readsLyrics = false; // driven by word or syl, or with a lyrics key in the pattern
};

} // namespace choralesearch

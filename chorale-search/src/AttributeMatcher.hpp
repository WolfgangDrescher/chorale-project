#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include "HumdrumChorale.hpp"
#include "Query.hpp"

namespace choralesearch {

struct AttributeMatch {
    std::size_t voice;           // 1-indexed voice (spine) the match was found in
    std::size_t startLineNumber; // 1-indexed line in the compiled Humdrum file; 0 = not set
    std::size_t endLineNumber;   // same, for the last matched line number
    hum::HumNum startPosition;   // musical position (quarter notes from the start of the piece getDurationFromStart)
    hum::HumNum endPosition;     // same, for the last matched position without the duration of that slice
};

class AttributeMatcher {
public:
    AttributeMatcher(std::string drivingFeature, std::vector<AttributeMap> pattern,
                      MatcherOptions options = {});

    std::vector<AttributeMatch> findAll(const HumdrumChorale& chorale, std::size_t voice) const;

private:
    // One onset of the driving spine as the pattern walk sees it. Without
    // MatcherOptions::metweightSkipUnclassified that is just the spine's own token and its own
    // sounding duration. With it, ornamental onsets are gone from the walk entirely and survive
    // only in what they changed about their neighbours: the duration of the note they decorate,
    // and the interval into the note that follows them -- see buildOnsets().
    struct Onset {
        hum::HTp token;
        hum::HumNum duration;            // own sounding duration, plus that of any ornament skipped after it
        std::optional<std::string> mint; // interval measured across skipped ornaments; nullopt = ask the spine
    };

    // The driving spine's onsets in order, with ornaments already folded away when
    // MatcherOptions::metweightSkipUnclassified asks for it. Rests are never ornaments (their
    // **metweight is unclassified regardless of where they fall), and neither is a voice's very
    // first onset -- there is no note in front of it whose duration it could belong to.
    std::vector<Onset> buildOnsets(const HumdrumChorale& chorale, std::size_t voice) const;

    // Judges a single pattern key (possibly "!"-negated) against a single onset, with the
    // negation already applied to the returned value. nullopt means the key couldn't be
    // judged at all because the spine or token it needs isn't there -- that fails the
    // position outright, negation included.
    std::optional<bool> matchKey(const HumdrumChorale& chorale, std::size_t voice, const Onset& onset,
                                  const std::string& rawKey, const std::vector<std::string>& allowed) const;

    // Consumes one or more consecutive onsets starting at onsets[onsetIndex] that together
    // make up one "logical note" for this pattern position, until their summed duration hits
    // one of the position's allowed "duration" values. Returns how many onsets were consumed,
    // or nullopt if no prefix of the run lands exactly on an allowed duration before
    // overshooting all of them or leaving the logical note. Only used when
    // MatcherOptions::durationAllowSplitNotes is set.
    std::optional<std::size_t> matchSplitPosition(const HumdrumChorale& chorale, std::size_t voice,
                                                   const std::vector<Onset>& onsets, std::size_t onsetIndex,
                                                   const AttributeMap& position) const;

    // Judges a "mint" or "kern" key (possibly "!"-negated) that a merged run's later position
    // states about the re-attack the merged onset doesn't have, against what that re-attack
    // *would* have been rather than against the onset's own token -- see the two ...ReAttackInList
    // helpers. Same contract as matchKey otherwise: negation applied, nullopt when the key
    // can't be judged at all.
    std::optional<bool> matchReAttackKey(const HumdrumChorale& chorale, std::size_t voice, const Onset& onset,
                                          const std::string& rawKey, const std::vector<std::string>& allowed) const;

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
                                                     hum::HumNum remaining, bool isContinuation) const;

    std::string m_drivingFeature;
    std::vector<AttributeMap> m_pattern;
    MatcherOptions m_options;
};

} // namespace choralesearch

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
    // Judges a single pattern key (possibly "!"-negated) against a single onset, with the
    // negation already applied to the returned value. nullopt means the key couldn't be
    // judged at all because the spine or token it needs isn't there -- that fails the
    // position outright, negation included.
    std::optional<bool> matchKey(const HumdrumChorale& chorale, std::size_t voice, hum::HTp tok,
                                  const std::string& rawKey, const std::vector<std::string>& allowed) const;

    // Consumes one or more consecutive onsets starting at onsets[onsetIndex] that together
    // make up one "logical note" for this pattern position, until their summed duration hits
    // one of the position's allowed "duration" values. Returns how many onsets were consumed,
    // or nullopt if no prefix of the run lands exactly on an allowed duration before
    // overshooting all of them or leaving the logical note. Only used when
    // MatcherOptions::durationAllowSplitNotes is set.
    std::optional<std::size_t> matchSplitPosition(const HumdrumChorale& chorale, std::size_t voice,
                                                   const std::vector<hum::HTp>& onsets, std::size_t onsetIndex,
                                                   const AttributeMap& position) const;

    // Judges a "mint" or "kern" key (possibly "!"-negated) that a merged run's later position
    // states about the re-attack the merged onset doesn't have, against what that re-attack
    // *would* have been rather than against the onset's own token -- see the two ...ReAttackInList
    // helpers. Same contract as matchKey otherwise: negation applied, nullopt when the key
    // can't be judged at all.
    std::optional<bool> matchReAttackKey(const HumdrumChorale& chorale, std::size_t voice, hum::HTp tok,
                                          const std::string& rawKey, const std::vector<std::string>& allowed) const;

    // The converse of matchSplitPosition: consumes one or more consecutive pattern positions
    // starting at m_pattern[patternIndex] that together describe the single onset `tok`,
    // until their durations have used up exactly `remaining` (the onset's sounding duration,
    // minus whatever earlier positions of the run already claimed). Returns how many positions
    // were consumed, or nullopt if no prefix of them divides the onset up exactly before
    // overshooting or running into a position without a concrete duration. isContinuation says
    // whether m_pattern[patternIndex] is itself already part of a started run. Only used when
    // MatcherOptions::durationAllowMergedNotes is set.
    std::optional<std::size_t> matchMergedPositions(const HumdrumChorale& chorale, std::size_t voice, hum::HTp tok,
                                                     std::size_t patternIndex, hum::HumNum remaining,
                                                     bool isContinuation) const;

    std::string m_drivingFeature;
    std::vector<AttributeMap> m_pattern;
    MatcherOptions m_options;
};

} // namespace choralesearch

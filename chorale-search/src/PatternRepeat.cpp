#include "PatternRepeat.hpp"

#include <algorithm>
#include <stdexcept>
#include <string>

namespace choralesearch {

namespace {

// An exact length a split or a merged note could add up to.
bool hasExactDuration(const PatternPosition& position) {
    auto duration = position.keys.find("duration");
    return duration != position.keys.end() && std::none_of(duration->second.begin(), duration->second.end(), [](const PatternValue& v) {
        return v.text == "*" || v.hasComparisonOperator() || v.isVoiceReference() || v.isPositionReference();
    });
}

void validateNestedRepeats(const std::vector<PatternPosition>& positions, const std::string& path, bool insideRepeat, bool notesMaySplitOrMerge) {
    for (std::size_t index = 0; index < positions.size(); ++index) {
        const PatternPosition& position = positions[index];
        const std::string here = "'" + path + "[" + std::to_string(index) + "]'";
        if (!position.subPattern.empty() && !position.repeat) throw std::invalid_argument(here + ": a subPattern needs a repeat");
        if (position.repeat) {
            const PatternRepeat& repeat = *position.repeat;
            if (repeat.max && *repeat.max == 0) throw std::invalid_argument(here + ": a repeat needs a max of at least 1");
            if (repeat.max && repeat.min > *repeat.max) throw std::invalid_argument(here + ": a repeat needs a min of at most its max");
            if (!position.subPattern.empty() && minimumOnsets(position.subPattern) == 0) {
                throw std::invalid_argument(here + ": each repeat of a subPattern has to match an onset, but each of its positions "
                                            "may repeat 0 times");
            }
        }
        // Split and merged notes add up the durations of neighbouring positions, a repeat matches one onset each time.
        const bool repeats = insideRepeat || position.repeat;
        if (repeats && notesMaySplitOrMerge && hasExactDuration(position)) {
            throw std::invalid_argument(here + ": a position that repeats cannot have a duration with durationAllowSplitNotes or "
                                        "durationAllowMergedNotes");
        }
        validateNestedRepeats(position.subPattern, path + "[" + std::to_string(index) + "][subPattern]", repeats, notesMaySplitOrMerge);
    }
}

} // namespace

bool hasRepeatWithoutFixedCount(const std::vector<PatternPosition>& pattern) {
    return std::any_of(pattern.begin(), pattern.end(), [](const PatternPosition& position) {
        return position.repeat && (position.repeat->max != position.repeat->min || hasRepeatWithoutFixedCount(position.subPattern));
    });
}

std::size_t minimumOnsets(const std::vector<PatternPosition>& pattern) {
    std::size_t onsets = 0;
    for (const PatternPosition& position : pattern) {
        const std::size_t once = position.subPattern.empty() ? 1 : minimumOnsets(position.subPattern);
        onsets += position.repeat ? position.repeat->min * once : once;
    }
    return onsets;
}

void validateRepeats(const std::vector<PatternPosition>& pattern, bool notesMaySplitOrMerge) {
    validateNestedRepeats(pattern, "pattern", false, notesMaySplitOrMerge);
    if (!pattern.empty() && minimumOnsets(pattern) == 0) {
        throw std::invalid_argument("'pattern' has to match an onset, but each of its positions may repeat 0 times");
    }
}

void validateLastRepeatGoesOn(const std::vector<PatternPosition>& pattern) {
    for (std::size_t index = pattern.size(); index-- > 0;) {
        const PatternPosition& position = pattern[index];
        const bool mayTakeMore = position.repeat && (!position.repeat->max || *position.repeat->max > position.repeat->min);
        if (mayTakeMore && !position.repeat->greedy) {
            throw std::invalid_argument("'pattern[" + std::to_string(index) + "]': a repeat that nothing has to follow takes no more "
                                        "than its min: give it \"greedy\": true, or a number of times");
        }
        if (minimumOnsets({position}) > 0) break;
    }
}

} // namespace choralesearch

#include "PatternRepeat.hpp"

#include <algorithm>
#include <limits>
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
            if (repeat.maxQuarterNotes && *repeat.maxQuarterNotes == 0) {
                throw std::invalid_argument(here + ": a repeat needs a maxQuarterNotes of at least 1");
            }
            const auto shortest = shortestDuration(repeat);
            const auto longest = longestDuration(repeat);
            if (shortest && longest && *shortest > *longest) {
                throw std::invalid_argument(here + ": a repeat cannot last at least its minDuration or minQuarterNotes and at most its "
                                            "maxDuration or maxQuarterNotes");
            }
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

// A number of quarter notes as a HumNum, which holds an int: no chorale lasts as long as the largest.
hum::HumNum quarterNotesToHumNum(std::size_t count) {
    return hum::HumNum(static_cast<int>(std::min<std::size_t>(count, std::numeric_limits<int>::max())));
}

} // namespace

std::optional<hum::HumNum> shortestDuration(const PatternRepeat& repeat) {
    std::optional<hum::HumNum> shortest;
    if (repeat.minDuration) shortest = hum::Convert::recipToDuration(*repeat.minDuration);
    if (repeat.minQuarterNotes) {
        const hum::HumNum quarterNotes = quarterNotesToHumNum(*repeat.minQuarterNotes);
        if (!shortest || *shortest < quarterNotes) shortest = quarterNotes;
    }
    return shortest;
}

std::optional<hum::HumNum> longestDuration(const PatternRepeat& repeat) {
    std::optional<hum::HumNum> longest;
    if (repeat.maxDuration) longest = hum::Convert::recipToDuration(*repeat.maxDuration);
    if (repeat.maxQuarterNotes) {
        const hum::HumNum quarterNotes = quarterNotesToHumNum(*repeat.maxQuarterNotes);
        if (!longest || *longest > quarterNotes) longest = quarterNotes;
    }
    return longest;
}

bool hasRepeatWithoutFixedCount(const std::vector<PatternPosition>& pattern) {
    return std::any_of(pattern.begin(), pattern.end(), [](const PatternPosition& position) {
        return position.repeat && (position.repeat->max != position.repeat->min || hasRepeatWithoutFixedCount(position.subPattern));
    });
}

std::size_t minimumOnsets(const std::vector<PatternPosition>& pattern) {
    std::size_t onsets = 0;
    for (const PatternPosition& position : pattern) {
        const std::size_t once = position.subPattern.empty() ? 1 : minimumOnsets(position.subPattern);
        if (!position.repeat) {
            onsets += once;
            continue;
        }
        // A run that has to last takes an onset.
        const auto shortest = shortestDuration(*position.repeat);
        const std::size_t times = position.repeat->min == 0 && shortest && *shortest > 0 ? 1 : position.repeat->min;
        onsets += times * once;
    }
    return onsets;
}

void validateRepeats(const std::vector<PatternPosition>& pattern, bool notesMaySplitOrMerge) {
    validateNestedRepeats(pattern, "pattern", false, notesMaySplitOrMerge);
    if (!pattern.empty() && minimumOnsets(pattern) == 0) {
        throw std::invalid_argument("'pattern' has to match an onset, but each of its positions may repeat 0 times");
    }
}

std::optional<std::string> lastRepeatThatStops(const std::vector<PatternPosition>& pattern, const std::string& path) {
    for (std::size_t index = pattern.size(); index-- > 0;) {
        const PatternPosition& position = pattern[index];
        const std::string here = path + "[" + std::to_string(index) + "]";
        // A run that has to last goes on until it does.
        if (position.repeat && !shortestDuration(*position.repeat)) {
            const bool mayTakeMore = !position.repeat->max || *position.repeat->max > position.repeat->min;
            if (mayTakeMore && !position.repeat->greedy) return here;
            // Nothing follows the last pass of a subPattern either.
            if (const auto inside = lastRepeatThatStops(position.subPattern, here + "[subPattern]")) return inside;
        }
        if (minimumOnsets({position}) > 0) break;
    }
    return std::nullopt;
}

void validateLastRepeatGoesOn(const std::vector<PatternPosition>& pattern) {
    if (const auto path = lastRepeatThatStops(pattern)) {
        throw std::invalid_argument("'" + *path + "': a repeat that nothing has to follow takes no more than its min: give it "
                                    "\"greedy\": true, or a number of times");
    }
}

} // namespace choralesearch

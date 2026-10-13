#include "ChoraleFilter.hpp"

#include <algorithm>

namespace choralesearch {

namespace {

bool anyIdRangeContains(const std::vector<ChoraleIdRange>& ranges, std::size_t number) {
    return std::any_of(ranges.begin(), ranges.end(), [&](const ChoraleIdRange& r) { return r.contains(number); });
}

bool contains(const std::vector<std::string>& values, const std::string& value) {
    return std::find(values.begin(), values.end(), value) != values.end();
}

} // namespace

bool ChoraleFilter::matchesId(const std::string& id) const {
    if (ids.empty() && excludeIds.empty()) return true;
    const std::optional<std::size_t> number = integerFromChoraleId(id);
    // A score with no number in its id cannot be named by one: it is never among the chorales asked
    // for, and never among the ones left out.
    if (!number) return ids.empty();
    if (!ids.empty() && !anyIdRangeContains(ids, *number)) return false;
    return !anyIdRangeContains(excludeIds, *number);
}

bool ChoraleFilter::matches(const ChoraleInfo& info) const {
    if (!matchesId(info.id)) return false;

    if (!title.empty()) {
        const bool found = std::any_of(title.begin(), title.end(), [&](const std::string& wanted) {
            const std::string needle = lowercaseLatin(wanted);
            return std::any_of(info.titles.begin(), info.titles.end(), [&](const std::string& t) {
                return lowercaseLatin(t).find(needle) != std::string::npos;
            });
        });
        if (!found) return false;
    }

    if (!meter.empty() &&
        !std::any_of(meter.begin(), meter.end(), [&](const std::string& m) { return contains(info.meters, m); })) {
        return false;
    }

    if (measureCount && !measureCount->contains(info.measureCount)) return false;
    if (phraseCount && !phraseCount->contains(info.phraseCount)) return false;
    if (!startKey.empty() && (!info.key || !contains(startKey, *info.key))) return false;
    if (!startMode.empty() && (!info.mode || !contains(startMode, *info.mode))) return false;
    return true;
}

} // namespace choralesearch

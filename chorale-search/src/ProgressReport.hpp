#pragma once

#include <iostream>
#include <string>

#include <nlohmann/json.hpp>

#include "CorpusSearch.hpp"

namespace choralesearch {

// What --progress writes: one JSON object per line on stderr, each with an "event" key, for a
// caller that reads along while the tool runs. stdout stays the result alone, and the tools'
// own "Error: ..." lines stay what they were: they are not JSON. The keys keep the order they
// are written in (ordered_json), so "event" always comes first.
inline void reportEvent(const nlohmann::ordered_json& event) {
    std::cerr << event.dump() << '\n';
}

// A stage of the work that has no count of its own, named by a short code the caller words:
// "convert-musicxml", "split-score-into-voices", "analyze-score", "segment-score", "search-corpus".
inline void reportPhase(const std::string& phase) {
    reportEvent({{"event", "phase"}, {"phase", phase}});
}

// The corpus search's progress as events: one per chorale file gone through, each carrying
// the total so that a single line says everything.
inline ProgressCallback progressToStderr() {
    return [](const SearchProgress& progress) {
        reportEvent({{"event", "search-progress"},
                     {"chorale", progress.choraleId},
                     {"choralesSearched", progress.choralesSearched},
                     {"choralesTotal", progress.choralesTotal},
                     {"matchesSoFar", progress.matchesSoFar}});
    };
}

} // namespace choralesearch

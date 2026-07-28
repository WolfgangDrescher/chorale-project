#pragma once

#include <filesystem>
#include <map>
#include <string>
#include <vector>

#include "humlib.h"

namespace choralesearch {

// One annotated key change: from `pos` onward the piece is in `key`.
struct Modulation {
    std::string pos; // "measure/beat", e.g. "10/3.5"
    int measure = 0;
    hum::HumNum beat;
    std::string key;
};

// Reads an annotation file -- a JSON object keyed by chorale id, each holding [position, key]
// pairs, e.g. {"chor001": [["0/3", "G"], ["10/3", "e"]]}. Comments in the JSON are tolerated,
// since the annotations are hand-maintained. Throws if the file can't be read or parsed, or if
// a position isn't in measure/beat form.
std::map<std::string, std::vector<Modulation>> readModulations(const std::filesystem::path& path);

// Writes `mods` into `infile` as key designations (*G:, *e: ...) on every kern spine, in place.
// A modulation the score has no matching position for is reported on stderr, prefixed with
// `choraleId`, and skipped -- one bad annotation shouldn't cost the whole score.
//
// The file is left re-parsed, so callers can hand it straight to further analysis: an inserted
// line only joins the file's spine structure once its text has been read back in.
void applyModulations(hum::HumdrumFile& infile, const std::string& choraleId, const std::vector<Modulation>& mods);

} // namespace choralesearch

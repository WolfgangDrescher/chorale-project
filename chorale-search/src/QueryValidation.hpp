#pragma once

#include <string>
#include <vector>

namespace choralesearch {

// True if `key` is one of the fields shared by both Query and SimultaneousGroup ("feature",
// "voices", "pattern", the matcher-option booleans, "simultaneousAlignment").
bool isKnownSimultaneousGroupKey(const std::string& key);

// True if `key` is any field a top-level Query can have -- everything
// isKnownSimultaneousGroupKey accepts, plus "id", "limit", "simultaneousWith" (only meaningful
// on the top-level query, not inside a nested simultaneousWith group).
bool isKnownQueryKey(const std::string& key);

// The only spines the analysis ever generates (see SpineAnalysis.cpp's
// applySpineAnalysisTools) -- the only features that can drive a search. Always the same set,
// never discovered per file, so this can be checked up front against untrusted query JSON.
const std::vector<std::string>& drivingFeatureNames();

// True if `feature` is one of drivingFeatureNames().
bool isKnownDrivingFeature(const std::string& feature);

// True if `key` is something a pattern position can check: any driving feature,
// "duration"/"fermata" (read off the underlying kern token rather than being spines of their
// own), or a hint-<voice>/hint-<pair> relative or wildcard form (e.g. "hint-2", "hint-*4",
// "hint-1*", "hint-**"). `key` may carry a leading '!' negation (see AttributeMatcher.cpp /
// docs/patterns#negating-a-feature, e.g. "!deg") -- negation doesn't change which keys are
// known, only how a match is interpreted, so it's stripped internally before checking.
bool isKnownPatternKey(const std::string& key);

// True if `value` is syntactically valid for `key` (already known-good per isKnownPatternKey),
// per that feature's documented token format (see chorale-webapp's docs/features pages). `key`
// may carry a leading '!' negation, same as isKnownPatternKey. The universal wildcard "*" and
// the empty string are the caller's responsibility to handle first -- this only judges the
// feature-specific grammar.
bool isValidPatternValue(const std::string& key, const std::string& value);

// True if `value` is a legal entry of the "mintAllowIntervalComplementation" option: a single
// diatonic number "1"-"8" (the simple intervals, the only ones with a complement inside the
// octave), or "*" for every number at once.
bool isValidMintComplementationValue(const std::string& value);

} // namespace choralesearch

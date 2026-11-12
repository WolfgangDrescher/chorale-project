#pragma once

#include <optional>
#include <string>
#include <vector>

namespace choralesearch {

// The keys a position of a pattern can check, and what their values may be.

inline const std::string kKernKey = "kern";
inline const std::string kDegKey = "deg";
inline const std::string kMintKey = "mint";
inline const std::string kFbKey = "fb";
inline const std::string kMetweightKey = "metweight";
inline const std::string kWordKey = "word";
inline const std::string kSylKey = "syl";
inline const std::string kDurationKey = "duration";
inline const std::string kFermataKey = "fermata";
inline const std::string kPhraseKey = "phrase";
inline const std::string kModeKey = "mode";
inline const std::string kKeyKey = "key";
inline const std::string kKeyDegKey = "keyDeg";
inline const std::string kSylposKey = "sylpos";
inline const std::string kMelismaKey = "melisma";

// HintPair is "hint-14", HintVoice "hint-2", HintPairWildcard "hint-*4", HintPairCurrent "hint-@pair".
enum class PatternKeyId {
    Kern, Deg, Mint, Fb, Metweight, Word, Syl, HintPair,
    Duration, Fermata, Phrase, Mode, Key, KeyDeg, Sylpos, Melisma,
    HintVoice, HintPairWildcard, HintPairCurrent,
};

// Whether a value of the query is valid: "+M2" for mint, "4." for duration.
using ValueCheck = bool (*)(const std::string& value);

// What the values of a key are like.
struct ValueSpec {
    bool comparable = false;     // { "operator": "gt", "value": "4" }
    bool voiceReference = false; // "@2", "@below"
    ValueCheck isValidValue = nullptr;           // "+M2", "+", "3"
    ValueCheck isValidComparisonValue = nullptr; // the value of a comparison: "+3", "m6"
};

// A part of a value, checked under a key of its own: "mint.direction". Its values are values of the whole
// key too, with the same meaning: "mint.number": "2" is "mint": "2".
struct PatternKeyPart {
    std::string name; // "pitch", "direction", "number", "interval", "numbers"
    ValueSpec valueSpec;
};

// A key a position can check.
struct PatternKey {
    PatternKeyId id = PatternKeyId::Kern;
    // The key, or a placeholder for many keys: "kern", "mint", "hint-@pair"; "hint-<pair>", "hint-<voice>".
    std::string name;
    // Only for a placeholder name: whether a key of the query is one of it, "hint-14" of "hint-<pair>".
    bool (*coversQueryKey)(const std::string& queryKey) = nullptr;
    bool driving = false;  // can be the feature of a query
    bool perVerse = false; // the lyrics, read verse by verse
    ValueSpec valueSpec;
    std::vector<PatternKeyPart> parts = {};
};

// What a key of the query names: "!mint.direction" is the part direction of the key mint, negated.
struct PatternKeyLookup {
    std::string rawKey;                             // as the query writes it: "!hint-14.number"
    std::string baseKey;                            // without negation and part: "hint-14"
    const PatternKey* patternKey = nullptr;         // nullptr for an unknown key, or an unknown part of one
    const PatternKeyPart* patternKeyPart = nullptr; // nullptr for the whole key
    bool negated = false;

    bool is(PatternKeyId id) const { return patternKey && patternKey->id == id; }
    bool readsLyrics() const { return patternKey && patternKey->perVerse; }
    // The part's values, or the key's own.
    const ValueSpec& valueSpec() const { return patternKeyPart ? patternKeyPart->valueSpec : patternKey->valueSpec; }
};

// Every key: the driving features first.
const std::vector<PatternKey>& patternKeys();

PatternKeyLookup findPatternKey(const std::string& rawKey);

// A value as the score spells it, or nullopt for no such value.
std::optional<std::string> normalizeMetweightValue(const std::string& value); // "strong", "1": "s"
std::optional<std::string> normalizeSylposValue(const std::string& value);    // "i": "initial"
std::optional<std::string> normalizeMelismaValue(const std::string& value);   // "n": "none", true

// The direction of a unison, which goes nowhere: "none".
inline const std::string kNoDirection = "none";

} // namespace choralesearch

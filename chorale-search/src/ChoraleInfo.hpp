#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace choralesearch {

// What a ChoraleFilter asks about a chorale: the facts a score states about
// itself as a whole, as opposed to anything a pattern finds in it.
struct ChoraleInfo {
    std::string id;

    // Every title the reference records give (!!!OTL, !!!OTL@@DE, !!!OTL@EN, ...), HTML entities decoded.
    std::vector<std::string> titles;

    // The distinct meters the score states, in the order they appear: "3/4", "4/4".
    std::vector<std::string> meters;

    // The number of measures: the highest number a barline carries. A pickup measure has no
    // number of its own and is not counted.
    std::size_t measureCount = 0;

    // The number of phrases, which the fermatas delimit (see HumdrumChorale::phraseEndings).
    std::size_t phraseCount = 0;

    // The key the score starts in, as the score writes it ("G", "e", "B-", "f#"), and its mode in the
    // canonical spelling of normalizeModeName().
    std::optional<std::string> key;
    std::optional<std::string> mode;
};

// The number in a chorale's id: 29 for "chor029". The digits at the end count; nullopt where the id has none.
std::optional<std::size_t> integerFromChoraleId(const std::string& id);

// The modes a chorale filter can ask for, by their canonical name: "major", "minor", "dorian", "phrygian",
// "lydian", "mixolydian", "locrian". "ionian" is interpreted as major and "aeolian" as minor, and a name in capitals
// is the same name. nullopt for anything else.
std::optional<std::string> normalizeModeName(const std::string& name);

// The names normalizeModeName accepts, for an error message.
const std::vector<std::string>& modeNames();

// `text` in lower case, as far as titles need it: ASCII and the Latin-1 letters (Ä Ö Ü ...) in UTF-8.
std::string lowercaseLatin(const std::string& text);

// `text` with the HTML entities of the reference records decoded ("Wasserfl&uuml;ssen" is "Wasserflüssen"): the
// named ones a German title has and any numeric one. Anything else is left as it is.
std::string decodeHtmlEntities(const std::string& text);

// The mode a key designation (*G:, *e:, *f:dor) states, in the canonical spelling of normalizeModeName(). `key`
// is what stands before the colon and `label` what stands after it. A label is the three letters Humdrum abbreviates
// a mode with ("dor", "phr", ...); without one, or with one that is not known, the case of the key says major or minor.
std::string modeFromKeyDesignation(const std::string& key, const std::string& label);

} // namespace choralesearch

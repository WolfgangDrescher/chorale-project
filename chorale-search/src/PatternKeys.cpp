#include "PatternKeys.hpp"

#include "ChoraleInfo.hpp"
#include "HintPairs.hpp"
#include "Intervals.hpp"
#include "QueryValidation.hpp"

#include <algorithm>
#include <cctype>
#include <iterator>
#include <map>
#include <regex>
#include <sstream>

namespace choralesearch {

namespace {

// kern compares a value without rhythm, pitch or fermata literally (see kern.md).
bool isAnyValue(const std::string&) {
    return true;
}

bool isValidDegValue(const std::string& v) {
    static const std::regex re(R"(^([1-7][+-]*|\++|-+|r)$)");
    return std::regex_match(v, re);
}

// A key degree is never a rest.
bool isValidKeyDegValue(const std::string& v) {
    return v != "r" && isValidDegValue(v);
}

bool isValidKeyValue(const std::string& v) {
    static const std::regex re(R"(^[a-gA-G][#-]*$)");
    return std::regex_match(v, re);
}

bool isValidModeValue(const std::string& v) {
    return normalizeModeName(v).has_value();
}

bool isValidPhraseValue(const std::string& v) {
    return v == "start" || v == "end" || v == "choraleStart" || v == "choraleEnd";
}

bool isValidFermataValue(const std::string& v) {
    return v == "true" || v == "false";
}

bool isValidMetweightValue(const std::string& v) {
    return normalizeMetweightValue(v).has_value();
}

// Letters (any byte beyond ASCII, for ä, ö, ü), digits, apostrophes and "*" for any characters.
bool isValidLyricsText(const std::string& v) {
    return !v.empty() && std::all_of(v.begin(), v.end(), [](unsigned char c) {
        return std::isalnum(c) || c >= 0x80 || c == '\'' || c == '*';
    });
}

bool isValidSylposValue(const std::string& v) {
    return normalizeSylposValue(v).has_value();
}

bool isValidMelismaValue(const std::string& v) {
    return normalizeMelismaValue(v).has_value();
}

// "P", "M", "m", or up to three "A" or "d".
bool isIntervalQuality(const std::string& quality) {
    if (quality == "P" || quality == "M" || quality == "m") return true;
    if (quality.empty() || quality.size() > 3) return false;
    const char letter = quality[0];
    return (letter == 'A' || letter == 'd') && quality.find_first_not_of(letter) == std::string::npos;
}

// A number from 1 to 999, without a leading zero.
bool isIntervalNumber(const std::string& number) {
    return !number.empty() && number.size() <= 3 && number[0] != '0' &&
           number.find_first_not_of("0123456789") == std::string::npos;
}

// A quality, a number, or both where the quality fits the number ("P5", not "P3").
bool isValidQualityAndNumber(const std::string& quality, const std::string& number) {
    if (!quality.empty() && !isIntervalQuality(quality)) return false;
    if (!number.empty() && !isIntervalNumber(number)) return false;
    return quality.empty() || number.empty() || isValidIntervalQuality(quality, std::stoi(number));
}

// [direction][quality][number], not all empty, or the first note of a voice, "[gg]" (see mint.md).
bool isValidMintValue(const std::string& v) {
    static const std::regex firstNote(R"(^\[[A-Ga-g]+[#n-]*\]$)");
    if (std::regex_match(v, firstNote)) return true;
    const auto parsed = parseMintValue(v);
    return parsed && !v.empty() && isValidQualityAndNumber(parsed->quality, parsed->number);
}

// [quality]number: "m6", "6".
bool isValidIntervalFigure(const std::string& v) {
    const auto parsed = parseFbValue(v);
    return parsed && isValidQualityAndNumber(parsed->quality, parsed->figure);
}

bool isValidFbValue(const std::string& v) {
    std::istringstream iss(v);
    std::vector<std::string> figures{std::istream_iterator<std::string>{iss}, std::istream_iterator<std::string>{}};
    return !figures.empty() && std::all_of(figures.begin(), figures.end(), isValidIntervalFigure);
}

bool isValidDurationValue(const std::string& v) {
    static const std::regex re(R"(^\d+(%\d+)?\.*$)");
    return std::regex_match(v, re);
}

// One letter repeated for the register ("gg", "GG"), then sharps, flats or a natural.
bool isValidKernPitchValue(const std::string& v) {
    static const std::regex re(R"(^([A-Ga-g])\1*(?:#+|-+|n)?$)");
    return std::regex_match(v, re);
}

// [direction][quality]number: with a direction mint compares in it only.
bool isValidMintComparisonValue(const std::string& v) {
    const auto parsed = parseMintValue(v);
    return parsed && !parsed->number.empty() && isValidQualityAndNumber(parsed->quality, parsed->number);
}

// [quality]number: an interval between two voices has no direction.
bool isValidHintComparisonValue(const std::string& v) {
    return isValidIntervalFigure(v);
}

// A lone "!" is no negation.
bool isNegatedKey(const std::string& rawKey) {
    return rawKey.size() > 1 && rawKey[0] == '!';
}

constexpr ValueSpec kHintValues{
    .comparable = true,
    .isValidValue = isValidIntervalFigure, .isValidComparisonValue = isValidHintComparisonValue,
};

std::vector<PatternKey> buildPatternKeys() {
    using Id = PatternKeyId;
    return {
        {
            .id = Id::Kern,
            .name = kKernKey,
            .driving = true,
            .valueSpec = {
                .comparable = true, .voiceReference = true,
                .isValidValue = isAnyValue, .isValidComparisonValue = isValidKernPitchValue,
            },
        },
        {
            .id = Id::Deg,
            .name = kDegKey,
            .driving = true,
            .valueSpec = {.isValidValue = isValidDegValue},
        },
        {
            .id = Id::Mint,
            .name = kMintKey,
            .driving = true,
            .valueSpec = {
                .comparable = true,
                .isValidValue = isValidMintValue, .isValidComparisonValue = isValidMintComparisonValue,
            },
        },
        {
            .id = Id::Fb,
            .name = kFbKey,
            .driving = true,
            .valueSpec = {.isValidValue = isValidFbValue},
        },
        {
            .id = Id::Metweight,
            .name = kMetweightKey,
            .driving = true,
            .valueSpec = {.isValidValue = isValidMetweightValue},
        },
        {
            .id = Id::Word,
            .name = kWordKey,
            .driving = true,
            .perVerse = true,
            .valueSpec = {.isValidValue = isValidLyricsText},
        },
        {
            .id = Id::Syl,
            .name = kSylKey,
            .driving = true,
            .perVerse = true,
            .valueSpec = {.isValidValue = isValidLyricsText},
        },
        {
            .id = Id::HintPair,
            .name = "hint-<pair>",
            .coversQueryKey = isHintPairFeature,
            .driving = true,
            .valueSpec = kHintValues,
        },
        {
            .id = Id::Duration,
            .name = kDurationKey,
            .valueSpec = {
                .comparable = true, .voiceReference = true,
                .isValidValue = isValidDurationValue, .isValidComparisonValue = isValidDurationValue,
            },
        },
        {
            .id = Id::Fermata,
            .name = kFermataKey,
            .valueSpec = {.isValidValue = isValidFermataValue},
        },
        {
            .id = Id::Phrase,
            .name = kPhraseKey,
            .valueSpec = {.isValidValue = isValidPhraseValue},
        },
        {
            .id = Id::Mode,
            .name = kModeKey,
            .valueSpec = {.isValidValue = isValidModeValue},
        },
        {
            .id = Id::Key,
            .name = kKeyKey,
            .valueSpec = {.isValidValue = isValidKeyValue},
        },
        {
            .id = Id::KeyDeg,
            .name = kKeyDegKey,
            .valueSpec = {.isValidValue = isValidKeyDegValue},
        },
        {
            .id = Id::Sylpos,
            .name = kSylposKey,
            .perVerse = true,
            .valueSpec = {.isValidValue = isValidSylposValue},
        },
        {
            .id = Id::Melisma,
            .name = kMelismaKey,
            .perVerse = true,
            .valueSpec = {.isValidValue = isValidMelismaValue},
        },
        {
            .id = Id::HintVoice,
            .name = "hint-<voice>",
            .coversQueryKey = isHintRelativeKey,
            .valueSpec = kHintValues,
        },
        {
            .id = Id::HintPairWildcard,
            .name = "hint-<pairWildcard>",
            .coversQueryKey = isHintPairWildcardKey,
            .valueSpec = kHintValues,
        },
        {
            .id = Id::HintPairCurrent,
            .name = kPairSearchedKey,
            .valueSpec = kHintValues,
        },
    };
}

} // namespace

const std::vector<PatternKey>& patternKeys() {
    static const std::vector<PatternKey> keys = buildPatternKeys();
    return keys;
}

PatternKeyLookup findPatternKey(const std::string& rawKey) {
    PatternKeyLookup lookup;
    lookup.rawKey = rawKey;
    lookup.negated = isNegatedKey(rawKey);
    lookup.baseKey = lookup.negated ? rawKey.substr(1) : rawKey;
    for (const PatternKey& patternKey : patternKeys()) {
        const bool found = patternKey.coversQueryKey ? patternKey.coversQueryKey(lookup.baseKey) : patternKey.name == lookup.baseKey;
        if (found) {
            lookup.patternKey = &patternKey;
            break;
        }
    }
    return lookup;
}

std::optional<std::string> normalizeMetweightValue(const std::string& value) {
    static const std::map<std::string, std::string> tokens = {
        {"s", "s"}, {"strong", "s"}, {"1", "s"},       {"hs", "hs"}, {"half-strong", "hs"}, {"2", "hs"},
        {"w", "w"}, {"weak", "w"},   {"3", "w"},       {"u", "u"},   {"unclassified", "u"}, {"4", "u"},
    };
    const auto token = tokens.find(value);
    if (token == tokens.end()) return std::nullopt;
    return token->second;
}

std::optional<std::string> normalizeSylposValue(const std::string& value) {
    static const std::map<std::string, std::string> names = {
        {"single", "single"}, {"initial", "initial"}, {"medial", "medial"}, {"terminal", "terminal"},
        {"s", "single"},      {"i", "initial"},       {"m", "medial"},      {"t", "terminal"},
    };
    const auto name = names.find(value);
    if (name == names.end()) return std::nullopt;
    return name->second;
}

std::optional<std::string> normalizeMelismaValue(const std::string& value) {
    static const std::map<std::string, std::string> names = {
        {"none", "none"}, {"initial", "initial"}, {"medial", "medial"}, {"terminal", "terminal"},
        {"n", "none"},    {"i", "initial"},       {"m", "medial"},      {"t", "terminal"},
        {"true", "true"}, {"false", "false"},
    };
    const auto name = names.find(value);
    if (name == names.end()) return std::nullopt;
    return name->second;
}

} // namespace choralesearch

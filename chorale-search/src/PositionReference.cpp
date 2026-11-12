#include "PositionReference.hpp"

#include "PatternKeys.hpp"
#include "QueryValidation.hpp"

#include <algorithm>
#include <stdexcept>

namespace choralesearch {

namespace {

// Where an error is: "'pattern[1][kern]' (position 2)", the array counted from 0, the positions from 1.
std::string patternKeyLocation(std::size_t position, const std::string& rawKey) {
    return "'pattern[" + std::to_string(position) + "][" + rawKey + "]' (position " + std::to_string(position + 1) + ")";
}

bool isHintKey(const PatternKey& patternKey) {
    return patternKey.id == PatternKeyId::HintPair || patternKey.id == PatternKeyId::HintVoice ||
           patternKey.id == PatternKeyId::HintPairWildcard || patternKey.id == PatternKeyId::HintPairCurrent;
}

// Whether the key `own` accepts a value read under `read`: the same key and part, a hint key under a hint key,
// an interval under a hint key or fb, its number under a hint number or fb.numbers.
bool acceptsValueOf(const PatternKeyLookup& own, const PatternKeyLookup& read) {
    const std::string ownPart = own.partName();
    const std::string readPart = read.partName();
    const bool ownHint = isHintKey(*own.patternKey);
    const bool readHint = isHintKey(*read.patternKey);
    if ((own.patternKey == read.patternKey || (ownHint && readHint)) && ownPart == readPart) return true;

    const bool readMint = read.patternKey->id == PatternKeyId::Mint;
    const bool readInterval = (readHint && readPart.empty()) || (readMint && readPart == "interval");
    const bool readNumber = (readHint && readPart == "number") || (readMint && readPart == "number");
    const bool ownFb = own.patternKey->id == PatternKeyId::Fb;
    if ((ownHint || ownFb) && ownPart.empty()) return readInterval;
    if ((ownHint && ownPart == "number") || (ownFb && ownPart == "numbers")) return readNumber;
    return false;
}

PositionReference resolve(const PatternValue& value, std::size_t ownPosition, const std::string& rawKey,
                          const std::vector<std::optional<std::string>>& positionNames) {
    const std::string here = patternKeyLocation(ownPosition, rawKey) + ": '" + value.text + "'";
    const PatternKeyLookup own = findPatternKey(rawKey);
    const std::string ownKey = own.negated ? rawKey.substr(1) : rawKey; // "hint-14.number"
    // The position, then the key it reads, if another than the own: "#1", "$start.hint-14.number".
    const std::size_t dot = value.text.find('.');
    const std::string target = value.text.substr(0, dot);
    const std::string readKey = dot == std::string::npos ? ownKey : value.text.substr(dot + 1);

    PositionReference reference;
    reference.key = readKey;
    if (target[0] == '#') {
        const std::string digits = target.substr(1);
        if (digits.size() > 6 || digits.find_first_not_of("0123456789") != std::string::npos) {
            throw std::invalid_argument(here + " does not name a position: a position is '#' and its number, counted from 1");
        }
        const std::size_t number = std::stoul(digits);
        if (number == 0) throw std::invalid_argument(here + " does not name a position: positions are counted from 1");
        if (number > positionNames.size()) {
            throw std::invalid_argument(here + " names position " + digits + ", but the pattern has only " +
                                        std::to_string(positionNames.size()) + (positionNames.size() == 1 ? " position" : " positions"));
        }
        reference.position = number - 1;
    } else {
        auto named = std::find(positionNames.begin(), positionNames.end(), std::optional<std::string>(target));
        if (named == positionNames.end()) {
            throw std::invalid_argument(here + " names no position: no position has \"as\": \"" + target + "\"");
        }
        reference.position = static_cast<std::size_t>(named - positionNames.begin());
    }
    if (reference.position > ownPosition) {
        throw std::invalid_argument(here + " names a later position, a position can only read the positions before it and itself");
    }

    if (!own.patternKey || !own.valueSpec().positionReference) {
        throw std::invalid_argument(here + ": '" + rawKey + "' cannot be held against a position of the pattern");
    }
    const PatternKeyLookup read = findPatternKey(readKey);
    if (!read.patternKey || read.negated || !read.valueSpec().positionReference) {
        throw std::invalid_argument(here + ": '" + readKey + "' is no key a reference can read");
    }
    if (!acceptsValueOf(own, read)) {
        throw std::invalid_argument(here + ": a '" + readKey + "' value is no '" + ownKey + "' value");
    }
    return reference;
}

} // namespace

std::vector<std::optional<std::string>> positionNames(const std::vector<AttributeMap>& pattern) {
    std::vector<std::optional<std::string>> names(pattern.size());
    for (std::size_t position = 0; position < pattern.size(); ++position) {
        auto it = pattern[position].find(kPositionNameKey);
        if (it == pattern[position].end()) continue;
        if (it->second.size() != 1 || it->second.front().hasComparisonOperator() || !isValidPositionName(it->second.front().text)) {
            throw std::invalid_argument(patternKeyLocation(position, kPositionNameKey) +
                                        " must be one name: '$', a letter, then letters, digits or underscores");
        }
        const std::string& name = it->second.front().text;
        if (std::find(names.begin(), names.end(), std::optional<std::string>(name)) != names.end()) {
            throw std::invalid_argument(patternKeyLocation(position, kPositionNameKey) + ": the name '" + name + "' is given to two positions");
        }
        names[position] = name;
    }
    return names;
}

std::vector<AttributeMap> resolvePositionReferences(const std::vector<AttributeMap>& pattern) {
    const std::vector<std::optional<std::string>> names = positionNames(pattern);
    std::vector<AttributeMap> resolved = pattern;
    for (std::size_t position = 0; position < resolved.size(); ++position) {
        for (auto& [rawKey, values] : resolved[position]) {
            if (rawKey == kPositionNameKey) continue;
            for (PatternValue& value : values) {
                if (value.isPositionReference()) value.positionReference = resolve(value, position, rawKey, names);
            }
        }
    }
    return resolved;
}

} // namespace choralesearch

#include "ChoraleInfo.hpp"

#include <algorithm>
#include <cctype>
#include <map>

namespace choralesearch {

namespace {

void appendUtf8(std::string& out, unsigned long codePoint) {
    if (codePoint < 0x80) {
        out += static_cast<char>(codePoint);
    } else if (codePoint < 0x800) {
        out += static_cast<char>(0xC0 | (codePoint >> 6));
        out += static_cast<char>(0x80 | (codePoint & 0x3F));
    } else if (codePoint < 0x10000) {
        out += static_cast<char>(0xE0 | (codePoint >> 12));
        out += static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (codePoint & 0x3F));
    } else {
        out += static_cast<char>(0xF0 | (codePoint >> 18));
        out += static_cast<char>(0x80 | ((codePoint >> 12) & 0x3F));
        out += static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (codePoint & 0x3F));
    }
}

// The modes a key interpretation can label (*a:dor), the way Humdrum abbreviates them.
const std::map<std::string, std::string>& modeLabels() {
    static const std::map<std::string, std::string> labels = {
        {"ion", "major"}, {"dor", "dorian"}, {"phr", "phrygian"}, {"lyd", "lydian"},
        {"mix", "mixolydian"}, {"aeo", "minor"}, {"loc", "locrian"},
    };
    return labels;
}

} // namespace

std::optional<std::size_t> integerFromChoraleId(const std::string& id) {
    std::size_t digitsStart = id.size();
    while (digitsStart > 0 && std::isdigit(static_cast<unsigned char>(id[digitsStart - 1]))) --digitsStart;
    if (digitsStart == id.size() || id.size() - digitsStart > 9) return std::nullopt;
    return static_cast<std::size_t>(std::stoul(id.substr(digitsStart)));
}

const std::vector<std::string>& modeNames() {
    static const std::vector<std::string> names = {
        "major", "minor", "ionian", "aeolian", "dorian", "phrygian", "lydian", "mixolydian", "locrian",
    };
    return names;
}

std::optional<std::string> normalizeModeName(const std::string& name) {
    std::string lower = name;
    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return std::tolower(c); });
    if (lower == "ionian") return "major";
    if (lower == "aeolian") return "minor";
    const auto& names = modeNames();
    if (std::find(names.begin(), names.end(), lower) != names.end()) return lower;
    return std::nullopt;
}

std::string lowercaseLatin(const std::string& text) {
    std::string lower;
    lower.reserve(text.size());
    for (std::size_t i = 0; i < text.size(); ++i) {
        const unsigned char c = static_cast<unsigned char>(text[i]);
        // Ä, Ö, Ü and the other capitals of Latin-1 are C3 80 to C3 9E (not C3 97, the multiplication
        // sign), and their small letters lie 0x20 higher.
        if (c == 0xC3 && i + 1 < text.size()) {
            const unsigned char next = static_cast<unsigned char>(text[i + 1]);
            lower += static_cast<char>(c);
            lower += static_cast<char>(next >= 0x80 && next <= 0x9E && next != 0x97 ? next + 0x20 : next);
            ++i;
            continue;
        }
        lower += c < 0x80 ? static_cast<char>(std::tolower(c)) : static_cast<char>(c);
    }
    return lower;
}

std::string decodeHtmlEntities(const std::string& text) {
    static const std::map<std::string, unsigned long> named = {
        {"auml", 0xE4}, {"ouml", 0xF6}, {"uuml", 0xFC}, {"Auml", 0xC4}, {"Ouml", 0xD6},
        {"Uuml", 0xDC}, {"szlig", 0xDF}, {"amp", '&'},
    };
    std::string decoded;
    for (std::size_t i = 0; i < text.size(); ++i) {
        const std::size_t semicolon = text[i] == '&' ? text.find(';', i) : std::string::npos;
        if (semicolon == std::string::npos || semicolon - i > 10) {
            decoded += text[i];
            continue;
        }
        const std::string name = text.substr(i + 1, semicolon - i - 1);
        std::optional<unsigned long> codePoint;
        if (auto it = named.find(name); it != named.end()) {
            codePoint = it->second;
        } else if (name.size() > 1 && name[0] == '#') {
            const bool hex = name[1] == 'x' || name[1] == 'X';
            const std::string digits = name.substr(hex ? 2 : 1);
            if (!digits.empty() && digits.find_first_not_of(hex ? "0123456789abcdefABCDEF" : "0123456789") == std::string::npos) {
                codePoint = std::stoul(digits, nullptr, hex ? 16 : 10);
            }
        }
        if (!codePoint) {
            decoded += text[i];
            continue;
        }
        appendUtf8(decoded, *codePoint);
        i = semicolon;
    }
    return decoded;
}

std::string modeFromKeyDesignation(const std::string& key, const std::string& label) {
    const auto known = modeLabels().find(label);
    if (known != modeLabels().end()) return known->second;
    return !key.empty() && std::islower(static_cast<unsigned char>(key[0])) ? "minor" : "major";
}

} // namespace choralesearch

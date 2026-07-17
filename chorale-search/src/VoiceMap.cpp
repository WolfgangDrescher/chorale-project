#include "VoiceMap.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <optional>
#include <sstream>
#include <stdexcept>

namespace choralesearch {

namespace {

std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
    return s;
}

const std::array<std::string, 4> kNames = {"bass", "tenor", "alto", "soprano"};
const std::array<std::string, 4> kLetters = {"b", "t", "a", "s"};

} // namespace

std::string voiceLabel(std::size_t voice) {
    if (voice < 1 || voice > 4) return "?";
    std::string s = kNames[voice - 1];
    s[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(s[0])));
    return s;
}

namespace {

std::vector<std::string> splitComma(const std::string& s) {
    std::vector<std::string> out;
    std::stringstream ss(s);
    std::string tok;
    while (std::getline(ss, tok, ',')) {
        std::size_t b = tok.find_first_not_of(" \t");
        std::size_t e = tok.find_last_not_of(" \t");
        if (b == std::string::npos) continue;
        out.push_back(tok.substr(b, e - b + 1));
    }
    return out;
}

// Multi-digit expansion for tokens like "124" or "4321": every character must
// be a digit 1-4. Returns std::nullopt if the token isn't a pure digit string,
// so callers fall back to name-based resolution.
std::optional<std::vector<std::size_t>> expandDigitList(const std::string& tokLower) {
    if (tokLower.empty()) return std::nullopt;
    std::vector<std::size_t> voices;
    for (char c : tokLower) {
        if (!std::isdigit(static_cast<unsigned char>(c))) return std::nullopt;
        int v = c - '0';
        if (v < 1 || v > 4) {
            throw std::invalid_argument("Unknown voice selector: '" + tokLower + "' (digits must be 1-4)");
        }
        voices.push_back(static_cast<std::size_t>(v));
    }
    return voices;
}

std::size_t resolveNameToken(const std::string& tokLower) {
    for (std::size_t i = 0; i < kNames.size(); ++i) {
        if (tokLower == kNames[i] || tokLower == kLetters[i]) return i + 1;
    }
    throw std::invalid_argument("Unknown voice selector: '" + tokLower + "'");
}

} // namespace

std::vector<std::size_t> resolveVoices(const std::string& selector) {
    std::string sel = toLower(selector);
    std::vector<std::size_t> result;

    if (sel.empty() || sel == "all") return {1, 2, 3, 4};

    for (const auto& tok : splitComma(sel)) {
        if (auto digits = expandDigitList(tok)) {
            result.insert(result.end(), digits->begin(), digits->end());
            continue;
        }
        result.push_back(resolveNameToken(tok));
    }
    std::sort(result.begin(), result.end());
    result.erase(std::unique(result.begin(), result.end()), result.end());
    return result;
}

} // namespace choralesearch

#pragma once

#include <cstddef>
#include <string>

namespace choralesearch {

// Reads a short value from left to right, one kind of character after the other; much faster than a
// std::regex. Holds a reference, so the text has to outlive the scanner.
class ValueScanner {
public:
    explicit ValueScanner(const std::string& text) : m_text(text) {}

    std::string digits() { return readWhile(isDigit); }
    std::string letters() { return readWhile(isLetter); }
    // The longest run of characters that are all one of `chars`.
    std::string anyOf(const std::string& chars) {
        return readWhile([&](char c) { return chars.find(c) != std::string::npos; });
    }
    // Moves past `c` if it comes next.
    bool accept(char c) {
        if (m_position >= m_text.size() || m_text[m_position] != c) return false;
        ++m_position;
        return true;
    }
    bool atEnd() const { return m_position == m_text.size(); }

private:
    // ASCII only: the locale has no say.
    static bool isDigit(char c) { return c >= '0' && c <= '9'; }
    static bool isLetter(char c) { return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'); }

    template <typename Accept>
    std::string readWhile(Accept accept) {
        std::size_t begin = m_position;
        while (m_position < m_text.size() && accept(m_text[m_position])) ++m_position;
        return m_text.substr(begin, m_position - begin);
    }

    const std::string& m_text;
    std::size_t m_position = 0;
};

} // namespace choralesearch

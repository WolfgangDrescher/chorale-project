#pragma once

#include <cstddef>
#include <vector>

namespace choralesearch {

// A set of verses, by their number from 1: those a match holds in, or that have a word on some notes. A match
// that reads no lyrics holds in all of them.
class VerseSet {
public:
    static VerseSet all() { return VerseSet(~0u); }
    static VerseSet none() { return VerseSet(0u); }
    static VerseSet of(std::size_t verse) { return VerseSet(1u << (verse - 1)); }

    bool empty() const { return m_bits == 0; }
    bool contains(std::size_t verse) const { return (m_bits & of(verse).m_bits) != 0; }
    VerseSet& operator&=(VerseSet other) { m_bits &= other.m_bits; return *this; }
    VerseSet& operator|=(VerseSet other) { m_bits |= other.m_bits; return *this; }
    friend VerseSet operator&(VerseSet a, VerseSet b) { return VerseSet(a.m_bits & b.m_bits); }
    bool operator==(VerseSet other) const { return m_bits == other.m_bits; }

    // The numbers of the verses, for a set a lyrics key narrowed down to a chorale's verses.
    std::vector<std::size_t> list() const {
        std::vector<std::size_t> verses;
        for (std::size_t verse = 1; verse <= kMaxVerses; ++verse) {
            if (contains(verse)) verses.push_back(verse);
        }
        return verses;
    }

    static constexpr std::size_t kMaxVerses = 32;

private:
    explicit VerseSet(unsigned bits) : m_bits(bits) {}
    unsigned m_bits;
};

} // namespace choralesearch

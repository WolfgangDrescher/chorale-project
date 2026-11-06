#include "ChoraleLyrics.hpp"

#include <algorithm>
#include <optional>
#include <utility>

#include "HumdrumUtils.hpp"

namespace choralesearch {

namespace {

// The hyphens that join the syllables of a word are taken off before, see withoutHyphens().
const std::vector<std::string> kEdgePunctuation = {
    ".", ",", ";", ":", "!", "?", "'", "’", "\"", "(", ")", "–", "-", " ",
};

// tolower() works a byte at a time and leaves a two-byte letter alone; the lyrics have no others than
// these. ß becomes ss, as the lyrics also spell it (dass, grosser).
const std::vector<std::pair<std::string, std::string>> kLettersBeyondAscii = {
    {"Ä", "ä"}, {"Ö", "ö"}, {"Ü", "ü"}, {"ß", "ss"},
};

bool startsWith(const std::string& text, const std::string& prefix) {
    return text.compare(0, prefix.size(), prefix) == 0;
}

bool endsWith(const std::string& text, const std::string& suffix) {
    return text.size() >= suffix.size() && text.compare(text.size() - suffix.size(), suffix.size(), suffix) == 0;
}

std::string replaceAll(std::string text, const std::string& from, const std::string& to) {
    for (std::size_t at = text.find(from); at != std::string::npos; at = text.find(from, at + to.size())) {
        text.replace(at, from.size(), to);
    }
    return text;
}

// A syllable without the hyphens that join it to its neighbours: "Her-" is "Her", "-zen" is "zen".
std::string withoutHyphens(const std::string& written) {
    const std::size_t first = written.find_first_not_of('-');
    if (first == std::string::npos) return "";
    return written.substr(first, written.find_last_not_of('-') - first + 1);
}

// Only the hyphen at the end of a syllable joins the next one: the hyphenation is not always consistent
// ("Eh-", "ren", "-durch").
bool continuesWord(const std::string& written) {
    return endsWith(written, "-");
}

} // namespace

const char* syllablePositionName(SyllablePosition position) {
    switch (position) {
        case SyllablePosition::Single: return "single";
        case SyllablePosition::Initial: return "initial";
        case SyllablePosition::Medial: return "medial";
        case SyllablePosition::Terminal: return "terminal";
    }
    return "single";
}

const char* melismaRoleName(MelismaRole role) {
    switch (role) {
        case MelismaRole::None: return "none";
        case MelismaRole::Initial: return "initial";
        case MelismaRole::Medial: return "medial";
        case MelismaRole::Terminal: return "terminal";
    }
    return "none";
}

std::string normalizeLyricsText(const std::string& text) {
    std::string normalized = replaceAll(text, " ", " ");
    for (char& c : normalized) {
        if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
    }
    for (const auto& [letter, normal] : kLettersBeyondAscii) normalized = replaceAll(normalized, letter, normal);

    for (bool stripped = true; stripped;) {
        stripped = false;
        for (const std::string& mark : kEdgePunctuation) {
            if (startsWith(normalized, mark)) {
                normalized.erase(0, mark.size());
                stripped = true;
            }
            if (endsWith(normalized, mark)) {
                normalized.erase(normalized.size() - mark.size());
                stripped = true;
            }
        }
    }
    return normalized;
}

// A star stands for any characters, the shortest run first; the rest has to match letter by letter.
bool matchesGlob(std::string_view glob, std::string_view text) {
    if (glob.empty()) return text.empty();
    if (glob.front() == '*') {
        for (std::size_t skipped = 0; skipped <= text.size(); ++skipped) {
            if (matchesGlob(glob.substr(1), text.substr(skipped))) return true;
        }
        return false;
    }
    return !text.empty() && text.front() == glob.front() && matchesGlob(glob.substr(1), text.substr(1));
}

bool matchesLyricsGlob(const std::string& glob, const std::string& text) {
    return matchesGlob(glob, text);
}

MelismaRole LyricsSyllable::melismaRoleAtLine(int lineNumber) const {
    if (lines.size() < 2) return MelismaRole::None;
    // The last of its attacks on or before the line: the note sounding there.
    auto after = std::upper_bound(lines.begin(), lines.end(), lineNumber);
    if (after == lines.begin()) return MelismaRole::None;
    const int attackLine = *(after - 1);
    if (attackLine == lines.front()) return MelismaRole::Initial;
    if (attackLine == lines.back()) return MelismaRole::Terminal;
    return MelismaRole::Medial;
}

const LyricsSyllable* LyricsVerse::syllableAtLine(int lineNumber) const {
    if (lineNumber < 1 || lineNumber > static_cast<int>(m_syllableByLine.size())) return nullptr;
    return m_syllableByLine[lineNumber - 1];
}

ChoraleLyrics::ChoraleLyrics(hum::HTp soprano, const std::vector<hum::HTp>& textSpines) {
    hum::HumdrumFile& file = *soprano->getOwner()->getOwner();
    const int lineCount = file.getLineCount();

    for (hum::HTp textSpine : textSpines) {
        std::vector<std::string> writtenByLine(lineCount);
        for (hum::HTp token = textSpine->getNextToken(); token; token = token->getNextToken()) {
            if (token->getOwner()->isData() && !token->isNull()) writtenByLine[token->getLineIndex()] = *token;
        }

        // Note by note down the soprano, see LyricsSyllable::lines for the notes a syllable sounds on.
        LyricsVerse verse;
        std::vector<LyricsSyllable> syllables;
        bool sounding = false;
        for (hum::HTp token = soprano->getNextToken(); token; token = token->getNextToken()) {
            if (!token->getOwner()->isData() || token->isNull() || token->isSecondaryTiedNote() || token->isGrace()) continue;
            if (token->isRest()) {
                sounding = false;
                continue;
            }
            const std::string& written = writtenByLine[token->getLineIndex()];
            if (!written.empty()) {
                LyricsSyllable syllable;
                syllable.writtenText = written;
                syllable.normalizedText = normalizeLyricsText(withoutHyphens(written));
                syllable.start = token->getDurationFromStart();
                syllables.push_back(std::move(syllable));
                sounding = true;
            }
            if (!sounding) continue;
            LyricsSyllable& syllable = syllables.back();
            syllable.lines.push_back(token->getLineNumber());
            syllable.end = token->getDurationFromStart() + token->getTiedDuration();
            if (token->hasFermata()) sounding = false;
        }

        for (std::size_t s = 0; s < syllables.size(); ++s) {
            if (verse.m_words.empty() || !continuesWord(verse.m_words.back().syllables.back().writtenText)) {
                verse.m_words.push_back(LyricsWord{});
            }
            LyricsWord& word = verse.m_words.back();
            word.writtenText += withoutHyphens(syllables[s].writtenText);
            word.syllables.push_back(std::move(syllables[s]));
        }
        for (LyricsWord& word : verse.m_words) {
            word.normalizedText = normalizeLyricsText(word.writtenText);
            for (std::size_t s = 0; s < word.syllables.size(); ++s) {
                const bool first = s == 0;
                const bool last = s + 1 == word.syllables.size();
                word.syllables[s].position = first && last ? SyllablePosition::Single
                                             : first      ? SyllablePosition::Initial
                                             : last       ? SyllablePosition::Terminal
                                                          : SyllablePosition::Medial;
            }
        }

        // The syllable sounding on each data line, from its own note until its last note is over. The words
        // and syllables are all there now, so pointers to them stay good.
        verse.m_syllableByLine.assign(lineCount, nullptr);
        for (LyricsWord& word : verse.m_words) {
            for (LyricsSyllable& syllable : word.syllables) {
                syllable.word = &word;
                for (int i = syllable.firstLine() - 1; i < lineCount; ++i) {
                    if (!file[i].isData()) continue;
                    if (file[i].getDurationFromStart() >= syllable.end) break;
                    verse.m_syllableByLine[i] = &syllable;
                }
            }
        }
        m_verses.push_back(std::move(verse));
    }
}

const LyricsVerse* ChoraleLyrics::verse(std::size_t n) const {
    if (n < 1 || n > m_verses.size()) return nullptr;
    return &m_verses[n - 1];
}

} // namespace choralesearch

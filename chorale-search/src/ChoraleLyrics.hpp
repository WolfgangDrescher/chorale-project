#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include "humlib.h"

namespace choralesearch {

// Where a syllable stands in its word, as the MEI wordpos has it.
enum class SyllablePosition { Single, Initial, Medial, Terminal };

// "single", "initial", "medial", "terminal": the values of the sylpos pattern key.
const char* syllablePositionName(SyllablePosition position);

// A word or a syllable as the search compares it: in small letters, without the punctuation at either
// end ("Gott," is "gott"), with a plain space for the no-break space of two words elided onto one note.
std::string normalizeLyricsText(const std::string& text);

// Whether `text` is what a word or syl pattern value asks for: the value itself, with "*" standing for any
// characters ("her*", "*zen", "*erz*"). Both are normalized, see normalizeLyricsText().
bool matchesLyricsGlob(const std::string& glob, const std::string& text);

struct LyricsWord;

// One syllable of a verse, and the soprano notes it sounds on.
struct LyricsSyllable {
    std::string writtenText;    // as the **text spine has it: "Her-", "-zen", "Gott,"
    std::string normalizedText; // without the hyphens, see normalizeLyricsText(): "her"
    const LyricsWord* word = nullptr;
    SyllablePosition position = SyllablePosition::Single;
    // The lines of the soprano attacks it sounds on: its own, then the attacks without text up to the next
    // syllable, a rest or a fermata. After a rest or a fermata, attacks without text are silent.
    std::vector<int> lines;
    hum::HumNum start; // where it begins, in quarter notes from the start of the piece
    hum::HumNum end;   // where its last note is over

    int firstLine() const { return lines.front(); }
    int lastLine() const { return lines.back(); }
};

// A word of a verse: its syllables one after the other.
struct LyricsWord {
    std::string writtenText;    // "Herzen", "Gott,"
    std::string normalizedText; // see normalizeLyricsText(): "herzen", "gott"
    std::vector<LyricsSyllable> syllables;

    int firstLine() const { return syllables.front().firstLine(); }
    int lastLine() const { return syllables.back().lastLine(); }
    hum::HumNum start() const { return syllables.front().start; }
    hum::HumNum end() const { return syllables.back().end; }
};

// One **text spine: a stanza, or one line of text where the music repeats. Its words in the order of the
// score, and which syllable sounds on each line. Never copied: the search keeps pointers into it.
class LyricsVerse {
public:
    LyricsVerse() = default;
    LyricsVerse(const LyricsVerse&) = delete;
    LyricsVerse& operator=(const LyricsVerse&) = delete;
    LyricsVerse(LyricsVerse&&) = default;
    LyricsVerse& operator=(LyricsVerse&&) = default;

    const std::vector<LyricsWord>& words() const { return m_words; }

    // The syllable sounding on `lineNumber` (1-indexed), on the lines where only other voices move too;
    // nullptr where none sounds.
    const LyricsSyllable* syllableAtLine(int lineNumber) const;

private:
    friend class ChoraleLyrics;
    std::vector<LyricsWord> m_words;
    std::vector<const LyricsSyllable*> m_syllableByLine; // by 0-indexed line
};

// The lyrics of a chorale, read from its **text spines, one verse each, as they sit under the soprano.
class ChoraleLyrics {
public:
    ChoraleLyrics() = default;
    ChoraleLyrics(const ChoraleLyrics&) = delete;
    ChoraleLyrics& operator=(const ChoraleLyrics&) = delete;
    ChoraleLyrics(ChoraleLyrics&&) = default;
    ChoraleLyrics& operator=(ChoraleLyrics&&) = default;
    // `soprano` is the **kern spine the syllables sound in, `textSpines` the **text spines in the order of
    // the verses.
    ChoraleLyrics(hum::HTp soprano, const std::vector<hum::HTp>& textSpines);

    std::size_t verseCount() const { return m_verses.size(); }
    // 1-indexed like HumdrumChorale::spine(); nullptr for a verse the chorale does not have.
    const LyricsVerse* verse(std::size_t n) const;

private:
    std::vector<LyricsVerse> m_verses;
};

} // namespace choralesearch

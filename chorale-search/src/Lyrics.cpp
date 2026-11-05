#include "Lyrics.hpp"

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

#include <nlohmann/json.hpp>

namespace choralesearch {

namespace {

// Verovio draws a space in a syllable ("1. An") as an elision tie.
std::string withNoBreakSpaces(std::string syllable) {
    for (std::size_t at = syllable.find(' '); at != std::string::npos; at = syllable.find(' ', at)) {
        syllable.replace(at, 1, "\u00a0");
    }
    return syllable;
}

void reparse(hum::HumdrumFile& infile) {
    std::stringstream text;
    text << infile;
    infile.readString(text.str());
}

} // namespace

bool isSyllableAttack(hum::HTp token) {
    return !token->isNull() && !token->isRest() && !token->isGrace() && !token->isSecondaryTiedNote();
}

std::map<std::string, std::vector<Verse>> readLyrics(const std::filesystem::path& path) {
    std::ifstream f(path);
    if (!f.is_open()) {
        throw std::runtime_error("Could not open lyrics file: " + path.string());
    }
    nlohmann::json root = nlohmann::json::parse(f, nullptr, true, /*ignore_comments=*/true);

    std::map<std::string, std::vector<Verse>> byChorale;
    for (const auto& [choraleId, entry] : root.items()) {
        std::vector<Verse>& verses = byChorale[choraleId];
        for (const auto& verseJson : entry.at("verses")) {
            Verse verse;
            for (const auto& syllable : verseJson) {
                if (syllable.is_null()) verse.emplace_back();
                else verse.emplace_back(syllable.get<std::string>());
            }
            verses.push_back(std::move(verse));
        }
    }
    return byChorale;
}

void applyLyrics(hum::HumdrumFile& infile, const std::string& choraleId, const std::vector<Verse>& verses) {
    if (verses.empty()) return;

    std::vector<hum::HTp> starts;
    infile.getKernSpineStartList(starts);
    if (starts.empty()) return;
    const int soprano = starts.back()->getTrack();

    std::vector<int> attackLines;
    for (int i = 0; i < infile.getLineCount(); ++i) {
        if (!infile[i].isData()) continue;
        for (int j = 0; j < infile[i].getFieldCount(); ++j) {
            hum::HTp token = infile.token(i, j);
            if (token->getTrack() == soprano && isSyllableAttack(token)) {
                attackLines.push_back(i);
                break;
            }
        }
    }
    if (attackLines.empty()) return;
    for (const Verse& verse : verses) {
        if (verse.size() != attackLines.size()) {
            std::cerr << choraleId << ": " << verse.size() << " syllables for " << attackLines.size()
                      << " soprano notes, leaving out the lyrics\n";
            return;
        }
    }

    // At the end of the line, after the computed spines of the soprano if there are any. Last verse
    // first, since each one is inserted right there.
    const int lastTrack = infile.getMaxTrack();
    for (auto verse = verses.rbegin(); verse != verses.rend(); ++verse) {
        std::vector<std::string> syllables(infile.getLineCount());
        for (std::size_t n = 0; n < attackLines.size(); ++n) {
            if ((*verse)[n]) syllables[attackLines[n]] = withNoBreakSpaces(*(*verse)[n]);
        }
        infile.insertDataSpineAfter(lastTrack, syllables, ".", "**text");
    }
    reparse(infile);
}

} // namespace choralesearch

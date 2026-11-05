#include "Lyrics.hpp"

#include <algorithm>
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

// Two for the bar form [A,A,B], one without repeats, none for other repeats (Te Deum).
int linesPerStanza(hum::HTp soprano) {
    bool repeated = false;
    for (hum::HTp token = soprano; token; token = token->getNextToken()) {
        if (token->compare(0, 3, "*>[") == 0) {
            std::vector<std::string> sections;
            std::stringstream list(token->substr(3, token->size() - 4));
            for (std::string section; std::getline(list, section, ',');) sections.push_back(section);
            if (sections.size() > 1 && sections[0] == sections[1]) return 2;
            std::sort(sections.begin(), sections.end());
            return std::adjacent_find(sections.begin(), sections.end()) == sections.end() ? 1 : 0;
        }
        if (token->isBarline() && token->find(":|") != std::string::npos) repeated = true;
    }
    return repeated ? 2 : 1;
}

// "1.", "", "2.", "" for two stanzas of two lines each. None unless every stanza but the last ends
// a sentence, otherwise the lines belong to one stanza.
std::vector<std::string> stanzaLabels(const std::vector<Verse>& verses, int linesPerStanza) {
    std::vector<std::string> labels(verses.size());
    if (linesPerStanza == 0 || verses.size() % linesPerStanza || verses.size() / linesPerStanza < 2) return labels;
    for (std::size_t line = linesPerStanza - 1; line + linesPerStanza < verses.size(); line += linesPerStanza) {
        auto last = std::find_if(verses[line].rbegin(), verses[line].rend(), [](const auto& s) { return s; });
        if (last == verses[line].rend() || (*last)->find_last_of(".!?") != (*last)->size() - 1) return labels;
    }
    for (std::size_t line = 0; line < verses.size(); line += linesPerStanza) {
        labels[line] = std::to_string(line / linesPerStanza + 1) + ".";
    }
    return labels;
}

// *v:1. above the first note, in the **text spines.
void addVerseLabels(hum::HumdrumFile& infile, int firstNoteLine, const std::vector<std::string>& labels) {
    if (std::all_of(labels.begin(), labels.end(), [](const std::string& label) { return label.empty(); })) return;
    hum::HLp line = infile.insertNullInterpretationLineAboveIndex(firstNoteLine);
    std::size_t verse = 0;
    for (int j = 0; j < line->getFieldCount(); ++j) {
        if (!infile[firstNoteLine + 1].token(j)->isDataType("**text")) continue;
        if (!labels[verse].empty()) line->token(j)->setText("*v:" + labels[verse]);
        ++verse;
    }
    line->createLineFromTokens();
    reparse(infile);
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
    const std::vector<std::string> labels = stanzaLabels(verses, linesPerStanza(starts.back()));

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
    addVerseLabels(infile, attackLines.front(), labels);
}

} // namespace choralesearch

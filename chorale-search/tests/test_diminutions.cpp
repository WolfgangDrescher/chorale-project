#include "test_framework.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

#include "CorpusSearch.hpp"
#include "HumdrumChorale.hpp"
#include "JsonIO.hpp"
#include "LinePositions.hpp"
#include "PatternKeys.hpp"
#include "Query.hpp"
#include "VoiceMap.hpp"
#include "humlib.h"

namespace fs = std::filesystem;

using choralesearch::CorpusSearch;
using choralesearch::HumdrumChorale;
using choralesearch::LinePosition;
using choralesearch::Query;
using choralesearch::Result;

// The queries of queries/diminutions.json, with which the webapp marks the diminutions of a chorale, must find
// exactly what annotations/bach-diminutions.json annotates by hand in each chorale it names, read from the
// scores of the submodule. An annotation is a note where it is attacked, as "measure/beat" and voice:
// ["3/2+1/2", "alto"]. Or it is a figure, its voice and where each of its notes sounds, by role:
// {"voice": "tenor", "preparation": "3/4", "suspension": "4/1", "resolution": "4/1+1/2"}. A match is a figure of
// the notes of its named positions.

namespace {

const fs::path kTestsDir = fs::path(__FILE__).parent_path();
const fs::path kProjectDir = kTestsDir.parent_path().parent_path();
const fs::path kAnnotations = kProjectDir / "annotations" / "bach-diminutions.json";
const fs::path kScoresDir = kProjectDir / "bach-370-chorales" / "kern";
const std::vector<std::string> kVoiceNames = {"bass", "tenor", "alto", "soprano"};

nlohmann::json readJson(const fs::path& path) {
    std::ifstream file(path);
    return nlohmann::json::parse(file);
}

// The line on which the voice attacks the note it sounds on `lineNumber`, also one it holds or ties from before.
// 0 where it rests or has no note yet.
int attackLineAt(const HumdrumChorale& chorale, std::size_t voice, int lineNumber) {
    hum::HTp note = chorale.soundingTokenAtLine(choralesearch::kKernKey, voice, lineNumber);
    while (note && note->isSecondaryTiedNote()) {
        do note = note->getPreviousToken();
        while (note && (!note->getOwner()->isData() || note->isNull()));
    }
    return note && note->getOwner()->isData() && !note->isRest() ? note->getLineNumber() : 0;
}

// By the id of a diminution, its queries.
nlohmann::json readDiminutions() {
    return readJson(kTestsDir.parent_path() / "queries" / "diminutions.json");
}

// Every query of every diminution, its results told apart by the id of the diminution.
std::vector<Query> diminutionQueries() {
    std::vector<Query> queries;
    const nlohmann::json diminutions = readDiminutions();
    for (const auto& [diminution, queriesOfDiminution] : diminutions.items()) {
        for (Query& query : choralesearch::queryArrayFromJson(queriesOfDiminution)) {
            query.id = diminution;
            queries.push_back(std::move(query));
        }
    }
    return queries;
}

// The lines of the notes of a figure, in order, and its voice. A single note is a figure of one line.
using Figure = std::pair<std::vector<int>, std::size_t>;

std::string positionOfLine(int line, const std::vector<LinePosition>& positions) {
    auto position = std::find_if(positions.begin(), positions.end(), [&](const LinePosition& p) { return p.line == line; });
    return position == positions.end() ? "?" : position->position;
}

// The first line of a position, 0 if the chorale has none.
int lineOfPosition(const std::string& position, const std::vector<LinePosition>& positions) {
    auto linePosition = std::find_if(positions.begin(), positions.end(), [&](const LinePosition& p) { return p.position == position; });
    return linePosition == positions.end() ? 0 : linePosition->line;
}

// "3/2 alto, 3/4 4/1 4/1+1/2 tenor"
std::string describe(const std::set<Figure>& figures, const std::vector<LinePosition>& positions) {
    std::string text;
    for (const auto& [lines, voice] : figures) {
        text += text.empty() ? "" : ", ";
        for (int line : lines) text += positionOfLine(line, positions) + " ";
        text += kVoiceNames[voice - 1];
    }
    return text;
}

} // namespace

TEST_CASE(the_queries_find_the_annotated_diminutions_and_no_others) {
    const std::vector<Query> queries = diminutionQueries();
    const nlohmann::json annotations = readJson(kAnnotations);
    REQUIRE(!annotations.empty());

    for (const auto& [choraleId, annotatedDiminutions] : annotations.items()) {
        const std::string path = (kScoresDir / (choraleId + ".krn")).string();
        REQUIRE(fs::exists(path)); // the submodule bach-370-chorales, see git submodule update --init
        const HumdrumChorale chorale(path);
        hum::HumdrumFile infile(path);
        const std::vector<LinePosition> positions = choralesearch::linePositions(infile);

        std::map<std::string, std::set<Figure>> foundFigures;
        std::map<std::string, std::set<Figure>> foundNotes;
        for (const Result& result : CorpusSearch(path).run(queries)) {
            std::set<int> lines;
            for (const auto& [name, line] : result.namedPositionLines) lines.insert(static_cast<int>(line));
            foundFigures[*result.queryId].insert({{lines.begin(), lines.end()}, result.voice});
            for (int line : lines) foundNotes[*result.queryId].insert({{line}, result.voice});
        }

        // A diminution the annotations of a chorale leave out is not checked in it.
        for (const auto& [diminution, annotationsOfDiminution] : annotatedDiminutions.items()) {
            const std::string where = choraleId + " " + diminution + ": ";
            bool ofFigures = false;
            std::set<Figure> annotated;
            for (const auto& annotation : annotationsOfDiminution) {
                if (annotation.is_object()) {
                    ofFigures = true;
                    std::vector<int> lines;
                    for (const auto& [role, position] : annotation.items()) {
                        if (role == "voice") continue;
                        const int line = lineOfPosition(position.get<std::string>(), positions);
                        CHECK_EQ(where + position.get<std::string>() + (line ? "" : " is no position of the chorale"), where + position.get<std::string>());
                        lines.push_back(line);
                    }
                    std::sort(lines.begin(), lines.end());
                    annotated.insert({lines, choralesearch::resolveVoices(annotation.at("voice").get<std::string>()).front()});
                    continue;
                }
                const std::string position = annotation.at(0).get<std::string>();
                const std::string voiceName = annotation.at(1).get<std::string>();
                const std::size_t voice = choralesearch::resolveVoices(voiceName).front();
                const int line = lineOfPosition(position, positions);
                const std::string problem = !line ? " is no position of the chorale" : attackLineAt(chorale, voice, line) != line ? " is no attack" : "";
                CHECK_EQ(where + position + " " + voiceName + problem, where + position + " " + voiceName);
                annotated.insert({{line}, voice});
            }
            const std::set<Figure>& found = ofFigures ? foundFigures[diminution] : foundNotes[diminution];
            std::set<Figure> notAnnotated, notFound;
            std::set_difference(found.begin(), found.end(), annotated.begin(), annotated.end(),
                                std::inserter(notAnnotated, notAnnotated.end()));
            std::set_difference(annotated.begin(), annotated.end(), found.begin(), found.end(),
                                std::inserter(notFound, notFound.end()));
            CHECK_EQ(where + "found but not annotated: " + describe(notAnnotated, positions), where + "found but not annotated: ");
            CHECK_EQ(where + "annotated but not found: " + describe(notFound, positions), where + "annotated but not found: ");
        }
    }
}

TEST_CASE(every_annotated_diminution_has_queries) {
    const nlohmann::json diminutions = readDiminutions();
    const nlohmann::json annotations = readJson(kAnnotations);
    for (const auto& [choraleId, annotatedDiminutions] : annotations.items()) {
        for (const auto& [diminution, annotatedNotes] : annotatedDiminutions.items()) CHECK(diminutions.contains(diminution));
    }
}

TEST_MAIN()

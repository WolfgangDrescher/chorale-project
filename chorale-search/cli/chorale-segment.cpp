#include <algorithm>
#include <cctype>
#include <iostream>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#if defined(__unix__) || defined(__APPLE__)
#include <unistd.h>
#endif

#include <nlohmann/json.hpp>

#include "CorpusSearch.hpp"
#include "SplitScoreIntoVoices.hpp"
#include "HumdrumChorale.hpp"
#include "HumdrumUtils.hpp"
#include "ProgressReport.hpp"
#include "Query.hpp"
#include "Result.hpp"
#include "ScoreImport.hpp"
#include "Segmentation.hpp"

using choralesearch::CorpusSearch;
using choralesearch::HumdrumChorale;
using choralesearch::Query;
using choralesearch::Result;
using choralesearch::Segment;
using choralesearch::SegmentationOptions;
using choralesearch::SegmentQueryOptions;

namespace {

constexpr int kExitError = 1;
constexpr int kExitInvalidArgumentError = 2;
constexpr int kExitValidationError = 3;

void printUsage(const char* argv0) {
    std::cerr <<
        "Usage: " << argv0 << " INPUT [OPTIONS]\n"
        "\n"
        "Arguments:\n"
        "    INPUT                 the score: a Humdrum **kern or MusicXML file, or '-' for\n"
        "                          stdin. Four voices pass through, a two-staff score is split\n"
        "                          into four. Analysis spines are derived here.\n"
        "\n"
        "Options:\n"
        "    --length N            segment length in quarter notes (default: 4)\n"
        "    --mint-ignore-quality true|false\n"
        "                          leave the interval qualities out of the queries (\"+M2\"\n"
        "                          becomes \"+2\") so a passage is found in major and minor\n"
        "                          (default: true)\n"
        "    --mint-allow-interval-complementation true|false\n"
        "                          let a query's interval also match its inversion (\"+3\" also\n"
        "                          finds \"-6\") (default: true)\n"
        "    --metweight-skip-unclassified true|false\n"
        "                          fold the ornaments (notes on unclassified metric positions)\n"
        "                          away in the queries instead of counting them (default: true)\n"
        "    --inner-voices true|false\n"
        "                          also ask for the harmony the inner voices make with the\n"
        "                          bass: the chord at every bass note, compared exactly and\n"
        "                          in no particular order of the voices (default: false)\n"
        "    --no-kern             leave the converted four-voice **kern text out of the\n"
        "                          output -- for command-line use, where the score is\n"
        "                          already at hand and only the segments matter\n"
        "    --stats CORPUS_DIR    search the corpus for every segment's query and add a\n"
        "                          \"stats\" property to each segment: matches, choraleCount\n"
        "                          and topChorales\n"
        "    --no-analysis         with --stats: read the analysis spines straight from the\n"
        "                          corpus instead of deriving them per run -- for a corpus\n"
        "                          built by chorale-generate --analysis\n"
        "    --progress            write the progress to stderr, one JSON object per line: the\n"
        "                          stage being worked on, and with --stats one event for every\n"
        "                          chorale file searched\n"
        "    --help, -h            show this help\n";
}

hum::HumNum parseLength(const std::string& value) {
    try {
        std::size_t consumed = 0;
        int length = std::stoi(value, &consumed);
        if (consumed != value.size() || length <= 0) throw std::invalid_argument("");
        return length;
    } catch (const std::exception&) {
        throw std::invalid_argument("--length takes a single positive whole number of quarter notes, got '" + value +
                                     "'");
    }
}

// true/false, yes/no, y/n or 1/0, in any case.
bool parseBoolean(const std::string& flag, const std::string& value) {
    std::string lowered = value;
    std::transform(lowered.begin(), lowered.end(), lowered.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (lowered == "true" || lowered == "yes" || lowered == "y" || lowered == "1") return true;
    if (lowered == "false" || lowered == "no" || lowered == "n" || lowered == "0") return false;
    throw std::invalid_argument(flag + " takes true/false, yes/no, y/n or 1/0, got '" + value + "'");
}

// Whether stdin is a terminal rather than a pipe -- reading it would sit and wait for a human
// to type a score.
bool stdinIsInteractive() {
#if defined(__unix__) || defined(__APPLE__)
    return isatty(fileno(stdin)) != 0;
#else
    return false;
#endif
}

std::string readInput(const std::string& inputPath) {
    std::ostringstream content;
    if (inputPath == "-") {
        content << std::cin.rdbuf();
    } else {
        std::ifstream file(inputPath, std::ios::binary);
        if (!file.is_open()) throw std::invalid_argument("no such file: " + inputPath);
        content << file.rdbuf();
    }
    return content.str();
}

// The full stats per segment id, gathered in one corpus pass over all the segments' queries:
// what chorale-search --stats says about a single query (matches, choraleCount), plus the
// topChorales ranking this tool adds -- a segment is looked at one at a time, so where its
// passage turns up is worth naming.
std::map<std::string, nlohmann::json> statsForSegments(const std::vector<Segment>& segments,
                                                        const std::string& corpusDir, bool applyAnalysis,
                                                        bool reportProgress) {
    std::vector<Query> queries;
    queries.reserve(segments.size());
    for (const Segment& segment : segments) queries.push_back(segment.query);

    CorpusSearch search(corpusDir, applyAnalysis);
    if (reportProgress) search.setProgressCallback(choralesearch::progressToStderr());
    const choralesearch::Results results = search.run(queries);

    std::map<std::string, std::map<std::string, std::size_t>> matchesPerChorale; // by queryId, then choraleId
    for (const Result& result : results) {
        ++matchesPerChorale[result.queryId.value_or("")][result.choraleId];
    }

    // Walked over the segments rather than over what the corpus answered, so a segment whose
    // query matched nothing is built by these same lines and states its zeroes. Nowhere else
    // does a stats object have to be spelled out, so renaming a property is one edit.
    std::map<std::string, nlohmann::json> stats;
    for (const Segment& segment : segments) {
        const std::string id = segment.query.id.value_or("");
        const std::map<std::string, std::size_t>& perChorale = matchesPerChorale[id];

        std::size_t matches = 0;
        for (const auto& [choraleId, count] : perChorale) matches += count;

        // Where the matches pile up, most first. Capped, so a segment whose passage is
        // everywhere doesn't answer with the whole corpus -- five is enough to see whether the
        // matches cluster or scatter. Ties keep chorale order, which the map already has.
        std::vector<std::pair<std::string, std::size_t>> ranked(perChorale.begin(), perChorale.end());
        std::stable_sort(ranked.begin(), ranked.end(), [](const auto& a, const auto& b) { return a.second > b.second; });
        constexpr std::size_t kTopChorales = 5;
        nlohmann::json top = nlohmann::json::array();
        for (std::size_t i = 0; i < ranked.size() && i < kTopChorales; ++i) {
            top.push_back({{"choraleId", ranked[i].first}, {"matches", ranked[i].second}});
        }

        stats[id] = {
            {"matches", matches},
            {"choraleCount", perChorale.size()},
            {"topChorales", std::move(top)},
        };
    }
    return stats;
}

nlohmann::json segmentToJson(const Segment& segment) {
    nlohmann::json entry;
    entry["id"] = segment.query.id.value_or("");
    entry["startPosition"] = choralesearch::humNumToString(segment.startPosition);
    entry["endPosition"] = choralesearch::humNumToString(segment.endPosition);
    entry["startLine"] = segment.startLineNumber;
    entry["endLine"] = segment.endLineNumber;
    entry["query"] = choralesearch::queryToJson(segment.query);
    return entry;
}

} // namespace

int main(int argc, char** argv) {
    std::string inputPath;
    bool includeKern = true;
    std::string statsCorpusDir;
    bool applyAnalysis = true;
    bool progress = false;
    SegmentationOptions segmentationOptions;
    SegmentQueryOptions queryOptions;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        auto next = [&](const char* flag) -> std::string {
            if (i + 1 >= argc) throw std::invalid_argument(std::string(flag) + " needs a value");
            return argv[++i];
        };
        try {
            if (arg == "--length") { segmentationOptions.length = parseLength(next("--length")); }
            else if (arg == "--mint-ignore-quality") {
                queryOptions.ignoreIntervalQuality =
                    parseBoolean("--mint-ignore-quality", next("--mint-ignore-quality"));
            }
            else if (arg == "--mint-allow-interval-complementation") {
                const bool allowed = parseBoolean("--mint-allow-interval-complementation",
                                                   next("--mint-allow-interval-complementation"));
                queryOptions.matcherOptions.mintAllowIntervalComplementation =
                    allowed ? std::vector<std::string>{"*"} : std::vector<std::string>{};
            }
            else if (arg == "--metweight-skip-unclassified") {
                queryOptions.matcherOptions.metweightSkipUnclassified =
                    parseBoolean("--metweight-skip-unclassified", next("--metweight-skip-unclassified"));
            }
            else if (arg == "--inner-voices") {
                queryOptions.innerVoices = parseBoolean("--inner-voices", next("--inner-voices"));
            }
            else if (arg == "--no-kern") { includeKern = false; }
            else if (arg == "--stats") { statsCorpusDir = next("--stats"); }
            else if (arg == "--no-analysis") { applyAnalysis = false; }
            else if (arg == "--progress") { progress = true; }
            else if (arg == "--help" || arg == "-h") { printUsage(argv[0]); return 0; }
            else if (!arg.empty() && arg[0] == '-' && arg != "-") {
                std::cerr << "Unknown option: " << arg << "\n";
                printUsage(argv[0]);
                return kExitInvalidArgumentError;
            }
            else { inputPath = arg; }
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << "\n";
            return kExitInvalidArgumentError;
        }
    }

    if (inputPath.empty()) {
        printUsage(argv[0]);
        return kExitInvalidArgumentError;
    }

    if (!applyAnalysis && statsCorpusDir.empty()) {
        std::cerr << "Error: --no-analysis requires --stats\n\n";
        printUsage(argv[0]);
        return kExitInvalidArgumentError;
    }

    // '-' was asked for explicitly, but nothing is piped in -- reading would sit and wait for
    // a human to type a score, so the usage is the better answer.
    if (inputPath == "-" && stdinIsInteractive()) {
        printUsage(argv[0]);
        return kExitInvalidArgumentError;
    }

    std::string input;
    try {
        input = readInput(inputPath);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return kExitInvalidArgumentError;
    }
    if (input.find_first_not_of(" \t\r\n") == std::string::npos) {
        std::cerr << "Error: the input is empty\n\n";
        printUsage(argv[0]);
        return kExitInvalidArgumentError;
    }

    try {
        // What arrived, and what it takes to make it the corpus's own shape (four **kern
        // spines, bass to soprano): MusicXML converts first, a two-staff grand staff score is
        // pulled apart into voices, a four-voice score passes through as it is.
        const std::string inputFormat = choralesearch::looksLikeMusicXml(input) ? "musicxml" : "kern";
        if (progress && inputFormat == "musicxml") choralesearch::reportPhase("convert-musicxml");
        const std::string kernText = inputFormat == "musicxml" ? choralesearch::musicXmlToKern(input) : input;

        hum::HumdrumFile infile;
        if (!infile.readString(kernText)) {
            throw std::invalid_argument("could not parse the score as Humdrum **kern");
        }

        const std::size_t voices = choralesearch::kernTracks(infile).size();
        std::string layout;
        std::string kern;
        if (voices == 4) {
            layout = "satb";
            kern = kernText;
        } else if (voices == 2) {
            layout = "grand-staff";
            if (progress) choralesearch::reportPhase("split-score-into-voices");
            kern = choralesearch::splitScoreIntoVoices(infile);
        } else {
            throw std::invalid_argument("expected a score with 4 voices or a two-staff grand staff score, got " +
                                         std::to_string(voices) + " **kern spine(s)");
        }

        // The corpus's own header shape, whatever the input carried: canonical voice
        // interpretations instead of part/staff/instrument bookkeeping. Segmentation runs on
        // this same text, so the segments' line numbers mean lines of the output.
        kern = choralesearch::normalizeChoraleHeader(kern);

        // Building the chorale derives its analysis spines (**deg, **mint, **metweight, ...),
        // which is where most of the time before the search goes, so it is a phase of its own.
        if (progress) choralesearch::reportPhase("analyze-score");
        std::istringstream contents(kern);
        HumdrumChorale chorale(contents, inputPath == "-" ? "stdin" : inputPath);

        if (progress) choralesearch::reportPhase("segment-score");
        const std::vector<Segment> segments = choralesearch::segmentScore(chorale, segmentationOptions,
                                                                           queryOptions);

        nlohmann::json j;
        if (inputPath != "-") j["source"] = inputPath;
        j["inputFormat"] = inputFormat;
        j["layout"] = layout;
        if (includeKern) j["kern"] = kern;
        std::map<std::string, nlohmann::json> stats;
        if (!statsCorpusDir.empty()) {
            if (progress) choralesearch::reportPhase("search-corpus");
            stats = statsForSegments(segments, statsCorpusDir, applyAnalysis, progress);
        }

        j["segments"] = nlohmann::json::array();
        for (const Segment& segment : segments) {
            nlohmann::json entry = segmentToJson(segment);
            if (!stats.empty()) entry["stats"] = stats.at(segment.query.id.value_or(""));
            j["segments"].push_back(std::move(entry));
        }
        std::cout << j.dump(1, '\t') << '\n';
        std::cerr << segments.size() << " segment(s)\n";
    } catch (const std::invalid_argument& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return kExitValidationError;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return kExitError;
    }

    return 0;
}

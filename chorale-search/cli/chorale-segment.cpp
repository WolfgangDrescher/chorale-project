#include <algorithm>
#include <filesystem>
#include <iostream>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

#include "CorpusSearch.hpp"
#include "HumdrumChorale.hpp"
#include "HumdrumUtils.hpp"
#include "Query.hpp"
#include "Result.hpp"
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

void printUsage(const char* argv0) {
    std::cerr <<
        "Usage: " << argv0 << " INPUT [OPTIONS]\n"
        "\n"
        "Arguments:\n"
        "    INPUT                 the score to segment: a Humdrum **kern file. The analysis\n"
        "                          spines a query talks about (**mint, **hint-14, ...) are\n"
        "                          derived here, so an unannotated score is what to hand it\n"
        "\n"
        "Options:\n"
        "    --length N            segment length in quarter notes (default: 4)\n"
        "    --stats CORPUS_DIR    search the corpus for every segment's query and add a\n"
        "                          \"stats\" property to each segment: matches, choraleCount\n"
        "                          and topChorales\n"
        "    --no-analysis         with --stats: read the analysis spines straight from the\n"
        "                          corpus instead of deriving them per run -- for a corpus\n"
        "                          built by chorale-generate --analysis\n"
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

// The full stats per segment id, gathered in one corpus pass over all the segments' queries:
// what chorale-search --stats says about a single query (matches, choraleCount), plus the
// topChorales ranking this tool adds -- a segment is looked at one at a time, so where its
// passage turns up is worth naming.
std::map<std::string, nlohmann::json> statsForSegments(const std::vector<Segment>& segments,
                                                        const std::string& corpusDir, bool applyAnalysis) {
    std::vector<Query> queries;
    queries.reserve(segments.size());
    for (const Segment& segment : segments) queries.push_back(segment.query);

    CorpusSearch search(corpusDir, applyAnalysis);
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

// `stats` is empty unless --stats asked for it; when it did, it has an entry for every segment.
void printSegmentsAsJson(const HumdrumChorale& chorale, const std::vector<Segment>& segments,
                          const std::map<std::string, nlohmann::json>& stats) {
    nlohmann::json j;
    j["source"] = chorale.path();
    j["segments"] = nlohmann::json::array();
    for (const Segment& segment : segments) {
        nlohmann::json entry;
        entry["id"] = segment.query.id.value_or("");
        entry["startPosition"] = choralesearch::humNumToString(segment.startPosition);
        entry["endPosition"] = choralesearch::humNumToString(segment.endPosition);
        entry["startLine"] = segment.startLineNumber;
        entry["endLine"] = segment.endLineNumber;
        entry["query"] = choralesearch::queryToJson(segment.query);
        if (!stats.empty()) entry["stats"] = stats.at(segment.query.id.value_or(""));
        j["segments"].push_back(std::move(entry));
    }
    std::cout << j.dump(1, '\t') << '\n';
    std::cerr << segments.size() << " segment(s)\n";
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 2 || std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h") {
        printUsage(argv[0]);
        return (argc < 2) ? kExitInvalidArgumentError : 0;
    }

    std::string inputPath = argv[1];
    std::string statsCorpusDir;
    bool applyAnalysis = true;
    SegmentationOptions segmentationOptions;

    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        auto next = [&](const char* flag) -> std::string {
            if (i + 1 >= argc) throw std::invalid_argument(std::string(flag) + " needs a value");
            return argv[++i];
        };
        try {
            if (arg == "--length") { segmentationOptions.length = parseLength(next("--length")); }
            else if (arg == "--stats") { statsCorpusDir = next("--stats"); }
            else if (arg == "--no-analysis") { applyAnalysis = false; }
            else if (arg == "--help" || arg == "-h") { printUsage(argv[0]); return 0; }
            else {
                std::cerr << "Unknown option: " << arg << "\n";
                printUsage(argv[0]);
                return kExitInvalidArgumentError;
            }
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << "\n";
            return kExitInvalidArgumentError;
        }
    }

    if (!std::filesystem::is_regular_file(inputPath)) {
        std::cerr << "Error: no such file: " << inputPath << "\n";
        return kExitInvalidArgumentError;
    }

    // On its own it would say nothing: without a corpus to search there are no analysis spines
    // to read from one, and the segments' own are derived from the score either way.
    if (!applyAnalysis && statsCorpusDir.empty()) {
        std::cerr << "Error: --no-analysis requires --stats\n\n";
        printUsage(argv[0]);
        return kExitInvalidArgumentError;
    }

    try {
        HumdrumChorale chorale(inputPath);
        const std::vector<Segment> segments = choralesearch::segmentScore(chorale, segmentationOptions,
                                                                           SegmentQueryOptions{});
        std::map<std::string, nlohmann::json> stats;
        if (!statsCorpusDir.empty()) stats = statsForSegments(segments, statsCorpusDir, applyAnalysis);
        printSegmentsAsJson(chorale, segments, stats);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return kExitError;
    }

    return 0;
}

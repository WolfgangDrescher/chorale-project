#include <algorithm>
#include <fstream>
#include <iostream>

#include <nlohmann/json.hpp>

#include "CorpusSearch.hpp"
#include "JsonIO.hpp"
#include "Query.hpp"

using choralesearch::CorpusSearch;
using choralesearch::Query;
using choralesearch::Result;
using choralesearch::Results;

namespace {

constexpr int kExitError = 1;
constexpr int kExitInvalidArgumentError = 2;
constexpr int kExitValidationError = 3;

void printUsage(const char* argv0) {
    std::cerr <<
        "Usage: " << argv0 << " CORPUS_DIR (--query JSON | --query-file FILE.json) [OPTIONS]\n"
        "\n"
        "Arguments:\n"
        "    CORPUS_DIR            directory containing the corpus's *.krn files (searched\n"
        "                          recursively), or a path to a single .krn file\n"
        "\n"
        "Options:\n"
        "    --format table|json   output format (default: table)\n"
        "    --group-by-chorale    with --format json, group results into an object keyed\n"
        "                          by choraleId instead of a flat array\n"
        "    --no-analysis         read the analysis spines (**deg, **mint, ...) straight from\n"
        "                          the corpus instead of deriving them per run -- for a corpus\n"
        "                          built by chorale-generate --analysis\n"
        "    --help, -h            show this help\n";
}

void printTable(const Results& results) {
    // Only a combined array-of-queries run tags results with queryId; a single-query run's
    // table output stays exactly as it always has, no extra column.
    bool hasQueryId = std::any_of(results.begin(), results.end(), [](const Result& r) { return r.queryId.has_value(); });

    std::cout << "chorale\tfeature\tvoice\tstart_line\tend_line\tstart_position\tend_position";
    if (hasQueryId) std::cout << "\tquery_id";
    std::cout << "\n";
    for (const auto& r : results) {
        std::cout << r.choraleId << '\t' << r.feature << '\t' << r.voiceLabel << '\t' << r.startLineNumber << '\t'
                   << r.endLineNumber << '\t' << r.startPosition << '\t' << r.endPosition;
        if (hasQueryId) std::cout << '\t' << r.queryId.value_or("");
        std::cout << '\n';
    }
    std::cerr << results.size() << " match(es)\n";
}

void printJson(const Results& results, bool groupByChorale) {
    nlohmann::json j = groupByChorale ? choralesearch::resultsGroupedByChoraleToJson(results)
                                       : choralesearch::resultsToJson(results);
    std::cout << j.dump(1, '\t') << '\n';
    std::cerr << results.size() << " match(es)\n";
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 2 || std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h") {
        printUsage(argv[0]);
        return (argc < 2) ? kExitInvalidArgumentError : 0;
    }

    std::string corpusDir = argv[1];
    std::string queryFile;
    std::string queryString;
    bool haveQueryString = false;
    std::string format = "table";
    bool groupByChorale = false;
    bool applyAnalysis = true;

    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        auto next = [&](const char* flag) -> std::string {
            if (i + 1 >= argc) throw std::invalid_argument(std::string(flag) + " needs a value");
            return argv[++i];
        };
        try {
            if (arg == "--query-file") { queryFile = next("--query-file"); }
            else if (arg == "--query") { queryString = next("--query"); haveQueryString = true; }
            else if (arg == "--format") { format = next("--format"); }
            else if (arg == "--group-by-chorale") { groupByChorale = true; }
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

    if (queryFile.empty() && !haveQueryString) {
        std::cerr << "Error: need --query or --query-file\n\n";
        printUsage(argv[0]);
        return kExitInvalidArgumentError;
    }
    if (!queryFile.empty() && haveQueryString) {
        std::cerr << "Error: --query and --query-file are mutually exclusive\n\n";
        printUsage(argv[0]);
        return kExitInvalidArgumentError;
    }
    if (format != "table" && format != "json") {
        std::cerr << "Error: --format must be 'table' or 'json'\n\n";
        printUsage(argv[0]);
        return kExitInvalidArgumentError;
    }
    if (groupByChorale && format != "json") {
        std::cerr << "Error: --group-by-chorale requires --format json\n\n";
        printUsage(argv[0]);
        return kExitInvalidArgumentError;
    }

    try {
        nlohmann::json j;
        if (haveQueryString) {
            j = nlohmann::json::parse(queryString, nullptr, true, /*ignore_comments=*/true);
        } else {
            std::ifstream f(queryFile);
            if (!f.is_open()) throw std::runtime_error("Could not open query file: " + queryFile);
            j = nlohmann::json::parse(f, nullptr, true, /*ignore_comments=*/true);
        }
        CorpusSearch search(corpusDir, applyAnalysis);
        Results results;
        if (j.is_array()) {
            std::vector<Query> queries = choralesearch::queryArrayFromJson(j);
            results = search.run(queries);
        } else {
            Query query = choralesearch::queryFromJson(j);
            results = search.run(query);
        }

        if (format == "json") {
            printJson(results, groupByChorale);
        } else {
            printTable(results);
        }
    } catch (const nlohmann::json::exception& e) {
        std::cerr << "Error: invalid JSON: " << e.what() << "\n";
        return kExitInvalidArgumentError;
    } catch (const std::invalid_argument& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return kExitValidationError;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return kExitError;
    }

    return 0;
}

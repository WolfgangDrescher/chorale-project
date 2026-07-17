#include <fstream>
#include <iostream>

#include <nlohmann/json.hpp>

#include "CorpusSearch.hpp"
#include "JsonIO.hpp"
#include "Query.hpp"

using choralesearch::CorpusSearch;
using choralesearch::Query;
using choralesearch::Result;

namespace {

void printUsage(const char* argv0) {
    std::cerr <<
        "Usage: " << argv0 << " CORPUS_DIR --query JSON\n"
        "   or: " << argv0 << " CORPUS_DIR --query-file FILE.json\n";
}

void printTable(const std::vector<Result>& results) {
    std::cout << "chorale\tfeature\tvoice\tstart_line\tend_line\tstart_position\tend_position\n";
    for (const auto& r : results) {
        std::cout << r.choraleId << '\t' << r.feature << '\t' << r.voiceLabel << '\t' << r.startLineNumber << '\t'
                   << r.endLineNumber << '\t' << r.startPosition << '\t' << r.endPosition << '\n';
    }
    std::cerr << results.size() << " match(es)\n";
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 2 || std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h") {
        printUsage(argv[0]);
        return (argc < 2) ? 1 : 0;
    }

    std::string corpusDir = argv[1];
    std::string queryFile;
    std::string queryString;
    bool haveQueryString = false;

    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        auto next = [&](const char* flag) -> std::string {
            if (i + 1 >= argc) throw std::invalid_argument(std::string(flag) + " needs a value");
            return argv[++i];
        };
        try {
            if (arg == "--query-file") { queryFile = next("--query-file"); }
            else if (arg == "--query") { queryString = next("--query"); haveQueryString = true; }
            else if (arg == "--help" || arg == "-h") { printUsage(argv[0]); return 0; }
            else { std::cerr << "Unknown option: " << arg << "\n"; printUsage(argv[0]); return 1; }
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << "\n";
            return 1;
        }
    }

    if (queryFile.empty() && !haveQueryString) {
        std::cerr << "Error: need --query or --query-file\n\n";
        printUsage(argv[0]);
        return 1;
    }
    if (!queryFile.empty() && haveQueryString) {
        std::cerr << "Error: --query and --query-file are mutually exclusive\n\n";
        printUsage(argv[0]);
        return 1;
    }

    try {
        nlohmann::json j;
        if (haveQueryString) {
            j = nlohmann::json::parse(queryString);
        } else {
            std::ifstream f(queryFile);
            if (!f.is_open()) throw std::runtime_error("Could not open query file: " + queryFile);
            f >> j;
        }
        Query query = choralesearch::queryFromJson(j);

        CorpusSearch search(corpusDir);
        printTable(search.run(query));
    } catch (const nlohmann::json::exception& e) {
        std::cerr << "Error: invalid JSON: " << e.what() << "\n";
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}

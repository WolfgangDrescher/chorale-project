#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "CorpusSearch.hpp"
#include "GroupMatchQueries.hpp"
#include "Grouping.hpp"
#include "GroupingJson.hpp"
#include "ProgressReport.hpp"

using choralesearch::GroupQuery;
using choralesearch::CorpusSearch;
using choralesearch::Group;

namespace {

constexpr int kExitError = 1;
constexpr int kExitInvalidArgumentError = 2;
constexpr int kExitValidationError = 3;
constexpr int kExitVerificationError = 4;

void printUsage(const char* argv0) {
    std::cerr <<
        "Usage: " << argv0 << " CORPUS_DIR (--query JSON | --query-file FILE.json) [OPTIONS]\n"
        "\n"
        "Searches the corpus with the query and groups its matches by what\n"
        "the score has at them, level by level, the most frequent group first, each with the\n"
        "query that finds exactly its matches.\n"
        "\n"
        "Arguments:\n"
        "    CORPUS_DIR            directory containing the corpus's *.krn files (searched\n"
        "                          recursively), or a path to a single .krn file\n"
        "\n"
        "Options:\n"
        "    --query JSON          the group query; its keys are documented in\n"
        "                          chorale-webapp/analyses/README.md\n"
        "    --query-file FILE.json\n"
        "                          the group query from a file, or '-' for stdin\n"
        "    --verify              run the query of every group and fail when it finds another\n"
        "                          number of matches than the group counts\n"
        "    --no-groupQuery         read the groupQuery spines (**deg, **mint, ...) straight from\n"
        "                          the corpus instead of deriving them per run -- for a corpus\n"
        "                          built by chorale-generate --groupQuery\n"
        "    --progress            write the progress of the search to stderr, one JSON object\n"
        "                          per line, one for every chorale file searched\n"
        "    --help, -h            show this help\n"
        "\n"
        "Exit codes:\n"
        "    0  success\n"
        "    1  error\n"
        "    2  invalid argument\n"
        "    3  invalid group query\n"
        "    4  a group whose query finds other matches (--verify)\n";
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
    bool verify = false;
    bool applyAnalysis = true;
    bool progress = false;

    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        auto next = [&](const char* flag) -> std::string {
            if (i + 1 >= argc) throw std::invalid_argument(std::string(flag) + " needs a value");
            return argv[++i];
        };
        try {
            if (arg == "--query-file") { queryFile = next("--query-file"); }
            else if (arg == "--query") { queryString = next("--query"); haveQueryString = true; }
            else if (arg == "--verify") { verify = true; }
            else if (arg == "--no-groupQuery") { applyAnalysis = false; }
            else if (arg == "--progress") { progress = true; }
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

    try {
        nlohmann::json j;
        if (haveQueryString) {
            j = nlohmann::json::parse(queryString, nullptr, true, /*ignore_comments=*/true);
        } else if (queryFile == "-") {
            j = nlohmann::json::parse(std::cin, nullptr, true, /*ignore_comments=*/true);
        } else {
            std::ifstream f(queryFile);
            if (!f.is_open()) throw std::runtime_error("Could not open query file: " + queryFile);
            j = nlohmann::json::parse(f, nullptr, true, /*ignore_comments=*/true);
        }
        const GroupQuery groupQuery = choralesearch::groupQueryFromJson(j);

        CorpusSearch search(corpusDir, applyAnalysis);
        if (progress) search.setProgressCallback(choralesearch::progressToStderr());
        // The group query, then each of its variants.
        std::vector<std::pair<std::string, GroupQuery>> runs = {{"", groupQuery}};
        for (const auto& variant : groupQuery.variants) runs.emplace_back(variant.name, choralesearch::applyVariant(groupQuery, variant));

        nlohmann::json output;
        for (const auto& [variant, run] : runs) {
            const Group root = choralesearch::groupMatchesInCorpus(search, run);
            if (verify) {
                const std::vector<std::string> mismatches = choralesearch::verifyGroupMatchQueries(search, root);
                for (const std::string& mismatch : mismatches) {
                    std::cerr << "Error: " << (variant.empty() ? "" : variant + ": ") << mismatch << "\n";
                }
                if (!mismatches.empty()) return kExitVerificationError;
            }
            if (variant.empty()) output = choralesearch::groupToJson(root);
            else output["variants"][variant] = choralesearch::groupToJson(root);
        }
        output["groupBy"] = j["groupBy"];
        std::cout << output.dump(1, '\t') << "\n";
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

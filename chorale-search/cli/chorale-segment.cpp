#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "HumdrumChorale.hpp"
#include "HumdrumUtils.hpp"
#include "Query.hpp"
#include "Segmentation.hpp"

using choralesearch::HumdrumChorale;
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

void printSegmentsAsJson(const HumdrumChorale& chorale, const std::vector<Segment>& segments) {
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
    SegmentationOptions segmentationOptions;

    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        auto next = [&](const char* flag) -> std::string {
            if (i + 1 >= argc) throw std::invalid_argument(std::string(flag) + " needs a value");
            return argv[++i];
        };
        try {
            if (arg == "--length") { segmentationOptions.length = parseLength(next("--length")); }
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

    try {
        HumdrumChorale chorale(inputPath);
        printSegmentsAsJson(chorale, choralesearch::segmentScore(chorale, segmentationOptions, SegmentQueryOptions{}));
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return kExitError;
    }

    return 0;
}

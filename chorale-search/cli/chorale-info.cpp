#include <algorithm>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "HumdrumChorale.hpp"

namespace fs = std::filesystem;

using choralesearch::ChoraleInfo;
using choralesearch::HumdrumChorale;

namespace {

constexpr int kExitError = 1;
constexpr int kExitInvalidArgumentError = 2;

void printUsage(const char* argv0) {
    std::cerr <<
        "Usage: " << argv0 << " INPUT...\n"
        "\n"
        "Prints what each score states about itself as a whole -- the titles, the meters, the key it\n"
        "starts in, the number of measures and phrases, and its length in quarter notes -- as a JSON\n"
        "array with one object per chorale, in the order of their ids.\n"
        "\n"
        "Arguments:\n"
        "    INPUT...              Humdrum **kern files, or directories that hold some (searched\n"
        "                          recursively)\n"
        "\n"
        "Options:\n"
        "    --help, -h            show this help\n";
}

nlohmann::json infoToJson(const ChoraleInfo& info) {
    nlohmann::json j = {
        {"id", info.id},
        {"titles", info.titles},
        {"meters", info.meters},
        {"measureCount", info.measureCount},
        {"phraseCount", info.phraseCount},
        {"duration", info.duration},
    };
    if (info.key) j["key"] = *info.key;
    if (info.mode) j["mode"] = *info.mode;
    return j;
}

std::vector<fs::path> findScores(const std::vector<std::string>& inputs) {
    std::vector<fs::path> files;
    for (const std::string& input : inputs) {
        const fs::path path(input);
        if (!fs::exists(path)) throw std::invalid_argument("no such file or directory: " + input);
        if (fs::is_directory(path)) {
            for (const auto& entry : fs::recursive_directory_iterator(path)) {
                if (entry.is_regular_file() && entry.path().extension() == ".krn") files.push_back(entry.path());
            }
        } else {
            files.push_back(path);
        }
    }
    std::sort(files.begin(), files.end());
    return files;
}

} // namespace

int main(int argc, char** argv) {
    std::vector<std::string> inputs;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") { printUsage(argv[0]); return 0; }
        if (!arg.empty() && arg[0] == '-') {
            std::cerr << "Unknown option: " << arg << "\n";
            printUsage(argv[0]);
            return kExitInvalidArgumentError;
        }
        inputs.push_back(arg);
    }
    if (inputs.empty()) {
        printUsage(argv[0]);
        return kExitInvalidArgumentError;
    }

    try {
        nlohmann::json infos = nlohmann::json::array();
        // The info needs no analysis spines, so the scores are read as they are.
        for (const fs::path& file : findScores(inputs)) {
            infos.push_back(infoToJson(HumdrumChorale(file.string(), false).collectInfo()));
        }
        std::cout << infos.dump(1, '\t') << '\n';
    } catch (const std::invalid_argument& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return kExitInvalidArgumentError;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return kExitError;
    }
    return 0;
}

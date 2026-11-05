#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "HumdrumChorale.hpp"
#include "LinePositions.hpp"

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
        "starts in and the keys it changes to, the number of measures and phrases, and its length in\n"
        "quarter notes -- as a JSON array with one object per chorale, in the order of their ids.\n"
        "\n"
        "Arguments:\n"
        "    INPUT...              Humdrum **kern files, or directories that hold some (searched\n"
        "                          recursively)\n"
        "\n"
        "Options:\n"
        "    --positions-dir DIR   write DIR/<id>.json per score, with the position (\"measure/beat\")\n"
        "                          of every data line and of every key change, and print nothing\n"
        "    --help, -h            show this help\n"
        "\n"
        "Exit codes:\n"
        "    0  success\n"
        "    1  error\n"
        "    2  invalid argument\n";
}

nlohmann::json keyToJson(const choralesearch::KeyContext& key) {
    return {
        {"key", key.key},
        {"mode", key.mode},
        {"keyDeg", key.keyDeg},
        {"signature", choralesearch::keySignature(key.key, key.mode)},
    };
}

nlohmann::json keyChangeToJson(const choralesearch::KeyChange& change) {
    if (change.keyContext.key == "?") return {{"line", change.line}, {"key", "?"}};
    return {
        {"line", change.line},
        {"key", change.keyContext.key},
        {"mode", change.keyContext.mode},
        {"keyDeg", change.keyContext.keyDeg},
        {"signature", change.signature},
    };
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
    if (info.cantusFirmus) j["cantusFirmus"] = keyToJson(*info.cantusFirmus);
    j["modulations"] = nlohmann::json::array();
    for (const choralesearch::KeyChange& change : info.keyChanges) j["modulations"].push_back(keyChangeToJson(change));
    return j;
}

nlohmann::json positionsToJson(const ChoraleInfo& info, hum::HumdrumFile& infile) {
    nlohmann::json j = {{"positions", nlohmann::json::object()}, {"modulations", nlohmann::json::array()}};
    std::map<int, std::string> positionOfLine;
    for (const choralesearch::LinePosition& position : choralesearch::linePositions(infile)) {
        positionOfLine[position.line] = position.position;
        j["positions"][std::to_string(position.line)] = position.position;
    }
    for (const choralesearch::KeyChange& change : info.keyChanges) {
        nlohmann::json modulation = keyChangeToJson(change);
        modulation["position"] = positionOfLine[change.line];
        j["modulations"].push_back(modulation);
    }
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

// The info needs no computed spines, so the scores are read as they are.
void printInfos(const std::vector<fs::path>& files) {
    nlohmann::json infos = nlohmann::json::array();
    for (const fs::path& file : files) infos.push_back(infoToJson(HumdrumChorale(file.string(), false).collectInfo()));
    std::cout << infos.dump(1, '\t') << '\n';
}

void writePositions(const std::vector<fs::path>& files, const fs::path& dir) {
    fs::create_directories(dir);
    for (const fs::path& file : files) {
        hum::HumdrumFile infile;
        if (!infile.read(file.string())) throw std::runtime_error("could not read " + file.string());
        const ChoraleInfo info = HumdrumChorale(file.string(), false).collectInfo();
        std::ofstream(dir / (info.id + ".json")) << positionsToJson(info, infile).dump(1, '\t') << '\n';
    }
}

} // namespace

int main(int argc, char** argv) {
    std::vector<std::string> inputs;
    std::optional<fs::path> positionsDir;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") { printUsage(argv[0]); return 0; }
        if (arg == "--positions-dir" && i + 1 < argc) { positionsDir = argv[++i]; continue; }
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
        const std::vector<fs::path> files = findScores(inputs);
        if (positionsDir) writePositions(files, *positionsDir);
        else printInfos(files);
    } catch (const std::invalid_argument& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return kExitInvalidArgumentError;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return kExitError;
    }
    return 0;
}

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "Modulations.hpp"
#include "SpineAnalysis.hpp"
#include "humlib.h"

using choralesearch::Modulation;

namespace fs = std::filesystem;

namespace {

void processChorale(const std::string& choraleId, const std::vector<Modulation>& mods, const fs::path& sourceDir,
                     const fs::path& outDir, bool applyAnalysis) {
    fs::path sourcePath = sourceDir / (choraleId + ".krn");
    hum::HumdrumFile infile;
    if (!infile.read(sourcePath.string())) {
        throw std::runtime_error("Could not parse Humdrum file: " + sourcePath.string());
    }

    choralesearch::applyModulations(infile, choraleId, mods);

    // After the modulations, so the analysis sees the modulated score: **deg is spelled
    // relative to the key in effect, so a key designation added above changes the degrees.
    if (applyAnalysis) choralesearch::applySpineAnalysisTools(infile);

    // Remove instrument tandem interpretations (*ICvox, *Ibass, *I" names, *I' abbreviations)
    // Backwards, since deleteLine shifts everything after it.
    for (int i = infile.getLineCount() - 1; i >= 0; --i) {
        if (infile[i].compare(0, 2, "*I") == 0) infile.deleteLine(i);
    }

    fs::path outPath = outDir / (choraleId + ".krn");
    std::ofstream out(outPath);
    if (!out.is_open()) {
        throw std::runtime_error("Could not write file: " + outPath.string());
    }

    out << infile;
    std::cout << "generated " << choraleId << "\n";
}

// Every chorale the source directory offers, by id (the filename stem), sorted.
std::vector<std::string> sourceChoraleIds(const fs::path& sourceDir) {
    if (!fs::is_directory(sourceDir)) {
        throw std::runtime_error("Source directory does not exist: " + sourceDir.string());
    }
    std::vector<std::string> ids;
    for (const auto& entry : fs::directory_iterator(sourceDir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".krn") {
            ids.push_back(entry.path().stem().string());
        }
    }
    std::sort(ids.begin(), ids.end());
    return ids;
}

void printUsage(const char* argv0) {
    std::cerr <<
        "Usage: " << argv0 << " SOURCE_KERN_DIR OUT_KERN_DIR [OPTIONS] [CHORALE_ID...]\n"
        "\n"
        "Generates the chorale scores a search runs against: reads the unmodified sources and\n"
        "writes them back out with the requested additions baked in.\n"
        "\n"
        "Arguments:\n"
        "    SOURCE_KERN_DIR       directory holding the unmodified *.krn sources\n"
        "    OUT_KERN_DIR          where the generated scores are written (created if needed)\n"
        "    CHORALE_ID...         generate only these chorales, e.g. chor001 (default: all of\n"
        "                          SOURCE_KERN_DIR)\n"
        "\n"
        "Options:\n"
        "    --modulations FILE    add the key designations annotated in FILE (a JSON object\n"
        "                          keyed by chorale id); chorales it doesn't mention are\n"
        "                          written out unchanged\n"
        "    --analysis            bake in the analysis spines (**deg, **mint, **metweight,\n"
        "                          **fb, **hint-xy) that chorale-search otherwise derives on\n"
        "                          every run -- pair with its --no-analysis\n"
        "    --help, -h            show this help\n";
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 2 || std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h") {
        printUsage(argv[0]);
        return (argc < 2) ? 1 : 0;
    }
    if (argc < 3) {
        printUsage(argv[0]);
        return 1;
    }

    fs::path sourceDir = argv[1];
    fs::path outDir = argv[2];
    fs::path modulationsPath;
    bool applyAnalysis = false;
    std::vector<std::string> requestedIds;

    for (int i = 3; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--modulations") {
            if (i + 1 >= argc) {
                std::cerr << "Error: --modulations needs a value\n";
                return 1;
            }
            modulationsPath = argv[++i];
        } else if (arg == "--analysis") {
            applyAnalysis = true;
        } else if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            return 0;
        } else if (arg.rfind("--", 0) == 0) {
            std::cerr << "Unknown option: " << arg << "\n";
            printUsage(argv[0]);
            return 1;
        } else {
            requestedIds.push_back(arg);
        }
    }

    try {
        std::map<std::string, std::vector<Modulation>> modulations;
        if (!modulationsPath.empty()) modulations = choralesearch::readModulations(modulationsPath);

        std::vector<std::string> availableIds = sourceChoraleIds(sourceDir);
        for (const std::string& choraleId : requestedIds) {
            if (std::find(availableIds.begin(), availableIds.end(), choraleId) == availableIds.end()) {
                throw std::runtime_error("No such chorale in " + sourceDir.string() + ": " + choraleId);
            }
        }
        // An annotated chorale the sources don't have is a typo in the annotations rather than
        // a reason to stop -- say so and generate the rest.
        for (const auto& [choraleId, mods] : modulations) {
            if (std::find(availableIds.begin(), availableIds.end(), choraleId) == availableIds.end()) {
                std::cerr << choraleId << ": annotated but not in " << sourceDir << ", skipping\n";
            }
        }

        fs::create_directories(outDir);

        for (const std::string& choraleId : availableIds) {
            if (!requestedIds.empty() &&
                std::find(requestedIds.begin(), requestedIds.end(), choraleId) == requestedIds.end()) {
                continue;
            }
            auto it = modulations.find(choraleId);
            processChorale(choraleId, it == modulations.end() ? std::vector<Modulation>{} : it->second, sourceDir,
                            outDir, applyAnalysis);
        }
    } catch (const nlohmann::json::exception& e) {
        std::cerr << "Error: invalid JSON in modulations file: " << e.what() << "\n";
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}

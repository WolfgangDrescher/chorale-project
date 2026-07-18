#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "humlib.h"

namespace fs = std::filesystem;

namespace {

struct Modulation {
    std::string pos; // "measure/beat", e.g. "10/3.5"
    int measure = 0;
    hum::HumNum beat;
    std::string key;
};

hum::HumNum parseDecimal(const std::string& s) {
    auto dot = s.find('.');
    if (dot == std::string::npos) {
        return hum::HumNum(std::stoi(s));
    }
    std::string intPart = s.substr(0, dot);
    std::string fracPart = s.substr(dot + 1);
    int denom = 1;
    for (std::size_t i = 0; i < fracPart.size(); ++i) denom *= 10;
    int numer = (intPart.empty() ? 0 : std::stoi(intPart)) * denom + std::stoi(fracPart);
    return hum::HumNum(numer, denom);
}

Modulation parseModulation(const std::string& pos, const std::string& key) {
    auto slash = pos.find('/');
    if (slash == std::string::npos) {
        throw std::runtime_error("Invalid modulation position (expected measure/beat): " + pos);
    }
    Modulation mod;
    mod.pos = pos;
    mod.measure = std::stoi(pos.substr(0, slash));
    mod.beat = parseDecimal(pos.substr(slash + 1));
    mod.key = key;
    return mod;
}

bool isInitialModulation(const Modulation& mod) {
    return mod.measure == 0 || (mod.measure == 1 && mod.beat == hum::HumNum(1));
}

std::set<int> kernTracks(hum::HumdrumFile& infile) {
    std::vector<hum::HTp> starts;
    infile.getKernSpineStartList(starts);
    std::set<int> tracks;
    for (hum::HTp start : starts) tracks.insert(start->getTrack());
    return tracks;
}

void setKeyOnLine(hum::HLp line, const std::set<int>& tracks, const std::string& key) {
    for (int i = 0; i < line->getFieldCount(); ++i) {
        hum::HTp token = line->token(i);
        if (tracks.count(token->getTrack())) {
            token->setText("*" + key + ":");
        }
    }
    // HumdrumLine caches its full text (it is itself a std::string, rebuilt
    // from tokens on creation); regenerate it now that tokens changed.
    line->createLineFromTokens();
}

hum::HumNum findTimestamp(hum::HumdrumFile& infile, int measure, hum::HumNum beat) {
    std::vector<int> measureNumbers = infile.getMeasureNumbers();
    for (int i = 0; i < infile.getLineCount(); ++i) {
        if (!infile[i].isData()) continue;
        if (measureNumbers[i] == measure && infile[i].getBeat() == beat) {
            return infile[i].getDurationFromStart();
        }
    }
    return hum::HumNum(-1);
}

// Applies the piece's starting key: replaces the key designation that
// (normally) already follows the key signature (*k[...]), or inserts one if
// none is present yet.
void applyInitialModulation(hum::HumdrumFile& infile, const std::set<int>& tracks, const std::string& choraleId, const Modulation& mod) {
    int keySigIndex = -1;
    for (int i = 0; i < infile.getLineCount(); ++i) {
        if (infile[i].isKeySignature()) {
            keySigIndex = i;
            break;
        }
    }
    if (keySigIndex < 0) {
        std::cerr << choraleId << ": no key signature (*k[...]) found, skipping initial modulation\n";
        return;
    }

    int nextIndex = keySigIndex + 1;
    if (nextIndex < infile.getLineCount() && infile[nextIndex].isKeyDesignation()) {
        setKeyOnLine(&infile[nextIndex], tracks, mod.key);
    } else {
        hum::HLp newLine = infile.insertNullInterpretationLineAboveIndex(nextIndex);
        setKeyOnLine(newLine, tracks, mod.key);
    }
}

void applyModulation(hum::HumdrumFile& infile, const std::set<int>& tracks, const std::string& choraleId, const Modulation& mod) {
    hum::HumNum timestamp = findTimestamp(infile, mod.measure, mod.beat);
    if (timestamp.isNegative()) {
        std::cerr << choraleId << ": no note found at " << mod.pos << ", skipping modulation to " << mod.key
                  << "\n";
        return;
    }
    // insertNullInterpretationLineAbove() would walk back past a barline that
    // shares the same timestamp (e.g. a modulation on beat 1), landing the new
    // key *before* the barline instead of at the start of the new measure.
    // insertNullInterpretationLine() only walks back past local comments, so
    // it lands right above the data line but below any preceding barline.
    hum::HLp newLine = infile.insertNullInterpretationLine(timestamp);
    setKeyOnLine(newLine, tracks, mod.key);
}

void processChorale(const std::string& choraleId, const std::vector<Modulation>& mods, const fs::path& sourceDir, const fs::path& outDir) {
    fs::path sourcePath = sourceDir / (choraleId + ".krn");
    hum::HumdrumFile infile;
    if (!infile.read(sourcePath.string())) {
        throw std::runtime_error("Could not parse Humdrum file: " + sourcePath.string());
    }

    std::set<int> tracks = kernTracks(infile);

    const Modulation* initial = nullptr;
    for (const auto& mod : mods) {
        if (isInitialModulation(mod)) {
            initial = &mod;
            break;
        }
    }
    if (initial) applyInitialModulation(infile, tracks, choraleId, *initial);

    for (const auto& mod : mods) {
        if (initial && mod.pos == initial->pos) continue;
        applyModulation(infile, tracks, choraleId, mod);
    }

    fs::path outPath = outDir / (choraleId + ".krn");
    std::ofstream out(outPath);
    if (!out.is_open()) {
        throw std::runtime_error("Could not write file: " + outPath.string());
    }
    out << infile;
    std::cout << "added modulations for " << choraleId << "\n";
}

void printUsage(const char* argv0) {
    std::cerr << "Usage: " << argv0 << " ANNOTATIONS_JSON SOURCE_KERN_DIR OUT_KERN_DIR\n";
}

} // namespace

int main(int argc, char** argv) {
    if (argc != 4) {
        printUsage(argv[0]);
        return 1;
    }

    fs::path annotationsPath = argv[1];
    fs::path sourceDir = argv[2];
    fs::path outDir = argv[3];

    try {
        std::ifstream f(annotationsPath);
        if (!f.is_open()) {
            throw std::runtime_error("Could not open annotations file: " + annotationsPath.string());
        }
        nlohmann::json root = nlohmann::json::parse(f, nullptr, true, /*ignore_comments=*/true);

        fs::create_directories(outDir);

        for (const auto& [choraleId, modsJson] : root.items()) {
            std::vector<Modulation> mods;
            for (const auto& entry : modsJson) {
                mods.push_back(parseModulation(entry.at(0).get<std::string>(), entry.at(1).get<std::string>()));
            }
            processChorale(choraleId, mods, sourceDir, outDir);
        }
    } catch (const nlohmann::json::exception& e) {
        std::cerr << "Error: invalid JSON in annotations file: " << e.what() << "\n";
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}

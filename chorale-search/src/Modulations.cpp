#include "Modulations.hpp"

#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <stdexcept>

#include <nlohmann/json.hpp>

namespace choralesearch {

namespace {

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
        // An unnumbered barline (e.g. "=:|!") resets the beat count without advancing the
        // measure number, so the anacrusis after it is labeled (old measure, beat 1) instead
        // of (old measure, last beat) -- only the barline itself still carries that combination.
        if (!infile[i].isData() && !infile[i].isBarline()) continue;
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

} // namespace

std::map<std::string, std::vector<Modulation>> readModulations(const std::filesystem::path& path) {
    std::ifstream f(path);
    if (!f.is_open()) {
        throw std::runtime_error("Could not open modulations file: " + path.string());
    }
    nlohmann::json root = nlohmann::json::parse(f, nullptr, true, /*ignore_comments=*/true);

    std::map<std::string, std::vector<Modulation>> byChorale;
    for (const auto& [choraleId, modsJson] : root.items()) {
        std::vector<Modulation>& mods = byChorale[choraleId];
        for (const auto& entry : modsJson) {
            mods.push_back(parseModulation(entry.at(0).get<std::string>(), entry.at(1).get<std::string>()));
        }
    }
    return byChorale;
}

void applyModulations(hum::HumdrumFile& infile, const std::string& choraleId, const std::vector<Modulation>& mods) {
    if (mods.empty()) return;

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

    // See the header: the inserted lines are text-only until this round trip, which is what
    // lets the caller run the analysis tools over the result.
    std::stringstream modulated;
    modulated << infile;
    infile.readString(modulated.str());
}

} // namespace choralesearch

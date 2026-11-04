#include "Modulations.hpp"

#include "LinePositions.hpp"

#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <stdexcept>

#include <nlohmann/json.hpp>

namespace choralesearch {

namespace {

// A beat as the annotations write it: "3", or "3+1/2" with the part of a beat as a fraction.
hum::HumNum parseBeat(const std::string& text) {
    const std::size_t plus = text.find('+');
    return hum::HumNum(text) + (plus == std::string::npos ? hum::HumNum(0) : hum::HumNum(text.substr(plus + 1)));
}

Modulation parseModulation(const std::string& pos, const std::string& key) {
    auto slash = pos.find('/');
    if (slash == std::string::npos) {
        throw std::runtime_error("Invalid modulation position (expected measure/beat): " + pos);
    }
    Modulation mod;
    mod.pos = pos;
    mod.measure = std::stoi(pos.substr(0, slash));
    mod.beat = parseBeat(pos.substr(slash + 1));
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
            // A key with a mode ("g:dor") already has its colon.
            token->setText(key.find(':') == std::string::npos ? "*" + key + ":" : "*" + key);
        }
    }
    // HumdrumLine caches its full text (it is itself a std::string, rebuilt
    // from tokens on creation); regenerate it now that tokens changed.
    line->createLineFromTokens();
}

// Whether a key designation line labels a mode (*e:phr, *g:dor), not just major or minor by case.
bool hasChurchMode(hum::HumdrumLine& line) {
    for (int i = 0; i < line.getFieldCount(); ++i) {
        hum::HTp token = line.token(i);
        if (!token->isKeyDesignation()) continue;
        const std::string label = token->substr(token->find(':') + 1);
        return !label.empty() && label != "ion" && label != "aeo";
    }
    return false;
}

// The time of the first data line at the position, see LinePositions.hpp for how it is counted.
hum::HumNum findTimestamp(hum::HumdrumFile& infile, int measure, hum::HumNum beat) {
    for (const LinePosition& position : linePositions(infile)) {
        if (position.measure == measure && position.beat == beat) {
            return infile[position.line - 1].getDurationFromStart();
        }
    }
    return hum::HumNum(-1);
}

// Applies the piece's starting key below the key signature (*k[...]). A key designation of the source with a church
// mode (*e:phr) is kept as the mode of the cantus firmus, any other is replaced.
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
        if (hasChurchMode(infile[nextIndex])) {
            hum::HLp newLine = infile.insertNullInterpretationLineAboveIndex(nextIndex + 1);
            setKeyOnLine(newLine, tracks, mod.key);
        } else {
            setKeyOnLine(&infile[nextIndex], tracks, mod.key);
        }
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

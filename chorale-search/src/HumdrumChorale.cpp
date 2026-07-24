#include "HumdrumChorale.hpp"

#include <filesystem>
#include <regex>
#include <stdexcept>

namespace fs = std::filesystem;

namespace choralesearch {

HumdrumChorale::HumdrumChorale(const std::string& path) : m_path(path) {
    m_id = fs::path(path).stem().string();

    if (!m_infile.read(path)) {
        throw std::runtime_error("Could not parse Humdrum file: " + path);
    }

    // Apply analysis spines such as **deg and **mint so every chorale has them
    // available before search runs
    applySpineAnalysisTools(m_infile);

    int maxTrack = m_infile.getMaxTrack();
    for (int track = 1; track <= maxTrack; ++track) {
        hum::HTp start = m_infile.getTrackStart(track);
        if (!start || !start->isExclusiveInterpretation()) continue;
        std::string type = start->getDataType();
        if (type.rfind("**", 0) == 0) type = type.substr(2);
        m_spinesByFeature[type].push_back(start);
    }
}

std::vector<std::string> HumdrumChorale::availableFeatures() const {
    std::vector<std::string> names;
    names.reserve(m_spinesByFeature.size());
    for (const auto& [name, spines] : m_spinesByFeature) names.push_back(name);
    return names;
}

bool HumdrumChorale::hasFeature(const std::string& name) const {
    return m_spinesByFeature.count(name) > 0;
}

hum::HTp HumdrumChorale::spine(const std::string& feature, std::size_t voice) const {
    auto it = m_spinesByFeature.find(feature);
    if (it == m_spinesByFeature.end()) return nullptr;
    if (voice < 1 || voice > it->second.size()) return nullptr;
    return it->second[voice - 1];
}

hum::HTp findTokenAtLine(hum::HTp spineStart, int targetLineNumber) {
    if (!spineStart) return nullptr;
    hum::HTp t = spineStart->getNextToken();
    while (t) {
        if (t->getLineNumber() > targetLineNumber) return nullptr; // walked past it -- not present
        if (t->getOwner()->isData() && !t->isNull() && t->getLineNumber() == targetLineNumber) return t;
        t = t->getNextToken();
    }
    return nullptr;
}

void applySpineAnalysisTools(hum::HumdrumFile& infile) {
    hum::Tool_deg degTool;
    degTool.run(infile);
    if (degTool.hasHumdrumText()) {
        infile.readString(degTool.getHumdrumText());
    }

    hum::Tool_mint mintTool;
    mintTool.run(infile);
    if (mintTool.hasHumdrumText()) {
        infile.readString(mintTool.getHumdrumText());
    }

    hum::Tool_metweight metweightTool;
    metweightTool.run(infile);
    if (metweightTool.hasHumdrumText()) {
        infile.readString(metweightTool.getHumdrumText());
    }

    hum::Tool_fb fbTool;
    fbTool.process("-c -n --hint");
    fbTool.run(infile);
    if (fbTool.hasHumdrumText()) {
        // Rename **hint to **fb before parsing -- renaming the token after the fact
        // wouldn't update HumdrumFile's separately cached line text.
        static const std::regex hintRe(R"((^|\t)\*\*hint(?=\t|$))", std::regex::multiline);
        infile.readString(std::regex_replace(fbTool.getHumdrumText(), hintRe, "$1**fb"));
    }
}

} // namespace choralesearch

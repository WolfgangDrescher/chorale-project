#include "HumdrumChorale.hpp"

#include <algorithm>
#include <filesystem>
#include <stdexcept>

#include "HumdrumUtils.hpp"
#include "SpineAnalysis.hpp"

namespace fs = std::filesystem;

namespace choralesearch {

HumdrumChorale::HumdrumChorale(const std::string& path, bool applyAnalysis) : m_path(path) {
    m_id = fs::path(path).stem().string();

    if (!m_infile.read(path)) {
        throw std::runtime_error("Could not parse Humdrum file: " + path);
    }

    prepare(applyAnalysis);
}

HumdrumChorale::HumdrumChorale(std::istream& contents, const std::string& id, bool applyAnalysis) : m_id(id) {
    if (!m_infile.read(contents)) {
        throw std::runtime_error("Could not parse Humdrum data: " + id);
    }

    prepare(applyAnalysis);
}

void HumdrumChorale::prepare(bool applyAnalysis) {
    // Apply analysis spines such as **deg and **mint so every chorale has them
    // available before search runs -- unless the corpus was generated with them
    // already baked in (see chorale-generate), in which case redoing the work
    // would cost ~90ms per file for nothing.
    if (applyAnalysis) applySpineAnalysisTools(m_infile);

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

std::size_t HumdrumChorale::voiceCount() const {
    std::size_t count = 0;
    while (spine("kern", count + 1)) ++count;
    return count;
}

std::vector<hum::HumNum> HumdrumChorale::phraseEndings() const {
    std::vector<std::pair<hum::HumNum, hum::HumNum>> fermataNotes; // attacked at .first, over at .second
    for (std::size_t voice = 1; voice <= voiceCount(); ++voice) {
        hum::HTp start = spine("kern", voice);
        if (!start) continue;
        for (hum::HTp t = start->getNextToken(); t; t = t->getNextToken()) {
            if (!t->getOwner()->isData() || t->isNull() || t->isSecondaryTiedNote() || !t->hasFermata()) continue;
            fermataNotes.emplace_back(t->getDurationFromStart(), t->getDurationFromStart() + soundingDuration(t));
        }
    }
    std::sort(fermataNotes.begin(), fermataNotes.end());

    std::vector<hum::HumNum> endings;
    for (const auto& [attacked, over] : fermataNotes) {
        bool sameChordAsPrevious = !endings.empty() && attacked < endings.back();
        if (sameChordAsPrevious) endings.back() = std::max(endings.back(), over);
        else endings.push_back(over);
    }
    return endings;
}

ChoraleInfo HumdrumChorale::collectInfo() const {
    // humlib's accessors are not const, though reading a score changes nothing.
    hum::HumdrumFile& file = const_cast<hum::HumdrumFile&>(m_infile);

    ChoraleInfo info;
    info.id = m_id;

    for (hum::HLp record : file.getReferenceRecords()) {
        if (record->getReferenceKey().rfind("OTL", 0) != 0) continue;
        const std::string title = decodeHtmlEntities(record->getReferenceValue());
        if (!title.empty()) info.titles.push_back(title);
    }

    // Every voice states the meters and the keys, so the first one speaks for the score.
    for (hum::HTp token = spine("kern", 1); token; token = token->getNextToken()) {
        if (token->isTimeSignature()) {
            const std::string meter = token->substr(2);
            if (std::find(info.meters.begin(), info.meters.end(), meter) == info.meters.end()) {
                info.meters.push_back(meter);
            }
        } else if (!info.key && token->isKeyDesignation()) {
            const std::size_t colon = token->find(':');
            info.key = token->substr(1, colon - 1);
            info.mode = modeFromKeyDesignation(*info.key, token->substr(colon + 1));
        }
    }

    // The highest number a barline carries, so a pickup measure, which has none, is not counted.
    const std::vector<int> measureNumbers = file.getMeasureNumbers();
    if (!measureNumbers.empty()) info.measureCount = std::max(0, *std::max_element(measureNumbers.begin(), measureNumbers.end()));

    info.phraseCount = phraseEndings().size();
    return info;
}

hum::HTp HumdrumChorale::tokenAtLine(const std::string& feature, std::size_t voice, int lineNumber) const {
    hum::HTp start = spine(feature, voice);
    if (!start) return nullptr;
    hum::HumdrumFile* file = start->getOwner()->getOwner();
    if (lineNumber < 1 || lineNumber > file->getLineCount()) return nullptr;
    hum::HLp line = file->getLine(lineNumber - 1);
    if (!line->isData()) return nullptr;

    int track = start->getTrack();
    for (int field = 0; field < line->getFieldCount(); ++field) {
        hum::HTp candidate = line->token(field);
        // The leftmost field of the track, which for a split spine is the branch
        // getNextToken() follows.
        if (candidate->getTrack() != track) continue;
        return candidate->isNull() ? nullptr : candidate;
    }
    return nullptr;
}

} // namespace choralesearch

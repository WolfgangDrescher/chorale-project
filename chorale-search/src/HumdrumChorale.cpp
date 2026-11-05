#include "HumdrumChorale.hpp"

#include <algorithm>
#include <filesystem>
#include <stdexcept>

#include "ComputedSpines.hpp"

namespace fs = std::filesystem;

namespace choralesearch {

HumdrumChorale::HumdrumChorale(const std::string& path, bool withComputedSpines) : m_path(path) {
    m_id = fs::path(path).stem().string();

    if (!m_infile.read(path)) {
        throw std::runtime_error("Could not parse Humdrum file: " + path);
    }

    prepare(withComputedSpines);
}

HumdrumChorale::HumdrumChorale(std::istream& contents, const std::string& id, bool withComputedSpines) : m_id(id) {
    if (!m_infile.read(contents)) {
        throw std::runtime_error("Could not parse Humdrum data: " + id);
    }

    prepare(withComputedSpines);
}

void HumdrumChorale::prepare(bool withComputedSpines) {
    if (withComputedSpines) {
        try {
            addComputedSpinesWhereMissing(m_infile);
        } catch (const std::runtime_error& e) {
            throw std::runtime_error(m_id + ": " + e.what());
        }
    }

    int maxTrack = m_infile.getMaxTrack();
    for (int track = 1; track <= maxTrack; ++track) {
        hum::HTp start = m_infile.getTrackStart(track);
        if (!start || !start->isExclusiveInterpretation()) continue;
        std::string type = start->getDataType();
        if (type.rfind("**", 0) == 0) type = type.substr(2);
        m_spinesByFeature[type].push_back(start);
    }

    // All voices share the key designations, so the first one is read.
    // keyDeg is reckoned from the key of the first note, below a church mode of the source.
    std::optional<KeyContext> current;
    std::optional<KeyContext> starting;
    bool beforeFirstNote = true;
    const int kernTrack = spine("kern", 1) ? spine("kern", 1)->getTrack() : -1;
    m_keyContextByLine.assign(m_infile.getLineCount(), std::nullopt);
    for (int i = 0; i < m_infile.getLineCount(); ++i) {
        if (m_infile[i].isInterpretation()) {
            for (int field = 0; field < m_infile[i].getFieldCount(); ++field) {
                hum::HTp token = m_infile[i].token(field);
                if (token->getTrack() != kernTrack) continue;
                if (std::optional<KeyContext> designated = keyContextFromDesignation(*token)) {
                    if (beforeFirstNote) starting = designated;
                    designated->keyDeg = keyScaleDegree(designated->key, starting->key, starting->mode);
                    current = designated;
                }
                break;
            }
        }
        if (m_infile[i].isData()) beforeFirstNote = false;
        m_keyContextByLine[i] = current;
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
            fermataNotes.emplace_back(t->getDurationFromStart(), t->getDurationFromStart() + t->getTiedDuration());
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
    bool beforeFirstNote = true;
    std::optional<KeyChange> pendingKeyChange;
    for (hum::HTp token = spine("kern", 1); token; token = token->getNextToken()) {
        if (token->isData()) {
            beforeFirstNote = false;
            if (pendingKeyChange) {
                pendingKeyChange->line = token->getLineNumber();
                info.keyChanges.push_back(*pendingKeyChange);
                pendingKeyChange.reset();
            }
        }
        if (const std::optional<KeyContext> key = keyContextFromDesignation(*token)) {
            pendingKeyChange = KeyChange{0, *key, keySignature(key->key, key->mode)};
        }
        if (token->isTimeSignature()) {
            const std::string meter = token->substr(2);
            if (std::find(info.meters.begin(), info.meters.end(), meter) == info.meters.end()) {
                info.meters.push_back(meter);
            }
        } else if (beforeFirstNote && token->isKeyDesignation()) {
            const std::size_t colon = token->find(':');
            info.key = token->substr(1, colon - 1);
            info.mode = modeFromKeyDesignation(*info.key, token->substr(colon + 1));
            if (!info.cantusFirmus) info.cantusFirmus = KeyContext{*info.key, *info.mode, ""};
        }
    }

    // The highest number a barline carries, so a pickup measure, which has none, is not counted.
    const std::vector<int> measureNumbers = file.getMeasureNumbers();
    if (!measureNumbers.empty()) info.measureCount = std::max(0, *std::max_element(measureNumbers.begin(), measureNumbers.end()));

    info.duration = file.getScoreDuration().getFloat();

    for (KeyChange& change : info.keyChanges) {
        const KeyContext& first = info.keyChanges.front().keyContext;
        change.keyContext.keyDeg = keyScaleDegree(change.keyContext.key, first.key, first.mode);
    }
    if (info.cantusFirmus && !info.keyChanges.empty()) {
        const KeyContext& first = info.keyChanges.front().keyContext;
        info.cantusFirmus->keyDeg = keyScaleDegree(info.cantusFirmus->key, first.key, first.mode);
    }

    info.phraseCount = phraseEndings().size();
    return info;
}

const KeyContext* HumdrumChorale::keyContextAtLine(int lineNumber) const {
    if (lineNumber < 1 || lineNumber > static_cast<int>(m_keyContextByLine.size())) return nullptr;
    const std::optional<KeyContext>& context = m_keyContextByLine[lineNumber - 1];
    return context ? &*context : nullptr;
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

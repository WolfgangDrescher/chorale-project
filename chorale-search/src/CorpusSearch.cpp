#include "CorpusSearch.hpp"

#include <algorithm>
#include <stdexcept>

#include "AttributeMatcher.hpp"
#include "HumdrumChorale.hpp"
#include "VoiceMap.hpp"

namespace fs = std::filesystem;

namespace choralesearch {

namespace {

std::string getHumNumTwoPart(const hum::HumNum& n) {
    std::ostringstream oss;
    n.printTwoPart(oss);
    return oss.str();
}

} // namespace

CorpusSearch::CorpusSearch(fs::path corpusRoot) : m_corpusRoot(std::move(corpusRoot)) {}

std::vector<fs::path> CorpusSearch::findChoraleFiles() const {
    if (!fs::exists(m_corpusRoot)) {
        throw std::runtime_error("Corpus root does not exist: " + m_corpusRoot.string());
    }
    std::vector<fs::path> files;
    if (fs::is_regular_file(m_corpusRoot)) {
        files.push_back(m_corpusRoot);
    } else {
        for (const auto& entry : fs::recursive_directory_iterator(m_corpusRoot)) {
            if (entry.is_regular_file() && entry.path().extension() == ".krn") {
                files.push_back(entry.path());
            }
        }
    }
    std::sort(files.begin(), files.end());
    return files;
}

Results CorpusSearch::runOne(const HumdrumChorale& chorale, const Query& query) const {
    Results results;
    if (!chorale.hasFeature(query.feature)) return results;

    AttributeMatcher matcher(query.feature, query.pattern);

    for (std::size_t voice : resolveVoices(query.voices)) {
        for (auto& m : matcher.findAll(chorale, voice)) {
            Result r;
            r.choraleId = chorale.id();
            r.feature = query.feature;
            r.voiceLabel = voiceLabel(voice);
            r.voice = m.voice;
            r.startPosition = getHumNumTwoPart(m.startPosition);
            r.endPosition = getHumNumTwoPart(m.endPosition);
            r.startLineNumber = m.startLineNumber;
            r.endLineNumber = m.endLineNumber;
            results.push_back(std::move(r));

            if (query.limit && results.size() >= *query.limit) return results;
        }
    }
    return results;
}

Results CorpusSearch::run(const Query& query) const {
    Results allResults;
    for (const auto& file : findChoraleFiles()) {
        HumdrumChorale chorale(file.string());
        if (!chorale.hasFeature(query.feature)) continue;
        auto results = runOne(chorale, query);
        allResults.insert(allResults.end(), std::make_move_iterator(results.begin()), std::make_move_iterator(results.end()));
        if (query.limit && allResults.size() >= *query.limit) {
            allResults.resize(*query.limit);
            break;
        }
    }
    return allResults;
}

} // namespace choralesearch

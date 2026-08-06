#pragma once

#include <istream>
#include <map>
#include <string>
#include <vector>

#include "humlib.h"

namespace choralesearch {

class HumdrumChorale {
public:
    // `applyAnalysis` false means the file is expected to carry the analysis spines already
    // (a corpus built by chorale-generate --analysis); the spines are then read straight from
    // the file instead of being derived on load.
    explicit HumdrumChorale(const std::string& path, bool applyAnalysis = true);

    // The same, for a score that only exists as text -- an upload, or another tool's output.
    // `id` stands in for what would otherwise be derived from the file name.
    HumdrumChorale(std::istream& contents, const std::string& id, bool applyAnalysis = true);

    const std::string& id() const { return m_id; }
    const std::string& path() const { return m_path; }

    std::vector<std::string> availableFeatures() const;
    bool hasFeature(const std::string& name) const;

    hum::HTp spine(const std::string& feature, std::size_t voice) const;

private:
    // What both constructors do once the file is parsed: derive the analysis spines, then note
    // down which spine answers to which feature name.
    void prepare(bool applyAnalysis);

    std::string m_path;
    std::string m_id;
    hum::HumdrumFile m_infile;
    std::map<std::string, std::vector<hum::HTp>> m_spinesByFeature;
};

hum::HTp findTokenAtLine(hum::HTp spineStart, int targetLineNumber);

} // namespace choralesearch

#pragma once

#include <map>
#include <string>
#include <vector>

#include "humlib.h"

namespace choralesearch {

class HumdrumChorale {
public:
    explicit HumdrumChorale(const std::string& path);

    const std::string& id() const { return m_id; }
    const std::string& path() const { return m_path; }

    std::vector<std::string> availableFeatures() const;
    bool hasFeature(const std::string& name) const;

    hum::HTp spine(const std::string& feature, std::size_t voice) const;

private:
    std::string m_path;
    std::string m_id;
    hum::HumdrumFile m_infile;
    std::map<std::string, std::vector<hum::HTp>> m_spinesByFeature;
};

hum::HTp findTokenAtLine(hum::HTp spineStart, int targetLineNumber);

void applySpineAnalysisTools(hum::HumdrumFile& infile);

} // namespace choralesearch

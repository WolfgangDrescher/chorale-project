#pragma once

#include <istream>
#include <map>
#include <string>
#include <vector>

#include "ChoraleInfo.hpp"
#include "humlib.h"

namespace choralesearch {

class HumdrumChorale {
public:
    // Adds the computed spines where the file lacks them; with `withComputedSpines` false reads it as is.
    explicit HumdrumChorale(const std::string& path, bool withComputedSpines = true);

    // The same, for a score that only exists as text -- an upload, or another tool's output.
    // `id` stands in for what would otherwise be derived from the file name.
    HumdrumChorale(std::istream& contents, const std::string& id, bool withComputedSpines = true);

    const std::string& id() const { return m_id; }
    const std::string& path() const { return m_path; }

    std::vector<std::string> availableFeatures() const;
    bool hasFeature(const std::string& name) const;

    hum::HTp spine(const std::string& feature, std::size_t voice) const;

    // The number of voices: the **kern spines the score has.
    std::size_t voiceCount() const;

    // Where the phrases end, in order: a fermata closes one, and fermata notes sounding at the same time
    // close one phrase together, where the last of them stops.
    std::vector<hum::HumNum> phraseEndings() const;

    // What the score states about itself as a whole, which a ChoraleFilter is held against. It reads the
    // reference records and walks the voices every time it is called, so ask once per chorale.
    ChoraleInfo collectInfo() const;

    // The data token the spine of `feature` and `voice` has on `lineNumber`, or nullptr when the
    // spine is missing, the line is no data line, or the voice holds its note there (a null
    // token). It goes straight to the line, so asking it of every onset of a corpus stays cheap.
    hum::HTp tokenAtLine(const std::string& feature, std::size_t voice, int lineNumber) const;

private:
    // What both constructors do once the file is parsed: add the computed spines, then note
    // down which spine answers to which feature name.
    void prepare(bool withComputedSpines);

    std::string m_path;
    std::string m_id;
    hum::HumdrumFile m_infile;
    std::map<std::string, std::vector<hum::HTp>> m_spinesByFeature;
};

} // namespace choralesearch

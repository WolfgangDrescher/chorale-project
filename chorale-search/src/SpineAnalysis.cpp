#include "SpineAnalysis.hpp"

#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

namespace choralesearch {

namespace {

// The analysis tools are only supposed to add spines, never lines. A differing line count means
// the following analyses would be misaligned with the original file, so bail out instead.
void verifyLineCount(const hum::HumdrumFile& infile, int expectedLineCount, const std::string& toolName) {
    if (infile.getLineCount() != expectedLineCount) {
        throw std::runtime_error("Tool " + toolName + " changed the line count of the file from " +
                                 std::to_string(expectedLineCount) + " to " +
                                 std::to_string(infile.getLineCount()));
    }
}

} // namespace

void applySpineAnalysisTools(hum::HumdrumFile& infile) {
    const int lineCount = infile.getLineCount();

    hum::Tool_deg degTool;
    degTool.run(infile);
    if (degTool.hasHumdrumText()) {
        infile.readString(degTool.getHumdrumText());
        verifyLineCount(infile, lineCount, "deg");
    }

    hum::Tool_mint mintTool;
    mintTool.run(infile);
    if (mintTool.hasHumdrumText()) {
        infile.readString(mintTool.getHumdrumText());
        verifyLineCount(infile, lineCount, "mint");
    }

    hum::Tool_metweight metweightTool;
    metweightTool.run(infile);
    if (metweightTool.hasHumdrumText()) {
        infile.readString(metweightTool.getHumdrumText());
        verifyLineCount(infile, lineCount, "metweight");
    }

    hum::Tool_fb fbTool;
    fbTool.process("--process-removes-this-argv -c -n --hint");
    fbTool.run(infile);
    if (fbTool.hasHumdrumText()) {
        // Rename **hint to **fb before parsing -- renaming the token after the fact
        // wouldn't update HumdrumFile's separately cached line text.
        static const std::regex hintRe(R"((^|\t)\*\*hint(?=\t|$))", std::regex::multiline);
        infile.readString(std::regex_replace(fbTool.getHumdrumText(), hintRe, "$1**fb"));
        verifyLineCount(infile, lineCount, "fb");
    }

    std::vector<std::tuple<std::size_t, std::size_t>> hintPairs = {
        {1, 2}, {1, 3}, {1, 4},
        {2, 3}, {2, 4},
        {3, 4}
    };

    for (auto hintPair : hintPairs) {
        std::stringstream cmd;
        cmd << "--process-removes-this-argv -b " << std::get<0>(hintPair) << " -k " << std::get<1>(hintPair) << " --hint";
        hum::Tool_fb fbTool;
        fbTool.process(cmd.str());
        fbTool.run(infile);
        if (fbTool.hasHumdrumText()) {
            // Rename **hint to **hint-xy before parsing -- renaming the token after the fact
            // wouldn't update HumdrumFile's separately cached line text.
            std::stringstream exinterp;
            exinterp << "$1**hint-" << std::get<0>(hintPair) << std::get<1>(hintPair);
            static const std::regex hintRe(R"((^|\t)\*\*hint(?=\t|$))", std::regex::multiline);
            infile.readString(std::regex_replace(fbTool.getHumdrumText(), hintRe, exinterp.str()));
            std::stringstream toolName;
            toolName << "fb --hint " << std::get<0>(hintPair) << std::get<1>(hintPair);
            verifyLineCount(infile, lineCount, toolName.str());
        }
    }
}

} // namespace choralesearch

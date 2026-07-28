#include "SpineAnalysis.hpp"

#include <regex>
#include <sstream>
#include <tuple>
#include <vector>

namespace choralesearch {

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
    fbTool.process("--process-removes-this-argv -c -n --hint");
    fbTool.run(infile);
    if (fbTool.hasHumdrumText()) {
        // Rename **hint to **fb before parsing -- renaming the token after the fact
        // wouldn't update HumdrumFile's separately cached line text.
        static const std::regex hintRe(R"((^|\t)\*\*hint(?=\t|$))", std::regex::multiline);
        infile.readString(std::regex_replace(fbTool.getHumdrumText(), hintRe, "$1**fb"));
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
        }
    }
}

} // namespace choralesearch

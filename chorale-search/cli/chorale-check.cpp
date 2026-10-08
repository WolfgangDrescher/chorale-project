#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "CliInput.hpp"
#include "HumdrumChorale.hpp"
#include "ProgressReport.hpp"
#include "ScoreImport.hpp"
#include "Check.hpp"

using choralesearch::HumdrumChorale;
using choralesearch::Finding;

namespace {

constexpr int kExitError = 1;
constexpr int kExitInvalidArgumentError = 2;
constexpr int kExitValidationError = 3;

void printUsage(const char* argv0) {
    std::cerr <<
        "Usage: " << argv0 << " INPUT [OPTIONS]\n"
        "\n"
        "Checks a chorale score for parallel and hidden fifths and octaves, and for notes outside\n"
        "the range of their voice.\n"
        "\n"
        "Arguments:\n"
        "    INPUT                 the score: a Humdrum **kern or MusicXML file, or '-' for\n"
        "                          stdin. Four voices pass through, a two-staff score is split\n"
        "                          into four.\n"
        "\n"
        "Options:\n"
        "    --voice-ranges strauss-berlioz|bach\n"
        "                          the voice ranges the notes are measured against: the ones of\n"
        "                          Berlioz and Strauss, or the ones Bach's chorales use\n"
        "                          (default: strauss-berlioz)\n"
        "    --hidden-motion-allow-steps true|false\n"
        "                          whether a hidden fifth or octave is excused when the soprano\n"
        "                          reaches it by a step; false finds those too (default: true)\n"
        "    --no-kern             leave the converted four-voice **kern text out of the\n"
        "                          output -- for command-line use, where the score is\n"
        "                          already at hand and only the findings matter\n"
        "    --progress            write the progress to stderr, one JSON object per line: the\n"
        "                          stage being worked on\n"
        "    --help, -h            show this help\n";
}

// A finding as the page draws it: the two voices (1 is the bass), and the line each of the two
// sonorities is attacked on -- the same line in both voices.
nlohmann::json findingToJson(const Finding& finding) {
    return {
        {"check", finding.check},
        {"severity", finding.severity},
        {"direction", finding.direction},
        {"voices", {finding.lowerVoice, finding.upperVoice}},
        {"startLine", finding.startLine},
        {"endLine", finding.endLine},
        {"startPosition", finding.startPosition},
        {"endPosition", finding.endPosition},
    };
}

} // namespace

int main(int argc, char** argv) {
    std::string inputPath;
    bool includeKern = true;
    bool progress = false;
    bool allowStepwiseHiddenMotion = true;
    choralesearch::VoiceRangeSet voiceRanges = choralesearch::VoiceRangeSet::StraussBerlioz;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        auto next = [&](const char* flag) -> std::string {
            if (i + 1 >= argc) throw std::invalid_argument(std::string(flag) + " needs a value");
            return argv[++i];
        };
        try {
            if (arg == "--voice-ranges") {
                const std::string value = next("--voice-ranges");
                if (value == "strauss-berlioz") voiceRanges = choralesearch::VoiceRangeSet::StraussBerlioz;
                else if (value == "bach") voiceRanges = choralesearch::VoiceRangeSet::Bach;
                else throw std::invalid_argument("--voice-ranges takes strauss-berlioz or bach, got '" + value + "'");
            }
            else if (arg == "--hidden-motion-allow-steps") {
                allowStepwiseHiddenMotion =
                    parseBoolean("--hidden-motion-allow-steps", next("--hidden-motion-allow-steps"));
            }
            else if (arg == "--no-kern") { includeKern = false; }
            else if (arg == "--progress") { progress = true; }
            else if (arg == "--help" || arg == "-h") { printUsage(argv[0]); return 0; }
            else if (!arg.empty() && arg[0] == '-' && arg != "-") {
                std::cerr << "Unknown option: " << arg << "\n";
                printUsage(argv[0]);
                return kExitInvalidArgumentError;
            }
            else { inputPath = arg; }
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << "\n";
            return kExitInvalidArgumentError;
        }
    }

    if (inputPath.empty()) {
        printUsage(argv[0]);
        return kExitInvalidArgumentError;
    }

    // '-' was asked for explicitly, but nothing is piped in -- reading would sit and wait for
    // a human to type a score, so the usage is the better answer.
    if (inputPath == "-" && stdinIsInteractive()) {
        printUsage(argv[0]);
        return kExitInvalidArgumentError;
    }

    std::string input;
    try {
        input = readInput(inputPath);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return kExitInvalidArgumentError;
    }
    if (input.find_first_not_of(" \t\r\n") == std::string::npos) {
        std::cerr << "Error: the input is empty\n\n";
        printUsage(argv[0]);
        return kExitInvalidArgumentError;
    }

    try {
        const choralesearch::PreparedScore score = choralesearch::prepareScore(input, progress);

        // Building the chorale derives its analysis spines (**mint, **hint-xy, ...) the queries
        // read, so it is a phase of its own.
        if (progress) choralesearch::reportPhase("analyze-score");
        std::istringstream contents(score.kern);
        HumdrumChorale chorale(contents, inputPath == "-" ? "stdin" : inputPath);

        if (progress) choralesearch::reportPhase("run-checks");
        const std::vector<Finding> findings = choralesearch::runChecks(chorale, voiceRanges, allowStepwiseHiddenMotion);

        nlohmann::json j;
        if (inputPath != "-") j["source"] = inputPath;
        j["inputFormat"] = score.inputFormat;
        j["layout"] = score.layout;
        if (includeKern) j["kern"] = score.kern;
        j["findings"] = nlohmann::json::array();
        for (const Finding& finding : findings) j["findings"].push_back(findingToJson(finding));
        std::cout << j.dump(1, '\t') << '\n';
        std::cerr << findings.size() << " finding(s)\n";
    } catch (const std::invalid_argument& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return kExitValidationError;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return kExitError;
    }

    return 0;
}

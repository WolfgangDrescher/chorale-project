#include <algorithm>
#include <cctype>
#include <iostream>
#include <fstream>
#include <map>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

#include "AttributeMatcher.hpp"
#include "CliInput.hpp"
#include "CorpusSearch.hpp"
#include "SplitScoreIntoVoices.hpp"
#include "HumdrumChorale.hpp"
#include "HumdrumUtils.hpp"
#include "ProgressReport.hpp"
#include "Query.hpp"
#include "Result.hpp"
#include "ScoreImport.hpp"
#include "Segmentation.hpp"

using choralesearch::AttributeMatcher;
using choralesearch::CorpusSearch;
using choralesearch::HumdrumChorale;
using choralesearch::Query;
using choralesearch::Result;
using choralesearch::Segment;
using choralesearch::SegmentationOptions;
using choralesearch::SegmentQueryOptions;

namespace {

constexpr int kExitError = 1;
constexpr int kExitInvalidArgumentError = 2;
constexpr int kExitValidationError = 3;

void printUsage(const char* argv0) {
    std::cerr <<
        "Usage: " << argv0 << " INPUT [OPTIONS]\n"
        "\n"
        "Arguments:\n"
        "    INPUT                 the score: a Humdrum **kern or MusicXML file, or '-' for\n"
        "                          stdin. Four voices pass through, a two-staff score is split\n"
        "                          into four. Analysis spines are derived here.\n"
        "\n"
        "Options:\n"
        "    --length N            segment length in quarter notes (default: 4)\n"
        "    --mint-ignore-quality true|false\n"
        "                          leave the interval qualities out of the queries (\"+M2\"\n"
        "                          becomes \"+2\") so a passage is found in major and minor\n"
        "                          (default: true)\n"
        "    --mint-allow-interval-complementation true|false\n"
        "                          let a query's interval also match its inversion (\"+3\" also\n"
        "                          finds \"-6\") (default: true)\n"
        "    --metweight-skip-unclassified true|false\n"
        "                          fold the ornaments (notes on unclassified metric positions)\n"
        "                          away in the queries instead of counting them (default: true)\n"
        "    --inner-voices true|false\n"
        "                          also ask for the harmony the inner voices make with the\n"
        "                          bass: the chord at every bass note, compared exactly and\n"
        "                          in no particular order of the voices (default: false)\n"
        "    --no-kern             leave the converted four-voice **kern text out of the\n"
        "                          output -- for command-line use, where the score is\n"
        "                          already at hand and only the segments matter\n"
        "    --stats CORPUS_DIR    search the corpus for every segment's query and add a\n"
        "                          \"stats\" property to each segment: matches, choraleCount\n"
        "                          and topChorales\n"
        "    --bass-lines true|false\n"
        "                          with --stats, also find how often each segment's soprano line\n"
        "                          occurs in the corpus (on the same metric positions) and the five\n"
        "                          most frequent bass lines under those matches (default: false)\n"
        "    --no-analysis         with --stats: read the analysis spines straight from the\n"
        "                          corpus instead of deriving them per run -- for a corpus\n"
        "                          built by chorale-generate --analysis\n"
        "    --progress            write the progress to stderr, one JSON object per line: the\n"
        "                          stage being worked on, and with --stats one event for every\n"
        "                          chorale file searched\n"
        "    --help, -h            show this help\n";
}

hum::HumNum parseLength(const std::string& value) {
    try {
        std::size_t consumed = 0;
        int length = std::stoi(value, &consumed);
        if (consumed != value.size() || length <= 0) throw std::invalid_argument("");
        return length;
    } catch (const std::exception&) {
        throw std::invalid_argument("--length takes a single positive whole number of quarter notes, got '" + value +
                                     "'");
    }
}

// true/false, yes/no, y/n or 1/0, in any case.
bool parseBoolean(const std::string& flag, const std::string& value) {
    std::string lowered = value;
    std::transform(lowered.begin(), lowered.end(), lowered.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (lowered == "true" || lowered == "yes" || lowered == "y" || lowered == "1") return true;
    if (lowered == "false" || lowered == "no" || lowered == "n" || lowered == "0") return false;
    throw std::invalid_argument(flag + " takes true/false, yes/no, y/n or 1/0, got '" + value + "'");
}

// The full stats per segment id, gathered in one corpus pass over all the segments' queries:
// what chorale-search --stats says about a single query (matches, choraleCount), plus the
// topChorales ranking this tool adds -- a segment is looked at one at a time, so where its
// passage turns up is worth naming.
std::map<std::string, nlohmann::json> statsForSegments(const std::vector<Segment>& segments,
                                                        const std::string& corpusDir, bool applyAnalysis,
                                                        bool reportProgress) {
    std::vector<Query> queries;
    queries.reserve(segments.size());
    for (const Segment& segment : segments) queries.push_back(segment.query);

    CorpusSearch search(corpusDir, applyAnalysis);
    if (reportProgress) search.setProgressCallback(choralesearch::progressToStderr());
    const choralesearch::Results results = search.run(queries);

    std::map<std::string, std::map<std::string, std::size_t>> matchesPerChorale; // by queryId, then choraleId
    for (const Result& result : results) {
        ++matchesPerChorale[result.queryId.value_or("")][result.choraleId];
    }

    // Walked over the segments rather than over what the corpus answered, so a segment whose
    // query matched nothing is built by these same lines and states its zeroes. Nowhere else
    // does a stats object have to be spelled out, so renaming a property is one edit.
    std::map<std::string, nlohmann::json> stats;
    for (const Segment& segment : segments) {
        const std::string id = segment.query.id.value_or("");
        const std::map<std::string, std::size_t>& perChorale = matchesPerChorale[id];

        std::size_t matches = 0;
        for (const auto& [choraleId, count] : perChorale) matches += count;

        // Where the matches pile up, most first. Capped, so a segment whose passage is
        // everywhere doesn't answer with the whole corpus -- five is enough to see whether the
        // matches cluster or scatter. Ties keep chorale order, which the map already has.
        std::vector<std::pair<std::string, std::size_t>> ranked(perChorale.begin(), perChorale.end());
        std::stable_sort(ranked.begin(), ranked.end(), [](const auto& a, const auto& b) { return a.second > b.second; });
        constexpr std::size_t kTopChorales = 5;
        nlohmann::json top = nlohmann::json::array();
        for (std::size_t i = 0; i < ranked.size() && i < kTopChorales; ++i) {
            top.push_back({{"choraleId", ranked[i].first}, {"matches", ranked[i].second}});
        }

        stats[id] = {
            {"matches", matches},
            {"choraleCount", perChorale.size()},
            {"topChorales", std::move(top)},
        };
    }
    return stats;
}

// The bass lines Bach sets under the cantus firmus (c.f.) of each segment: the soprano line of the
// segment is searched in the corpus, and under every match the bass is read.

constexpr std::size_t kBassVoice = 1;   // voices are numbered from the bass up
constexpr std::size_t kBassLineLimit = 5; // how many bass lines a segment gets

// The first note the voice attacks on or after `line`.
hum::HTp firstNoteFromLine(hum::HTp voiceStart, int line) {
    if (!voiceStart) return nullptr;
    for (hum::HTp token = voiceStart->getNextToken(); token; token = token->getNextToken()) {
        if (!token->getOwner()->isData() || token->isNull() || token->isSecondaryTiedNote()) continue;
        if (token->getLineNumber() >= line) return token;
    }
    return nullptr;
}

// The interval, as the transpose filter takes it ("-M2", "P4"), that moves the note `from` onto
// the note `to`. Empty where they are the same note, or where the interval has no name.
std::string transpositionInterval(hum::HTp from, hum::HTp to) {
    std::string interval = choralesearch::mintIntervalToken(from, to);
    if (interval.empty() || interval == "P1" || interval.find('X') != std::string::npos) return "";
    if (interval.front() == '+') interval.erase(0, 1);
    return interval;
}

// What the bass does under one match of a c.f.
struct BassLine {
    std::string startInterval;          // to the c.f., where the match starts
    std::vector<std::string> intervals; // between its own notes from there on
    std::size_t startLine = 0;          // of the bass note under the match, which may sound from before it

    // The same for the same bass line.
    std::string id() const {
        std::string id = startInterval + " |";
        for (const std::string& interval : intervals) id += " " + interval;
        return id;
    }
};

// The bass line under `match`, or nothing where the bass has no note in it. The interval into the
// first bass note belongs to what came before the match and is left out.
std::optional<BassLine> readBassLine(const HumdrumChorale& chorale, const std::vector<AttributeMatcher::Onset>& bass,
                                     const Result& match, const SegmentQueryOptions& options) {
    auto lineOf = [](const AttributeMatcher::Onset& note) { return static_cast<std::size_t>(note.token->getLineNumber()); };

    // The bass note sounding where the match starts is the last one attacked on or before it.
    std::size_t first = 0;
    while (first + 1 < bass.size() && lineOf(bass[first + 1]) <= match.startLineNumber) ++first;
    if (first >= bass.size() || lineOf(bass[first]) > match.endLineNumber) return std::nullopt;

    BassLine bassLine;
    bassLine.startLine = std::min(match.startLineNumber, lineOf(bass[first]));

    for (std::size_t i = first + 1; i < bass.size() && lineOf(bass[i]) <= match.endLineNumber; ++i) {
        // Where ornaments were folded away, the interval is the one measured across them.
        std::string interval = bass[i].mint ? *bass[i].mint : std::string(*bass[i].token);
        if (options.ignoreIntervalQuality) interval = choralesearch::withoutQuality(interval);
        bassLine.intervals.push_back(std::move(interval));
    }

    // The same bass line under another c.f. note is another setting, so the interval between the
    // two where the match starts belongs to it.
    const std::string intervalSpine = "hint-" + std::to_string(kBassVoice) + std::to_string(match.voice);
    const int startLine = static_cast<int>(match.startLineNumber);
    if (hum::HTp token = choralesearch::findTokenAtLine(chorale.spine(intervalSpine, 1), startLine)) {
        bassLine.startInterval = std::string(*token);
        if (options.matcherOptions.hintReduceCompound) {
            bassLine.startInterval = choralesearch::reduceHintInterval(bassLine.startInterval);
        }
        if (options.ignoreIntervalQuality) bassLine.startInterval = choralesearch::withoutQuality(bassLine.startInterval);
    }
    return bassLine;
}

// A bass line and where in the corpus it is.
struct BassLineFinding {
    nlohmann::json occurrences = nlohmann::json::array(); // {choraleId, startLine, endLine} of each
    std::set<std::string> chorales;
    nlohmann::json example; // the first occurrence, and how to show it
};

// What the corpus has for the c.f. of one segment.
struct CantusFirmusFindings {
    std::size_t matches = 0;
    std::map<std::string, BassLineFinding> bassLines; // by BassLine::id()

    // The most frequent bass lines first, as the JSON the segment's stats carry.
    nlohmann::json topBassLines() const {
        std::vector<const BassLineFinding*> ranked;
        for (const auto& entry : bassLines) ranked.push_back(&entry.second);
        std::stable_sort(ranked.begin(), ranked.end(), [](const auto* a, const auto* b) {
            if (a->occurrences.size() != b->occurrences.size()) return a->occurrences.size() > b->occurrences.size();
            return a->chorales.size() > b->chorales.size();
        });

        nlohmann::json json = nlohmann::json::array();
        for (std::size_t i = 0; i < ranked.size() && i < kBassLineLimit; ++i) {
            json.push_back({{"matches", ranked[i]->occurrences.size()},
                            {"choraleCount", ranked[i]->chorales.size()},
                            {"example", ranked[i]->example},
                            {"occurrences", ranked[i]->occurrences}});
        }
        return json;
    }
};

// Collects the bass lines under the matches the corpus search finds, per segment.
class BassLineCollector {
public:
    // `segmentStartNotes`: the first note of each segment's c.f., by segment id.
    BassLineCollector(const SegmentQueryOptions& options, std::map<std::string, hum::HTp> segmentStartNotes)
        : m_options(options), m_segmentStartNotes(std::move(segmentStartNotes)) {
        m_bassOptions.metweightSkipUnclassified = options.matcherOptions.metweightSkipUnclassified;
    }

    // To be called for every match of a segment's c.f.
    void add(const HumdrumChorale& chorale, const Result& match) {
        const std::string segmentId = match.queryId.value_or("");
        CantusFirmusFindings& findings = m_findings[segmentId];
        ++findings.matches;

        if (m_bassChoraleId != chorale.id()) { // the corpus is searched chorale by chorale
            m_bassChoraleId = chorale.id();
            m_bass = AttributeMatcher("mint", {}, m_bassOptions).buildOnsets(chorale, kBassVoice);
        }
        const std::optional<BassLine> bassLine = readBassLine(chorale, m_bass, match, m_options);
        if (!bassLine) return;

        const nlohmann::json occurrence = {{"choraleId", match.choraleId},
                                           {"startLine", bassLine->startLine},
                                           {"endLine", match.endLineNumber}};
        BassLineFinding& finding = findings.bassLines[bassLine->id()];
        if (finding.occurrences.empty()) {
            finding.example = occurrence;
            finding.example["transpose"] =
                transpositionInterval(matchStartNote(chorale, match), m_segmentStartNotes[segmentId]);
        }
        finding.occurrences.push_back(occurrence);
        finding.chorales.insert(match.choraleId);
    }

    const CantusFirmusFindings& findings(const std::string& segmentId) { return m_findings[segmentId]; }

private:
    static hum::HTp matchStartNote(const HumdrumChorale& chorale, const Result& match) {
        return choralesearch::findTokenAtLine(chorale.spine("kern", match.voice), static_cast<int>(match.startLineNumber));
    }

    SegmentQueryOptions m_options;
    std::map<std::string, hum::HTp> m_segmentStartNotes;
    std::map<std::string, CantusFirmusFindings> m_findings; // by segment id

    // The bass of the chorale being searched, read once for all of its matches.
    choralesearch::MatcherOptions m_bassOptions;
    std::string m_bassChoraleId;
    std::vector<AttributeMatcher::Onset> m_bass;
};

// Adds "cantusFirmus" to the stats of every segment: how often the corpus has its c.f. (on the
// same metric positions, without the other voices), and the most frequent bass lines under those.
// One search of the corpus for all segments.
void addCantusFirmusFindings(const HumdrumChorale& score, const std::vector<Segment>& segments,
                             std::map<std::string, nlohmann::json>& stats,
                             const SegmentationOptions& segmentationOptions, const SegmentQueryOptions& queryOptions,
                             const std::string& corpusDir, bool applyAnalysis, bool reportProgress) {
    // The segments again as their c.f. alone: the windows are the same, so the ids are too.
    SegmentQueryOptions cantusFirmusOptions = queryOptions;
    cantusFirmusOptions.simultaneousVoices.clear();
    cantusFirmusOptions.metricPositions = true;
    std::vector<Query> queries;
    for (const Segment& segment : choralesearch::segmentScore(score, segmentationOptions, cantusFirmusOptions)) {
        queries.push_back(segment.query);
    }

    std::map<std::string, hum::HTp> segmentStartNotes;
    for (const Segment& segment : segments) {
        segmentStartNotes[segment.query.id.value_or("")] =
            firstNoteFromLine(score.spine("kern", queryOptions.voice), segment.startLineNumber);
    }

    BassLineCollector collector(queryOptions, std::move(segmentStartNotes));
    if (reportProgress) choralesearch::reportPhase("collect-bass-lines");
    CorpusSearch search(corpusDir, applyAnalysis);
    if (reportProgress) search.setProgressCallback(choralesearch::progressToStderr());
    search.forEachMatch(queries, [&](const HumdrumChorale& chorale, const Result& match) { collector.add(chorale, match); });

    for (const Segment& segment : segments) {
        const std::string id = segment.query.id.value_or("");
        const CantusFirmusFindings& findings = collector.findings(id);
        stats[id]["cantusFirmus"] = {
            {"matches", findings.matches},
            {"topBassLines", findings.topBassLines()},
        };
    }
}

nlohmann::json segmentToJson(const Segment& segment) {
    nlohmann::json entry;
    entry["id"] = segment.query.id.value_or("");
    entry["startPosition"] = choralesearch::humNumToString(segment.startPosition);
    entry["endPosition"] = choralesearch::humNumToString(segment.endPosition);
    entry["startLine"] = segment.startLineNumber;
    entry["endLine"] = segment.endLineNumber;
    entry["query"] = choralesearch::queryToJson(segment.query);
    return entry;
}

} // namespace

int main(int argc, char** argv) {
    std::string inputPath;
    bool includeKern = true;
    std::string statsCorpusDir;
    bool applyAnalysis = true;
    bool progress = false;
    bool bassLines = false;
    SegmentationOptions segmentationOptions;
    SegmentQueryOptions queryOptions;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        auto next = [&](const char* flag) -> std::string {
            if (i + 1 >= argc) throw std::invalid_argument(std::string(flag) + " needs a value");
            return argv[++i];
        };
        try {
            if (arg == "--length") { segmentationOptions.length = parseLength(next("--length")); }
            else if (arg == "--mint-ignore-quality") {
                queryOptions.ignoreIntervalQuality =
                    parseBoolean("--mint-ignore-quality", next("--mint-ignore-quality"));
            }
            else if (arg == "--mint-allow-interval-complementation") {
                const bool allowed = parseBoolean("--mint-allow-interval-complementation",
                                                   next("--mint-allow-interval-complementation"));
                queryOptions.matcherOptions.mintAllowIntervalComplementation =
                    allowed ? std::vector<std::string>{"*"} : std::vector<std::string>{};
            }
            else if (arg == "--metweight-skip-unclassified") {
                queryOptions.matcherOptions.metweightSkipUnclassified =
                    parseBoolean("--metweight-skip-unclassified", next("--metweight-skip-unclassified"));
            }
            else if (arg == "--inner-voices") {
                queryOptions.innerVoices = parseBoolean("--inner-voices", next("--inner-voices"));
            }
            else if (arg == "--bass-lines") {
                bassLines = parseBoolean("--bass-lines", next("--bass-lines"));
            }
            else if (arg == "--no-kern") { includeKern = false; }
            else if (arg == "--stats") { statsCorpusDir = next("--stats"); }
            else if (arg == "--no-analysis") { applyAnalysis = false; }
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

    if (bassLines && statsCorpusDir.empty()) {
        std::cerr << "Error: --bass-lines requires --stats\n\n";
        printUsage(argv[0]);
        return kExitInvalidArgumentError;
    }

    if (!applyAnalysis && statsCorpusDir.empty()) {
        std::cerr << "Error: --no-analysis requires --stats\n\n";
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
        // The four **kern spines, bass to soprano, whatever arrived (see prepareScore).
        const choralesearch::PreparedScore score = choralesearch::prepareScore(input, progress);
        const std::string& kern = score.kern;
        const std::string& layout = score.layout;
        const std::string& inputFormat = score.inputFormat;

        // Building the chorale derives its analysis spines (**deg, **mint, **metweight, ...),
        // which is where most of the time before the search goes, so it is a phase of its own.
        if (progress) choralesearch::reportPhase("analyze-score");
        std::istringstream contents(kern);
        HumdrumChorale chorale(contents, inputPath == "-" ? "stdin" : inputPath);

        if (progress) choralesearch::reportPhase("segment-score");
        const std::vector<Segment> segments = choralesearch::segmentScore(chorale, segmentationOptions,
                                                                           queryOptions);

        nlohmann::json j;
        if (inputPath != "-") j["source"] = inputPath;
        j["inputFormat"] = inputFormat;
        j["layout"] = layout;
        if (includeKern) j["kern"] = kern;
        std::map<std::string, nlohmann::json> stats;
        if (!statsCorpusDir.empty()) {
            if (progress) choralesearch::reportPhase("search-corpus");
            stats = statsForSegments(segments, statsCorpusDir, applyAnalysis, progress);
            if (bassLines) {
                addCantusFirmusFindings(chorale, segments, stats, segmentationOptions, queryOptions, statsCorpusDir,
                                        applyAnalysis, progress);
            }
        }

        j["segments"] = nlohmann::json::array();
        for (const Segment& segment : segments) {
            nlohmann::json entry = segmentToJson(segment);
            if (!stats.empty()) entry["stats"] = stats.at(segment.query.id.value_or(""));
            j["segments"].push_back(std::move(entry));
        }
        std::cout << j.dump(1, '\t') << '\n';
        std::cerr << segments.size() << " segment(s)\n";
    } catch (const std::invalid_argument& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return kExitValidationError;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return kExitError;
    }

    return 0;
}

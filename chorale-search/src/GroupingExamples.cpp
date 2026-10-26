#include "GroupingExamples.hpp"

#include <algorithm>
#include <map>
#include <string>

#include "HumdrumChorale.hpp"

namespace choralesearch {

namespace {

// The notes of the voice between the two lines on an unclassified metric position.
std::size_t ornamentCountBetween(const HumdrumChorale& chorale, std::size_t voice, std::size_t startLine, std::size_t endLine) {
    std::size_t count = 0;
    for (std::size_t line = startLine; line <= endLine; ++line) {
        hum::HTp token = chorale.tokenAtLine("metweight", voice, static_cast<int>(line));
        if (token && std::string(*token) == "u") ++count;
    }
    return count;
}

bool isLessOrnamented(const GroupExample& example, const GroupExample& other) {
    return example.ornamentedVoices.size() < other.ornamentedVoices.size();
}

bool showsChorale(const std::vector<GroupExample>& examples, const std::string& choraleId) {
    return std::any_of(examples.begin(), examples.end(), [&](const GroupExample& example) { return example.choraleId == choraleId; });
}

} // namespace

GroupExample exampleOf(const FoundMatch& match, const std::set<std::size_t>& voicesReadInOwnRhythm) {
    GroupExample example;
    example.choraleId = match.result.choraleId;
    example.startLine = match.firstLine();
    example.endLine = match.lastLine();
    // The measure of each line, -1 before the first barline.
    const std::vector<int> measures = match.chorale.spine("kern", 1)->getOwner()->getOwner()->getMeasureNumbers();
    example.firstMeasure = std::max(0, measures[example.startLine - 1]);
    example.lastMeasure = std::max(0, measures[example.endLine - 1]);
    std::set<std::size_t> voicesRead = voicesReadInOwnRhythm;
    voicesRead.insert(match.result.voice);
    for (std::size_t voice : voicesRead) {
        const std::size_t ornaments = ornamentCountBetween(match.chorale, voice, example.startLine, example.endLine);
        if (ornaments > 0) example.ornamentedVoices.push_back(voice);
        example.ornamentCount += ornaments;
    }
    return example;
}

std::vector<GroupExample> chooseExamples(const std::vector<const MatchReading*>& matches, const ExampleSettings& settings) {
    if (settings.maxCount == 0) return {};
    // The first passage of each set of ornamented voices, the less ornamented first.
    std::map<std::vector<std::size_t>, GroupExample> firstPerOrnamentedVoices;
    for (const MatchReading* match : matches) firstPerOrnamentedVoices.emplace(match->example.ornamentedVoices, match->example);
    std::vector<GroupExample> examples;
    for (const auto& [ornamentedVoices, example] : firstPerOrnamentedVoices) examples.push_back(example);
    std::stable_sort(examples.begin(), examples.end(), isLessOrnamented);
    if (examples.size() > settings.maxCount) examples.resize(settings.maxCount);
    // Then the first passage of each further chorale.
    for (const MatchReading* match : matches) {
        if (examples.size() == settings.maxCount) break;
        if (!showsChorale(examples, match->example.choraleId)) examples.push_back(match->example);
    }
    return examples;
}

const GroupExample& preferredExampleOf(const std::vector<const MatchReading*>& matches) {
    const GroupExample* preferred = &matches.front()->example;
    for (const MatchReading* match : matches) {
        if (isLessOrnamented(match->example, *preferred)) preferred = &match->example;
    }
    return *preferred;
}

} // namespace choralesearch

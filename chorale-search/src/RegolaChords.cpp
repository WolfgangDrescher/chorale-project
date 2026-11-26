#include "RegolaChords.hpp"

#include <algorithm>
#include <sstream>
#include <stdexcept>

namespace choralesearch {

namespace {

const std::string kDefaultTag = "core";

bool contains(const auto& values, const auto& value) {
    return std::find(values.begin(), values.end(), value) != values.end();
}

// "1 3 #6" as its figures.
std::vector<RegolaFigure> figures(const std::string& written) {
    std::vector<RegolaFigure> parsed;
    std::istringstream stream(written);
    for (std::string figure; stream >> figure;) {
        const int alteration = figure[0] == '#' ? 1 : figure[0] == 'b' ? -1 : 0;
        parsed.push_back({std::stoi(figure.substr(alteration == 0 ? 0 : 1)), alteration});
    }
    return parsed;
}

std::vector<RegolaChord> buildChords() {
    using enum BassMotion;
    using enum ChordMode;
    const std::vector<std::string> core = {"core"};
    const std::vector<std::string> extended = {"extended"};
    const auto with = [](std::vector<std::string> tags, std::initializer_list<std::string> more) {
        tags.insert(tags.end(), more);
        return tags;
    };
    // From glarean.mh-freiburg.de/kartimento.
    return {
        {"1", "5 3", {}, core, figures("1 3 5")},
        {"2", "6", {Ascending, Descending}, with(core, {"semitonium-modi"}), figures("1 3 6")},
        {"3", "6", {}, core, figures("1 3 6")},
        {"4", "6 5", {Ascending}, with(core, {"quinta-syncopata"}), figures("1 3 5 6")},
        {"4", "2", {Descending}, with(core, {"semitonium-modi", "passing"}), figures("1 2 4 6")},
        {"5", "5 3", {}, with(core, {"semitonium-modi"}), figures("1 3 5")},
        {"6", "6", {Ascending}, core, figures("1 3 6")},
        {"6", "6", {Descending}, with(core, {"major-sixth"}), figures("1 3 6")},
        {"7", "6 5", {Ascending}, core, figures("1 3 5 6")},
        {"7", "6", {Descending}, core, figures("1 3 6")},
        {"2", "4 3", {Ascending, Descending}, with(core, {"enriched", "semitonium-modi"}), figures("1 3 4 6")},
        {"6", "4 3", {Descending}, with(core, {"enriched", "major-sixth"}), figures("1 3 4 6")},
        {"2", "5 3", {Leap}, with(extended, {"leap"}), figures("1 3 5"), Major},
        {"4", "5 3", {Leap}, with(extended, {"leap"}), figures("1 3 5")},
        {"4", "5 3", {Descending}, extended, figures("1 3 5")},
        {"5", "5 4", {}, with(extended, {"suspension"}), figures("1 3 4 5")},
        {"5", "6 4", {}, with(extended, {"suspension", "double-suspension"}), figures("1 3 4 5 6")},
        {"6", "5 3", {Leap}, with(extended, {"leap"}), figures("1 3 5")},
        {"6", "5 3", {}, extended, figures("1 3 5")},
        {"4+", "6 5", {}, extended, figures("1 3 5 6")},
        {"4", "b6", {Ascending}, with(extended, {"affect"}), figures("1 3 b6")},
        {"6", "#6", {Descending}, with(extended, {"affect"}), figures("1 3 #6")},
        {"6", "#6 4 3", {Descending}, with(extended, {"affect"}), figures("1 3 4 #6")},
        {"6", "#6 5", {Descending}, with(extended, {"affect"}), figures("1 3 5 #6")},
        {"7", "7", {Ascending}, with(extended, {"affect"}), figures("1 3 5 7")},
        {"4+", "7", {}, with(extended, {"affect"}), figures("1 3 5 7")},
        {"2", "5 6", {Ascending}, with(extended, {"affect"}), figures("1 3 5 6"), Major},
        {"2", "6 5", {Ascending}, with(extended, {"affect", "semitonium-modi"}), figures("1 3 5 6"), Minor},
        {"4", "3 4", {Descending}, with(extended, {"affect"}), figures("1 3 4 5"), Major},
        {"4", "4 3", {Descending}, with(extended, {"affect", "semitonium-modi"}), figures("1 3 4 6"), Minor},
        {"6", "2", {Descending}, with(extended, {"affect", "semitonium-modi"}), figures("1 2 4 6"), Minor},
        {"5", "6 4", {}, with(extended, {"passing"}), figures("1 4 6")},
        {"1", "6 4", {}, with(extended, {"passing"}), figures("1 4 6")},
    };
}

std::vector<std::string> collectTagNames() {
    std::vector<std::string> names;
    for (const RegolaChord& chord : regolaChords()) {
        for (const std::string& tag : chord.tags) {
            if (!contains(names, tag)) names.push_back(tag);
        }
    }
    return names;
}

bool isNegatedTag(const std::string& tag) {
    return !tag.empty() && tag[0] == '!';
}

} // namespace

bool RegolaChord::allowsBassMotion(const std::vector<BassMotion>& motions) const {
    return bassMotions.empty() ||
           std::any_of(motions.begin(), motions.end(), [&](BassMotion motion) { return contains(bassMotions, motion); });
}

bool RegolaChord::allowsMode(bool minor) const {
    return mode == ChordMode::Any || (mode == ChordMode::Minor) == minor;
}

bool RegolaChord::hasTag(const std::string& tag) const {
    return contains(tags, tag);
}

const std::vector<RegolaChord>& regolaChords() {
    static const std::vector<RegolaChord> chords = buildChords();
    return chords;
}

const std::vector<std::string>& regolaTagNames() {
    static const std::vector<std::string> names = collectTagNames();
    return names;
}

bool isValidRegolaTag(const std::string& tag) {
    return contains(regolaTagNames(), isNegatedTag(tag) ? tag.substr(1) : tag);
}

std::vector<std::size_t> allowedRegolaChords(const std::vector<std::string>& tags) {
    std::vector<std::string> positive;
    std::vector<std::string> negated;
    for (const std::string& tag : tags) {
        if (!isValidRegolaTag(tag)) throw std::invalid_argument("'" + tag + "' is no tag of a chord of the regola");
        if (isNegatedTag(tag)) negated.push_back(tag.substr(1));
        else positive.push_back(tag);
    }
    if (tags.empty()) positive.push_back(kDefaultTag);

    std::vector<std::size_t> allowed;
    const std::vector<RegolaChord>& chords = regolaChords();
    for (std::size_t index = 0; index < chords.size(); ++index) {
        const auto hasTag = [&](const std::string& tag) { return chords[index].hasTag(tag); };
        const bool chosen = positive.empty() || std::any_of(positive.begin(), positive.end(), hasTag);
        if (chosen && std::none_of(negated.begin(), negated.end(), hasTag)) allowed.push_back(index);
    }
    return allowed;
}

} // namespace choralesearch

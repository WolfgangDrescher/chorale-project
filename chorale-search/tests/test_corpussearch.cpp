#include "test_framework.hpp"

#include <algorithm>
#include <atomic>
#include <filesystem>
#include <fstream>
#include <map>
#include <set>

#include "CorpusSearch.hpp"
#include "HumdrumChorale.hpp"
#include "Query.hpp"

using choralesearch::AttributeMap;
using choralesearch::ChoraleIdRange;
using choralesearch::CorpusSearch;
using choralesearch::CountRange;
using choralesearch::HumdrumChorale;
using choralesearch::NoteSection;
using choralesearch::Query;
using choralesearch::Result;
using choralesearch::SimultaneousGroup;

// Every fixture chorale has exactly 6 soprano fermatas except chor006.krn (added
// for rest-matching coverage) with 4, chor005.krn with 8 and chor008.krn with 9
// (both added for ornament coverage); all three sort between chor001 and chor009.
// Tests below exercise CorpusSearch behaviour (file discovery, aggregation,
// result mapping, limit truncation), not AttributeMatcher.

TEST_CASE(run_one_populates_result_fields_from_a_real_match) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    CorpusSearch search(chorale.path()); // corpusRoot is unused by runOne, but the constructor needs one
    Query q;
    q.feature = "kern";
    q.pattern = {AttributeMap{{"fermata", {"true"}}}};
    q.voices = "soprano";
    auto results = search.runOne(chorale, q);

    REQUIRE(results.size() == 6u);
    const Result& r = results[0];
    CHECK_EQ(r.choraleId, std::string("chor029"));
    CHECK_EQ(r.feature, std::string("kern"));
    CHECK_EQ(r.voice, std::size_t{4});
    CHECK(r.startLineNumber > 0);
    CHECK_EQ(r.startLineNumber, r.endLineNumber); // single-position pattern
	CHECK_EQ(r.startPosition, std::string("7"));
    CHECK_EQ(r.endPosition, std::string("7"));
}

TEST_CASE(run_reports_progress_once_per_chorale_file) {
    auto fixturesDir = std::filesystem::path(FIXTURE_CHORALE("chor029")).parent_path();
    std::size_t files = 0;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(fixturesDir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".krn") ++files;
    }

    CorpusSearch search(fixturesDir);
    std::vector<choralesearch::SearchProgress> reports;
    search.setProgressCallback([&](const choralesearch::SearchProgress& progress) { reports.push_back(progress); });
    Query q;
    q.feature = "kern";
    q.pattern = {AttributeMap{{"fermata", {"true"}}}};
    q.voices = "soprano";
    search.run(q);

    REQUIRE(reports.size() == files);
    for (std::size_t i = 0; i < reports.size(); ++i) {
        CHECK_EQ(reports[i].choralesSearched, i + 1);
        CHECK_EQ(reports[i].choralesTotal, files);
    }
}

TEST_CASE(run_one_truncates_results_at_the_query_limit) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    CorpusSearch search(chorale.path());
    Query q;
    q.feature = "kern";
    q.pattern = {AttributeMap{{"fermata", {"true"}}}};
    q.voices = "soprano";
    q.limit = 2; // out of 6 available
    auto results = search.runOne(chorale, q);
    CHECK_EQ(results.size(), std::size_t{2});
}

TEST_CASE(run_searches_a_single_file_used_as_the_corpus_root) {
    CorpusSearch search(FIXTURE_CHORALE("chor029"));
    Query q;
    q.feature = "kern";
    q.pattern = {AttributeMap{{"fermata", {"true"}}}};
    q.voices = "soprano";
    auto results = search.run(q);
    REQUIRE(results.size() == 6u);
    CHECK_EQ(results[0].choraleId, std::string("chor029"));
}

TEST_CASE(run_aggregates_matches_across_every_file_in_a_directory_corpus_root) {
    auto fixturesDir = std::filesystem::path(FIXTURE_CHORALE("chor029")).parent_path();
    CorpusSearch search(fixturesDir);
    Query q;
    q.feature = "kern";
    q.pattern = {AttributeMap{{"fermata", {"true"}}}};
    q.voices = "soprano";
    auto results = search.run(q);

    CHECK_EQ(results.size(), std::size_t{51}); // 6 fermatas x 5 fixture chorales, plus chor005's 8, chor006's 4, chor008's 9
    std::vector<std::string> ids;
    for (const auto& r : results) ids.push_back(r.choraleId);
    std::sort(ids.begin(), ids.end());
    ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
    CHECK_EQ(ids, (std::vector<std::string>{"chor001", "chor005", "chor006", "chor008", "chor009", "chor029",
                                            "chor039", "chor103"}));
}

// The parallel fifths downwards of chor008, between every pair of voices: both voices of the pair move down into
// the interval the pair has again. A query over all voice pairs (hint-**) is the query of each pair, the pair
// named in the pattern (hint-@pair), in the voices and in the groups, and each result has its pair as feature.
TEST_CASE(a_query_over_all_voice_pairs_finds_what_a_query_for_each_pair_finds_and_says_which_pair) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor008"));
    CorpusSearch search(chorale.path());

    Query all;
    all.feature = "hint-**";
    all.voices = "@upper";
    all.hintReduceCompound = true;
    all.pattern = {AttributeMap{{"hint-@pair", {"P5"}}}, AttributeMap{{"hint-@pair", {"P5"}}}};
    for (const char* voice : {"@lower", "@upper"}) {
        SimultaneousGroup group;
        group.feature = "mint";
        group.voices = voice;
        group.pattern = {AttributeMap{{"!kern", {"r"}}}, AttributeMap{{"mint", {"-"}}}};
        group.simultaneousAlignment = "end";
        all.simultaneousWith.push_back(group);
    }
    const auto found = search.runOne(chorale, all);
    REQUIRE(!found.empty());
    // The notes of the parallel: a section of each of the two voices of the pair, as its group found it, from the
    // line it attacked its first note on, which can be before the sonority the match starts in, to the line it
    // moved on.
    for (const Result& r : found) {
        REQUIRE(r.linkedSections.size() == 2u);
        std::set<std::size_t> voices;
        for (const NoteSection& section : r.linkedSections) {
            voices.insert(section.voice);
            CHECK(section.startLineNumber <= r.startLineNumber);
            CHECK_EQ(section.endLineNumber, r.endLineNumber);
        }
        CHECK(voices == (std::set<std::size_t>{static_cast<std::size_t>(r.feature[5] - '0'),
                                               static_cast<std::size_t>(r.feature[6] - '0')}));
    }
    CHECK(std::is_sorted(found.begin(), found.end(),
                         [](const Result& a, const Result& b) { return a.startLineNumber < b.startLineNumber; }));

    std::size_t expected = 0;
    for (const std::string pair : {"12", "13", "14", "23", "24", "34"}) {
        Query one = all;
        one.feature = "hint-" + pair;
        one.voices = pair.substr(1);
        one.pattern = {AttributeMap{{one.feature, {"P5"}}}, AttributeMap{{one.feature, {"P5"}}}};
        one.simultaneousWith[0].voices = pair.substr(0, 1);
        one.simultaneousWith[1].voices = pair.substr(1);
        const auto expectedForPair = search.runOne(chorale, one);
        const auto foundForPair = std::count_if(found.begin(), found.end(),
                                                [&](const Result& r) { return r.feature == one.feature; });
        CHECK_EQ(static_cast<std::size_t>(foundForPair), expectedForPair.size());
        expected += expectedForPair.size();
    }
    CHECK_EQ(found.size(), expected);
}

TEST_CASE(a_query_over_a_set_of_voice_pairs_only_searches_the_pairs_of_the_set_and_honours_the_limit) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor008"));
    CorpusSearch search(chorale.path());

    Query withSoprano;
    withSoprano.feature = "hint-*4";
    withSoprano.voices = "@upper";
    withSoprano.pattern = {AttributeMap{{"hint-@pair", {"P8"}}}};
    const auto found = search.runOne(chorale, withSoprano);
    REQUIRE(found.size() > 2u);
    for (const Result& r : found) {
        CHECK(r.feature == "hint-14" || r.feature == "hint-24" || r.feature == "hint-34");
    }

    withSoprano.limit = 2;
    CHECK_EQ(search.runOne(chorale, withSoprano).size(), std::size_t{2});
}

// A star stands for any voice in its place, the lower voice of a pair first: hint-2* is the tenor with a voice above
// (23, 24), hint-*2 with the bass below (12), and a set that has no pair (hint-4*, the soprano below another voice)
// finds nothing.
TEST_CASE(a_set_of_voice_pairs_is_the_pairs_whose_lower_and_upper_voice_match_the_digits) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor008"));
    CorpusSearch search(chorale.path());

    for (const auto& [set, pairs] : std::vector<std::pair<std::string, std::set<std::string>>>{
             {"hint-2*", {"hint-23", "hint-24"}}, {"hint-*2", {"hint-12"}}, {"hint-1*", {"hint-12", "hint-13", "hint-14"}},
             {"hint-4*", {}}}) {
        Query query;
        query.feature = set;
        query.voices = "@upper";
        query.pattern = {AttributeMap{{"hint-@pair", {"*"}}}};
        std::set<std::string> found;
        for (const Result& r : search.runOne(chorale, query)) found.insert(r.feature);
        CHECK(found == pairs);
    }
}

// The spine of a pair is the same whichever voice reads it: a query over all pairs that names no voice finds each
// interval once.
TEST_CASE(a_query_over_all_voice_pairs_that_names_no_voice_finds_each_interval_once) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor008"));
    CorpusSearch search(chorale.path());

    Query upper;
    upper.feature = "hint-**";
    upper.voices = "@upper";
    upper.pattern = {AttributeMap{{"hint-@pair", {"P8"}}}};
    Query all = upper;
    all.voices = "all";
    const auto expected = search.runOne(chorale, upper);
    REQUIRE(!expected.empty());
    CHECK_EQ(search.runOne(chorale, all).size(), expected.size());
}

// A query over voice pairs that reads all voices reads each pair once, however all voices are written. A set in the
// pattern stays any of its pairs: a fifth of any pair, wherever the soprano sounds with another voice, is found more
// often than a fifth of the pair searched.
TEST_CASE(a_query_over_voice_pairs_reads_each_pair_once_and_keeps_another_set_any_of_its_pairs) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor008"));
    CorpusSearch search(chorale.path());

    Query query;
    query.feature = "hint-**";
    query.pattern = {AttributeMap{{"hint-@pair", {"P8"}}}};
    const auto once = search.runOne(chorale, query);
    REQUIRE(!once.empty());
    for (const char* voices : {"ALL", "", "1234"}) {
        query.voices = voices;
        CHECK_EQ(search.runOne(chorale, query).size(), once.size());
    }

    Query anyPair;
    anyPair.feature = "hint-*4";
    anyPair.pattern = {AttributeMap{{"hint-@pair", {"*"}}, {"hint-**", {"P5"}}}};
    Query ownPair = anyPair;
    ownPair.pattern = {AttributeMap{{"hint-@pair", {"P5"}}}};
    CHECK(search.runOne(chorale, anyPair).size() > search.runOne(chorale, ownPair).size());
}

// Over a corpus, a query over voice pairs keeps to the chorales of its filter and to its limit.
TEST_CASE(run_searches_the_voice_pairs_of_the_chorales_of_the_filter_up_to_the_limit) {
    auto fixturesDir = std::filesystem::path(FIXTURE_CHORALE("chor029")).parent_path();
    CorpusSearch search(fixturesDir);

    Query query;
    query.feature = "hint-**";
    query.pattern = {AttributeMap{{"hint-@pair", {"P8"}}}};
    query.choraleFilter.ids = {ChoraleIdRange{"8", 8, 8}};
    const auto found = search.run(query);
    REQUIRE(!found.empty());
    for (const Result& r : found) CHECK_EQ(r.choraleId, std::string("chor008"));
    HumdrumChorale chorale(FIXTURE_CHORALE("chor008"));
    CHECK_EQ(found.size(), search.runOne(chorale, query).size());

    query.choraleFilter = {};
    query.limit = 3;
    CHECK_EQ(search.run(query).size(), std::size_t{3});
}

// A result links the sections of notes it is made of, so the page can mark them.
TEST_CASE(a_result_links_the_section_of_the_voice_searched_and_the_sections_of_its_simultaneous_groups) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    CorpusSearch search(chorale.path());

    Query fermatas;
    fermatas.feature = "kern";
    fermatas.voices = "soprano";
    fermatas.pattern = {AttributeMap{{"fermata", {"true"}}}};
    for (const Result& r : search.runOne(chorale, fermatas)) {
        REQUIRE(r.linkedSections.size() == 1u);
        CHECK_EQ(r.linkedSections[0].feature, std::string("kern"));
        CHECK_EQ(r.linkedSections[0].voice, std::size_t{4});
        CHECK_EQ(r.linkedSections[0].startLineNumber, r.startLineNumber);
        CHECK_EQ(r.linkedSections[0].endLineNumber, r.startLineNumber);
        CHECK_EQ(r.linkedSections[0].startPosition, r.startPosition);
    }

    // With the bass under it: the note the bass attacks on the same line.
    SimultaneousGroup bass;
    bass.feature = "kern";
    bass.voices = "bass";
    bass.pattern = {AttributeMap{{"kern", {"*"}}}};
    fermatas.simultaneousWith = {bass};
    const auto withBass = search.runOne(chorale, fermatas);
    REQUIRE(!withBass.empty());
    for (const Result& r : withBass) {
        REQUIRE(r.linkedSections.size() == 2u);
        CHECK_EQ(r.linkedSections[0].voice, std::size_t{1});
        CHECK_EQ(r.linkedSections[1].voice, std::size_t{4});
        CHECK_EQ(r.linkedSections[0].startLineNumber, r.startLineNumber);
        CHECK_EQ(r.linkedSections[1].startLineNumber, r.startLineNumber);
    }
}

TEST_CASE(a_result_of_a_pair_spine_or_a_hint_voice_key_links_the_voices_of_the_pair) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    CorpusSearch search(chorale.path());

    Query pair;
    pair.feature = "hint-14";
    pair.voices = "bass";
    pair.pattern = {AttributeMap{{"hint-14", {"*"}}}, AttributeMap{{"hint-14", {"*"}}}};
    const auto found = search.runOne(chorale, pair);
    REQUIRE(!found.empty());
    for (const Result& r : found) {
        std::set<std::size_t> linked;
        for (const NoteSection& section : r.linkedSections) linked.insert(section.voice);
        CHECK(linked == (std::set<std::size_t>{1, 4}));
        // A section of each voice, over the whole match.
        REQUIRE(r.linkedSections.size() == 2u);
        for (const NoteSection& section : r.linkedSections) {
            CHECK_EQ(section.feature, std::string("hint-14"));
            CHECK_EQ(section.startLineNumber, r.startLineNumber);
            CHECK_EQ(section.endLineNumber, r.endLineNumber);
        }
    }

    // The alto with the tenor: the walked voice and the voice of the key.
    Query key;
    key.feature = "kern";
    key.voices = "alto";
    key.pattern = {AttributeMap{{"hint-2", {"*"}}}};
    const auto withTenor = search.runOne(chorale, key);
    REQUIRE(!withTenor.empty());
    for (const Result& r : withTenor) {
        REQUIRE(r.linkedSections.size() == 2u);
        CHECK_EQ(r.linkedSections[0].voice, std::size_t{2});
        CHECK_EQ(r.linkedSections[0].feature, std::string("hint-2"));
        CHECK_EQ(r.linkedSections[1].voice, std::size_t{3});
        CHECK_EQ(r.linkedSections[1].feature, std::string("kern"));
    }
}

// The soprano is linked at the positions that name it, not at the position in between: three notes of the alto, the
// soprano's octave on the first and the last.
TEST_CASE(a_hint_voice_key_links_the_other_voice_at_the_positions_that_name_it_only) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    CorpusSearch search(chorale.path());

    Query query;
    query.feature = "kern";
    query.voices = "alto";
    query.pattern = {AttributeMap{{"kern", {"*"}}, {"hint-4", {"*"}}}, AttributeMap{{"kern", {"*"}}},
                     AttributeMap{{"kern", {"*"}}, {"hint-4", {"*"}}}};
    const auto found = search.runOne(chorale, query);
    REQUIRE(found.size() > 2u);
    for (const Result& r : found) {
        std::vector<NoteSection> alto;
        std::vector<NoteSection> soprano;
        for (const NoteSection& section : r.linkedSections) (section.voice == 3 ? alto : soprano).push_back(section);
        REQUIRE(alto.size() == 1u);
        REQUIRE(soprano.size() == 2u);
        CHECK_EQ(alto[0].startLineNumber, r.startLineNumber);
        CHECK_EQ(alto[0].endLineNumber, r.endLineNumber);
        CHECK_EQ(soprano[0].feature, std::string("hint-4"));
        CHECK_EQ(soprano[0].startLineNumber, r.startLineNumber);
        CHECK_EQ(soprano[0].endLineNumber, r.startLineNumber);
        CHECK_EQ(soprano[1].startLineNumber, r.endLineNumber);
        CHECK_EQ(soprano[1].endLineNumber, r.endLineNumber);
    }
}

// A group links the notes it found by its own rules: both voices of a pair spine, the voice of a hint-<voice> key, and
// every voice it matched in.
TEST_CASE(a_group_links_the_voices_of_its_pair_its_hint_keys_and_every_voice_it_matched_in) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    CorpusSearch search(chorale.path());

    Query fermatas;
    fermatas.feature = "kern";
    fermatas.voices = "soprano";
    fermatas.pattern = {AttributeMap{{"fermata", {"true"}}}};

    SimultaneousGroup pair;
    pair.feature = "hint-23";
    pair.voices = "alto";
    pair.pattern = {AttributeMap{{"hint-23", {"*"}}}};
    Query withPair = fermatas;
    withPair.simultaneousWith = {pair};
    const auto foundWithPair = search.runOne(chorale, withPair);
    REQUIRE(!foundWithPair.empty());
    for (const Result& r : foundWithPair) {
        std::set<std::size_t> linked;
        for (const NoteSection& section : r.linkedSections) {
            if (section.feature == "hint-23") linked.insert(section.voice);
        }
        CHECK(linked == (std::set<std::size_t>{2, 3}));
    }

    SimultaneousGroup key;
    key.feature = "kern";
    key.voices = "tenor";
    key.pattern = {AttributeMap{{"hint-1", {"*"}}}};
    Query withKey = fermatas;
    withKey.simultaneousWith = {key};
    const auto foundWithKey = search.runOne(chorale, withKey);
    REQUIRE(!foundWithKey.empty());
    for (const Result& r : foundWithKey) {
        std::set<std::size_t> linked;
        for (const NoteSection& section : r.linkedSections) linked.insert(section.voice);
        CHECK(linked == (std::set<std::size_t>{1, 2, 4}));
    }

    SimultaneousGroup everyVoice;
    everyVoice.feature = "kern";
    everyVoice.voices = "all";
    everyVoice.pattern = {AttributeMap{{"kern", {"*"}}}};
    Query withEveryVoice = fermatas;
    withEveryVoice.simultaneousWith = {everyVoice};
    const auto foundWithEveryVoice = search.runOne(chorale, withEveryVoice);
    REQUIRE(!foundWithEveryVoice.empty());
    std::size_t complete = 0;
    for (const Result& r : foundWithEveryVoice) {
        std::set<std::size_t> linked;
        for (const NoteSection& section : r.linkedSections) linked.insert(section.voice);
        if (linked == std::set<std::size_t>{1, 2, 3, 4}) ++complete;
    }
    CHECK(complete > 0u);
}

// A melodic interval that starts at the note before begins its section at that note, in the voice searched and in a
// group alike.
TEST_CASE(a_melodic_interval_from_the_note_before_begins_its_section_at_that_note) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    CorpusSearch search(chorale.path());

    Query query;
    query.feature = "mint";
    query.voices = "soprano";
    query.mintStartAtPreviousToken = true;
    query.pattern = {AttributeMap{{"mint", {"+"}}}};
    SimultaneousGroup bass;
    bass.feature = "mint";
    bass.voices = "bass";
    bass.pattern = {AttributeMap{{"mint", {"-"}}}};
    bass.simultaneousAlignment = "end";
    query.simultaneousWith = {bass};
    const auto found = search.runOne(chorale, query);
    REQUIRE(!found.empty());
    for (const Result& r : found) {
        REQUIRE(r.linkedSections.size() == 2u);
        for (const NoteSection& section : r.linkedSections) {
            CHECK(section.startLineNumber < section.endLineNumber);
            CHECK_EQ(section.endLineNumber, r.endLineNumber);
            if (section.voice == 4) CHECK_EQ(section.startLineNumber, r.startLineNumber);
        }
    }
}

// An fb key links the voices that make its figure, at the position that names it: in chor029 the bass walks
// 3-4-5-1 into a fermata with a fourth above the 5, which is the alto's g over the D (line 110: D d g a).
TEST_CASE(an_fb_key_links_the_voices_that_make_its_figure_at_the_position_that_names_it) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    CorpusSearch search(chorale.path());

    Query query;
    query.feature = "kern";
    query.voices = "bass";
    query.pattern = {AttributeMap{{"deg", {"3"}}}, AttributeMap{{"deg", {"4"}}},
                     AttributeMap{{"deg", {"5"}}, {"fb", {"4"}}}, AttributeMap{{"deg", {"1"}}, {"fermata", {"true"}}}};
    const auto found = search.runOne(chorale, query);
    REQUIRE(!found.empty());
    bool foundTheAlto = false;
    for (const Result& r : found) {
        std::vector<NoteSection> figures;
        for (const NoteSection& section : r.linkedSections) {
            if (section.feature == "fb") figures.push_back(section);
        }
        REQUIRE(!figures.empty());
        for (const NoteSection& section : figures) {
            CHECK(section.voice != 1u);
            CHECK_EQ(section.startLineNumber, section.endLineNumber);
            CHECK(section.startLineNumber > r.startLineNumber);
            CHECK(section.startLineNumber < r.endLineNumber);
            if (section.voice == 3 && section.startLineNumber == 110) foundTheAlto = true;
        }
    }
    CHECK(foundTheAlto);

    // An fb key that names no figure links nothing.
    query.pattern[2] = AttributeMap{{"deg", {"5"}}, {"fb", {"*"}}};
    for (const Result& r : search.runOne(chorale, query)) {
        for (const NoteSection& section : r.linkedSections) CHECK(section.feature != std::string("fb"));
    }
}

// An fb key in a simultaneousWith entry driven by fb links the voices of its figures, but covers none of them: the
// alto searched keeps its own section where it forms the figure.
TEST_CASE(an_fb_key_of_a_simultaneous_entry_covers_no_voice) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    CorpusSearch search(chorale.path());

    Query query;
    query.feature = "kern";
    query.voices = "alto";
    query.pattern = {AttributeMap{{"kern", {"*"}}}};
    SimultaneousGroup figures;
    figures.feature = "fb";
    figures.voices = "bass";
    figures.pattern = {AttributeMap{{"fb", {"4"}}}};
    query.simultaneousWith = {figures};
    const auto found = search.runOne(chorale, query);
    REQUIRE(!found.empty());
    for (const Result& r : found) {
        const bool ownSection = std::any_of(r.linkedSections.begin(), r.linkedSections.end(), [](const NoteSection& s) {
            return s.voice == 3 && s.feature == "kern";
        });
        CHECK(ownSection);
    }
}

// A hint-<pair> key links both voices of the pair at the position that names it: the alto's notes, the outer voices
// an octave apart on the first.
TEST_CASE(a_hint_pair_key_links_both_voices_of_the_pair_at_the_position_that_names_it) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    CorpusSearch search(chorale.path());

    Query query;
    query.feature = "kern";
    query.voices = "alto";
    query.pattern = {AttributeMap{{"kern", {"*"}}, {"hint-14", {"*"}}}, AttributeMap{{"kern", {"*"}}}};
    const auto found = search.runOne(chorale, query);
    REQUIRE(!found.empty());
    for (const Result& r : found) {
        std::set<std::size_t> pairVoices;
        for (const NoteSection& section : r.linkedSections) {
            if (section.feature != "hint-14") continue;
            pairVoices.insert(section.voice);
            CHECK_EQ(section.startLineNumber, r.startLineNumber);
            CHECK_EQ(section.endLineNumber, r.startLineNumber);
        }
        CHECK(pairVoices == (std::set<std::size_t>{1, 4}));
    }

    // A set of pairs names no voices.
    query.pattern[0] = AttributeMap{{"kern", {"*"}}, {"hint-*4", {"*"}}};
    for (const Result& r : search.runOne(chorale, query)) {
        for (const NoteSection& section : r.linkedSections) CHECK_EQ(section.voice, std::size_t{3});
    }
}

TEST_CASE(run_stops_at_the_limit_partway_through_a_later_file) {
    // findChoraleFiles() sorts, so chor001 < chor005 < chor006 < chor008 < chor009 < chor029.
    // chor001 alone has 6 soprano fermatas, so a limit of 8 must exhaust chor001 and then take
    // 2 more from chor005 -- exercising the cross-file accumulate-then-truncate logic that's
    // unique to CorpusSearch::run, not something a single-file test (or AttributeMatcher) can show.
    auto fixturesDir = std::filesystem::path(FIXTURE_CHORALE("chor029")).parent_path();
    CorpusSearch search(fixturesDir);
    Query q;
    q.feature = "kern";
    q.pattern = {AttributeMap{{"fermata", {"true"}}}};
    q.voices = "soprano";
    q.limit = 8;
    auto results = search.run(q);

    REQUIRE(results.size() == 8u);
    for (std::size_t i = 0; i < 6; ++i) CHECK_EQ(results[i].choraleId, std::string("chor001"));
    for (std::size_t i = 6; i < 8; ++i) CHECK_EQ(results[i].choraleId, std::string("chor005"));
}

TEST_CASE(run_with_a_single_query_never_sets_query_id) {
    CorpusSearch search(FIXTURE_CHORALE("chor029"));
    Query q;
    q.feature = "kern";
    q.pattern = {AttributeMap{{"fermata", {"true"}}}};
    q.voices = "soprano";
    auto results = search.run(q); // the plain Query overload, not the vector<Query> one

    REQUIRE(!results.empty());
    for (const auto& r : results) CHECK(!r.queryId.has_value());
}

TEST_CASE(run_with_multiple_queries_tags_each_result_with_its_own_id_or_index) {
    auto fixturesDir = std::filesystem::path(FIXTURE_CHORALE("chor029")).parent_path();
    CorpusSearch search(fixturesDir);

    Query withId;
    withId.id = "sopranoFermatas";
    withId.feature = "kern";
    withId.pattern = {AttributeMap{{"fermata", {"true"}}}};
    withId.voices = "soprano";

    Query withoutId;
    withoutId.feature = "kern";
    withoutId.pattern = {AttributeMap{{"fermata", {"true"}}}};
    withoutId.voices = "bass";

    auto results = search.run(std::vector<Query>{withId, withoutId});

    std::size_t taggedWithId = 0;
    std::size_t taggedWithIndex = 0;
    for (const auto& r : results) {
        REQUIRE(r.queryId.has_value());
        if (*r.queryId == "sopranoFermatas") ++taggedWithId;
        else if (*r.queryId == "1") ++taggedWithIndex;
    }
    CHECK_EQ(taggedWithId, std::size_t{51});
    CHECK(taggedWithIndex > 0u);
    CHECK_EQ(taggedWithId + taggedWithIndex, results.size());
}

TEST_CASE(run_with_multiple_queries_respects_each_querys_own_limit_independently) {
    auto fixturesDir = std::filesystem::path(FIXTURE_CHORALE("chor029")).parent_path();
    CorpusSearch search(fixturesDir);

    Query capped3;
    capped3.id = "capped3";
    capped3.feature = "kern";
    capped3.pattern = {AttributeMap{{"fermata", {"true"}}}};
    capped3.voices = "soprano";
    capped3.limit = 3;

    Query capped5;
    capped5.id = "capped5";
    capped5.feature = "kern";
    capped5.pattern = {AttributeMap{{"fermata", {"true"}}}};
    capped5.voices = "soprano";
    capped5.limit = 5;

    auto results = search.run(std::vector<Query>{capped3, capped5});

    std::size_t count3 = std::count_if(results.begin(), results.end(),
                                        [](const Result& r) { return *r.queryId == "capped3"; });
    std::size_t count5 = std::count_if(results.begin(), results.end(),
                                        [](const Result& r) { return *r.queryId == "capped5"; });
    CHECK_EQ(count3, std::size_t{3});
    CHECK_EQ(count5, std::size_t{5});
    CHECK_EQ(results.size(), std::size_t{8});
}

TEST_CASE(run_throws_when_the_corpus_root_does_not_exist) {
    CorpusSearch search("/nonexistent/corpus/root");
    Query q;
    q.feature = "kern";
    q.pattern = {AttributeMap{{"kern", {"*"}}}};
    CHECK_THROWS(search.run(q));
}

TEST_CASE(simultaneous_with_default_alignment_keeps_matches_that_align_with_the_group) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    CorpusSearch search(chorale.path());
    Query q;
    q.feature = "kern";
    q.pattern = {AttributeMap{{"fermata", {"true"}}}};
    q.voices = "soprano";

    SimultaneousGroup group;
    group.feature = "kern";
    group.voices = "bass";
    group.pattern = {AttributeMap{{"fermata", {"true"}}}};
    q.simultaneousWith = {group};

    auto results = search.runOne(chorale, q);
    CHECK_EQ(results.size(), std::size_t{6}); // same 6 as without the constraint
}

TEST_CASE(simultaneous_with_group_that_never_matches_filters_out_every_result) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    CorpusSearch search(chorale.path());
    Query q;
    q.feature = "kern";
    q.pattern = {AttributeMap{{"fermata", {"true"}}}};
    q.voices = "soprano";

    SimultaneousGroup group;
    group.feature = "kern";
    group.voices = "alto";
    group.pattern = {AttributeMap{{"kern", {"r"}}}}; // no rests anywhere in this chorale
    q.simultaneousWith = {group};

    auto results = search.runOne(chorale, q);
    CHECK_EQ(results.size(), std::size_t{0});
}

TEST_CASE(simultaneous_with_requires_every_group_to_have_a_match) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    CorpusSearch search(chorale.path());
    Query q;
    q.feature = "kern";
    q.pattern = {AttributeMap{{"fermata", {"true"}}}};
    q.voices = "soprano";

    SimultaneousGroup alwaysAligns;
    alwaysAligns.feature = "kern";
    alwaysAligns.voices = "bass";
    alwaysAligns.pattern = {AttributeMap{{"fermata", {"true"}}}};

    SimultaneousGroup neverMatches;
    neverMatches.feature = "kern";
    neverMatches.voices = "alto";
    neverMatches.pattern = {AttributeMap{{"kern", {"r"}}}};

    q.simultaneousWith = {alwaysAligns, neverMatches};
    CHECK_EQ(search.runOne(chorale, q).size(), std::size_t{0}); // the second group has no match

    q.simultaneousWith = {alwaysAligns};
    CHECK_EQ(search.runOne(chorale, q).size(), std::size_t{6}); // only the aligning group remains
}

TEST_CASE(simultaneous_alignment_start_only_checks_the_matchs_start_position) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    CorpusSearch search(chorale.path());
    Query q;
    q.feature = "kern";
    q.voices = "soprano";
    q.pattern = {AttributeMap{{"kern", {"g"}}}, AttributeMap{{"kern", {"a"}}}};
    q.simultaneousAlignment = "start"; // default, set explicitly for clarity

    SimultaneousGroup group;
    group.feature = "kern";
    group.voices = "bass";
    group.pattern = {AttributeMap{{"kern", {"G"}}}};
    q.simultaneousWith = {group};

    auto results = search.runOne(chorale, q);
    REQUIRE(results.size() == 1u);
    CHECK_EQ(results[0].startPosition, std::string("0"));
}

TEST_CASE(simultaneous_alignment_end_only_checks_the_matchs_end_position) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    CorpusSearch search(chorale.path());
    Query q;
    q.feature = "kern";
    q.voices = "soprano";
    q.pattern = {AttributeMap{{"kern", {"g"}}}, AttributeMap{{"kern", {"a"}}}};
    q.simultaneousAlignment = "end";

    SimultaneousGroup group;
    group.feature = "kern";
    group.voices = "bass";
    group.pattern = {AttributeMap{{"kern", {"F#"}}}};
    q.simultaneousWith = {group};

    auto results = search.runOne(chorale, q);
    REQUIRE(results.size() == 2u);
    CHECK_EQ(results[0].startPosition, std::string("0"));
    CHECK_EQ(results[1].startPosition, std::string("8"));
}

TEST_CASE(simultaneous_alignment_start_end_requires_both_positions_at_once) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    CorpusSearch search(chorale.path());
    Query q;
    q.feature = "kern";
    q.voices = "soprano";
    q.pattern = {AttributeMap{{"kern", {"g"}}}, AttributeMap{{"kern", {"a"}}}};
    q.simultaneousAlignment = "start-end";

    SimultaneousGroup group;
    group.feature = "kern";
    group.voices = "bass";
    group.pattern = {AttributeMap{{"kern", {"G"}}}};
    q.simultaneousWith = {group};
    CHECK_EQ(search.runOne(chorale, q).size(), std::size_t{0});

    group.pattern = {AttributeMap{{"kern", {"F#"}}}};
    q.simultaneousWith = {group};
    CHECK_EQ(search.runOne(chorale, q).size(), std::size_t{0});

    group.pattern = {AttributeMap{{"kern", {"G"}}}, AttributeMap{{"kern", {"F#"}}}};
    q.simultaneousWith = {group};
    CHECK_EQ(search.runOne(chorale, q).size(), std::size_t{1});
}

TEST_CASE(simultaneous_alignment_start_end_matches_when_both_patterns_are_single_position) {
    // Every fermata in this chorale is single-position, so its start and end position are
    // identical -- "start-end" behaves exactly like the "start" default here.
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    CorpusSearch search(chorale.path());
    Query q;
    q.feature = "kern";
    q.pattern = {AttributeMap{{"fermata", {"true"}}}};
    q.voices = "soprano";
    q.simultaneousAlignment = "start-end";

    SimultaneousGroup group;
    group.feature = "kern";
    group.voices = "bass";
    group.pattern = {AttributeMap{{"fermata", {"true"}}}};
    q.simultaneousWith = {group};

    CHECK_EQ(search.runOne(chorale, q).size(), std::size_t{6});
}

TEST_CASE(simultaneous_with_group_can_override_the_querys_simultaneous_alignment) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    CorpusSearch search(chorale.path());
    Query q;
    q.feature = "kern";
    q.voices = "soprano";
    q.pattern = {AttributeMap{{"kern", {"g"}}}, AttributeMap{{"kern", {"a"}}}};
    q.simultaneousAlignment = "end"; // would find nothing against bass "G" (see test above)

    SimultaneousGroup group;
    group.feature = "kern";
    group.voices = "bass";
    group.pattern = {AttributeMap{{"kern", {"G"}}}};
    group.simultaneousAlignment = "start"; // override back to "start" for this group only
    q.simultaneousWith = {group};

    auto results = search.runOne(chorale, q);
    REQUIRE(results.size() == 1u);
    CHECK_EQ(results[0].startPosition, std::string("0"));
}

TEST_CASE(simultaneous_with_group_can_override_kern_ignore_octave) {
    // The bass never has a literal lowercase "g" (specific octave) in this chorale, only
    // uppercase "G" -- so the group's pattern only matches once kernIgnoreOctave folds them
    // together, whether that comes from the group's own override or an inherited query value.
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    CorpusSearch search(chorale.path());
    Query q;
    q.feature = "kern";
    q.voices = "soprano";
    q.pattern = {AttributeMap{{"kern", {"g"}}}, AttributeMap{{"kern", {"a"}}}};

    SimultaneousGroup group;
    group.feature = "kern";
    group.voices = "bass";
    group.pattern = {AttributeMap{{"kern", {"g"}}}};
    q.simultaneousWith = {group};
    CHECK_EQ(search.runOne(chorale, q).size(), std::size_t{0}); // neither overridden nor inherited

    q.simultaneousWith[0].kernIgnoreOctave = true; // group-level override
    CHECK_EQ(search.runOne(chorale, q).size(), std::size_t{1});

    q.simultaneousWith[0].kernIgnoreOctave.reset();
    q.kernIgnoreOctave = true; // query-level, inherited by the group
    CHECK_EQ(search.runOne(chorale, q).size(), std::size_t{1});
}

TEST_CASE(simultaneous_with_group_can_override_hint_reduce_compound) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    CorpusSearch search(chorale.path());
    Query q;
    q.feature = "hint-14";
    q.voices = "1";
    q.pattern = {AttributeMap{{"hint-14", {"*"}}}};

    SimultaneousGroup group;
    group.feature = "hint-14";
    group.voices = "1";
    group.pattern = {AttributeMap{{"hint-14", {"M3"}}}};
    q.simultaneousWith = {group};
    CHECK_EQ(search.runOne(chorale, q).size(), std::size_t{0}); // neither overridden nor inherited

    q.simultaneousWith[0].hintReduceCompound = true; // group-level override
    CHECK_EQ(search.runOne(chorale, q).size(), std::size_t{17});

    q.simultaneousWith[0].hintReduceCompound.reset();
    q.hintReduceCompound = true; // query-level, inherited by the group
    CHECK_EQ(search.runOne(chorale, q).size(), std::size_t{17});
}

TEST_CASE(simultaneous_with_group_can_override_duration_allow_split_notes) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor001"));
    CorpusSearch search(chorale.path());
    Query q;
    q.feature = "kern";
    q.voices = "3";
    q.pattern = {AttributeMap{{"kern", {"*"}}}};

    SimultaneousGroup group;
    group.feature = "kern";
    group.voices = "3";
    group.pattern = {AttributeMap{{"kern", {"d"}}, {"duration", {"2"}}}};
    q.simultaneousWith = {group};
    CHECK_EQ(search.runOne(chorale, q).size(), std::size_t{3}); // neither overridden nor inherited

    q.simultaneousWith[0].durationAllowSplitNotes = true; // group-level override
    CHECK_EQ(search.runOne(chorale, q).size(), std::size_t{4});

    q.simultaneousWith[0].durationAllowSplitNotes.reset();
    q.durationAllowSplitNotes = true; // query-level, inherited by the group
    CHECK_EQ(search.runOne(chorale, q).size(), std::size_t{4});
}

TEST_CASE(simultaneous_with_group_can_override_duration_allow_merged_notes) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor009"));
    CorpusSearch search(chorale.path());
    Query q;
    q.feature = "kern";
    q.voices = "4";
    q.pattern = {AttributeMap{{"kern", {"*"}}}};

    SimultaneousGroup group;
    group.feature = "kern";
    group.voices = "4";
    // The soprano's two cadences written as a half note a plus the fermata g, asked for as
    // three quarters.
    group.pattern = {AttributeMap{{"kern", {"a"}}, {"duration", {"4"}}},
                      AttributeMap{{"duration", {"4"}}},
                      AttributeMap{{"kern", {"g"}}, {"duration", {"4"}}, {"fermata", {"true"}}}};
    q.simultaneousWith = {group};
    CHECK_EQ(search.runOne(chorale, q).size(), std::size_t{0}); // neither overridden nor inherited

    q.simultaneousWith[0].durationAllowMergedNotes = true; // group-level override
    CHECK_EQ(search.runOne(chorale, q).size(), std::size_t{2});

    q.simultaneousWith[0].durationAllowMergedNotes.reset();
    q.durationAllowMergedNotes = true; // query-level, inherited by the group
    CHECK_EQ(search.runOne(chorale, q).size(), std::size_t{2});
}

TEST_CASE(simultaneous_with_group_can_override_metweight_skip_unclassified) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor005"));
    CorpusSearch search(chorale.path());
    Query q;
    q.feature = "kern";
    q.voices = "4";
    q.pattern = {AttributeMap{{"kern", {"*"}}}};

    SimultaneousGroup group;
    group.feature = "kern";
    group.voices = "4";
    // The soprano's bar-4 cadence with its offbeat neighbour note folded away, so the two a's
    // it decorates are adjacent -- see test_fixture_results.cpp.
    group.pattern = {AttributeMap{{"mint", {"*"}}, {"duration", {"4"}}},
                      AttributeMap{{"mint", {"P1"}}, {"duration", {"4"}}},
                      AttributeMap{{"mint", {"-M2"}}, {"duration", {"*"}}, {"fermata", {"true"}}}};
    q.simultaneousWith = {group};
    CHECK_EQ(search.runOne(chorale, q).size(), std::size_t{0}); // neither overridden nor inherited

    q.simultaneousWith[0].metweightSkipUnclassified = true; // group-level override
    CHECK_EQ(search.runOne(chorale, q).size(), std::size_t{1});

    q.simultaneousWith[0].metweightSkipUnclassified.reset();
    q.metweightSkipUnclassified = true; // query-level, inherited by the group
    CHECK_EQ(search.runOne(chorale, q).size(), std::size_t{1});
}

TEST_CASE(simultaneous_with_group_can_override_mint_allow_interval_complementation) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    CorpusSearch search(chorale.path());
    Query q;
    q.feature = "mint";
    q.voices = "1";
    q.pattern = {AttributeMap{{"mint", {"+4"}}}};

    SimultaneousGroup group;
    group.feature = "mint";
    group.voices = "1";
    group.pattern = {AttributeMap{{"mint", {"-5"}}}}; // never true where the primary "+4" is
    q.simultaneousWith = {group};
    CHECK_EQ(search.runOne(chorale, q).size(), std::size_t{0}); // neither overridden nor inherited
    
    q.simultaneousWith[0].mintAllowIntervalComplementation = {"5"}; // group-level override
    CHECK_EQ(search.runOne(chorale, q).size(), std::size_t{2});

    q.simultaneousWith[0].mintAllowIntervalComplementation.reset();
    q.mintAllowIntervalComplementation = {"5"}; // query-level, inherited by the group
    CHECK_EQ(search.runOne(chorale, q).size(), std::size_t{2});

    q.mintAllowIntervalComplementation = {};
    CHECK_EQ(search.runOne(chorale, q).size(), std::size_t{0});
}

TEST_CASE(for_each_match_visits_the_same_matches_run_collects_with_their_chorale) {
    auto fixturesDir = std::filesystem::path(FIXTURE_CHORALE("chor029")).parent_path();
    CorpusSearch search(fixturesDir);
    Query q;
    q.id = "fermatas";
    q.feature = "kern";
    q.pattern = {AttributeMap{{"fermata", {"true"}}}};
    q.voices = "soprano";

    std::size_t visited = 0;
    bool choraleMatchesResult = true;
    bool queryIdSet = true;
    search.forEachMatch({q}, [&](const HumdrumChorale& chorale, const Result& result) {
        ++visited;
        if (chorale.id() != result.choraleId) choraleMatchesResult = false;
        if (result.queryId != std::string("fermatas")) queryIdSet = false;
    });

    CHECK_EQ(visited, search.run(std::vector<Query>{q}).size());
    CHECK(visited > 0u);
    CHECK(choraleMatchesResult);
    CHECK(queryIdSet);
}

TEST_CASE(for_each_match_visits_one_file_at_a_time_and_in_file_order) {
    // The files are searched side by side, but `visit` is called for one match at a time, files in
    // order, with the chorale the match was found in.
    auto fixturesDir = std::filesystem::path(FIXTURE_CHORALE("chor029")).parent_path();
    CorpusSearch search(fixturesDir);
    Query q;
    q.feature = "kern";
    q.pattern = {AttributeMap{{"fermata", {"true"}}}};
    q.voices = "soprano";

    std::atomic<int> visiting{0};
    bool neverTwoAtOnce = true;
    bool choraleMatchesResult = true;
    std::vector<std::string> choraleIds;
    search.forEachMatch({q}, [&](const HumdrumChorale& chorale, const Result& result) {
        if (++visiting != 1) neverTwoAtOnce = false;
        if (chorale.id() != result.choraleId) choraleMatchesResult = false;
        choraleIds.push_back(result.choraleId);
        --visiting;
    });

    CHECK(neverTwoAtOnce);
    CHECK(choraleMatchesResult);
    CHECK_EQ(choraleIds.size(), std::size_t{51});
    CHECK(std::is_sorted(choraleIds.begin(), choraleIds.end()));
}

TEST_CASE(run_throws_for_a_file_that_cannot_be_parsed) {
    auto fixturesDir = std::filesystem::path(FIXTURE_CHORALE("chor029")).parent_path();
    auto corpus = std::filesystem::temp_directory_path() / "chorale-search-unparsable-corpus";
    std::filesystem::remove_all(corpus);
    std::filesystem::create_directories(corpus);
    for (const auto& entry : std::filesystem::directory_iterator(fixturesDir)) {
        if (entry.path().extension() == ".krn") std::filesystem::copy_file(entry.path(), corpus / entry.path().filename());
    }
    std::ofstream(corpus / "chor0050.krn") << "this is not humdrum\n";

    CorpusSearch search(corpus);
    Query q;
    q.feature = "kern";
    q.pattern = {AttributeMap{{"fermata", {"true"}}}};
    q.voices = "soprano";
    bool threw = false;
    try {
        minitest::SilenceStderr silence; // humlib writes what it cannot parse to stderr
        search.run(q);
    } catch (const std::exception&) {
        threw = true;
    }
    std::filesystem::remove_all(corpus);
    CHECK(threw);
}

TEST_CASE(chorale_filter_without_filters_searches_every_chorale) {
    CorpusSearch search(std::filesystem::path(FIXTURE_CHORALE("chor029")).parent_path());
    Query q;
    q.feature = "kern";
    q.pattern = {AttributeMap{{"fermata", {"true"}}}};
    q.voices = "soprano";
    std::vector<std::string> found; // the chorales with a match, in the order of the results

    found.clear();
    for (const Result& result : search.run(q)) {
        if (found.empty() || found.back() != result.choraleId) found.push_back(result.choraleId);
    }
    CHECK_EQ(found.size(), std::size_t{8});
    CHECK_EQ(search.run(q).size(), std::size_t{51});
}

TEST_CASE(chorale_filter_ids_and_excluded_ids_pick_the_chorales_searched) {
    CorpusSearch search(std::filesystem::path(FIXTURE_CHORALE("chor029")).parent_path());
    Query q;
    q.feature = "kern";
    q.pattern = {AttributeMap{{"fermata", {"true"}}}};
    q.voices = "soprano";
    std::vector<std::string> found; // the chorales with a match, in the order of the results

    q.choraleFilter.ids = {ChoraleIdRange{"29", 29, 29}, ChoraleIdRange{"chor039", 39, 39}};
    found.clear();
    for (const Result& result : search.run(q)) {
        if (found.empty() || found.back() != result.choraleId) found.push_back(result.choraleId);
    }
    CHECK_EQ(found, (std::vector<std::string>{"chor029", "chor039"}));

    q.choraleFilter = {};
    q.choraleFilter.ids = {ChoraleIdRange{"5-9", 5, 9}};
    q.choraleFilter.excludeIds = {ChoraleIdRange{"006", 6, 6}};
    found.clear();
    for (const Result& result : search.run(q)) {
        if (found.empty() || found.back() != result.choraleId) found.push_back(result.choraleId);
    }
    CHECK_EQ(found, (std::vector<std::string>{"chor005", "chor008", "chor009"}));

    q.choraleFilter = {};
    q.choraleFilter.excludeIds = {ChoraleIdRange{"1-100", 1, 100}};
    found.clear();
    for (const Result& result : search.run(q)) {
        if (found.empty() || found.back() != result.choraleId) found.push_back(result.choraleId);
    }
    CHECK_EQ(found, (std::vector<std::string>{"chor103"}));

    q.choraleFilter = {};
    q.choraleFilter.ids = {ChoraleIdRange{"2", 2, 2}};
    CHECK(search.run(q).empty());
}

TEST_CASE(chorale_filter_title_finds_a_chorale_by_a_word_of_either_title_without_regard_to_case) {
    CorpusSearch search(std::filesystem::path(FIXTURE_CHORALE("chor029")).parent_path());
    Query q;
    q.feature = "kern";
    q.pattern = {AttributeMap{{"fermata", {"true"}}}};
    q.voices = "soprano";
    std::vector<std::string> found; // the chorales with a match, in the order of the results

    // chor103 is "Nun ruhen alle W&auml;lder" in the file
    q.choraleFilter.title = {"W\xC3\x84LDER"};
    found.clear();
    for (const Result& result : search.run(q)) {
        if (found.empty() || found.back() != result.choraleId) found.push_back(result.choraleId);
    }
    CHECK_EQ(found, (std::vector<std::string>{"chor103"}));

    q.choraleFilter.title = {"depths of my"};
    found.clear();
    for (const Result& result : search.run(q)) {
        if (found.empty() || found.back() != result.choraleId) found.push_back(result.choraleId);
    }
    CHECK_EQ(found, (std::vector<std::string>{"chor001"}));

    q.choraleFilter.title = {"babylon", "S\xC3\xBCnder"};
    found.clear();
    for (const Result& result : search.run(q)) {
        if (found.empty() || found.back() != result.choraleId) found.push_back(result.choraleId);
    }
    CHECK_EQ(found, (std::vector<std::string>{"chor005", "chor039"}));
}

TEST_CASE(chorale_filter_meter_and_counts_pick_the_chorales_searched) {
    CorpusSearch search(std::filesystem::path(FIXTURE_CHORALE("chor029")).parent_path());
    Query q;
    q.feature = "kern";
    q.pattern = {AttributeMap{{"fermata", {"true"}}}};
    q.voices = "soprano";
    std::vector<std::string> found; // the chorales with a match, in the order of the results

    q.choraleFilter.meter = {"3/4", "2/2"};
    found.clear();
    for (const Result& result : search.run(q)) {
        if (found.empty() || found.back() != result.choraleId) found.push_back(result.choraleId);
    }
    CHECK_EQ(found, (std::vector<std::string>{"chor001"}));

    q.choraleFilter = {};
    q.choraleFilter.measureCount = CountRange{17, std::nullopt};
    found.clear();
    for (const Result& result : search.run(q)) {
        if (found.empty() || found.back() != result.choraleId) found.push_back(result.choraleId);
    }
    CHECK_EQ(found, (std::vector<std::string>{"chor001", "chor005", "chor008"}));

    q.choraleFilter.measureCount = CountRange{std::nullopt, 8};
    found.clear();
    for (const Result& result : search.run(q)) {
        if (found.empty() || found.back() != result.choraleId) found.push_back(result.choraleId);
    }
    CHECK_EQ(found, (std::vector<std::string>{"chor006"}));

    q.choraleFilter.measureCount = CountRange{13, 13};
    found.clear();
    for (const Result& result : search.run(q)) {
        if (found.empty() || found.back() != result.choraleId) found.push_back(result.choraleId);
    }
    CHECK_EQ(found, (std::vector<std::string>{"chor029"}));

    q.choraleFilter = {};
    q.choraleFilter.phraseCount = CountRange{8, std::nullopt};
    found.clear();
    for (const Result& result : search.run(q)) {
        if (found.empty() || found.back() != result.choraleId) found.push_back(result.choraleId);
    }
    CHECK_EQ(found, (std::vector<std::string>{"chor005", "chor008"}));

    q.choraleFilter.phraseCount = CountRange{4, 4};
    found.clear();
    for (const Result& result : search.run(q)) {
        if (found.empty() || found.back() != result.choraleId) found.push_back(result.choraleId);
    }
    CHECK_EQ(found, (std::vector<std::string>{"chor006"}));
}

TEST_CASE(chorale_filter_key_and_mode_are_those_a_chorale_starts_in) {
    CorpusSearch search(std::filesystem::path(FIXTURE_CHORALE("chor029")).parent_path());
    Query q;
    q.feature = "kern";
    q.pattern = {AttributeMap{{"fermata", {"true"}}}};
    q.voices = "soprano";
    std::vector<std::string> found; // the chorales with a match, in the order of the results

    // chor001 modulates to e minor, but starts in G major
    q.choraleFilter.startKey = {"G"};
    found.clear();
    for (const Result& result : search.run(q)) {
        if (found.empty() || found.back() != result.choraleId) found.push_back(result.choraleId);
    }
    CHECK_EQ(found, (std::vector<std::string>{"chor001", "chor005", "chor009", "chor029"}));

    q.choraleFilter = {};
    q.choraleFilter.startMode = {"minor"};
    found.clear();
    for (const Result& result : search.run(q)) {
        if (found.empty() || found.back() != result.choraleId) found.push_back(result.choraleId);
    }
    CHECK_EQ(found, (std::vector<std::string>{"chor039"}));

    q.choraleFilter.startMode = {"dorian"};
    found.clear();
    for (const Result& result : search.run(q)) {
        if (found.empty() || found.back() != result.choraleId) found.push_back(result.choraleId);
    }
    CHECK_EQ(found, (std::vector<std::string>{"chor008"}));

    q.choraleFilter.startMode = {"dorian", "minor"};
    found.clear();
    for (const Result& result : search.run(q)) {
        if (found.empty() || found.back() != result.choraleId) found.push_back(result.choraleId);
    }
    CHECK_EQ(found, (std::vector<std::string>{"chor008", "chor039"}));

    q.choraleFilter.startMode = {"phrygian"};
    CHECK(search.run(q).empty());

    q.choraleFilter = {};
    q.choraleFilter.startMode = {"major"};
    q.choraleFilter.phraseCount = CountRange{6, 6};
    q.choraleFilter.meter = {"4/4"};
    q.choraleFilter.startKey = {"G", "B-"};
    found.clear();
    for (const Result& result : search.run(q)) {
        if (found.empty() || found.back() != result.choraleId) found.push_back(result.choraleId);
    }
    CHECK_EQ(found, (std::vector<std::string>{"chor009", "chor029", "chor103"}));
}

TEST_CASE(chorale_filter_leaves_the_matches_of_the_chorales_it_lets_in_as_they_are) {
    CorpusSearch search(std::filesystem::path(FIXTURE_CHORALE("chor029")).parent_path());
    Query q;
    q.feature = "kern";
    q.pattern = {AttributeMap{{"fermata", {"true"}}}};
    q.voices = "soprano";

    std::vector<Result> expected;
    for (const Result& result : search.run(q)) {
        if (result.choraleId == "chor008" || result.choraleId == "chor029") expected.push_back(result);
    }

    q.choraleFilter.ids = {ChoraleIdRange{"8", 8, 8}, ChoraleIdRange{"29", 29, 29}};
    std::vector<Result> filtered = search.run(q);
    REQUIRE(filtered.size() == expected.size());
    for (std::size_t i = 0; i < filtered.size(); ++i) {
        CHECK_EQ(filtered[i].choraleId, expected[i].choraleId);
        CHECK_EQ(filtered[i].startLineNumber, expected[i].startLineNumber);
        CHECK_EQ(filtered[i].endPosition, expected[i].endPosition);
    }
}

TEST_CASE(chorale_filter_applies_before_the_limit_counts) {
    CorpusSearch search(std::filesystem::path(FIXTURE_CHORALE("chor029")).parent_path());
    Query q;
    q.feature = "kern";
    q.pattern = {AttributeMap{{"fermata", {"true"}}}};
    q.voices = "soprano";
    q.choraleFilter.ids = {ChoraleIdRange{"29-39", 29, 39}};
    q.limit = 8;

    std::vector<Result> results = search.run(q);
    CHECK_EQ(results.size(), std::size_t{8});
    // chor029 has six matches and chor039 the two after them: the limit is not used up by chorales left out
    CHECK_EQ(results.front().choraleId, std::string("chor029"));
    CHECK_EQ(results.back().choraleId, std::string("chor039"));
}

TEST_CASE(every_query_of_a_combined_run_has_a_chorale_filter_of_its_own) {
    CorpusSearch search(std::filesystem::path(FIXTURE_CHORALE("chor029")).parent_path());
    Query minor;
    minor.id = "minor";
    minor.feature = "kern";
    minor.pattern = {AttributeMap{{"fermata", {"true"}}}};
    minor.voices = "soprano";
    Query dorian = minor;
    dorian.id = "dorian";
    Query one = minor;
    one.id = "one";
    minor.choraleFilter.startMode = {"minor"};
    dorian.choraleFilter.startMode = {"dorian"};
    one.choraleFilter.ids = {ChoraleIdRange{"103", 103, 103}};

    std::map<std::string, std::vector<std::string>> chorales; // by the id of the query
    for (const Result& result : search.run(std::vector<Query>{minor, dorian, one})) {
        std::vector<std::string>& found = chorales[*result.queryId];
        if (found.empty() || found.back() != result.choraleId) found.push_back(result.choraleId);
    }
    CHECK_EQ(chorales["minor"], (std::vector<std::string>{"chor039"}));
    CHECK_EQ(chorales["dorian"], (std::vector<std::string>{"chor008"}));
    CHECK_EQ(chorales["one"], (std::vector<std::string>{"chor103"}));
}

TEST_CASE(chorale_filter_ids_leave_the_other_files_unopened_and_out_of_the_progress) {
    CorpusSearch search(std::filesystem::path(FIXTURE_CHORALE("chor029")).parent_path());
    std::vector<choralesearch::SearchProgress> reports;
    search.setProgressCallback([&](const choralesearch::SearchProgress& progress) { reports.push_back(progress); });
    Query q;
    q.feature = "kern";
    q.pattern = {AttributeMap{{"fermata", {"true"}}}};
    q.voices = "soprano";
    q.choraleFilter.ids = {ChoraleIdRange{"1", 1, 1}, ChoraleIdRange{"5", 5, 5}};
    search.run(q);
    REQUIRE(reports.size() == 2u);
    CHECK_EQ(reports[0].choralesTotal, std::size_t{2});
    CHECK_EQ(reports[0].choraleId, std::string("chor001"));
    CHECK_EQ(reports[1].choraleId, std::string("chor005"));

    // The files are only skipped when no query wants them.
    reports.clear();
    Query one = q;
    one.choraleFilter = {};
    one.choraleFilter.ids = {ChoraleIdRange{"1", 1, 1}};
    Query other = q;
    other.choraleFilter = {};
    other.choraleFilter.startMode = {"minor"};
    search.run(std::vector<Query>{one, other});
    CHECK_EQ(reports.size(), std::size_t{8});
}

TEST_CASE(chorale_filter_applies_to_for_each_match_and_to_run_one) {
    CorpusSearch search(std::filesystem::path(FIXTURE_CHORALE("chor029")).parent_path());
    Query q;
    q.feature = "kern";
    q.pattern = {AttributeMap{{"fermata", {"true"}}}};
    q.voices = "soprano";

    Query minor = q;
    minor.choraleFilter.startMode = {"minor"};
    std::vector<std::string> visited;
    search.forEachMatch({minor}, [&](const HumdrumChorale&, const Result& result) {
        if (visited.empty() || visited.back() != result.choraleId) visited.push_back(result.choraleId);
    });
    CHECK_EQ(visited, (std::vector<std::string>{"chor039"}));

    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    CHECK(!search.runOne(chorale, q).empty());

    q.choraleFilter.ids = {ChoraleIdRange{"29", 29, 29}};
    q.choraleFilter.startMode = {"major"};
    CHECK(!search.runOne(chorale, q).empty());

    q.choraleFilter.ids = {ChoraleIdRange{"30", 30, 30}};
    CHECK(search.runOne(chorale, q).empty());

    q.choraleFilter = {};
    q.choraleFilter.startMode = {"minor"};
    CHECK(search.runOne(chorale, q).empty());
}

TEST_MAIN()

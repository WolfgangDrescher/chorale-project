#include "test_framework.hpp"

#include <cctype>
#include <string>
#include <vector>

#include "CorpusSearch.hpp"
#include "HumdrumChorale.hpp"
#include "Segmentation.hpp"

using choralesearch::CorpusSearch;
using choralesearch::HumdrumChorale;
using choralesearch::Segment;
using choralesearch::SegmentQueryOptions;
using choralesearch::segmentScore;

// chor005 and chor008 are the fixtures with ornaments in them.
static const std::vector<std::string> kFixtures = {"chor029", "chor005", "chor008"};

// How many of a chorale's segments do not find the passage they were cut from.
static std::size_t segmentsMissingTheirSource(const std::string& id, const SegmentQueryOptions& options) {
    HumdrumChorale chorale(FIXTURE_CHORALE(id));
    CorpusSearch search(chorale.path());
    std::size_t missing = 0;
    for (const Segment& segment : segmentScore(chorale, {}, options)) {
        if (search.runOne(chorale, segment.query).empty()) ++missing;
    }
    return missing;
}

TEST_CASE(every_segment_finds_its_own_source_by_default) {
    for (const std::string& id : kFixtures) CHECK_EQ(segmentsMissingTheirSource(id, {}), std::size_t{0});
}

TEST_CASE(every_segment_finds_its_own_source_with_exact_intervals_and_kept_ornaments) {
    SegmentQueryOptions exact;
    exact.ignoreIntervalQuality = false;
    exact.matcherOptions.metweightSkipUnclassified = false;
    for (const std::string& id : kFixtures) CHECK_EQ(segmentsMissingTheirSource(id, exact), std::size_t{0});
}

TEST_CASE(every_segment_finds_its_own_source_with_the_inner_voices) {
    SegmentQueryOptions inner;
    inner.innerVoices = true;
    SegmentQueryOptions innerWithQualities = inner;
    innerWithQualities.ignoreIntervalQuality = false;
    for (const std::string& id : kFixtures) {
        CHECK_EQ(segmentsMissingTheirSource(id, inner), std::size_t{0});
        CHECK_EQ(segmentsMissingTheirSource(id, innerWithQualities), std::size_t{0});
    }
}

TEST_CASE(the_inner_voices_are_asked_about_as_an_exact_fb_chord_in_the_groups_only_when_asked_for) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    SegmentQueryOptions inner;
    inner.innerVoices = true;
    SegmentQueryOptions innerWithQualities = inner;
    innerWithQualities.ignoreIntervalQuality = false;

    auto fbValues = [](const std::vector<Segment>& segments) {
        std::vector<std::string> values;
        for (const Segment& segment : segments) {
            // The query's own pattern stays the plain melody.
            for (const auto& position : segment.query.pattern) CHECK(position.find("fb") == position.end());
            for (const auto& group : segment.query.simultaneousWith) {
                for (const auto& position : group.pattern) {
                    auto it = position.find("fb");
                    if (it != position.end() && !it->second.empty()) values.push_back(it->second.front().text);
                }
            }
        }
        return values;
    };
    auto anyQuality = [](const std::vector<std::string>& values) {
        for (const std::string& value : values) {
            if (value.find_first_of("MmPAd") != std::string::npos) return true;
        }
        return false;
    };

    const std::vector<Segment> byDefault = segmentScore(chorale);
    REQUIRE(!byDefault.empty());
    CHECK(fbValues(byDefault).empty());
    for (const Segment& segment : byDefault) CHECK(!segment.query.fbCompareExactChord);

    const std::vector<Segment> withInnerVoices = segmentScore(chorale, {}, inner);
    REQUIRE(!withInnerVoices.empty());
    const std::vector<std::string> plain = fbValues(withInnerVoices);
    CHECK(!plain.empty());
    CHECK(!anyQuality(plain));
    for (const Segment& segment : withInnerVoices) CHECK(segment.query.fbCompareExactChord);

    const std::vector<std::string> exact = fbValues(segmentScore(chorale, {}, innerWithQualities));
    CHECK(anyQuality(exact));
}

TEST_CASE(the_inner_voices_only_narrow_what_a_segment_finds) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    CorpusSearch search(chorale.path());
    SegmentQueryOptions inner;
    inner.innerVoices = true;

    const std::vector<Segment> without = segmentScore(chorale);
    const std::vector<Segment> with = segmentScore(chorale, {}, inner);
    REQUIRE(with.size() == without.size());
    for (std::size_t i = 0; i < with.size(); ++i) {
        CHECK(search.runOne(chorale, with[i].query).size() <= search.runOne(chorale, without[i].query).size());
    }
}

TEST_CASE(interval_complementation_is_allowed_unless_switched_off) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    SegmentQueryOptions off;
    off.matcherOptions.mintAllowIntervalComplementation.clear();

    const std::vector<Segment> byDefault = segmentScore(chorale);
    const std::vector<Segment> without = segmentScore(chorale, {}, off);
    REQUIRE(!byDefault.empty());
    REQUIRE(!without.empty());
    CHECK(!byDefault.front().query.mintAllowIntervalComplementation.empty());
    CHECK(without.front().query.mintAllowIntervalComplementation.empty());
}

TEST_CASE(interval_qualities_are_left_out_of_the_patterns_unless_asked_for) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor029"));
    SegmentQueryOptions exact;
    exact.ignoreIntervalQuality = false;

    // A unison keeps its quality on purpose ("P1" is the repeated note, "+A1" a chromatic step).
    auto hasQuality = [](const std::string& value) {
        bool unison = value.back() == '1' && (value.size() < 2 || !std::isdigit(static_cast<unsigned char>(value[value.size() - 2])));
        return !unison && value.find_first_of("MmPAd") != std::string::npos;
    };
    auto anyQuality = [&](const std::vector<Segment>& segments) {
        for (const Segment& segment : segments) {
            for (const auto& position : segment.query.pattern) {
                auto it = position.find("mint");
                if (it != position.end() && !it->second.empty() && it->second.front().text != "*" &&
                    hasQuality(it->second.front().text)) {
                    return true;
                }
            }
        }
        return false;
    };
    CHECK(!anyQuality(segmentScore(chorale)));
    CHECK(anyQuality(segmentScore(chorale, {}, exact)));
}

TEST_CASE(ornaments_are_skipped_in_every_voice_only_when_asked_for) {
    HumdrumChorale chorale(FIXTURE_CHORALE("chor008"));
    SegmentQueryOptions kept;
    kept.matcherOptions.metweightSkipUnclassified = false;

    // The option lives on the query and its groups inherit it, so none of them carries its own.
    auto skippingQueries = [](const std::vector<Segment>& segments) {
        std::size_t skipping = 0;
        for (const Segment& segment : segments) {
            if (segment.query.metweightSkipUnclassified) ++skipping;
            for (const auto& group : segment.query.simultaneousWith) CHECK(!group.metweightSkipUnclassified);
        }
        return skipping;
    };

    const std::vector<Segment> byDefault = segmentScore(chorale);
    REQUIRE(!byDefault.empty());
    CHECK_EQ(skippingQueries(byDefault), byDefault.size());
    CHECK_EQ(skippingQueries(segmentScore(chorale, {}, kept)), std::size_t{0});
}

TEST_MAIN()


TEST_CASE(segments_still_find_their_own_source_without_phrase_positions) {
    SegmentQueryOptions options;
    options.includePhrase = false;
    for (const std::string& id : kFixtures) CHECK_EQ(segmentsMissingTheirSource(id, options), std::size_t{0});
}

TEST_CASE(phrase_positions_only_narrow_what_a_segment_finds) {
    SegmentQueryOptions pinned;
    SegmentQueryOptions open;
    open.includePhrase = false;
    for (const std::string& id : kFixtures) {
        HumdrumChorale chorale(FIXTURE_CHORALE(id));
        CorpusSearch search(chorale.path());
        auto pinnedSegments = segmentScore(chorale, {}, pinned);
        auto openSegments = segmentScore(chorale, {}, open);
        REQUIRE(pinnedSegments.size() == openSegments.size());
        for (std::size_t i = 0; i < pinnedSegments.size(); ++i) {
            CHECK(search.runOne(chorale, pinnedSegments[i].query).size() <=
                  search.runOne(chorale, openSegments[i].query).size());
        }
    }
}

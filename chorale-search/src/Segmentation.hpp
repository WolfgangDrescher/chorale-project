#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "HumdrumChorale.hpp"
#include "Query.hpp"

namespace choralesearch {

// How wide a segment is and how it moves.
struct SegmentationOptions {
    hum::HumNum length = 4; // in quarter notes, exactly -- never rounded to what the notes allow
    hum::HumNum step = 1;   // how far the window rolls between two segments, in quarter notes
};

// How a segment is turned into a query. The outer voices, the soprano driving the search -- and
// the rest is what the frontend's checkboxes will set, which is why they're options rather than
// constants inside the builder.
struct SegmentQueryOptions {
    // The voice the query walks, and whose line its own pattern describes. A window this voice
    // holds a single note across says nothing about it and becomes no segment at all.
    std::size_t voice = 4;

    // Voices added as simultaneousWith groups, each with its own pattern over the same segment:
    // a passage only matches when all of them do their part at the same time. Adding 2 and 3 is
    // what "take the inner voices into account" will mean.
    std::vector<std::size_t> simultaneousVoices = {1};

    std::string feature = "mint"; // the driving feature of the query and of every group

    bool includeDuration = true;
    bool includeFermata = true;

    // hint spines stated at every position of the query's own pattern. Not repeated inside the
    // groups: with both voices' own lines pinned down, the same pair there says nothing new.
    std::vector<std::string> hintPairs = {"hint-14"};

    // Handed to the query verbatim (see Query.hpp): none of them change what the pattern asks
    // for, only how strictly a passage has to answer it. metweightSkipUnclassified needs a
    // second half here first -- it takes the ornaments out of what a search walks, so a pattern
    // still spelling out its own would match nothing, its source included, until it is built
    // from the same folded onsets (AttributeMatcher's buildOnsets).
    MatcherOptions matcherOptions = {};
};

// One position of the window, and the query that searches the corpus for what happens there.
// The window itself doesn't survive: it decides which onsets become pattern positions and
// whether the last note keeps its duration, and has nothing left to say once that is settled.
struct Segment {
    Query query;

    // First and last note attacked inside the window, across every voice the query talks about:
    // "0 to 3" for a four-quarter window whose last attack falls on the fourth quarter, "0 to
    // 3+1/2" when the bass answers half a quarter later. That last note may go on sounding past
    // the window; only its attack is inside. A match reports the *query voice's* own onsets (see
    // Result.hpp), which can be narrower.
    hum::HumNum startPosition;
    hum::HumNum endPosition;
    int startLineNumber = 0;
    int endLineNumber = 0;
};

// Every position of a window of the requested length, slid over the score one step at a time:
// 0-4, 1-5, 2-6, ... Segments overlap, so no reading of a passage is lost to where the counting
// began, and boundaries fall where the grid puts them, in the middle of a sounding note
// included. A window becomes no segment when a phrase ending falls in its middle (a fermata
// always closes the segment it falls in) or when the query voice never attacks inside it.
//
// Each segment carries the query that finds it again, tagged "segment-1", "segment-2", ... so a
// combined run's results can be told apart by their queryId. Every value in it is read straight
// out of the score, which is what makes a segment always match its own source.
std::vector<Segment> segmentScore(const HumdrumChorale& chorale, const SegmentationOptions& options = {},
                                   const SegmentQueryOptions& queryOptions = {});

} // namespace choralesearch

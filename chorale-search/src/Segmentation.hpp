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

// An interval token ("+M2", "-m3", "m10") reduced to direction and number ("+2", "-3", "10"), the
// way a segment query writes its intervals when the quality is ignored. A unison keeps its
// quality, and anything that isn't a plain interval is returned as it is.
std::string withoutQuality(const std::string& interval);

// The matcher tolerances a segment query asks for by default: everything that widens a match
// without ever losing the segment's own source. Compound intervals fold to their simple ones,
// complementary intervals count for every number, ornaments are left out, and a note may be found
// written as its own split or merger.
inline MatcherOptions defaultSegmentMatcherOptions() {
    MatcherOptions options;
    options.hintReduceCompound = true;
    options.mintAllowIntervalComplementation = {"*"};
    options.metweightSkipUnclassified = true;
    options.durationAllowSplitNotes = true;
    options.durationAllowMergedNotes = true;
    return options;
}

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

    // Intervals (mint and the hint pairs) are written without their quality, so a minor
    // passage is found in major pieces and the other way round, and a raised leading tone
    // doesn't hide a melody. A unison keeps its quality.
    bool ignoreIntervalQuality = true;

    bool includeDuration = true;
    bool includeFermata = true;

    // Every position also states the metric weight its note falls on: a strong note is found on a
    // strong or half-strong beat and the other way round, so beat 1 of a 4/4 measure also finds
    // beat 3 (a passage shifted by half a measure), a weak note stays on a weak beat (2 and 4
    // change places), and an unclassified one stays unclassified.
    bool metricPositions = false;

    // hint spines stated at every position of each simultaneous group's pattern -- not the
    // query's own, which stays the plain cantus firmus so it can be lifted out and used alone.
    // With both voices' lines pinned down either placement selects the same passages; a window
    // that loses its group (see buildQuery) loses the vertical anchoring with it and falls
    // back to matching the outer voices' contours at any distance.
    std::vector<std::string> hintPairs = {"hint-14"};

    // The inner voices, taken into account as the harmony they form with the bass: every
    // position of each simultaneous group also states the fb chord sounding there -- the
    // intervals above the bass, in no particular voice, folded into one octave and without
    // doubled ones (see docs/features/fb). Which inner voice sings which figure, and so their
    // order, doesn't matter. Their own melodies and rhythms aren't asked about, only the chords
    // they make with the outer voices. The chord is compared exactly, since a chord with one more
    // figure has a voice the segment doesn't have. Follows ignoreIntervalQuality, and has nothing
    // to hang on where a window lost its group (see buildQuery).
    bool innerVoices = false;

    // Handed to the query verbatim (see Query.hpp): none of them change what the pattern asks
    // for, only how strictly a passage has to answer it. metweightSkipUnclassified also decides
    // how the pattern is built: ornaments (notes on a metrically unclassified position) are left
    // out of every voice's pattern, taken from the same folded onsets the search will walk, so a
    // voice with passing notes is found by the same voice without them and the other way round.
    MatcherOptions matcherOptions = defaultSegmentMatcherOptions();
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

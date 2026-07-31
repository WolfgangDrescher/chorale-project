#include "Segmentation.hpp"

#include "HumdrumUtils.hpp"

#include <algorithm>
#include <stdexcept>
#include <string>

namespace choralesearch {

namespace {

const std::string kKernFeature = "kern";
const std::string kMintFeature = "mint";
const std::string kDurationKey = "duration";
const std::string kFermataKey = "fermata";
const std::string kWildcard = "*";

// Every note (or rest) a voice attacks between two positions, in order -- exactly what
// AttributeMatcher walks when it runs the query later. The continuation of a tie isn't an attack
// of its own: it belongs to the note that started earlier, whose duration already covers it
// (getTiedDuration). A note the window opened in the middle of was attacked before it and is
// therefore not one of them either: the pattern describes what happens inside the window, not
// what was already sounding when it opened.
std::vector<hum::HTp> onsetsBetween(const HumdrumChorale& chorale, const std::string& feature, std::size_t voice,
                                     hum::HumNum from, hum::HumNum until) {
    std::vector<hum::HTp> onsets;
    hum::HTp start = chorale.spine(feature, voice);
    if (!start) return onsets;
    for (hum::HTp t = start->getNextToken(); t; t = t->getNextToken()) {
        if (!t->getOwner()->isData() || t->isNull() || t->isSecondaryTiedNote()) continue;
        hum::HumNum position = t->getDurationFromStart();
        if (position < from) continue;
        if (position >= until) break;
        onsets.push_back(t);
    }
    return onsets;
}

std::size_t voiceCount(const HumdrumChorale& chorale) {
    std::size_t count = 0;
    while (chorale.spine(kKernFeature, count + 1)) ++count;
    return count;
}

// Where the phrases end, in order: a fermata closes one, and it is over when that note stops
// sounding. The voices disagree about where the fermata is -- one may take it a quarter before
// the others (chor039) or let go early and keep moving under the held chord (chor234's bass) --
// so fermata notes sounding at the same time close one phrase, which lasts until the last of
// them stops. The final ending is where the piece ends: every chorale closes on a fermata, and
// the three writing a rest after it have nothing there worth a segment.
std::vector<hum::HumNum> phraseEndings(const HumdrumChorale& chorale) {
    std::vector<std::pair<hum::HumNum, hum::HumNum>> fermataNotes; // attacked at .first, over at .second
    for (std::size_t voice = 1; voice <= voiceCount(chorale); ++voice) {
        hum::HTp start = chorale.spine(kKernFeature, voice);
        if (!start) continue;
        for (hum::HTp t = start->getNextToken(); t; t = t->getNextToken()) {
            if (!t->getOwner()->isData() || t->isNull() || t->isSecondaryTiedNote() || !t->hasFermata()) continue;
            fermataNotes.emplace_back(t->getDurationFromStart(), t->getDurationFromStart() + soundingDuration(t));
        }
    }
    std::sort(fermataNotes.begin(), fermataNotes.end());

    std::vector<hum::HumNum> endings;
    for (const auto& [attacked, over] : fermataNotes) {
        bool sameChordAsPrevious = !endings.empty() && attacked < endings.back();
        if (sameChordAsPrevious) endings.back() = std::max(endings.back(), over);
        else endings.push_back(over);
    }
    return endings;
}

// One pattern position per onset, stating everything the options ask for about it. A key whose
// spine has nothing at that line is left out rather than written as a wildcard: a rest, for
// instance, has no interval to the soprano, and demanding one there would keep the segment from
// matching even its own source.
std::vector<AttributeMap> buildPattern(const HumdrumChorale& chorale, const std::vector<hum::HTp>& onsets,
                                        std::size_t voice, hum::HumNum windowEnd,
                                        const SegmentQueryOptions& options, bool crossReferenceHintPairs) {
    std::vector<AttributeMap> pattern;

    for (std::size_t i = 0; i < onsets.size(); ++i) {
        hum::HTp token = onsets[i];
        int lineNumber = token->getLineNumber();
        AttributeMap position;

        // A **mint token is the interval *into* its note, so the first one of a segment
        // measures from the note before it -- something about the passage that came earlier,
        // not about this segment. Asking for it as a wildcard keeps the note itself as the
        // pattern's first position (see docs/options#mintstartatprevioustoken) without saying
        // anything about how the music got there.
        bool isMintLeadIn = options.feature == kMintFeature && i == 0;
        position[options.feature] = {isMintLeadIn ? kWildcard : std::string(*token)};

        // The last note of a segment routinely goes on sounding past the window -- a half note
        // it cuts in two. Only the part inside belongs to the segment, and a pattern can't ask
        // for part of a note, so the duration is left open there instead of pinning down a
        // length the segment doesn't actually cover.
        bool soundsPastTheEnd = token->getDurationFromStart() + soundingDuration(token) > windowEnd;
        if (options.includeDuration && !soundsPastTheEnd) {
            position[kDurationKey] = {hum::Convert::durationToRecip(soundingDuration(token))};
        }
        if (options.includeFermata) {
            if (hum::HTp kernToken = findTokenAtLine(chorale.spine(kKernFeature, voice), lineNumber)) {
                position[kFermataKey] = {kernToken->hasFermata() ? "true" : "false"};
            }
        }
        if (crossReferenceHintPairs) {
            for (const std::string& pair : options.hintPairs) {
                // A hint spine belongs to the pair, not to a voice: there is one per score, and
                // it's always found under the first voice (see docs/features/hint).
                if (hum::HTp hintToken = findTokenAtLine(chorale.spine(pair, 1), lineNumber)) {
                    position[pair] = {std::string(*hintToken)};
                }
            }
        }
        pattern.push_back(std::move(position));
    }
    return pattern;
}

Query buildQuery(const HumdrumChorale& chorale, const std::vector<hum::HTp>& onsets, hum::HumNum windowStart,
                  hum::HumNum windowEnd, const SegmentQueryOptions& options, const std::string& id) {
    Query query;
    query.id = id;
    query.feature = options.feature;
    query.voices = std::to_string(options.voice); // a voice selector takes the number as it is
    query.pattern = buildPattern(chorale, onsets, options.voice, windowEnd, options, true);

    const hum::HumNum patternStart = onsets.front()->getDurationFromStart();
    for (std::size_t voice : options.simultaneousVoices) {
        if (voice == options.voice) continue; // it's already the query's own pattern

        // A group only constrains a match that starts where the group's own match does (see
        // docs/options#simultaneousalignment). A voice whose first onset here isn't the query's
        // -- the window opened while it was holding a note -- would leave the two patterns
        // looking for different starting points and the query finding nothing, its own source
        // included, so it is left to its hint pair instead.
        const std::vector<hum::HTp> groupOnsets =
            onsetsBetween(chorale, options.feature, voice, windowStart, windowEnd);
        if (groupOnsets.empty() || groupOnsets.front()->getDurationFromStart() != patternStart) continue;

        SimultaneousGroup group;
        group.feature = options.feature;
        group.voices = std::to_string(voice);
        group.pattern = buildPattern(chorale, groupOnsets, voice, windowEnd, options, false);
        query.simultaneousWith.push_back(std::move(group));
    }

    // simultaneousAlignment is left at the default "start" on purpose: the two voices' last
    // onsets inside a segment rarely fall on the same position (whoever moves last ends the
    // match), while the durations spelled out in both patterns already pin the group to the
    // same stretch of music.
    query.mintStartAtPreviousToken = options.matcherOptions.mintStartAtPreviousToken;
    query.mintAllowIntervalComplementation = options.matcherOptions.mintAllowIntervalComplementation;
    query.fbCompareExactChord = options.matcherOptions.fbCompareExactChord;
    query.kernIgnoreOctave = options.matcherOptions.kernIgnoreOctave;
    query.hintReduceCompound = options.matcherOptions.hintReduceCompound;
    query.durationAllowSplitNotes = options.matcherOptions.durationAllowSplitNotes;
    query.durationAllowMergedNotes = options.matcherOptions.durationAllowMergedNotes;
    query.metweightSkipUnclassified = options.matcherOptions.metweightSkipUnclassified;
    return query;
}

} // namespace

std::vector<Segment> segmentScore(const HumdrumChorale& chorale, const SegmentationOptions& options,
                                   const SegmentQueryOptions& queryOptions) {
    if (options.length <= 0) throw std::invalid_argument("Segment length must be positive");
    if (options.step <= 0) throw std::invalid_argument("Segment step must be positive");

    std::vector<Segment> segments;
    const std::vector<hum::HumNum> endings = phraseEndings(chorale);
    if (endings.empty()) return segments;

    // A segment may end where a phrase does and it may start there, but it may not run from one
    // side of it to the other: that is what "no segment reaches across a fermata" means once
    // boundaries no longer have to be onsets.
    auto crossesPhraseEnding = [&](hum::HumNum windowStart, hum::HumNum windowEnd) {
        return std::any_of(endings.begin(), endings.end(), [&](const hum::HumNum& phraseEnd) {
            return windowStart < phraseEnd && windowEnd > phraseEnd;
        });
    };

    for (hum::HumNum start = 0; start + options.length <= endings.back(); start += options.step) {
        const hum::HumNum stop = start + options.length;
        if (crossesPhraseEnding(start, stop)) continue;

        const std::vector<hum::HTp> onsets =
            onsetsBetween(chorale, queryOptions.feature, queryOptions.voice, start, stop);
        if (onsets.empty()) continue; // the voice holds one note across it -- nothing to ask about

        // What the segment covers is what the query as a whole asks about, so a voice that comes
        // in before the query's own or moves after it stretches the segment to itself -- even
        // where that voice lost its group (see buildQuery), since the window did hold its notes.
        hum::HTp earliest = onsets.front();
        hum::HTp latest = onsets.back();
        for (std::size_t voice : queryOptions.simultaneousVoices) {
            if (voice == queryOptions.voice) continue;
            const std::vector<hum::HTp> other = onsetsBetween(chorale, queryOptions.feature, voice, start, stop);
            if (other.empty()) continue;
            if (other.front()->getDurationFromStart() < earliest->getDurationFromStart()) earliest = other.front();
            if (other.back()->getDurationFromStart() > latest->getDurationFromStart()) latest = other.back();
        }

        Segment segment;
        segment.query = buildQuery(chorale, onsets, start, stop, queryOptions,
                                    "segment-" + std::to_string(segments.size() + 1));
        segment.startPosition = earliest->getDurationFromStart();
        segment.endPosition = latest->getDurationFromStart();
        segment.startLineNumber = earliest->getLineNumber();
        segment.endLineNumber = latest->getLineNumber();
        segments.push_back(std::move(segment));
    }
    return segments;
}

} // namespace choralesearch

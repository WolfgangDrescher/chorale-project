#pragma once

#include <cstddef>
#include <filesystem>
#include <functional>
#include <string>
#include <utility>
#include <vector>

#include "Query.hpp"
#include "Result.hpp"

namespace choralesearch {

class HumdrumChorale;

// Where a corpus run stands: `choralesSearched` of `choralesTotal` chorale files are finished,
// `choraleId` being the last of them, and `matchesSoFar` all the results collected from the
// files searched up to there.
struct SearchProgress {
    std::size_t choralesSearched = 0;
    std::size_t choralesTotal = 0;
    std::string choraleId;
    std::size_t matchesSoFar = 0;
};

using ProgressCallback = std::function<void(const SearchProgress&)>;

// One match together with the chorale it was found in, for a caller that needs to look at the
// score around the match and not only at where it is.
using MatchVisitor = std::function<void(const HumdrumChorale&, const Result&)>;

class CorpusSearch {
public:
    // `applyAnalysis` false skips deriving the analysis spines while loading each chorale,
    // for a corpus that already carries them (see chorale-generate --analysis). It is by far
    // the dominant cost of a search, so a generated corpus searches in a fraction of the time.
    explicit CorpusSearch(std::filesystem::path corpusRoot, bool applyAnalysis = true);

    // Called after every chorale file a run has gone through, for the progress of a long run.
    // The library reports and leaves it to the caller what to do with it.
    void setProgressCallback(ProgressCallback callback) { m_onProgress = std::move(callback); }

    // Runs `query` across every *.krn file found (recursively) under the corpus root.
    Results run(const Query& query) const;

    // Runs every query in `queries` across every *.krn file found (recursively) under the
    // corpus root, parsing/analyzing each chorale only once regardless of how many queries
    // there are. Every Result's queryId is set to that query's own id (or its index in
    // `queries`, stringified, if it didn't set one) -- see Query::id.
    Results run(const std::vector<Query>& queries) const;

    // The same search as run(queries), except that no results are collected: every match is
    // handed to `visit` along with its chorale, which is loaded once per file and gone again
    // after it. The Result's queryId is set the way run(queries) sets it. A query's `limit` is
    // not applied, since there is nothing collected to cap.
    void forEachMatch(const std::vector<Query>& queries, const MatchVisitor& visit) const;

    // Runs `query` against a single already-loaded chorale.
    Results runOne(const HumdrumChorale& chorale, const Query& query) const;

private:
    std::filesystem::path m_corpusRoot;
    bool m_applyAnalysis;
    ProgressCallback m_onProgress;
    std::vector<std::filesystem::path> findChoraleFiles() const;
    void reportProgress(std::size_t choralesSearched, std::size_t choralesTotal, const std::string& choraleId,
                        std::size_t matchesSoFar) const;
};

} // namespace choralesearch

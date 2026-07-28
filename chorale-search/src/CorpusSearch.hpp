#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "Query.hpp"
#include "Result.hpp"

namespace choralesearch {

class HumdrumChorale;

class CorpusSearch {
public:
    // `applyAnalysis` false skips deriving the analysis spines while loading each chorale,
    // for a corpus that already carries them (see chorale-generate --analysis). It is by far
    // the dominant cost of a search, so a generated corpus searches in a fraction of the time.
    explicit CorpusSearch(std::filesystem::path corpusRoot, bool applyAnalysis = true);

    // Runs `query` across every *.krn file found (recursively) under the corpus root.
    Results run(const Query& query) const;

    // Runs every query in `queries` across every *.krn file found (recursively) under the
    // corpus root, parsing/analyzing each chorale only once regardless of how many queries
    // there are. Every Result's queryId is set to that query's own id (or its index in
    // `queries`, stringified, if it didn't set one) -- see Query::id.
    Results run(const std::vector<Query>& queries) const;

    // Runs `query` against a single already-loaded chorale.
    Results runOne(const HumdrumChorale& chorale, const Query& query) const;

private:
    std::filesystem::path m_corpusRoot;
    bool m_applyAnalysis;
    std::vector<std::filesystem::path> findChoraleFiles() const;
};

} // namespace choralesearch

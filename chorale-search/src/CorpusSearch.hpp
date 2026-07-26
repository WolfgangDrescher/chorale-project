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
    explicit CorpusSearch(std::filesystem::path corpusRoot);

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
    std::vector<std::filesystem::path> findChoraleFiles() const;
};

} // namespace choralesearch

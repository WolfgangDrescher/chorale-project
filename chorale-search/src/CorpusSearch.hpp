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

    // Runs `query` against a single already-loaded chorale.
    Results runOne(const HumdrumChorale& chorale, const Query& query) const;

private:
    std::filesystem::path m_corpusRoot;
    std::vector<std::filesystem::path> findChoraleFiles() const;
};

} // namespace choralesearch

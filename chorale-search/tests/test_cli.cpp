#include "test_framework.hpp"

#include <algorithm>
#include <array>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sys/wait.h>

#include <nlohmann/json.hpp>

// End-to-end smoke test: does the actual compiled `chorale-search` binary
// work when invoked the way a real user would invoke it? This is
// deliberately a handful of cases, not exhaustive coverage -- the search
// engine's own correctness across many use cases belongs in
// test_attributematcher.cpp / test_corpussearch.cpp, which check results
// directly instead of round-tripping through a subprocess and JSON/table
// text output.

namespace {

struct ProcessResult {
    int exitCode = -1;
    std::string stdOut;
};

std::string shellQuote(const std::string& s) {
    std::string out = "'";
    for (char c : s) {
        if (c == '\'') out += "'\\''";
        else out += c;
    }
    out += "'";
    return out;
}

ProcessResult runCli(const std::vector<std::string>& args) {
    std::string cmd = shellQuote(CHORALE_SEARCH_BINARY);
    for (const auto& arg : args) cmd += " " + shellQuote(arg);
    cmd += " 2>/dev/null";

    ProcessResult result;
    FILE* pipe = popen(cmd.c_str(), "r");
    REQUIRE(pipe != nullptr);
    std::array<char, 4096> buffer{};
    while (fgets(buffer.data(), buffer.size(), pipe)) result.stdOut += buffer.data();
    int status = pclose(pipe);
    result.exitCode = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
    return result;
}

std::string writeTempQueryFile(const std::string& jsonText) {
    auto path = std::filesystem::temp_directory_path() / "chorale_search_test_query.json";
    std::ofstream f(path);
    f << jsonText;
    return path.string();
}

std::string fixturesDir() {
    return std::filesystem::path(FIXTURE_CHORALE("chor029")).parent_path().string();
}

} // namespace

TEST_CASE(cli_help_exits_zero) {
    CHECK_EQ(runCli({"--help"}).exitCode, 0);
}

TEST_CASE(cli_with_no_arguments_exits_nonzero) {
    CHECK(runCli({}).exitCode != 0);
}

TEST_CASE(cli_missing_query_exits_nonzero) {
    CHECK(runCli({fixturesDir()}).exitCode != 0);
}

TEST_CASE(cli_nonexistent_corpus_exits_nonzero) {
    auto queryFile = writeTempQueryFile(R"({"feature":"kern","pattern":[{"kern":"*"}]})");
    CHECK(runCli({"/nonexistent/corpus", "--query-file", queryFile}).exitCode != 0);
}

TEST_CASE(cli_query_argument_finds_the_same_matches_as_query_file) {
    // Same query, fed in both ways: --query (inline JSON, exercising that
    // runCli's shell-quoting survives the embedded double quotes) and
    // --query-file. Both must produce byte-identical output.
    std::string queryJson = R"({"feature":"kern","voices":"soprano","pattern":[{"fermata":true}]})";
    auto queryFile = writeTempQueryFile(queryJson);

    auto viaQuery = runCli({fixturesDir(), "--query", queryJson, "--format", "json"});
    auto viaQueryFile = runCli({fixturesDir(), "--query-file", queryFile, "--format", "json"});

    REQUIRE(viaQuery.exitCode == 0);
    REQUIRE(viaQueryFile.exitCode == 0);
    CHECK_EQ(viaQuery.stdOut, viaQueryFile.stdOut);
}

TEST_CASE(cli_query_and_query_file_together_is_rejected) {
    auto queryFile = writeTempQueryFile(R"({"feature":"kern","pattern":[{"kern":"*"}]})");
    auto result = runCli({fixturesDir(), "--query", "{}", "--query-file", queryFile});
    CHECK(result.exitCode != 0);
}

TEST_CASE(cli_json_format_matches_what_corpussearch_finds_directly) {
    auto queryFile = writeTempQueryFile(R"({"feature":"kern","voices":"soprano","pattern":[{"fermata":true}]})");
    auto result = runCli({fixturesDir(), "--query-file", queryFile, "--format", "json"});
    REQUIRE(result.exitCode == 0);

    nlohmann::json parsed;
    CHECK_NOTHROW(parsed = nlohmann::json::parse(result.stdOut));
    REQUIRE(parsed.is_array());
    CHECK_EQ(parsed.size(), std::size_t{28}); // 6+4+6+6+6 soprano fermatas across the 5 fixture chorales, see test_corpussearch.cpp
    REQUIRE(!parsed.empty());
    CHECK(parsed[0].contains("chorale"));
    CHECK(parsed[0].contains("voice"));
}

TEST_CASE(cli_table_format_has_a_header_plus_one_row_per_match) {
    auto queryFile = writeTempQueryFile(R"({"feature":"kern","voices":"soprano","pattern":[{"fermata":true}]})");
    auto result = runCli({fixturesDir(), "--query-file", queryFile}); // default format is "table"
    REQUIRE(result.exitCode == 0);
    CHECK_EQ(result.stdOut.substr(0, 7), std::string("chorale"));
    auto lineCount = std::count(result.stdOut.begin(), result.stdOut.end(), '\n');
    CHECK_EQ(lineCount, std::ptrdiff_t{29}); // 1 header + 28 matches
}

TEST_CASE(cli_accepts_an_array_of_queries_and_tags_results_with_query_id) {
    std::string queryJson = R"([
        {"id":"soprano","feature":"kern","voices":"soprano","pattern":[{"fermata":true}]},
        {"feature":"kern","voices":"bass","pattern":[{"fermata":true}]}
    ])";
    auto result = runCli({fixturesDir(), "--query", queryJson, "--format", "json"});
    REQUIRE(result.exitCode == 0);

    nlohmann::json parsed;
    CHECK_NOTHROW(parsed = nlohmann::json::parse(result.stdOut));
    REQUIRE(parsed.is_array());
    REQUIRE(!parsed.empty());
    CHECK(parsed[0].contains("queryId"));

    bool sawCustomId = false;
    bool sawPositionalIndex = false;
    for (const auto& entry : parsed) {
        std::string queryId = entry.at("queryId").get<std::string>();
        if (queryId == "soprano") sawCustomId = true;
        if (queryId == "1") sawPositionalIndex = true;
    }
    CHECK(sawCustomId);
    CHECK(sawPositionalIndex);
}

TEST_CASE(cli_single_object_json_output_never_has_a_query_id_field) {
    auto queryFile = writeTempQueryFile(R"({"feature":"kern","voices":"soprano","pattern":[{"fermata":true}]})");
    auto result = runCli({fixturesDir(), "--query-file", queryFile, "--format", "json"});
    REQUIRE(result.exitCode == 0);

    nlohmann::json parsed;
    CHECK_NOTHROW(parsed = nlohmann::json::parse(result.stdOut));
    REQUIRE(!parsed.empty());
    for (const auto& entry : parsed) CHECK(!entry.contains("queryId"));
}

TEST_CASE(cli_table_format_adds_query_id_column_only_for_array_input) {
    std::string queryJson = R"([{"feature":"kern","voices":"soprano","pattern":[{"fermata":true}]}])";
    auto result = runCli({fixturesDir(), "--query", queryJson}); // default format is "table"
    REQUIRE(result.exitCode == 0);
    auto headerEnd = result.stdOut.find('\n');
    std::string header = result.stdOut.substr(0, headerEnd);
    CHECK(header.find("query_id") != std::string::npos);
}

TEST_CASE(cli_rejects_an_empty_query_array) {
    auto result = runCli({fixturesDir(), "--query", "[]"});
    CHECK(result.exitCode != 0);
}

TEST_MAIN()

#pragma once

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#if defined(__unix__) || defined(__APPLE__)
#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace minitest {

struct TestCase {
    std::string name;
    void (*fn)();
};

inline std::vector<TestCase>& registry() {
    static std::vector<TestCase> r;
    return r;
}

struct Registrar {
    Registrar(const char* name, void (*fn)()) { registry().push_back({name, fn}); }
};

inline int& failureCount() { static int n = 0; return n; }
inline int& checkCount() { static int n = 0; return n; }
inline std::string& currentTestName() { static std::string s; return s; }
inline std::vector<std::string>& currentTestFailures() { static std::vector<std::string> v; return v; }

// Shortens an absolute/relative source path down to "tests/whatever.cpp"
// when possible, so output reads like a repo-relative path instead of
// whatever absolute build path __FILE__ happened to expand to.
inline std::string shortFileName(const std::string& file) {
    std::size_t pos = file.rfind("/tests/");
    if (pos != std::string::npos) return file.substr(pos + 1);
    pos = file.find_last_of('/');
    return pos == std::string::npos ? file : file.substr(pos + 1);
}

// Resolves tests/fixtures relative to the calling test file (not the test
// binary's working directory, which varies by how it's invoked -- directly,
// via ctest, via `make test`, ...). Use the FIXTURES_DIR and FIXTURE_CHORALE(id)
// macros below rather than calling these directly, so __FILE__ is always the
// test file itself.
inline std::filesystem::path fixturesDirPath(const char* sourceFile) {
    return std::filesystem::path(sourceFile).parent_path() / "fixtures";
}

inline std::string fixtureChoralePath(const char* sourceFile, const std::string& choraleId) {
    return (fixturesDirPath(sourceFile) / (choraleId + ".krn")).string();
}

// RAII guard that redirects stderr to /dev/null for its scope -- for
// silencing known-noisy diagnostic output an external library (e.g.
// humlib) writes directly to stderr around an expected failure, without
// touching stdout (so minitest's own report still prints normally).
// A no-op on non-POSIX platforms, where it's simply not implemented.
#if defined(__unix__) || defined(__APPLE__)
class SilenceStderr {
public:
    SilenceStderr() {
        std::fflush(stderr);
        m_savedFd = dup(fileno(stderr));
        if (FILE* devnull = std::fopen("/dev/null", "w")) {
            dup2(fileno(devnull), fileno(stderr));
            std::fclose(devnull);
        }
    }
    ~SilenceStderr() {
        std::fflush(stderr);
        if (m_savedFd >= 0) {
            dup2(m_savedFd, fileno(stderr));
            close(m_savedFd);
        }
    }
    SilenceStderr(const SilenceStderr&) = delete;
    SilenceStderr& operator=(const SilenceStderr&) = delete;

private:
    int m_savedFd = -1;
};
#else
class SilenceStderr {};
#endif

inline void reportFailure(const char* file, int line, const std::string& msg) {
    ++failureCount();
    std::ostringstream oss;
    oss << shortFileName(file) << ":" << line << ": " << msg;
    currentTestFailures().push_back(oss.str());
}

// Thrown by REQUIRE_* on failure to unwind out of the current test case.
struct RequireFailed : std::exception {
    const char* what() const noexcept override { return "REQUIRE failed"; }
};

// Formats a value for a CHECK_EQ failure message. Generic fallback uses
// operator<<; the std::vector<T> overload (more specialized, so preferred
// by overload resolution whenever the argument actually is a vector)
// prints "[a,b,c]" instead of failing to compile for types like
// std::vector<std::size_t> that have no operator<<.
template <class T>
std::string toDebugString(const T& v) {
    std::ostringstream oss;
    oss << v;
    return oss.str();
}

template <class T>
std::string toDebugString(const std::vector<T>& v) {
    std::ostringstream oss;
    oss << "[";
    for (std::size_t i = 0; i < v.size(); ++i) {
        if (i) oss << ",";
        oss << toDebugString(v[i]);
    }
    oss << "]";
    return oss.str();
}

namespace color {

inline bool enabled() {
    static const bool on = [] {
        if (std::getenv("NO_COLOR")) return false;
        if (std::getenv("FORCE_COLOR")) return true;
#if defined(__unix__) || defined(__APPLE__)
        return isatty(fileno(stdout)) != 0;
#else
        return false;
#endif
    }();
    return on;
}

inline std::string wrap(const char* code, const std::string& s) {
    if (!enabled()) return s;
    return std::string("\033[") + code + "m" + s + "\033[0m";
}

inline std::string green(const std::string& s) { return wrap("32", s); }
inline std::string red(const std::string& s) { return wrap("31", s); }
inline std::string gray(const std::string& s) { return wrap("2", s); }
inline std::string boldRed(const std::string& s) { return wrap("1;31", s); }

} // namespace color

inline std::string formatMillis(double ms) {
    std::ostringstream oss;
    if (ms < 10) oss.precision(2);
    else oss.precision(0);
    oss << std::fixed << ms << "ms";
    return oss.str();
}

inline long currentProcessId() {
#if defined(__unix__) || defined(__APPLE__)
    return getpid();
#else
    return 0;
#endif
}

// The directory this run of the suite keeps its files in, made before the first test case and
// removed after the last.
inline std::filesystem::path& tempDirectory() {
    static std::filesystem::path path;
    return path;
}

// The path of a file of that name for the running test case, in the directory of the run. The test
// cases run side by side, so each gets a file of its own, and a name can be used by all of them.
inline std::filesystem::path tempFile(const std::string& name) {
    return tempDirectory() / (std::to_string(currentProcessId()) + "_" + name);
}

// What running a test case came to.
struct TestCaseResult {
    std::vector<std::string> failures; // what it reported
    std::string printed;               // what it wrote to stdout and stderr
};

// Runs a test case in this process and returns the failures it reported.
inline std::vector<std::string> runTestCase(const TestCase& testCase, const char* sourceFile) {
    currentTestName() = testCase.name;
    currentTestFailures().clear();
    try {
        testCase.fn();
    } catch (const RequireFailed&) {
        // already reported; test case aborted early
    } catch (const std::exception& e) {
        reportFailure(sourceFile, 0, std::string("uncaught exception: ") + e.what());
    } catch (...) {
        reportFailure(sourceFile, 0, "uncaught non-std::exception");
    }
    return currentTestFailures();
}

// One character per finished test case, written as soon as it is: the progress of the suite.
inline void printProgressDot(bool passed) {
    std::fputs((passed ? color::green(".") : color::boldRed("×")).c_str(), stdout);
    std::fflush(stdout);
}

#if defined(__unix__) || defined(__APPLE__)

// How many test cases run at the same time: MINITEST_JOBS if it is set (1 runs them one after the
// other in this process, which is what a debugger needs), otherwise one per processor.
inline std::size_t maxParallelTestCases() {
    if (const char* setting = std::getenv("MINITEST_JOBS")) {
        const long jobs = std::strtol(setting, nullptr, 10);
        if (jobs > 0) return static_cast<std::size_t>(jobs);
    }
    const long processors = sysconf(_SC_NPROCESSORS_ONLN);
    return processors > 0 ? static_cast<std::size_t>(processors) : 1;
}

inline std::string readWholeFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    std::ostringstream content;
    content << file.rdbuf();
    return content.str();
}

// The failures of a test case go to its .err file one after the other, each ended by a NUL
// character, which a failure message does not contain.
inline void writeFailures(const std::string& path, const std::vector<std::string>& failures) {
    std::ofstream file(path, std::ios::binary);
    for (const std::string& failure : failures) file << failure << '\0';
}

inline std::vector<std::string> readFailures(const std::string& path) {
    std::vector<std::string> failures;
    std::istringstream file(readWholeFile(path));
    for (std::string failure; std::getline(file, failure, '\0');) failures.push_back(failure);
    return failures;
}

// Starts a process that runs the test case and writes what it printed to <filePrefix>.out and the
// failures it reported to <filePrefix>.err. Returns the id of the process.
inline pid_t startTestProcess(const TestCase& testCase, const char* sourceFile, const std::string& filePrefix) {
    std::fflush(nullptr); // or the new process would write out what this one has buffered, too
    const pid_t process = fork();
    if (process < 0) {
        std::perror("minitest: fork");
        std::exit(2);
    }
    if (process > 0) return process;

    const int printedFile = open((filePrefix + ".out").c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
    dup2(printedFile, STDOUT_FILENO);
    dup2(printedFile, STDERR_FILENO);
    close(printedFile);
    writeFailures(filePrefix + ".err", runTestCase(testCase, sourceFile));
    std::fflush(nullptr);
    _exit(0); // not exit(), which would run the copy of the static destructors in this process
}

// What the process of a test case came to, from its files and from the way it ended.
inline TestCaseResult collectTestCaseResult(const std::string& filePrefix, int exitStatus) {
    TestCaseResult testCaseResult;
    testCaseResult.printed = readWholeFile(filePrefix + ".out");
    if (WIFEXITED(exitStatus) && WEXITSTATUS(exitStatus) == 0) {
        testCaseResult.failures = readFailures(filePrefix + ".err");
    } else if (WIFSIGNALED(exitStatus)) {
        testCaseResult.failures = {"the test case crashed (signal " + std::to_string(WTERMSIG(exitStatus)) + ")"};
    } else {
        testCaseResult.failures = {"the test case ended without reporting (exit status " +
                             std::to_string(WEXITSTATUS(exitStatus)) + ")"};
    }
    return testCaseResult;
}

// Runs the test cases in processes of their own, `maxProcesses` at a time, and calls
// `onFinished(testIndex, testCaseResult)` as each one is done. Only this process writes to the terminal,
// because what the test cases print goes to their .out files.
template <class OnFinished>
void runInProcesses(const std::vector<TestCase>& testCases, const char* sourceFile, std::size_t maxProcesses,
                    OnFinished onFinished) {
    auto filePrefix = [](std::size_t testIndex) { return (tempDirectory() / std::to_string(testIndex)).string(); };

    std::map<pid_t, std::size_t> testOfProcess;
    std::size_t nextTest = 0;
    for (std::size_t finishedTests = 0; finishedTests < testCases.size(); ++finishedTests) {
        while (testOfProcess.size() < maxProcesses && nextTest < testCases.size()) {
            const pid_t process = startTestProcess(testCases[nextTest], sourceFile, filePrefix(nextTest));
            testOfProcess[process] = nextTest++;
        }

        int exitStatus = 0;
        const pid_t process = waitpid(-1, &exitStatus, 0);
        if (process < 0) {
            std::perror("minitest: waitpid");
            std::exit(2);
        }
        const std::size_t testIndex = testOfProcess.at(process);
        testOfProcess.erase(process);
        onFinished(testIndex, collectTestCaseResult(filePrefix(testIndex), exitStatus));
    }
}

#endif

// Runs all the test cases of the suite: side by side in processes of their own, or one after the
// other in this process where that is not possible or not wanted.
inline std::vector<TestCaseResult> runAllTestCases(const std::vector<TestCase>& testCases, const char* sourceFile) {
    std::vector<TestCaseResult> testCaseResults(testCases.size());
    auto onFinished = [&](std::size_t testIndex, TestCaseResult testCaseResult) {
        printProgressDot(testCaseResult.failures.empty());
        testCaseResults[testIndex] = std::move(testCaseResult);
    };

    if (!testCases.empty()) std::printf(" ");
    std::fflush(stdout);

#if defined(__unix__) || defined(__APPLE__)
    const std::size_t maxProcesses = std::min(maxParallelTestCases(), testCases.size());
    if (maxProcesses > 1) {
        runInProcesses(testCases, sourceFile, maxProcesses, onFinished);
        return testCaseResults;
    }
#endif
    for (std::size_t testIndex = 0; testIndex < testCases.size(); ++testIndex) {
        onFinished(testIndex, TestCaseResult{runTestCase(testCases[testIndex], sourceFile), ""});
    }
    return testCaseResults;
}

// Prints how the suite went, and returns whether every test case passed.
inline bool printSummary(const std::string& fileLabel, const std::vector<TestCase>& testCases,
                         const std::vector<TestCaseResult>& testCaseResults, double durationMs) {
    int failedCount = 0;
    for (const TestCaseResult& testCaseResult : testCaseResults) {
        failedCount += testCaseResult.failures.empty() ? 0 : 1;
    }
    const bool allPassed = failedCount == 0;
    const std::size_t total = testCases.size();

    if (allPassed) {
        std::printf(" %s %s %s\n", color::green("✓").c_str(), fileLabel.c_str(),
                     color::gray("(" + std::to_string(total) + (total == 1 ? " test)" : " tests)") + " " + formatMillis(durationMs)).c_str());
    } else {
        std::printf(" %s %s %s\n", color::red("❯").c_str(), fileLabel.c_str(),
                     color::gray("(" + std::to_string(total) + (total == 1 ? " test | " : " tests | ") +
                                  std::to_string(failedCount) + " failed) " + formatMillis(durationMs)).c_str());
        for (std::size_t testIndex = 0; testIndex < total; ++testIndex) {
            const TestCaseResult& testCaseResult = testCaseResults[testIndex];
            if (testCaseResult.failures.empty()) continue;
            std::printf("   %s %s\n", color::red("×").c_str(), testCases[testIndex].name.c_str());
            for (const auto& failure : testCaseResult.failures) {
                std::printf("     %s %s\n", color::gray("→").c_str(), failure.c_str());
            }
            if (!testCaseResult.printed.empty()) {
                std::printf("     %s\n", color::gray("printed:").c_str());
                std::istringstream lines(testCaseResult.printed);
                for (std::string line; std::getline(lines, line);) std::printf("       %s\n", line.c_str());
            }
        }
    }
    std::printf("\n");
    return allPassed;
}

inline int minitestMain(const char* sourceFile) {
    using clock = std::chrono::steady_clock;
    const auto suiteStart = clock::now();
    const auto& testCases = registry();

    tempDirectory() = std::filesystem::temp_directory_path() / ("minitest-" + std::to_string(currentProcessId()));
    std::filesystem::create_directories(tempDirectory());
    const std::vector<TestCaseResult> testCaseResults = runAllTestCases(testCases, sourceFile);
    std::filesystem::remove_all(tempDirectory());

    const double durationMs = std::chrono::duration<double, std::milli>(clock::now() - suiteStart).count();
    if (!testCases.empty()) std::printf("\n");
    return printSummary(shortFileName(sourceFile), testCases, testCaseResults, durationMs) ? 0 : 1;
}

} // namespace minitest

#define TEST_CASE(name)                                                             \
    static void name();                                                             \
    static minitest::Registrar registrar_##name(#name, name);                       \
    static void name()

// Path to tests/fixtures, and to tests/fixtures/<id>.krn, e.g. FIXTURE_CHORALE("chor029").
#define FIXTURES_DIR minitest::fixturesDirPath(__FILE__)
#define FIXTURE_CHORALE(id) minitest::fixtureChoralePath(__FILE__, id)

#define CHECK(cond)                                                                 \
    do {                                                                            \
        ++minitest::checkCount();                                                   \
        if (!(cond)) minitest::reportFailure(__FILE__, __LINE__, "CHECK failed: " #cond); \
    } while (0)

#define CHECK_EQ(a, b)                                                              \
    do {                                                                            \
        ++minitest::checkCount();                                                   \
        auto _mt_a = (a);                                                           \
        auto _mt_b = (b);                                                           \
        if (!(_mt_a == _mt_b)) {                                                    \
            std::string _mt_msg = "CHECK_EQ failed: " #a " == " #b " (got '" +      \
                                   minitest::toDebugString(_mt_a) + "', expected '" + \
                                   minitest::toDebugString(_mt_b) + "')";           \
            minitest::reportFailure(__FILE__, __LINE__, _mt_msg);                   \
        }                                                                           \
    } while (0)

#define CHECK_THROWS(expr)                                                          \
    do {                                                                            \
        ++minitest::checkCount();                                                   \
        bool _mt_threw = false;                                                     \
        try {                                                                       \
            expr;                                                                   \
        } catch (...) {                                                             \
            _mt_threw = true;                                                       \
        }                                                                           \
        if (!_mt_threw)                                                             \
            minitest::reportFailure(__FILE__, __LINE__, "CHECK_THROWS failed: no exception from: " #expr); \
    } while (0)

#define CHECK_NOTHROW(expr)                                                         \
    do {                                                                            \
        ++minitest::checkCount();                                                   \
        try {                                                                       \
            expr;                                                                   \
        } catch (const std::exception& e) {                                         \
            minitest::reportFailure(__FILE__, __LINE__,                             \
                                     std::string("CHECK_NOTHROW failed: threw: ") + e.what()); \
        }                                                                           \
    } while (0)

#define CHECK_RESULT_FIELD(a, b)                                                    \
    do {                                                                            \
        ++minitest::checkCount();                                                   \
        auto _mt_a = (a);                                                           \
        auto _mt_b = (b);                                                           \
        if (!(_mt_a == _mt_b)) {                                                    \
            std::string _mt_msg = "CHECK_RESULT failed: " #a " == " #b " (got '" +  \
                                   minitest::toDebugString(_mt_a) + "', expected '" + \
                                   minitest::toDebugString(_mt_b) + "')";           \
            minitest::reportFailure(__FILE__, __LINE__, _mt_msg);                   \
        }                                                                           \
    } while (0)

#define CHECK_RESULT(r, expChoraleId, expVoice, expStartPosition, expEndPosition)   \
    do {                                                                            \
        CHECK_RESULT_FIELD((r).choraleId, expChoraleId);                            \
        CHECK_RESULT_FIELD((r).voice, expVoice);                                    \
        CHECK_RESULT_FIELD((r).startPosition, expStartPosition);                    \
        CHECK_RESULT_FIELD((r).endPosition, expEndPosition);                        \
    } while (0)

#define REQUIRE(cond)                                                               \
    do {                                                                            \
        ++minitest::checkCount();                                                   \
        if (!(cond)) {                                                              \
            minitest::reportFailure(__FILE__, __LINE__, "REQUIRE failed: " #cond);  \
            throw minitest::RequireFailed();                                        \
        }                                                                           \
    } while (0)

#define TEST_MAIN() \
    int main() { return minitest::minitestMain(__FILE__); }

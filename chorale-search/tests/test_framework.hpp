#pragma once

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <sstream>
#include <string>
#include <vector>

#if defined(__unix__) || defined(__APPLE__)
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

inline int minitestMain(const char* sourceFile) {
    using clock = std::chrono::steady_clock;

    const std::string fileLabel = shortFileName(sourceFile);
    const auto suiteStart = clock::now();

    struct FailedTest {
        std::string name;
        std::vector<std::string> failures;
    };
    std::vector<FailedTest> failedTests;
    std::string dots; // one character per test case, in run order: '.' passed, '×' failed

    const auto& tests = minitest::registry();
    int failedCount = 0;

    for (const auto& tc : tests) {
        minitest::currentTestName() = tc.name;
        minitest::currentTestFailures().clear();
        try {
            tc.fn();
        } catch (const minitest::RequireFailed&) {
            // already reported; test case aborted early
        } catch (const std::exception& e) {
            minitest::reportFailure(sourceFile, 0, std::string("uncaught exception: ") + e.what());
        } catch (...) {
            minitest::reportFailure(sourceFile, 0, "uncaught non-std::exception");
        }
        if (minitest::currentTestFailures().empty()) {
            dots += color::green(".");
        } else {
            ++failedCount;
            failedTests.push_back({tc.name, minitest::currentTestFailures()});
            dots += color::boldRed("×");
        }
    }

    const auto suiteEnd = clock::now();
    const double durationMs = std::chrono::duration<double, std::milli>(suiteEnd - suiteStart).count();

    const bool allPassed = failedCount == 0;
    const std::size_t total = tests.size();

    if (!dots.empty()) std::printf(" %s\n", dots.c_str());
    if (allPassed) {
        std::printf(" %s %s %s\n", color::green("✓").c_str(), fileLabel.c_str(),
                     color::gray("(" + std::to_string(total) + (total == 1 ? " test)" : " tests)") + " " + formatMillis(durationMs)).c_str());
    } else {
        std::printf(" %s %s %s\n", color::red("❯").c_str(), fileLabel.c_str(),
                     color::gray("(" + std::to_string(total) + (total == 1 ? " test | " : " tests | ") +
                                  std::to_string(failedCount) + " failed) " + formatMillis(durationMs)).c_str());
        for (const auto& ft : failedTests) {
            std::printf("   %s %s\n", color::red("×").c_str(), ft.name.c_str());
            for (const auto& msg : ft.failures) {
                std::printf("     %s %s\n", color::gray("→").c_str(), msg.c_str());
            }
        }
    }
    std::printf("\n");

    return allPassed ? 0 : 1;
}

} // namespace minitest

#define TEST_CASE(name)                                                             \
    static void name();                                                             \
    static minitest::Registrar registrar_##name(#name, name);                       \
    static void name()

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

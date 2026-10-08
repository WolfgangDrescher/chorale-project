#pragma once

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#if defined(__unix__) || defined(__APPLE__)
#include <unistd.h>
#endif

// What the tools that take a score ("-" for stdin, or a file) have in common on the command line.

// Whether stdin is a terminal rather than a pipe -- reading it would sit and wait for a human
// to type a score.
inline bool stdinIsInteractive() {
#if defined(__unix__) || defined(__APPLE__)
    return isatty(fileno(stdin)) != 0;
#else
    return false;
#endif
}

inline std::string readInput(const std::string& inputPath) {
    std::ostringstream content;
    if (inputPath == "-") {
        content << std::cin.rdbuf();
    } else {
        std::ifstream file(inputPath, std::ios::binary);
        if (!file.is_open()) throw std::invalid_argument("no such file: " + inputPath);
        content << file.rdbuf();
    }
    return content.str();
}

// true/false, yes/no, y/n or 1/0, in any case.
inline bool parseBoolean(const std::string& flag, const std::string& value) {
    std::string lowered = value;
    std::transform(lowered.begin(), lowered.end(), lowered.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (lowered == "true" || lowered == "yes" || lowered == "y" || lowered == "1") return true;
    if (lowered == "false" || lowered == "no" || lowered == "n" || lowered == "0") return false;
    throw std::invalid_argument(flag + " takes true/false, yes/no, y/n or 1/0, got '" + value + "'");
}

#pragma once

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

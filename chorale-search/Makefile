# Thin convenience wrapper around the CMake build.
#   make deps            download external libraries (pinned by commit hash, see scripts/deps.json)
#   make                 build everything (Release) into build/
#   make debug           build with debug info / no optimization
#   make run-cli ARGS="data --query '{...}'"
#   make clean
#
# No `test`/`test-verbose` target right now -- tests/ is deliberately not
# part of the active build (see CMakeLists.txt); the files are still on
# disk for later.

BUILD_DIR := build
JOBS      ?= $(shell nproc 2>/dev/null || echo 4)

.PHONY: all deps debug clean run-cli
.DEFAULT_GOAL := all

deps:
	./scripts/fetch-deps.sh

all: deps
	cmake -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Release
	cmake --build $(BUILD_DIR) -j$(JOBS)

debug: deps
	cmake -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Debug
	cmake --build $(BUILD_DIR) -j$(JOBS)

clean:
	rm -rf $(BUILD_DIR)

run-cli: all
	./$(BUILD_DIR)/chorale-search $(ARGS)

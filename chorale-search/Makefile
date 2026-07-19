# Thin convenience wrapper around the CMake build.
#
# make deps
#     download external libraries (pinned by commit hash, see scripts/deps.json)
#
# make
#     build everything (Release) into build/
#
# make debug
#     build with debug info / no optimization
#
# make run-cli ARGS="data --query '{...}'"
#
# make test
#     build and run the tests/ suite (see tests/test_framework.hpp). Runs
#     each test binary directly instead of through ctest, so you get its
#     own output with none of ctest's own reporter scaffolding around it
#
# make clean

SHELL     := /bin/bash
BUILD_DIR := build
JOBS      ?= $(shell nproc 2>/dev/null || echo 4)

.PHONY: all deps debug clean run-cli test
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

test: all
	@status=0; \
	while IFS= read -r cmd; do \
		(cd $(BUILD_DIR) && eval "$$cmd") || status=1; \
	done < <(ctest --test-dir $(BUILD_DIR) --show-only=json-v1 | jq -r '.tests[] | (.command | join(" "))'); \
	exit $$status

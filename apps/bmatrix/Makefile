# Defaults match the root Makefile, which forwards
# `make build BUILD_TYPE=... JOBS=...`.
BUILD_TYPE    ?= Debug
JOBS          ?= $(shell nproc)
BUILD_TESTING ?= ON

# The repository root, two levels above either subproject (/workspace).
ROOT_DIR := $(abspath $(CURDIR)/../..)

# Only this project's own sources: the vendored copy under libs/ is generated.
CPP_FILES := $(shell find . -maxdepth 1 -type f -name '*.cpp' 2>/dev/null)
HDR_FILES := $(shell find . -maxdepth 1 -type f -name '*.hpp' 2>/dev/null)

.PHONY: all clean format format-check root test tidy

all:
	cmake -S . -B build -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) -DBUILD_TESTING=$(BUILD_TESTING)
	cmake --build build --parallel $(JOBS)

# `test` depends on `all` because ctest runs the binary but never builds it.
test: all
	ctest --test-dir build --output-on-failure --parallel $(JOBS)

format:
	clang-format -i $(CPP_FILES) $(HDR_FILES)

format-check:
	@clang-format --dry-run --Werror $(CPP_FILES) $(HDR_FILES)

# run-clang-tidy reads the translation units from the build database.
tidy: all
	@run-clang-tidy -p build -j $(JOBS) -quiet

# `root` opens a subshell placed at the repository root. A recipe cannot move
# the shell that invoked make, so it cannot pop the subshell either: to leave
# the subshell opened by `make demo` and be back at the root, plain `exit` does
# it in one step. This target is for going straight to the root from here.
root:
	@cd $(ROOT_DIR) && exec $${SHELL:-bash}

clean:
	cmake --build build --target clean

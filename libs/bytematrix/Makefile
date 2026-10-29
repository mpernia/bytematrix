# Defaults match the root Makefile, which forwards
# `make build BUILD_TYPE=... JOBS=...`.
BUILD_TYPE    ?= Debug
JOBS          ?= $(shell nproc)
BUILD_TESTING ?= ON

# The repository root, two levels above either subproject (/workspace).
ROOT_DIR := $(abspath $(CURDIR)/../..)

# Sources covered by the style and analysis targets.
CPP_FILES := $(shell find src tests -type f -name '*.cpp' 2>/dev/null)
HDR_FILES := $(shell find include src tests -type f \( -name '*.h' -o -name '*.hpp' \) 2>/dev/null)

.PHONY: all clean format format-check root test test-tsan tidy tidy-src

all:
	cmake -S . -B build -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) -DBUILD_TESTING=$(BUILD_TESTING)
	cmake --build build --parallel $(JOBS)

# `test` depends on `all` because ctest runs the binary but never builds it: on
# a clean tree it would report a missing executable instead of a real failure.
# Tests run in parallel: two of them spend 200 ms proving that a push blocks,
# and with --parallel those waits overlap instead of adding up.
test: all
	ctest --test-dir build --output-on-failure --parallel $(JOBS)

# Instrumented and plain objects cannot be mixed, so this needs its own tree;
# build-tsan is already covered by build-*/ in .gitignore.
test-tsan:
	cmake -S . -B build-tsan -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) -DBUILD_TESTING=$(BUILD_TESTING) -DCMAKE_CXX_FLAGS="-fsanitize=thread -g" -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=thread"
	cmake --build build-tsan --parallel $(JOBS)
	TSAN_OPTIONS=halt_on_error=1 ctest --test-dir build-tsan --output-on-failure --parallel $(JOBS)

# `format` rewrites the files. `format-check` only reports and exits non-zero,
# so a hook or a CI job can reject style without touching the tree.
format:
	clang-format -i $(CPP_FILES) $(HDR_FILES)

format-check:
	@clang-format --dry-run --Werror $(CPP_FILES) $(HDR_FILES)

# run-clang-tidy fans the translation units out over -j workers and reads the
# file list from the build database, so CPP_FILES is not needed here. clang-tidy
# itself has no -j: the driver is what parallelises. Measured on the 5 TUs of
# this project, ~145 s in parallel against a serial run that does not finish
# inside 300 s.
tidy: all
	@run-clang-tidy -p build -j $(JOBS) -quiet

# The same analysis restricted to the library sources. The test translation
# units dominate the cost (GoogleTest headers: ~130 s each against ~20 s for the
# sources), so this is the variant cheap enough to run on every save.
tidy-src: all
	@run-clang-tidy -p build -j $(JOBS) -quiet 'src/'

# `root` opens a subshell placed at the repository root. A recipe cannot move
# the shell that invoked make, so it cannot pop the subshell either: to leave
# the subshell opened by `make lib` and be back at the root, plain `exit` does
# it in one step. This target is for going straight to the root from here.
root:
	@cd $(ROOT_DIR) && exec $${SHELL:-bash}

clean:
	cmake --build build --target clean

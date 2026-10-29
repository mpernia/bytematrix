# Defaults match the root Makefile, which forwards
# `make build BUILD_TYPE=... JOBS=...`.
BUILD_TYPE ?= Debug
JOBS       ?= $(shell nproc)

# The repository root, two levels above either subproject (/workspace).
ROOT_DIR := $(abspath $(CURDIR)/../..)

.PHONY: all clean root

all:
	cmake -S . -B build -DCMAKE_BUILD_TYPE=$(BUILD_TYPE)
	cmake --build build --parallel $(JOBS)

# `root` opens a subshell placed at the repository root. A recipe cannot move
# the shell that invoked make, so it cannot pop the subshell either: to leave
# the subshell opened by `make lib` and be back at the root, plain `exit` does
# it in one step. This target is for going straight to the root from here.
root:
	@cd $(ROOT_DIR) && exec $${SHELL:-bash}

clean:
	cmake --build build --target clean

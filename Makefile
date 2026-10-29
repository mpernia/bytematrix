# bytematrix orchestration.
#
# Everything — configure, build, test, run — happens inside the Debian
# development container; the host only needs Docker and GNU Make.
#

SHELL := /bin/bash
.DEFAULT_GOAL := help
BUILD_TYPE ?= Debug
BUILD      ?= build
JOBS       ?= $(shell nproc)
SECONDS    ?= 100

# Paths inside the container (the repository is mounted at /workspace).
WORKDIR := /workspace
PREFIX  := $(WORKDIR)/$(BUILD)/prefix

LIB_DIR    := libs/bytematrix
DEMO_DIR   := apps/bmatrix
VENDOR_DIR := $(DEMO_DIR)/libs/bytematrix

IMAGE_STAMP := docker/.image.stamp
COMPOSE     := docker compose -f docker/docker-compose.yml
EXEC        := $(COMPOSE) exec -T dev
INTERACTIVE := $(COMPOSE) exec dev
RUN         := $(if $(filter 1,$(TTY)),$(INTERACTIVE),$(EXEC))

# ---------------------------------------------------------------------------
# Help
# ---------------------------------------------------------------------------
.PHONY: help
help: ## Show this help
	@printf '\n\033[1mbytematrix\033[0m — C++20 in Docker\n'
	@printf 'libs/bytematrix (static library) + apps/bmatrix (consumer)\n\n'
	@printf '\033[1mTargets\033[0m\n'
	@awk 'BEGIN {FS = ":.*?## "} /^[a-zA-Z0-9_.-]+:.*?## / {printf "  \033[36m%-11s\033[0m %s\n", $$1, $$2}' $(MAKEFILE_LIST)
	@printf '\n\033[1mVars\033[0m\n  BUILD_TYPE=%s  SECONDS=%s  JOBS=%s\n\n' \
		'$(BUILD_TYPE)' '$(SECONDS)' '$(JOBS)'


# ---------------------------------------------------------------------------
# Container lifecycle
# ---------------------------------------------------------------------------
$(IMAGE_STAMP): docker/Dockerfile docker/docker-compose.yml
	$(COMPOSE) build
	@touch $@
	@$(COMPOSE) up -d --force-recreate dev

.PHONY: container
container: $(IMAGE_STAMP) ## Make sure the dev container is running
	@$(COMPOSE) up -d dev

.PHONY: init
init: container ## Build (or refresh) the image and start the container
	@echo "Image ready: bytematrix-dev:local (container up)"

.PHONY: shell
shell: container ## Interactive shell inside the running container
	$(INTERACTIVE) bash

.PHONY: exec
exec: container ## Run an ad-hoc command: make exec CMD="ctest --test-dir build/test"
	@test -n "$(CMD)" || { echo 'Usage: make exec CMD="<shell command>"'; exit 1; }
	$(EXEC) bash -lc '$(CMD)'

.PHONY: down
down: ## Stop and remove the dev container (the image is kept)
	$(COMPOSE) down


# ---------------------------------------------------------------------------
# Build
#
# These targets run INSIDE the dev container: start them from `make shell`.
# A recipe runs in a child process, so a `cd` inside it cannot move the shell
# that invoked make. `lib` and `demo` therefore open a subshell already placed
# in the target directory; leave it with `exit` to come back to /workspace.
# ---------------------------------------------------------------------------
.PHONY: lib
lib: ## Change directory to the static library (no installation)
	@cd $(WORKDIR)/$(LIB_DIR) && exec $${SHELL:-bash}

.PHONY: vendor
vendor: ## Copy the header and the static library into apps/bmatrix/libs/bytematrix
	@mkdir -p $(WORKDIR)/$(VENDOR_DIR)
	@cp $(WORKDIR)/$(LIB_DIR)/bytematrix.h $(WORKDIR)/$(VENDOR_DIR)/
	@cp $(WORKDIR)/$(LIB_DIR)/build/*.a $(WORKDIR)/$(VENDOR_DIR)/

.PHONY: demo
demo: ## Change directory to the demo project
	@cd $(WORKDIR)/$(DEMO_DIR) && exec $${SHELL:-bash}

.PHONY: build
build: ## Compile the library and the demo
	@$(MAKE) --no-print-directory -C $(WORKDIR)/$(LIB_DIR) BUILD_TYPE=$(BUILD_TYPE) JOBS=$(JOBS)
	@$(MAKE) --no-print-directory -C $(WORKDIR) vendor
	@$(MAKE) --no-print-directory -C $(WORKDIR)/$(DEMO_DIR) BUILD_TYPE=$(BUILD_TYPE) JOBS=$(JOBS)

.PHONY: install
install: ## Install the library into build/prefix (for external consumers)
	@mkdir -p $(PREFIX)/lib $(PREFIX)/include
	@cp $(WORKDIR)/$(LIB_DIR)/bytematrix.h $(PREFIX)/include/
	@cp $(WORKDIR)/$(LIB_DIR)/build/*.a $(PREFIX)/lib/



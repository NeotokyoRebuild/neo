# Convenience aliases over the CMake presets in CMakePresets.json. This
# Makefile is not a build system and must not become one: the CMake layer is
# the build, and cmake --preset / cmake --build --preset remain the primary,
# supported way to configure and build. The aliases only add what CMake
# cannot express on its own: running the toolchain inside the build container
# (the host has no cmake or ninja), recording which preset is applied, and
# launching the hot reload sidecar. Anything that affects what gets compiled
# or how belongs in CMakeLists.txt / CMakePresets.json, not here.
#
#   make build [PRESET=linux-debug]        configure if needed, build, deploy, record the applied preset
#   make watch [MODE=save|trigger]         ensure the linux-debug-hotreload preset is applied, run the sidecar
#   make watch-configure                   one-time hot reload setup: container, sidecar, preset
#   make configure [PRESET=...]            configure only
#   make status                            applied preset and sidecar view of the mailbox
#   make clean [PRESET=...]                remove that preset's build dir
#
# The linux-debug and linux-debug-hotreload presets use separate build dirs and
# both deploy to game/neo/bin/linux64, so switching presets is just make build /
# make watch; .ntre-applied-preset records which one is live.

PRESET ?= linux-debug
WATCH_PRESET ?= linux-debug-hotreload
MODE ?= save
JOBS ?= 12
CONTAINER ?= neo-sniper
SDK_DIR ?= $(HOME)/.steam/steam/steamapps/common/Source SDK Base 2013 Multiplayer

REPO_ROOT := $(abspath ..)
DEPLOY_DIR := $(REPO_ROOT)/game/neo/bin/linux64
STAMP := $(DEPLOY_DIR)/.ntre-applied-preset

# The sidecar binary is not vendored; a static musl build works both on the
# host and inside the container. Override with SIDECAR=... or the
# NTRE_HOTRELOAD_SIDECAR env var.
NTRE_HOTRELOAD_SIDECAR ?= $(REPO_ROOT)/.ide/bin/ntre-hr-sidecar
SIDECAR ?= $(NTRE_HOTRELOAD_SIDECAR)
# Checkout of ntre-build-hot-reloader, used by watch-configure to build the sidecar.
RELOADER_DIR ?= $(REPO_ROOT)/../ntre-build-hot-reloader
SNIPER_IMAGE ?= registry.gitlab.steamos.cloud/steamrt/sniper/sdk

# Everything that needs cmake, ninja, g++ or the sidecar runs inside a
# persistent build container that mounts $HOME at the host path (so the repo
# and the Steam SDK are visible) and runs as the host uid; watch-configure
# creates one with plain podman, and a distrobox or toolbox also qualifies.
IN_CONTAINER = podman start $(CONTAINER) >/dev/null && \
	podman exec -i $(shell [ -t 0 ] && echo -t) --user $(shell id -u):$(shell id -g) \
	-e HOME=$(HOME) -w $(CURDIR) $(CONTAINER)

.PHONY: build watch watch-configure configure status clean help

# One-time setup for make watch. Each step is skipped when already done.
watch-configure:
	@podman container exists $(CONTAINER) 2>/dev/null && echo "container: $(CONTAINER)" || { \
		real=$$(realpath "$(HOME)"); extra=""; [ "$$real" = "$(HOME)" ] || extra="-v $$real:$$real"; \
		podman create --name $(CONTAINER) --userns=keep-id --security-opt label=disable \
			-v "$(HOME):$(HOME)" $$extra -v "$(REPO_ROOT):$(REPO_ROOT)" -w "$(REPO_ROOT)" \
			$(SNIPER_IMAGE) sleep infinity >/dev/null && \
		echo "container: created $(CONTAINER) from $(SNIPER_IMAGE), home and the repo mounted at host paths"; }
	@test -x "$(SIDECAR)" && echo "sidecar: $(SIDECAR)" || { \
		test -f "$(RELOADER_DIR)/Makefile" || { echo "sidecar missing and no reloader checkout at $(RELOADER_DIR) (set RELOADER_DIR=...)"; exit 1; }; \
		$(MAKE) --no-print-directory -C "$(RELOADER_DIR)" sidecar-musl && \
		mkdir -p "$(dir $(SIDECAR))" && \
		cp "$(RELOADER_DIR)/sidecar/target/x86_64-unknown-linux-musl/release/ntre-hr-sidecar" "$(SIDECAR)" && \
		echo "sidecar: built from $(RELOADER_DIR) into $(SIDECAR)"; }
	@$(IN_CONTAINER) sh -c 'test -f build/$(WATCH_PRESET)/build.ninja || cmake --preset $(WATCH_PRESET)' && echo "preset: $(WATCH_PRESET) configured"

build:
	@$(IN_CONTAINER) sh -c 'test -f build/$(PRESET)/build.ninja || cmake --preset $(PRESET)'
	$(IN_CONTAINER) cmake --build --preset $(PRESET) -j $(JOBS)
	@printf '%s\n' '$(PRESET)' > $(STAMP)
	@echo "deployed: $(PRESET)"

watch:
	@test -x "$(SIDECAR)" || { \
		echo "sidecar not found at $(SIDECAR)"; \
		echo "get it: make -C <ntre-build-hot-reloader> sidecar-musl,"; \
		echo "then copy sidecar/target/x86_64-unknown-linux-musl/release/ntre-hr-sidecar"; \
		echo "to $(REPO_ROOT)/.ide/bin/ (or set NTRE_HOTRELOAD_SIDECAR)"; \
		exit 1; }
	@applied=$$(cat $(STAMP) 2>/dev/null || echo none); \
	if [ "$$applied" != "$(WATCH_PRESET)" ]; then \
		echo "applied preset is $$applied, building $(WATCH_PRESET) first"; \
		$(MAKE) --no-print-directory build PRESET=$(WATCH_PRESET); \
	else \
		echo "deployed: $$applied"; \
	fi
	$(IN_CONTAINER) "$(SIDECAR)" --build-dir $(CURDIR)/build/$(WATCH_PRESET) watch \
		--mode $(MODE) \
		--link-dir $(DEPLOY_DIR) \
		--link-dir "$(SDK_DIR)/bin/linux64" \
		--never-shim game/shared/neo/hotreload/

configure:
	$(IN_CONTAINER) cmake --preset $(PRESET)

status:
	@echo "deployed: $$(cat $(STAMP) 2>/dev/null || echo none)"
	@test -x "$(SIDECAR)" && $(IN_CONTAINER) "$(SIDECAR)" --build-dir $(CURDIR)/build/$(WATCH_PRESET) status || true

clean:
	rm -rf build/$(PRESET)

help:
	@sed -n '1,19p' Makefile

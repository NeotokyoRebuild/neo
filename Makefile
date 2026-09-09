# Convenience aliases only. Every target here forwards to src/Makefile, and
# src/Makefile in turn only wraps the CMake presets in src/CMakePresets.json.
# Nothing in either Makefile is part of the build itself: the CMake layer is
# the build, and cmake --preset / cmake --build --preset stay the primary,
# supported way to configure and build. Do not add build logic here.

%:
	@$(MAKE) -C src $@

.DEFAULT_GOAL := help

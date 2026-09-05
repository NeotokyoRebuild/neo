# Vendored loader core

Source: ntre-build-hot-reloader at `8587f7d`, copied 2026-08-26 by scripts/vendor.sh.

Do not edit these files here. Change the reloader repo, re-run
`scripts/vendor.sh <this dir>`, review the diff, commit.

Consumer build: add this directory to the include path and compile `src/*.cpp`
into the module with the module's own flags. Only `ntre_hr.h` is meant to be
included by consumer code.

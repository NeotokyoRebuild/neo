#!/bin/bash
#                    ████
#       ████████    ██████████     ▓▓▓▓
#     ████████████  ███████████   ▓▓▓▓▓▓
#    █████░░░█████░ █████░░░░░░░  ▓▓▓▓▓▓░
#    █████░  █████░ █████░         ▓▓▓▓░░
#    █████░  █████░ █████░          ░░░░
#    █████░  █████░ ██████████      █████
#    █████░  █████░ ██████████░     █████░
#     ░░░░░  █████░  ░░░░░░░░░░    ████░░░
#            █████░               ███░░░
#             ░░░░░                ░░░
#
#      N E O T O K Y O ; R E B U I L D

# Repairs an NT;RE Linux install whose engine binaries have fully
# lowercased file names. The engine loads these binaries by exact
# mixed-case name, and Steam neither renames files on disk nor fails
# verification over casing, so a broken install stays broken until the
# files are renamed. Run from (or point at) the game install root, the
# directory that contains bin/ and neo/:
#
#     ./fix-linux-casing.sh ~/.local/share/Steam/steamapps/common/NEOTOKYOREBUILD
#
# Safe to run repeatedly. Prints every change it makes.
# The canonical list matches src/launcher_main/neo_case_heal.cpp
# (PR #2055, the launcher-side repair that runs at every startup).

set -u

# Show the logo when running in a terminal. The helper lives next to this
# script in the repo; a standalone copy of this script skips it silently.
[ -f "$(dirname "$0")/nt-logo.sh" ] && . "$(dirname "$0")/nt-logo.sh" && nt_logo

root="${1:-.}"

if [ ! -d "$root/bin" ] || [ ! -d "$root/neo" ]; then
    echo "error: $root does not look like an NT;RE install root (needs bin/ and neo/)" >&2
    exit 2
fi

canonical_paths=(
    "bin/GameUI.so"
    "bin/ServerBrowser.so"
    "bin/libMiles.so"
    "bin/libTelemetryX64.so"
    "bin/libTelemetryX86.so"
    "bin/linux64/GameUI.so"
    "bin/linux64/ServerBrowser.so"
)

repairs=0
for canon in "${canonical_paths[@]}"; do
    lower=$(printf '%s' "$canon" | tr '[:upper:]' '[:lower:]')
    [ "$lower" = "$canon" ] && continue
    lower_path="$root/$lower"
    canon_path="$root/$canon"
    [ -e "$lower_path" ] || [ -L "$lower_path" ] || continue

    if [ ! -e "$canon_path" ] && [ ! -L "$canon_path" ]; then
        mv "$lower_path" "$canon_path" && echo "renamed: $lower -> $canon" && repairs=$((repairs+1))
    elif [ "$lower_path" -nt "$canon_path" ]; then
        # The lowercased twin is newer, so Steam wrote it last and it
        # carries the current content. Replace the canonical file.
        mv -f "$lower_path" "$canon_path" && echo "replaced with newer: $canon" && repairs=$((repairs+1))
    else
        rm -f "$lower_path" && echo "removed stale twin: $lower" && repairs=$((repairs+1))
    fi
done

if [ "$repairs" -eq 0 ]; then
    echo "Nothing to repair. File name casing is already correct."
else
    echo "Done. $repairs file(s) repaired."
fi

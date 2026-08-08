#!/bin/sh
# Checks a depot staging tree for engine binaries staged under the wrong
# file name casing. The engine loads these binaries by exact mixed-case
# name, and Steam never repairs casing on installs, so a miscased file
# in one upload breaks fresh Linux installs until the next full fix.
# Run this against the staging root before every steamcmd upload:
#
#     ./check-depot-casing.sh /path/to/staging/root
#
# Exits 0 when clean, 1 with one line per offending file otherwise.
# Keep the list below in sync with src/launcher_main/neo_case_heal.cpp
# (PR #2055, the launcher-side repair for already-broken installs).

root="${1:?usage: check-depot-casing.sh <staging-root>}"

canonical_paths="
bin/GameUI.so
bin/ServerBrowser.so
bin/libMiles.so
bin/libTelemetryX64.so
bin/libTelemetryX86.so
bin/linux64/GameUI.so
bin/linux64/ServerBrowser.so
"

status=0
for canon in $canonical_paths; do
    dir=$(dirname "$canon")
    want=$(basename "$canon")
    want_lower=$(printf '%s' "$want" | tr '[:upper:]' '[:lower:]')
    [ -d "$root/$dir" ] || continue
    for path in "$root/$dir"/*; do
        [ -e "$path" ] || continue
        have=$(basename "$path")
        have_lower=$(printf '%s' "$have" | tr '[:upper:]' '[:lower:]')
        if [ "$have_lower" = "$want_lower" ] && [ "$have" != "$want" ]; then
            echo "WRONG CASING: $dir/$have (expected $dir/$want)" >&2
            status=1
        fi
    done
done

if [ "$status" -eq 0 ]; then
    echo "Depot casing OK."
fi
exit $status

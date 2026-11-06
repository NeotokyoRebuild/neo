#!/usr/bin/env bash
#
# Runs the map compile tools (vbsp, vvis, vrad) over every combination of
#
#   * where the tool binaries are run from:
#       build-tree      straight out of the CMake build tree, relying on the rpath
#                       entries added by add_origin_rpath() (needs the build to be
#                       configured with NEO_TOOLS_ENGINE_BIN_PATH)
#       engine-bin      from an engine's bin/linux64 directory, which is how the
#                       NEO_INSTALL_UTILS package is meant to be unpacked. A symlink
#                       mirror of the engine directory is used so that nothing is
#                       written into the Steam installation.
#       library-path    from an unrelated directory with LD_LIBRARY_PATH pointing at
#                       the engine binaries
#
#   * what kind of game directory they are pointed at with -game:
#       sourcemod       steamapps/sourcemods/neo, no engine next to it, so the engine
#                       is resolved from the SteamAppId in gameinfo.txt
#       source-tree     game/neo in this repository, same engine lookup as above
#       ntre            the Steam "NeotokyoRebuild" installation, which has the engine
#                       binaries sitting right next to the game directory
#       no-engine       negative test: a gameinfo.txt naming an appid that isn't
#                       installed, which every tool has to reject
#
# See the "Map compile tools" section of README.md for what these layouts are about.
#
# Usage: utils/test_tools.sh [options]
# Run with --help for the option list.

set -uo pipefail

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)
SRC_DIR=$(cd "$SCRIPT_DIR/.." && pwd -P)
REPO_DIR=$(cd "$SRC_DIR/.." && pwd -P)

PLATSUBDIR="linux64"

# Source SDK Base 2013 Multiplayer, the engine the mod's gameinfo.txt asks for
SDK_APPID=243750
# The Steam "NeotokyoRebuild" release
NTRE_APPID=3172910
# Nothing is ever installed under this one
BOGUS_APPID=99999999

DEFAULT_LOCATIONS=(build-tree engine-bin library-path)
DEFAULT_GAME_TYPES=(sourcemod source-tree ntre no-engine)
DEFAULT_TOOLS=(vbsp vvis vrad)

BUILD_DIR=""
MAP_FILE=""
STEAM_DIR=""
SDK_DIR=""
NTRE_DIR=""
SOURCEMOD_DIR=""
SOURCE_TREE_GAME_DIR="$REPO_DIR/game/neo"
WORK_DIR=""
KEEP_WORK_DIR=0
LIST_ONLY=0
TIMEOUT_SECS=900

LOCATIONS=("${DEFAULT_LOCATIONS[@]}")
GAME_TYPES=("${DEFAULT_GAME_TYPES[@]}")
TOOLS=("${DEFAULT_TOOLS[@]}")

# Only devbox.vmf is known to reference an HL2 skybox, which is what proves that the
# engine content actually got mounted. Any other map only gets the weaker checks.
CHECK_ENGINE_CONTENT=1
ENGINE_CONTENT_MARKER="sky_day01_01"

declare -A RESULTS=()
declare -A LOCATION_DIR=()
declare -A GAME_DIR=()
declare -A GAME_ENGINE_DIR=()
declare -A SKIP_REASON=()

usage()
{
    cat <<EOF
Usage: ${BASH_SOURCE[0]##*/} [options]

  --build-dir DIR      CMake build directory holding the tools
                       (default: the newest of $SRC_DIR/build/linux-*)
  --map FILE           .vmf to compile (default: game/neo/mapsrc/devbox.vmf)
  --steam-dir DIR      Steam installation to look the games up in (autodetected)
  --sdk-dir DIR        "Source SDK Base 2013 Multiplayer" installation (autodetected)
  --ntre-dir DIR       "NeotokyoRebuild" installation (autodetected)
  --sourcemod-dir DIR  steamapps/sourcemods/neo (autodetected, simulated if missing)
  --game-dir DIR       source tree game directory (default: $SOURCE_TREE_GAME_DIR)
  --work-dir DIR       scratch directory to use (default: a new one under \$TMPDIR)
  --locations LIST     comma separated subset of: ${DEFAULT_LOCATIONS[*]}
  --game-types LIST    comma separated subset of: ${DEFAULT_GAME_TYPES[*]}
  --tools LIST         comma separated subset of: ${DEFAULT_TOOLS[*]}
  --timeout SECS       per tool timeout, 0 disables (default: $TIMEOUT_SECS)
  --keep               keep the work directory (logs and compiled maps) around
  --list               only print what was detected, then exit
  -h, --help           this text
EOF
}

log()  { printf '%s\n' "$*"; }
warn() { printf 'warning: %s\n' "$*" >&2; }
die()  { printf 'error: %s\n' "$*" >&2; exit 1; }

split_list()
{
    local IFS=','
    # shellcheck disable=SC2206
    printf '%s\n' $1
}

while [[ $# -gt 0 ]]; do
    case $1 in
        --build-dir)     BUILD_DIR=$2; shift 2 ;;
        --map)           MAP_FILE=$2; shift 2 ;;
        --steam-dir)     STEAM_DIR=$2; shift 2 ;;
        --sdk-dir)       SDK_DIR=$2; shift 2 ;;
        --ntre-dir)      NTRE_DIR=$2; shift 2 ;;
        --sourcemod-dir) SOURCEMOD_DIR=$2; shift 2 ;;
        --game-dir)      SOURCE_TREE_GAME_DIR=$2; shift 2 ;;
        --work-dir)      WORK_DIR=$2; shift 2 ;;
        --locations)     mapfile -t LOCATIONS < <(split_list "$2"); shift 2 ;;
        --game-types)    mapfile -t GAME_TYPES < <(split_list "$2"); shift 2 ;;
        --tools)         mapfile -t TOOLS < <(split_list "$2"); shift 2 ;;
        --timeout)       TIMEOUT_SECS=$2; shift 2 ;;
        --keep)          KEEP_WORK_DIR=1; shift ;;
        --list)          LIST_ONLY=1; shift ;;
        -h|--help)       usage; exit 0 ;;
        *)               usage >&2; die "unknown option '$1'" ;;
    esac
done

# ---------------------------------------------------------------------------- #
# Locating things
# ---------------------------------------------------------------------------- #

detect_steam_dir()
{
    local candidates=() dir
    [[ -n ${XDG_DATA_HOME:-} ]] && candidates+=("$XDG_DATA_HOME/Steam")
    candidates+=(
        "$HOME/.steam/steam"
        "$HOME/.steam/root"
        "$HOME/.local/share/Steam"
        "$HOME/.steam/debian-installation"
        "$HOME/.var/app/com.valvesoftware.Steam/data/Steam"
    )

    for dir in "${candidates[@]}"; do
        if [[ -f "$dir/config/libraryfolders.vdf" ]]; then
            printf '%s\n' "$(cd "$dir" && pwd -P)"
            return 0
        fi
    done

    return 1
}

steam_libraries()
{
    printf '%s\n' "$STEAM_DIR"
    sed -n 's/.*"path"[[:space:]]*"\(.*\)".*/\1/p' "$STEAM_DIR/config/libraryfolders.vdf" 2>/dev/null
}

# Resolve an installed app the same way filesystem_init.cpp does: walk the library
# folders, read the app manifest, then check the directory really is there.
find_steam_app()
{
    local appid=$1 library manifest installdir

    while read -r library; do
        [[ -n $library ]] || continue
        manifest="$library/steamapps/appmanifest_$appid.acf"
        [[ -f $manifest ]] || continue
        installdir=$(sed -n 's/.*"installdir"[[:space:]]*"\(.*\)".*/\1/p' "$manifest" | head -n1)
        [[ -n $installdir ]] || continue
        [[ -d "$library/steamapps/common/$installdir" ]] || continue
        printf '%s\n' "$library/steamapps/common/$installdir"
        return 0
    done < <(steam_libraries | awk '!seen[$0]++')

    return 1
}

detect_build_dir()
{
    local dir newest=""
    for dir in "$SRC_DIR"/build/linux-*; do
        [[ -x "$dir/utils/vbsp/vbsp" ]] || continue
        if [[ -z $newest || "$dir/utils/vbsp/vbsp" -nt "$newest/utils/vbsp/vbsp" ]]; then
            newest=$dir
        fi
    done
    [[ -n $newest ]] || return 1
    printf '%s\n' "$newest"
}

if [[ -z $BUILD_DIR ]]; then
    BUILD_DIR=$(detect_build_dir) || die "no build directory with a built vbsp found, pass --build-dir"
fi
BUILD_DIR=$(cd "$BUILD_DIR" && pwd -P) || die "--build-dir '$BUILD_DIR' does not exist"

for tool_path in \
    "$BUILD_DIR/utils/vbsp/vbsp" \
    "$BUILD_DIR/utils/vvis_launcher/vvis" \
    "$BUILD_DIR/utils/vrad_launcher/vrad" \
    "$BUILD_DIR/utils/vvis/libvvis.so" \
    "$BUILD_DIR/utils/vrad/libvrad.so"
do
    [[ -e $tool_path ]] || die "$tool_path is missing, build the utils first"
done

if [[ -z $STEAM_DIR ]]; then
    STEAM_DIR=$(detect_steam_dir) || warn "no Steam installation found"
fi

if [[ -z $SDK_DIR && -n $STEAM_DIR ]]; then
    SDK_DIR=$(find_steam_app "$SDK_APPID") || true
fi

if [[ -z $NTRE_DIR && -n $STEAM_DIR ]]; then
    NTRE_DIR=$(find_steam_app "$NTRE_APPID") || true
fi

if [[ -z $SOURCEMOD_DIR && -n $STEAM_DIR ]]; then
    if [[ -f "$STEAM_DIR/steamapps/sourcemods/neo/gameinfo.txt" ]]; then
        SOURCEMOD_DIR="$STEAM_DIR/steamapps/sourcemods/neo"
    fi
fi

if [[ -z $MAP_FILE ]]; then
    MAP_FILE="$SOURCE_TREE_GAME_DIR/mapsrc/devbox.vmf"
else
    CHECK_ENGINE_CONTENT=0
fi
[[ -f $MAP_FILE ]] || die "map '$MAP_FILE' not found, pass --map"

MAP_NAME=$(basename "$MAP_FILE" .vmf)

# Each tool works on the previous one's output, so they always run in this order
# no matter how --tools was spelled
ordered_tools=()
for tool in "${DEFAULT_TOOLS[@]}"; do
    for selected in "${TOOLS[@]}"; do
        [[ $tool == "$selected" ]] && ordered_tools+=("$tool") && break
    done
done
[[ ${#ordered_tools[@]} -gt 0 ]] || die "no known tool selected, --tools takes: ${DEFAULT_TOOLS[*]}"
TOOLS=("${ordered_tools[@]}")

log "source tree:      $REPO_DIR"
log "build directory:  $BUILD_DIR"
log "map:              $MAP_FILE"
log "Steam:            ${STEAM_DIR:-<not found>}"
log "SDK Base 2013 MP: ${SDK_DIR:-<not installed>}"
log "NeotokyoRebuild:  ${NTRE_DIR:-<not installed>}"
log "sourcemods/neo:   ${SOURCEMOD_DIR:-<not installed, will be simulated>}"

if [[ -L "$BUILD_DIR/utils/vbsp/engine" ]]; then
    log "build tree engine: $(readlink "$BUILD_DIR/utils/vbsp/engine")"
else
    log "build tree engine: <none, NEO_TOOLS_ENGINE_BIN_PATH was not configured>"
fi
log ""

[[ $LIST_ONLY -eq 1 ]] && exit 0

# ---------------------------------------------------------------------------- #
# Work directory
# ---------------------------------------------------------------------------- #

if [[ -z $WORK_DIR ]]; then
    # No dots: the tools run the whole map path through Q_DefaultExtension, so a dot
    # anywhere in it makes them skip appending .vmf
    WORK_DIR=$(mktemp -d "${TMPDIR:-/tmp}/neo-test-tools-XXXXXX") || die "can't create a work directory"
else
    mkdir -p "$WORK_DIR" || die "can't create '$WORK_DIR'"
    WORK_DIR=$(cd "$WORK_DIR" && pwd -P)
fi

case ${WORK_DIR#/} in
    *.*) die "work directory '$WORK_DIR' contains a '.', which the tools can't handle in a map path" ;;
esac

cleanup()
{
    if [[ $KEEP_WORK_DIR -eq 1 ]]; then
        log "work directory kept at $WORK_DIR"
    else
        rm -rf "$WORK_DIR"
    fi
}
trap cleanup EXIT

log "work directory:   $WORK_DIR"
log ""

stage_tools()
{
    local dest=$1
    mkdir -p "$dest" || return 1
    cp -f "$BUILD_DIR/utils/vbsp/vbsp"          "$dest/" || return 1
    cp -f "$BUILD_DIR/utils/vvis_launcher/vvis" "$dest/" || return 1
    cp -f "$BUILD_DIR/utils/vrad_launcher/vrad" "$dest/" || return 1
    cp -f "$BUILD_DIR/utils/vvis/libvvis.so"    "$dest/" || return 1
    cp -f "$BUILD_DIR/utils/vrad/libvrad.so"    "$dest/" || return 1
}

# Symlink farm standing in for "the utils package was unpacked over an engine
# installation", without touching the installation itself.
mirror_engine_bin()
{
    local engine_bin=$1 dest=$2 entry
    mkdir -p "$dest" || return 1
    for entry in "$engine_bin"/*; do
        [[ -e $entry ]] || continue
        ln -sfn "$entry" "$dest/$(basename "$entry")" || return 1
    done
    stage_tools "$dest"
}

# ---------------------------------------------------------------------------- #
# Game directories
# ---------------------------------------------------------------------------- #

if [[ -n $SOURCEMOD_DIR ]]; then
    GAME_DIR[sourcemod]=$SOURCEMOD_DIR
elif [[ -d $SOURCE_TREE_GAME_DIR ]]; then
    # Same layout as steamapps/sourcemods/neo: a mod directory with no engine next
    # to it, which is all the engine lookup cares about.
    mkdir -p "$WORK_DIR/sourcemods"
    ln -sfn "$SOURCE_TREE_GAME_DIR" "$WORK_DIR/sourcemods/neo"
    GAME_DIR[sourcemod]="$WORK_DIR/sourcemods/neo"
    log "note: simulating steamapps/sourcemods/neo at ${GAME_DIR[sourcemod]}"
else
    SKIP_REASON[sourcemod]="no sourcemods/neo and no source tree game directory"
fi

if [[ -d $SOURCE_TREE_GAME_DIR ]]; then
    GAME_DIR[source-tree]=$SOURCE_TREE_GAME_DIR
else
    SKIP_REASON[source-tree]="'$SOURCE_TREE_GAME_DIR' does not exist"
fi

if [[ -n $NTRE_DIR && -d "$NTRE_DIR/neo" ]]; then
    GAME_DIR[ntre]="$NTRE_DIR/neo"
else
    SKIP_REASON[ntre]="NeotokyoRebuild is not installed"
fi

# A valid mod whose engine simply isn't installed. The tools have to bail out on the
# engine lookup, before they ever look at any content.
mkdir -p "$WORK_DIR/no-engine/neo"
cat > "$WORK_DIR/no-engine/neo/gameinfo.txt" <<EOF
"GameInfo"
{
	game		"engine lookup test"
	FileSystem
	{
		SteamAppId	$BOGUS_APPID
		SearchPaths
		{
			Game	.
		}
	}
}
EOF
GAME_DIR[no-engine]="$WORK_DIR/no-engine/neo"

# The engine each game type ends up running on, which is also the one the tool
# binaries have to be able to load their libraries from.
GAME_ENGINE_DIR[sourcemod]=${SDK_DIR:+$SDK_DIR/bin/$PLATSUBDIR}
GAME_ENGINE_DIR[source-tree]=${SDK_DIR:+$SDK_DIR/bin/$PLATSUBDIR}
GAME_ENGINE_DIR[ntre]=${NTRE_DIR:+$NTRE_DIR/bin/$PLATSUBDIR}
GAME_ENGINE_DIR[no-engine]=${SDK_DIR:+$SDK_DIR/bin/$PLATSUBDIR}

for game_type in sourcemod source-tree no-engine; do
    if [[ -n ${GAME_DIR[$game_type]:-} && -z ${GAME_ENGINE_DIR[$game_type]:-} ]]; then
        unset "GAME_DIR[$game_type]"
        SKIP_REASON[$game_type]="Source SDK Base 2013 Multiplayer is not installed"
    fi
done

# ---------------------------------------------------------------------------- #
# Running
# ---------------------------------------------------------------------------- #

# Returns the binary to run for a tool, which lives in a per tool directory in the
# build tree and in one shared directory everywhere else.
tool_binary()
{
    local location=$1 tool=$2

    if [[ $location == build-tree ]]; then
        case $tool in
            vbsp) printf '%s\n' "$BUILD_DIR/utils/vbsp/vbsp" ;;
            vvis) printf '%s\n' "$BUILD_DIR/utils/vvis_launcher/vvis" ;;
            vrad) printf '%s\n' "$BUILD_DIR/utils/vrad_launcher/vrad" ;;
        esac
        return
    fi

    printf '%s\n' "${LOCATION_DIR[$location]}/$tool"
}

# Sets up the directory the tools are run from for a location, once per engine.
prepare_location()
{
    local location=$1 engine_bin=$2 key dest

    case $location in
        build-tree)
            LOCATION_DIR[$location]="$BUILD_DIR"
            return 0
            ;;
        engine-bin)
            key="engine-bin:$engine_bin"
            dest="$WORK_DIR/engine-mirror/$(basename "$(dirname "$(dirname "$engine_bin")")")"
            ;;
        library-path)
            key="library-path"
            dest="$WORK_DIR/staged-tools"
            ;;
        *)
            return 1
            ;;
    esac

    if [[ -n ${LOCATION_DIR[$key]:-} ]]; then
        LOCATION_DIR[$location]=${LOCATION_DIR[$key]}
        return 0
    fi

    if [[ $location == engine-bin ]]; then
        mirror_engine_bin "$engine_bin" "$dest" || return 1
    else
        stage_tools "$dest" || return 1
    fi

    LOCATION_DIR[$key]=$dest
    LOCATION_DIR[$location]=$dest
    return 0
}

run_tool()
{
    local location=$1 game_type=$2 tool=$3
    local engine_bin=${GAME_ENGINE_DIR[$game_type]}
    local game_dir=${GAME_DIR[$game_type]}
    local out_dir="$WORK_DIR/out/$location-$game_type"
    local log_file="$out_dir/$tool.log"
    local binary args=() env=() timeout_cmd=()

    binary=$(tool_binary "$location" "$tool")

    if [[ $location == library-path ]]; then
        env=(env "LD_LIBRARY_PATH=$engine_bin${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}")
    fi

    if [[ $TIMEOUT_SECS -gt 0 ]] && command -v timeout >/dev/null 2>&1; then
        timeout_cmd=(timeout "$TIMEOUT_SECS")
    fi

    case $tool in
        vbsp) args=(-game "$game_dir" "$out_dir/$MAP_NAME") ;;
        vvis) args=(-game "$game_dir" -fast "$out_dir/$MAP_NAME") ;;
        vrad) args=(-game "$game_dir" -fast "$out_dir/$MAP_NAME") ;;
    esac

    "${env[@]}" "${timeout_cmd[@]}" "$binary" "${args[@]}" > "$log_file" 2>&1
    return $?
}

check_tool_output()
{
    local tool=$1 log_file=$2 out_dir=$3

    if grep -q -e "Can't load" \
               -e "Couldn't find appid" \
               -e "Failed to load content for steam AppID" \
               -e "doesn't say which engine to use" "$log_file"; then
        printf '%s\n' "the engine could not be resolved"
        return 1
    fi

    case $tool in
        vbsp)
            if [[ ! -s "$out_dir/$MAP_NAME.bsp" ]]; then
                printf '%s\n' "no $MAP_NAME.bsp was written"
                return 1
            fi
            if [[ $CHECK_ENGINE_CONTENT -eq 1 ]] && ! grep -q "$ENGINE_CONTENT_MARKER" "$log_file"; then
                # The default map's skybox only exists in the engine's hl2 content, so
                # not seeing it means the wrong base directory got mounted.
                printf '%s\n' "engine content was not mounted ($ENGINE_CONTENT_MARKER not used)"
                return 1
            fi
            ;;
        vvis)
            if ! grep -q "visdatasize" "$log_file"; then
                printf '%s\n' "no visibility data was written"
                return 1
            fi
            ;;
        vrad)
            if ! grep -q "BuildFacelights" "$log_file" || \
               ! grep -qiE "writing .*\.bsp" "$log_file"; then
                printf '%s\n' "no lighting data was written"
                return 1
            fi
            ;;
    esac

    return 0
}

run_case()
{
    local location=$1 game_type=$2 tool
    local out_dir="$WORK_DIR/out/$location-$game_type"
    local key exit_code reason

    mkdir -p "$out_dir"
    cp -f "$MAP_FILE" "$out_dir/$MAP_NAME.vmf" || return

    for tool in "${TOOLS[@]}"; do
        key="$location/$game_type/$tool"

        printf '  %-12s %-12s %-5s ... ' "$location" "$game_type" "$tool"

        run_tool "$location" "$game_type" "$tool"
        exit_code=$?

        if [[ $game_type == no-engine ]]; then
            # Everything here has to fail, and it has to fail on the engine lookup
            if [[ $exit_code -eq 0 ]]; then
                RESULTS[$key]="FAIL: exited 0 without a usable engine"
                printf 'FAIL (exited 0 without a usable engine)\n'
            elif ! grep -q -e "Couldn't find appid" -e "Can't load" \
                           "$out_dir/$tool.log"; then
                RESULTS[$key]="FAIL: failed for an unexpected reason"
                printf 'FAIL (failed for an unexpected reason)\n'
            else
                RESULTS[$key]="PASS"
                printf 'PASS (rejected as expected)\n'
            fi
            # Only vbsp is worth running, the others have no .bsp to chew on
            break
        fi

        if [[ $exit_code -ne 0 ]]; then
            RESULTS[$key]="FAIL: exit code $exit_code"
            printf 'FAIL (exit code %s)\n' "$exit_code"
            tail -n 15 "$out_dir/$tool.log" | sed 's/^/      | /'
            # The following tools work on this one's output, so stop here
            break
        fi

        reason=$(check_tool_output "$tool" "$out_dir/$tool.log" "$out_dir")
        if [[ -n $reason ]]; then
            RESULTS[$key]="FAIL: $reason"
            printf 'FAIL (%s)\n' "$reason"
            tail -n 15 "$out_dir/$tool.log" | sed 's/^/      | /'
            break
        fi

        RESULTS[$key]="PASS"
        printf 'PASS\n'
    done
}

for location in "${LOCATIONS[@]}"; do
    for game_type in "${GAME_TYPES[@]}"; do
        key="$location/$game_type"

        if [[ -z ${GAME_DIR[$game_type]:-} ]]; then
            RESULTS[$key]="SKIP: ${SKIP_REASON[$game_type]:-not available}"
            printf '  %-12s %-12s ... SKIP (%s)\n' \
                "$location" "$game_type" "${SKIP_REASON[$game_type]:-not available}"
            continue
        fi

        if [[ $location == build-tree && ! -L "$BUILD_DIR/utils/vbsp/engine" ]]; then
            RESULTS[$key]="SKIP: build was configured without NEO_TOOLS_ENGINE_BIN_PATH"
            printf '  %-12s %-12s ... SKIP (%s)\n' \
                "$location" "$game_type" "build was configured without NEO_TOOLS_ENGINE_BIN_PATH"
            continue
        fi

        if ! prepare_location "$location" "${GAME_ENGINE_DIR[$game_type]}"; then
            RESULTS[$key]="SKIP: could not stage the tools for this location"
            printf '  %-12s %-12s ... SKIP (%s)\n' \
                "$location" "$game_type" "could not stage the tools for this location"
            continue
        fi

        run_case "$location" "$game_type"
    done
done

# ---------------------------------------------------------------------------- #
# Summary
# ---------------------------------------------------------------------------- #

log ""
log "Summary"
log "-------"

failed=0
skipped=0
passed=0

for key in $(printf '%s\n' "${!RESULTS[@]}" | sort); do
    result=${RESULTS[$key]}
    case $result in
        PASS)  passed=$((passed + 1)) ;;
        SKIP*) skipped=$((skipped + 1)) ;;
        *)     failed=$((failed + 1)) ;;
    esac
    printf '  %-40s %s\n' "$key" "$result"
done

log ""
log "$passed passed, $failed failed, $skipped skipped"

[[ $failed -eq 0 ]]

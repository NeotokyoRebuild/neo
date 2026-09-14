#!/usr/bin/env bash
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
#
# Bootstraps C++ IntelliSense: a compile_commands.json symlinked at the repo root
# and a .clangd beside it - the two interfaces every C++ indexer understands.
#
# Why a separate CMake directory: a unity build's database lists only generated
# unity_*.cxx blobs, so ~1300 translation units get no flags at all. This preset
# turns NEO_UNITY_BUILD_CLIENT_SERVER and NEO_UNITY_BUILD_OTHERS off for the
# database alone, leaving your real build directory configured however you like;
# nothing is compiled from it.
#
# Safe to re-run; it skips what is already in place. See --help.

set -euo pipefail

CLANGD_VERSION="22.1.6"
CLANGD_SHA256_LINUX_X86_64="a9c77443af2e447ed467e84771848d3a6ac1c56f84bcfcde717e66318de77cfa"
# clangd older than this mishandles the C++20 this tree is built with.
CLANGD_MIN_MAJOR=17

PRESET="linux-debug-ide"
BASE_PRESET="linux-debug"
# Globbed for the GCC 10 and Clang 19 in CONTRIBUTING.md, which install as
# gcc-10, clang-19, /usr/lib/llvm-19/bin/clang++. A driver clangd cannot query
# leaves it guessing the standard library paths.
QUERY_DRIVER='/usr/bin/g++*,/usr/bin/gcc*,/usr/bin/clang*,/usr/lib/llvm-*/bin/clang*'
EDITOR_TARGET="auto"
USE_SYSTEM_CLANGD=0
RECONFIGURE=0
FORCE=0
CLANGD_VERSION_OVERRIDDEN=0
CLANGD_SHA256_OVERRIDE=""

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
IDE_DIR="${REPO_ROOT}/.ide"
SRC_DIR="${REPO_ROOT}/src"
BUILD_DIR="${SRC_DIR}/build/${PRESET}"

usage() {
    cat <<'EOF'
Usage: tools/ntre-dev-setup.sh [options]

  --editor <auto|vscode|zed|none>
                               Write editor-specific config too. "auto" (default)
                               picks VS Code or Zed when run from that editor's
                               terminal or task. Every editor works without this;
                               it just saves pointing your client at
                               .ide/bin/clangd by hand.
  --system-clangd              Use clangd from PATH instead of downloading a
                               pinned one (must be >= major 17).
  --clangd-version <ver>       Override the pinned clangd release. Downloads
                               unverified unless --clangd-sha256 is also given.
  --clangd-sha256 <hash>       SHA-256 of the linux-x86_64 zip for an overridden
                               --clangd-version.
  --preset <name>              IntelliSense preset name (default linux-debug-ide).
  --base-preset <name>         Preset it inherits build flags from (default linux-debug).
  --reconfigure                Regenerate the compile database even if it exists.
                               Do this after adding/removing/renaming sources.
  --force                      Overwrite generated config files (.clangd etc).
  -h, --help                   This text.
EOF
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --editor) EDITOR_TARGET="$2"; shift 2 ;;
        --system-clangd) USE_SYSTEM_CLANGD=1; shift ;;
        --clangd-version) CLANGD_VERSION="$2"; CLANGD_VERSION_OVERRIDDEN=1; shift 2 ;;
        --clangd-sha256) CLANGD_SHA256_OVERRIDE="$2"; shift 2 ;;
        --preset) PRESET="$2"; BUILD_DIR="${SRC_DIR}/build/${PRESET}"; shift 2 ;;
        --base-preset) BASE_PRESET="$2"; shift 2 ;;
        --reconfigure) RECONFIGURE=1; shift ;;
        --force) FORCE=1; shift ;;
        -h|--help) usage; exit 0 ;;
        *) echo "unknown option: $1" >&2; usage >&2; exit 2 ;;
    esac
done

# The pinned hash only vouches for the pinned version; an override replaces both.
if [[ $CLANGD_VERSION_OVERRIDDEN -eq 1 ]]; then
    CLANGD_SHA256_LINUX_X86_64="$CLANGD_SHA256_OVERRIDE"
fi

say()  { printf '\033[1m==>\033[0m %s\n' "$*"; }
warn() { printf '\033[33mwarning:\033[0m %s\n' "$*" >&2; }
die()  { printf '\033[31merror:\033[0m %s\n' "$*" >&2; exit 1; }

# Writes stdin to $1 unless it exists (or --force). Returns 1 when skipped.
write_if_absent() {
    local path="$1"
    if [[ -e "$path" && $FORCE -eq 0 ]]; then
        cat > /dev/null
        return 1
    fi
    mkdir -p "$(dirname "$path")"
    cat > "$path"
    return 0
}

# ---------------------------------------------------------------- prerequisites
for tool in cmake ninja; do
    command -v "$tool" >/dev/null || die "$tool not found in PATH"
done
command -v g++ >/dev/null || warn "g++ not in PATH; the compile database will name a compiler this machine cannot query"
[[ -f "${SRC_DIR}/CMakePresets.json" ]] || die "no src/CMakePresets.json - run this from inside the repo"

# ------------------------------------------------------------------- 1. clangd
CLANGD_BIN="${IDE_DIR}/bin/clangd"

clangd_major() { "$1" --version 2>/dev/null | sed -n 's/.*clangd version \([0-9]*\).*/\1/p' | head -1; }

install_clangd() {
    local os arch asset major
    os="$(uname -s)"; arch="$(uname -m)"

    case "${os}/${arch}" in
        Linux/x86_64)  asset="clangd-linux-${CLANGD_VERSION}.zip" ;;
        *)
            # No pinned release for this platform - notably linux/aarch64, and
            # macOS, where this tree does not build anyway.
            local sys; sys="$(command -v clangd || true)"
            [[ -n "$sys" ]] || die "no clangd release for ${os}/${arch}; install clangd >= ${CLANGD_MIN_MAJOR} and re-run"
            major="$(clangd_major "$sys")"
            [[ -n "$major" && "$major" -ge $CLANGD_MIN_MAJOR ]] \
                || die "clangd ${major:-?} at ${sys} is too old for this tree's C++20; need >= ${CLANGD_MIN_MAJOR}"
            warn "no clangd release for ${os}/${arch}; falling back to ${sys}"
            mkdir -p "${IDE_DIR}/bin"; ln -sfn "$sys" "$CLANGD_BIN"
            return
            ;;
    esac

    local dest="${IDE_DIR}/toolchain/clangd_${CLANGD_VERSION}"
    if [[ ! -x "${dest}/bin/clangd" ]]; then
        command -v curl >/dev/null || die "curl not found; needed to fetch clangd"
        command -v unzip >/dev/null || die "unzip not found; needed to unpack clangd"
        say "downloading clangd ${CLANGD_VERSION} (${asset})"
        local tmp; tmp="$(mktemp -d)"
        trap 'rm -rf "$tmp"' RETURN
        curl -fsSL -o "${tmp}/clangd.zip" \
            "https://github.com/clangd/clangd/releases/download/${CLANGD_VERSION}/${asset}"
        if [[ -n "$CLANGD_SHA256_LINUX_X86_64" ]]; then
            echo "${CLANGD_SHA256_LINUX_X86_64}  ${tmp}/clangd.zip" | sha256sum -c - >/dev/null \
                || die "clangd download failed checksum verification"
        else
            warn "no checksum for clangd ${CLANGD_VERSION}; skipping verification (pass --clangd-sha256 to pin one)"
        fi
        mkdir -p "${IDE_DIR}/toolchain"
        rm -rf "$dest"
        unzip -q "${tmp}/clangd.zip" -d "${IDE_DIR}/toolchain"
    fi
    mkdir -p "${IDE_DIR}/bin"
    ln -sfn "../toolchain/clangd_${CLANGD_VERSION}/bin/clangd" "$CLANGD_BIN"
}

if [[ $USE_SYSTEM_CLANGD -eq 1 ]]; then
    sys="$(command -v clangd || true)"
    [[ -n "$sys" ]] || die "--system-clangd given but no clangd in PATH"
    major="$(clangd_major "$sys")"
    [[ -n "$major" && "$major" -ge $CLANGD_MIN_MAJOR ]] \
        || die "clangd $major is too old for this tree's C++20; need >= ${CLANGD_MIN_MAJOR}"
    mkdir -p "${IDE_DIR}/bin"; ln -sfn "$sys" "$CLANGD_BIN"
else
    install_clangd
fi
say "clangd: $("$CLANGD_BIN" --version | head -1)"

# ------------------------------------------------------- 2. CMakeUserPresets.json
# Per-developer by design and git-ignored, so writing here never dirties the tree.
# CMake Tools, CLion and `cmake --preset` all read it.
PRESETS_FILE="${SRC_DIR}/CMakeUserPresets.json"
if [[ -f "$PRESETS_FILE" ]] && grep -q "\"${PRESET}\"" "$PRESETS_FILE"; then
    say "preset ${PRESET} already present in src/CMakeUserPresets.json"
elif [[ -f "$PRESETS_FILE" ]]; then
    command -v python3 >/dev/null || die "src/CMakeUserPresets.json exists without a '${PRESET}' preset; install python3 so it can be merged, or add the preset by hand"
    say "adding ${PRESET} to existing src/CMakeUserPresets.json"
    PRESET="$PRESET" BASE_PRESET="$BASE_PRESET" python3 - "$PRESETS_FILE" <<'EOF'
import json, os, sys
path = sys.argv[1]
doc = json.load(open(path))
doc.setdefault("version", 3)
doc.setdefault("configurePresets", []).append({
    "name": os.environ["PRESET"],
    "displayName": "Linux Debug (IntelliSense index only)",
    "description": "Configure-only, unity off, for the IntelliSense database. Do not build from it.",
    "inherits": os.environ["BASE_PRESET"],
    "cacheVariables": {
        "NEO_UNITY_BUILD_CLIENT_SERVER": "OFF",
        "NEO_UNITY_BUILD_OTHERS": "OFF",
        "NEO_EXTRA_ASSETS": "OFF",
        "NEO_COPY_LIBRARIES": "OFF",
        "NEO_USE_CCACHE": "OFF",
    },
})
json.dump(doc, open(path, "w"), indent=2)
open(path, "a").write("\n")
EOF
else
    say "writing src/CMakeUserPresets.json"
    cat > "$PRESETS_FILE" <<EOF
{
  "version": 3,
  "configurePresets": [
    {
      "name": "${PRESET}",
      "displayName": "Linux Debug (IntelliSense index only)",
      "description": "Configure-only, unity off, for the IntelliSense database. Do not build from it.",
      "inherits": "${BASE_PRESET}",
      "cacheVariables": {
        "NEO_UNITY_BUILD_CLIENT_SERVER": "OFF",
        "NEO_UNITY_BUILD_OTHERS": "OFF",
        "NEO_EXTRA_ASSETS": "OFF",
        "NEO_COPY_LIBRARIES": "OFF",
        "NEO_USE_CCACHE": "OFF"
      }
    }
  ]
}
EOF
fi

# ----------------------------------------------------------------- 3. .clangd
if write_if_absent "${REPO_ROOT}/.clangd" <<EOF
# Read by clangd itself, so it applies in every editor and from the terminal.
CompileFlags:
  CompilationDatabase: src/build/${PRESET}
  # GCC-only flags; clang's driver rejects them and drops the whole command.
  Remove:
    - -fno-ipa-cp-clone
    - -fno-devirtualize-speculatively
    - -fabi-compat-version=*
    - -Winvalid-pch
    # Keep editor diagnostics as warnings.
    - -Werror
    - -Werror=*
  Add:
    - -Wno-unknown-warning-option
    - -Wno-unknown-attributes
EOF
then say "wrote .clangd"; else say ".clangd already present (use --force to rewrite)"; fi

# ------------------------------------------------------- 4. the compile database
# A database generated at another mount point (host vs dev container) matches
# nothing the editor opens, and CMake refuses a moved cache - start over.
if [[ -f "${BUILD_DIR}/CMakeCache.txt" ]]; then
    cached_src="$(sed -n 's/^CMAKE_HOME_DIRECTORY:INTERNAL=//p' "${BUILD_DIR}/CMakeCache.txt")"
    if [[ "$cached_src" != "$SRC_DIR" ]]; then
        warn "compile database was generated for ${cached_src%/src}, repo is now at ${REPO_ROOT}; regenerating"
        rm -rf "$BUILD_DIR" || die "could not remove stale ${BUILD_DIR}; delete it and re-run"
    fi
fi
if [[ $RECONFIGURE -eq 1 || ! -f "${BUILD_DIR}/compile_commands.json" ]]; then
    say "configuring ${PRESET} (no compilation, just the compile database)"
    cmake -S "$SRC_DIR" --preset "$PRESET" >/dev/null || die "cmake configure failed; re-run without >/dev/null to see why"
else
    say "compile database already present (--reconfigure to regenerate)"
fi

entries=$(python3 -c "import json,sys; print(len(json.load(open(sys.argv[1]))))" "${BUILD_DIR}/compile_commands.json" 2>/dev/null || echo '?')
unity=$(grep -c 'Unity/unity_' "${BUILD_DIR}/compile_commands.json" 2>/dev/null || true)
say "compile database: ${entries} entries, ${unity:-0} unity blobs"
[[ "${unity:-0}" == "0" ]] || warn "unity entries present - something is overriding this preset's NEO_UNITY_BUILD_* = OFF"

# Root symlink: the location nearly every C++ tool probes by default.
ln -sfn "src/build/${PRESET}/compile_commands.json" "${REPO_ROOT}/compile_commands.json"
say "symlinked compile_commands.json at the repo root"

# ------------------------------------------------------------------ 5. env.sh
# For shell-launched editors (nvim, helix, emacs): source it and clangd is on PATH.
cat > "${IDE_DIR}/env.sh" <<EOF
# source .ide/env.sh - puts the pinned clangd first on PATH
export PATH="${IDE_DIR}/bin:\$PATH"
EOF

# ------------------------------------------------------- 6. keep the tree clean
# .gitignore covers these, but a fork on an older .gitignore or a custom --preset
# still needs them hidden - so exclude locally whatever git does not ignore yet.
EXCLUDE="${REPO_ROOT}/.git/info/exclude"
if [[ -f "$EXCLUDE" ]] && command -v git >/dev/null; then
    for path in '.ide/' 'compile_commands.json' 'src/CMakeUserPresets.json' '.clangd' '.zed/' "src/build/${PRESET}"; do
        git -C "$REPO_ROOT" check-ignore -q "$path" && continue
        grep -qxF "/${path}" "$EXCLUDE" || echo "/${path}" >> "$EXCLUDE"
    done
fi

# ------------------------------------------------------------- 7. editor config
# For people outside a container; inside one the same settings arrive from
# .devcontainer/devcontainer.json instead. Keep the two in sync.
write_vscode() {
    local wrote=0
    write_if_absent "${REPO_ROOT}/.vscode/settings.json" <<EOF && wrote=1
{
    // Presets live in src/CMakePresets.json; configuring from the repo root fails.
    "cmake.sourceDirectory": "\${workspaceFolder}/src",
    "cmake.useCMakePresets": "always",
    "cmake.configureOnOpen": false,

    // In the workspace, so container rebuilds and outside shells share one binary.
    "clangd.path": "\${workspaceFolder}/.ide/bin/clangd",
    "clangd.checkUpdates": false,
    "clangd.onConfigChanged": "restart",
    // Database path and flag fixups live in .clangd at the repo root.
    "clangd.arguments": [
        // Command-line-only option: reads include paths and predefined macros
        // from the compiler that actually builds this tree.
        "--query-driver=${QUERY_DRIVER}",
        "--header-insertion=never",
        "--background-index",
        "--completion-style=detailed",
        "--pch-storage=memory"
    ],

    // clangd provides IntelliSense; stop cpptools double-indexing.
    "C_Cpp.intelliSenseEngine": "disabled",

    "files.associations": { "*.h": "cpp", "*.inc": "cpp" },
    "[cpp]": { "editor.defaultFormatter": "llvm-vs-code-extensions.vscode-clangd" },
    "files.watcherExclude": {
        "**/src/build/**": true,
        "**/.cache/**": true,
        "**/.ide/**": true
    },
    "search.exclude": { "**/src/build/**": true, "**/.ide/**": true }
}
EOF
    write_if_absent "${REPO_ROOT}/.vscode/extensions.json" <<'EOF' && wrote=1
{
    "recommendations": [
        "llvm-vs-code-extensions.vscode-clangd",
        "ms-vscode.cmake-tools",
        "vadimcn.vscode-lldb"
    ],
    "unwantedRecommendations": [
        "ms-vscode.cpptools",
        "ms-vscode.cpptools-extension-pack"
    ]
}
EOF
    # tasks.json is tracked, so it is deliberately not generated here: rewriting
    # it under --force would dirty the tree.
    if [[ $wrote -eq 1 ]]; then say "wrote VS Code config"; else say "VS Code config already present (--force to rewrite)"; fi
}

# Zed needs this only for the clangd flags: C/C++ is native, and it prefers a
# clangd already on PATH, which the dev container arranges via remoteEnv. Its
# devcontainer.json customizations namespace takes extensions only, no settings.
write_zed() {
    if write_if_absent "${REPO_ROOT}/.zed/settings.json" <<EOF
{
  // Zed wants an absolute path here, so this file is machine-specific - and
  // git-ignored, like everything else this script generates.
  "lsp": {
    "clangd": {
      "binary": {
        "path": "${IDE_DIR}/bin/clangd",
        "arguments": [
          "--query-driver=${QUERY_DRIVER}",
          "--header-insertion=never",
          "--background-index",
          "--completion-style=detailed",
          "--pch-storage=memory"
        ]
      }
    }
  }
}
EOF
    then say "wrote Zed config"; else say "Zed config already present (--force to rewrite)"; fi
}

is_vscode() { [[ "${TERM_PROGRAM:-}" == "vscode" || -n "${VSCODE_IPC_HOOK_CLI:-}${VSCODE_PID:-}" ]]; }
is_zed()    { [[ "${TERM_PROGRAM:-}" == "zed" || -n "${ZED_TERM:-}" ]]; }

case "$EDITOR_TARGET" in
    vscode) write_vscode ;;
    zed)    write_zed ;;
    # Detected from the environment rather than from .vscode/ or .zed/ existing:
    # the tracked tasks.json means .vscode/ is always there.
    auto)   is_vscode && write_vscode || true
            is_zed    && write_zed    || true ;;
    none)   ;;
    *) die "unknown --editor value: ${EDITOR_TARGET}" ;;
esac

# ------------------------------------------------------------------- 8. summary
# Plain SGR codes only (bold/dim/16-color); render everywhere ANSI does.
bold=$'\033[1m'; dim=$'\033[2m'; cyan=$'\033[36m'; green=$'\033[32m'; reset=$'\033[0m'
printf '\n%sDone.%s\n\n' "$bold$green" "$reset"
printf '  %s%-9s%s  %s\n' "$bold$cyan" 'clangd'   "$reset" ".ide/bin/clangd  ${dim}(source .ide/env.sh to put it on PATH)${reset}"
printf '  %s%-9s%s  %s\n' "$bold$cyan" 'database' "$reset" "compile_commands.json -> src/build/${PRESET}/"
printf '  %s%-9s%s  %s\n' "$bold$cyan" 'config'   "$reset" '.clangd'
printf '\n'
printf '  Other editors: point clangd at .ide/bin/clangd with\n'
printf '    %s--query-driver=%s%s\n' "$bold" "$QUERY_DRIVER" "$reset"
printf '  %swithout it clangd guesses the standard library paths%s\n' "$dim" "$reset"
printf '\n'
printf '  See %s"Development using Linux"%s in README.md.\n' "$bold" "$reset"
printf '  Re-run with %s--reconfigure%s after adding or renaming sources.\n' "$bold" "$reset"

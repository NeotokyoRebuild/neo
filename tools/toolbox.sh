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

# Single entry point for the NT;RE devtools. Shows the tools as a numbered
# menu, prompts for the one path argument, and runs the selection.
#
#     ./toolbox.sh
#
# To plug a new tool into the menu, add one line to the table below:
# "script name|path prompt|default path|one-line description". The script
# must live next to this file and take a single path argument.

set -u

here="$(cd "$(dirname "$0")" && pwd)"

[ -t 0 ] || { echo "error: toolbox.sh is an interactive menu, run it from a terminal" >&2; exit 2; }

[ -f "$here/nt-logo.sh" ] && . "$here/nt-logo.sh" && nt_logo

tools=(
    "check-depot-casing.sh|Staging root to check|.|Check a depot staging tree for wrongly cased binaries"
    "fix-linux-casing.sh|Install root to repair|.|Repair a Linux install's binary file name casing"
)

while :; do
    echo
    echo "Tools:"
    for i in "${!tools[@]}"; do
        IFS='|' read -r script _ _ desc <<< "${tools[$i]}"
        printf '  %d) %-24s %s\n' "$((i + 1))" "$script" "$desc"
    done
    echo "  Q) quit (default)"
    read -rp "Select [# or Q]: " choice || exit 0

    case "$choice" in
        ''|q|Q)
            exit 0
            ;;
        *[!0-9]*)
            echo "Pick a tool number or Q." >&2
            ;;
        *)
            idx=$(( choice - 1 ))
            if [ "$idx" -lt 0 ] || [ "$idx" -ge "${#tools[@]}" ]; then
                echo "No tool number $choice." >&2
                continue
            fi
            IFS='|' read -r script prompt default _ <<< "${tools[$idx]}"
            read -rp "$prompt [$default]: " arg || exit 0
            echo
            "$here/$script" "${arg:-$default}"
            echo "($script exited $?)"
            ;;
    esac
done

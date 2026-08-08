#!/bin/sh
# Prints the shadowed NT;RE logo, in color when the terminal supports it.
# Prints nothing when stdout is not a terminal, so piped and logged output
# stays clean. POSIX sh so both bash and sh scripts can source it:
#
#     [ -f "$(dirname "$0")/nt-logo.sh" ] && . "$(dirname "$0")/nt-logo.sh" && nt_logo
#
# Running this file directly also prints the logo.

nt_logo() {
    [ -t 1 ] || return 0
    # Print once per process tree: child scripts inherit the mark, so a
    # wrapper like toolbox.sh shows the logo and the tools it runs skip it.
    [ "${NT_LOGO_SHOWN:-}" = 1 ] && return 0
    NT_LOGO_SHOWN=1
    export NT_LOGO_SHOWN
    r=''; s=''; b=''; x=''
    if [ -z "${NO_COLOR:-}" ] && [ "${TERM:-dumb}" != dumb ]; then
        r='\033[31m'   # red semicolon dot
        s='\033[2m'    # dim drop shadow
        b='\033[1m'    # bold title
        x='\033[0m'
    fi
    printf '%b\n' \
        "                    ████" \
        "       ████████    ██████████     ${r}▓▓▓▓${x}" \
        "     ████████████  ███████████   ${r}▓▓▓▓▓▓${x}" \
        "    █████${s}░░░${x}█████${s}░${x} █████${s}░░░░░░░${x}  ${r}▓▓▓▓▓▓${x}${s}░${x}" \
        "    █████${s}░${x}  █████${s}░${x} █████${s}░${x}         ${r}▓▓▓▓${x}${s}░░${x}" \
        "    █████${s}░${x}  █████${s}░${x} █████${s}░${x}          ${s}░░░░${x}" \
        "    █████${s}░${x}  █████${s}░${x} ██████████      █████" \
        "    █████${s}░${x}  █████${s}░${x} ██████████${s}░${x}     █████${s}░${x}" \
        "     ${s}░░░░░${x}  █████${s}░${x}  ${s}░░░░░░░░░░${x}    ████${s}░░░${x}" \
        "            █████${s}░${x}               ███${s}░░░${x}" \
        "             ${s}░░░░░${x}                ${s}░░░${x}" \
        "" \
        "  ${b}    N E O T O K Y O ; R E B U I L D${x}" \
        ""
}

# Print right away when executed instead of sourced.
case "$0" in
    *nt-logo.sh) nt_logo ;;
esac

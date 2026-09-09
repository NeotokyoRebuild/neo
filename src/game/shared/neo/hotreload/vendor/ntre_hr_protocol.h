/*
 * ntre_hr_protocol.h
 *
 * Single source of truth for the mailbox protocol constants shared by the
 * sidecar (Rust) and the in-process loader (C++).
 *
 * sidecar/build.rs parses this file: every line of the form
 *     #define NTRE_HR_<NAME> <integer>
 *     #define NTRE_HR_<NAME> "<string>"
 * becomes a Rust constant. Keep values on one line, keep the forms above, and
 * do not add computed macros here. Semantics live in protocol/README.md.
 *
 * Bump NTRE_HR_PROTOCOL_VERSION on any incompatible change to file names or
 * JSON fields. Both halves reject files whose "protocol" differs from theirs.
 */
#ifndef NTRE_HR_PROTOCOL_H
#define NTRE_HR_PROTOCOL_H

/* Protocol version carried in every mailbox file. */
#define NTRE_HR_PROTOCOL_VERSION 1

/* Mailbox directory name, created inside the build dir: <build dir>/.hotreload */
#define NTRE_HR_MAILBOX_DIR ".hotreload"

/* Sidecar presence file (sidecar to game): <mailbox>/sidecar.json */
#define NTRE_HR_SIDECAR_FILE "sidecar.json"
/* Objects the sidecar has published, per module; relinked for a game process that has applied nothing yet. */
#define NTRE_HR_UNITS_FILE "sidecar.units.json"

/* Game state file (game to sidecar): <mailbox>/state.<module>.json */
#define NTRE_HR_STATE_PREFIX "state."
#define NTRE_HR_STATE_SUFFIX ".json"

/* Shim binary and manifest (sidecar to game): <mailbox>/<module>.<seq>.so and .json */
#define NTRE_HR_SHIM_SO_SUFFIX ".so"
#define NTRE_HR_SHIM_MANIFEST_SUFFIX ".json"

/* Apply result (game to sidecar): <mailbox>/<module>.<seq>.result.json */
#define NTRE_HR_RESULT_SUFFIX ".result.json"

/* Files are written to <final name><tmp suffix> and renamed into place. Readers ignore this suffix. */
#define NTRE_HR_TMP_SUFFIX ".tmp"

/* A presence or state file whose mtime is older than this is treated as gone. */
#define NTRE_HR_HEARTBEAT_STALE_SECONDS 5

/* Result status strings. */
#define NTRE_HR_STATUS_APPLIED "applied"
#define NTRE_HR_STATUS_REJECTED "rejected"
#define NTRE_HR_STATUS_FAILED "failed"

/* Defaults for the reserved shim region (game side, overridable in ntre_hr_config): 256 slots of 1 MB,
   256 MB of address space, one slot per typical shim, so 256 reloads per game session before the
   sidecar has to link without a base. */
#define NTRE_HR_DEFAULT_SLOT_SIZE 1048576
#define NTRE_HR_DEFAULT_SLOT_COUNT 256

#endif /* NTRE_HR_PROTOCOL_H */

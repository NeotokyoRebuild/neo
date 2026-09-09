/*
 * ntre_hr.h
 *
 * Public API of the in-process hot reload loader core.
 *
 * C linkage, no Source SDK types: the same header drives the loader from the
 * game glue (neo_hot_reload.cpp) and from the fixture host. One ntre_hr context
 * per module (server.so and client.so each own one). Functions are hidden by
 * default so that two modules in one process, each carrying a copy of the
 * core, never clash; define NTRE_HR_API yourself to change that.
 *
 * Threading: every function must be called from the same thread, normally the
 * one that runs the game frame. ntre_hr_poll is meant to be called once per
 * frame; it rate limits itself (poll_interval_ms) and returns quickly when
 * there is nothing to do.
 */
#ifndef NTRE_HR_H
#define NTRE_HR_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifndef NTRE_HR_API
#if defined(__GNUC__)
#define NTRE_HR_API __attribute__((visibility("hidden")))
#else
#define NTRE_HR_API
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Bumped when this header changes incompatibly. Independent of the mailbox protocol version. */
#define NTRE_HR_API_VERSION 2

/* Opaque per-module context. */
typedef struct ntre_hr ntre_hr;

typedef enum ntre_hr_log_level {
    NTRE_HR_LOG_DEBUG = 0, /* per-symbol detail, only emitted when verbose */
    NTRE_HR_LOG_INFO = 1,
    NTRE_HR_LOG_WARN = 2,
    NTRE_HR_LOG_ERROR = 3
} ntre_hr_log_level;

/* Describes the shim being applied. Passed to the registry callbacks. */
typedef struct ntre_hr_shim_info {
    uint32_t struct_size;
    const char* module;    /* module name ("server") */
    uint32_t seq;          /* shim sequence number */
    const char* shim_path; /* absolute path of the shim .so */
    void* handle;          /* dlopen handle; NULL in pre_apply (the shim is not mapped yet) */
} ntre_hr_shim_info;

typedef struct ntre_hr_apply_counts {
    uint32_t functions_hooked;  /* original entries now jumping to shim code */
    uint32_t statics_shared;    /* shim references patched to the original's statics */
    uint32_t statics_copied;    /* fallback byte copies (placement out of PC32 range) */
    uint32_t functions_skipped; /* matched functions that could not be hooked safely */
    uint32_t globals_rebound;   /* GOT and pointer slots re-pointed at the original's globals */
} ntre_hr_apply_counts;

typedef void (*ntre_hr_log_fn)(void* user, ntre_hr_log_level level, const char* message);

/* Called before the shim is mapped (its .init_array has not run): snapshot engine registries. */
typedef void (*ntre_hr_pre_apply_fn)(void* user, const ntre_hr_shim_info* shim);

/* Called after hooks and relocations are in place: rebuild registries from the snapshot. */
typedef void (*ntre_hr_post_apply_fn)(void* user, const ntre_hr_shim_info* shim,
                                      const ntre_hr_apply_counts* counts);

typedef struct ntre_hr_config {
    uint32_t struct_size; /* set by ntre_hr_config_init */

    /* Identity of the module this context serves. Required. Must equal the .so basename without
     * extension and the CMake target name (the sidecar maps objects to modules by that name). */
    const char* module_name;

    /* Any address inside the module, for example the address of a function defined in it.
     * dladdr turns it into the module path and load base. Required. */
    const void* module_anchor;

    /* CMake build dir the sidecar drives. The mailbox is <build_dir>/.hotreload. Absolute, or
     * relative to the directory containing the module. Required. */
    const char* build_dir;

    bool auto_apply; /* apply shims as they appear (true) or wait for ntre_hr_apply_pending (false) */
    bool verbose;    /* emit NTRE_HR_LOG_DEBUG messages */

    uint32_t poll_interval_ms;      /* 0 = 250 */
    uint32_t heartbeat_interval_ms; /* 0 = 1000; how often the state file is rewritten */

    uint64_t slot_size;  /* 0 = NTRE_HR_DEFAULT_SLOT_SIZE */
    uint32_t slot_count; /* 0 = NTRE_HR_DEFAULT_SLOT_COUNT */

    ntre_hr_log_fn log; /* NULL = stderr */
    void* log_user;

    ntre_hr_pre_apply_fn pre_apply;   /* optional */
    ntre_hr_post_apply_fn post_apply; /* optional */
    void* apply_user;
} ntre_hr_config;

typedef struct ntre_hr_status {
    uint32_t struct_size;

    bool sidecar_attached;        /* sidecar.json present and fresh */
    int64_t sidecar_seen_ms_ago;  /* -1 when never seen */
    bool sidecar_auto_apply;      /* the sidecar's mode, when attached */

    bool auto_apply;              /* this context's mode */
    uint32_t pending;             /* shims queued, waiting for ntre_hr_apply_pending */
    uint32_t applied;             /* shims applied since init */
    uint32_t applied_seq;         /* highest sequence number applied */

    bool region_reserved;
    bool region_in_range;         /* whole region within PC32 reach of the module */
    uintptr_t region_base;
    uint64_t slot_size;
    uint32_t slot_count;
    uint32_t next_slot;
} ntre_hr_status;

/* Zero the struct and fill struct_size and defaults. Call before setting fields. */
NTRE_HR_API void ntre_hr_config_init(ntre_hr_config* cfg);

/* Create a context: resolve the module, read its build-id, reserve the shim region, create the
 * mailbox, write the state file and queue any shims already waiting. Returns NULL on failure
 * (the reason is logged). */
NTRE_HR_API ntre_hr* ntre_hr_init(const ntre_hr_config* cfg);

/* Call once per frame. Refreshes the heartbeat, tracks the sidecar, picks up new shims and, in
 * auto mode, applies them. Returns true when at least one shim was applied during this call. */
NTRE_HR_API bool ntre_hr_poll(ntre_hr* hr);

/* Apply everything queued regardless of mode (the in-game trigger command). Returns the number
 * of shims applied. */
NTRE_HR_API uint32_t ntre_hr_apply_pending(ntre_hr* hr);

NTRE_HR_API void ntre_hr_set_auto_apply(ntre_hr* hr, bool on);
NTRE_HR_API void ntre_hr_set_verbose(ntre_hr* hr, bool on);

/* Fill a status struct. out->struct_size must be set by the caller. Returns false on bad input. */
NTRE_HR_API bool ntre_hr_get_status(const ntre_hr* hr, ntre_hr_status* out);

/* Absolute mailbox directory and the running module's build-id (lowercase hex). Valid until shutdown. */
NTRE_HR_API const char* ntre_hr_mailbox_dir(const ntre_hr* hr);
NTRE_HR_API const char* ntre_hr_build_id(const ntre_hr* hr);

/* Remove the state file, restore hooked entries, release the region and free the context.
 * Shims stay mapped (their code may still be referenced) but their static destructors run here,
 * so nothing of theirs is left for process exit; call it after the module has unregistered what
 * shims registered (ConVars) and before the engine unloads the module. */
NTRE_HR_API void ntre_hr_shutdown(ntre_hr* hr);

/* "ntre_hr <core version> protocol <n>" */
NTRE_HR_API const char* ntre_hr_version(void);
NTRE_HR_API uint32_t ntre_hr_protocol_version(void);

#ifdef __cplusplus
}
#endif

#endif /* NTRE_HR_H */

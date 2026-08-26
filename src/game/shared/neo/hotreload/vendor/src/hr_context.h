// The per-module context behind the opaque ntre_hr handle.
#ifndef NTRE_HR_CONTEXT_H
#define NTRE_HR_CONTEXT_H

#include <cstdint>
#include <deque>
#include <map>
#include <string>
#include <vector>

#include "hr_elf.h"
#include "hr_hook.h"
#include "hr_log.h"
#include "hr_mailbox.h"
#include "hr_region.h"
#include "ntre_hr.h"

struct ntre_hr {
    // Configuration (copied out of ntre_hr_config).
    std::string module_name;
    const void* module_anchor = nullptr;
    hr::Logger log;
    ntre_hr_pre_apply_fn pre_apply = nullptr;
    ntre_hr_post_apply_fn post_apply = nullptr;
    void* apply_user = nullptr;
    bool auto_apply = true;
    uint32_t poll_interval_ms = 250;
    uint32_t heartbeat_interval_ms = 1000;

    // Module identity.
    std::string module_path; // absolute
    std::string module_dir;
    uintptr_t module_base = 0; // load bias
    uintptr_t module_lo = 0;   // mapped extent
    uintptr_t module_hi = 0;
    std::string build_id;          // build-id of the loaded image
    hr::elf::File module_file;     // opened on first apply
    std::vector<hr::elf::Symbol> module_symbols;
    std::map<std::string, std::vector<size_t>> module_funcs;   // name -> indexes into module_symbols (STT_FUNC)
    std::map<std::string, std::vector<size_t>> module_objects; // name -> indexes into module_symbols (STT_OBJECT, defined, sized)
    bool module_symbols_loaded = false;

    hr::Region region;
    hr::Mailbox mailbox;

    // Runtime state.
    std::map<uintptr_t, hr::hook::Patch> hooks; // original entry -> patch
    std::deque<hr::Manifest> pending;
    std::vector<void*> shim_handles;
    std::vector<uintptr_t> shim_dso_handles; // __dso_handle of each shim, 0 when unknown; finalized at shutdown
    std::map<uint32_t, int64_t> seen_manifests; // seq -> mtime of the manifest already handled; seqs restart per game process
    uint32_t applied_seq = 0;
    uint32_t applied_count = 0;
    uint64_t heartbeat = 0;
    int64_t last_poll_ms = -1;
    int64_t last_heartbeat_ms = -1;
    hr::SidecarPresence sidecar;
    bool sidecar_attached = false;
    bool sidecar_ever_seen = false;
    bool foreign_noted = false; // told the developer once about shims linked for another process
    std::string sidecar_session; // id of the sidecar session last seen attached; manifests of other sessions are ignored
    bool other_session_noted = false;
};

#endif

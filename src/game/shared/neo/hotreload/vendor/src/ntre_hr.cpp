// Public API implementation: init, poll, apply, status, shutdown.
#include "ntre_hr.h"

#include <unistd.h>

#include <cstring>

#include "hr_apply.h"
#include "hr_context.h"
#include "hr_paths.h"
#include "ntre_hr_protocol.h"

#define NTRE_HR_CORE_VERSION "0.2.0"
#define NTRE_HR_STRINGIFY_(x) #x
#define NTRE_HR_STRINGIFY(x) NTRE_HR_STRINGIFY_(x)

namespace {

using hr::json::Value;

Value build_state_json(const ntre_hr& hr) {
    Value st = Value::object();
    st.set("protocol", Value::integer(NTRE_HR_PROTOCOL_VERSION));
    st.set("build_id", Value::string(hr.build_id));
    st.set("pid", Value::integer(static_cast<int64_t>(getpid())));
    st.set("module", Value::string(hr.module_name));
    st.set("module_path", Value::string(hr.mailbox.rel(hr.module_path)));
    if (hr.region.reserved) {
        Value r = Value::object();
        r.set("base", Value::string(hr::hex_address(hr.region.base)));
        r.set("slot_size", Value::integer(static_cast<int64_t>(hr.region.slot_size)));
        r.set("slot_count", Value::integer(hr.region.slot_count));
        r.set("next_slot", Value::integer(hr.region.next_slot));
        r.set("in_range", Value::boolean(hr.region.in_range));
        st.set("region", r);
    } else {
        st.set("region", Value::null());
    }
    st.set("module_base", Value::string(hr::hex_address(hr.module_base)));
    st.set("auto_apply", Value::boolean(hr.auto_apply));
    st.set("applied_seq", Value::integer(hr.applied_seq));
    st.set("heartbeat", Value::integer(static_cast<int64_t>(hr.heartbeat)));
    return st;
}

Value build_result_json(const ntre_hr& hr, const hr::Manifest& m, const hr::ApplyOutcome& o) {
    Value r = Value::object();
    r.set("protocol", Value::integer(NTRE_HR_PROTOCOL_VERSION));
    r.set("build_id", Value::string(hr.build_id));
    r.set("module", Value::string(hr.module_name));
    r.set("seq", Value::integer(m.seq));
    r.set("status", Value::string(o.status));
    r.set("applied_unix_ms", Value::integer(hr::paths::now_unix_ms()));
    Value c = Value::object();
    c.set("functions_hooked", Value::integer(o.counts.functions_hooked));
    c.set("statics_shared", Value::integer(o.counts.statics_shared));
    c.set("statics_copied", Value::integer(o.counts.statics_copied));
    c.set("globals_rebound", Value::integer(o.counts.globals_rebound));
    c.set("functions_skipped", Value::integer(o.counts.functions_skipped));
    r.set("counts", c);
    Value w = Value::array();
    for (const std::string& s : o.warnings) w.push(Value::string(s));
    r.set("warnings", w);
    r.set("error", o.error.empty() ? Value::null() : Value::string(o.error));
    return r;
}

void write_state(ntre_hr& hr) {
    hr.heartbeat++;
    std::string err;
    if (!hr.mailbox.write_json(hr.mailbox.state_path(hr.module_name), build_state_json(hr), err))
        hr.log.warn("state file: %s", err.c_str());
}

void refresh_sidecar(ntre_hr& hr) {
    hr.sidecar = hr.mailbox.sidecar_presence(NTRE_HR_HEARTBEAT_STALE_SECONDS * 1000);
    bool attached = hr.sidecar.present && hr.sidecar.fresh && hr.sidecar.protocol == NTRE_HR_PROTOCOL_VERSION;
    if (hr.sidecar.present && hr.sidecar.fresh && hr.sidecar.protocol != NTRE_HR_PROTOCOL_VERSION && !hr.sidecar_attached) {
        hr.log.warn("sidecar speaks protocol %lld, this module speaks %d: update the sidecar or the vendored loader",
                    static_cast<long long>(hr.sidecar.protocol), NTRE_HR_PROTOCOL_VERSION);
    }
    if (attached && !hr.sidecar.session.empty() && hr.sidecar.session != hr.sidecar_session) {
        hr.sidecar_session = hr.sidecar.session;
        hr.other_session_noted = false;
    }
    if (attached && !hr.sidecar_attached) {
        hr.log.info("sidecar attached (pid %lld, %s)", static_cast<long long>(hr.sidecar.pid), hr.sidecar.auto_apply ? "mode save" : "mode trigger");
        hr.sidecar_ever_seen = true;
    } else if (!attached && hr.sidecar_attached) {
        hr.log.info("sidecar detached");
    }
    hr.sidecar_attached = attached;
}

// A manifest this process will never apply gets a "rejected" result right away, so the sidecar
// prints the reason instead of waiting for a result that never comes (protocol/README.md).
void reject(ntre_hr& hr, const hr::Manifest& m, const std::string& why, ntre_hr_log_level level) {
    hr::ApplyOutcome o;
    o.status = NTRE_HR_STATUS_REJECTED;
    o.error = why;
    std::string err;
    if (!hr.mailbox.write_json(hr.mailbox.result_path(hr.module_name, m.seq), build_result_json(hr, m, o), err))
        hr.log.warn("result file: %s", err.c_str());
    hr.log.log(level, "rejected %s: %s", hr::manifest_file_name(hr.module_name, m.seq).c_str(), why.c_str());
}

// Read the manifests not handled yet and queue the ones that target this build. A manifest is
// known by its seq and mtime: sequence numbers restart per game process and a sidecar session
// may replace a file of the same seq, so a high-water mark would skip the new one.
void scan_manifests(ntre_hr& hr) {
    uint32_t foreign = 0, other_session = 0;
    std::map<uint32_t, int64_t> seen;
    for (uint32_t seq : hr.mailbox.manifest_seqs(hr.module_name)) {
        const int64_t stamp = hr::paths::mtime_ms(hr.mailbox.manifest_path(hr.module_name, seq));
        auto known = hr.seen_manifests.find(seq);
        seen[seq] = stamp;
        if (known != hr.seen_manifests.end() && known->second == stamp) continue;
        hr::Manifest m;
        std::string err;
        if (!hr.mailbox.read_manifest(hr.module_name, seq, m, err)) {
            m = hr::Manifest();
            m.seq = seq;
            reject(hr, m, "manifest unreadable: " + err, NTRE_HR_LOG_WARN);
            continue;
        }
        if (m.protocol != NTRE_HR_PROTOCOL_VERSION) {
            reject(hr, m,
                   "protocol " + std::to_string(m.protocol) + ", this module speaks " + std::to_string(NTRE_HR_PROTOCOL_VERSION) +
                       ": update the sidecar or the vendored loader",
                   NTRE_HR_LOG_WARN);
            continue;
        }
        if (!m.session.empty() && m.session != hr.sidecar_session) {
            // Published by a sidecar session this process has not seen attached (an earlier one,
            // or one whose presence file is not refreshed yet): read again next poll.
            other_session++;
            seen.erase(seq);
            continue;
        }
        if (m.has_module_base && m.module_base != hr.module_base) {
            // Linked for another game process: shims bind the module by address. The sidecar
            // relinks the edits for this process when it attaches and removes these.
            foreign++;
            hr.log.debug("ignored %s: linked for the module at 0x%lx, this process has it at 0x%lx", m.shim.c_str(),
                         static_cast<unsigned long>(m.module_base), static_cast<unsigned long>(hr.module_base));
            continue;
        }
        if (m.build_id != hr.build_id) {
            // Leftovers from an earlier build are expected after a rebuild; the sidecar prunes them.
            reject(hr, m, "targets build " + m.build_id.substr(0, 12) + ", this process runs " + hr.build_id.substr(0, 12) + ": rebuild or restart the game",
                   NTRE_HR_LOG_DEBUG);
            continue;
        }
        hr.pending.push_back(m);
        hr.log.debug("queued %s", m.shim.c_str());
    }
    hr.seen_manifests.swap(seen);
    if (other_session && !hr.other_session_noted && hr.sidecar_attached) {
        hr.other_session_noted = true;
        hr.log.info("%u shim(s) in the mailbox came from another sidecar session and are ignored", other_session);
    }
    if (foreign && !hr.foreign_noted) {
        hr.foreign_noted = true;
        hr.log.info("%u shim(s) in the mailbox were linked for another game process and are ignored; the sidecar relinks your edits when it attaches", foreign);
    }
}

uint32_t apply_queue(ntre_hr& hr) {
    uint32_t applied = 0;
    while (!hr.pending.empty()) {
        hr::Manifest m = hr.pending.front();
        hr.pending.pop_front();
        hr::ApplyOutcome o;
        hr::apply_shim(hr, m, o);
        std::string err;
        if (!hr.mailbox.write_json(hr.mailbox.result_path(hr.module_name, m.seq), build_result_json(hr, m, o), err))
            hr.log.warn("result file: %s", err.c_str());
        if (strcmp(o.status, NTRE_HR_STATUS_APPLIED) == 0) {
            applied++;
            hr.applied_count++;
            if (m.seq > hr.applied_seq) hr.applied_seq = m.seq;
            hr.log.info("applied %s: %u hooked, %u statics shared, %u copied, %u globals rebound, %u skipped, %zu warning(s)", m.shim.c_str(),
                        o.counts.functions_hooked, o.counts.statics_shared, o.counts.statics_copied, o.counts.globals_rebound,
                        o.counts.functions_skipped, o.warnings.size());
        } else {
            hr.log.error("%s %s: %s", o.status, m.shim.c_str(), o.error.c_str());
        }
        for (const std::string& w : o.warnings) hr.log.warn("%s: %s", m.shim.c_str(), w.c_str());
    }
    if (applied) write_state(hr);
    return applied;
}

// First hint for the region: where shims already in the mailbox expect to live (replay on attach).
uintptr_t preferred_region_base(const ntre_hr& hr, uint64_t slot_size) {
    for (uint32_t seq : hr.mailbox.manifest_seqs(hr.module_name)) {
        hr::Manifest m;
        std::string err;
        if (!hr.mailbox.read_manifest(hr.module_name, seq, m, err)) continue;
        if (m.build_id != hr.build_id || !m.has_link_base || !m.has_slot) continue;
        if (m.link_base < m.slot * slot_size) continue;
        return m.link_base - m.slot * slot_size;
    }
    return 0;
}

} // namespace

extern "C" {

void ntre_hr_config_init(ntre_hr_config* cfg) {
    if (!cfg) return;
    memset(cfg, 0, sizeof *cfg);
    cfg->struct_size = sizeof *cfg;
    cfg->auto_apply = true;
    cfg->poll_interval_ms = 250;
    cfg->heartbeat_interval_ms = 1000;
}

ntre_hr* ntre_hr_init(const ntre_hr_config* cfg) {
    hr::Logger boot;
    if (!cfg || cfg->struct_size < sizeof(ntre_hr_config)) { boot.error("init: bad config (call ntre_hr_config_init first)"); return nullptr; }
    boot.fn = cfg->log;
    boot.user = cfg->log_user;
    boot.verbose = cfg->verbose;
    if (!cfg->module_name || !*cfg->module_name || !cfg->module_anchor || !cfg->build_dir || !*cfg->build_dir) {
        boot.error("init: module_name, module_anchor and build_dir are required");
        return nullptr;
    }

    ntre_hr* hr = new ntre_hr();
    hr->log = boot;
    hr->module_name = cfg->module_name;
    hr->module_anchor = cfg->module_anchor;
    hr->pre_apply = cfg->pre_apply;
    hr->post_apply = cfg->post_apply;
    hr->apply_user = cfg->apply_user;
    hr->auto_apply = cfg->auto_apply;
    if (cfg->poll_interval_ms) hr->poll_interval_ms = cfg->poll_interval_ms;
    if (cfg->heartbeat_interval_ms) hr->heartbeat_interval_ms = cfg->heartbeat_interval_ms;

    std::string err;
    if (!hr::paths::module_of(cfg->module_anchor, hr->module_path, hr->module_base, err)) {
        hr->log.error("init: %s", err.c_str());
        delete hr;
        return nullptr;
    }
    hr->module_dir = hr::paths::dirname(hr->module_path);
    hr::elf::LoadedImage img;
    if (!hr::elf::find_loaded_image(reinterpret_cast<uintptr_t>(cfg->module_anchor), img)) {
        hr->log.error("init: could not find the loaded image of %s", hr->module_path.c_str());
        delete hr;
        return nullptr;
    }
    hr->module_base = img.bias;
    hr->module_lo = img.lo;
    hr->module_hi = img.hi;
    hr->build_id = img.build_id;
    if (hr->build_id.empty()) {
        hr->log.warn("init: %s has no GNU build-id (link with -Wl,--build-id); skew guard disabled", hr->module_path.c_str());
    }
    std::string build_dir = hr::paths::is_absolute(cfg->build_dir) ? cfg->build_dir : hr::paths::join(hr->module_dir, cfg->build_dir);
    build_dir = hr::paths::realpath_or(build_dir);
    if (!hr->mailbox.init(build_dir, err)) {
        hr->log.error("init: mailbox: %s", err.c_str());
        delete hr;
        return nullptr;
    }

    uint64_t slot_size = cfg->slot_size ? cfg->slot_size : NTRE_HR_DEFAULT_SLOT_SIZE;
    uint32_t slot_count = cfg->slot_count ? cfg->slot_count : NTRE_HR_DEFAULT_SLOT_COUNT;
    uintptr_t preferred = preferred_region_base(*hr, slot_size);
    hr->region.reserve(hr->module_lo, hr->module_hi, slot_size, slot_count, preferred, hr->log);

    hr->log.info("%s: module %s build %s, mailbox %s, mode %s", hr->module_name.c_str(), hr->module_path.c_str(),
                 hr->build_id.empty() ? "unknown" : hr->build_id.substr(0, 12).c_str(), hr->mailbox.dir().c_str(),
                 hr->auto_apply ? "auto" : "trigger");

    int64_t now = hr::paths::now_mono_ms();
    hr->last_heartbeat_ms = now;
    write_state(*hr);
    refresh_sidecar(*hr);
    if (!hr->sidecar_attached) hr->log.info("no sidecar attached: run `make watch` in your build shell to enable hot reload");
    scan_manifests(*hr);
    if (!hr->pending.empty()) hr->log.info("%zu shim(s) waiting in the mailbox", hr->pending.size());
    return hr;
}

bool ntre_hr_poll(ntre_hr* hr) {
    if (!hr) return false;
    int64_t now = hr::paths::now_mono_ms();
    if (hr->last_poll_ms >= 0 && now - hr->last_poll_ms < hr->poll_interval_ms) return false;
    hr->last_poll_ms = now;
    if (now - hr->last_heartbeat_ms >= hr->heartbeat_interval_ms) {
        hr->last_heartbeat_ms = now;
        write_state(*hr);
        refresh_sidecar(*hr);
    }
    scan_manifests(*hr);
    if (!hr->auto_apply || hr->pending.empty()) return false;
    return apply_queue(*hr) > 0;
}

uint32_t ntre_hr_apply_pending(ntre_hr* hr) {
    if (!hr) return 0;
    scan_manifests(*hr);
    if (hr->pending.empty()) return 0;
    return apply_queue(*hr);
}

void ntre_hr_set_auto_apply(ntre_hr* hr, bool on) {
    if (!hr || hr->auto_apply == on) return;
    hr->auto_apply = on;
    write_state(*hr);
}

void ntre_hr_set_verbose(ntre_hr* hr, bool on) {
    if (hr) hr->log.verbose = on;
}

bool ntre_hr_get_status(const ntre_hr* hr, ntre_hr_status* out) {
    if (!hr || !out || out->struct_size < sizeof(ntre_hr_status)) return false;
    uint32_t size = out->struct_size;
    memset(out, 0, sizeof *out);
    out->struct_size = size;
    out->sidecar_attached = hr->sidecar_attached;
    out->sidecar_seen_ms_ago = hr->sidecar.present ? hr->sidecar.age_ms : -1;
    out->sidecar_auto_apply = hr->sidecar.auto_apply;
    out->auto_apply = hr->auto_apply;
    out->pending = static_cast<uint32_t>(hr->pending.size());
    out->applied = hr->applied_count;
    out->applied_seq = hr->applied_seq;
    out->region_reserved = hr->region.reserved;
    out->region_in_range = hr->region.in_range;
    out->region_base = hr->region.base;
    out->slot_size = hr->region.slot_size;
    out->slot_count = hr->region.slot_count;
    out->next_slot = hr->region.next_slot;
    return true;
}

const char* ntre_hr_mailbox_dir(const ntre_hr* hr) { return hr ? hr->mailbox.dir().c_str() : ""; }
const char* ntre_hr_build_id(const ntre_hr* hr) { return hr ? hr->build_id.c_str() : ""; }

extern "C" void __cxa_finalize(void*);

void ntre_hr_shutdown(ntre_hr* hr) {
    if (!hr) return;
    for (auto& kv : hr->hooks) {
        std::string err;
        if (!hr::hook::restore(kv.second, err)) hr->log.warn("shutdown: %s", err.c_str());
    }
    hr->hooks.clear();
    hr->mailbox.remove(hr->mailbox.state_path(hr->module_name));
    hr->region.unreserve();
    // Shims stay mapped on purpose: their code and statics may still be referenced. Their
    // static destructors run now, newest shim first, while the module and the engine are
    // still alive; nothing of theirs is left for process exit, and the shims hold no
    // dependency on the module, so the engine unloads it on its own schedule.
    uint32_t finalized = 0;
    for (size_t i = hr->shim_dso_handles.size(); i-- > 0;) {
        if (!hr->shim_dso_handles[i]) continue;
        __cxa_finalize(reinterpret_cast<void*>(hr->shim_dso_handles[i]));
        finalized++;
    }
    hr->shim_dso_handles.clear();
    hr->log.info("%s: hot reload shut down (%u shim(s) applied, %u finalized)", hr->module_name.c_str(), hr->applied_count, finalized);
    delete hr;
}

const char* ntre_hr_version(void) { return "ntre_hr " NTRE_HR_CORE_VERSION " protocol " NTRE_HR_STRINGIFY(NTRE_HR_PROTOCOL_VERSION); }

uint32_t ntre_hr_protocol_version(void) { return NTRE_HR_PROTOCOL_VERSION; }

} // extern "C"

#include "hr_mailbox.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "hr_paths.h"
#include "ntre_hr_protocol.h"

namespace hr {

std::string state_file_name(const std::string& module) { return std::string(NTRE_HR_STATE_PREFIX) + module + NTRE_HR_STATE_SUFFIX; }
std::string shim_file_name(const std::string& module, uint32_t seq) { return module + "." + std::to_string(seq) + NTRE_HR_SHIM_SO_SUFFIX; }
std::string manifest_file_name(const std::string& module, uint32_t seq) { return module + "." + std::to_string(seq) + NTRE_HR_SHIM_MANIFEST_SUFFIX; }
std::string result_file_name(const std::string& module, uint32_t seq) { return module + "." + std::to_string(seq) + NTRE_HR_RESULT_SUFFIX; }

bool parse_manifest_name(const std::string& name, const std::string& module, uint32_t& seq) {
    // <module>.<digits>.json, and nothing else (so ".result.json" and ".json.tmp" do not match)
    const std::string prefix = module + ".";
    const std::string suffix = NTRE_HR_SHIM_MANIFEST_SUFFIX;
    if (name.size() <= prefix.size() + suffix.size()) return false;
    if (name.compare(0, prefix.size(), prefix) != 0) return false;
    if (name.compare(name.size() - suffix.size(), suffix.size(), suffix) != 0) return false;
    std::string digits = name.substr(prefix.size(), name.size() - prefix.size() - suffix.size());
    if (digits.empty() || digits.size() > 9) return false;
    for (char c : digits) if (c < '0' || c > '9') return false;
    seq = static_cast<uint32_t>(strtoul(digits.c_str(), nullptr, 10));
    return seq > 0;
}

bool parse_hex_address(const std::string& s, uintptr_t& out) {
    if (s.size() < 3 || s[0] != '0' || (s[1] != 'x' && s[1] != 'X')) return false;
    char* end = nullptr;
    errno = 0;
    unsigned long long v = strtoull(s.c_str() + 2, &end, 16);
    if (errno != 0 || !end || *end != '\0') return false;
    out = static_cast<uintptr_t>(v);
    return true;
}

std::string hex_address(uintptr_t a) {
    char buf[32];
    snprintf(buf, sizeof buf, "0x%lx", static_cast<unsigned long>(a));
    return buf;
}

bool Mailbox::init(const std::string& build_dir, std::string& err) {
    if (!paths::is_absolute(build_dir)) { err = "build dir must be absolute: " + build_dir; return false; }
    build_dir_ = paths::normalize(build_dir);
    dir_ = paths::join(build_dir_, NTRE_HR_MAILBOX_DIR);
    return paths::mkdir_p(dir_, err);
}

std::string Mailbox::abs(const std::string& rel) const { return paths::normalize(paths::join(build_dir_, rel)); }
std::string Mailbox::rel(const std::string& abs_path) const { return paths::relative(build_dir_, abs_path); }

std::string Mailbox::state_path(const std::string& module) const { return paths::join(dir_, state_file_name(module)); }
std::string Mailbox::shim_path(const std::string& module, uint32_t seq) const { return paths::join(dir_, shim_file_name(module, seq)); }
std::string Mailbox::manifest_path(const std::string& module, uint32_t seq) const { return paths::join(dir_, manifest_file_name(module, seq)); }
std::string Mailbox::result_path(const std::string& module, uint32_t seq) const { return paths::join(dir_, result_file_name(module, seq)); }
std::string Mailbox::sidecar_path() const { return paths::join(dir_, NTRE_HR_SIDECAR_FILE); }

bool Mailbox::write_json(const std::string& path, const json::Value& v, std::string& err) const {
    return paths::write_file_atomic(path, v.dump(2), err);
}

bool Mailbox::remove(const std::string& path) const { return paths::remove_file(path); }

std::vector<uint32_t> Mailbox::manifest_seqs(const std::string& module) const {
    std::vector<uint32_t> out;
    for (const std::string& name : paths::list_dir(dir_)) {
        uint32_t seq;
        if (parse_manifest_name(name, module, seq)) out.push_back(seq);
    }
    std::sort(out.begin(), out.end());
    return out;
}

bool Mailbox::parse_manifest(const std::string& text, Manifest& out, std::string& err) const {
    json::Value v;
    if (!json::parse(text, v, err)) return false;
    if (!v.is_object()) { err = "manifest is not an object"; return false; }
    out = Manifest();
    out.protocol = json::get_int(v, "protocol", -1);
    out.build_id = json::get_string(v, "build_id", "");
    out.module = json::get_string(v, "module", "");
    int64_t seq = json::get_int(v, "seq", 0);
    out.shim = json::get_string(v, "shim", "");
    out.created_unix_ms = json::get_int(v, "created_unix_ms", 0);
    if (out.protocol < 0) { err = "manifest: missing protocol"; return false; }
    if (seq > 0 && seq <= static_cast<int64_t>(UINT32_MAX)) out.seq = static_cast<uint32_t>(seq);
    // A manifest from another protocol version is returned as far as it parsed: the caller rejects
    // it by version instead of guessing which field of an unknown shape is missing.
    if (out.protocol != NTRE_HR_PROTOCOL_VERSION) return true;
    if (out.module.empty() || out.seq == 0 || out.shim.empty()) { err = "manifest: missing module, seq or shim"; return false; }
    if (const json::Value* lb = v.get("link_base")) {
        if (lb->is_string()) {
            if (!parse_hex_address(lb->as_string(), out.link_base)) { err = "manifest: bad link_base"; return false; }
            out.has_link_base = true;
        }
    }
    if (const json::Value* mb = v.get("module_base")) {
        if (mb->is_string()) {
            if (!parse_hex_address(mb->as_string(), out.module_base)) { err = "manifest: bad module_base"; return false; }
            out.has_module_base = true;
        }
    }
    out.session = json::get_string(v, "session", "");
    const json::Value* slot = v.get("slot");
    const json::Value* slots = v.get("slots");
    if (slot && slots && slot->is_number() && slots->is_number()) {
        out.has_slot = true;
        out.slot = static_cast<uint32_t>(slot->as_int());
        out.slots = static_cast<uint32_t>(slots->as_int());
        if (out.slots == 0) out.slots = 1;
    }
    const json::Value* units = v.get("units");
    if (!units || !units->is_array()) { err = "manifest: missing units"; return false; }
    for (size_t i = 0; i < units->size(); ++i) {
        const json::Value& u = units->at(i);
        ManifestUnit mu;
        mu.source = json::get_string(u, "source", "");
        mu.object = json::get_string(u, "object", "");
        if (mu.object.empty()) { err = "manifest: unit without object"; return false; }
        out.units.push_back(mu);
    }
    return true;
}

bool Mailbox::read_manifest(const std::string& module, uint32_t seq, Manifest& out, std::string& err) const {
    std::string path = manifest_path(module, seq);
    std::string text;
    if (!paths::read_file(path, text, err)) return false;
    if (!parse_manifest(text, out, err)) { err = path + ": " + err; return false; }
    out.manifest_path = path;
    out.shim_path = paths::join(dir_, out.shim);
    return true;
}

SidecarPresence Mailbox::sidecar_presence(int64_t stale_ms) const {
    SidecarPresence p;
    std::string path = sidecar_path();
    int64_t mtime = paths::mtime_ms(path);
    if (mtime < 0) return p;
    std::string text, err;
    if (!paths::read_file(path, text, err)) return p;
    json::Value v;
    if (!json::parse(text, v, err) || !v.is_object()) return p;
    p.present = true;
    p.age_ms = paths::now_unix_ms() - mtime;
    if (p.age_ms < 0) p.age_ms = 0;
    p.fresh = p.age_ms <= stale_ms;
    p.pid = json::get_int(v, "pid", 0);
    p.auto_apply = json::get_bool(v, "auto_apply", true);
    p.session = json::get_string(v, "session", "");
    p.protocol = json::get_int(v, "protocol", 0);
    return p;
}

} // namespace hr

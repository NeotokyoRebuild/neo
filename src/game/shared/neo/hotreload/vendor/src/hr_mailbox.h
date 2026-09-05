// Mailbox reader/writer, loader side: file names, atomic
// writes, manifest parsing, sidecar presence. Everything inside mailbox files is
// relative to the build dir; this class converts to and from absolute paths.
#ifndef NTRE_HR_MAILBOX_H
#define NTRE_HR_MAILBOX_H

#include <cstdint>
#include <string>
#include <vector>

#include "hr_json.h"

namespace hr {

struct ManifestUnit {
    std::string source; // relative to the build dir
    std::string object; // relative to the build dir
};

struct Manifest {
    int64_t protocol = 0;
    std::string build_id;
    std::string module;
    uint32_t seq = 0;
    std::string shim; // bare file name
    bool has_link_base = false;
    uintptr_t link_base = 0;
    bool has_slot = false;
    uint32_t slot = 0;
    uint32_t slots = 0;
    bool has_module_base = false; // the load bias the shim's symbol script was generated for
    uintptr_t module_base = 0;
    std::string session; // the watch session that published it, empty for one-shot publishes
    std::vector<ManifestUnit> units;
    int64_t created_unix_ms = 0;

    std::string manifest_path; // absolute, filled by Mailbox::read_manifest
    std::string shim_path;     // absolute
};

struct SidecarPresence {
    bool present = false;   // file exists and parses
    bool fresh = false;     // mtime younger than the stale threshold
    int64_t age_ms = -1;
    int64_t pid = 0;
    bool auto_apply = true;
    int64_t protocol = 0;
    std::string session; // the watch session id, empty for old sidecars
};

// Pure name helpers.
std::string state_file_name(const std::string& module);
std::string shim_file_name(const std::string& module, uint32_t seq);
std::string manifest_file_name(const std::string& module, uint32_t seq);
std::string result_file_name(const std::string& module, uint32_t seq);
// "<module>.<N>.json" -> N. False for result files, tmp files and other modules.
bool parse_manifest_name(const std::string& name, const std::string& module, uint32_t& seq);
bool parse_hex_address(const std::string& s, uintptr_t& out); // "0x7f..." (lowercase or uppercase)
std::string hex_address(uintptr_t a);                         // "0x7f..."

class Mailbox {
public:
    // build_dir must be absolute. Creates <build_dir>/.hotreload.
    bool init(const std::string& build_dir, std::string& err);

    const std::string& dir() const { return dir_; }
    const std::string& build_dir() const { return build_dir_; }
    std::string abs(const std::string& rel_to_build_dir) const;
    std::string rel(const std::string& abs_path) const;

    std::string state_path(const std::string& module) const;
    std::string shim_path(const std::string& module, uint32_t seq) const;
    std::string manifest_path(const std::string& module, uint32_t seq) const;
    std::string result_path(const std::string& module, uint32_t seq) const;
    std::string sidecar_path() const;

    bool write_json(const std::string& path, const json::Value& v, std::string& err) const;
    bool remove(const std::string& path) const;

    // All manifest sequence numbers present for a module, ascending.
    std::vector<uint32_t> manifest_seqs(const std::string& module) const;
    bool read_manifest(const std::string& module, uint32_t seq, Manifest& out, std::string& err) const;
    bool parse_manifest(const std::string& text, Manifest& out, std::string& err) const;

    SidecarPresence sidecar_presence(int64_t stale_ms) const;

private:
    std::string dir_;
    std::string build_dir_;
};

} // namespace hr

#endif

// Filesystem and path helpers (POSIX only, no std::filesystem to keep the old
// libstdc++ ABI build simple).
#ifndef NTRE_HR_PATHS_H
#define NTRE_HR_PATHS_H

#include <cstdint>
#include <string>
#include <vector>

namespace hr {
namespace paths {

bool is_absolute(const std::string& p);
std::string dirname(const std::string& p);   // "/a/b/c" -> "/a/b", "c" -> ".", "/" -> "/"
std::string basename(const std::string& p);  // "/a/b/c.so" -> "c.so"
std::string join(const std::string& a, const std::string& b); // b absolute wins
std::string normalize(const std::string& p); // collapse ".", "..", "//" lexically (no symlink resolution)
std::string realpath_or(const std::string& p); // realpath(3), or normalize(p) when it fails
// Lexical relative path from directory `from` to path `to`, both absolute and normalized.
std::string relative(const std::string& from_dir, const std::string& to);

bool exists(const std::string& p);
bool is_dir(const std::string& p);
bool mkdir_p(const std::string& p, std::string& err);
bool read_file(const std::string& p, std::string& out, std::string& err);
// Write to <p>.tmp and rename into place.
bool write_file_atomic(const std::string& p, const std::string& data, std::string& err);
bool remove_file(const std::string& p);
// mtime in unix milliseconds, or -1 when stat fails.
int64_t mtime_ms(const std::string& p);
// Directory entries (names only, no "." and "..").
std::vector<std::string> list_dir(const std::string& p);

int64_t now_unix_ms();
int64_t now_mono_ms();

// dladdr wrapper: path and load base of the object containing `anchor`.
bool module_of(const void* anchor, std::string& path, uintptr_t& base, std::string& err);

} // namespace paths
} // namespace hr

#endif

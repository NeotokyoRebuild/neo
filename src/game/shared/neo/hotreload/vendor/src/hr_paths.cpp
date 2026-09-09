#include "hr_paths.h"

#include <dlfcn.h>
#include <dirent.h>
#include <cerrno>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <sys/stat.h>
#include <unistd.h>

namespace hr {
namespace paths {

bool is_absolute(const std::string& p) { return !p.empty() && p[0] == '/'; }

std::string dirname(const std::string& p) {
    if (p.empty()) return ".";
    std::string s = p;
    while (s.size() > 1 && s.back() == '/') s.pop_back();
    size_t slash = s.rfind('/');
    if (slash == std::string::npos) return ".";
    if (slash == 0) return "/";
    return s.substr(0, slash);
}

std::string basename(const std::string& p) {
    std::string s = p;
    while (s.size() > 1 && s.back() == '/') s.pop_back();
    size_t slash = s.rfind('/');
    return slash == std::string::npos ? s : s.substr(slash + 1);
}

std::string join(const std::string& a, const std::string& b) {
    if (b.empty()) return a;
    if (is_absolute(b) || a.empty()) return b;
    if (a.back() == '/') return a + b;
    return a + "/" + b;
}

std::string normalize(const std::string& p) {
    std::vector<std::string> parts;
    bool abs = is_absolute(p);
    size_t i = 0;
    while (i < p.size()) {
        size_t j = p.find('/', i);
        if (j == std::string::npos) j = p.size();
        std::string seg = p.substr(i, j - i);
        if (seg.empty() || seg == ".") {
            // skip
        } else if (seg == "..") {
            if (!parts.empty() && parts.back() != "..") parts.pop_back();
            else if (!abs) parts.push_back("..");
        } else {
            parts.push_back(seg);
        }
        i = j + 1;
    }
    std::string out = abs ? "/" : "";
    for (size_t k = 0; k < parts.size(); ++k) {
        if (k) out += "/";
        out += parts[k];
    }
    if (out.empty()) out = abs ? "/" : ".";
    return out;
}

std::string realpath_or(const std::string& p) {
    char buf[PATH_MAX];
    if (::realpath(p.c_str(), buf)) return std::string(buf);
    return normalize(p);
}

std::string relative(const std::string& from_dir, const std::string& to) {
    std::string a = normalize(from_dir);
    std::string b = normalize(to);
    if (!is_absolute(a) || !is_absolute(b)) return b;
    std::vector<std::string> pa, pb;
    auto split = [](const std::string& s, std::vector<std::string>& out) {
        size_t i = 1;
        while (i <= s.size()) {
            size_t j = s.find('/', i);
            if (j == std::string::npos) j = s.size();
            if (j > i) out.push_back(s.substr(i, j - i));
            i = j + 1;
        }
    };
    split(a, pa);
    split(b, pb);
    size_t common = 0;
    while (common < pa.size() && common < pb.size() && pa[common] == pb[common]) ++common;
    std::string out;
    for (size_t k = common; k < pa.size(); ++k) out += out.empty() ? ".." : "/..";
    for (size_t k = common; k < pb.size(); ++k) out += out.empty() ? pb[k] : "/" + pb[k];
    if (out.empty()) out = ".";
    return out;
}

bool exists(const std::string& p) {
    struct stat st;
    return ::stat(p.c_str(), &st) == 0;
}

bool is_dir(const std::string& p) {
    struct stat st;
    return ::stat(p.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
}

bool mkdir_p(const std::string& p, std::string& err) {
    std::string cur;
    std::string n = normalize(p);
    size_t i = 0;
    if (is_absolute(n)) { cur = "/"; i = 1; }
    while (i <= n.size()) {
        size_t j = n.find('/', i);
        if (j == std::string::npos) j = n.size();
        if (j > i) {
            cur = join(cur, n.substr(i, j - i));
            if (::mkdir(cur.c_str(), 0775) != 0 && errno != EEXIST) {
                err = "mkdir " + cur + ": " + strerror(errno);
                return false;
            }
        }
        i = j + 1;
    }
    if (!is_dir(n)) { err = n + " is not a directory"; return false; }
    return true;
}

bool read_file(const std::string& p, std::string& out, std::string& err) {
    FILE* f = fopen(p.c_str(), "rb");
    if (!f) { err = p + ": " + strerror(errno); return false; }
    out.clear();
    char buf[65536];
    size_t n;
    while ((n = fread(buf, 1, sizeof buf, f)) > 0) out.append(buf, n);
    bool ok = !ferror(f);
    if (!ok) err = p + ": read error";
    fclose(f);
    return ok;
}

bool write_file_atomic(const std::string& p, const std::string& data, std::string& err) {
    std::string tmp = p + ".tmp";
    FILE* f = fopen(tmp.c_str(), "wb");
    if (!f) { err = tmp + ": " + strerror(errno); return false; }
    bool ok = fwrite(data.data(), 1, data.size(), f) == data.size();
    ok = (fflush(f) == 0) && ok;
    ok = (fclose(f) == 0) && ok;
    if (!ok) { err = tmp + ": write error"; ::unlink(tmp.c_str()); return false; }
    if (::rename(tmp.c_str(), p.c_str()) != 0) {
        err = "rename " + tmp + " -> " + p + ": " + strerror(errno);
        ::unlink(tmp.c_str());
        return false;
    }
    return true;
}

bool remove_file(const std::string& p) { return ::unlink(p.c_str()) == 0; }

int64_t mtime_ms(const std::string& p) {
    struct stat st;
    if (::stat(p.c_str(), &st) != 0) return -1;
    return static_cast<int64_t>(st.st_mtim.tv_sec) * 1000 + st.st_mtim.tv_nsec / 1000000;
}

std::vector<std::string> list_dir(const std::string& p) {
    std::vector<std::string> out;
    DIR* d = ::opendir(p.c_str());
    if (!d) return out;
    while (struct dirent* e = ::readdir(d)) {
        if (strcmp(e->d_name, ".") == 0 || strcmp(e->d_name, "..") == 0) continue;
        out.push_back(e->d_name);
    }
    ::closedir(d);
    return out;
}

int64_t now_unix_ms() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return static_cast<int64_t>(ts.tv_sec) * 1000 + ts.tv_nsec / 1000000;
}

int64_t now_mono_ms() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return static_cast<int64_t>(ts.tv_sec) * 1000 + ts.tv_nsec / 1000000;
}

bool module_of(const void* anchor, std::string& path, uintptr_t& base, std::string& err) {
    Dl_info info;
    if (!anchor || dladdr(anchor, &info) == 0 || !info.dli_fname) {
        err = "dladdr could not resolve the module anchor";
        return false;
    }
    path = realpath_or(info.dli_fname);
    base = reinterpret_cast<uintptr_t>(info.dli_fbase);
    return true;
}

} // namespace paths
} // namespace hr

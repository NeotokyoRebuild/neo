#include "hr_elf.h"

#include <fcntl.h>
#include <link.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>

namespace hr {
namespace elf {

File::~File() { close(); }

void File::close() {
    if (map_) { ::munmap(map_, len_); map_ = nullptr; }
    if (fd_ >= 0) { ::close(fd_); fd_ = -1; }
    len_ = 0;
    sections_.clear();
}

bool File::open(const std::string& path, std::string& err) {
    close();
    path_ = path;
    fd_ = ::open(path.c_str(), O_RDONLY | O_CLOEXEC);
    if (fd_ < 0) { err = path + ": " + strerror(errno); return false; }
    struct stat st;
    if (::fstat(fd_, &st) != 0 || st.st_size < static_cast<off_t>(sizeof(Elf64_Ehdr))) {
        err = path + ": not an ELF file (too small)";
        close();
        return false;
    }
    len_ = static_cast<size_t>(st.st_size);
    ino_ = static_cast<uint64_t>(st.st_ino);
    mtime_ns_ = static_cast<int64_t>(st.st_mtim.tv_sec) * 1000000000 + st.st_mtim.tv_nsec;
    void* m = ::mmap(nullptr, len_, PROT_READ, MAP_PRIVATE, fd_, 0);
    if (m == MAP_FAILED) { err = path + ": mmap: " + strerror(errno); map_ = nullptr; close(); return false; }
    map_ = static_cast<uint8_t*>(m);
    if (!parse(err)) { close(); return false; }
    return true;
}

bool File::changed_on_disk() const {
    if (!map_) return false;
    struct stat st;
    if (::stat(path_.c_str(), &st) != 0) return true;
    int64_t mtime_ns = static_cast<int64_t>(st.st_mtim.tv_sec) * 1000000000 + st.st_mtim.tv_nsec;
    return static_cast<uint64_t>(st.st_ino) != ino_ || static_cast<size_t>(st.st_size) != len_ || mtime_ns != mtime_ns_;
}

bool File::parse(std::string& err) {
    const Elf64_Ehdr* eh = header();
    if (memcmp(eh->e_ident, ELFMAG, SELFMAG) != 0 || eh->e_ident[EI_CLASS] != ELFCLASS64 ||
        eh->e_ident[EI_DATA] != ELFDATA2LSB || eh->e_machine != EM_X86_64) {
        err = path_ + ": not a little endian x86-64 ELF64 file";
        return false;
    }
    if (eh->e_shoff == 0 || eh->e_shentsize != sizeof(Elf64_Shdr)) {
        // No section table (stripped beyond use). Keep the file open for program headers.
        return true;
    }
    if (!in_bounds(eh->e_shoff, static_cast<uint64_t>(eh->e_shnum) * sizeof(Elf64_Shdr))) {
        err = path_ + ": section table out of bounds";
        return false;
    }
    const Elf64_Shdr* sh = reinterpret_cast<const Elf64_Shdr*>(map_ + eh->e_shoff);
    const char* shstr = nullptr;
    uint64_t shstr_len = 0;
    if (eh->e_shstrndx < eh->e_shnum && in_bounds(sh[eh->e_shstrndx].sh_offset, sh[eh->e_shstrndx].sh_size)) {
        shstr = reinterpret_cast<const char*>(map_ + sh[eh->e_shstrndx].sh_offset);
        shstr_len = sh[eh->e_shstrndx].sh_size;
    }
    sections_.resize(eh->e_shnum);
    for (uint32_t i = 0; i < eh->e_shnum; ++i) {
        Section& s = sections_[i];
        if (shstr && sh[i].sh_name < shstr_len) s.name = std::string(shstr + sh[i].sh_name, strnlen(shstr + sh[i].sh_name, shstr_len - sh[i].sh_name));
        s.type = sh[i].sh_type;
        s.flags = sh[i].sh_flags;
        s.addr = sh[i].sh_addr;
        s.offset = sh[i].sh_offset;
        s.size = sh[i].sh_size;
        s.link = sh[i].sh_link;
        s.info = sh[i].sh_info;
        s.addralign = sh[i].sh_addralign;
        s.entsize = sh[i].sh_entsize;
    }
    return true;
}

const Section* File::find_section(const char* name) const {
    int i = find_section_index(name);
    return i < 0 ? nullptr : &sections_[static_cast<size_t>(i)];
}

int File::find_section_index(const char* name) const {
    for (size_t i = 0; i < sections_.size(); ++i)
        if (sections_[i].name == name) return static_cast<int>(i);
    return -1;
}

bool File::read_symbols(const char* table, std::vector<Symbol>& out, std::string& err) const {
    out.clear();
    const Section* sym = find_section(table);
    if (!sym) { err = path_ + ": no " + table + " section"; return false; }
    if (sym->type != SHT_SYMTAB && sym->type != SHT_DYNSYM) { err = path_ + ": " + table + " is not a symbol table"; return false; }
    if (sym->link >= sections_.size()) { err = path_ + ": bad symtab link"; return false; }
    const Section& str = sections_[sym->link];
    if (!in_bounds(sym->offset, sym->size) || !in_bounds(str.offset, str.size)) { err = path_ + ": symtab out of bounds"; return false; }
    const Elf64_Sym* syms = reinterpret_cast<const Elf64_Sym*>(map_ + sym->offset);
    size_t count = sym->size / sizeof(Elf64_Sym);
    const char* strs = reinterpret_cast<const char*>(map_ + str.offset);
    out.reserve(count);
    std::string current_file;
    for (size_t i = 0; i < count; ++i) {
        const Elf64_Sym& s = syms[i];
        Symbol o;
        o.index = static_cast<uint32_t>(i);
        o.type = ELF64_ST_TYPE(s.st_info);
        o.bind = ELF64_ST_BIND(s.st_info);
        o.value = s.st_value;
        o.size = s.st_size;
        o.shndx = s.st_shndx;
        if (s.st_name < str.size) o.name = std::string(strs + s.st_name, strnlen(strs + s.st_name, str.size - s.st_name));
        if (o.type == STT_FILE) {
            current_file = o.name;
        } else if (o.bind == STB_LOCAL) {
            o.file = current_file;
        } else {
            // Globals follow all locals in a valid symtab; scoping no longer applies.
            current_file.clear();
        }
        if (o.type == STT_SECTION && o.name.empty() && o.shndx < sections_.size()) o.name = sections_[o.shndx].name;
        out.push_back(std::move(o));
    }
    return true;
}

bool File::read_rela(const Section& sec, std::vector<Rela>& out, std::string& err) const {
    out.clear();
    if (sec.type != SHT_RELA) { err = path_ + ": " + sec.name + " is not a RELA section"; return false; }
    if (!in_bounds(sec.offset, sec.size)) { err = path_ + ": rela out of bounds"; return false; }
    const Elf64_Rela* r = reinterpret_cast<const Elf64_Rela*>(map_ + sec.offset);
    size_t count = sec.size / sizeof(Elf64_Rela);
    out.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        Rela o;
        o.offset = r[i].r_offset;
        o.sym = ELF64_R_SYM(r[i].r_info);
        o.type = ELF64_R_TYPE(r[i].r_info);
        o.addend = r[i].r_addend;
        out.push_back(o);
    }
    return true;
}

std::string to_hex(const uint8_t* p, size_t n) {
    static const char* digits = "0123456789abcdef";
    std::string s;
    s.reserve(n * 2);
    for (size_t i = 0; i < n; ++i) { s.push_back(digits[p[i] >> 4]); s.push_back(digits[p[i] & 15]); }
    return s;
}

std::string find_build_id_in_notes(const uint8_t* p, size_t len, size_t align) {
    if (align < 4) align = 4;
    size_t off = 0;
    while (off + sizeof(Elf64_Nhdr) <= len) {
        const Elf64_Nhdr* nh = reinterpret_cast<const Elf64_Nhdr*>(p + off);
        size_t name_off = off + sizeof(Elf64_Nhdr);
        size_t desc_off = name_off + ((nh->n_namesz + align - 1) & ~(align - 1));
        size_t next = desc_off + ((nh->n_descsz + align - 1) & ~(align - 1));
        if (desc_off > len || next > len) break;
        if (nh->n_type == NT_GNU_BUILD_ID && nh->n_namesz == 4 && memcmp(p + name_off, "GNU", 4) == 0 && nh->n_descsz > 0)
            return to_hex(p + desc_off, nh->n_descsz);
        off = next;
    }
    return std::string();
}

std::string File::build_id() const {
    if (!map_) return std::string();
    for (const Section& s : sections_) {
        if (s.type != SHT_NOTE || !in_bounds(s.offset, s.size)) continue;
        std::string id = find_build_id_in_notes(map_ + s.offset, s.size, s.addralign);
        if (!id.empty()) return id;
    }
    const Elf64_Ehdr* eh = header();
    if (eh->e_phoff && eh->e_phentsize == sizeof(Elf64_Phdr) && in_bounds(eh->e_phoff, static_cast<uint64_t>(eh->e_phnum) * sizeof(Elf64_Phdr))) {
        const Elf64_Phdr* ph = reinterpret_cast<const Elf64_Phdr*>(map_ + eh->e_phoff);
        for (uint32_t i = 0; i < eh->e_phnum; ++i) {
            if (ph[i].p_type != PT_NOTE || !in_bounds(ph[i].p_offset, ph[i].p_filesz)) continue;
            std::string id = find_build_id_in_notes(map_ + ph[i].p_offset, ph[i].p_filesz, ph[i].p_align);
            if (!id.empty()) return id;
        }
    }
    return std::string();
}

bool File::load_extent(uint64_t& lo, uint64_t& hi) const {
    if (!map_) return false;
    const Elf64_Ehdr* eh = header();
    if (!eh->e_phoff || eh->e_phentsize != sizeof(Elf64_Phdr) || !in_bounds(eh->e_phoff, static_cast<uint64_t>(eh->e_phnum) * sizeof(Elf64_Phdr))) return false;
    const Elf64_Phdr* ph = reinterpret_cast<const Elf64_Phdr*>(map_ + eh->e_phoff);
    bool any = false;
    lo = UINT64_MAX;
    hi = 0;
    for (uint32_t i = 0; i < eh->e_phnum; ++i) {
        if (ph[i].p_type != PT_LOAD) continue;
        any = true;
        if (ph[i].p_vaddr < lo) lo = ph[i].p_vaddr;
        if (ph[i].p_vaddr + ph[i].p_memsz > hi) hi = ph[i].p_vaddr + ph[i].p_memsz;
    }
    return any;
}

bool File::relro_extent(uint64_t& lo, uint64_t& hi) const {
    if (!map_) return false;
    const Elf64_Ehdr* eh = header();
    if (!eh->e_phoff || eh->e_phentsize != sizeof(Elf64_Phdr) || !in_bounds(eh->e_phoff, static_cast<uint64_t>(eh->e_phnum) * sizeof(Elf64_Phdr))) return false;
    const Elf64_Phdr* ph = reinterpret_cast<const Elf64_Phdr*>(map_ + eh->e_phoff);
    for (uint32_t i = 0; i < eh->e_phnum; ++i) {
        if (ph[i].p_type != PT_GNU_RELRO) continue;
        lo = ph[i].p_vaddr;
        hi = ph[i].p_vaddr + ph[i].p_memsz;
        return true;
    }
    return false;
}

namespace {

struct ImageQuery {
    uintptr_t addr;
    LoadedImage* out;
    bool found;
};

int image_callback(struct dl_phdr_info* info, size_t, void* data) {
    ImageQuery* q = static_cast<ImageQuery*>(data);
    uintptr_t lo = UINTPTR_MAX, hi = 0;
    bool contains = false;
    for (int i = 0; i < info->dlpi_phnum; ++i) {
        const ElfW(Phdr)& ph = info->dlpi_phdr[i];
        if (ph.p_type != PT_LOAD) continue;
        uintptr_t s = info->dlpi_addr + ph.p_vaddr;
        uintptr_t e = s + ph.p_memsz;
        if (s < lo) lo = s;
        if (e > hi) hi = e;
        if (q->addr >= s && q->addr < e) contains = true;
    }
    if (!contains) return 0;
    std::string build_id;
    for (int i = 0; i < info->dlpi_phnum && build_id.empty(); ++i) {
        const ElfW(Phdr)& ph = info->dlpi_phdr[i];
        if (ph.p_type != PT_NOTE) continue;
        const uint8_t* p = reinterpret_cast<const uint8_t*>(info->dlpi_addr + ph.p_vaddr);
        build_id = find_build_id_in_notes(p, ph.p_memsz, ph.p_align);
    }
    q->out->bias = info->dlpi_addr;
    q->out->lo = lo;
    q->out->hi = hi;
    q->out->build_id = build_id;
    q->out->name = info->dlpi_name ? info->dlpi_name : "";
    q->found = true;
    return 1;
}

} // namespace

bool find_loaded_image(uintptr_t addr, LoadedImage& out) {
    ImageQuery q{addr, &out, false};
    dl_iterate_phdr(image_callback, &q);
    return q.found;
}

bool is_std_symbol(const std::string& n) {
    static const char* const prefixes[] = {
        "_ZSt", "_ZNSt", "_ZNKSt", "_ZNVSt", "_ZNKVSt", "_ZNSa", "_ZNKSa", "_ZNSs", "_ZNKSs",
        "_ZNSb", "_ZNKSb", "_ZNSi", "_ZNSo", "_ZNSd", "_ZZNSt", "_ZZSt", "_ZTSSt", "_ZTVSt", "_ZTISt",
        "_ZGVZNSt", "_ZGVZSt", "_ZThn", "_ZTv",
    };
    for (const char* p : prefixes)
        if (n.compare(0, strlen(p), p) == 0) return true;
    return n.compare(0, 6, "__cxa_") == 0 || n.compare(0, 6, "__gxx_") == 0 || n.compare(0, 9, "__gnu_cxx") == 0;
}

bool is_crt_symbol(const std::string& n) {
    static const char* const names[] = {
        "_init", "_fini", "_start", "deregister_tm_clones", "register_tm_clones",
        "__do_global_dtors_aux", "frame_dummy", "__do_global_ctors_aux", "__libc_csu_init",
        "__libc_csu_fini", "_dl_relocate_static_pie", "__stack_chk_fail_local",
    };
    for (const char* p : names)
        if (n == p) return true;
    // Per-TU static init and teardown glue is present in both shim and module; hooking it is noise.
    static const char* const prefixes[] = {"_GLOBAL__sub_I_", "_GLOBAL__sub_D_", "__tcf_"};
    for (const char* p : prefixes)
        if (n.compare(0, strlen(p), p) == 0) return true;
    // Mangled as _Z41__static_initialization_and_destruction_0ii, so match anywhere.
    return n.find("__static_initialization_and_destruction_") != std::string::npos;
}

} // namespace elf
} // namespace hr

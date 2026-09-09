// ELF64 reader for the pieces the loader needs: section table, .symtab with
// STT_FILE scoping, RELA sections, GNU build-id, PT_LOAD extent. Reads files
// through mmap and never copies section data. Also helpers that inspect the
// images already loaded in this process through dl_iterate_phdr.
//
// Mined from jet-live's ElfProgramInfoLoader but
// reimplemented: no ELFIO, no exceptions.
#ifndef NTRE_HR_ELF_H
#define NTRE_HR_ELF_H

#include <elf.h>

#include <cstdint>
#include <string>
#include <vector>

namespace hr {
namespace elf {

struct Section {
    std::string name;
    uint32_t type = 0;
    uint64_t flags = 0;
    uint64_t addr = 0;
    uint64_t offset = 0;
    uint64_t size = 0;
    uint32_t link = 0;
    uint32_t info = 0;
    uint64_t addralign = 0;
    uint64_t entsize = 0;
};

struct Symbol {
    std::string name;
    std::string file; // preceding STT_FILE name for local symbols, empty for globals
    uint64_t value = 0;
    uint64_t size = 0;
    uint8_t type = 0; // STT_*
    uint8_t bind = 0; // STB_*
    uint16_t shndx = 0;
    uint32_t index = 0; // index in the symbol table
};

struct Rela {
    uint64_t offset = 0;
    uint32_t sym = 0;
    uint32_t type = 0; // R_X86_64_*
    int64_t addend = 0;
};

class File {
public:
    File() = default;
    ~File();
    File(const File&) = delete;
    File& operator=(const File&) = delete;

    bool open(const std::string& path, std::string& err);
    void close();
    bool is_open() const { return map_ != nullptr; }
    // The file at path() is no longer the one that was opened (inode, size or mtime differ), for
    // example because it was rebuilt in place. The mapping still shows the bytes it had at open.
    bool changed_on_disk() const;
    const std::string& path() const { return path_; }
    const uint8_t* data() const { return map_; }
    size_t size() const { return len_; }
    const Elf64_Ehdr* header() const { return reinterpret_cast<const Elf64_Ehdr*>(map_); }

    const std::vector<Section>& sections() const { return sections_; }
    const Section* find_section(const char* name) const;
    int find_section_index(const char* name) const;

    // Symbols of ".symtab" or ".dynsym". STT_SECTION symbols get the section name as `name`.
    bool read_symbols(const char* table, std::vector<Symbol>& out, std::string& err) const;
    bool read_rela(const Section& sec, std::vector<Rela>& out, std::string& err) const;

    // Lowercase hex, empty when the file has no GNU build-id note.
    std::string build_id() const;
    // Lowest and highest virtual address covered by PT_LOAD segments.
    bool load_extent(uint64_t& lo, uint64_t& hi) const;
    // The PT_GNU_RELRO range (read-only after relocation), false when there is none.
    bool relro_extent(uint64_t& lo, uint64_t& hi) const;

private:
    bool parse(std::string& err);
    bool in_bounds(uint64_t off, uint64_t len) const { return off <= len_ && len <= len_ - off; }

    int fd_ = -1;
    uint8_t* map_ = nullptr;
    size_t len_ = 0;
    std::string path_;
    std::vector<Section> sections_;
    // Identity of the file at open, for changed_on_disk().
    uint64_t ino_ = 0;
    int64_t mtime_ns_ = 0;
};

// Parse a run of ELF notes and return the GNU build-id as hex, or "".
std::string find_build_id_in_notes(const uint8_t* p, size_t len, size_t align);

// The image (shared object or executable) whose PT_LOAD segments contain `addr`.
struct LoadedImage {
    uintptr_t bias = 0;   // dlpi_addr: add to st_value / p_vaddr for runtime addresses
    uintptr_t lo = 0;     // mapped extent
    uintptr_t hi = 0;
    std::string build_id; // lowercase hex, "" when absent
    std::string name;     // dlpi_name as reported by ld.so (empty for the main program)
};
bool find_loaded_image(uintptr_t addr, LoadedImage& out);

// True for mangled names in namespace std and friends (library code present in both shim and module).
bool is_std_symbol(const std::string& name);
// True for crt glue every DSO carries (_init, frame_dummy, ...) and per-TU static init glue (_GLOBAL__sub_I_*).
bool is_crt_symbol(const std::string& name);

std::string to_hex(const uint8_t* p, size_t n);

} // namespace elf
} // namespace hr

#endif

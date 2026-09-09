#include "hr_got.h"

#include <elf.h>
#include <sys/mman.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <set>

namespace hr {

namespace {

const elf::Symbol* module_symbol(const std::map<std::string, std::vector<size_t>>& index, const std::vector<elf::Symbol>& syms, const std::string& name) {
    auto it = index.find(name);
    if (it == index.end()) return nullptr;
    for (size_t i : it->second) {
        const elf::Symbol& m = syms[i];
        if (m.bind != STB_LOCAL) return &m; // dynamic symbols are global or weak; so is the module's counterpart
    }
    return nullptr;
}

// Store a pointer into a slot. Slots in the shim's RELRO range (.got, .data.rel.ro) are read-only
// since dlopen finished relocating; open the page for the write and close it again.
bool write_slot(uintptr_t addr, uint64_t value, bool relro, std::string& err) {
    if (!relro) {
        memcpy(reinterpret_cast<void*>(addr), &value, sizeof value);
        return true;
    }
    long ps = sysconf(_SC_PAGESIZE);
    uintptr_t page = addr & ~static_cast<uintptr_t>(ps - 1);
    size_t span = (addr + sizeof value) - page;
    span = (span + static_cast<size_t>(ps) - 1) & ~static_cast<size_t>(ps - 1);
    if (mprotect(reinterpret_cast<void*>(page), span, PROT_READ | PROT_WRITE) != 0) { err = std::string("mprotect(rw): ") + strerror(errno); return false; }
    memcpy(reinterpret_cast<void*>(addr), &value, sizeof value);
    if (mprotect(reinterpret_cast<void*>(page), span, PROT_READ) != 0) { err = std::string("mprotect(r): ") + strerror(errno); return false; }
    return true;
}

} // namespace

bool rebind_globals(const GotRebindInput& in, GotRebindOutcome& out, const Logger& log) {
    out = GotRebindOutcome();
    const elf::File& shim = *in.shim;
    const std::vector<elf::Section>& secs = shim.sections();
    std::string err;

    int dynsym_index = shim.find_section_index(".dynsym");
    std::vector<elf::Symbol> dyn;
    if (dynsym_index < 0 || !shim.read_symbols(".dynsym", dyn, err)) {
        out.warnings.push_back("shim has no readable .dynsym, globals stay private copies" + (err.empty() ? std::string() : ": " + err));
        return true;
    }
    uint64_t relro_lo = 0, relro_hi = 0;
    const bool has_relro = shim.relro_extent(relro_lo, relro_hi);

    std::set<std::string> objects, functions, fresh, warned;
    uint32_t slots = 0, unhooked = 0;
    for (const elf::Section& s : secs) {
        if (s.type != SHT_RELA || static_cast<int>(s.link) != dynsym_index) continue; // .rela.dyn (and .rela.plt, whose JUMP_SLOTs are skipped below)
        std::vector<elf::Rela> rel;
        if (!shim.read_rela(s, rel, err)) { out.warnings.push_back(err); continue; }
        for (const elf::Rela& r : rel) {
            if (r.type != R_X86_64_GLOB_DAT && r.type != R_X86_64_64) continue;
            if (r.sym == 0 || r.sym >= dyn.size()) continue;
            const elf::Symbol& ds = dyn[r.sym];
            // Only the shim's own definitions: undefined ones were already bound to the module or a library.
            if (ds.shndx == SHN_UNDEF || ds.shndx == SHN_ABS || ds.name.empty()) continue;
            uintptr_t value = 0;
            if (ds.type == STT_OBJECT) {
                const elf::Symbol* m = module_symbol(*in.module_objects, *in.module_symbols, ds.name);
                if (!m) { fresh.insert(ds.name); continue; } // new object: stays shim-owned
                if (m->size != ds.size) {
                    if (warned.insert(ds.name).second)
                        out.warnings.push_back(ds.name + ": size differs between the module (" + std::to_string(m->size) + ") and the shim (" +
                                               std::to_string(ds.size) + "), not rebound; its type changed, restart for exact state");
                    continue;
                }
                value = in.module_base + m->value;
                objects.insert(ds.name);
            } else if (ds.type == STT_FUNC) {
                // A pointer to a function the module also has: point it at the module's entry while that
                // entry is hooked, so the pointer follows later reloads. Otherwise it stays on the shim's copy.
                const elf::Symbol* m = module_symbol(*in.module_funcs, *in.module_symbols, ds.name);
                if (!m) { fresh.insert(ds.name); continue; }
                uintptr_t entry = in.module_base + m->value;
                auto h = in.hooks->find(entry);
                if (h == in.hooks->end() || !h->second.active) { unhooked++; continue; }
                value = entry;
                functions.insert(ds.name);
            } else {
                continue; // TLS, ifuncs, section symbols: not ours to rebind
            }
            if (r.type == R_X86_64_64) value += static_cast<uintptr_t>(r.addend);
            const uintptr_t slot = in.shim_bias + r.offset;
            const bool relro = has_relro && r.offset >= relro_lo && r.offset < relro_hi;
            if (!write_slot(slot, value, relro, err)) { out.warnings.push_back(ds.name + ": " + err); continue; }
            slots++;
            log.debug("got: %s slot 0x%lx -> 0x%lx%s", ds.name.c_str(), static_cast<unsigned long>(slot), static_cast<unsigned long>(value), relro ? " (relro)" : "");
        }
    }
    out.rebound = static_cast<uint32_t>(objects.size() + functions.size());
    log.debug("got: %zu object(s) and %zu function(s) rebound in %u slot(s), %zu new, %u function pointer(s) left on unhooked entries",
              objects.size(), functions.size(), slots, fresh.size(), unhooked);
    return true;
}

} // namespace hr

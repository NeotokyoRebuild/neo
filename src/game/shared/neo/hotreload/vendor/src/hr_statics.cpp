#include "hr_statics.h"

#include <elf.h>

#include <algorithm>
#include <climits>
#include <cstring>

#include "hr_hook.h"
#include "hr_paths.h"

namespace hr {

namespace {

// One shim static with every site that references it.
struct Candidate {
    const elf::Symbol* shim_sym = nullptr;
    const elf::Symbol* module_sym = nullptr;
    uintptr_t shim_addr = 0;   // runtime address of the shim's copy
    uintptr_t module_addr = 0; // runtime address of the module's copy
    std::vector<uintptr_t> sites; // runtime addresses of the 32-bit displacement fields
};

// The one STT_OBJECT a --unique section holds: defined at the section's address, as large as the section.
const elf::Symbol* section_owner(const std::vector<elf::Symbol>& syms, uint16_t shndx, const elf::Section& sec) {
    for (const elf::Symbol& s : syms) {
        if (s.type != STT_OBJECT || s.shndx != shndx || s.value != sec.addr || s.size == 0) continue;
        if (s.size != sec.size) return nullptr; // more than one object in there: not a per-static section
        return &s;
    }
    return nullptr;
}

// The module's copy: locals by name and STT_FILE basename, others by name.
const elf::Symbol* match_module_object(const StaticShareInput& in, const elf::Symbol& shim_sym) {
    auto it = in.module_objects->find(shim_sym.name);
    if (it == in.module_objects->end()) return nullptr;
    if (shim_sym.bind == STB_LOCAL) {
        std::string want = paths::basename(shim_sym.file);
        for (size_t i : it->second) {
            const elf::Symbol& m = (*in.module_symbols)[i];
            if (m.bind == STB_LOCAL && paths::basename(m.file) == want) return &m;
        }
        return nullptr;
    }
    for (size_t i : it->second) {
        const elf::Symbol& m = (*in.module_symbols)[i];
        if (m.bind != STB_LOCAL) return &m;
    }
    return nullptr;
}

bool starts_with(const std::string& s, const char* p) { return s.compare(0, strlen(p), p) == 0; }

} // namespace

bool share_statics(const StaticShareInput& in, StaticShareOutcome& out, const Logger& log) {
    out = StaticShareOutcome();
    const elf::File& shim = *in.shim;
    const std::vector<elf::Section>& secs = shim.sections();
    const std::vector<elf::Symbol>& syms = *in.shim_symbols;

    int symtab_index = shim.find_section_index(".symtab");
    std::vector<const elf::Section*> relas;
    for (const elf::Section& s : secs)
        if (s.type == SHT_RELA && starts_with(s.name, ".rela.text") && static_cast<int>(s.link) == symtab_index) relas.push_back(&s);
    if (relas.empty()) {
        out.warnings.push_back("shim carries no .rela.text, statics stay private copies: link shims with -Wl,--emit-relocs -Wl,--unique=.data.* -Wl,--unique=.bss.* (update the sidecar)");
        return true;
    }

    uint64_t relro_lo = 0, relro_hi = 0;
    const bool has_relro = shim.relro_extent(relro_lo, relro_hi);

    // Sites inside the shim's static-init glue ran at dlopen and never run again; leave them alone.
    std::vector<std::pair<uint64_t, uint64_t>> init_ranges;
    for (const elf::Symbol& s : syms)
        if (s.type == STT_FUNC && s.size && elf::is_crt_symbol(s.name)) init_ranges.push_back({s.value, s.value + s.size});
    auto in_init = [&](uint64_t off) {
        for (const auto& r : init_ranges)
            if (off >= r.first && off < r.second) return true;
        return false;
    };

    std::map<const elf::Symbol*, Candidate> cands;
    uint32_t sites = 0, init_sites = 0;
    std::string err;
    for (const elf::Section* rs : relas) {
        std::vector<elf::Rela> rel;
        if (!shim.read_rela(*rs, rel, err)) { out.warnings.push_back(err); continue; }
        for (const elf::Rela& r : rel) {
            if (r.type != R_X86_64_PC32 || r.sym >= syms.size()) continue;
            const elf::Symbol& ss = syms[r.sym];
            if (ss.type != STT_SECTION || ss.shndx >= secs.size()) continue;
            const elf::Section& sec = secs[ss.shndx];
            // Only per-static sections (--unique over -fdata-sections); plain .data/.bss hold anything.
            if (!starts_with(sec.name, ".data.") && !starts_with(sec.name, ".bss.")) continue;
            if (!(sec.flags & SHF_ALLOC) || !(sec.flags & SHF_WRITE) || (sec.flags & SHF_TLS)) continue;
            if (has_relro && sec.addr >= relro_lo && sec.addr < relro_hi) continue; // read-only after relocation: nothing to share
            const elf::Symbol* owner = section_owner(syms, ss.shndx, sec);
            if (!owner) continue;
            if (in_init(r.offset)) { init_sites++; continue; }
            Candidate& c = cands[owner];
            if (!c.shim_sym) {
                c.shim_sym = owner;
                c.module_sym = match_module_object(in, *owner);
                c.shim_addr = in.shim_bias + owner->value;
                if (c.module_sym) c.module_addr = in.module_base + c.module_sym->value;
            }
            c.sites.push_back(in.shim_bias + r.offset);
            sites++;
        }
    }

    uint32_t fresh = 0;
    for (auto& kv : cands) {
        Candidate& c = kv.second;
        const std::string& name = c.shim_sym->name;
        if (!c.module_sym) {
            fresh++;
            log.debug("statics: %s is new, stays shim-owned (%zu site(s))", name.c_str(), c.sites.size());
            continue;
        }
        if (c.module_sym->size != c.shim_sym->size) {
            out.warnings.push_back(name + ": size differs between the module (" + std::to_string(c.module_sym->size) + ") and the shim (" +
                                   std::to_string(c.shim_sym->size) + "), not shared; its type changed, restart for exact state");
            continue;
        }
        if (!in.in_pc32_range) {
            // One-time copy of the module's bytes over the shim's copy; state diverges from here.
            memcpy(reinterpret_cast<void*>(c.shim_addr), reinterpret_cast<const void*>(c.module_addr), c.shim_sym->size);
            out.copied++;
            continue;
        }
        // Every site: displacement += module copy - shim copy. The instruction's own layout cancels out.
        const int64_t delta = static_cast<int64_t>(c.module_addr - c.shim_addr);
        bool ok = true;
        for (uintptr_t site : c.sites) {
            int32_t v;
            memcpy(&v, reinterpret_cast<const void*>(site), 4);
            int64_t nv = static_cast<int64_t>(v) + delta;
            if (nv < INT32_MIN || nv > INT32_MAX) { ok = false; break; }
        }
        if (!ok) {
            memcpy(reinterpret_cast<void*>(c.shim_addr), reinterpret_cast<const void*>(c.module_addr), c.shim_sym->size);
            out.copied++;
            out.warnings.push_back(name + ": a reference does not reach the module's copy; copied once instead, restart for exact state");
            continue;
        }
        for (uintptr_t site : c.sites) {
            int32_t v;
            memcpy(&v, reinterpret_cast<const void*>(site), 4);
            int32_t nv = static_cast<int32_t>(static_cast<int64_t>(v) + delta);
            if (!hook::write_code(site, &nv, 4, err)) { out.warnings.push_back(name + ": " + err); ok = false; break; }
        }
        if (!ok) continue;
        out.shared++;
        log.debug("statics: %s shared, %zu site(s) now read 0x%lx", name.c_str(), c.sites.size(), static_cast<unsigned long>(c.module_addr));
    }
    if (out.copied && !in.in_pc32_range)
        out.warnings.push_back(std::to_string(out.copied) + " static(s) copied once because the shim is out of PC32 range of the module; state diverges from here, restart for exact state");
    log.debug("statics: %u shared, %u copied, %u new, %u site(s), %u in static init left alone", out.shared, out.copied, fresh, sites, init_sites);
    return true;
}

} // namespace hr

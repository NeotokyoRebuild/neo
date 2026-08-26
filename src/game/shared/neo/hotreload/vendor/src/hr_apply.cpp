#include "hr_apply.h"

#include <dlfcn.h>
#include <link.h>

#include <cstring>

#include "hr_got.h"
#include "hr_paths.h"
#include "hr_statics.h"
#include "ntre_hr_protocol.h"

namespace hr {

bool ensure_module_symbols(ntre_hr& hr, std::string& err) {
    if (hr.module_symbols_loaded) return true;
    if (!hr.module_file.is_open() && !hr.module_file.open(hr.module_path, err)) return false;
    std::string disk_id = hr.module_file.build_id();
    if (!hr.build_id.empty() && disk_id != hr.build_id) {
        err = "module on disk (" + disk_id.substr(0, 12) + ") differs from the loaded module (" + hr.build_id.substr(0, 12) +
              "): it was rebuilt after launch, restart the game";
        return false;
    }
    if (!hr.module_file.read_symbols(".symtab", hr.module_symbols, err)) {
        err += " (hot reload needs an unstripped Debug build)";
        return false;
    }
    hr.module_funcs.clear();
    hr.module_objects.clear();
    for (size_t i = 0; i < hr.module_symbols.size(); ++i) {
        const elf::Symbol& s = hr.module_symbols[i];
        if (s.shndx == SHN_UNDEF || s.shndx >= hr.module_file.sections().size() || s.value == 0) continue;
        if (s.type == STT_FUNC) hr.module_funcs[s.name].push_back(i);
        else if (s.type == STT_OBJECT && s.size > 0) hr.module_objects[s.name].push_back(i);
    }
    hr.module_symbols_loaded = true;
    hr.log.debug("module symbols: %zu total, %zu functions, %zu objects", hr.module_symbols.size(), hr.module_funcs.size(), hr.module_objects.size());
    return true;
}

namespace {

// Pick the module function matching a shim function: globals by name, locals by name and
// STT_FILE basename (two translation units may both define `static void helper()`). A local
// without a file match is new to its translation unit, never a hook target: the shim and the
// module were compiled by the same command, so a same-file static always carries the same file.
const elf::Symbol* match_module_function(const ntre_hr& hr, const elf::Symbol& shim_sym) {
    auto it = hr.module_funcs.find(shim_sym.name);
    if (it == hr.module_funcs.end()) return nullptr;
    const std::vector<size_t>& cands = it->second;
    if (shim_sym.bind == STB_LOCAL) {
        std::string want = paths::basename(shim_sym.file);
        for (size_t i : cands) {
            const elf::Symbol& m = hr.module_symbols[i];
            if (m.bind == STB_LOCAL && paths::basename(m.file) == want) return &m;
        }
        return nullptr;
    }
    for (size_t i : cands) {
        const elf::Symbol& m = hr.module_symbols[i];
        if (m.bind != STB_LOCAL) return &m;
    }
    return nullptr;
}

void hook_functions(ntre_hr& hr, const elf::File& shim, const std::vector<elf::Symbol>& shim_syms, uintptr_t bias, ApplyOutcome& out) {
    const std::vector<elf::Section>& shim_secs = shim.sections();
    const std::vector<elf::Section>& mod_secs = hr.module_file.sections();
    uint32_t long_form = 0; // functions hooked with the 13 byte jump because the shim is out of rel32 reach
    for (const elf::Symbol& s : shim_syms) {
        if (s.type != STT_FUNC || s.shndx == SHN_UNDEF || s.shndx >= shim_secs.size() || s.value == 0) continue;
        if (!(shim_secs[s.shndx].flags & SHF_EXECINSTR)) continue;
        if (elf::is_crt_symbol(s.name) || elf::is_std_symbol(s.name)) {
            hr.log.debug("hook: skip library symbol %s", s.name.c_str());
            continue;
        }
        const elf::Symbol* m = match_module_function(hr, s);
        if (!m) {
            hr.log.debug("hook: %s is new (no original)", s.name.c_str());
            continue;
        }
        uintptr_t original = hr.module_base + m->value;
        uintptr_t target = bias + s.value;
        if (m->shndx >= mod_secs.size()) { out.counts.functions_skipped++; continue; }
        const elf::Section& msec = mod_secs[m->shndx];
        const size_t need = hook::short_reaches(original, target) ? hook::kShortSize : hook::kPatchSize;
        if (m->value + need > msec.addr + msec.size) {
            out.counts.functions_skipped++;
            out.warnings.push_back("skipped " + s.name + ": fewer than " + std::to_string(need) + " bytes before the end of " + msec.name);
            continue;
        }
        // The entry as the module file has it: what the saved copy must hold, and the reference
        // that tells a debugger's int3 (0xCC) apart from the code's own bytes.
        const uint8_t* pristine = nullptr;
        if (msec.type == SHT_PROGBITS && msec.offset + (m->value - msec.addr) + hook::kPatchSize <= hr.module_file.size())
            pristine = hr.module_file.data() + msec.offset + (m->value - msec.addr);
        auto it = hr.hooks.find(original);
        hook::Patch fresh;
        hook::Patch& p = it != hr.hooks.end() ? it->second : fresh;
        hook::Report r = hook::install(original, target, p, pristine);
        switch (r.outcome) {
        case hook::Outcome::Installed:
        case hook::Outcome::Repointed:
            out.counts.functions_hooked++;
            if (r.form == hook::Form::Long) long_form++;
            hr.log.debug("hook: %s 0x%lx -> 0x%lx (%s form)", s.name.c_str(), static_cast<unsigned long>(original), static_cast<unsigned long>(target),
                         r.form == hook::Form::Short ? "5 byte" : "13 byte");
            break;
        case hook::Outcome::RepointedAroundInt3:
            out.counts.functions_hooked++;
            if (r.form == hook::Form::Long) long_form++;
            out.warnings.push_back(s.name + ": debugger breakpoint (int3 0xCC) at entry offset " + hook::offsets(r.int3_mask) + " kept; the jump was re-pointed around it");
            break;
        case hook::Outcome::RepointedOverInt3:
            out.counts.functions_hooked++;
            if (r.form == hook::Form::Long) long_form++;
            out.warnings.push_back(s.name + ": debugger breakpoint (int3 0xCC) at entry offset " + hook::offsets(r.int3_mask) +
                                   " sat on bytes the hook rewrites and was displaced; delete that breakpoint now (the debugger will corrupt the jump when it restores its byte) and set it in the shim instead");
            break;
        case hook::Outcome::SkippedInt3:
            out.counts.functions_skipped++;
            out.warnings.push_back("skipped " + s.name + ": debugger breakpoint (int3 0xCC) at entry offset " + hook::offsets(r.int3_mask) + "; remove it and save again");
            break;
        case hook::Outcome::SkippedForeign:
            out.counts.functions_skipped++;
            out.warnings.push_back("skipped " + s.name + ": entry bytes differ from the module file at offset " + hook::offsets(r.foreign_mask) + " (patched by another tool?)");
            break;
        case hook::Outcome::Failed:
            out.counts.functions_skipped++;
            out.warnings.push_back("skipped " + s.name + ": " + r.err);
            break;
        }
        if (p.active && it == hr.hooks.end()) hr.hooks[original] = p;
    }
    if (long_form)
        out.warnings.push_back(std::to_string(long_form) + " function(s) hooked with the 13 byte jump because the shim is more than 2 GB from the module: a breakpoint placed after their prologue in the module would corrupt the jump, set such breakpoints in the shim");
}

// Everything after the shim is mapped: read its symbols, hook, share statics, rebind globals.
// Returns false with out.error set; the counts still reflect what was done before the failure.
bool apply_mapped(ntre_hr& hr, const Manifest& m, uintptr_t bias, ApplyOutcome& out) {
    std::string err;
    elf::File shim;
    if (!shim.open(m.shim_path, err)) { out.error = err; return false; }
    uint64_t slo = 0, shi = 0;
    shim.load_extent(slo, shi);
    uintptr_t shim_lo = bias + slo, shim_hi = bias + shi;
    bool in_range = pc32_reachable(shim_lo, shim_hi, hr.module_lo, hr.module_hi);
    if (!in_range) out.warnings.push_back("shim at " + hex_address(shim_lo) + " is out of PC32 range of the module; statics will be copied, restart for exact state");
    hr.log.debug("shim %s mapped at 0x%lx..0x%lx (bias 0x%lx, %s)", m.shim.c_str(), static_cast<unsigned long>(shim_lo),
                 static_cast<unsigned long>(shim_hi), static_cast<unsigned long>(bias), in_range ? "in range" : "out of range");

    std::vector<elf::Symbol> shim_syms;
    if (!shim.read_symbols(".symtab", shim_syms, err)) { out.error = "shim has no .symtab: " + err; return false; }
    // The handle its static destructors were registered under; shutdown finalizes them with it.
    for (const elf::Symbol& s : shim_syms) {
        if (s.name == "__dso_handle" && s.shndx != SHN_UNDEF && !hr.shim_dso_handles.empty()) {
            hr.shim_dso_handles.back() = bias + s.value;
            break;
        }
    }

    hook_functions(hr, shim, shim_syms, bias, out);

    StaticShareInput si;
    si.shim = &shim;
    si.shim_bias = bias;
    si.shim_symbols = &shim_syms;
    si.module_base = hr.module_base;
    si.module_symbols = &hr.module_symbols;
    si.module_objects = &hr.module_objects;
    si.in_pc32_range = in_range;
    StaticShareOutcome so;
    share_statics(si, so, hr.log);
    out.counts.statics_shared += so.shared;
    out.counts.statics_copied += so.copied;
    out.warnings.insert(out.warnings.end(), so.warnings.begin(), so.warnings.end());

    GotRebindInput gi;
    gi.shim = &shim;
    gi.shim_bias = bias;
    gi.module_base = hr.module_base;
    gi.module_symbols = &hr.module_symbols;
    gi.module_objects = &hr.module_objects;
    gi.module_funcs = &hr.module_funcs;
    gi.hooks = &hr.hooks;
    GotRebindOutcome go;
    rebind_globals(gi, go, hr.log);
    out.counts.globals_rebound += go.rebound;
    out.warnings.insert(out.warnings.end(), go.warnings.begin(), go.warnings.end());
    return true;
}

} // namespace

bool apply_shim(ntre_hr& hr, const Manifest& m, ApplyOutcome& out) {
    out = ApplyOutcome();
    out.status = NTRE_HR_STATUS_FAILED;
    std::string err;

    if (!ensure_module_symbols(hr, err)) { out.error = err; return false; }
    if (hr.module_file.changed_on_disk()) {
        out.error = "the module on disk changed since launch (rebuilt in place): restart the game";
        return false;
    }
    if (!paths::exists(m.shim_path)) { out.error = "shim file missing: " + m.shim_path; return false; }
    if (m.has_module_base && m.module_base != hr.module_base) {
        out.error = "shim binds the module at " + hex_address(m.module_base) + " but this process has it at " + hex_address(hr.module_base) + ": save again";
        return false;
    }

    ntre_hr_shim_info info;
    memset(&info, 0, sizeof info);
    info.struct_size = sizeof info;
    info.module = hr.module_name.c_str();
    info.seq = m.seq;
    info.shim_path = m.shim_path.c_str();
    info.handle = nullptr;
    if (hr.pre_apply) hr.pre_apply(hr.apply_user, &info);

    // Free the advertised slots so ld.so's mmap hint (the linked base) lands there.
    bool released = false;
    if (m.has_slot && m.has_link_base && hr.region.reserved) {
        if (m.link_base != hr.region.slot_addr(m.slot) || m.slot + m.slots > hr.region.slot_count) {
            out.warnings.push_back("manifest link base " + hex_address(m.link_base) + " does not match this process's region; placement is up to the kernel");
        } else if (!hr.region.slots_free(m.slot, m.slots)) {
            out.warnings.push_back("region slot " + std::to_string(m.slot) + " is not free in this process (used by an earlier shim, or lost); placement is up to the kernel");
        } else {
            released = hr.region.release_slots(m.slot, m.slots);
            if (!released) out.warnings.push_back("could not release region slots for the shim; placement is up to the kernel");
        }
    }

    void* h = dlopen(m.shim_path.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (!h) {
        const char* e = dlerror();
        out.error = std::string("dlopen failed: ") + (e ? e : "unknown error");
        if (released && !hr.region.reclaim_slots(m.slot, m.slots))
            out.warnings.push_back("could not reclaim the released region slots; they are lost for this process");
        return false;
    }
    hr.shim_handles.push_back(h);
    hr.shim_dso_handles.push_back(0);
    info.handle = h;

    struct link_map* lm = nullptr;
    uintptr_t bias = 0;
    if (dlinfo(h, RTLD_DI_LINKMAP, &lm) == 0 && lm) bias = static_cast<uintptr_t>(lm->l_addr);

    // Slot bookkeeping first, so the region state is right whatever happens below.
    if (m.has_link_base) {
        if (bias == 0) {
            if (released) hr.region.occupy_slots(m.slot, m.slots);
        } else {
            out.warnings.push_back("shim landed at " + hex_address(m.link_base + bias) + " instead of " + hex_address(m.link_base) + " (mmap hint refused)");
            if (released && !hr.region.reclaim_slots(m.slot, m.slots))
                out.warnings.push_back("could not reclaim the released region slots; they are lost for this process");
        }
    }

    // The shim is mapped and its constructors have run: post_apply must run whatever happens next,
    // so the glue can rebuild its registries even when a later step fails.
    bool ok = apply_mapped(hr, m, bias, out);
    if (hr.post_apply) hr.post_apply(hr.apply_user, &info, &out.counts);
    out.status = ok ? NTRE_HR_STATUS_APPLIED : NTRE_HR_STATUS_FAILED;
    return ok;
}

} // namespace hr

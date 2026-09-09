// Static sharing. New code in the shim references its own copies
// of file-scope and function-local statics through 32-bit PC-relative
// displacements. For every such static that also exists in the original module
// the loader rewrites those displacements to the original's copy, so state is
// shared. Out of PC32 range: one-time byte copy plus a warning.
//
// The shim carries everything needed: the sidecar links it with
// -Wl,--emit-relocs -Wl,--unique=.data.* -Wl,--unique=.bss.*, so its .rela.text
// survives and every static sits in its own output section whose address names
// it through the shim's .symtab (with STT_FILE scoping, like functions). The
// new displacement is the old one plus (original address minus shim address):
// no instruction decoding, no object files.
#ifndef NTRE_HR_STATICS_H
#define NTRE_HR_STATICS_H

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "hr_elf.h"
#include "hr_log.h"

namespace hr {

struct StaticShareInput {
    const elf::File* shim = nullptr;                        // mapped shim file
    uintptr_t shim_bias = 0;                                // l_addr of the shim
    const std::vector<elf::Symbol>* shim_symbols = nullptr; // the shim's .symtab
    uintptr_t module_base = 0;                              // load bias of the module
    const std::vector<elf::Symbol>* module_symbols = nullptr;
    const std::map<std::string, std::vector<size_t>>* module_objects = nullptr; // name -> module_symbols indexes (STT_OBJECT)
    bool in_pc32_range = false;                             // shim within reach of the module
};

struct StaticShareOutcome {
    uint32_t shared = 0; // statics whose references now point at the module's copy
    uint32_t copied = 0; // statics whose shim copy received the module's bytes once
    std::vector<std::string> warnings;
};

bool share_statics(const StaticShareInput& in, StaticShareOutcome& out, const Logger& log);

} // namespace hr

#endif

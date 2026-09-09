// Global rebinding. Everything with external linkage that new
// code mentions (globals, class statics, ConVar objects, vtables, typeinfo,
// ServerClass/ClientClass objects) is reached through the shim's GOT or through
// absolute pointer slots in its data, and ld.so bound those slots to the shim's
// own copies: under RTLD_LOCAL the shim precedes its module in its own lookup
// scope. For every such object that also exists in the module (same name, same
// size) the slot is rewritten to the module's address, so new code reads the
// original globals and objects built by new code carry the original vtables.
// Function slots are rewritten too when the module's entry is hooked right now,
// so a pointer taken by new code follows later reloads through the hook.
#ifndef NTRE_HR_GOT_H
#define NTRE_HR_GOT_H

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "hr_elf.h"
#include "hr_hook.h"
#include "hr_log.h"

namespace hr {

struct GotRebindInput {
    const elf::File* shim = nullptr; // mapped shim file
    uintptr_t shim_bias = 0;         // l_addr of the shim
    uintptr_t module_base = 0;       // load bias of the module
    const std::vector<elf::Symbol>* module_symbols = nullptr;
    const std::map<std::string, std::vector<size_t>>* module_objects = nullptr; // name -> module_symbols indexes (STT_OBJECT)
    const std::map<std::string, std::vector<size_t>>* module_funcs = nullptr;   // name -> module_symbols indexes (STT_FUNC)
    const std::map<uintptr_t, hook::Patch>* hooks = nullptr;                    // module entry -> patch (to rebind function slots)
};

struct GotRebindOutcome {
    uint32_t rebound = 0; // distinct symbols whose slots now point at the module
    std::vector<std::string> warnings;
};

bool rebind_globals(const GotRebindInput& in, GotRebindOutcome& out, const Logger& log);

} // namespace hr

#endif

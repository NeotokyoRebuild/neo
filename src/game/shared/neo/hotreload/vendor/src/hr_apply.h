// The apply pipeline for one shim: map, read symbols, hook,
// share statics, rebind globals, registry callbacks.
#ifndef NTRE_HR_APPLY_H
#define NTRE_HR_APPLY_H

#include <string>
#include <vector>

#include "hr_context.h"

namespace hr {

struct ApplyOutcome {
    const char* status = "failed"; // NTRE_HR_STATUS_* from the protocol header
    ntre_hr_apply_counts counts = {};
    std::vector<std::string> warnings;
    std::string error;
};

// Open the module on disk, verify it is the loaded build and index its functions. Idempotent.
bool ensure_module_symbols(ntre_hr& hr, std::string& err);

// Apply one manifest. Returns true when the status is "applied".
bool apply_shim(ntre_hr& hr, const Manifest& m, ApplyOutcome& out);

} // namespace hr

#endif

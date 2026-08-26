// Entry hooking: redirect the original function's entry to the
// shim's copy. Two forms:
//   short  E9 rel32                    5 bytes, when the shim is within 2 GB of the entry
//                                      (always the case when it landed in its region slot)
//   long   49 BB imm64 ; 41 FF E3      13 bytes, movabs $target, %r11 ; jmp *%r11, otherwise
// The original entry is always the patch site, so re-hooking the same function
// for a newer shim just rewrites the jump. Safe because the module is built
// with -falign-functions=16.
//
// The short form matters for debuggers: a breakpoint placed after the prologue
// of the module's copy (the usual "break at function" spot, offset 8 to 11 at
// -O0) lands on bytes the short form never touches and never executes, so it
// is harmless there and fires in the shim instead; under the long form it
// would land inside the movabs and corrupt the jump.
//
// Debuggers also share the bytes we write: a software breakpoint is a 0xCC
// (int3) poked over the first byte of an instruction, and the debugger later
// writes its saved byte back. The hook therefore compares the bytes it is
// about to write with what they should hold (the module file's bytes, or the
// jump it installed earlier) and reports what it found.
//
// Not thread safe against threads executing the patched function while the
// bytes are written; apply on the frame thread only.
#ifndef NTRE_HR_HOOK_H
#define NTRE_HR_HOOK_H

#include <cstddef>
#include <cstdint>
#include <string>

namespace hr {
namespace hook {

constexpr size_t kPatchSize = 13; // the long form; also how many entry bytes are saved
constexpr size_t kShortSize = 5;
constexpr uint8_t kInt3 = 0xCC; // the one-byte x86 breakpoint instruction debuggers poke into code

enum class Form : uint8_t { Short, Long };
constexpr size_t form_size(Form f) { return f == Form::Short ? kShortSize : kPatchSize; }

// Whether `jmp rel32` at `entry` reaches `target`.
bool short_reaches(uintptr_t entry, uintptr_t target);

// Write the jump of the given form into out (form_size(form) bytes).
void encode(Form form, uintptr_t entry, uintptr_t target, uint8_t* out);

struct Patch {
    uintptr_t original = 0;          // patched entry
    uintptr_t target = 0;            // where it jumps now
    uint8_t saved[kPatchSize] = {};  // the entry as the module file has it
    Form form = Form::Short;         // form currently installed (when active)
    bool active = false;
};

enum class Outcome {
    Installed,           // first install over pristine bytes
    Repointed,           // jump updated, nothing else in the bytes written
    RepointedAroundInt3, // a breakpoint sat on bytes the write leaves unchanged; kept in place
    RepointedOverInt3,   // a breakpoint sat on bytes that change; displaced (the debugger's restore will corrupt the jump)
    SkippedInt3,         // first install refused: a debugger breakpoint is in the bytes to write
    SkippedForeign,      // refused: the bytes to write differ from what they should be in some other way
    Failed,              // bad arguments or mprotect; see err
};

struct Report {
    Outcome outcome = Outcome::Failed;
    Form form = Form::Short;   // form installed (when the outcome installed or re-pointed)
    uint32_t int3_mask = 0;    // bit i: offset i holds a debugger's int3
    uint32_t foreign_mask = 0; // bit i: offset i differs in some other way
    std::string err;
};

// Compare `span` bytes at `entry` with `expected`. Bit i of the result is set where memory holds
// 0xCC and `expected` does not (a debugger's breakpoint); `foreign_mask` gets every other
// difference.
uint32_t scan(uintptr_t entry, const uint8_t* expected, size_t span, uint32_t* foreign_mask);

// Install or re-point, choosing the short form when it reaches and `allow_short` (tests turn it
// off). `pristine` is the entry as the module file has it, kPatchSize bytes, required on the first
// install (the saved copy comes from there, never from memory, so a debugger's poke is never
// "restored" later); ignored on re-points. Only the bytes the chosen form needs are examined and
// written; all kPatchSize are when the form changes, so the other form's tail goes back to the
// saved bytes.
Report install(uintptr_t original, uintptr_t target, Patch& p, const uint8_t* pristine, bool allow_short = true);

// Put the saved bytes back, leaving a debugger's int3 where it is.
bool restore(Patch& p, std::string& err);

// "0" or "0,8": the offsets in a mask, for messages.
std::string offsets(uint32_t mask);

// Write `len` bytes into mapped code (mprotect to RWX around the write, back to RX after). Used by
// the static sharer to rewrite displacements inside the shim's .text.
bool write_code(uintptr_t addr, const void* src, size_t len, std::string& err);

} // namespace hook
} // namespace hr

#endif

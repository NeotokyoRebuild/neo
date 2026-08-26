#include "hr_hook.h"

#include <sys/mman.h>
#include <unistd.h>

#include <cerrno>
#include <climits>
#include <cstring>

namespace hr {
namespace hook {

bool short_reaches(uintptr_t entry, uintptr_t target) {
    int64_t d = static_cast<int64_t>(target - (entry + kShortSize));
    return d >= INT32_MIN && d <= INT32_MAX;
}

void encode(Form form, uintptr_t entry, uintptr_t target, uint8_t* out) {
    if (form == Form::Short) {
        // E9 rel32         jmp target
        int32_t rel = static_cast<int32_t>(static_cast<int64_t>(target - (entry + kShortSize)));
        out[0] = 0xE9;
        memcpy(out + 1, &rel, 4);
        return;
    }
    // 49 BB imm64      movabs $target, %r11
    // 41 FF E3         jmp *%r11
    out[0] = 0x49;
    out[1] = 0xBB;
    memcpy(out + 2, &target, 8);
    out[10] = 0x41;
    out[11] = 0xFF;
    out[12] = 0xE3;
}

std::string offsets(uint32_t mask) {
    std::string s;
    for (size_t i = 0; i < kPatchSize; ++i) {
        if (!(mask & (1u << i))) continue;
        if (!s.empty()) s += ",";
        s += std::to_string(i);
    }
    return s;
}

uint32_t scan(uintptr_t entry, const uint8_t* expected, size_t span, uint32_t* foreign_mask) {
    const uint8_t* mem = reinterpret_cast<const uint8_t*>(entry);
    uint32_t int3 = 0, foreign = 0;
    for (size_t i = 0; i < span && i < kPatchSize; ++i) {
        if (mem[i] == expected[i]) continue;
        if (mem[i] == kInt3) int3 |= 1u << i;
        else foreign |= 1u << i;
    }
    if (foreign_mask) *foreign_mask = foreign;
    return int3;
}

namespace {

bool with_writable(uintptr_t addr, size_t len, std::string& err, void (*fn)(void*), void* ctx) {
    long ps = sysconf(_SC_PAGESIZE);
    uintptr_t page = addr & ~static_cast<uintptr_t>(ps - 1);
    size_t span = (addr + len) - page;
    span = (span + static_cast<size_t>(ps) - 1) & ~static_cast<size_t>(ps - 1);
    if (mprotect(reinterpret_cast<void*>(page), span, PROT_READ | PROT_WRITE | PROT_EXEC) != 0) {
        err = std::string("mprotect(rwx): ") + strerror(errno);
        return false;
    }
    fn(ctx);
    if (mprotect(reinterpret_cast<void*>(page), span, PROT_READ | PROT_EXEC) != 0) {
        err = std::string("mprotect(rx): ") + strerror(errno);
        return false;
    }
    return true;
}

struct WriteCtx {
    uintptr_t dst;
    const uint8_t* src;
    size_t len;
};

void do_write(void* c) {
    WriteCtx* w = static_cast<WriteCtx*>(c);
    memcpy(reinterpret_cast<void*>(w->dst), w->src, w->len);
}

// The kPatchSize bytes an active patch should hold: its jump, then the saved tail.
void expected_bytes(const Patch& p, uint8_t* out) {
    memcpy(out, p.saved, kPatchSize);
    encode(p.form, p.original, p.target, out);
}

} // namespace

bool write_code(uintptr_t addr, const void* src, size_t len, std::string& err) {
    if (!addr || !src || !len) { err = "write_code: bad arguments"; return false; }
    WriteCtx w{addr, static_cast<const uint8_t*>(src), len};
    return with_writable(addr, len, err, do_write, &w);
}

Report install(uintptr_t original, uintptr_t target, Patch& p, const uint8_t* pristine, bool allow_short) {
    Report r;
    if (!original || !target) { r.err = "hook: null address"; return r; }
    if (p.original && p.original != original) { r.err = "hook: patch record belongs to another entry"; return r; }
    const bool first = !p.active;
    if (first && !pristine) { r.err = "hook: the entry's file image is required for the first install"; return r; }

    const Form form = (allow_short && short_reaches(original, target)) ? Form::Short : Form::Long;
    r.form = form;

    // What the entry holds now, and what it will hold.
    uint8_t expected[kPatchSize];
    uint8_t code[kPatchSize];
    if (first) {
        memcpy(expected, pristine, kPatchSize);
        memcpy(code, pristine, kPatchSize);
    } else {
        expected_bytes(p, expected);
        memcpy(code, p.saved, kPatchSize);
    }
    encode(form, original, target, code);

    // The bytes this write touches: the chosen form's, or all of them when the form changes (the
    // other form's tail goes back to the saved bytes).
    const size_t span = (!first && p.form != form) ? kPatchSize : form_size(form);

    r.int3_mask = scan(original, expected, span, &r.foreign_mask);
    if (r.foreign_mask) { r.outcome = Outcome::SkippedForeign; return r; }
    if (first && r.int3_mask) { r.outcome = Outcome::SkippedInt3; return r; }

    // Re-point: keep a breakpoint that sits on a byte the write leaves unchanged (the debugger
    // will restore exactly that byte later); one on a byte that changes has to go, and the caller
    // tells the developer, because the debugger's restore will then corrupt the jump.
    bool around = false, over = false;
    for (size_t i = 0; i < span; ++i) {
        if (!(r.int3_mask & (1u << i))) continue;
        if (code[i] == expected[i]) { code[i] = kInt3; around = true; }
        else over = true;
    }
    WriteCtx w{original, code, span};
    if (!with_writable(original, span, r.err, do_write, &w)) { r.outcome = Outcome::Failed; return r; }
    if (first) {
        p.original = original;
        memcpy(p.saved, pristine, kPatchSize);
    }
    p.target = target;
    p.form = form;
    p.active = true;
    r.outcome = first ? Outcome::Installed : over ? Outcome::RepointedOverInt3 : around ? Outcome::RepointedAroundInt3 : Outcome::Repointed;
    return r;
}

bool restore(Patch& p, std::string& err) {
    if (!p.original || !p.active) return true;
    uint8_t expected[kPatchSize];
    expected_bytes(p, expected);
    const size_t span = form_size(p.form);
    uint32_t foreign = 0;
    uint32_t int3 = scan(p.original, expected, span, &foreign);
    uint8_t code[kPatchSize];
    memcpy(code, p.saved, kPatchSize);
    for (size_t i = 0; i < span; ++i)
        if (int3 & (1u << i)) code[i] = kInt3; // the debugger owns that byte now
    WriteCtx w{p.original, code, span};
    if (!with_writable(p.original, span, err, do_write, &w)) return false;
    p.active = false;
    p.target = 0;
    return true;
}

} // namespace hook
} // namespace hr

#include "hr_region.h"

#include <sys/mman.h>

#include <cerrno>
#include <cstring>

#ifndef MAP_FIXED_NOREPLACE
#define MAP_FIXED_NOREPLACE 0x100000
#endif

namespace hr {

bool pc32_reachable(uintptr_t a_lo, uintptr_t a_hi, uintptr_t b_lo, uintptr_t b_hi) {
    const uint64_t limit = (1ull << 31) - (64ull << 20); // 2 GB minus a 64 MB margin
    uint64_t d1 = a_hi > b_lo ? a_hi - b_lo : b_lo - a_hi;
    uint64_t d2 = b_hi > a_lo ? b_hi - a_lo : a_lo - b_hi;
    return d1 < limit && d2 < limit;
}

namespace {

const uintptr_t kAlign = 2u << 20; // 2 MB, keeps hints huge-page friendly

uintptr_t align_down(uintptr_t v, uintptr_t a) { return v & ~(a - 1); }
uintptr_t align_up(uintptr_t v, uintptr_t a) { return (v + a - 1) & ~(a - 1); }

void* map_at(uintptr_t hint, uint64_t size, bool fixed_noreplace) {
    int flags = MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE;
    if (fixed_noreplace) flags |= MAP_FIXED_NOREPLACE;
    void* p = mmap(reinterpret_cast<void*>(hint), size, PROT_NONE, flags, -1, 0);
    return p;
}

} // namespace

bool Region::reserve(uintptr_t mod_lo, uintptr_t mod_hi, uint64_t slot_size_, uint32_t slot_count_,
                     uintptr_t preferred_base, const Logger& log) {
    unreserve();
    slot_size = slot_size_;
    slot_count = slot_count_;
    next_slot = 0;
    const uint64_t total = size();
    if (total == 0) { log.error("region: zero size"); return false; }

    std::vector<uintptr_t> hints;
    if (preferred_base) hints.push_back(preferred_base);
    static const uint64_t gaps[] = {64ull << 20, 256ull << 20, 512ull << 20, 1024ull << 20, 1536ull << 20};
    for (uint64_t gap : gaps) {
        if (mod_lo > gap + total) hints.push_back(align_down(mod_lo - gap - total, kAlign));
    }
    for (uint64_t gap : gaps) hints.push_back(align_up(mod_hi + gap, kAlign));

    for (uintptr_t hint : hints) {
        void* p = map_at(hint, total, true);
        if (p == MAP_FAILED) continue;
        if (reinterpret_cast<uintptr_t>(p) != hint) {
            // Kernel without MAP_FIXED_NOREPLACE ignored the flag and placed it elsewhere.
            munmap(p, total);
            continue;
        }
        if (!pc32_reachable(hint, hint + total, mod_lo, mod_hi)) {
            munmap(p, total);
            continue;
        }
        base = hint;
        reserved = true;
        in_range = true;
        states.assign(slot_count, SlotState::Free);
        log.info("region: reserved %u x %llu KB at 0x%lx (module 0x%lx..0x%lx, in PC32 range)",
                 slot_count, static_cast<unsigned long long>(slot_size >> 10), static_cast<unsigned long>(base),
                 static_cast<unsigned long>(mod_lo), static_cast<unsigned long>(mod_hi));
        return true;
    }

    // Nothing near the module was free: take whatever the kernel gives and flag it.
    void* p = map_at(0, total, false);
    if (p == MAP_FAILED) {
        log.error("region: mmap failed: %s", strerror(errno));
        return false;
    }
    base = reinterpret_cast<uintptr_t>(p);
    reserved = true;
    in_range = pc32_reachable(base, base + total, mod_lo, mod_hi);
    states.assign(slot_count, SlotState::Free);
    log.warn("region: no free range near the module; reserved at 0x%lx (%s). Statics will be %s.",
             static_cast<unsigned long>(base), in_range ? "in PC32 range by luck" : "out of PC32 range",
             in_range ? "shared" : "copied, restart for exact state");
    return true;
}

bool Region::in_bounds(uint32_t first, uint32_t count) const {
    return reserved && count > 0 && first < slot_count && count <= slot_count - first;
}

bool Region::all_in_state(uint32_t first, uint32_t count, SlotState s) const {
    for (uint32_t i = first; i < first + count; ++i)
        if (states[i] != s) return false;
    return true;
}

void Region::set_state(uint32_t first, uint32_t count, SlotState s) {
    for (uint32_t i = first; i < first + count; ++i) states[i] = s;
}

bool Region::slots_free(uint32_t first, uint32_t count) const {
    return in_bounds(first, count) && all_in_state(first, count, SlotState::Free);
}

bool Region::release_slots(uint32_t first, uint32_t count) {
    if (!slots_free(first, count)) return false;
    if (munmap(reinterpret_cast<void*>(slot_addr(first)), count * slot_size) != 0) return false;
    set_state(first, count, SlotState::Released);
    return true;
}

bool Region::reclaim_slots(uint32_t first, uint32_t count) {
    if (!in_bounds(first, count) || !all_in_state(first, count, SlotState::Released)) return false;
    uintptr_t addr = slot_addr(first);
    void* p = map_at(addr, count * slot_size, true);
    if (p != MAP_FAILED && reinterpret_cast<uintptr_t>(p) == addr) {
        set_state(first, count, SlotState::Free);
        return true;
    }
    if (p != MAP_FAILED) munmap(p, count * slot_size);
    // Something else lives there now; never release or unmap these slots again.
    set_state(first, count, SlotState::Lost);
    if (first + count > next_slot) next_slot = first + count;
    return false;
}

void Region::occupy_slots(uint32_t first, uint32_t count) {
    if (!in_bounds(first, count)) return;
    set_state(first, count, SlotState::Occupied);
    if (first + count > next_slot) next_slot = first + count;
}

void Region::unreserve() {
    if (reserved && base) {
        // Only Free slots are still our mappings: Occupied ones belong to ld.so, Lost ones to whoever landed there.
        uint32_t i = 0;
        while (i < slot_count) {
            if (states[i] != SlotState::Free) { ++i; continue; }
            uint32_t j = i;
            while (j < slot_count && states[j] == SlotState::Free) ++j;
            munmap(reinterpret_cast<void*>(slot_addr(i)), (j - i) * slot_size);
            i = j;
        }
    }
    base = 0;
    reserved = false;
    in_range = false;
    next_slot = 0;
    states.clear();
}

} // namespace hr

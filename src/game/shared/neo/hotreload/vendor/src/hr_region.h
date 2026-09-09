// Address region reservation: a PROT_NONE mapping near the
// module, carved into slots. The sidecar links each shim at a slot address; the
// loader releases the slot right before dlopen so ld.so's mmap hint (the linked
// base) lands there, which keeps shim code within PC32 reach of the module's
// statics. When no in-range reservation is possible the loader says so and
// static sharing falls back to byte copies.
//
// Every slot carries a state so the loader never unmaps memory it no longer
// owns: a slot that a shim landed in belongs to ld.so, and a slot that could
// not be reclaimed after a refused hint may belong to anyone.
#ifndef NTRE_HR_REGION_H
#define NTRE_HR_REGION_H

#include <cstdint>
#include <vector>

#include "hr_log.h"

namespace hr {

// Every address in [a_lo, a_hi) is within PC32 reach of every address in [b_lo, b_hi), with margin.
bool pc32_reachable(uintptr_t a_lo, uintptr_t a_hi, uintptr_t b_lo, uintptr_t b_hi);

enum class SlotState : uint8_t {
    Free,     // PROT_NONE mapping owned by the loader
    Released, // unmapped for a dlopen in progress
    Occupied, // a shim landed here; the mapping belongs to ld.so now
    Lost,     // released, the shim landed elsewhere and the slot could not be reclaimed; never touched again
};

struct Region {
    uintptr_t base = 0;
    uint64_t slot_size = 0;
    uint32_t slot_count = 0;
    uint32_t next_slot = 0; // published to the sidecar: every slot at or above this index is Free
    bool reserved = false;
    bool in_range = false;
    std::vector<SlotState> states; // one per slot, empty when not reserved

    uint64_t size() const { return slot_size * slot_count; }
    uintptr_t slot_addr(uint32_t slot) const { return base + slot * slot_size; }
    bool contains(uintptr_t addr, uint64_t len) const { return reserved && addr >= base && addr + len <= base + size(); }
    uint32_t slots_for(uint64_t bytes) const { return slot_size ? static_cast<uint32_t>((bytes + slot_size - 1) / slot_size) : 0; }
    // In bounds and every slot Free.
    bool slots_free(uint32_t first, uint32_t count) const;

    // Reserve near the module mapped at [mod_lo, mod_hi). preferred_base is tried first when
    // non-zero (used to replay shims linked for a previous process). Logs what happened.
    bool reserve(uintptr_t mod_lo, uintptr_t mod_hi, uint64_t slot_size, uint32_t slot_count,
                 uintptr_t preferred_base, const Logger& log);

    // Free -> Released: munmap the slots so dlopen can land the shim there.
    bool release_slots(uint32_t first, uint32_t count);
    // Released -> Free: put the PROT_NONE mapping back (the shim landed elsewhere or dlopen failed).
    // When that fails the slots become Lost and next_slot moves past them.
    bool reclaim_slots(uint32_t first, uint32_t count);
    // Released -> Occupied: the shim landed in the slots; next_slot moves past them.
    void occupy_slots(uint32_t first, uint32_t count);

    // munmap what is still ours (Free slots) and forget the reservation.
    void unreserve();

private:
    bool in_bounds(uint32_t first, uint32_t count) const;
    bool all_in_state(uint32_t first, uint32_t count, SlotState s) const;
    void set_state(uint32_t first, uint32_t count, SlotState s);
};

} // namespace hr

#endif

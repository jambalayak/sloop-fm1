/* SPDX-License-Identifier: GPL-3.0-only
 * HORATIO-1 build-only layout probe. This object is compiled by
 * --check-plaits-cxx but is never linked into the installable firmware. */
#include <stdint.h>
#include "plaits/dsp/voice.h"
#include "stmlib/utils/buffer_allocator.h"

struct HoratioProbeSlot {
    uint8_t allocator_storage[sizeof(stmlib::BufferAllocator)] __attribute__((aligned(8)));
    plaits::Voice::Frame frames[plaits::kBlockSize];
    uint8_t arena[16384] __attribute__((aligned(8)));
    uint32_t rng;
    int32_t pitch_q7;
    uint16_t harmonics, timbre, morph, decay, lpg_colour, aux_mix, level;
    uint8_t model, gate, retrigger_low_pending, fifo_pos, fifo_count;
};

#define HORATIO_ALIGN8(n) (((n) + 7u) & ~7u)
extern "C" {
__attribute__((used)) uint8_t horatio_probe_voice[sizeof(plaits::Voice)];
__attribute__((used)) uint8_t horatio_probe_voice_pool[4][HORATIO_ALIGN8(sizeof(plaits::Voice))];
__attribute__((used)) uint8_t horatio_probe_allocator[sizeof(stmlib::BufferAllocator)];
__attribute__((used)) uint8_t horatio_probe_frame[sizeof(plaits::Voice::Frame)];
__attribute__((used)) uint8_t horatio_probe_slot[sizeof(HoratioProbeSlot)];
}

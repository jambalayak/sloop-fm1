/* SPDX-License-Identifier: GPL-3.0-only
 * HORATIO-1 build-only layout probe. This object is compiled by
 * --check-plaits-cxx but is never linked into the installable firmware. */
#include <stdint.h>
#include "plaits/dsp/voice.h"
#include "stmlib/utils/buffer_allocator.h"

struct HoratioProbeState {
    plaits::Voice voice;
    stmlib::BufferAllocator allocator;
    plaits::Voice::Frame frames[24];
    uint8_t arena[16384];
};

extern "C" {
__attribute__((used)) uint8_t horatio_probe_voice[sizeof(plaits::Voice)];
__attribute__((used)) uint8_t horatio_probe_allocator[sizeof(stmlib::BufferAllocator)];
__attribute__((used)) uint8_t horatio_probe_frame[sizeof(plaits::Voice::Frame)];
__attribute__((used)) uint8_t horatio_probe_state[sizeof(HoratioProbeState)];
}

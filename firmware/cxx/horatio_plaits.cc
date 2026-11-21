/* SPDX-License-Identifier: GPL-3.0-only
 * HORATIO-1 fixed-memory adapter for all 24 Plaits 1.2 models.
 * Plaits/stmlib DSP remains MIT-licensed; retain upstream notices. */
#include <new>
#include <stdint.h>
#include <string.h>

#include "horatio_plaits.h"
#include "plaits/dsp/voice.h"
#include "stmlib/utils/random.h"

namespace {

static const uint32_t kArenaBytes = 6144u;
static const uint32_t kVoiceCount = HORATIO_PLAITS_VOICES;
static const uint32_t kFrameCount = plaits::kBlockSize;

struct PlaitsSlot {
    uint8_t allocator_storage[sizeof(stmlib::BufferAllocator)] __attribute__((aligned(8)));
    plaits::Voice::Frame frames[kFrameCount];
    uint8_t arena[kArenaBytes] __attribute__((aligned(8)));
    uint32_t rng;
    int32_t pitch_q7;
    uint16_t harmonics;
    uint16_t timbre;
    uint16_t morph;
    uint16_t decay;
    uint16_t lpg_colour;
    uint16_t aux_mix;
    uint16_t level;
    uint8_t model;
    uint8_t gate;
    uint8_t retrigger_low_pending;
    uint8_t fifo_pos;
    uint8_t fifo_count;
};

#define HORATIO_ALIGN8(n) (((n) + 7u) & ~7u)
static uint8_t voice_storage[kVoiceCount][HORATIO_ALIGN8(sizeof(plaits::Voice))]
    __attribute__((section(".pool"), aligned(8)));
static PlaitsSlot slots[kVoiceCount] __attribute__((section(".pool"), aligned(8)));
static plaits::Voice *voices[kVoiceCount];
static stmlib::BufferAllocator *allocators[kVoiceCount];
static uint8_t initialized;

static float unit_q15(uint16_t value) {
    /* Q15 is signed 1.15: 32767 represents 1 - 2^-15, not 1.0.
     * Keeping the endpoint below 1.0 also avoids an upstream Plaits DX-FM
     * interpolation over-read at exactly velocity 1.0. */
    return static_cast<float>(value) * (1.0f / 32768.0f);
}

static void render_chunk(uint32_t index, uint32_t trigger) {
    PlaitsSlot *s = &slots[index];
    plaits::Patch patch = {};
    patch.note = static_cast<float>(s->pitch_q7) * (1.0f / 128.0f);
    patch.harmonics = unit_q15(s->harmonics);
    patch.timbre = unit_q15(s->timbre);
    patch.morph = unit_q15(s->morph);
    patch.frequency_modulation_amount = 0.0f;
    patch.timbre_modulation_amount = 0.0f;
    patch.morph_modulation_amount = 0.0f;
    patch.engine = s->model;
    patch.decay = unit_q15(s->decay);
    patch.lpg_colour = unit_q15(s->lpg_colour);

    plaits::Modulations mod = {};
    mod.engine = 0.0f;
    mod.note = 0.0f;
    mod.frequency = 0.0f;
    mod.harmonics = 0.0f;
    mod.timbre = 0.0f;
    mod.morph = 0.0f;
    mod.trigger = trigger ? 1.0f : 0.0f;
    mod.level = unit_q15(s->level);
    mod.frequency_patched = false;
    mod.timbre_patched = false;
    mod.morph_patched = false;
    mod.trigger_patched = true;
    mod.level_patched = true;

    const uint32_t previous_rng = stmlib::Random::state();
    stmlib::Random::Seed(s->rng);
    voices[index]->Render(patch, mod, s->frames, kFrameCount);
    s->rng = stmlib::Random::state();
    stmlib::Random::Seed(previous_rng);
}

static int16_t mix_frame(const plaits::Voice::Frame &frame, uint16_t aux_mix) {
    const int32_t dry = 32767 - aux_mix;
    const int32_t mixed =
        (static_cast<int32_t>(frame.out) * dry +
         static_cast<int32_t>(frame.aux) * aux_mix) / 32767;
    return static_cast<int16_t>(mixed);
}

}  // namespace

extern "C" {

void horatio_plaits_init(void) {
    if (initialized) return;
    memset(slots, 0, sizeof(slots));
    memset(voice_storage, 0, sizeof(voice_storage));
    for (uint32_t i = 0; i < kVoiceCount; ++i) {
        PlaitsSlot *s = &slots[i];
        allocators[i] = new (s->allocator_storage)
            stmlib::BufferAllocator(s->arena, sizeof(s->arena));
        voices[i] = new (voice_storage[i]) plaits::Voice();
        voices[i]->Init(allocators[i]);
        s->rng = 0x6d2b79f5u ^ (i * 0x9e3779b9u);
        s->model = 0xffu;
        s->decay = 16384u;
        s->lpg_colour = 16384u;
        s->level = 32767u;
    }
    initialized = 1u;
}

void horatio_plaits_configure(uint32_t slot, uint32_t model, int32_t pitch_q7,
                              uint16_t harmonics, uint16_t timbre, uint16_t morph,
                              uint16_t decay, uint16_t lpg_colour, uint16_t aux_mix,
                              uint16_t level) {
    if (!initialized || slot >= kVoiceCount) return;
    PlaitsSlot *s = &slots[slot];
    if (model >= HORATIO_PLAITS_MODELS) model = HORATIO_PLAITS_MODELS - 1u;
    if (harmonics > 32767u) harmonics = 32767u;
    if (timbre > 32767u) timbre = 32767u;
    if (morph > 32767u) morph = 32767u;
    if (aux_mix > 32767u) aux_mix = 32767u;
    if (level > 32767u) level = 32767u;
    if (s->model != model && s->gate) {
        s->retrigger_low_pending = 1u;
        s->fifo_count = 0u;
        s->fifo_pos = 0u;
    }
    s->model = static_cast<uint8_t>(model);
    s->pitch_q7 = pitch_q7;
    s->harmonics = harmonics;
    s->timbre = timbre;
    s->morph = morph;
    s->decay = decay;
    s->lpg_colour = lpg_colour;
    s->aux_mix = aux_mix;
    s->level = level;
}

void horatio_plaits_note_on(uint32_t slot) {
    if (!initialized || slot >= kVoiceCount) return;
    PlaitsSlot *s = &slots[slot];
    s->gate = 1u;
    s->retrigger_low_pending = 1u;
    s->fifo_count = 0u;
    s->fifo_pos = 0u;
}

void horatio_plaits_gate(uint32_t slot, uint32_t gate) {
    if (!initialized || slot >= kVoiceCount) return;
    slots[slot].gate = gate != 0u;
}

void horatio_plaits_all_off(uint32_t slot) {
    if (!initialized || slot >= kVoiceCount) return;
    PlaitsSlot *s = &slots[slot];
    s->gate = 0u;
    s->retrigger_low_pending = 0u;
    s->fifo_count = 0u;
    s->fifo_pos = 0u;
}

void horatio_plaits_render(uint32_t slot, int16_t *out, uint32_t n) {
    if (!out) return;
    if (!initialized || slot >= kVoiceCount) {
        for (uint32_t i = 0; i < n; ++i) out[i] = 0;
        return;
    }
    PlaitsSlot *s = &slots[slot];
    for (uint32_t i = 0; i < n; ++i) {
        if (!s->fifo_count) {
            const uint32_t trigger = s->retrigger_low_pending ? 0u : s->gate;
            render_chunk(slot, trigger);
            if (s->retrigger_low_pending) s->retrigger_low_pending = 0u;
            s->fifo_pos = 0u;
            s->fifo_count = static_cast<uint8_t>(kFrameCount);
        }
        out[i] = mix_frame(s->frames[s->fifo_pos], s->aux_mix);
        ++s->fifo_pos;
        --s->fifo_count;
    }
}

}  // extern "C"

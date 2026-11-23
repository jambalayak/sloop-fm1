/* SPDX-License-Identifier: GPL-3.0-only
 * HORATIO-1 compact granular processor.
 *
 * The grain and delay-line DSP classes are used from Mutable Instruments
 * Beads, Copyright 2017 Emilie Gillet, under the MIT license. Their original
 * copyright and permission notices remain in the upstream headers:
 * beads/dsp/delay_grain.h, grain.h, grain_envelope.h,
 * voice_stealing_envelope.h and delay_line.h.
 *
 * HORATIO uses an 8192-sample mono recording line, captured at selectable
 * 16, 8 or 4 kHz from the host stream, and eight grains. This is a compact
 * adaptation, not a port of the complete Beads processor.
 */
#include "horatio_beads_granular.h"

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "horatio_delay_grain.h"

namespace {

static const int kMaxDelay = HORATIO_GRANULAR_BUFFER_SAMPLES;
static const int kSafety = beads::DelayGrain::head_crossing_safety_zone + 1;
static const size_t kBankBytes = (kMaxDelay + 4u) * sizeof(int16_t);
static const size_t kAllocatedBytes = HORATIO_GRANULAR_BUFFER_BYTES;
static uint32_t capture_rate = HORATIO_GRANULAR_RATE_16K;
static float capture_filter_coefficient = 0.65f;

typedef beads::DelayLine<beads::RESOLUTION_16_BIT, kMaxDelay, 1,
                         kBankBytes, 1> CompactDelayLine;

static uint8_t recording_buffer[kAllocatedBytes] __attribute__((aligned(8)));
static CompactDelayLine delay_line;
static beads::DelayGrain grains[HORATIO_GRANULAR_MAX_GRAINS];
static float grain_speed[HORATIO_GRANULAR_MAX_GRAINS];
static uint32_t sample_rate = 48000u;
static uint32_t recorded_samples;
static float capture_l;
static float capture_r;
static uint32_t capture_phase;
static uint32_t next_grain;
static float trigger_phase;
static float density_hz;
static float time_position;
static float grain_size;
static float pitch_semitones;
static float grain_shape;
static float feedback;
static float previous_wet_l;
static float previous_wet_r;
static float feedback_dc_l;
static float feedback_dc_r;
static float feedback_peak;
static float feedback_peak_lp;
static float feedback_decay;
static uint8_t frozen;
static uint8_t reverse;
static uint8_t initialized;

static float clamp_unit(float value) {
    if (value < 0.0f) return 0.0f;
    if (value > 1.0f) return 1.0f;
    return value;
}

static float clamp_float(float value, float low, float high) {
    if (value < low) return low;
    if (value > high) return high;
    return value;
}

static int32_t to_q15(float value) {
    value = clamp_float(value, -1.0f, 0.999969482421875f);
    return static_cast<int32_t>(value * 32768.0f);
}

static void start_grain(void) {
    if (recorded_samples <= static_cast<uint32_t>(kSafety + 32)) return;

    uint32_t active = 0;
    uint32_t slot = HORATIO_GRANULAR_MAX_GRAINS;
    for (uint32_t offset = 0; offset < HORATIO_GRANULAR_MAX_GRAINS; ++offset) {
        const uint32_t candidate = (next_grain + offset) % HORATIO_GRANULAR_MAX_GRAINS;
        if (grains[candidate].done()) {
            if (slot == HORATIO_GRANULAR_MAX_GRAINS) slot = candidate;
        } else {
            ++active;
        }
    }
    if (slot == HORATIO_GRANULAR_MAX_GRAINS) return;

    beads::DelayGrain &grain = grains[slot];

    const uint32_t usable = recorded_samples > static_cast<uint32_t>(kSafety)
        ? recorded_samples - static_cast<uint32_t>(kSafety) : 0u;
    const float delay = static_cast<float>(kSafety) +
        time_position * static_cast<float>(usable);
    const float duration_seconds = 0.012f + 0.065f * grain_size;
    const float duration = duration_seconds * static_cast<float>(sample_rate);
    /* Grain speed is relative to the recording head. The read-head renderer
     * advances the write head by capture_rate/sample_rate each host frame. */
    const float write_speed = static_cast<float>(capture_rate) /
                              static_cast<float>(sample_rate);
    float speed = write_speed *
                  powf(2.0f, pitch_semitones * (1.0f / 12.0f));
    if (reverse) speed = -speed;

    const float amplitude = 1.0f / sqrtf(static_cast<float>(active + 1u));
    const float pan = static_cast<float>((slot * 5u) & 7u) / 7.0f;
    grain.Start(&delay_line, delay, speed, duration, grain_shape,
                amplitude, pan, 0.0f, write_speed);
    grain_speed[slot] = speed;

    next_grain = (slot + 1u) % HORATIO_GRANULAR_MAX_GRAINS;
}

}  // namespace

extern "C" {

void horatio_granular_init(uint32_t requested_sample_rate) {
    if (requested_sample_rate < 8000u || requested_sample_rate > 96000u)
        requested_sample_rate = 48000u;

    sample_rate = requested_sample_rate;
    capture_rate = HORATIO_GRANULAR_RATE_16K;
    capture_filter_coefficient = 0.65f;
    memset(recording_buffer, 0, sizeof(recording_buffer));
    delay_line.Init(recording_buffer, sizeof(recording_buffer));

    for (uint32_t i = 0; i < HORATIO_GRANULAR_MAX_GRAINS; ++i) {
        grains[i].Init(static_cast<float>(sample_rate));
        grain_speed[i] = 0.0f;
    }

    recorded_samples = 0u;
    capture_l = capture_r = 0.0f;
    capture_phase = 0u;
    next_grain = 0u;
    trigger_phase = 0.0f;
    density_hz = 0.0f;
    time_position = 0.5f;
    grain_size = 0.5f;
    pitch_semitones = 0.0f;
    grain_shape = 0.5f;
    feedback = 0.0f;
    previous_wet_l = previous_wet_r = 0.0f;
    feedback_dc_l = feedback_dc_r = 0.0f;
    feedback_peak = feedback_peak_lp = 0.5f;
    feedback_decay = 1.0f / static_cast<float>(sample_rate);
    frozen = reverse = 0u;
    initialized = 1u;
}

void horatio_granular_set(float density, float time, float size,
                          float pitch, float shape, float fb,
                          uint32_t freeze, uint32_t rev,
                          uint32_t requested_capture_rate) {
    if (!initialized) horatio_granular_init(48000u);
    if (requested_capture_rate != HORATIO_GRANULAR_RATE_16K &&
        requested_capture_rate != HORATIO_GRANULAR_RATE_8K &&
        requested_capture_rate != HORATIO_GRANULAR_RATE_4K)
        requested_capture_rate = HORATIO_GRANULAR_RATE_16K;
    if (requested_capture_rate != capture_rate) {
        /* A rate change changes the meaning of every stored delay in the ring.
         * Clear it rather than mixing samples recorded at different rates. */
        capture_rate = requested_capture_rate;
        capture_filter_coefficient = 0.65f *
            (static_cast<float>(capture_rate) / 16000.0f);
        memset(recording_buffer, 0, sizeof(recording_buffer));
        delay_line.Init(recording_buffer, sizeof(recording_buffer));
        for (uint32_t i = 0; i < HORATIO_GRANULAR_MAX_GRAINS; ++i) {
            grains[i].Init(static_cast<float>(sample_rate));
            grain_speed[i] = 0.0f;
        }
        recorded_samples = 0u;
        capture_l = capture_r = 0.0f;
        capture_phase = 0u;
        trigger_phase = 0.0f;
        previous_wet_l = previous_wet_r = 0.0f;
        feedback_dc_l = feedback_dc_r = 0.0f;
    }
    density_hz = clamp_float(density, 0.0f, 60.0f);
    time_position = clamp_unit(time);
    grain_size = clamp_unit(size);
    pitch_semitones = clamp_float(pitch, -24.0f, 24.0f);
    grain_shape = clamp_unit(shape);
    feedback = clamp_unit(fb);
    frozen = freeze != 0u;
    reverse = rev != 0u;
}

uint32_t horatio_granular_recorded_samples(void) {
    return recorded_samples;
}

void horatio_granular_process(const int32_t *input_l, const int32_t *input_r,
                              int32_t *output_l, int32_t *output_r,
                              uint32_t samples) {
    if (!output_l || !output_r) return;
    if (!initialized) horatio_granular_init(48000u);

    /* Pitch is a block-rate control. Calculate its exponential ratio once
     * per audio block, not once per sample on the embedded CPU. */
    const float write_speed = static_cast<float>(capture_rate) /
                              static_cast<float>(sample_rate);
    float target_speed = write_speed *
                         powf(2.0f, pitch_semitones * (1.0f / 12.0f));
    if (reverse) target_speed = -target_speed;
    /* The host sample rate is constant for this block, so calculate the
     * 20 ms glide coefficient once rather than once per audio sample. */
    const float speed_slew = 1.0f - expf(
        -1.0f / (0.020f * static_cast<float>(sample_rate)));

    for (uint32_t i = 0; i < samples; ++i) {
        const float in_l = input_l ? static_cast<float>(input_l[i]) * (1.0f / 32768.0f) : 0.0f;
        const float in_r = input_r ? static_cast<float>(input_r[i]) * (1.0f / 32768.0f) : in_l;

        /* Beads' digital-resolution feedback path feeds the previous, DC-blocked
         * granular return back into the recording input with a squared control
         * law and 3x loop gain. Its input limiter keeps resonant settings stable. */
        const float feedback_gain = feedback * feedback * 3.35f;
        beads::FloatFrame input;
        input.l = in_l + previous_wet_l * feedback_gain;
        input.r = in_r + previous_wet_r * feedback_gain;
        {
            float peak = fabsf(input.l);
            float right_peak = fabsf(input.r);
            float gain;
            if (right_peak > peak) peak = right_peak;
            if (peak > 40.0f) peak = 40.0f;
            feedback_peak += (peak - feedback_peak) *
                (peak > feedback_peak ? 0.1f : feedback_decay);
            feedback_peak_lp += (feedback_peak - feedback_peak_lp) *
                (feedback_peak > feedback_peak_lp ? 0.5f : feedback_decay);
            gain = feedback_peak_lp > 0.95f ? 0.95f / feedback_peak_lp : 1.0f;
            input.l *= gain;
            input.r *= gain;
        }

        /* One-pole anti-alias filtering, then rational-rate 16 kHz capture.
         * The phase accumulator also works when the host runs at 44.1 kHz. */
        capture_l += capture_filter_coefficient * (input.l - capture_l);
        capture_r += capture_filter_coefficient * (input.r - capture_r);
        capture_phase += capture_rate;
        if (capture_phase >= sample_rate) {
            capture_phase -= sample_rate;
            input.l = capture_l;
            input.r = capture_r;
            delay_line.Write(input, frozen != 0u);
            if (!frozen && recorded_samples < static_cast<uint32_t>(kMaxDelay))
                ++recorded_samples;
        }

        trigger_phase += density_hz / static_cast<float>(sample_rate);
        if (trigger_phase >= 1.0f) {
            trigger_phase -= 1.0f;
            start_grain();
        }

        /* Keep the pitch glide at 20 ms on every supported host sample rate. */
        beads::FloatFrame wet;
        wet.l = 0.0f;
        wet.r = 0.0f;
        for (uint32_t g = 0; g < HORATIO_GRANULAR_MAX_GRAINS; ++g) {
            if (!grains[g].done()) {
                /* Exponential 20 ms slew, without restarting the grain envelope. */
                grain_speed[g] += (target_speed - grain_speed[g]) * speed_slew;
                grains[g].SetSpeed(grain_speed[g]);
            }
            grains[g].Render<beads::HEAD_CROSSING_POLICY_STOP, true>(
                &delay_line, &wet, write_speed);
        }

        /* Beads stores a high-passed version of the wet return for feedback,
         * preventing DC from accumulating in the recirculating loop. */
        feedback_dc_l += 0.005f * (wet.l - feedback_dc_l);
        feedback_dc_r += 0.005f * (wet.r - feedback_dc_r);
        previous_wet_l = wet.l - feedback_dc_l;
        previous_wet_r = wet.r - feedback_dc_r;
        output_l[i] = to_q15(wet.l);
        output_r[i] = to_q15(wet.r);
    }
}

}  // extern "C"

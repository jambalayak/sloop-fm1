#include "horatio_beads_granular.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>


static uint32_t captured_after_one_second(uint32_t rate) {
    const uint32_t sr = 48000u;
    int32_t in_l[128], in_r[128], out_l[128], out_r[128];
    horatio_granular_init(sr);
    horatio_granular_set(0.0f, 0.5f, 0.55f, 0.0f, 0.65f, 0.0f,
                         0u, 0u, rate);
    for (uint32_t block = 0; block < 375u; ++block) {
        for (uint32_t i = 0; i < 128u; ++i) in_l[i] = in_r[i] = 0;
        horatio_granular_process(in_l, in_r, out_l, out_r, 128u);
    }
    return horatio_granular_recorded_samples();
}

static uint32_t captured_after_one_second_at_host_rate(uint32_t rate,
                                                               uint32_t host_rate) {
    int32_t in_l[128], in_r[128], out_l[128], out_r[128];
    uint32_t remaining = host_rate;
    horatio_granular_init(host_rate);
    horatio_granular_set(0.0f, 0.5f, 0.55f, 0.0f, 0.65f, 0.0f,
                         0u, 0u, rate);
    while (remaining != 0u) {
        const uint32_t count = remaining > 128u ? 128u : remaining;
        for (uint32_t i = 0; i < count; ++i) in_l[i] = in_r[i] = 0;
        horatio_granular_process(in_l, in_r, out_l, out_r, count);
        remaining -= count;
    }
    return horatio_granular_recorded_samples();
}

static void render_position_variant(float density, float time, float size,
                                    float pitch, float shape, float feedback,
                                    uint32_t freeze, uint32_t reverse,
                                    int32_t *left, int32_t *right,
                                    uint32_t samples) {
    const uint32_t sr = 48000u;
    int32_t in_l[128], in_r[128], out_l[128], out_r[128];
    uint32_t noise = 0x13579bdu;

    horatio_granular_init(sr);
    horatio_granular_set(0.0f, 0.5f, 0.55f, 0.0f, 0.65f, 0.0f, 0u, 0u, HORATIO_GRANULAR_RATE_16K);

    /* Fill the complete capture line with repeatable, non-periodic material. */
    for (uint32_t block = 0; block < 200u; ++block) {
        for (uint32_t i = 0; i < 128u; ++i) {
            noise = noise * 1664525u + 1013904223u;
            const int32_t x = static_cast<int32_t>((noise >> 16) & 0x7fffu) - 16384;
            in_l[i] = in_r[i] = x;
        }
        horatio_granular_process(in_l, in_r, out_l, out_r, 128u);
    }

    /* Freeze the full buffer so each comparison reads the same recorded audio. */
    horatio_granular_set(density, time, size, pitch, shape, feedback, freeze, reverse, HORATIO_GRANULAR_RATE_16K);
    for (uint32_t offset = 0; offset < samples; offset += 128u) {
        for (uint32_t i = 0; i < 128u; ++i) in_l[i] = in_r[i] = 0;
        horatio_granular_process(in_l, in_r, out_l, out_r, 128u);
        for (uint32_t i = 0; i < 128u && offset + i < samples; ++i) {
            left[offset + i] = out_l[i];
            right[offset + i] = out_r[i];
        }
    }
}

static uint64_t compare_audio(const int32_t *a_l, const int32_t *a_r,
                              const int32_t *b_l, const int32_t *b_r,
                              uint32_t samples) {
    uint64_t difference = 0u;
    for (uint32_t i = 0; i < samples; ++i) {
        difference += static_cast<uint64_t>(abs(a_l[i] - b_l[i]));
        difference += static_cast<uint64_t>(abs(a_r[i] - b_r[i]));
    }
    return difference;
}

int main() {
    const uint32_t sr = 48000u;
    int32_t in_l[128], in_r[128], out_l[128], out_r[128];
    uint32_t audible = 0u;
    uint32_t frozen_audible = 0u;
    int32_t peak = 0;

    if (HORATIO_GRANULAR_MAX_GRAINS != 8u ||
        HORATIO_GRANULAR_BUFFER_SAMPLES != 8192u ||
        HORATIO_GRANULAR_BUFFER_BYTES > 20u * 1024u) {
        fprintf(stderr, "compact Beads memory budget changed unexpectedly\n");
        return 1;
    }

    {
        const uint32_t samples_16k = captured_after_one_second(HORATIO_GRANULAR_RATE_16K);
        const uint32_t samples_8k = captured_after_one_second(HORATIO_GRANULAR_RATE_8K);
        const uint32_t samples_4k = captured_after_one_second(HORATIO_GRANULAR_RATE_4K);
        if (samples_16k != HORATIO_GRANULAR_BUFFER_SAMPLES ||
            samples_8k != 8000u || samples_4k != 4000u) {
            fprintf(stderr, "capture-rate duration mismatch: 16k=%u 8k=%u 4k=%u\n",
                    samples_16k, samples_8k, samples_4k);
            return 7;
        }
    }

    {
        const uint32_t samples_16k = captured_after_one_second_at_host_rate(
            HORATIO_GRANULAR_RATE_16K, 44100u);
        const uint32_t samples_8k = captured_after_one_second_at_host_rate(
            HORATIO_GRANULAR_RATE_8K, 44100u);
        const uint32_t samples_4k = captured_after_one_second_at_host_rate(
            HORATIO_GRANULAR_RATE_4K, 44100u);
        if (samples_16k != HORATIO_GRANULAR_BUFFER_SAMPLES ||
            samples_8k != 8000u || samples_4k != 4000u) {
            fprintf(stderr, "44.1 kHz host capture-rate mismatch: %u %u %u\\n",
                    samples_16k, samples_8k, samples_4k);
            return 8;
        }
    }

    {
        const uint32_t samples_16k = captured_after_one_second_at_host_rate(
            HORATIO_GRANULAR_RATE_16K, 32000u);
        const uint32_t samples_8k = captured_after_one_second_at_host_rate(
            HORATIO_GRANULAR_RATE_8K, 32000u);
        const uint32_t samples_4k = captured_after_one_second_at_host_rate(
            HORATIO_GRANULAR_RATE_4K, 32000u);
        if (samples_16k != HORATIO_GRANULAR_BUFFER_SAMPLES ||
            samples_8k != 8000u || samples_4k != 4000u) {
            fprintf(stderr, "32 kHz host capture-rate mismatch: %u %u %u\\n",
                    samples_16k, samples_8k, samples_4k);
            return 18;
        }
    }

    {
        const uint32_t samples_16k = captured_after_one_second_at_host_rate(
            HORATIO_GRANULAR_RATE_16K, 96000u);
        const uint32_t samples_8k = captured_after_one_second_at_host_rate(
            HORATIO_GRANULAR_RATE_8K, 96000u);
        const uint32_t samples_4k = captured_after_one_second_at_host_rate(
            HORATIO_GRANULAR_RATE_4K, 96000u);
        if (samples_16k != HORATIO_GRANULAR_BUFFER_SAMPLES ||
            samples_8k != 8000u || samples_4k != 4000u) {
            fprintf(stderr, "96 kHz host capture-rate mismatch: %u %u %u\\n",
                    samples_16k, samples_8k, samples_4k);
            return 17;
        }
    }

    {
        /* A rate switch must discard fractional capture phase as well as the
         * old recording. At 32 kHz, one 16 kHz input sample leaves half a
         * host-frame interval accumulated; switching to 4 kHz must restart
         * from phase zero, yielding exactly 16 samples in the next 128 frames. */
        int32_t in_l[128] = {0}, in_r[128] = {0};
        int32_t out_l[128], out_r[128];
        horatio_granular_init(32000u);
        horatio_granular_set(0.0f, 0.5f, 0.55f, 0.0f, 0.65f, 0.0f,
                             0u, 0u, HORATIO_GRANULAR_RATE_16K);
        horatio_granular_process(in_l, in_r, out_l, out_r, 1u);
        horatio_granular_set(0.0f, 0.5f, 0.55f, 0.0f, 0.65f, 0.0f,
                             0u, 0u, HORATIO_GRANULAR_RATE_4K);
        if (horatio_granular_recorded_samples() != 0u) {
            fprintf(stderr, "32 kHz rate switch did not clear recording\\n");
            return 19;
        }
        horatio_granular_process(in_l, in_r, out_l, out_r, 128u);
        if (horatio_granular_recorded_samples() != 16u) {
            fprintf(stderr, "32 kHz rate switch retained stale capture phase: %u\\n",
                    horatio_granular_recorded_samples());
            return 20;
        }
    }

    {
        int32_t zeros[128] = {0}, out_l[128], out_r[128];
        horatio_granular_init(48000u);
        horatio_granular_set(0.0f, 0.5f, 0.55f, 0.0f, 0.65f, 0.0f,
                             0u, 0u, HORATIO_GRANULAR_RATE_16K);
        for (uint32_t block = 0; block < 192u; ++block)
            horatio_granular_process(zeros, zeros, out_l, out_r, 128u);
        if (horatio_granular_recorded_samples() != 8192u) return 9;
        horatio_granular_set(0.0f, 0.5f, 0.55f, 0.0f, 0.65f, 0.0f,
                             0u, 0u, HORATIO_GRANULAR_RATE_4K);
        if (horatio_granular_recorded_samples() != 0u) {
            fprintf(stderr, "changing capture rate did not clear old recording\\n");
            return 10;
        }
        horatio_granular_process(zeros, zeros, out_l, out_r, 128u);
        if (horatio_granular_recorded_samples() != 10u) {
            fprintf(stderr, "4 kHz capture did not resume after rate change\\n");
            return 11;
        }
    }

    {
        /* FREEZE must hold the current recording position while audio keeps
         * arriving, then resume capture cleanly when released. Start with a
         * partial buffer so resumed capture can be measured exactly. */
        int32_t input_l[128], input_r[128], out_l[128], out_r[128];
        for (uint32_t i = 0; i < 128u; ++i) input_l[i] = input_r[i] = 12000;
        horatio_granular_init(48000u);
        horatio_granular_set(0.0f, 0.5f, 0.55f, 0.0f, 0.65f, 0.0f,
                             0u, 0u, HORATIO_GRANULAR_RATE_16K);
        for (uint32_t block = 0; block < 46u; ++block)
            horatio_granular_process(input_l, input_r, out_l, out_r, 128u);
        const uint32_t before_freeze = horatio_granular_recorded_samples();
        if (before_freeze != 1962u) {
            fprintf(stderr, "unexpected partial recording before freeze: %u\\n",
                    before_freeze);
            return 24;
        }
        horatio_granular_set(0.0f, 0.5f, 0.55f, 0.0f, 0.65f, 0.0f,
                             1u, 0u, HORATIO_GRANULAR_RATE_16K);
        for (uint32_t block = 0; block < 40u; ++block)
            horatio_granular_process(input_l, input_r, out_l, out_r, 128u);
        if (horatio_granular_recorded_samples() != before_freeze) {
            fprintf(stderr, "FREEZE changed recording length: %u (expected %u)\\n",
                    horatio_granular_recorded_samples(), before_freeze);
            return 25;
        }
        horatio_granular_set(0.0f, 0.5f, 0.55f, 0.0f, 0.65f, 0.0f,
                             0u, 0u, HORATIO_GRANULAR_RATE_16K);
        for (uint32_t block = 0; block < 3u; ++block)
            horatio_granular_process(input_l, input_r, out_l, out_r, 128u);
        if (horatio_granular_recorded_samples() != 2090u) {
            fprintf(stderr, "capture did not resume after FREEZE: %u\\n",
                    horatio_granular_recorded_samples());
            return 26;
        }
    }

    {
        /* Exercise the feedback limiter under sustained worst-case settings:
         * maximum density, pitch, grain size and feedback with a repeatable
         * high-level input. The recording line must remain bounded and the
         * wet return must stay audible without exceeding Q15 output range. */
        uint32_t noise = 0x2468ace1u;
        uint32_t stress_audible = 0u;
        int32_t stress_peak = 0;
        horatio_granular_init(48000u);
        horatio_granular_set(60.0f, 0.5f, 1.0f, 24.0f, 0.65f, 1.0f,
                             0u, 0u, HORATIO_GRANULAR_RATE_16K);
        for (uint32_t block = 0; block < 750u; ++block) {
            for (uint32_t i = 0; i < 128u; ++i) {
                noise = noise * 1664525u + 1013904223u;
                const int32_t x = static_cast<int32_t>((noise >> 16) & 0xffffu) - 32768;
                in_l[i] = in_r[i] = x;
            }
            horatio_granular_process(in_l, in_r, out_l, out_r, 128u);
            for (uint32_t i = 0; i < 128u; ++i) {
                const int32_t left = abs(out_l[i]);
                const int32_t right = abs(out_r[i]);
                if (left > 2 || right > 2) ++stress_audible;
                if (left > stress_peak) stress_peak = left;
                if (right > stress_peak) stress_peak = right;
                if (out_l[i] < -32768 || out_l[i] > 32767 ||
                    out_r[i] < -32768 || out_r[i] > 32767) {
                    fprintf(stderr, "maximum-feedback stress exceeded Q15 bounds\\n");
                    return 21;
                }
            }
        }
        if (horatio_granular_recorded_samples() != HORATIO_GRANULAR_BUFFER_SAMPLES) {
            fprintf(stderr, "maximum-feedback stress changed recording capacity: %u\\n",
                    horatio_granular_recorded_samples());
            return 22;
        }
        if (stress_audible < 1000u || stress_peak < 100) {
            fprintf(stderr, "maximum-feedback stress became silent: audible=%u peak=%d\\n",
                    stress_audible, stress_peak);
            return 23;
        }
    }

    horatio_granular_init(sr);
    horatio_granular_set(30.0f, 0.45f, 0.55f, 0.0f, 0.65f, 0.15f, 0u, 0u, HORATIO_GRANULAR_RATE_16K);

    uint32_t phase = 0u;
    for (uint32_t block = 0; block < 120u; ++block) {
        for (uint32_t i = 0; i < 128u; ++i, ++phase) {
            const float x = 0.35f * sinf(6.28318530718f * 440.0f *
                                         static_cast<float>(phase) / sr);
            in_l[i] = in_r[i] = static_cast<int32_t>(x * 32768.0f);
        }
        horatio_granular_process(in_l, in_r, out_l, out_r, 128u);
        for (uint32_t i = 0; i < 128u; ++i) {
            const int32_t a = abs(out_l[i]);
            const int32_t b = abs(out_r[i]);
            if (a > 2 || b > 2) ++audible;
            if (a > peak) peak = a;
            if (b > peak) peak = b;
        }
    }

    horatio_granular_set(30.0f, 0.45f, 0.55f, 7.0f, 0.7f, 0.1f, 1u, 0u, HORATIO_GRANULAR_RATE_16K);
    for (uint32_t block = 0; block < 16u; ++block) {
        for (uint32_t i = 0; i < 128u; ++i) in_l[i] = in_r[i] = 0;
        horatio_granular_process(in_l, in_r, out_l, out_r, 128u);
        for (uint32_t i = 0; i < 128u; ++i)
            if (abs(out_l[i]) > 2 || abs(out_r[i]) > 2) ++frozen_audible;
    }

    if (audible < 100u) {
        fprintf(stderr, "granular render too quiet: %u audible samples\n", audible);
        return 2;
    }
    if (frozen_audible < 100u) {
        fprintf(stderr, "freeze did not sustain granular playback: %u samples\n", frozen_audible);
        return 3;
    }
    if (peak > 32767) {
        fprintf(stderr, "output exceeded Q15 bounds: %d\n", peak);
        return 4;
    }


    {
        const uint32_t compare_samples = 4096u;
        int32_t base_l[4096], base_r[4096];
        int32_t moved_l[4096], moved_r[4096];
        int32_t reverse_l[4096], reverse_r[4096];
        int32_t pitched_l[4096], pitched_r[4096];
        uint64_t time_difference, reverse_difference, pitch_difference;

        render_position_variant(45.0f, 0.08f, 0.55f, 0.0f, 0.65f, 0.0f, 1u, 0u, base_l, base_r, compare_samples);
        render_position_variant(45.0f, 0.92f, 0.55f, 0.0f, 0.65f, 0.0f, 1u, 0u, moved_l, moved_r, compare_samples);
        render_position_variant(45.0f, 0.08f, 0.55f, 0.0f, 0.65f, 0.0f, 1u, 1u, reverse_l, reverse_r, compare_samples);
        render_position_variant(45.0f, 0.08f, 0.55f, 12.0f, 0.65f, 0.0f, 1u, 0u, pitched_l, pitched_r, compare_samples);
        time_difference = compare_audio(base_l, base_r, moved_l, moved_r, compare_samples);
        reverse_difference = compare_audio(base_l, base_r, reverse_l, reverse_r, compare_samples);
        pitch_difference = compare_audio(base_l, base_r, pitched_l, pitched_r, compare_samples);
        if (time_difference < 100000u) {
            fprintf(stderr, "TIME control did not materially change granular playback: %llu\\n",
                    static_cast<unsigned long long>(time_difference));
            return 5;
        }
        if (reverse_difference < 100000u) {
            fprintf(stderr, "REVERSE control did not materially change granular playback: %llu\\n",
                    static_cast<unsigned long long>(reverse_difference));
            return 6;
        }
        if (pitch_difference < 100000u) {
            fprintf(stderr, "PITCH control did not materially change granular playback: %llu\\n",
                    static_cast<unsigned long long>(pitch_difference));
            return 12;
        }

        {
            int32_t variant_l[4096], variant_r[4096];
            uint64_t difference;
            render_position_variant(8.0f, 0.08f, 0.55f, 0.0f, 0.65f, 0.0f,
                                    1u, 0u, variant_l, variant_r, compare_samples);
            difference = compare_audio(base_l, base_r, variant_l, variant_r, compare_samples);
            if (difference < 100000u) {
                fprintf(stderr, "DENSITY control did not materially change granular playback: %llu\\n",
                        static_cast<unsigned long long>(difference));
                return 13;
            }
            render_position_variant(45.0f, 0.08f, 0.05f, 0.0f, 0.65f, 0.0f,
                                    1u, 0u, variant_l, variant_r, compare_samples);
            difference = compare_audio(base_l, base_r, variant_l, variant_r, compare_samples);
            if (difference < 100000u) {
                fprintf(stderr, "SIZE control did not materially change granular playback: %llu\\n",
                        static_cast<unsigned long long>(difference));
                return 14;
            }
            render_position_variant(45.0f, 0.08f, 0.55f, 0.0f, 0.0f, 0.0f,
                                    1u, 0u, variant_l, variant_r, compare_samples);
            difference = compare_audio(base_l, base_r, variant_l, variant_r, compare_samples);
            if (difference < 100000u) {
                fprintf(stderr, "SHAPE control did not materially change granular playback: %llu\\n",
                        static_cast<unsigned long long>(difference));
                return 15;
            }
            render_position_variant(45.0f, 0.08f, 0.55f, 0.0f, 0.65f, 1.0f,
                                    0u, 0u, variant_l, variant_r, compare_samples);
            difference = compare_audio(base_l, base_r, variant_l, variant_r, compare_samples);
            if (difference < 100000u) {
                fprintf(stderr, "FEEDBACK control did not materially change granular playback: %llu\\n",
                        static_cast<unsigned long long>(difference));
                return 16;
            }
        }
    }

    printf("HORATIO compact Beads granular smoke: PASS (audible=%u freeze=%u peak=%d, buffer=%u B, grains=%u)\n",
           audible, frozen_audible, peak, HORATIO_GRANULAR_BUFFER_BYTES,
           HORATIO_GRANULAR_MAX_GRAINS);
    return 0;
}

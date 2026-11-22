#include "horatio_beads_granular.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

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

    horatio_granular_init(sr);
    horatio_granular_set(30.0f, 0.45f, 0.55f, 0.0f, 0.65f, 0.15f, 0u, 0u);

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

    horatio_granular_set(30.0f, 0.45f, 0.55f, 7.0f, 0.7f, 0.1f, 1u, 0u);
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

    printf("HORATIO compact Beads granular smoke: PASS (audible=%u freeze=%u peak=%d, buffer=%u B, grains=%u)\n",
           audible, frozen_audible, peak, HORATIO_GRANULAR_BUFFER_BYTES,
           HORATIO_GRANULAR_MAX_GRAINS);
    return 0;
}

/* SPDX-License-Identifier: GPL-3.0-only
 * HORATIO-1 host-side Plaits audio smoke test.
 * Exercises the same fixed-memory adapter used by the FM-1 target. */
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include "horatio_plaits.h"

int main(void) {
    int16_t samples[1024];
    uint32_t model;
    uint32_t silent = 0;

    horatio_plaits_init();

    for (model = 0; model < HORATIO_PLAITS_MODELS; ++model) {
        uint32_t i;
        int32_t peak = 0;
        uint64_t energy = 0;

        /* Let the previous model's delayed trigger return fully low. */
        horatio_plaits_all_off(0);
        horatio_plaits_render(0, samples, 48);

        horatio_plaits_configure(0, model, 60 * 128,
                                 19660, 16384, 19660,
                                 24575, 16384, 0, 32767);
        horatio_plaits_note_on(0);
        horatio_plaits_render(0, samples, 1024);

        for (i = 0; i < 1024; ++i) {
            int32_t value = samples[i];
            int32_t magnitude = value < 0 ? -value : value;
            if (magnitude > peak) peak = magnitude;
            energy += (uint64_t)(magnitude * magnitude);
        }

        printf("Plaits model %u: peak=%ld energy=%llu\n",
               (unsigned)model, (long)peak,
               (unsigned long long)energy);
        if (peak < 2 || energy < 1024u) {
            fprintf(stderr, "Plaits model %u produced no meaningful audio\n",
                    (unsigned)model);
            ++silent;
        }
    }


    /* Exercise every real patch in all three bundled DX7 banks. HARMONICS
     * selects one of 32 patches in each six-op model. */
    for (model = 2; model <= 4; ++model) {
        uint32_t patch;
        for (patch = 0; patch < 32; ++patch) {
            uint32_t i;
            int32_t peak = 0;
            uint64_t energy = 0;
            uint32_t harmonics = (patch * 127u + 15u) / 31u;
            uint16_t harmonics_q15 =
                (uint16_t)((harmonics * 32767u + 63u) / 127u);

            horatio_plaits_all_off(0);
            horatio_plaits_render(0, samples, 48);
            horatio_plaits_configure(0, model, 60 * 128,
                                     harmonics_q15, 16384, 16384,
                                     24575, 16384, 0, 32767);
            horatio_plaits_note_on(0);
            horatio_plaits_render(0, samples, 1024);

            for (i = 0; i < 1024; ++i) {
                int32_t value = samples[i];
                int32_t magnitude = value < 0 ? -value : value;
                if (magnitude > peak) peak = magnitude;
                energy += (uint64_t)(magnitude * magnitude);
            }
            if (peak < 2 || energy < 1024u) {
                fprintf(stderr,
                        "DX bank model %u patch %u produced no meaningful audio (peak=%ld)\\n",
                        (unsigned)model, (unsigned)(patch + 1u), (long)peak);
                ++silent;
            }
        }
    }

    if (silent) {
        fprintf(stderr, "%u of %u Plaits models failed audio smoke test\n",
                (unsigned)silent, (unsigned)HORATIO_PLAITS_MODELS);
        return 1;
    }
    puts("HORATIO-1 Plaits audio smoke test passed: all 23 models and 96 DX factory patches rendered.");
    return 0;
}

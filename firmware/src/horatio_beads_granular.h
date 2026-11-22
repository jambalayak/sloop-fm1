/* SPDX-License-Identifier: GPL-3.0-only
 * HORATIO-1 compact granular processor C interface.
 * DSP grain and delay-line implementation is adapted from Mutable Instruments
 * Beads (MIT); see third_party/eurorack/beads/dsp headers for notices.
 */
#ifndef HORATIO_BEADS_GRANULAR_H
#define HORATIO_BEADS_GRANULAR_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define HORATIO_GRANULAR_MAX_GRAINS 8u
#define HORATIO_GRANULAR_BUFFER_SAMPLES 8192u
#define HORATIO_GRANULAR_BUFFER_BYTES 18440u
#define HORATIO_GRANULAR_RATE_16K 16000u
#define HORATIO_GRANULAR_RATE_8K 8000u
#define HORATIO_GRANULAR_RATE_4K 4000u

void horatio_granular_init(uint32_t sample_rate);
void horatio_granular_set(float density_hz, float time, float size,
                          float pitch_semitones, float shape, float feedback,
                          uint32_t freeze, uint32_t reverse,
                          uint32_t capture_rate_hz);
void horatio_granular_process(const int32_t *input_l, const int32_t *input_r,
                              int32_t *output_l, int32_t *output_r,
                              uint32_t samples);

#ifdef __cplusplus
}
#endif

#endif

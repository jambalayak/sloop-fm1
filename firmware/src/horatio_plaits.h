/* SPDX-License-Identifier: GPL-3.0-only
 * HORATIO-1 C interface to the pinned Mutable Instruments Plaits DSP. */
#ifndef HORATIO_PLAITS_H
#define HORATIO_PLAITS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define HORATIO_PLAITS_VOICES 4u
#define HORATIO_PLAITS_MODELS 16u

void horatio_plaits_init(void);
void horatio_plaits_configure(uint32_t slot, uint32_t model, int32_t pitch_q7,
                              uint16_t harmonics, uint16_t timbre, uint16_t morph,
                              uint16_t decay, uint16_t lpg_colour, uint16_t aux_mix, uint16_t level);
void horatio_plaits_note_on(uint32_t slot);
void horatio_plaits_gate(uint32_t slot, uint32_t gate);
void horatio_plaits_all_off(uint32_t slot);
void horatio_plaits_render(uint32_t slot, int16_t *out, uint32_t n);

#ifdef __cplusplus
}
#endif

#endif

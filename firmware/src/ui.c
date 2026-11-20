/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* Felucca user interface. Four columns map to KNOB 1..4. Rendering is lazy:
 * every element remembers what it last drew and is redrawn only on change. */
#ifndef FELUCCA_VERSION
#define FELUCCA_VERSION "HORATIO-1"  /* HORATIO-1 instrument firmware for the FM-1 (based on Felucca) */
#endif
static void project_save(uint32_t slot);
static void arrangement_save(void);
static void panel_setup(void);

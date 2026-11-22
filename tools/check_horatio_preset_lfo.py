#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Validate sparse preset-specific Plaits LFO profiles and their DSP destinations."""
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
core = (ROOT / "firmware/src/core.h").read_text()
ui = (ROOT / "firmware/src/ui.c").read_text()
engine = (ROOT / "firmware/src/eng_plaits.c").read_text()

if "int8_t lfo[11]" not in core or "#define LFO(" not in core:
    raise SystemExit("preset_t is missing its compact optional LFO profile")
if "if (pr->lfo[0])" not in ui or "t->p[P_LRATE] = pr->lfo[1]" not in ui:
    raise SystemExit("factory preset loading does not apply its LFO profile")
for dest, param in ((1, "P_LD_PIT"), (2, "P_LD_FLT"), (3, "P_LD_SHP"), (4, "P_LD_AMP")):
    if f"pr->lfo[7] == {dest}" not in ui or f"pr->lfo[9] == {dest}" not in ui:
        raise SystemExit(f"preset LFO destination {dest} is not supported")
if "m->cutoff >> 8" not in engine or "m->shape >> 8" not in engine:
    raise SystemExit("Plaits does not consume LFO filter/shape modulation")

profiles = re.findall(r'\bLFO\(([^)]*)\)', engine)
if len(profiles) < 15:
    raise SystemExit(f"Expected a curated set of at least 15 LFO-enabled presets, found {len(profiles)}")
for profile in profiles:
    values = [int(x.strip()) for x in profile.split(",")]
    if len(values) != 10:
        raise SystemExit(f"Malformed LFO profile: {profile}")
    rate, wave, fade, sync, trig, pol, d1, a1, d2, a2 = values
    if not (0 <= rate <= 127 and 0 <= wave <= 4 and 0 <= fade <= 127 and 0 <= sync <= 10):
        raise SystemExit(f"LFO timing/wave value out of range: {profile}")
    if trig not in (0, 1) or pol not in (0, 1):
        raise SystemExit(f"LFO trigger/polarity value out of range: {profile}")
    if d1 not in range(1, 5) or not -64 <= a1 <= 127:
        raise SystemExit(f"First LFO destination/depth invalid: {profile}")
    if d2 not in range(0, 5) or not -64 <= a2 <= 127:
        raise SystemExit(f"Second LFO destination/depth invalid: {profile}")
    if d1 == 4 and not 0 <= a1 <= 127 or d2 == 4 and not 0 <= a2 <= 127:
        raise SystemExit(f"Amplitude LFO depth must be nonnegative: {profile}")
    if d2 == d1 and d2:
        raise SystemExit(f"LFO destinations must be distinct: {profile}")

print(f"HORATIO preset LFO profiles OK: {len(profiles)} curated sounds; other presets remain LFO-off.")

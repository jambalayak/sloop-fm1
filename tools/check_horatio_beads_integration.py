#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Guard the HORATIO Beads UI/global-control to audio-engine integration."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
fx = (ROOT / "firmware/src/fx.c").read_text()

def require(label, condition):
    if not condition:
        raise SystemExit("FAIL: " + label)
    print("ok: " + label)

start = fx.find("#ifdef HORATIO_PLAITS", fx.find("fx_buses(acc[0].sc"))
end = fx.find("#endif", start)
if start < 0 or end < 0:
    raise SystemExit("FAIL: missing HORATIO Beads mix section")
beads = fx[start:end]

require("Beads runs only when enabled", "if (song.g[G_BD_ON])" in beads)
require("granular engine initializes once at the host sample rate",
        "horatio_granular_init(FS);" in beads and "static uint8_t beads_ready;" in beads)
for label, token in [
    ("Density", "song.g[G_BD_DENS]"),
    ("Time", "song.g[G_BD_TIME]"),
    ("Size", "song.g[G_BD_SIZE]"),
    ("Pitch", "song.g[G_BD_PITCH]"),
    ("Shape", "song.g[G_BD_SHAPE]"),
    ("Feedback", "song.g[G_BD_FB]"),
    ("Freeze", "song.g[G_BD_FREEZE]"),
    ("Reverse", "song.g[G_BD_REVERSE]"),
    ("capture rate", "song.g[G_BD_RATE]"),
]:
    require(label + " reaches the granular setter", token in beads)
require("the granular processor receives the live stereo mix",
        "horatio_granular_process(mix_l, mix_r, wet_l, wet_r, n);" in beads)
require("MIX is clamped to its seven-bit range", "clamp(song.g[G_BD_MIX], 0, 127)" in beads)
require("MIX uses equal-power dry and wet gains",
        "beads_sqrt((float)(127 - amount)" in beads and
        "beads_sqrt((float)amount" in beads)
require("both stereo outputs blend the granular return",
        "mix_l[i] = (int32_t)((float)mix_l[i] * dry_gain + (float)wet_l[i] * wet_mix_gain);" in beads and
        "mix_r[i] = (int32_t)((float)mix_r[i] * dry_gain + (float)wet_r[i] * wet_mix_gain);" in beads)

require("wet RMS compensation only measures non-silent dry and wet blocks",
        "if (dry_energy > 0.000001f && wet_energy > 0.000001f)" in beads)
require("wet RMS compensation is bounded below",
        "if (target_gain < 0.75f) target_gain = 0.75f;" in beads)
require("wet RMS compensation is bounded above",
        "if (target_gain > 1.80f) target_gain = 1.80f;" in beads)
require("wet RMS compensation is smoothed between blocks",
        "beads_wet_gain += (target_gain - beads_wet_gain) * 0.125f;" in beads)
require("dry and wet compensation cannot divide by a silent wet block",
        "wet_energy > 0.000001f" in beads and
        "target_gain = beads_sqrt(dry_energy / wet_energy);" in beads)

require("the Beads return remains before the master effects chain",
        fx.find("horatio_granular_process(mix_l") < fx.find("dust_process(mix_l, mix_r, n);", end))

print("HORATIO Beads integration contract passed.")

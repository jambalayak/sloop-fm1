#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Guard HORATIO per-preset octave metadata, pitch application, and LED feedback."""
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
core = (ROOT / "firmware/src/core.h").read_text()
engine = (ROOT / "firmware/src/eng_plaits.c").read_text()
ui = (ROOT / "firmware/src/ui.c").read_text()
voice = (ROOT / "firmware/src/voice.c").read_text()
leds = (ROOT / "firmware/src/ui_input.c").read_text()

if not re.search(r"int8_t\s+octave\s*;[^\n]*HORATIO", core):
    raise SystemExit("preset_t must store a signed per-preset octave offset")
if not re.search(r"int8_t\s+preset_octave\s*;", core):
    raise SystemExit("track_t must retain the selected preset octave")

offsets = [int(v) for v in re.findall(r"\.octave\s*=\s*(-?\d+)", engine)]
if not offsets:
    raise SystemExit("No factory preset has an explicit octave offset")
if any(v < -2 or v > 2 for v in offsets):
    raise SystemExit("Factory preset octave offsets must stay within -2..+2")
if not any(v < 0 for v in offsets) or not any(v > 0 for v in offsets):
    raise SystemExit("Factory presets must demonstrate both lower and higher registers")

if "t->preset_octave = (int8_t)clamp(e->presets[pi].octave, -2, 2);" not in ui:
    raise SystemExit("Loading a preset must copy and clamp its octave offset")

helper = re.search(
    r"static int32_t preset_pitch16\(const track_t \*t, uint32_t note\)\s*\{([^}]*)\}",
    voice,
    re.S,
)
if not helper or "t->preset_octave * 12" not in helper.group(1) or "* 16" not in helper.group(1):
    raise SystemExit("Preset octave must be converted to semitones in 1/16-semitone pitch units")
if voice.count("v->pitch16 = preset_pitch16(t, note);") != 2:
    raise SystemExit("Both fresh notes and legato pitch changes must apply the preset octave")

if "int octave = (int)song.octave + (int)TSEL->preset_octave;" not in leds:
    raise SystemExit("Octave LEDs must reflect manual plus preset octave")
if "led_put(nl, panel.btn[B_OCTDN], octave < 0);" not in leds:
    raise SystemExit("OCT DOWN LED must reflect a negative effective octave")
if "led_put(nl, panel.btn[B_OCTUP], octave > 0);" not in leds:
    raise SystemExit("OCT UP LED must reflect a positive effective octave")

print(f"HORATIO preset octave check OK: {len(offsets)} explicit offsets in range; note-on, legato, and LEDs covered.")

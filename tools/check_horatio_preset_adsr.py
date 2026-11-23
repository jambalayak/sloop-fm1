#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Guard Plaits factory ADSR data and its application when a preset is loaded."""
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
core = (ROOT / "firmware/src/core.h").read_text()
engine = (ROOT / "firmware/src/eng_plaits.c").read_text()
ui = (ROOT / "firmware/src/ui.c").read_text()

# The preset record stores the four envelope stages in this fixed order.
if not re.search(r"uint8_t\s+env\[4\]\s*;\s*/\*\s*ATK DEC SUS REL", core):
    raise SystemExit("preset_t must retain its four-byte ATK/DEC/SUS/REL envelope")

start = engine.find("static const preset_t PL_PRESETS[] = {")
end = engine.find("\n};", start)
if start < 0 or end < 0:
    raise SystemExit("Could not find the Plaits factory preset table")
block = engine[start:end]
rows = re.findall(
    r'\{"([^"]+)",\s*\{[^}]*\},\s*\{\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*\}',
    block,
)
if len(rows) != 190:
    raise SystemExit(f"Expected 190 Plaits factory presets after removing the unwanted orchestra preset, found {len(rows)}")
for name, *values in rows:
    adsr = [int(value) for value in values]
    if any(value < 0 or value > 127 for value in adsr):
        raise SystemExit(f"{name}: ADSR value outside 0..127")
    if adsr == [0, 0, 0, 0]:
        raise SystemExit(f"{name}: factory ADSR is unexpectedly all zero")

# Verify the actual factory-preset load path copies each stored stage into
# the corresponding track parameter, rather than only validating table data.
loader_start = ui.find("UI_SMALL static void apply_preset_to(")
loader_end = ui.find("\n}", loader_start)
if loader_start < 0 or loader_end < 0:
    raise SystemExit("Could not find apply_preset_to factory preset loader")
loader = ui[loader_start:loader_end]
expected = {
    "P_ATK": 0,
    "P_DEC": 1,
    "P_SUS": 2,
    "P_REL": 3,
}
for param, index in expected.items():
    assignment = rf"t->p\[{param}\]\s*=\s*e->presets\[pi\]\.env\[{index}\]\s*;"
    if not re.search(assignment, loader):
        raise SystemExit(f"Factory preset loading does not restore {param} from env[{index}]")

# The preset browser must call this same loader for the selected factory entry.
go_start = ui.find("UI_SMALL static int preset_go_to(")
go_end = ui.find("\n}", go_start)
if go_start < 0 or go_end < 0 or "apply_preset_to(t, k);" not in ui[go_start:go_end]:
    raise SystemExit("Factory preset browser no longer applies presets through apply_preset_to")

print(f"HORATIO Plaits ADSR load check OK: {len(rows)} factory presets retain ATK/DEC/SUS/REL through the browser load path.")

#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Check that the HORATIO-1 firmware preset table matches its curated bank."""
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "firmware" / "src" / "eng_plaits.c"
SPEC = ROOT / "HORATIO-1-FACTORY-PRESET-BANK.md"
REMOVED_IDS = {11, 14, 29, 33, 39}
EXPECTED_IDS = [i for i in range(1, 51) if i not in REMOVED_IDS]

source = SOURCE.read_text()
match = re.search(
    r"static const preset_t PL_PRESETS\[\]\s*=\s*\{(.*?)\n\};",
    source,
    re.S,
)
if not match:
    raise SystemExit("Could not find PL_PRESETS in firmware/src/eng_plaits.c")

records = re.findall(
    r'^\s*\{"([^"]+)",\s*\{\s*(\d+)',
    match.group(1),
    re.M,
)
firmware_names = [name for name, _ in records]
model_ids = [int(model) for _, model in records]

spec = SPEC.read_text()
spec_rows = []
for line in spec.splitlines():
    row = re.match(r"^\|\s*(\d{2})\s*\|\s*([^|]+?)\s*\|", line)
    if row:
        spec_rows.append((int(row.group(1)), row.group(2).strip()))

spec_ids = [number for number, _ in spec_rows]
spec_names = [name for _, name in spec_rows]

if spec_ids != EXPECTED_IDS:
    raise SystemExit(
        f"Preset IDs differ from the approved 45-entry set: {spec_ids}"
    )
if len(firmware_names) != 45:
    raise SystemExit(f"Expected 45 firmware presets, found {len(firmware_names)}")
if len(set(firmware_names)) != len(firmware_names):
    raise SystemExit("Duplicate firmware preset names found")
if firmware_names != spec_names:
    raise SystemExit("Firmware preset names/order do not match the factory-bank specification")
if any(model < 0 or model > 15 for model in model_ids):
    raise SystemExit("A preset refers to a model outside the 16 standard Plaits models")
if "Glass Bell" not in firmware_names or "Glass Wire" in firmware_names:
    raise SystemExit("The approved Glass Bell replacement is missing")
glass_bell_model = model_ids[firmware_names.index("Glass Bell")]
if glass_bell_model != 12:
    raise SystemExit(f"Glass Bell must use Plaits Modal (model 12), got {glass_bell_model}")

print("HORATIO-1 factory bank validated: 45 unique presets, stable original IDs, 16-model range")

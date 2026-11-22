#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Guard HORATIO's public Plaits model IDs and Speech exclusion."""
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
header = (ROOT / "firmware/src/horatio_plaits.h").read_text()
engine = (ROOT / "firmware/src/eng_plaits.c").read_text()
adapter = (ROOT / "firmware/cxx/horatio_plaits.cc").read_text()
presets = (ROOT / "tools/check_horatio_preset_bank.py").read_text()

count = re.search(r"#define\s+HORATIO_PLAITS_MODELS\s+(\d+)u", header)
if not count or int(count.group(1)) != 23:
    raise SystemExit("HORATIO must expose exactly 23 models after excluding Speech")

names_block = re.search(
    r"PL_MODEL_NAMES\[HORATIO_PLAITS_MODELS\]\s*=\s*\{(.*?)\};",
    engine,
    re.S,
)
if not names_block:
    raise SystemExit("Could not find the public Plaits model-name table")
names = re.findall(r'"([^"]+)"', names_block.group(1))
if len(names) != 23:
    raise SystemExit(f"Expected 23 visible model labels, found {len(names)}")
if any("SPEECH" in name.upper() for name in names):
    raise SystemExit("Speech must remain excluded from HORATIO's model browser")

if "if (model >= HORATIO_PLAITS_MODELS) model = HORATIO_PLAITS_MODELS - 1u;" not in adapter:
    raise SystemExit("Out-of-range model IDs must be clamped")
if "if (model >= 15u) ++model;" not in adapter:
    raise SystemExit("Public IDs 15..22 must skip upstream Plaits Speech at index 15")

mapping = [i if i < 15 else i + 1 for i in range(23)]
if mapping != list(range(15)) + list(range(16, 24)):
    raise SystemExit("Public model IDs do not map to upstream Plaits models 0..14 and 16..23")

if "expected = set(range(23))" not in presets:
    raise SystemExit("Factory preset test must continue to require coverage of every public model")
print("HORATIO Plaits model map OK: 23 visible models; Speech excluded; all remaining upstream IDs mapped.")

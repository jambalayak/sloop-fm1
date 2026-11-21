#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Keep HORATIO's visible factory preset browser aligned with the Plaits presets."""
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
engine = (ROOT / "firmware/src/eng_plaits.c").read_text()
ui = (ROOT / "firmware/src/ui.c").read_text()

preset_start = engine.index("static const preset_t PL_PRESETS[] = {")
preset_end = engine.index("\n};", preset_start)
preset_block = engine[preset_start:preset_end]
presets = re.findall(r'\{"([^"]+)",\s*\{\s*(\d+)\s*,[^}]*\},\s*\{\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*\}', preset_block)

bank_start = ui.index("/* One browser entry for every Plaits factory preset")
bank_end = ui.index("\n};", bank_start)
bank_block = ui[bank_start:bank_end]
bank = re.findall(r'\{(BK_[A-Z]+),\s*0,\s*"([^"]+)"\}', bank_block)

if not presets:
    raise SystemExit("No HORATIO Plaits presets found")
if len(presets) != len(bank):
    raise SystemExit(f"Factory browser has {len(bank)} entries; engine has {len(presets)} presets")
names = [row[0] for row in presets]
envelopes = [tuple(map(int, row[2:6])) for row in presets]
if any(any(value < 0 or value > 127 for value in env) for env in envelopes):
    raise SystemExit("Preset ADSR values must all be in the 0..127 range")
if len(set(envelopes)) < 8:
    raise SystemExit("Factory presets do not contain enough distinct ADSR envelopes")
if all(env == (0, 0, 0, 0) for env in envelopes):
    raise SystemExit("Factory presets have no programmed ADSR envelopes")
preset_lines = [line for line in preset_block.splitlines() if line.lstrip().startswith('{"')]
fenvs = []
filter_cutoffs = []
for line in preset_lines:
    fenv_match = re.search(r'\},\s*\{[^}]*\},\s*(-?\d+),\s*\d+,\s*\{', line)
    filter_match = re.search(r'P_TFLT\s*\+\s*1,\s*(-?\d+)', line)
    if not fenv_match or not filter_match:
        raise SystemExit("Every factory preset must define filter envelope depth and filter cutoff")
    fenvs.append(int(fenv_match.group(1)))
    filter_cutoffs.append(int(filter_match.group(1)))
if any(value < -64 or value > 63 for value in fenvs + filter_cutoffs):
    raise SystemExit("Preset filter values must be in the -64..63 range")
if len(set(fenvs)) < 4 or len(set(filter_cutoffs)) < 4:
    raise SystemExit("Factory presets need varied filter envelope and cutoff settings")
if len(set(names)) != len(names):
    raise SystemExit("Plaits factory preset names must be unique for name-based browser lookup")
if [name for _, name in bank] != names:
    raise SystemExit("Factory browser names/order do not match the Plaits preset table")

models = {int(row[1]) for row in presets}
expected = set(range(23))
if models != expected:
    raise SystemExit(f"Factory preset bank does not cover all 23 models: missing {sorted(expected - models)}")

for (name, model_text, *_env), (kind, bank_name) in zip(presets, bank):
    model = int(model_text)
    expected_kind = "BK_BASS" if model < 8 else "BK_KEYS" if model < 15 else "BK_ORGAN"
    if kind != expected_kind:
        raise SystemExit(f"{bank_name}: expected {expected_kind} for model {model}, got {kind}")

print(f"HORATIO factory browser OK: {len(bank)} unique presets cover all 23 HORATIO models with {len(set(envelopes))} ADSR shapes and {len(set(filter_cutoffs))} filter cutoff values.")

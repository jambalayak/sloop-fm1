#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Guard HORATIO's global PRESET-knob browsing behaviour."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
source = (ROOT / "firmware/src/ui_input.c").read_text()

notes = "if (on_notes_page() && (s = panel_enc(EN_PRESET)) != 0)"
browse = "if ((s = panel_enc(EN_PRESET)) != 0 && !on_notes_page())"

if source.count(notes) != 1:
    raise SystemExit("NOTES must read PRESET once for its zoom control")
if source.count(browse) != 1:
    raise SystemExit("PRESET browsing must have one unconditional non-NOTES handler")

notes_at = source.index(notes)
browse_at = source.index(browse)
if notes_at >= browse_at:
    raise SystemExit("NOTES zoom must be handled before normal preset browsing")

block = source[browse_at:source.index("if ((s = panel_enc(EN_ALGO))", browse_at)]
for required in ("preset_pos(&total)", "preset_go(", "Every detent loads one preset"):
    if required not in block:
        raise SystemExit(f"PRESET browsing handler is missing: {required}")

for forbidden in ("cur_page()->scope", "cur_page()->fam", "cur_fam()"):
    if forbidden in block:
        raise SystemExit(f"PRESET browsing is incorrectly restricted by {forbidden}")

print("HORATIO PRESET encoder OK: every non-NOTES page browses sounds; NOTES retains zoom.")

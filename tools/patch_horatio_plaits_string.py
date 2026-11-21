#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Apply HORATIO's bounded-memory variant of the upstream Plaits String engine.

The upstream engine uses three strings per voice and 1024-sample delay lines.
HORATIO keeps the model but uses one string and a 512-sample delay line so its
initialisation fits the FM-1's fixed 6 KiB per-voice allocator.
"""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1] / "third_party/eurorack/plaits/dsp"
CHANGES = {
    Path("engine/string_engine.h"): (
        "const int kNumStrings = 3;",
        "const int kNumStrings = 1;",
    ),
    Path("physical_modelling/string.h"): (
        "const size_t kDelayLineSize = 1024;",
        "const size_t kDelayLineSize = 512;",
    ),
}

for relative, (original, replacement) in CHANGES.items():
    path = ROOT / relative
    if not path.is_file():
        raise SystemExit(f"Missing expected Plaits source file: {path}")
    source = path.read_text()
    if replacement in source and original not in source:
        print(f"Already patched: {relative}")
        continue
    if source.count(original) != 1:
        raise SystemExit(
            f"Unexpected upstream source in {relative}: "
            f"expected exactly one {original!r}"
        )
    path.write_text(source.replace(original, replacement, 1))
    print(f"Applied HORATIO String memory patch: {relative}")

# Verify the resulting source, rather than silently accepting a partial patch.
for relative, (_, replacement) in CHANGES.items():
    if replacement not in (ROOT / relative).read_text():
        raise SystemExit(f"Patch verification failed: {relative}")
print("HORATIO lightweight String engine patch verified.")

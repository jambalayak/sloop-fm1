#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Apply the small, documented FM-1 sample-rate adaptation to the pinned Plaits tree."""
from pathlib import Path

root = Path(__file__).resolve().parents[1]
header = root / "third_party" / "eurorack" / "plaits" / "dsp" / "dsp.h"
if not header.exists():
    raise SystemExit("Plaits submodule is missing; initialise third_party/eurorack")

source = header.read_text()
include = '#include "horatio_rate.h"'
if include not in source:
    anchor = '#include "stmlib/stmlib.h"'
    if anchor not in source:
        raise SystemExit("Plaits dsp.h changed: cannot safely add HORATIO sample-rate include")
    source = source.replace(anchor, anchor + "\n\n" + include, 1)

source = source.replace("static const float kSampleRate = 48000.0f;",
                        "static const float kSampleRate = HORATIO_FS_HZ;")
source = source.replace("static const float kCorrectedSampleRate = 47872.34f;",
                        "static const float kCorrectedSampleRate = HORATIO_FS_HZ;")
if "kSampleRate = HORATIO_FS_HZ" not in source or "kCorrectedSampleRate = HORATIO_FS_HZ" not in source:
    raise SystemExit("Plaits dsp.h changed: expected sample-rate constants were not found")
header.write_text(source)
print("Plaits sample rate adapted to HORATIO-1: 44.1 kHz")

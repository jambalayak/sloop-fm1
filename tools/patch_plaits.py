#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Apply the small, documented FM-1 sample-rate adaptation to the pinned Plaits tree."""
from pathlib import Path


def patch_file(path, replacements, label):
    """Apply required source substitutions, accepting an already-patched file."""
    source = path.read_text()
    changed = False
    for old, new in replacements:
        if old in source:
            source = source.replace(old, new, 1)
            changed = True
        elif new not in source:
            raise SystemExit(f"{label} source changed: expected patch anchor was not found")
    if changed:
        path.write_text(source)

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

# HORATIO-1 exposes the complete 24-model Plaits 1.2 set: the eight
# orange-bank additions followed by the original green and red banks.
# The pinned upstream voice.cc already registers these in model order:
# VA VCF, phase distortion, three 6-op FM banks, wave terrain,
# string machine, chiptune, then the original 16 engines.
voice_h = root / "third_party" / "eurorack" / "plaits" / "dsp" / "voice.h"
voice_cc = root / "third_party" / "eurorack" / "plaits" / "dsp" / "voice.cc"
vh = voice_h.read_text()
vc = voice_cc.read_text()
required_h = [
    '#include "plaits/dsp/engine2/chiptune_engine.h"',
    '#include "plaits/dsp/engine2/phase_distortion_engine.h"',
    '#include "plaits/dsp/engine2/six_op_engine.h"',
    '#include "plaits/dsp/engine2/string_machine_engine.h"',
    '#include "plaits/dsp/engine2/virtual_analog_vcf_engine.h"',
    '#include "plaits/dsp/engine2/wave_terrain_engine.h"',
    "const int kMaxEngines = 24;",
]
required_cc = [
    "engines_.RegisterInstance(&virtual_analog_vcf_engine_",
    "engines_.RegisterInstance(&phase_distortion_engine_",
    "engines_.RegisterInstance(&six_op_engine_",
    "engines_.RegisterInstance(&wave_terrain_engine_",
    "engines_.RegisterInstance(&string_machine_engine_",
    "engines_.RegisterInstance(&chiptune_engine_",
]
if any(token not in vh for token in required_h) or any(token not in vc for token in required_cc):
    raise SystemExit("Pinned Plaits source does not contain the complete 24-model 1.2 engine set")
print("Plaits 1.2: all 24 models and eight orange-bank slots enabled")

# Keep the Particle engine's diffuser independent per voice while reducing
# its delay-line footprint for the FM-1's four-voice memory budget. All seven
# all-pass/delay lines remain; their lengths and modulation ranges are scaled
# to one quarter, preserving the diffusion topology with shorter decay times.
particle_cc = root / "third_party" / "eurorack" / "plaits" / "dsp" / "engine" / "particle_engine.cc"
diffuser_h = root / "third_party" / "eurorack" / "plaits" / "dsp" / "fx" / "diffuser.h"
patch_file(particle_cc, [
    ("  diffuser_.Init(allocator->Allocate<uint16_t>(8192));",
     "  diffuser_.Init(allocator->Allocate<uint16_t>(2048));"),
], "particle engine")
patch_file(diffuser_h, [
    ("E::Reserve<126,\n      E::Reserve<180,\n      E::Reserve<269,\n      E::Reserve<444,\n      E::Reserve<1653,\n      E::Reserve<2010,\n      E::Reserve<3411>",
     "E::Reserve<32,\n      E::Reserve<45,\n      E::Reserve<67,\n      E::Reserve<111,\n      E::Reserve<413,\n      E::Reserve<503,\n      E::Reserve<853>"),
    ("c.Interpolate(ap4, 400.0f, LFO_1, 43.0f, kap);",
     "c.Interpolate(ap4, 100.0f, LFO_1, 10.75f, kap);"),
    ("c.Interpolate(del, 3070.0f, LFO_1, 340.0f, rt);",
     "c.Interpolate(del, 767.5f, LFO_1, 85.0f, rt);"),
    ("typedef FxEngine<8192, FORMAT_12_BIT> E;",
     "typedef FxEngine<2048, FORMAT_12_BIT> E;"),
], "Particle diffuser")
print("Plaits Particle diffuser adapted to 2048 samples per voice (independent voices)")

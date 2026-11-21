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

# HORATIO-1 exposes the 16 original Plaits models. The six engine2 classes
# are newer additions and are deliberately excluded from this instrument.
def patch_file(path, replacements, label):
    text = path.read_text()
    for old, new in replacements:
        if old not in text:
            if not new or new in text:
                continue
            raise SystemExit(f"Plaits {label} changed: expected one patch anchor")
        if text.count(old) != 1:
            raise SystemExit(f"Plaits {label} changed: expected one patch anchor")
        text = text.replace(old, new, 1)
    path.write_text(text)


voice_h = root / "third_party" / "eurorack" / "plaits" / "dsp" / "voice.h"
voice_cc = root / "third_party" / "eurorack" / "plaits" / "dsp" / "voice.cc"
engine2_includes = """#include "plaits/dsp/engine2/chiptune_engine.h"
#include "plaits/dsp/engine2/phase_distortion_engine.h"
#include "plaits/dsp/engine2/six_op_engine.h"
#include "plaits/dsp/engine2/string_machine_engine.h"
#include "plaits/dsp/engine2/virtual_analog_vcf_engine.h"
#include "plaits/dsp/engine2/wave_terrain_engine.h"
"""
engine2_members = """  VirtualAnalogVCFEngine virtual_analog_vcf_engine_;
  PhaseDistortionEngine phase_distortion_engine_;
  SixOpEngine six_op_engine_;
  WaveTerrainEngine wave_terrain_engine_;
  StringMachineEngine string_machine_engine_;
  ChiptuneEngine chiptune_engine_;
"""
engine2_register = """  engines_.RegisterInstance(&virtual_analog_vcf_engine_, false, 1.0f, 1.0f);
  engines_.RegisterInstance(&phase_distortion_engine_, false, 0.7f, 0.7f);
  engines_.RegisterInstance(&six_op_engine_, true, 1.0f, 1.0f);
  engines_.RegisterInstance(&six_op_engine_, true, 1.0f, 1.0f);
  engines_.RegisterInstance(&six_op_engine_, true, 1.0f, 1.0f);
  engines_.RegisterInstance(&wave_terrain_engine_, false, 0.7f, 0.7f);
  engines_.RegisterInstance(&string_machine_engine_, false, 0.8f, 0.8f);
  engines_.RegisterInstance(&chiptune_engine_, false, 0.5f, 0.5f);
"""
chiptune_branch = """  } else if (engine_index == 7) {
    if (modulations.trigger_patched && !modulations.timbre_patched) {
      // Disable internal envelope on TIMBRE, and enable the envelope generator
      // built into the chiptune engine.
      internal_envelope_amplitude_timbre = 0.0f;
      chiptune_engine_.set_envelope_shape(patch.timbre_modulation_amount);
    } else {
      chiptune_engine_.set_envelope_shape(ChiptuneEngine::NO_ENVELOPE);
    }
"""
patch_file(voice_h, [
    (engine2_includes, ""),
    ("const int kMaxEngines = 24;", "const int kMaxEngines = 16;"),
    (engine2_members, ""),
], "voice.h")
patch_file(voice_cc, [
    (engine2_register, ""),
    ("if (engine_index == 15) {", "if (engine_index == 7) {"),
    (chiptune_branch, ""),
], "voice.cc")

vh = voice_h.read_text()
vc = voice_cc.read_text()
if ("engine2/" in vh or "kMaxEngines = 24" in vh or
        "kMaxEngines = 16" not in vh or "chiptune_engine_" in vh or
        "virtual_analog_vcf_engine_" in vh):
    raise SystemExit("Plaits voice.h pruning did not leave exactly the 16 original models")
if ("chiptune_engine_" in vc or "phase_distortion_engine_" in vc or
        "if (engine_index == 15)" in vc):
    raise SystemExit("Plaits voice.cc still contains an engine2 registration or branch")
print("Plaits model set reduced to the 16 original engines")

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

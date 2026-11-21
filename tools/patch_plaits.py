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

# Four paraphonic voices use one shared Particle diffuser bus. The original
# Plaits implementation allocates an 8192-sample (16 KiB) diffuser per voice;
# sharing this effect state preserves the original delay lengths while
# avoiding 48 KiB of duplicated delay RAM. Particle voices are rendered
# sequentially by the FM-1 audio loop, so the shared diffuser is single-owner.
particle_cc = root / "third_party" / "eurorack" / "plaits" / "dsp" / "engine" / "particle_engine.cc"
patch_file(particle_cc, [
    ("namespace plaits {\n\nusing namespace std;\nusing namespace stmlib;\n",
     """namespace plaits {

using namespace std;
using namespace stmlib;

namespace {
Diffuser horatio_shared_particle_diffuser;
uint16_t horatio_shared_particle_diffuser_buffer[8192]
    __attribute__((section(".pool"), aligned(4)));
bool horatio_shared_particle_diffuser_initialized;
}
"""),
    ("  diffuser_.Init(allocator->Allocate<uint16_t>(8192));",
     """  if (!horatio_shared_particle_diffuser_initialized) {
    horatio_shared_particle_diffuser.Init(horatio_shared_particle_diffuser_buffer);
    horatio_shared_particle_diffuser_initialized = true;
  }"""),
    ("  diffuser_.Reset();", "  horatio_shared_particle_diffuser.Reset();"),
    ("  diffuser_.Process(\n      0.8f * diffusion * diffusion,",
     "  horatio_shared_particle_diffuser.Process(\n      0.8f * diffusion * diffusion,"),
], "particle diffuser")
print("Plaits Particle diffuser shared across the four paraphonic voices")


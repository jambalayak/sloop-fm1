# SLOOP, PLAITS port

A fork of SLOOP for the MVAVE FM-1. Changes from upstream isod89/sloop-fm1:

- **PLAITS engine** (`firmware/src/eng_plaits.c`, engine 12): eleven macro-oscillator models in the manner of Mutable
  Instruments Plaits, rewritten in fixed point (VA, WSHAPE, FM, ADDITIVE, CHORD, NOISE, SWARM, KICK, SNARE, HAT,
  PARTICLE) with HARM / TIMB / MORPH, LPG, SUB and DRIVE, and 30 presets (all also in the preset browser).
- **GRAIN effect** (`fx.c`, GLO > GRAIN): a Beads-style grain cloud (MIX, DENSITY, SIZE, PITCH, POSITION) that reads the
  delay line. MIX 0 is off and bit for bit as before.
- **VOLCA drum kit** (`tools/gen_drumkits.py`): original synthesised drum voices in the manner of an analog drum machine.
- **FREAK palette** (`gfx.c`, default): charcoal with an orange-red accent, also in `web/editor.html`.

## What this is not

- Not Plaits' code. Plaits is 32-bit float at 48 kHz; the FM-1's chip has no FPU. Models that need float, wavetables
  or speech data (WAVETABLE, SPEECH, MODAL, STRING, six-op FM, ...) are not included; SLOOP's own SAMPLE, VOICE, PHYS
  and FM6 engines are the nearest.
- Not Beads' code. Beads needs ~800 KB of buffers; the FM-1 has far less. The grain cloud is a small fixed-point
  version in its spirit.
- The look of the MicroFreak is approximated by colours only; the hardware is unchanged.

## Verified (host, in this repo's test harness)

Golden renders (212, existing ones unchanged), pitch and level of every PLAITS preset (`tests/plaits_test.c`), the grain
cloud (`tests/grain_test.c`), UI pages, drum kits, projects, presets, storage, MIDI, punch FX.

## Not verified

- **Not built for the FM-1.** It needs the JieLi AC79 toolchain and SDK (`./build.sh`), which was not available here.
- **Not heard or run on hardware.** The tests check pitch, level, clipping and voice release, not how it sounds.
- **CPU and RAM on the chip are unmeasured.** Run `BUDGET_UPDATE=1 sh tests/run_tests.sh` after a target build and
  check the PLAITS presets against the budget before flashing. The heaviest model is ADDITIVE.
- Settings of the GRAIN page are not saved across power-off.

## Licences

Plaits and Beads are MIT (Emilie Gillet); the credit is in `LICENSING.md` and `LICENSES/MIT-plaits.txt`. SLOOP is GPL-3.0.

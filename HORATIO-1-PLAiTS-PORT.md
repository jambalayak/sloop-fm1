# HORATIO-1: Plaits integration notes

## Source and attribution

The planned macro-oscillator engine is based on Mutable Instruments Plaits by Émilie Gillet.

- Upstream source: https://github.com/pichenettes/eurorack/tree/master/plaits
- Upstream documentation: https://pichenettes.github.io/mutable-instruments-documentation/modules/plaits/
- Plaits DSP source is released under the MIT License.
- Copyright: Copyright 2016 Emilie Gillet.
- The MIT permission notice and disclaimer below must be retained with copied or adapted Plaits source files.
- Do not use the Mutable Instruments name as the name of this derivative instrument. The instrument name is HORATIO-1.

## MIT License

Copyright 2016 Emilie Gillet.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

## Porting constraints

The existing FM-1 firmware engine interface is implemented in C and uses
fixed-point sample processing. The Plaits DSP implementation is C++ and uses
floating-point engine parameters and DSP routines. The current firmware build
compiles the main firmware translation unit with the JieLi pi32v2 Clang
toolchain; its build script does not currently compile a Plaits C++ source
tree or link the Plaits/stmlib dependencies.

Therefore Plaits is **not yet integrated**. It must not be added as a simple
include or linked as an unverified prebuilt library.

Before enabling it in a firmware build, the port needs to establish:

1. Whether this exact JieLi toolchain supports the required C++ language and
   runtime features without pulling in an unavailable standard library.
2. Whether the FM-1 target has acceptable floating-point performance for the
   chosen number of simultaneous voices at the existing 44.1 kHz audio rate.
3. The RAM and flash cost of the selected Plaits engines and their resource
   tables, measured in the actual target build.
4. A safe adapter from the existing `engine_t` / `voice_t` fixed-point
   interface to Plaits' block renderer, including trigger, note, timbre,
   morph, harmonics, envelope and output-level mapping.
5. Host-side regression tests and an FM-1 hardware build before changing the
   default engine list.


## FM-1 implementation reference

A separate open-source FM-1 project, [GATOR](https://github.com/npi3pak/fm1-gator),
has already demonstrated Plaits DSP on this hardware. Its implementation is a
useful engineering reference, not a drop-in replacement for HORATIO-1.

The relevant approach in GATOR is:

- Compile Plaits and stmlib `.cc` files separately with JieLi's C++ compiler,
  using the AC79 core's single-precision FPU (`-mcpu=r3 -mfprev1`). Keep the
  existing C platform and application code out of the floating-point path.
- Call the C++ renderer from C through a small `extern "C"` interface using
  integer values and pointers.
- Construct `plaits::Voice` with placement new in static storage; initialise
  its `stmlib::BufferAllocator` from a fixed shared buffer. Do not rely on
  dynamic allocation or static constructors.
- Render Plaits in its native  block size and copy samples from that block
  into the FM-1 audio callback.
- Check the final image, RAM sections, floating-point helper symbols and
  remaining pool headroom as part of the build.

GATOR's current macro-oscillator app uses **one** `plaits::Voice` object and
is monophonic. Its 16 KiB shared allocator and its memory measurements therefore
must not be treated as the cost of a polyphonic HORATIO-1 implementation.
GATOR exposes 24 models across three banks, including additional models beyond
the standard Plaits set. HORATIO-1's intended preset list should be mapped
explicitly to the upstream Plaits models rather than copied from GATOR's
extended model numbering.


A successful GATOR 48 kHz CI build (17 October 2026) reports a 273,240-byte
firmware image, 16,008 bytes of `.data + .bss`, and 84,780 bytes used from
the 344,064-byte pool, with no soft-floating-point routines linked. This
confirms the single-precision FPU path works on the target. These are GATOR
measurements for its **single-voice** app, not HORATIO-1 measurements; they
must not be presented as the RAM cost of a polyphonic Plaits engine.

### What this changes for HORATIO-1

The C++/FPU risk is no longer an unknown: the FM-1 can run Plaits when the C++
DSP is compiled separately for the FPU and the platform code remains integer
based. The remaining hard question is **polyphony and integration cost**.

HORATIO-1 targets **four simultaneous voices total**, shared across its three
synth parts (not four voices per part). The firmware voice-slot and POLY limits
are now set to four. Before replacing the current engines, prototype one Plaits
voice through the SLOOP audio path, then measure one, two and four simultaneous
Plaits voices for CPU, static RAM, allocator use and flash. Also test model
switching, note stealing, retriggering and sustained notes. The intended design
is a shared four-voice Plaits pool, not a separate four-voice pool for each track.

GATOR is GPL-3.0-only and Plaits/stmlib remain MIT-licensed. If any GATOR
adapter code is copied or adapted, retain its GPL notices and record the
source and changes; retain the MIT notices for Plaits/stmlib files as well.

## HORATIO-1 design intent

- Plaits synthesis models should be exposed as browsable HORATIO-1 presets.
- Target four-voice polyphony total, shared across the three synth parts.
- The separate Volca-inspired drum concept was removed from the project.
- Keep the SLOOP sequencer, effects, package identity and Harbour-compatible
  `.fwsc` format until replacements have been proven compatible.
- Do not remove an existing engine merely to make room until flash, RAM and
  CPU measurements show that this is necessary and the change is reviewed.


## Verified SLOOP build headroom (20 November 2026)

The four-voice source build completed successfully on GitHub Actions. Its linker
report is:

- Firmware image: **571,580 B** of a **581,564 B** app slot (9,984 B free).
- Shared `.pool`: **333,900 B** of **344,064 B** (10,164 B free).
- `.data + .bss`: 57,996 B of 98,304 B.
- Generated sample data: **283,212 B ADPCM**.

The standard Plaits `resources.cc` alone is 378,421 source bytes. It cannot
simply be added to the current image, and a four-voice Plaits allocator cannot
fit in the remaining pool. The GATOR port's 16 KiB allocator is for one voice;
four independent voices need a new shared-allocation design and measured
per-voice cost.

The likely route is to replace redundant legacy synthesis content rather than
stack Plaits on top of it: remove the legacy sample engine/sample bank and
consolidate the existing synth engine table around Plaits' 16 standard models.
The separate GM drum track is also out of the HORATIO-1 design. Before removing
legacy synth engines or sample support, confirm which SLOOP features are to be
retained; preserve the sequencer, preset/project format where possible, and
useful existing effects. The delay buffer and slicer buffers are further
possible RAM savings, but should not be shortened or removed without a design
decision.


## Approved HORATIO-1 redesign

The instrument redesign is approved with these product decisions:

- One Plaits-based synth engine with all 16 standard Plaits models exposed as its
  preset/model choices (not 16 separate engines).
- Four simultaneous Plaits voices total, shared across the three synth parts.
- No separate drum part or drum sounds.
- Retire the legacy SLOOP engine bank and sample-playback engine/sample bank as
  the Plaits engine takes over.
- Retain the three-part sequencer, MIDI performance workflow, project/preset
  workflow where compatibility permits, and useful existing effects.
- Keep the current SLOOP-based build as a known-good reference until each
  replacement stage builds and passes its checks.

Implementation order:
1. Introduce a HORATIO-1 build profile and remove legacy sample/drum payloads
   without breaking the current package format.
2. Port Plaits through the proven FM-1 C++/FPU route, starting with one voice
   and a small fixed allocator; measure flash, RAM and CPU on the FM-1 target.
3. Add the remaining Plaits models and expose the 16 model choices as presets.
4. Expand the shared Plaits allocator to four total voices and verify voice
   stealing, retriggering, and multi-part operation.
5. Remove obsolete legacy engine, sample and drum UI/protocol paths; preserve
   project conversion and sequencer/effect behavior where feasible.
6. Build and test the final `.fwsc` in Harbor before treating it as a release.

No Plaits DSP source has yet been integrated by this decision record alone.


## Approved paraphonic performance design

The four-note mode is paraphonic, not four independently enveloped
polyphonic voices:

- Maintain a four-note held-note stack with predictable note priority.
- Each active pitch is rendered by its own Plaits DSP voice so each note has
  its own oscillator/engine pitch and timbral state.
- All four DSP voices receive the same shared gate and common amplitude
  envelope. Adding or removing one note while other notes remain held does
  not retrigger the common envelope; releasing the final held note releases
  all notes together.
- A new note beyond the four-note limit replaces the oldest held note.
- Voice stealing and note-stack changes must be click-free.
- The shared envelope, note stack, and four DSP instances are one instrument
  resource, not four independently gated SLOOP tracks.

This is the intended musical behaviour. It is not yet implemented in the
firmware. The four-instance Plaits allocator and shared memory cost must be
measured before the engine can be enabled.

## Toolchain checkpoint — 20 November 2026

The opt-in Plaits C++ compile check now passes in GitHub Actions on the FM-1
JieLi toolchain. It compiles the pinned Plaits DSP sources and the required
stmlib DSP utilities, while excluding the upstream STM32 drivers and
bootloader. The normal SLOOP firmware build and .fwsc packaging also pass
with this integration groundwork present.

This checkpoint validates compilation only. It does not mean Plaits is
linked into the installable HORATIO-1 image or that the paraphonic engine is
implemented. The next engineering stage is to add a small C-compatible
HORATIO adapter, measure the RAM and flash cost of one Plaits voice, then
measure four voices and the shared-envelope mixer before changing the
existing track/voice allocator. Do not enable the adapter in release builds
until those measurements and audio tests pass.

## Engine integration constraint

SLOOP's existing engine interface calls `render()` once for each active
voice and `block()` once for each synth part. A four-note shared-gate Plaits
instrument cannot simply create a fresh Plaits `Voice` inside each
`render()` call: that would give late notes their own envelope start and
would not provide one instrument-wide gate across the three sequencer
parts.

The adapter must own the four Plaits DSP instances and a central note/gate
state. It must advance the shared gate/envelope exactly once per audio
block, render each active pitch with that same gate, and mix the results
without advancing the envelope separately for each note. The integration
must also map SLOOP's global four-voice allocation to those four DSP slots
and preserve click-free voice stealing. This is a design requirement, not
yet implemented.


## Scale-aware keyboard and sequence transposition — approved design

CTL-1 is a useful FM-1 reference for scale-aware keyboard interaction, but
HORATIO-1 should keep its SLOOP sequencer and project format rather than
replace them with CTL-1's application. The current SLOOP `seq.c` already has
a scale table, root/scale quantisation, white-key scale-degree mapping and
diatonic chord generation. It currently has 15 scale masks and chord-mode
black-key modifier behaviour; this is not yet the requested CTL-1-style
white-note / black-chord keyboard map.

Target behaviour:

- Manual root-note and scale selection, available from the performance and
  sequencer workflow.
- Keyboard mapping is an explicit user-selectable option, independent of
  the selected root and scale:
  - CHROMATIC: the standard keyboard layout; every key plays its normal
    semitone note.
  - SCALE + CHORDS: the 16 white keys play successive scale degrees; the 11
    black keys trigger the diatonic triad built on the corresponding scale
    degree.
- Switching mapping must not alter the selected root, scale, octave, sequence,
  or stored notes. Make the active mapping visible in the UI.
- Chord-key note-on must start all notes in that triad; key-up must release
  exactly those notes, including when keys are held together, the track changes,
  or the mapping is switched while a key is held. No hanging internal or MIDI
  notes.
- Chord voicing is capped at three notes for these black-key triggers, within
  the instrument's four-note performance limit. Keep the existing separate
  chord/strum features available where compatible; do not conflate them with
  this keyboard mapping.
- Preserve MIDI note input/output and keep incoming MIDI notes chromatic unless
  the user explicitly enables a separate MIDI scale-quantise option.
- The keyboard display should show the selected root and scale tones, with a
  restrained neon/vaporwave visual treatment while retaining readable labels.

Sequence key/scale transformation:

- Provide two distinct operations: (1) semitone transpose, which moves every
  stored note by a fixed interval and preserves intervals; (2) scale remap,
  which maps each note's scale degree from the source root/scale into a chosen
  destination root/scale.
- Preview the destination root and scale before applying. Apply/Cancel must
  leave the original pattern untouched until Apply.
- Preserve step positions, lengths/ties, velocity, accents, slides, ratchets,
  microtiming, fill conditions and parameter locks. Only note pitches change.
- Handle notes outside the source scale deterministically (nearest scale
  degree, with ties resolved downward); preserve octave/register as far as
  MIDI 0..127 allows. Report or preview any notes clipped at the MIDI limits.
- Operate on the selected synth track only; do not transform drum lanes.
- Do not silently change the track's live root/scale settings when transforming
  stored notes. The user chooses whether to also set the track's playback
  root/scale.
- Preserve old project-file compatibility; do not change the serialized step
  structure merely to add this feature.

Implementation sequence:

1. Add and host-test pure scale-degree mapping helpers (including negative
   octaves, non-diatonic source notes, and MIDI boundaries).
2. Add the white-key / black-key chord keyboard mode, reusing existing
   scale masks and chord builder where possible; ensure key-up releases all
   notes started by a chord key and never leaves MIDI notes hanging.
3. Add a previewable, undo-safe sequence transform for semitone transpose and
   scale remap; verify that all non-pitch step metadata is byte-for-byte
   unchanged.
4. Add compact UI controls and keyboard feedback; test all 15 existing scales,
   chord voicings, MIDI input/output, recording, ties and multi-part playback.
5. Build and test the Harbour-compatible package before enabling the feature
   in a release.

CTL-1 is a reference for interaction and FM-1 conventions, not a wholesale
firmware dependency. Preserve its licensing notices if any code is adapted;
prefer a clean implementation using HORATIO-1's existing scale/chord routines.


### Selectable keyboard mapping — first implementation pass

- Reused the existing per-track QNT parameter rather than adding a new
  serialized parameter: CHROM (standard semitone layout), SNAP (existing
  scale-quantized chromatic layout), and SCALE+CHORDS.
- In SCALE+CHORDS, white keys continue to play individual scale notes and
  black keys now trigger three-note diatonic triads, following the CTL-1
  interaction pattern with a clean-room implementation using SLOOP scale masks.
- Each physical key stores the exact notes it started. Existing key-up handling
  releases those stored notes and sends the matching MIDI note-offs, even if
  root, scale, octave, or mapping changes while the key is held.
- The existing CHORD / CHORD+ voicing mode remains available in the other
  keyboard mappings; SCALE+CHORDS uses its dedicated black-key triads instead.
- Added host regression coverage for chromatic notes, scale white keys, all
  black-key chord positions, and independent chord key-up/MIDI note-offs.
- This is the first implementation pass. It must pass the new host test and
  firmware build before being treated as validated; the installed FM-1 has not
  been updated.

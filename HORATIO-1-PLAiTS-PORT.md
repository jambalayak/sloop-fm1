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

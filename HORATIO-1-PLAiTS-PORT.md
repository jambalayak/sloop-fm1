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

## HORATIO-1 design intent

- Plaits synthesis models should be exposed as browsable HORATIO-1 presets.
- The separate Volca-inspired drum concept was removed from the project.
- Keep the SLOOP sequencer, effects, package identity and Harbour-compatible
  `.fwsc` format until replacements have been proven compatible.
- Do not remove an existing engine merely to make room until flash, RAM and
  CPU measurements show that this is necessary and the change is reviewed.

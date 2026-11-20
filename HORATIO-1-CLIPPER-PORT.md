# HORATIO-1: Clipper-based development plan

Status: architecture pivot agreed on 2026-11-20. This document records the direction; it does not claim that the Clipper source has already been imported or that Plaits is integrated.

## Upstream base

Use [isod89/clipper-fm1](https://github.com/isod89/clipper-fm1) as the firmware foundation, rather than continuing to remove SLOOP groovebox code from this repository.

The Clipper upstream inspected for this decision is:
- Branch: `main`
- Commit: `7bc15a950a8b3fd51aec280514905448e4faaf1b`
- Release described by that commit: Clipper 1.3.1

Keep this SLOOP-derived repository and its `plaits-development` branch intact as a reference. Do not overwrite `main` or merge this evaluation branch into the SLOOP line.

## Why Clipper

Clipper is already an FM-1 synthesizer firmware. Its synth profile has no drum machine, no drum track, no song mode and no built-in factory samples. It retains the useful instrument features we want to keep: the effects, arpeggiator, step sequencer, MIDI, USB audio, and a browser editor. It also has a known-working FM-1 build and install package.

The current SLOOP-derived HORATIO experiment has successfully compiled the upstream Plaits C++ sources, but its latest target link failed: the `.pool` section overflowed the target POOL region by 29,984 bytes. This makes a smaller, synth-first foundation preferable to further dismantling SLOOP.

## HORATIO-1 target

1. Base the instrument on Clipper's synth-only profile.
2. Remove or disable surplus oscillator engines as needed to make room for the complete Mutable Instruments Plaits DSP.
3. Integrate the complete 16-model Plaits engine. Clipper's existing MACRO engine is Plaits-inspired, but is a separate implementation with four models; it is not a substitute for Plaits.
4. Preserve the requested four-note shared voice budget and paraphonic envelope/gate behaviour.
5. Port the HORATIO-1 Synthwave / Vaporwave / IDM preset bank (50 presets) and map each preset to an actual Plaits model and its parameters.
6. Retain useful Clipper effects and add the planned Beads-inspired granular effect only if memory and CPU budgets allow.
7. Apply the HORATIO-1 visual direction: MicroFreak-inspired controls, vaporwave colours, and original audio-reactive visualizer screens. Keep the FM-1's small-screen legibility.
8. Keep the independent metronome/click; do not reintroduce a drum machine, drum lane, kit system, or drum engine.

## Porting and compatibility rules

- Start from an actual fork/copy of Clipper 1.3.1 source. Do not try to turn this branch's SLOOP history into a Clipper tree by a sequence of ad-hoc file substitutions.
- Keep upstream Clipper changes traceable and preserve its GPL-3.0-only notices and third-party licences.
- Keep FM-1 hardware identity, flash compatibility, update-loader behaviour, and `.fwsc` package generation from the Clipper base.
- Keep the original SLOOP and Clipper repositories untouched.
- Do not claim a Plaits integration based only on compiling Plaits source files. The engine must be connected to note events, pitch, modulation, gates, rendering, and the preset/editor controls, then pass host tests and a target build.
- Do not publish or recommend a firmware package until the HORATIO-1 build and package checks pass.

## First implementation milestones

1. Establish a dedicated HORATIO-1 fork of Clipper and pin the upstream base commit.
2. Reproduce an unmodified Clipper 1.3.1 build and host test suite before changing code.
3. Measure the baseline target flash, POOL, RAM, and CPU budgets.
4. Add Plaits as a separately testable engine, initially with one voice and one model, then enable all 16 models.
5. Reduce the engine set and voice allocation to meet the FM-1's memory and CPU limits.
6. Implement four-voice paraphonic behaviour and test note stealing, retrigger, release, and rapid MIDI input.
7. Port the 50 presets and verify each preset's engine/model mapping.
8. Add the visualizer and effect work only after the synth core is stable.

## Current branch note

This branch is a planning checkpoint created from `jambalayak/sloop-fm1:plaits-development`. It is not yet a Clipper source fork. The next repository-level operation is to create or obtain a writable fork of `isod89/clipper-fm1`, then move implementation work there.

## Source audit completed after the pivot

A read-only audit of the pinned Clipper tree confirms several important implementation details:

- The upstream tree is a complete 400-entry source tree at the pinned commit, with firmware under `firmware/`, 83 host-test files under `tests/`, and the web editor/installer under `web/`.
- The production synth profile is selected by `CLIPPER_SYNTH=1` in `firmware/src/core.h`. In that profile, the code documents tracks 2–4 and the drum machine as silent and unselected; the groovebox profile (`CLIPPER_SYNTH=0`) is retained for shared-code host tests. Drum-related source files and test fixtures still exist in the repository, so HORATIO must verify the final synth build and UI contain no accessible drum system rather than assuming the files are absent.
- The synth profile currently defaults to `NVOICE=8`; Clipper's build instructions document 12- and 16-voice options. HORATIO's four-voice target should be an explicit build configuration, not an assumption that the default already matches.
- Engine numbering is append-only and preset/editor protocol values depend on it. Clipper's existing MACRO engine is index 16; if engines are removed or reordered, the preset format and editor mappings need deliberate migration or a clean new HORATIO format.
- Clipper's `engines.c` uses a shared per-part engine arena for the larger engines. Plaits' per-voice state and shared resources must be budgeted against this arena and the linker POOL region before enabling four voices. The linker script `firmware/app.ld` assigns POOL a fixed length of `0x54000` bytes (344,064 bytes); this is a hard placement limit, not spare memory that can be assumed available.
- The build produces `felucca.bin`, `loader/ota.bin`, and `felucca.fwsc`. Its documented build needs the JieLi pi32v2 toolchain and AC79 SDK; host tests alone cannot certify a flashable package.
- Licensing is GPL-3.0-only for the firmware. The existing Clipper tree also identifies third-party components with separate notices (including Apache-2.0 msfa, MIT physical-model sources, and OFL font assets). HORATIO must retain applicable notices and must not bundle restricted Hügelton artwork/sample assets unless their separate terms are satisfied.

This audit is based on the source at `7bc15a950a8b3fd51aec280514905448e4faaf1b`; it is not a build or hardware test.

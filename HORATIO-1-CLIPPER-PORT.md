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
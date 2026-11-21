# HORATIO-1: dedicated drum-system removal plan

Status: approved scope; implementation not yet complete.

## User-approved target

HORATIO-1 has three pitched synth tracks and no dedicated drum track, drum sequencer, drum kits, drum-only voices, drum MIDI channel, or drum-only UI. Keep pitched sample playback for now. Plaits percussion models are not to be exposed as a dedicated drum system.

## Compatibility rules

- Keep the existing SLOOP project format identifiers and legacy readers intact until a deliberate migration is implemented and tested.
- Treat the on-disk four-track layouts in project formats 1–6 as historical structures. Do not change their array sizes or static size assertions merely because the live instrument will have three tracks.
- On import, retain the first three synth tracks and intentionally ignore the legacy fourth drum track. New saves must use a separately versioned three-track format if the live structure no longer matches the historical format.
- Keep the metronome click independent of drum kits and drum-track mute/level. Its current implementation is in the drum audio source and must be extracted before deleting that source.
- Preserve MIDI routing, note handling, scale/chord keyboard behavior, undo, pattern editing, and editor functionality for the three synth tracks.
- Keep global FX such as reverb, delay, chorus, filter, dust and master processing unless they are exclusively coupled to the drum bus. Remove kick-triggered ducking or redesign it without a drum dependency.
- Keep pitched sample playback and its assets. Remove only the dedicated drum-kit/sample/synth machinery.

## Dependency map

The current implementation is cross-cutting:
- `core.h`: four live tracks, `TRK_DRUM`, `dstep_t`, `TDRUM`, drum globals and drum-track helper.
- `drums.c` / `drum_synth.c`: GM and user drum kits, synthesized drum voices, drum lanes, tuning/decay, hit indicators and the metronome click.
- `voice.c`: drum note dispatch and drum-specific voice/visualization handling.
- `fx.c`: drum buffers, drum insert/filter/send path, kick-triggered ducking, metronome mixing and drum mix scheduling.
- `seq.c`: drum note recording, lane mapping, drum roll/input, drum MIDI channel, track-specific note and pattern branches.
- `project.c`: historical four-track project structs, drum-step conversion and drum-specific tuning serialization.
- `ui_input.c`, `ui_studio.c`, `params.c`: drum pages, grid, kit/sound controls, track selection and global parameter pages.
- `editor.c`: drum step protocol and drum kit editor compatibility.
- `panel.c`: drum preview and MIDI channel defaults.
- `tools/build.py`: generated drum-kit header dependency.

## Implementation sequence

1. **Extract independent metronome** from the drum implementation and mixer, preserving click level, timing and transport behavior. Build and test.
2. **Introduce explicit live-vs-legacy track counts**. Make the live sequencer contain three synth tracks while retaining fixed legacy project structs and converters. Add host tests proving formats 1–6 still load safely and the first three tracks survive. Define/version the new three-track save format before changing serialization.
3. **Remove live drum dispatch and mixer path**: drum voices, drum buffers, drum insert/filter, kick-triggered ducking, and drum-only scheduling. Preserve master FX and click. Build and test.
4. **Remove drum sequencer and MIDI paths** while retaining synth note input/output, recording, roll, chords, scale mapping, program changes and three-track routing. Build and test.
5. **Remove drum UI and editor surface**: drum pages, kit/sound pages, pads, drum globals and drum-only protocol operations. Preserve protocol safety for old clients where appropriate.
6. **Remove dead drum assets and build generation** only after no runtime or test references remain.
7. Run the complete firmware build, project-format host tests, sequencer/MIDI tests, and package generation. Verify the output remains installable in Harbour.

## Completion criteria

Do not call drum removal complete until the firmware builds, the package is generated, project compatibility tests pass, and inspection confirms there is no dedicated drum track, kit, drum voice, drum MIDI route or drum UI left. Report any retained compatibility-only code explicitly.


## HORATIO Plaits branch audit (2026-11-21)

The `plaits-development` branch already has some of the target architecture, but the drum implementation is not yet removed:

- `core.h` defines three live synth parts and a fourth legacy compatibility slot. The fourth slot remains in the in-memory track array for old project conversion; it is not a live drum track.
- `seq.c` rejects legacy fourth-track keyboard and incoming MIDI drum input, and the UI parameter page filter hides the DRUMS configuration page. The drum-related sequencing, MIDI mapping, and editor compatibility code is still present in the translation unit and needs a deliberate compile-time separation.
- `metronome.c` is already independent and `fx.c` mixes its click separately. Preserve this implementation; do not retain the drum mixer just to keep the click.
- `fx.c` already omits kick-triggered sidechain ducking and sizes its track-filter / insert state for `NPART`, but its mixer comments and other legacy drum references need auditing before removing the drum source.
- Under `HORATIO_PLAITS`, `engines.c` exposes only `ENG_PLAITS`; the legacy engine table and shared GRAIN/FM6/PHYS arena are excluded. The sample engine source is still included for sample/drum data declarations during migration, so its remaining dependencies must be separated rather than deleting the file blindly.
- `project.c` already declares the HORATIO FUN7 three-synth-track format and contains converters for historical drum-bearing formats. Keep those historical readers and converters; new-format serialization must remain three-track.
- The build workflow runs a SLOOP reference build first, then a clean `HORATIO_PLAITS=1` build. Any conditional compilation must preserve the reference build unchanged.

### Updated safe-removal sequence

1. Add HORATIO-only compile-time boundaries around drum audio/kit implementation while retaining the independent metronome and historical project conversion. Keep the SLOOP reference build unchanged.
2. Remove drum references from sequencer/MIDI/UI paths in the HORATIO build, using explicit legacy no-op handling only where required by old file formats or editor protocol.
3. Remove drum-kit generation and assets from the HORATIO build only after no included source references the generated header or sample zones.
4. Compare the linker map and POOL usage before/after each stage. Do not assume deleting a preset label or source file saves RAM: verify the linked sections.
5. Run both workflow builds, keyboard-scale tests, preset validation, project-format tests, and package generation before calling the removal complete.

No drum source, kit, sample, or compatibility reader was deleted in this audit-only update.


## 2026-11-21 — Plaits POOL / low-RAM follow-up

A second pass checked the actual HORATIO adapter and linker memory map before moving any buffers.

- `firmware/cxx/horatio_plaits.cc` places the four `PlaitsSlot` objects in `.pool`. Each slot contains a 4096-byte allocator arena plus a Plaits frame FIFO and small state. The separate placement storage for `plaits::Voice` objects is currently ordinary BSS.
- `firmware/app.ld` defines POOL as 0x54000 bytes (344,064 bytes). The known failing link reports an overflow of 29,984 bytes.
- The `.lowram` region is only 0x7F00 bytes (32,512 bytes) total, not 32 KiB free. It already contains the second-core stacks (3,072 bytes), the reverb delay lines (17,368 bytes), two mix accumulators, per-core part scratch, and preset-crossfade scratch. Moving all four Plaits slots there is therefore not safe and cannot fit.
- `main.c` explicitly probes and zeroes the entire `.lowram` region at boot; its safety depends on all linker-placed content staying within the declared region.
- The audio scope buffers are ordinary BSS, not POOL. They are not an obvious way to reclaim the overflowing section.
- The latest linker-map diagnostic was added to `tools/build.py`, but the previous failing CI run predates that diagnostic. Its log therefore contains no POOL input-object breakdown. The new `--print-memory-usage` option is also not yet confirmed by a run.

Decision: do not move Plaits slots into `.lowram`, do not enlarge POOL across the fixed stack-guard addresses, and do not delete drum compatibility data as a speculative memory fix. Next valid step is to obtain a fresh link map from the current source revision, then identify the largest individual `.pool` contributors and make a measured allocation change. Preserve all 16 Plaits models and four shared voice slots.

### Linker-log figures and largest buffer candidates

The archived CI log from the last SLOOP reference build provides the baseline figures:
- `.pool`: 333,900 / 344,064 B (10,164 B spare)
- `.bss` RAM: 55,180 / 98,304 B (43,124 B spare)
- `.lowram`: 22,260 / 29,384 B (7,124 B spare)

The failed HORATIO link overflowed POOL by 29,984 B. The build's post-link policy requires at least 8,192 B of spare POOL, so a successful fix needs roughly 38,176 B of POOL relief, not merely 29,984 B. The Plaits C++ voice-storage probe reports about 14.9 KB for four Voice objects in ordinary BSS; this is separate from the four PlaitsSlot objects in POOL. The exact HORATIO BSS total still needs a successful link or object-section accounting.

Large source-level POOL allocations worth accounting for in the fresh map:
- FX tempo delay: 65,536 int16 samples = 131,072 B
- Graph/visualiser canvas: 240 × 120 RGB565 = 57,600 B
- Slicer recording: NTRK × 4,096 int16 samples = 32,768 B (HORATIO has three active synth parts plus one retired compatibility slot; the fourth buffer is a possible 8 KB compile-time saving, but the synth slicer itself remains useful)
- Plaits slots: four fixed-size PlaitsSlot records, each with a 4 KB arena and frame FIFO
- Visualiser FFT/spectrogram scratch: 4,608 B total

Do not infer that every listed object survives the HORATIO link: compile-time guards and section garbage collection can change the final contribution. The new linker-failure diagnostics now try to print both map entries and per-object `.pool` sections/symbols, so the next real HORATIO build can settle the exact contributors before a memory-affecting edit.

### 2026-11-21 — Graphics-canvas reduction rejected

A 240 × 32 canvas looked sufficient when considering only the generated fonts (S = 16 px, L = 32 px), but `cv_px` is shared with the visualiser. `ui_vis.c::vis_draw()` renders the 240 × 240 display in two 240 × 120 bands, so reducing the shared canvas to 32 rows would clip most visualiser content. The 32-row proposal was reverted before validation. A smaller, safe trim then removed only the four unused rows below the 120-row visualiser band: `CV_MAX` is now 240 × 120 RGB565 = 57,600 B, saving 1,920 B (1.875 KiB) while preserving the full visualiser band. This modest estimate still needs confirmation from the linker map and post-link checks.

## Rejected canvas reduction (2026-11-21)

A proposed reduction of `CV_MAX` from `240 * 124` to `240 * 32` was audited and reverted before validation. Although generated fonts are at most 32 pixels high, the same shared canvas also renders the visualiser in 240 x 120-pixel bands (`ui_vis.c::vis_draw`). The 32-pixel limit would silently clip those screens. Keep the full 120-pixel visualiser canvas. The accepted trim from 124 to 120 rows saves only 1,920 B; the rejected 32-row proposal contributes no saving. No build success is implied.

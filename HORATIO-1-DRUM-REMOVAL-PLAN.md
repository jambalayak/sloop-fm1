# HORATIO-1: dedicated drum-system removal plan

Status: approved scope; staged implementation in progress. HORATIO's three-track runtime boundary is now built and Harbor-validated; the remaining drum-system removal stages are still outstanding.

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


### 2026-11-21 — Remove the retired drum slicer recording slot from HORATIO

The shared slicer recording buffer was sl_buf[NTRK][SL_LEN]: four 4,096-sample int16_t recordings (32,768 B total). HORATIO's audio mixer calls slicer_track() only for the three active synth parts (mix_part() iterates NPART); the fourth track is legacy project compatibility, not a live drum part. slicer_start() now sizes its state loop to SL_TRACKS, which is NPART under HORATIO_PLAITS and NTRK in SLOOP. The drum-only slicer_drums() path and its scratch buffer are excluded from HORATIO, while remaining intact for SLOOP.

**Estimated HORATIO POOL saving: 8,192 B** (one 4,096 × 16-bit recording slot). Synth-track SLICER GATE/STUT behavior is retained. This depends on the confirmed call graph: HORATIO's mix_part() uses only tracks 0–2, and the only drum-track caller was slicer_drums(), now compiled only for SLOOP. The next HORATIO build must confirm the reduced .pool size and run all existing regression, SLOOP-reference, keyboard, preset, and package checks. No build success is claimed yet.


### 2026-11-26 — Live four-slot runtime dependency audit

A source-level audit traced the remaining HORATIO references to the fourth in-memory track. The fourth slot is not an active Plaits part, but it is still part of the current working model; it is therefore not safe to change `NTRK` globally from 4 to 3.

Confirmed dependencies that still assume four working slots:

- `core.h`: `track_t trk[NTRK]`, the selected-track comment/range, expression state, and the legacy `TDRUM` alias.
- `seq.c`: four-slot program-change queues (`pc_req`, `pc_bank`), MIDI-out note ownership (`mo_set`, `mo_ch`), sustain and KBD HOLD state, live/MIDI-held-note bitsets, selected-track MIDI note-on ownership, panic masks, and per-track sequencer ticking. Lost-MIDI-note cleanup also scans all four slots.
- `voice.c`: the strum queue stores a track index resolved through `NTRK`; envelope-sync coefficients and note-visualisation state are also sized by the working-track count.
- `main.c` and `ui_input.c`: startup and NEW initialise/reset all four working slots, while engine setup is limited to `NPART`.
- `project.c`: the internal FUN6 working/staging representation remains four tracks so the historical readers and FUN7 bridge can coexist. The on-flash FUN7 structure is already exactly `NPART` (three) tracks; `proj_from_v7` fills the fourth staging record with an empty legacy drum record, and `proj_to_v7` writes only the first three.

Confirmed active boundaries remain three parts: audio mixing, Plaits engine switching, arp ticking, track selection, dedicated MIDI channel ownership, and FUN7 serialization all use `NPART`. Incoming note handling rejects the retired fourth slot as a drum path.

**Safe migration direction:** introduce a distinct three-track live runtime array and keep four-track legacy import/staging structures local to project conversion. Make project application copy only the first three converted records into live tracks. Then audit and resize each per-track runtime state array to `NPART`, replacing modulo-`NTRK` indexing with checked three-part indices where appropriate. Keep the SLOOP build's four-track behavior unchanged behind its existing build configuration. Do not attempt this as a one-line macro change.

No firmware source was changed in this audit and no new build is claimed. The current successful build remains the safety baseline.


### 2026-11-26 — HORATIO live track count split (implementation in progress)

The first runtime boundary change is now in `firmware/src/core.h`:

- With `HORATIO_PLAITS`, `NTRK` is now `NPART` (three). The live `trk[]` array and the generic sequencer/MIDI/editor state arrays therefore have three entries.
- In the SLOOP reference build, `NTRK` remains four and the original drum track remains present.
- `TRK_DRUM` remains the historical fourth-track index for project-format conversion; it is not a valid HORATIO live-track index.
- Historical project record types in `project.c` continue to use the explicit `PROJ_NTRK_LEGACY == 4`. FUN1..FUN6 readers retain their four-record layouts, and FUN7 remains a three-record on-flash format.
- The existing project capture/apply loops use the build-specific live `NTRK`: HORATIO captures/applies only three live tracks, while the fourth staging record stays zeroed by project capture. Legacy import conversion still processes all four historical records before applying only the three live synth tracks.

This is the first compile-time separation only. The HORATIO build and all regression tests must pass before the runtime boundary is considered validated; no build success is claimed here.


### 2026-11-26 — Three-track runtime build and Harbor validation

The live-track split described above has now passed the full SLOOP-reference and HORATIO-1 CI workflow (run 45758878743, commit 9563ea8f6d7ef623e6061f175fe115843396ac9b). Both firmware packages were generated, project migration/FUN7 compatibility tests passed, and the HORATIO Plaits/Beads audio smoke tests passed. The user has also confirmed that the resulting HORATIO-1 firmware package loads successfully in Harbor and that all three synth tracks work.

The runtime boundary is therefore validated at the build/package and Harbor-emulator level. This does not complete dedicated drum-system removal: legacy drum conversion code and SLOOP's drum implementation remain intentionally preserved. Next implementation work should continue with a narrow HORATIO-only separation of drum runtime dependencies, keeping the independent metronome, three synth tracks, global FX, MIDI behavior, pitched sample playback, and historical project readers intact. Each removal step must be measured and regression-tested before proceeding.


### 2026-11-26 — Make the legacy drum-track pointer SLOOP-only

A follow-up audit found that `TDRUM` was still defined unconditionally as `&trk[TRK_DRUM]`. In HORATIO, `trk` has only `NPART` (three) entries, so merely forming that pointer was an avoidable out-of-bounds design hazard even though HORATIO's `is_drum()` implementation never dereferenced it. The macro is now defined only in the SLOOP branch, alongside a compile-time assertion that SLOOP retains the fourth slot. HORATIO keeps its always-false `is_drum()` helper and has no drum-track pointer at all. This is a compile-time safety cleanup only: no audio, MIDI routing, sequencer behavior, project layout, or SLOOP runtime behavior is intentionally changed. The next CI run must confirm both targets still compile and pass their regression tests.

### 2026-11-26 — Drum-pointer boundary build confirmation

The SLOOP-only `TDRUM` pointer boundary and the HORATIO three-track runtime now pass the full workflow on the documentation commit after a transient external SDK-download failure was retried successfully. Run 45763308338, attempt 2, completed successfully. The corresponding source-only run 45763303780 also passed. The retry failure was a Gitee TLS disconnect while cloning the external JieLi SDK; no source or compiler failure was involved.

The existing HORATIO `drums.c` branch was rechecked: it contains only legacy drum-step conversion helpers, a no-op drum dispatch stub, and the shared level/pan ramp helper; the full drum voice/kit/sample/synth implementation and `drum_synth.c` include are behind the SLOOP `#else`. The HORATIO FX path mixes the independent metronome and has no drum renderer call. Stage 1's core audio/kit separation is therefore already present and validated. The next work should target the remaining compiled-but-inert sequencer/MIDI/editor branches, with the historical project conversion left intact.

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

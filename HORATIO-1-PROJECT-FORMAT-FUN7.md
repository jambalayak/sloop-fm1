# HORATIO-1 project migration: FUN7

Status: design specification only. No project serialization code has been changed by this document.

## Why a new format is required

The current live track count is `NTRK == 4`, and FUN6 serializes four track records. FUN1–FUN5 also have fixed historical layouts containing four tracks (FUN1 is the one-track exception). Changing `NTRK` in `core.h` and continuing to use it in the project structs would silently change historical structure sizes and break imports.

HORATIO-1 will have exactly three live synth tracks. The project format must therefore stop using the live `NTRK` constant to describe old on-disk formats.

## Proposed FUN7 layout

- Magic: `0x46554E37` ("FUN7").
- Header, globals, selection, drum-delay/reserved compatibility bytes, and checksum retain the FUN6 field ordering.
- Exactly three `proj_trk_t` records, one for each synth track.
- No drum step union member or drum tuning/decay payload is written in FUN7.
- The existing track record's note-step, microtiming, lock and fill representation is retained for each of the three synth tracks.
- The expected packed C structure size with the current parameter schema is 2,882 bytes (verify with a compile-time assertion before enabling writes). This is below the 3,840-byte flash payload limit.
- The checksum remains FNV-1a over all bytes except the trailing checksum field, matching existing project validation.

## Import and migration

1. Define explicit constants for live synth tracks (3) and legacy stored tracks (4). Do not use live `NTRK` in FUN1–FUN6 structure declarations, loops, or size assertions.
2. Keep byte-for-byte legacy structures for FUN1, FUN2, FUN3, FUN4, FUN5 and FUN6. Their sizes and checksums must remain unchanged.
3. On FUN1 import, load its single synth track into track 1; tracks 2 and 3 receive current synth defaults.
4. On FUN2–FUN6 import, convert only legacy tracks 0–2. Validate the full legacy payload and checksum, but intentionally ignore legacy track 3 (the old drum track).
5. Set the new project's selected track to a valid synth track (clamp legacy drum selection to track 3 / index 2).
6. New captures, section saves, autosaves and project writes use FUN7 with exactly three tracks.
7. Loading a FUN7 project applies only three synth tracks. No drum globals, drum delay, drum tune/decay, drum MIDI channel or drum-only track data are applied.
8. A project saved as FUN7 must never be written using the FUN6 magic or legacy four-track structure.

## Required tests before switching live NTRK to three

- Static assertions prove the exact sizes of FUN1–FUN7 and that FUN1–FUN6 remain unchanged.
- Existing golden FUN1–FUN6 fixtures still validate and import.
- For each legacy four-track format, deliberately populate track 4 with distinctive drum data and prove it is ignored while tracks 1–3 survive byte-for-byte where their format allows.
- A legacy selected-track value of 3 maps safely to synth track index 2.
- FUN7 capture/import round-trip preserves all three synth tracks, engine/preset, parameters, note steps, levels, ratchets, microtiming, locks and fill conditions.
- Bad checksum, wrong length, wrong magic and unsupported format are rejected.
- Autosave, section A–D save/load, flash erase and restore use the FUN7 size and magic.
- The project object and temporary flash staging union remain within their RAM/flash limits.
- Run `tests/run_tests.sh`, full firmware build, and package generation.

## Ordering constraint

Do not set `NTRK` to 3 until FUN7 is implemented, all storage paths use it for new writes, and the legacy-format test suite passes. Until then, keep the current four-track runtime and do not claim the drum track has been removed.

# HORATIO-1 Factory Preset Bank

**Factory bank:** 190 entries: 91 complete Rochefsky Plaits 1.2 recipes, three six-op FM bank selectors, and all 96 bundled DX7-format patches (32 in each of Plaits' three FM banks). The PRESETS browser exposes every entry in source-sheet order and groups them by Plaits bank (ORNG, GREEN, RED).

**Source:** [Rochefsky — Plaits sound-design recipes](https://www.rochefsky.com/learnings/plaits), shared spreadsheet/PDF supplied for this project.

## Model mapping

The source's bank/model numbers map to the 24 Plaits 1.2 engines as follows:

- Orange 1–8 → HORATIO model indices 0–7
- Green 1–7 → HORATIO model indices 8–14
- Red 1–8 → HORATIO model indices 15–22

The model index is zero-based in firmware. The recipe bank order follows the source sheet, not a genre grouping.

## Factory recipes

| # | Recipe | Source model → firmware index | Trigger in source | H/T/M values (0–127) |
|---:|---|---|---|---:|
| 1 | BOWED HARM | Red 1 → 15 | no | 25/25/64 |
| 2 | MOSQUITO | Red 1 → 15 | no | 0/0/0 |
| 3 | SCN PLUCK | Red 1 → 15 | yes | 25/0/64 |
| 4 | MARIMBA BNC | Red 2 → 16 | yes | 127/25/89 |
| 5 | NOISE MAR | Red 2 → 16 | yes | 64/127/127 |
| 6 | BUBBLE LIQ | Red 3 → 17 | no | 64/76/83 |
| 7 | PUDDLE DOG | Red 3 → 17 | yes | 127/76/83 |
| 8 | CEMENT TUNL | Red 3 → 17 | yes | 64/76/25 |
| 9 | WIND CHIME | Red 3 → 17 | no | 64/25/127 |
| 10 | WATER DRIP | Red 3 → 17 | yes | 0/0/70 |
| 11 | WATER DRIP 2 | Red 3 → 17 | yes | 0/0/70 |
| 12 | WATER DRIP 3 | Red 3 → 17 | yes | 0/0/127 |
| 13 | CPU NOISE | Red 3 → 17 | no | 102/89/127 |
| 14 | HARP SOFT | Red 4 → 18 | yes | 0/0/83 |
| 15 | HARP BUZZ | Red 4 → 18 | yes | 0/89/89 |
| 16 | CHURCH BELL | Red 4 → 18 | yes | 89/127/127 |
| 17 | STRING PAD | Red 4 → 18 | yes | 25/127/127 |
| 18 | SNARE BLOCK | Red 4 → 18 | yes | 127/127/127 |
| 19 | WOOD BLOCK | Red 4 → 18 | yes | 127/127/0 |
| 20 | STRING MUTE | Red 4 → 18 | yes | 0/102/25 |
| 21 | STEEL DRUM | Red 4 → 18 | yes | 102/127/51 |
| 22 | BRASS SECT | Red 4 → 18 | yes | 13/127/83 |
| 23 | STAND BASS | Red 4 → 18 | yes | 0/64/64 |
| 24 | SLAP BASS | Red 4 → 18 | yes | 0/127/0 |
| 25 | GLASS BELL | Red 5 → 19 | yes | 127/45/51 |
| 26 | PENCIL BELL | Red 5 → 19 | yes | 127/45/51 |
| 27 | LOW DRIP | Red 5 → 19 | yes | 25/127/0 |
| 28 | XYLOPHONE | Red 5 → 19 | yes | 127/64/51 |
| 29 | WOOD STICKS | Red 5 → 19 | yes | 127/127/0 |
| 30 | POP CORN | Red 5 → 19 | yes | 0/127/0 |
| 31 | LASER SHIP | Red 5 → 19 | yes | 64/64/96 |
| 32 | CHICKS | Red 5 → 19 | yes | 64/127/0 |
| 33 | RESO BASS | Red 5 → 19 | yes | 38/0/38 |
| 34 | TOY PIANO | Red 6 → 20 | yes | 64/0/64 |
| 35 | PLUCK BASS | Red 6 → 20 | yes | 64/25/102 |
| 36 | SIREN | Red 6 → 20 | yes | 127/64/127 |
| 37 | HIHAT SNAP | Red 7 → 21 | yes | 127/64/25 |
| 38 | HIHAT SOFT | Red 7 → 21 | yes | 127/64/25 |
| 39 | WHIP SNAP | Red 7 → 21 | yes | 127/64/51 |
| 40 | MUTED STR | Red 7 → 21 | yes | 0/25/25 |
| 41 | FROG CROAK | Red 7 → 21 | yes | 0/64/77 |
| 42 | GUNSHOT | Red 8 → 22 | yes | 127/0/70 |
| 43 | METAL POTS | Red 8 → 22 | yes | 45/64/70 |
| 44 | PHASER GUN | Red 8 → 22 | yes | 0/0/51 |
| 45 | LASER GUN | Red 8 → 22 | yes | 38/89/64 |
| 46 | BASS PLUCK | Green 1 → 8 | yes | 64/64/64 |
| 47 | HARMONIUM | Green 1 → 8 | yes | 64/64/64 |
| 48 | BASS DRUM | Green 1 → 8 | yes | 64/64/64 |
| 49 | MONSTER STEP | Green 1 → 8 | yes | 64/64/102 |
| 50 | CELLO SECT | Green 1 → 8 | no | 38/64/64 |
| 51 | DETUNE SYNTH | Green 1 → 8 | yes | 38/38/89 |
| 52 | GRUNGY DRONE | Green 1 → 8 | no | 38/38/89 |
| 53 | E-STR BASS | Green 2 → 9 | yes | 0/64/64 |
| 54 | PLUCK STR | Green 2 → 9 | yes | 0/64/64 |
| 55 | MARIMBA BELL | Green 2 → 9 | yes | 0/64/0 |
| 56 | MANDOLIN | Green 2 → 9 | yes | 38/0/76 |
| 57 | MUTE BELL | Green 2 → 9 | yes | 64/0/0 |
| 58 | MUTE STRING | Green 2 → 9 | yes | 64/0/127 |
| 59 | BUZZ SITAR | Green 3 → 10 | yes | 0/127/127 |
| 60 | MELODIC FROG | Green 4 → 11 | yes | 102/64/76 |
| 61 | AHH / EW | Green 4 → 11 | yes | 76/76/76 |
| 62 | TROMBONE | Green 5 → 12 | yes | 25/64/25 |
| 63 | WAH WAH | Green 5 → 12 | yes | 64/64/64 |
| 64 | PEW PEW | Green 5 → 12 | yes | 64/83/64 |
| 65 | CAT PURR | Green 5 → 12 | no | 64/38/64 |
| 66 | FLUTE | Green 5 → 12 | no | 0/45/127 |
| 67 | CLARINET | Green 5 → 12 | no | 45/38/0 |
| 68 | VOWEL MOD | Green 5 → 12 | no | 64/64/64 |
| 69 | HARM ORGAN | Green 6 → 13 | yes | 127/96/102 |
| 70 | BASS STAND | Green 6 → 13 | yes | 127/64/64 |
| 71 | WEH WAH | Green 6 → 13 | yes | 64/64/64 |
| 72 | SPACE CHAOS | Green 6 → 13 | yes | 127/64/64 |
| 73 | DOOM DRONE | Green 7 → 14 | no | 32/25/32 |
| 74 | HALLOWEEN | Green 7 → 14 | yes | 51/0/38 |
| 75 | ARPEGGIO | Green 7 → 14 | yes | 0/25/64 |
| 76 | PIANO SWEEP | Green 7 → 14 | yes | 89/25/64 |
| 77 | CHURCH ORGN | Green 7 → 14 | yes | 13/0/83 |
| 78 | CLASSIC BASS | Orange 1 → 0 | yes | 89/127/127 |
| 79 | CHEEK BASS | Orange 1 → 0 | yes | 127/25/25 |
| 80 | LOW HORNS | Orange 1 → 0 | yes | 64/83/64 |
| 81 | BASS CHOICE | Orange 1 → 0 | yes | 64/102/25 |
| 82 | VIOLIN PLUCK | Orange 2 → 1 | yes | 127/0/127 |
| 83 | PURE STRINGS | Orange 2 → 1 | yes | 127/127/0 |
| 84 | TUBA BASS | Orange 6 → 5 | yes | 64/25/25 |
| 85 | MOTOR | Orange 6 → 5 | yes | 127/64/64 |
| 86 | PULSE SAW | Orange 6 → 5 | yes | 127/0/0 |
| 87 | SLOW SAW | Orange 6 → 5 | yes | 95/127/127 |
| 88 | BASS+CHORD | Orange 7 → 6 | yes | 25/127/25 |
| 89 | HOUSE CHORD | Orange 7 → 6 | yes | 51/102/64 |
| 90 | SURPRISE ARP | Orange 7 → 6 | yes | 13/114/64 |
| 91 | ARP SET | Orange 8 → 7 | yes | 64/64/64 |

## Translation notes and limitations

- The three direct Plaits controls are imported: Harmonics, Timbre and Morph. Clock-face positions are converted to the firmware's 0–127 range, with 7 o'clock at 0, 12 o'clock at 64 and 5 o'clock at 127. Ranges are represented by their midpoint; “center”, “any”, “all” and static “modulate” entries use 64.
- AUX notes are translated to the HORATIO AUX MIX control: OUT-only/no-AUX = 0, OUT+AUX mix = 64, AUX-only = 127.
- The source's frequency ranges are playing-range guidance, not a Plaits parameter, so they are not applied as pitch limits.
- Timbre attenuator/CV, Morph attenuator/CV and FM attenuator/CV are modulation-patch instructions. HORATIO's current preset schema does not store those patch routings; their static knob values are imported, but external CV/modulation behavior is not recreated.
- “Trigger yes/no” is implemented per recipe. The 12 source recipes marked “no” leave Plaits' trigger input unpatched; all recipes marked “yes” retain normal keyboard/sequencer triggering. The factory-bank validator checks all 91 source rows against the firmware metadata. This reproduces the source patch distinction; it does not mean those patches have identical envelopes or sound to the original modular setup.
- Envelope LED counts are indicators from the original module, not ADSR values. They are not copied into unrelated HORATIO envelope parameters.
- The three “ANY 32 DX7 FM Synthesis Patches” rows are instructions to curate external patch banks, not sound settings. They are not represented as fake presets. The five rows under the source's “INCOMPLETE - FUTURE AREA” section are also excluded.
- Plaits' individual percussion models remain available as synth models. HORATIO still has no drum machine, drum track, drum kit or drum sequencer.

This is a faithful first translation of the complete recipe settings that HORATIO can currently represent, not a claim that every modular CV/trigger patch has been recreated.


## Additional six-op model selectors

The source recipe sheet does not enumerate the three DX7-style six-op banks. HORATIO adds three neutral starting presets so all eight orange-bank models are reachable from the factory browser:

| # | Preset | Model index | Patch source |
|---:|---|---:|---|
| 92 | FM BANK I | 2 | Bundled default patch data |
| 93 | FM BANK II | 3 | Bundled default patch data |
| 94 | FM BANK III | 4 | Bundled default patch data |

These are model-access presets, not named DX7 patch collections. The browser's three categories are the Plaits banks: ORNG (models 0–7), GREEN (8–15), and RED (16–23).


## Built-in six-operator FM patch banks

HORATIO exposes the 96 factory patches already bundled with the pinned Plaits v1.2 source. These are real DX7-format patch records unpacked by Plaits' six-op engine, not newly invented names or blank bank selectors.

- **DX1** uses Plaits model 2 and contains 32 bass/synth patches.
- **DX2** uses Plaits model 3 and contains 32 electric piano, keyboard, bell and percussion patches.
- **DX3** uses Plaits model 4 and contains 32 organ, pad, string and brass patches.

Each browser entry selects its own model and maps HARMONICS to one of the 32 factory patch slots. The embedded patch names are prefixed with the bank and slot number (for example, `DX2-01 EPIAN

The six-op engine's internal operator envelopes, algorithms, feedback, LFO and keyboard scaling remain intact. HORATIO's ordinary HARMONICS/TIMBRE/MORPH controls continue to affect the engine as designed; the factory preset sets a neutral starting point for TIMBRE and MORPH. No external SysEx upload mechanism is required for these built-in patches.

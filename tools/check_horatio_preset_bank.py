#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Keep HORATIO's visible factory preset browser aligned with the Plaits presets."""
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
engine = (ROOT / "firmware/src/eng_plaits.c").read_text()
ui = (ROOT / "firmware/src/ui.c").read_text()

preset_start = engine.index("static const preset_t PL_PRESETS[] = {")
preset_end = engine.index("\n};", preset_start)
preset_block = engine[preset_start:preset_end]
presets = re.findall(r'\{"([^"]+)",\s*\{\s*(\d+)\s*,[^}]*\},\s*\{\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*\}', preset_block)

bank_start = ui.index("/* One browser entry for every Plaits factory preset")
bank_end = ui.index("\n};", bank_start)
bank_block = ui[bank_start:bank_end]
bank = re.findall(r'\{(BK_[A-Z]+),\s*0,\s*"([^"]+)"\}', bank_block)

if not presets:
    raise SystemExit("No HORATIO Plaits presets found")
if len(presets) != len(bank):
    raise SystemExit(f"Factory browser has {len(bank)} entries; engine has {len(presets)} presets")
names = [row[0] for row in presets]
envelopes = [tuple(map(int, row[2:6])) for row in presets]
if any(any(value < 0 or value > 127 for value in env) for env in envelopes):
    raise SystemExit("Preset ADSR values must all be in the 0..127 range")
if len(set(envelopes)) < 8:
    raise SystemExit("Factory presets do not contain enough distinct ADSR envelopes")
if all(env == (0, 0, 0, 0) for env in envelopes):
    raise SystemExit("Factory presets have no programmed ADSR envelopes")

# Envelope checks are based on each sound's identity, not broad bass/lead/pad classes.
# The table is authored per preset; identical ADSR tuples are rejected.
percussive = re.compile(r"drip|marimba|mallet|chime|bell|wood block|snare|steel drum|xylophone|hi.hat|whip|gun|laser|pew pew|popcorn|stick|percuss|bouncing|mosquito|emergency vehicle|space ship|bubbling liquid|puddle walking|dog drinking|motor\\b|toy piano|harp|mandolin|oud|plucked|pluck|sitar|piano", re.I)
plucked = re.compile(r"bass(?!.*drone)(?!.*with chords)|guitar|piano|pluck|mandolin|oud|sitar|epiano|e\\. piano|clav|rhodes", re.I)
pads = re.compile(r"pad|string|orchestra|cello|violin|harmonium|organ|drone|church|choir|slow rise|sidechain|side.chain|chord|saw.chain|ensemble", re.I)
winds = re.compile(r"brass|horn|trumpet|trombone|sax|flute|clarinet|reed|voice|vocal", re.I)
textures = re.compile(r"noise|texture|evolving|atmosphere|ambient|windy|static|spaceship|space.ship|chaos", re.I)
# Short display names are intentionally compact; restore semantic aliases for
# sound-envelope classification so the display rename cannot change validation.
semantic_names = {
    "BOWED HARM": "Bowed string Instrument harmonics",
    "SCN PLUCK": "side-chainy string pluck (Bass - Guitar)",
    "MARIMBA BNC": "bouncing marimba mallets",
    "NOISE MAR": "noisy marimba",
    "BUBBLE LIQ": "Bubbling liquid",
    "PUDDLE DOG": "Puddle Walking / Dog Drinking",
    "CEMENT TUNL": "Walking through a cement tunnel",
    "WIND CHIME": "Wind Chimes",
    "WATER DRIP": "water dripping",
    "WATER DRIP 2": "water dripping too",
    "WATER DRIP 3": "water dripping three",
    "CPU NOISE": "Computer Thinking noise",
    "HARP SOFT": "Harp - soft",
    "HARP BUZZ": "Harp - buzzy",
    "CHURCH BELL": "Large InHarmonic Church Bells",
    "STRING PAD": "Buzzy String Pad",
    "SNARE BLOCK": "Snare-like Wood Blocks",
    "WOOD BLOCK": "Wood Blocks",
    "STRING MUTE": "String Mutes",
    "BRASS SECT": "Brass Section",
    "STAND BASS": "Standing Bass",
    "SLAP BASS": "Slap Standing Bass",
    "GLASS BELL": "Glass Bells",
    "PENCIL BELL": "Glass Bells hit with pencil",
    "LOW DRIP": "water dripping - low",
    "POP CORN": "Electronic popcorn",
    "LASER SHIP": "Space ships & Laser Beams",
    "RESO BASS": "Resonant Bass",
    "PLUCK BASS": "Standing Bass Plucked",
    "SIREN": "Emergency vehicle",
    "HIHAT SNAP": "snappy closed hi-hat",
    "HIHAT SOFT": "softer closed hi-hat",
    "WHIP SNAP": "Snap a whip",
    "MUTED STR": "Muted Strings",
    "FROG CROAK": "Frog Croak-ish",
    "METAL POTS": "trash can / pots & pans",
    "PHASER GUN": "great phaser gun",
    "LASER GUN": "phaser laser gun",
    "BASS PLUCK": "Bass - Plucky",
    "MONSTER STEP": "Monster FootSteps",
    "CELLO SECT": "Cello section",
    "DETUNE SYNTH": "De-tuned Classic Synth",
    "GRUNGY DRONE": "Deep Grungy bass drone",
    "E-STR BASS": "Electric string bass",
    "PLUCK STR": "Plucky Strings",
    "MARIMBA BELL": "Marimba / bells",
    "MANDOLIN": "Mandolin or Oud",
    "MUTE BELL": "Muted Bells",
    "MUTE STRING": "Muted Strings (Wave)",
    "BUZZ SITAR": "Buzzy Sitar",
    "MELODIC FROG": "Melodic Frog",
    "AHH / EW": "Say Ah and other Ew",
    "VOWEL MOD": "Vowel Sounds (modulate!)",
    "HARM ORGAN": "Harmonic Organ",
    "BASS STAND": "Standing Bass Too",
    "SPACE CHAOS": "SpaceShip Chaos",
    "DOOM DRONE": "Doom Bass Drone",
    "HALLOWEEN": "Halloween Organ",
    "ARPEGGIO": "Arpegiatish",
    "PIANO SWEEP": "Piano Sweep",
    "CHURCH ORGN": "Congregation Organ",
    "CLASSIC BASS": "Classic Synth Bass",
    "CHEEK BASS": "play your cheeks",
    "LOW HORNS": "low horns section",
    "BASS CHOICE": "Choice of Bass",
    "VIOLIN PLUCK": "Violin String Pluck",
    "PURE STRINGS": "Pure Electric strings",
    "TUBA BASS": "Tuba or Airy Standing Bass",
    "PULSE SAW": "pulsing sidechain saw",
    "SLOW SAW": "slow rise saw-chain",
    "BASS+CHORD": "Bass with Chords",
    "HOUSE CHORD": "sidechained rhythmic house chords",
    "SURPRISE ARP": "Your unexpected Arp",
    "ARP SET": "All the Arpeggios",
}
for display_name, env in zip(names, envelopes):
    name = semantic_names.get(display_name, display_name)
    attack, decay, sustain, release = env
    # Bundled DX7 patches use their own patch-derived envelopes; validate them
    # against the broad natural-tail limits, not recipe-name heuristics.
    if name.startswith("DX"):
        if attack > 64 or decay < 32 or release < 24:
            raise SystemExit(f"{name}: DX7 patch envelope is outside the safe playable range")
        continue
    bell_like = re.search(r"bell|chime|marimba|mallet|xylophone|steel drum|wood block|snare|hi.hat", name, re.I)
    if percussive.search(name) and (not pads.search(name) or bell_like):
        if decay < 64 or release < 44:
            raise SystemExit(f"{name}: percussive envelope is too short to avoid a click")
    elif pads.search(name) and not plucked.search(name):
        if attack < 18 or decay < 82 or sustain < 88 or release < 88:
            raise SystemExit(f"{name}: pad/string envelope is too short for a sustained sound")
    elif plucked.search(name):
        if attack > 12 or decay < 56 or release < 30:
            raise SystemExit(f"{name}: plucked/piano/bass envelope lacks a defined attack or natural tail")
    elif winds.search(name):
        if attack > 18 or decay < 68 or sustain < 70 or release < 42:
            raise SystemExit(f"{name}: wind/brass envelope does not sustain naturally")
    elif textures.search(name):
        if attack > 24 or decay < 48 or release < 40:
            raise SystemExit(f"{name}: evolving texture envelope is too short")
    elif attack > 24 or decay < 60 or release < 40:
        raise SystemExit(f"{name}: general synth envelope is outside the useful range")
if len(set(envelopes)) != len(envelopes):
    raise SystemExit("Every factory preset must have its own unique ADSR envelope")
preset_lines = [line for line in preset_block.splitlines() if line.lstrip().startswith('{"')]
fenvs = []
filter_cutoffs = []
for line in preset_lines:
    fenv_match = re.search(r'\},\s*\{[^}]*\},\s*(-?\d+),\s*\d+,\s*\{', line)
    filter_match = re.search(r'P_TFLT\s*\+\s*1,\s*(-?\d+)', line)
    if not fenv_match or not filter_match:
        raise SystemExit("Every factory preset must define filter envelope depth and filter cutoff")
    fenvs.append(int(fenv_match.group(1)))
    filter_cutoffs.append(int(filter_match.group(1)))
if any(value < -64 or value > 63 for value in fenvs + filter_cutoffs):
    raise SystemExit("Preset filter values must be in the -64..63 range")
if len(set(fenvs)) < 4 or len(set(filter_cutoffs)) < 4:
    raise SystemExit("Factory presets need varied filter envelope and cutoff settings")
if len(set(names)) != len(names):
    raise SystemExit("Plaits factory preset names must be unique for name-based browser lookup")
if [name for _, name in bank] != names:
    raise SystemExit("Factory browser names/order do not match the Plaits preset table")


# The three Plaits 1.2 six-op models each contain 32 real DX7-format factory patches.
fm_rows = re.findall(r'\{"(DX([1-3])-([0-9]{2}) [^"]+)",\s*\{\s*(\d+)\s*,\s*(\d+)', preset_block)
if len(fm_rows) != 96:
    raise SystemExit(f"Expected 96 named bundled DX patches, found {len(fm_rows)}")
for name, bank_text, slot_text, model_text, harmonics_text in fm_rows:
    bank_no, slot, model, harmonics = int(bank_text), int(slot_text), int(model_text), int(harmonics_text)
    expected_harmonics = round((slot - 1) * 127 / 31)
    if model != bank_no + 1:
        raise SystemExit(f"{name}: expected Plaits six-op model {bank_no + 1}, got {model}")
    if harmonics != expected_harmonics:
        raise SystemExit(f"{name}: expected patch selector {expected_harmonics}, got {harmonics}")
if len({row[0] for row in fm_rows}) != 96:
    raise SystemExit("DX factory patch names must be unique")

models = {int(row[1]) for row in presets}
expected = set(range(23))
if models != expected:
    raise SystemExit(f"Factory preset bank does not cover all 23 models: missing {sorted(expected - models)}")

for (name, model_text, *_env), (kind, bank_name) in zip(presets, bank):
    model = int(model_text)
    expected_kind = "BK_BASS" if model < 8 else "BK_KEYS" if model < 15 else "BK_ORGAN"
    if kind != expected_kind:
        raise SystemExit(f"{bank_name}: expected {expected_kind} for model {model}, got {kind}")

print(f"HORATIO factory browser OK: {len(bank)} unique presets cover all 23 HORATIO models with {len(set(envelopes))} ADSR shapes and {len(set(filter_cutoffs))} filter cutoff values.")

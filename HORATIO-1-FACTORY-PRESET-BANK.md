# HORATIO-1 Factory Preset Bank

**Status:** Preset-bank specification committed; not yet an installable firmware bank.

A 50-sound factory bank for the three HORATIO-1 synth tracks, plus 12 three-track performance templates. No drum engine, drum preset or drum lane is used; all four legacy drum lanes are to remain muted in these templates.

## Parameter and model translation

H/T/M mean Plaits **Harmonics / Timbre / Morph**, specified as percentages. Convert to the Plaits 0–127 range at import time (round percentage × 127 / 100, then clamp).

Target model translations:
- Virtual Analog → Virtual Analog
- Chords → Chord
- Waveshaping and Wavefolder → Waveshaping
- Two-Op FM → FM
- Additive and Harmonic Osc → Harmonics
- String (Karplus) → String
- Formant / Vowel → Speech, with formant-style settings
- Modal Resonator → Modal
- Granular Cloud → Grain
- Particle Noise → Particle
- Phase Distortion → JamesOS Phase Distortion only if that optional extension is deliberately adopted; otherwise use Waveshaping as a documented approximation.

Do not substitute Plaits' bass drum, snare or hi-hat models for any entry. Percussive character is made with pitched/synth models and envelopes. Filter, envelope, LFO and FX notes below are sound-design targets, not yet validated register values. Translate them to controls actually supported by HORATIO-1. If LFO 2, stereo sends, a specific modulation destination or a requested chord voicing is unavailable, document the closest supported sound rather than inventing controls.

## Synthwave (01–17)

| # | Name | Model | H/T/M % | Filter, envelope, modulation and FX target |
|---:|---|---|---:|---|
| 01 | Outrun Sub | Virtual Analog | 35/60/20 | LP 30%, resonance 25%; snappy filter decay 25%; driving 8th-note bass |
| 02 | Nightcall Pulse | Virtual Analog | 20/45/80 | Narrow PWM square; slow sine LFO 1 to Timbre for chorus-like width |
| 03 | Juno Brass 84 | Chord | 40/55/45 | Minor 7th voicing; slow amp attack 25%; subtle LFO 1 pitch drift |
| 04 | Miami Horizon | Waveshaping | 50/65/40 | Rich saw-fold; long release 55%; stereo reverb send |
| 05 | Turbo Arp | Virtual Analog | 30/70/35 | Short amp/filter decay 18%, zero sustain; crisp arpeggiator pluck |
| 06 | Laser Harp | FM | 65/75/30 | Bright sync-like harmonic bite; medium delay send |
| 07 | Chrome Lead | Virtual Analog | 50/80/25 | Detuned dual-saw; 5 Hz sine LFO 1 to Pitch for solo vibrato |
| 08 | Neon Poly | Chord | 60/40/50 | Major 9th chord; medium filter cutoff 45%; warm envelope |
| 09 | Kavinsky Growl | Waveshaping | 25/75/60 | Aggressive folded bass; LP cutoff 35%; high envelope depth |
| 10 | Sunset Strings | Harmonics | 45/50/65 | Slow attack 45%, long release 65%; slow LFO 2 sweep on Morph |
| 11 | Retrowave Bell | FM | 75/55/15 | Classic 80s DX-style chime; long decay 60% |
| 12 | Arcade Octaver | Virtual Analog | 50/50/10 | One-octave spread; punchy envelope for bass/lead doubling |
| 13 | Analog Tom-Bass | FM | 40/60/20 | Pitch-envelope drop on attack; hybrid percussive synth bass, not a drum voice |
| 14 | Vangelis Swell | Harmonics | 55/45/70 | Slow filter and amp swell 50%; heavy reverb for cinematic pads |
| 15 | Grid Runner | Phase Distortion* | 35/65/50 | Resonant digital saw; tight 16th-note decay |
| 16 | Tape Detune Keys | Virtual Analog | 42/38/30 | LP 40%; triangle LFO 1 gently detunes Pitch |
| 17 | Midnight Skyline | String | 50/60/75 | Bright plucked synth-guitar; stereo delay |

## Vaporwave (18–34)

| # | Name | Model | H/T/M % | Filter, envelope, modulation and FX target |
|---:|---|---|---:|---|
| 18 | Mallsoft DX EP | FM | 75/35/20 | Soft EP tine; slow random/triangle LFO to Pitch for tape warble |
| 19 | VHS Choir | Speech | 50/40/65 | Dark ooh/aah formant; slow attack 55%; LFO 2 sweeps Morph |
| 20 | Macintosh Pad | Harmonics | 45/30/70 | Filtered hazy partials; LP cutoff 38%; long release 75% |
| 21 | Seapunk Pluck | String | 60/45/55 | Glassy chorus-like pluck; medium decay and lush reverb |
| 22 | Muzak Slap Bass | Waveshaping | 25/45/35 | Rubbery mid-range fretless tone; snappy filter decay |
| 23 | Lobby Sax | Speech | 65/50/40 | Breathy lo-fi synth reed; LFO 1 vibrato on Pitch |
| 24 | Eccojam Loop | Chord | 30/35/60 | Minor chord stab; LP 35%; pitch LFO for slowed-tape sag |
| 25 | Crystal Fountain | Modal | 55/40/65 | Shimmering water-drop harmonics; long decay and delay |
| 26 | Faded Cassette | Virtual Analog | 40/25/50 | Heavily low-passed 28%; irregular LFO pitch flutter |
| 27 | Plaza Organ | Harmonics | 60/35/45 | Warm drawbar-style organ; zero attack, medium release |
| 28 | Dreamcast Menu | FM | 80/40/25 | Airy glassy bell pad; slow attack 35%; high reverb |
| 29 | Late Night Weather | Chord | 70/30/50 | Suspended 4th / 11th jazz chord; soft filter cutoff 42% |
| 30 | Poolside Marimba | Modal | 35/50/30 | Warm wooden mallet tone; short decay 28% |
| 31 | Hypnagogic Drift | Grain | 45/30/70 | Soft smeared harmonic cloud; slow LFO on Harmonics |
| 32 | Sega Sunset | FM | 50/45/35 | Warm 16-bit FM brass/pad hybrid; medium attack |
| 33 | Elevator Chime | Harmonics | 70/45/20 | Delicate high harmonics; long release 65% |
| 34 | Slowed Flute | Particle | 40/35/80 | Pitched airy-noise resonance; soft attack 30% |

## IDM & Braindance (35–50)

| # | Name | Model | H/T/M % | Filter, envelope, modulation and FX target |
|---:|---|---|---:|---|
| 35 | Clockwork Modal | Modal | 62/55/40 | Inharmonic metallic pluck; S&H LFO 1 at 1/16 to Harmonics and Morph |
| 36 | Autechre Grain | Grain | 70/60/50 | Scattered micro-grains; fast S&H LFO 2 to Timbre; conditional triggers |
| 37 | Aphex Rubber | Waveshaping | 25/70/65 | Squelchy acid bass; resonance 70%; S&H LFO modulates filter cutoff |
| 38 | Glass Wire | String | 75/80/35 | High-tension metallic string; short decay 18%; S&H on Timbre |
| 39 | Micro-Glitch Dust | Particle | 80/65/45 | Crackling pitched impulses; ultra-short amp decay 10% |
| 40 | BoC Warped Tape | Virtual Analog | 45/35/55 | Nostalgic detuned saw; slow S&H/triangle pitch drift |
| 41 | FM Metallic Ping | FM | 88/70/60 | Non-integer FM ratio; sharp transient, attack 0 and decay 15% |
| 42 | Liquid Formant | Speech | 55/75/50 | Squelchy vocal-filter sweep; fast LFO 1 modulates Morph |
| 43 | Squarepusher Sub | Waveshaping | 20/65/80 | Growling folded sub-bass; fast envelope for rapid 16th-note runs |
| 44 | Inharmonic Bell | Harmonics | 85/60/40 | Clashing upper partials; S&H LFO 1 randomizes partial tilt |
| 45 | Data Stream | Grain | 90/85/30 | High-density digital stutter texture; pair with Punch FX stutter |
| 46 | Resonant Pipe | Modal | 30/70/85 | Hollow bowed tube; slow attack 35%; LFO on Harmonics |
| 47 | Acid 303-FM | FM | 33/75/55 | Feedback-driven FM acid; high filter resonance and accented envelope |
| 48 | Fractured Chord | Chord | 65/70/40 | S&H LFO 1 to Harmonics for chord-inversion movement where supported |
| 49 | Static Geiger | Particle | 60/40/90 | Sparse resonant clicks; use sequencer probability, not a drum lane |
| 50 | Sub-Harmonic Pulse | Harmonics | 15/65/50 | Deep shifting sub-partials; slow LFO 2 modulates Timbre |

*Phase Distortion is a JamesOS extension, not one of the 16 standard Plaits 1.2 models. If HORATIO-1 remains strictly standard Plaits, preset 15 uses the documented Waveshaping approximation.

## Three-track combination trios

Each template assigns one sound to each of the three synth tracks. Keep all four drum lanes muted. Octave ranges and pattern ideas are starting points.

### Synthwave

1. **Classic Driving Outrun** — T1 [01] Outrun Sub, Oct 1–2, tight 8th-note pulse; T2 [03] Juno Brass 84, Oct 3–4, sustained chords; T3 [07] Chrome Lead, Oct 4–5, melody and stereo delay.
2. **Shimmering Retro-Arp** — T1 [09] Kavinsky Growl, Oct 1, whole/half-note roots; T2 [05] Turbo Arp, Oct 3–4, 16th-note arp; T3 [11] Retrowave Bell, Oct 5, sparse counter-melody and reverb.
3. **Dark Cyberpunk / Darksynth** — T1 [13] Analog Tom-Bass, Oct 1–2, syncopated octave jumps; T2 [15] Grid Runner, Oct 3, gated 16th-note ostinato and high resonance; T3 [14] Vangelis Swell, Oct 4–5, slow fifths/octaves and heavy reverb.
4. **Nostalgic Sunset Cruise** — T1 [02] Nightcall Pulse, Oct 1–2, steady PWM bass and LP cutoff about 35%; T2 [17] Midnight Skyline, Oct 3–4, off-beat plucks and dotted-8th delay; T3 [06] Laser Harp, Oct 4–5, expressive sync-style lead.

### Vaporwave

5. **Late-Night Mallsoft** — T1 [22] Muzak Slap Bass, Oct 1–2, syncopated fretless groove; T2 [18] Mallsoft DX EP, Oct 3–4, slow detuned 7th/9th chords; T3 [19] VHS Choir, Oct 4–5, held top-line notes, panned opposite T2.
6. **Hypnagogic Plunder-Loop** — T1 [20] Macintosh Pad, Oct 2–3, thick low-passed pad chords; T2 [21] Seapunk Pluck, Oct 4, off-beat stabs or broken arps; T3 [23] Lobby Sax, Oct 4–5, vibrato-heavy melody.
7. **Weather Channel / Smooth Jazz Fusion** — T1 [12] Arcade Octaver, Oct 1–2, LP cutoff about 30%; T2 [29] Late Night Weather, Oct 3–4, sustained sus4/11th chords; T3 [30] Poolside Marimba, Oct 4–5, syncopated tropical lead runs.
8. **Abandoned Atrium / Utopian Virtual** — T1 [27] Plaza Organ, Oct 2–3, held root-fifth pedal tones tucked back; T2 [25] Crystal Fountain, Oct 4, sparse 8th-note arp and heavy stereo delay; T3 [34] Slowed Flute, Oct 4–5, long breathy notes and high reverb.

### IDM & Braindance

9. **Clockwork Melodic Glitch** — T1 [43] Squarepusher Sub, Oct 1–2, tightly gated bass and low cutoff; T2 [35] Clockwork Modal, Oct 3–4, 16th-note melodic percussion and S&H LFO; T3 [39] Micro-Glitch Dust, Oct 5, sparse conditional triggers at 30–50% probability.
10. **Nostalgic Tape Braindance** — T1 [50] Sub-Harmonic Pulse, Oct 1, deep slowly shifting sub notes; T2 [40] BoC Warped Tape, Oct 3–4, detuned triads and slow pitch drift; T3 [48] Fractured Chord, Oct 4–5, sparse stabs and S&H chord movement.
11. **Acid Drill & Bass** — T1 [37] Aphex Rubber, Oct 2–3, 16th-note acid line, S&H cutoff and accents; T2 [41] FM Metallic Ping, Oct 5, rapid pitched FM hits at 50–70% probability; T3 [45] Data Stream, Oct 4, 1–2 events/bar at 25% probability and Punch FX stutters.
12. **Crystalline Ambient Glitch** — T1 [47] Acid 303-FM, Oct 1, LP cutoff about 20% and slow sustained sub notes; T2 [38] Glass Wire, Oct 3–4, 5- or 7-step polyrhythm and S&H Timbre; T3 [44] Inharmonic Bell, Oct 5, sparse bell tones, long release and delay/reverb.

## Performance and mix starting points

- Keep Track 1 (bass/sub) centered; pan Tracks 2 and 3 about 20–35% left/right.
- Try track lengths of 16 steps, 12 or 15 steps, and 7 or 9 steps for polyrhythmic combinations.
- For a live drop, mute Track 1 for four bars while Tracks 2 and 3 ring out, then bring it back on the downbeat.

## Implementation requirements

1. Keep all 50 names, numbers, H/T/M values and sound-design targets.
2. Implement as a factory bank separate from user-saved presets and projects.
3. Loading must not overwrite patterns, track level, pan, tuning, scale, MIDI settings or keyboard mapping.
4. Browsing should preview temporarily; explicit Load applies the sound.
5. Store actual engine IDs and supported parameter values, not only descriptive labels.
6. Provide category browsing and short screen-friendly names; retain full names in the editor/library.
7. Add tests for all 50 entries, model-ID validity, macro conversion, parameter bounds and round-trip loading.
8. Listen-test and level-match on hardware when available.
9. Keep the current SLOOP build and project compatibility until Plaits integration is complete and validated.

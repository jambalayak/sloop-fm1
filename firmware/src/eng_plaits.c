/* SPDX-License-Identifier: GPL-3.0-only
 * HORATIO-1 Plaits engine. Plaits/stmlib DSP is MIT-licensed; retain upstream notices.
 * Four global DSP slots, one paraphonic amp envelope, no drum track or kit system. */
#include "horatio_plaits.h"

static const char *const PL_MODEL_NAMES[HORATIO_PLAITS_MODELS] = {
    "VIRTUAL ANALOG", "WAVESHAPING", "FM", "GRAIN",
    "HARMONICS", "WAVETABLE", "CHORD", "SPEECH",
    "SWARM", "NOISE", "PARTICLE", "STRING",
    "MODAL", "BASS DRUM", "SNARE", "HI-HAT"
};

/* H/T/M percentages from HORATIO-1-FACTORY-PRESET-BANK.md are converted
 * to the Plaits 0..127 range. The ADSR and FX values are practical starting
 * translations of the sound-design targets, not hardware-validated claims. */
static const preset_t PL_PRESETS[] = {
    {"Outrun Sub", {0, 44, 76, 25, 32, 64, 0, 127}, {0, 32, 100, 20}, 0, 0, {0, 0, 0, 0}, {0, 0, 0, 0}, 0, {0, 0, 0, 0, 0, 0, 0, 0}},
    {"Nightcall Pulse", {0, 25, 57, 102, 48, 64, 0, 127}, {8, 48, 90, 60}, 0, 0, {0, 16, 0, 0}, {0, 0, 0, 0}, 0, {0, 0, 0, 0, 0, 0, 0, 0}},
    {"Juno Brass 84", {6, 51, 70, 57, 64, 64, 0, 127}, {32, 64, 90, 64}, 0, 0, {0, 11, 0, 11}, {0, 0, 0, 0}, 0, {0, 0, 0, 0, 0, 0, 0, 0}},
    {"Miami Horizon", {1, 64, 83, 51, 64, 64, 0, 127}, {40, 64, 90, 70}, 0, 0, {0, 0, 0, 41}, {0, 0, 0, 0}, 0, {0, 0, 0, 0, 0, 0, 0, 0}},
    {"Turbo Arp", {0, 38, 89, 44, 23, 64, 0, 127}, {0, 23, 0, 16}, 0, 0, {0, 0, 0, 0}, {0, 0, 0, 0}, 0, {0, 0, 0, 0, 0, 0, 0, 0}},
    {"Laser Harp", {2, 83, 95, 38, 50, 64, 0, 127}, {0, 50, 50, 45}, 0, 0, {0, 0, 31, 0}, {0, 0, 0, 0}, 0, {0, 0, 0, 0, 0, 0, 0, 0}},
    {"Chrome Lead", {0, 64, 102, 32, 50, 64, 0, 127}, {4, 50, 80, 40}, 0, 0, {0, 0, 0, 0}, {0, 0, 0, 0}, 0, {0, 0, 0, 0, 0, 0, 0, 0}},
    {"Neon Poly", {6, 76, 51, 64, 64, 64, 0, 127}, {16, 64, 90, 64}, 0, 0, {0, 11, 0, 11}, {0, 0, 0, 0}, 0, {0, 0, 0, 0, 0, 0, 0, 0}},
    {"Kavinsky Growl", {1, 32, 95, 76, 32, 64, 0, 127}, {0, 32, 70, 24}, 0, 0, {0, 0, 0, 0}, {0, 0, 0, 0}, 0, {0, 0, 0, 0, 0, 0, 0, 0}},
    {"Sunset Strings", {4, 57, 64, 83, 70, 64, 0, 127}, {58, 70, 90, 83}, 0, 0, {0, 0, 0, 46}, {0, 0, 0, 0}, 0, {0, 0, 0, 0, 0, 0, 0, 0}},
    {"Retrowave Bell", {2, 95, 70, 19, 76, 64, 0, 127}, {0, 76, 0, 60}, 0, 0, {0, 0, 0, 26}, {0, 0, 0, 0}, 0, {0, 0, 0, 0, 0, 0, 0, 0}},
    {"Arcade Octaver", {0, 64, 64, 13, 32, 64, 0, 127}, {0, 32, 85, 24}, 0, 0, {0, 0, 0, 0}, {0, 0, 0, 0}, 0, {0, 0, 0, 0, 0, 0, 0, 0}},
    {"Analog Tom-Bass", {2, 51, 76, 25, 25, 64, 0, 127}, {0, 25, 0, 16}, 0, 0, {0, 0, 0, 0}, {0, 0, 0, 0}, 0, {0, 0, 0, 0, 0, 0, 0, 0}},
    {"Vangelis Swell", {4, 70, 57, 89, 72, 64, 0, 127}, {64, 72, 90, 83}, 0, 0, {0, 0, 0, 61}, {0, 0, 0, 0}, 0, {0, 0, 0, 0, 0, 0, 0, 0}},
    {"Grid Runner", {1, 44, 83, 64, 20, 64, 0, 127}, {0, 20, 0, 16}, 0, 0, {0, 0, 0, 0}, {0, 0, 0, 0}, 0, {0, 0, 0, 0, 0, 0, 0, 0}},
    {"Tape Detune Keys", {0, 53, 48, 38, 60, 64, 0, 127}, {8, 60, 80, 50}, 0, 0, {0, 16, 0, 0}, {0, 0, 0, 0}, 0, {0, 0, 0, 0, 0, 0, 0, 0}},
    {"Midnight Skyline", {11, 64, 76, 95, 60, 64, 0, 127}, {0, 60, 30, 70}, 0, 0, {0, 0, 41, 21}, {0, 0, 0, 0}, 0, {0, 0, 0, 0, 0, 0, 0, 0}},
    {"Mallsoft DX EP", {2, 95, 44, 25, 70, 64, 0, 127}, {30, 70, 85, 70}, 0, 0, {0, 21, 0, 41}, {0, 0, 0, 0}, 0, {0, 0, 0, 0, 0, 0, 0, 0}},
    {"VHS Choir", {7, 64, 51, 83, 80, 64, 0, 127}, {70, 80, 90, 90}, 0, 0, {0, 0, 0, 51}, {0, 0, 0, 0}, 0, {0, 0, 0, 0, 0, 0, 0, 0}},
    {"Macintosh Pad", {4, 57, 38, 89, 80, 64, 0, 127}, {58, 80, 90, 95}, 0, 0, {0, 16, 0, 61}, {0, 0, 0, 0}, 0, {0, 0, 0, 0, 0, 0, 0, 0}},
    {"Seapunk Pluck", {11, 76, 57, 70, 60, 64, 0, 127}, {8, 60, 0, 50}, 0, 0, {0, 11, 0, 41}, {0, 0, 0, 0}, 0, {0, 0, 0, 0, 0, 0, 0, 0}},
    {"Muzak Slap Bass", {1, 32, 57, 44, 30, 64, 0, 127}, {0, 30, 50, 20}, 0, 0, {0, 0, 0, 0}, {0, 0, 0, 0}, 0, {0, 0, 0, 0, 0, 0, 0, 0}},
    {"Lobby Sax", {7, 83, 64, 51, 50, 64, 0, 127}, {8, 50, 80, 40}, 0, 0, {0, 0, 0, 16}, {0, 0, 0, 0}, 0, {0, 0, 0, 0, 0, 0, 0, 0}},
    {"Eccojam Loop", {6, 38, 44, 76, 30, 64, 0, 127}, {0, 30, 0, 30}, 0, 0, {0, 0, 0, 16}, {0, 0, 0, 0}, 0, {0, 0, 0, 0, 0, 0, 0, 0}},
    {"Crystal Fountain", {12, 70, 51, 83, 80, 64, 0, 127}, {20, 80, 40, 80}, 0, 0, {0, 0, 41, 51}, {0, 0, 0, 0}, 0, {0, 0, 0, 0, 0, 0, 0, 0}},
    {"Faded Cassette", {0, 51, 32, 64, 50, 64, 0, 127}, {10, 50, 70, 60}, 0, 0, {0, 0, 0, 16}, {0, 0, 0, 0}, 0, {0, 0, 0, 0, 0, 0, 0, 0}},
    {"Plaza Organ", {4, 76, 44, 57, 60, 64, 0, 127}, {0, 60, 100, 40}, 0, 0, {0, 11, 0, 11}, {0, 0, 0, 0}, 0, {0, 0, 0, 0, 0, 0, 0, 0}},
    {"Dreamcast Menu", {2, 102, 51, 32, 70, 64, 0, 127}, {45, 70, 80, 80}, 0, 0, {0, 0, 0, 61}, {0, 0, 0, 0}, 0, {0, 0, 0, 0, 0, 0, 0, 0}},
    {"Late Night Weather", {6, 89, 38, 64, 70, 64, 0, 127}, {25, 70, 80, 80}, 0, 0, {0, 0, 0, 36}, {0, 0, 0, 0}, 0, {0, 0, 0, 0, 0, 0, 0, 0}},
    {"Poolside Marimba", {12, 44, 64, 38, 36, 64, 0, 127}, {0, 36, 0, 25}, 0, 0, {0, 0, 0, 0}, {0, 0, 0, 0}, 0, {0, 0, 0, 0, 0, 0, 0, 0}},
    {"Hypnagogic Drift", {3, 57, 38, 89, 80, 64, 0, 127}, {40, 80, 70, 90}, 0, 0, {0, 0, 0, 31}, {0, 0, 0, 0}, 0, {0, 0, 0, 0, 0, 0, 0, 0}},
    {"Sega Sunset", {2, 64, 57, 44, 60, 64, 0, 127}, {20, 60, 80, 70}, 0, 0, {0, 0, 0, 21}, {0, 0, 0, 0}, 0, {0, 0, 0, 0, 0, 0, 0, 0}},
    {"Elevator Chime", {4, 89, 57, 25, 75, 64, 0, 127}, {0, 75, 0, 65}, 0, 0, {0, 0, 0, 51}, {0, 0, 0, 0}, 0, {0, 0, 0, 0, 0, 0, 0, 0}},
    {"Slowed Flute", {10, 51, 44, 102, 70, 64, 0, 127}, {38, 70, 60, 80}, 0, 0, {0, 0, 0, 46}, {0, 0, 0, 0}, 0, {0, 0, 0, 0, 0, 0, 0, 0}},
    {"Clockwork Modal", {12, 79, 70, 51, 40, 64, 0, 127}, {0, 40, 0, 30}, 0, 0, {0, 0, 0, 11}, {0, 0, 0, 0}, 0, {0, 0, 0, 0, 0, 0, 0, 0}},
    {"Autechre Grain", {3, 89, 76, 64, 20, 64, 0, 127}, {0, 20, 0, 16}, 0, 0, {0, 0, 0, 0}, {0, 0, 0, 0}, 0, {0, 0, 0, 0, 0, 0, 0, 0}},
    {"Aphex Rubber", {1, 32, 89, 83, 22, 64, 0, 127}, {0, 22, 20, 16}, 0, 0, {11, 0, 0, 0}, {0, 0, 0, 0}, 0, {0, 0, 0, 0, 0, 0, 0, 0}},
    {"Glass Wire", {11, 95, 102, 44, 23, 64, 0, 127}, {0, 23, 0, 20}, 0, 0, {0, 0, 36, 16}, {0, 0, 0, 0}, 0, {0, 0, 0, 0, 0, 0, 0, 0}},
    {"Micro-Glitch Dust", {10, 102, 83, 57, 13, 64, 0, 127}, {0, 13, 0, 10}, 0, 0, {0, 0, 0, 0}, {0, 0, 0, 0}, 0, {0, 0, 0, 0, 0, 0, 0, 0}},
    {"BoC Warped Tape", {0, 57, 44, 70, 60, 64, 0, 127}, {18, 60, 70, 80}, 0, 0, {0, 11, 0, 31}, {0, 0, 0, 0}, 0, {0, 0, 0, 0, 0, 0, 0, 0}},
    {"FM Metallic Ping", {2, 112, 89, 76, 20, 64, 0, 127}, {0, 20, 0, 15}, 0, 0, {0, 0, 0, 11}, {0, 0, 0, 0}, 0, {0, 0, 0, 0, 0, 0, 0, 0}},
    {"Liquid Formant", {7, 70, 95, 64, 45, 64, 0, 127}, {0, 45, 70, 30}, 0, 0, {0, 0, 0, 21}, {0, 0, 0, 0}, 0, {0, 0, 0, 0, 0, 0, 0, 0}},
    {"Squarepusher Sub", {1, 25, 83, 102, 20, 64, 0, 127}, {0, 20, 0, 15}, 0, 0, {0, 0, 0, 0}, {0, 0, 0, 0}, 0, {0, 0, 0, 0, 0, 0, 0, 0}},
    {"Inharmonic Bell", {4, 108, 76, 51, 55, 64, 0, 127}, {0, 55, 0, 55}, 0, 0, {0, 0, 0, 41}, {0, 0, 0, 0}, 0, {0, 0, 0, 0, 0, 0, 0, 0}},
    {"Data Stream", {3, 114, 108, 38, 20, 64, 0, 127}, {0, 20, 0, 12}, 0, 0, {11, 0, 21, 0}, {0, 0, 0, 0}, 0, {0, 0, 0, 0, 0, 0, 0, 0}},
    {"Resonant Pipe", {12, 38, 89, 108, 65, 64, 0, 127}, {38, 65, 50, 70}, 0, 0, {0, 0, 0, 36}, {0, 0, 0, 0}, 0, {0, 0, 0, 0, 0, 0, 0, 0}},
    {"Acid 303-FM", {2, 42, 95, 70, 25, 64, 0, 127}, {0, 25, 40, 20}, 0, 0, {0, 0, 0, 11}, {0, 0, 0, 0}, 0, {0, 0, 0, 0, 0, 0, 0, 0}},
    {"Fractured Chord", {6, 83, 89, 51, 45, 64, 0, 127}, {0, 45, 0, 35}, 0, 0, {0, 0, 31, 21}, {0, 0, 0, 0}, 0, {0, 0, 0, 0, 0, 0, 0, 0}},
    {"Static Geiger", {10, 76, 51, 114, 12, 64, 0, 127}, {0, 12, 0, 10}, 0, 0, {0, 0, 11, 16}, {0, 0, 0, 0}, 0, {0, 0, 0, 0, 0, 0, 0, 0}},
    {"Sub-Harmonic Pulse", {4, 19, 83, 64, 80, 64, 0, 127}, {20, 80, 60, 80}, 0, 0, {0, 0, 0, 41}, {0, 0, 0, 0}, 0, {0, 0, 0, 0, 0, 0, 0, 0}},
};

static uint16_t plaits_q15(int32_t value) {
    value = clamp(value, 0, 127);
    return (uint16_t)(((uint32_t)value * 32767u + 63u) / 127u);
}

static int plaits_slot_used(uint32_t slot, const voice_t *except) {
    uint32_t p, i;
    int32_t id = (int32_t)(slot + 1u);
    for (p = 0; p < NPART; ++p) {
        for (i = 0; i < NVOICE; ++i) {
            const voice_t *v = &trk[p].v[i];
            if (v != except && v->active && v->s[7] == id)
                return 1;
        }
    }
    return 0;
}

static void plaits_note_on(track_t *t, voice_t *v) {
    uint32_t slot, own = (uint32_t)v->s[7];
    (void)t;
    if (own >= 1u && own <= HORATIO_PLAITS_VOICES &&
        !plaits_slot_used(own - 1u, v)) {
        slot = own - 1u;
    } else {
        for (slot = 0; slot < HORATIO_PLAITS_VOICES; ++slot)
            if (!plaits_slot_used(slot, v))
                break;
    }
    if (slot >= HORATIO_PLAITS_VOICES) {
        v->s[7] = 0;
        return;
    }
    v->s[7] = (int32_t)(slot + 1u);
    horatio_plaits_note_on(slot);
}

static int plaits_done(track_t *t, voice_t *v) {
    uint32_t id = (uint32_t)v->s[7];
    (void)t;
    if (v->stage == 4u)
        return 0; /* keep the SLOOP click-free voice-steal fade */
    if (id < 1u || id > HORATIO_PLAITS_VOICES)
        return 1;
    if (!horatio_plaits_shared_done())
        return 0;
    horatio_plaits_all_off(id - 1u);
    v->s[7] = 0;
    return 1;
}

static int32_t plaits_amp(track_t *t, voice_t *v, int32_t adsr) {
    (void)t;
    return v->stage == 4u ? adsr : horatio_plaits_shared_amp();
}

static void plaits_render(track_t *t, voice_t *v, int32_t *out,
                          uint32_t n, const vmod_t *m) {
    uint32_t id = (uint32_t)v->s[7], offset = 0;
    const int16_t *p = t->p;
    int32_t pitch_q7;
    uint16_t level, vca;
    int16_t samples[32];
    if (id < 1u || id > HORATIO_PLAITS_VOICES)
        return;
    pitch_q7 = m->pitch16 * 8 + m->fine / 32;
    vca = plaits_q15(p[P_E7]);
    level = (uint16_t)(((uint32_t)v->vel * 258u * vca + 16383u) / 32767u);
    horatio_plaits_configure(id - 1u, (uint32_t)clamp(p[P_E0], 0, 15),
        pitch_q7, plaits_q15(p[P_E1]), plaits_q15(p[P_E2]),
        plaits_q15(p[P_E3]), plaits_q15(p[P_E4]), plaits_q15(p[P_E5]),
        plaits_q15(p[P_E6]), level);
    while (offset < n) {
        uint32_t count = n - offset, i;
        if (count > 32u)
            count = 32u;
        horatio_plaits_render(id - 1u, samples, count);
        for (i = 0; i < count; ++i)
            out[offset + i] += voice_amp(samples[i], m, offset + i);
        offset += count;
    }
}

static const engine_t ENG_PLAITS = {
    "PLT", {"MODEL", "SHAPE"},
    {
        {"MODEL", F_INT, 0, 15, 0, PL_MODEL_NAMES, 0},
        {"HARM", F_PCT, 0, 127, 64, 0, 0},
        {"TIMBRE", F_PCT, 0, 127, 64, 0, 0},
        {"MORPH", F_PCT, 0, 127, 64, 0, 0},
        {"DECAY", F_PCT, 0, 127, 64, 0, 0},
        {"LPG", F_PCT, 0, 127, 64, 0, 0},
        {"AUX MIX", F_PCT, 0, 127, 0, 0, 0},
        {"VCA", F_PCT, 0, 127, 127, 0, 0},
    },
    PL_PRESETS, NELEM(PL_PRESETS), -1, plaits_note_on, plaits_render,
    0xF81F, {P_E1, P_E2, P_E3, P_E4}, 4, plaits_amp, 0, 0,
    .ownenv = 1, .done = plaits_done,
};

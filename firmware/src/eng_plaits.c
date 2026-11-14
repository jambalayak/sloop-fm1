/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 the SLOOP PLAITS port; SLOOP is isod89's, Felucca Leo Kuroshita's (see LICENSING.md) */
/* PLAITS: eleven macro-oscillator models in the manner of Mutable Instruments Plaits (Emilie Gillet, MIT licence,
 * https://github.com/pichenettes/eurorack, directory plaits; the notice is in LICENSES/MIT-plaits.txt).
 *
 * Plaits itself is 32-bit floating point at 48 kHz; the FM-1's processor has no FPU and SLOOP's DSP is whole-number
 * arithmetic, so the models here are NOT Plaits' code: each one is written again in fixed point from its idea and its
 * three controls (HARMONICS, TIMBRE, MORPH), and is voiced after the original, not bit for bit. The models that
 * need Plaits' wavetables, speech data or float resonator banks (WAVETABLE, SPEECH, MODAL, STRING, GRAIN-FORMANT, the
 * six-operator FM, the wave terrain ...) are not here: SLOOP's own SAMPLE, FORMANT, PHYS, GRAIN and FM6 engines
 * cover them.
 *
 * MODEL (EDIT 1): VA, WSHAPE, FM, ADDITIVE, CHORD, NOISE, SWARM, KICK, SNARE, HAT, PARTICLE. HARM / TIMB / MORPH
 * (E1..E3) change their name and meaning with the model (px_desc). LPG (E4): a low-pass gate, the tone closes as the
 * note fades. SUB (E5): a sine an octave below (the pitched models). DRIVE (E6): a soft clip after everything.
 * The track's FLT moves TIMB, its SHP moves MORPH.
 *
 * State (v->s[], v->ph[]), by model:
 *   all     s[0] the LPG's pole, s[1] the random generator
 *   pitched s[6] the SUB's phase (KICK, SNARE, HAT, PARTICLE have no SUB)
 *   VA      ph[0] ph[1]            WSHAPE  ph[0]              ADDITIVE ph[0]
 *   FM      ph[0] ph[1], s[2] s[3] the modulator's feedback   CHORD   ph[0..2], s[2]
 *   NOISE   s[2] s[3] and s[4] s[5] the two filters           SWARM   ph[0..2], s[2] s[3], s[4] the flutter's phase
 *   KICK    ph[0], s[2] the pitch, s[3] the click, s[4] the amplitude (Q15 envelopes)
 *   SNARE   ph[0] ph[1], s[2] the body, s[3] the snares, s[4] s[5] the filter
 *   HAT     ph[0..2], s[2..4] the six squares' phases, s[5] s[6] the filter, s[7] the envelope
 *   PARTICLE s[2] s[3] the filter
 * px_note_on sets them up for the model the part plays; a model changed under a sounding note only plays something
 * odd until the note ends (every state is a whole number that wraps or is clamped). */
enum { PX_VA, PX_WSHP, PX_FM, PX_ADD, PX_CHORD, PX_NOISE, PX_SWARM, PX_KICK, PX_SNARE, PX_HAT, PX_PART, PX_COUNT };
static const char *const N_PX_MODEL[] = {"VA", "WSHAPE", "FM", "ADDITIVE", "CHORD", "NOISE", "SWARM", "KICK",
                                         "SNARE", "HAT", "PARTICLE"};
/* the three controls' names per model */
#define PXD(a, b, c) {{a, F_PCT, 0, 127, 64, 0, 0}, {b, F_PCT, 0, 127, 64, 0, 0}, {c, F_PCT, 0, 127, 64, 0, 0}}
static const param_desc_t PX_DESC[PX_COUNT][3] = {
    PXD("DETUN", "PWIDTH", "SHAPE"), PXD("WAVE", "FOLD", "ASYM"),   PXD("RATIO", "INDEX", "FDBK"),
    PXD("CENTRE", "SLOPE", "EVEN"),  PXD("TYPE", "INVERS", "WAVE"), PXD("FREQ", "RESO", "SPREAD"),
    PXD("SPREAD", "FLUTTR", "WAVE"), PXD("PUNCH", "DECAY", "CLICK"), PXD("TONE", "SNAP", "DECAY"),
    PXD("PITCH", "COLOUR", "DECAY"), PXD("DENS", "SPREAD", "RESO"),
};
static const param_desc_t *px_desc(const track_t *t, uint32_t k)
{
    return k >= 1u && k <= 3u ? &PX_DESC[(uint32_t)t->p[P_E0] % PX_COUNT][k - 1u] : 0;
}

/* 2^(n / 12) in Q8, n = 0..48 */
static const uint16_t PX_SEMI_Q8[49] = {
    256,  271,  287,  304,  323,  342,  362,  384,  406,  431,  456,  483,  512,  542,  575,  609,  645,
    683,  724,  767,  813,  861,  912,  967,  1024, 1085, 1149, 1218, 1290, 1367, 1448, 1534, 1625, 1722,
    1825, 1933, 2048, 2170, 2299, 2435, 2580, 2734, 2896, 3069, 3251, 3444, 3649, 3866, 4096};
static inline uint32_t px_semi(uint32_t inc, int32_t st) { return fm_ratio_inc(inc, PX_SEMI_Q8[clamp(st, 0, 48)]); }
/* FM: the modulator's ratio, 16 steps (Q8) */
static const uint16_t PX_FMR[16] = {64, 128, 192, 256, 320, 384, 512, 640, 768, 896, 1024, 1280, 1536, 1792, 2048, 3072};
/* CHORD: eleven chords of four notes (semitones above the key): octaves, fifth, sus4, minor, major, min7, maj7,
 * min9, maj9, sus2, diminished */
static const int8_t PX_CHORDS[11][4] = {{0, 12, 0, 12}, {0, 7, 12, 7},  {0, 5, 7, 12},  {0, 3, 7, 12},
                                       {0, 4, 7, 12},  {0, 3, 7, 10},  {0, 4, 7, 11},  {0, 3, 10, 14},
                                       {0, 4, 11, 14}, {0, 2, 7, 12},  {0, 3, 6, 9}};
/* HAT: the six square oscillators' ratios (Q8; after the 808's 205 / 304 / 370 / 523 / 540 / 800 Hz) */
static const uint16_t PX_HATR[6] = {256, 380, 461, 652, 674, 998};
/* VA: the second oscillator's interval above the first (HARM 64..127, eight steps) */
static const int8_t PX_VAIV[8] = {3, 4, 5, 7, 10, 12, 19, 24};

static inline int32_t px_fold(int32_t x)        /* a triangle fold into +-32768 */
{
    int32_t w = (x + 32768) & 0x1FFFF;
    if (w >= 65536)
        w = 131071 - w;
    return w - 32768;
}
static inline int32_t px_ramp(int32_t e0, int32_t e1, uint32_t i) { return e0 + (((e1 - e0) * (int32_t)i) >> CTL_LOG2); }
static inline int32_t px_sq(uint32_t ph) { return (ph >> 31) ? 4000 : -4000; }

static void px_note_on(track_t *t, voice_t *v)
{
    uint32_t seed = (v->note + 1u) * 0x9E3779B9u ^ (uint32_t)v->fine * 0x85EBCA6Bu;   /* UNISON: each its own */
    uint32_t md = (uint32_t)t->p[P_E0] % PX_COUNT, i;
    for (i = 0; i < 8u; i++)
        v->s[i] = 0;
    v->ph[0] = v->ph[1] = v->ph[2] = 0;
    v->s[1] = (int32_t)(seed ? seed : 1u);
    if (md == PX_KICK)
        v->s[2] = v->s[3] = v->s[4] = 32767;
    else if (md == PX_SNARE)
        v->s[2] = v->s[3] = 32767;
    else if (md == PX_HAT)
        v->s[7] = 32767;
}

static void px_render(track_t *t, voice_t *v, int32_t *out, uint32_t n, const vmod_t *m)
{
    const int16_t *p = t->p;
    uint32_t md = (uint32_t)p[P_E0] % PX_COUNT, i, inc = m->inc;
    int32_t h = p[P_E1], lpg = p[P_E4], subg = p[P_E5] * 258, drv = p[P_E6], dg = 32768 + drv * 512, lc = 32767;
    int32_t hq = h << 8, tq = clamp((p[P_E2] << 8) + m->cutoff, 0, 127 << 8),
            mq = clamp((p[P_E3] << 8) + m->shape - (64 << 8), 0, 127 << 8);    /* Q8: 0 .. 127 << 8 */
    int32_t S[8];                                  /* the state in locals: out[] may alias v->s[] */
    uint32_t P[3], inc2 = 0, inc3 = 0, inc4 = 0, inc5 = 0, inc6 = 0, thr = 0;
    uint32_t pinc[4] = {0, 0, 0, 0};
    tsvf_t c1 = {0, 0, 0}, c2 = {0, 0, 0};
    int32_t pw = 0, gfold = 0, off = 0, fbq = 0, idx = 0, sp = 0, fl = 0, kl = 0, na = 0, norm = 32767;
    int32_t am[8] = {0}, pn[8] = {0}, e0 = 0, e1 = 0, e2 = 0, e3 = 0, e4 = 0, e5 = 0;
    for (i = 0; i < 8u; i++)
        S[i] = v->s[i];
    P[0] = v->ph[0];
    P[1] = v->ph[1];
    P[2] = v->ph[2];

    /* ---- per block: the coefficients and the control-rate envelopes of the model ---- */
    switch (md) {
    case PX_VA:
        if (h < 64)
            inc2 = inc + (uint32_t)(((uint64_t)inc * (uint32_t)((h * h) >> 1)) >> 16);   /* up to 3 % (50 cents) */
        else
            inc2 = px_semi(inc, PX_VAIV[(h - 64) >> 3]);
        pw = (int32_t)(0x80000000u - (uint32_t)tq * 3450u);                              /* 50 % down to ~8 % */
        break;
    case PX_WSHP:
        gfold = 4096 + ((tq * 29) >> 5);                                                 /* 1x .. 8x */
        off = ((mq >> 8) - 64) * 400;
        break;
    case PX_FM:
        inc2 = fm_ratio_inc(inc, PX_FMR[(h * 16) >> 7]);
        idx = tq >> 8;
        fbq = mq >> 9;
        break;
    case PX_ADD: {
        int32_t k, c = 256 + ((h * 1792) >> 7), width = 200 + (tq >> 4), recip = (32767 * 256) / width, tot = 0;
        for (k = 0; k < 8; k++) {
            int32_t d = ((k + 1) << 8) - c, a;
            if (d < 0)
                d = -d;
            a = d >= width ? 0 : 32767 - ((d * recip) >> 8);
            if (k & 1)
                a = (a * (32767 - mq)) >> 15;                                            /* EVEN: they fade out */
            if (a > 0 && inc <= 0x73000000u / (uint32_t)(k + 1)) {
                am[na] = a;
                pn[na] = k + 1;
                tot += a;
                na++;
            }
        }
        norm = tot > 32767 ? (int32_t)(((uint32_t)32767 * 32767u) / (uint32_t)tot) : 32767;
        break;
    }
    case PX_CHORD: {
        int32_t type = (h * 11) >> 7, inv = (tq * 4) >> 15, j;
        for (j = 0; j < 4; j++)
            pinc[j] = px_semi(inc, PX_CHORDS[type][j] + (j < inv ? 12 : 0));
        break;
    }
    case PX_NOISE: {
        int32_t cut = (h << 8) + ((m->pitch16 - 960) * 19) - (64 << 8) + 11238;      /* HARM 64: the key's pitch */
        tsvf_coef(&c1, cut, tq >> 8);
        sp = mq >> 8;
        if (sp)
            tsvf_coef(&c2, cut + (sp << 7), (tq >> 8) >> 1);
        break;
    }
    case PX_SWARM:
        sp = (h * 5) >> 5;
        fl = tq >> 8;
        inc2 = fm_ratio_inc(inc, (uint32_t)(256 - 2 * sp));
        inc3 = fm_ratio_inc(inc, (uint32_t)(256 - sp));
        inc4 = inc;
        inc5 = fm_ratio_inc(inc, (uint32_t)(256 + sp));
        inc6 = fm_ratio_inc(inc, (uint32_t)(256 + 2 * sp));
        break;
    case PX_KICK:
        e0 = S[2];
        e1 = e0 - mulq16(e0, ENV_EXP[56]);                                               /* the pitch falls */
        inc2 = fm_ratio_inc(inc, (uint32_t)(256 + ((e0 * h) >> 15) * 14));               /* up to 8x at the start */
        e2 = S[3];
        e3 = e2 - mulq16(e2, ENV_EXP[10]);                                               /* the click: ~2 ms */
        e4 = S[4];
        e5 = e4 - mulq16(e4, ENV_EXP[70 + (tq >> 8) / 3]);                               /* the amplitude: DECAY */
        kl = mq >> 8;
        break;
    case PX_SNARE:
        e0 = S[2];
        e1 = e0 - mulq16(e0, ENV_EXP[66]);                                               /* the body: ~20 ms */
        e2 = S[3];
        e3 = e2 - mulq16(e2, ENV_EXP[60 + (((mq >> 8) * 58) >> 7)]);                     /* the snares: DECAY */
        inc2 = fm_ratio_inc(inc, 410u);
        tsvf_coef(&c1, 10240 + ((tq * 17) >> 5), 10);
        break;
    case PX_HAT:
        e0 = S[7];
        e1 = e0 - mulq16(e0, ENV_EXP[clamp(40 + (((mq >> 8) * 89) >> 7), 0, 127)]);
        pinc[0] = fm_ratio_inc(inc, ((uint32_t)PX_HATR[0] * (uint32_t)(96 + h)) >> 7);
        inc2 = fm_ratio_inc(inc, ((uint32_t)PX_HATR[1] * (uint32_t)(96 + h)) >> 7);
        inc3 = fm_ratio_inc(inc, ((uint32_t)PX_HATR[2] * (uint32_t)(96 + h)) >> 7);
        inc4 = fm_ratio_inc(inc, ((uint32_t)PX_HATR[3] * (uint32_t)(96 + h)) >> 7);
        inc5 = fm_ratio_inc(inc, ((uint32_t)PX_HATR[4] * (uint32_t)(96 + h)) >> 7);
        inc6 = fm_ratio_inc(inc, ((uint32_t)PX_HATR[5] * (uint32_t)(96 + h)) >> 7);
        tsvf_coef(&c1, 15360 + ((tq * 13) >> 5), 20);
        break;
    case PX_PART: {
        int32_t rnd = (int32_t)(noise32(&S[1]) >> 20) - 2048;                            /* +-2048 */
        int32_t cut = 11238 + ((m->pitch16 - 960) * 19) + ((rnd * (tq >> 8)) >> 6);      /* SPREAD: up to +-1.5 oct */
        thr = 200000u + (uint32_t)(h * h * h) * 420u;                                    /* impulses a sample */
        tsvf_coef(&c1, cut, 64 + (mq >> 9));
        break;
    }
    }
    if (lpg) {                                                  /* the low-pass gate follows the note's level */
        int32_t tg = 1500 + ((v->env_out * 31000) >> 15);
        lc = 32767 - (((32767 - tg) * lpg) >> 7);
    }

    for (i = 0; i < n; i++) {
        int32_t s = 0;
        switch (md) {
        case PX_VA: {
            int32_t a = osc_pulse(P[0], inc, (uint32_t)pw), saw = osc_saw(P[1], inc2), tri = osc_tri(P[1]);
            s = (a + saw + (((tri - saw) * mq) >> 15)) >> 1;
            P[0] += inc;
            P[1] += inc2;
            break;
        }
        case PX_WSHP: {
            int32_t w = (h & 63) * 512, sine = sine_i(P[0]), tri = osc_tri(P[0]), saw = (int32_t)(P[0] >> 16) - 32768;
            int32_t src = h < 64 ? sine + (((tri - sine) * w) >> 15) : tri + (((saw - tri) * w) >> 15);
            s = px_fold(((src * gfold) >> 12) + off);
            P[0] += inc;
            break;
        }
        case PX_FM: {
            int32_t om = sine_i(P[1] + digital_mod((S[2] + S[3]) >> 1, fbq));
            S[3] = S[2];
            S[2] = om;
            s = sine_i(P[0] + digital_mod(om, idx));
            P[0] += inc;
            P[1] += inc2;
            break;
        }
        case PX_ADD: {
            int32_t k, acc = 0;
            for (k = 0; k < na; k++)
                acc += (sine_i(P[0] * (uint32_t)pn[k]) * am[k]) >> 15;
            s = (acc * norm) >> 15;
            P[0] += inc;
            break;
        }
        case PX_CHORD: {
            int32_t sum = 0, j;
            uint32_t *php[4] = {&P[0], &P[1], &P[2], (uint32_t *)&S[2]};
            for (j = 0; j < 4; j++) {
                int32_t sq = osc_pulse(*php[j], pinc[j], 0x80000000u), saw = osc_saw(*php[j], pinc[j]);
                sum += sq + (((saw - sq) * mq) >> 15);
                *php[j] += pinc[j];
            }
            s = sum >> 1;
            break;
        }
        case PX_NOISE: {
            int32_t x = (int32_t)noise32(&S[1]) >> 17, y = tsvf_lp(&c1, x, &S[2], &S[3]);
            if (sp)
                y -= tsvf_lp(&c2, x, &S[4], &S[5]);
            s = clamp(y * 8, -32767, 32767);
            break;
        }
        case PX_SWARM: {
            int32_t sum = 0, j;
            uint32_t *php[5] = {&P[0], &P[1], &P[2], (uint32_t *)&S[2], (uint32_t *)&S[3]};
            const uint32_t ip[5] = {inc2, inc3, inc4, inc5, inc6};
            for (j = 0; j < 5; j++) {
                int32_t saw = osc_saw(*php[j], ip[j]), sin = sine_i(*php[j]), w = saw + (((sin - saw) * mq) >> 15);
                int32_t a = 32767 - ((fl * (osc_tri((uint32_t)S[4] * (uint32_t)(j + 2)) + 32768)) >> 8);
                sum += (w * a) >> 15;
                *php[j] += ip[j];
            }
            S[4] = (int32_t)((uint32_t)S[4] + 0x00060000u);
            s = sum >> 1;
            break;
        }
        case PX_KICK: {
            int32_t tone = sine_i(P[0]), ea = px_ramp(e4, e5, i), ec = px_ramp(e2, e3, i), click, y;
            click = ((((int32_t)noise32(&S[1]) >> 17) * ec >> 15) * kl) >> 6;
            y = ((tone + click) * ea) >> 15;
            s = softclip((y * (32768 + 2 * mq)) >> 15);
            P[0] += inc2;                                                    /* the swept pitch (per block) */
            break;
        }
        case PX_SNARE: {
            int32_t eb = px_ramp(e0, e1, i), en = px_ramp(e2, e3, i), x = (int32_t)noise32(&S[1]) >> 17;
            int32_t body = (sine_i(P[0]) + sine_i(P[1])) >> 1, hp = x - tsvf_lp(&c1, x, &S[4], &S[5]);
            s = clamp(((((body * eb) >> 15) * (32767 - hq)) >> 15) + ((((hp * 3 * en) >> 15) * hq) >> 15), -32767, 32767);
            P[0] += inc;
            P[1] += inc2;
            break;
        }
        case PX_HAT: {
            uint32_t *php[6] = {&P[0], &P[1], &P[2], (uint32_t *)&S[2], (uint32_t *)&S[3], (uint32_t *)&S[4]};
            const uint32_t ip[6] = {pinc[0], inc2, inc3, inc4, inc5, inc6};
            int32_t sum = 0, j, x, hp, ev = px_ramp(e0, e1, i);
            for (j = 0; j < 6; j++) {
                sum += px_sq(*php[j]);
                *php[j] += ip[j];
            }
            x = sum + ((int32_t)noise32(&S[1]) >> 20);
            hp = x - tsvf_lp(&c1, x, &S[5], &S[6]);
            s = clamp(((hp * ev) >> 15) * 2, -32767, 32767);
            break;
        }
        case PX_PART: {
            int32_t x = 0;
            if (noise32(&S[1]) < thr)
                x = (int32_t)noise32(&S[1]) >> 15;
            s = clamp(tsvf_lp(&c1, x, &S[2], &S[3]) * 8, -32767, 32767);
            break;
        }
        }
        if (md < PX_KICK) {
            s += mulq15(sine_i((uint32_t)S[6]), subg);
            S[6] = (int32_t)((uint32_t)S[6] + (inc >> 1));
        }
        if (drv)
            s = softclip((s * dg) >> 15);
        s = clamp(s, -32767, 32767);
        if (lpg) {
            S[0] += ((s - S[0]) * lc) >> 15;
            s = S[0];
        }
        out[i] += voice_amp(s, m, i);
    }

    /* the block's envelopes move on; the state goes back */
    if (md == PX_KICK) {
        S[2] = e1;
        S[3] = e3;
        S[4] = e5;
    } else if (md == PX_SNARE) {
        S[2] = e1;
        S[3] = e3;
    } else if (md == PX_HAT) {
        S[7] = e1;
    }
    for (i = 0; i < 8u; i++)
        v->s[i] = S[i];
    v->ph[0] = P[0];
    v->ph[1] = P[1];
    v->ph[2] = P[2];
}

/* The presets: one or two voicings of each model. E: MODEL HARM TIMB MORPH LPG SUB DRIVE - ; the drums' own envelopes
 * shape them (the ADSR is held open: SUS 127), the other models use the ADSR. */
static const preset_t PLAITS_PRESETS[] = {
    {"VA LEAD", {PX_VA, 18, 30, 20, 40, 30, 0, 0}, {2, 80, 100, 40}, 0, 1, FX(0, 20, 18, 22)},
    {"VA DETUNE", {PX_VA, 26, 10, 60, 0, 0, 0, 0}, {4, 90, 110, 60}, 0, 0, FX(0, 40, 14, 26)},
    {"VA FIFTH", {PX_VA, 90, 25, 40, 30, 0, 0, 0}, {2, 85, 100, 50}, 0, 0, FX(0, 24, 18, 26)},
    {"VA PWM BASS", {PX_VA, 3, 80, 10, 60, 60, 0, 0}, {0, 70, 90, 30}, 0, 1, FX(0, 0, 0, 6), XP(P_TRANS + 1, -12)},
    {"FOLD KEYS", {PX_WSHP, 20, 60, 70, 70, 0, 0, 0}, {0, 80, 50, 50}, 0, 0, FX(0, 20, 16, 26)},
    {"FOLD BASS", {PX_WSHP, 40, 90, 64, 0, 70, 20, 0}, {0, 70, 80, 30}, 0, 1, FX(0, 0, 0, 6), XP(P_TRANS + 1, -12)},
    {"FOLD LEAD", {PX_WSHP, 90, 80, 30, 0, 0, 10, 0}, {2, 80, 100, 40}, 0, 1, FX(0, 20, 20, 26)},
    {"FM BELL", {PX_FM, 76, 60, 0, 0, 0, 0, 0}, {0, 95, 0, 80}, 0, 0, FX(0, 10, 26, 44)},
    {"FM EP", {PX_FM, 28, 40, 10, 0, 0, 0, 0}, {0, 85, 40, 55}, 0, 0, FX(0, 30, 16, 30)},
    {"FM SUB", {PX_FM, 12, 50, 40, 0, 40, 0, 0}, {0, 60, 70, 25}, 0, 1, FX(0, 0, 0, 6), XP(P_TRANS + 1, -12)},
    {"FM METAL", {PX_FM, 108, 90, 70, 0, 0, 0, 0}, {0, 90, 20, 70}, 0, 0, FX(10, 10, 26, 40)},
    {"ADD ORGAN", {PX_ADD, 64, 100, 0, 0, 0, 0, 0}, {0, 80, 120, 30}, 0, 0, FX(0, 30, 10, 24)},
    {"ADD BRIGHT", {PX_ADD, 100, 60, 40, 0, 0, 0, 0}, {0, 85, 100, 40}, 0, 0, FX(0, 20, 16, 30)},
    {"ADD HOLLOW", {PX_ADD, 40, 50, 127, 0, 0, 0, 0}, {10, 90, 110, 60}, 0, 0, FX(0, 30, 20, 40)},
    {"CH MIN7", {PX_CHORD, 64, 30, 60, 40, 0, 0, 0}, {2, 85, 90, 50}, 0, 0, FX(0, 30, 16, 30)},
    {"CH MAJ7", {PX_CHORD, 76, 30, 60, 40, 0, 0, 0}, {2, 85, 90, 50}, 0, 0, FX(0, 30, 16, 30)},
    {"CH STAB", {PX_CHORD, 41, 64, 100, 80, 0, 0, 0}, {0, 60, 0, 35}, 0, 0, FX(10, 14, 20, 24)},
    {"CH PAD", {PX_CHORD, 52, 90, 40, 0, 0, 0, 0}, {70, 90, 110, 90}, 0, 0, FX(0, 50, 24, 56)},
    {"NZ BREATH", {PX_NOISE, 70, 20, 40, 0, 0, 0, 0}, {40, 90, 100, 80}, 0, 0, FX(0, 10, 14, 40)},
    {"NZ WHISTLE", {PX_NOISE, 64, 120, 10, 0, 0, 0, 0}, {10, 85, 100, 70}, 0, 0, FX(0, 10, 20, 44)},
    {"SWARM", {PX_SWARM, 40, 50, 30, 0, 0, 0, 0}, {50, 90, 100, 90}, 0, 0, FX(0, 40, 20, 50)},
    {"SWARM LEAD", {PX_SWARM, 15, 70, 60, 20, 20, 0, 0}, {4, 85, 100, 50}, 0, 1, FX(0, 30, 20, 30)},
    {"KICK 808", {PX_KICK, 90, 50, 20, 0, 0, 0, 0}, {0, 127, 127, 20}, 0, 0, FX(0, 0, 0, 0), XP(P_TRANS + 1, -24)},
    {"KICK PUNCH", {PX_KICK, 120, 20, 70, 0, 0, 0, 0}, {0, 127, 127, 20}, 0, 0, FX(0, 0, 0, 0), XP(P_TRANS + 1, -24)},
    {"SNARE", {PX_SNARE, 60, 80, 60, 0, 0, 0, 0}, {0, 127, 127, 20}, 0, 0, FX(0, 0, 10, 30)},
    {"SNARE SNAP", {PX_SNARE, 90, 100, 30, 0, 0, 0, 0}, {0, 127, 127, 20}, 0, 0, FX(0, 0, 10, 24)},
    {"HAT CLOSED", {PX_HAT, 64, 70, 20, 0, 0, 0, 0}, {0, 127, 127, 20}, 0, 0, FX(0, 0, 0, 12)},
    {"HAT OPEN", {PX_HAT, 64, 70, 100, 0, 0, 0, 0}, {0, 127, 127, 40}, 0, 0, FX(0, 0, 6, 20)},
    {"RAIN DROPS", {PX_PART, 40, 40, 90, 0, 0, 0, 0}, {0, 90, 127, 60}, 0, 0, FX(0, 0, 30, 50)},
    {"PART DUST", {PX_PART, 20, 20, 110, 0, 0, 0, 0}, {0, 90, 127, 60}, 0, 0, FX(0, 0, 24, 40)},
};

static const engine_t ENG_PLAITS = {
    "PLAITS", {"MODEL", "TONE"},
    {
        {"MODEL", F_ENUM, 0, PX_COUNT - 1, PX_VA, N_PX_MODEL, 0},
        {"HARM", F_PCT, 0, 127, 64, 0, 0},
        {"TIMB", F_PCT, 0, 127, 64, 0, 0},
        {"MORPH", F_PCT, 0, 127, 64, 0, 0},
        {"LPG", F_PCT, 0, 127, 0, 0, 0},
        {"SUB", F_PCT, 0, 127, 0, 0, 0},
        {"DRIVE", F_PCT, 0, 127, 0, 0, 0},
        {"-", F_INT, 0, 0, 0, 0, 0},
    },
    PLAITS_PRESETS, sizeof(PLAITS_PRESETS) / sizeof(PLAITS_PRESETS[0]), -1, px_note_on, px_render,
    0xFB60, {P_E1, P_E2, P_E3, P_E4}, 4, .desc = px_desc,
};

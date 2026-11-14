/* SPDX-License-Identifier: GPL-3.0-only */
/* PLAITS (eng_plaits.c): every preset renders sound that is in tune, is not silent, and does not clip.
 * Pitched models (VA, WSHAPE, FM, ADDITIVE, CHORD): the fundamental of notes 36 .. 96 every 6 (velocity 100, dry, no
 * vibrato) from the phase advance of its partial between two windows 0.25 s apart, as tests/phys_tune_test.c: within
 * TOL cents (default 8; a VA detuned from the first oscillator, whose second oscillator may be the louder partial near
 * it, 60). SWARM spreads its oscillators on purpose and NOISE / PARTICLE / the drums have no pitch: those are checked
 * for level only. Level: the peak of a held note at C4 (a drum's hit) between 1500 and 60000 (the tap is before the master limiter; the presets are trimmed to equal loudness), and the voice free after
 * the release. ONLY=name measures one preset. Exit status: failed checks. */
#define main hostsim_main
#include "hostsim.c"
#undef main
#define TAPL acc[0].l
#define TAPR acc[0].r
#define W 16384u
#define D 11025u
#define NX (W + D)
static double x[NX];
static void dft(uint32_t o, double f, double *re, double *im)
{
    uint32_t i;
    double r = 0, m = 0, w = 2 * M_PI * f / FS, c = cos(w), s = sin(w), cr = 1, ci = 0;
    for (i = 0; i < W; i++) {
        double h = (0.5 - 0.5 * cos(2 * M_PI * i / W)) * x[o + i], t;
        r += h * cr, m -= h * ci;
        t = cr * c - ci * s, ci = ci * c + cr * s, cr = t;
    }
    *re = r, *im = m;
}
static double f0_of(double fe, double *pow0)
{
    double best = 0, bf = fe, k, r0, i0, r1, i1, dp;
    for (k = -80; k <= 80; k += 2) {
        double ff = fe * pow(2, k / 1200.0), v;
        dft(0, ff, &r0, &i0);
        v = r0 * r0 + i0 * i0;
        if (v > best)
            best = v, bf = ff;
    }
    dft(0, bf, &r0, &i0);
    dft(D, bf, &r1, &i1);
    dp = atan2(i1, r1) - atan2(i0, r0) - 2 * M_PI * bf * D / FS;
    dp -= 2 * M_PI * floor(dp / (2 * M_PI) + 0.5);
    *pow0 = best;
    return bf + dp * FS / (2 * M_PI * D);
}
static void settle(int32_t *o)
{
    uint32_t b, i, any = 1, k;
    for (k = 0; k < NPART; k++)
        trk_all_off(&trk[k]);
    for (b = 0; b < 6000u && any; b++) {
        mix_block(o, CTL);
        for (any = 0, i = 0; i < NVOICE; i++)
            any |= trk[0].v[i].active;
    }
}
static void load(uint32_t pi)
{
    uint32_t i;
    for (i = 0; i < P_E0; i++)
        trk[0].p[i] = TP[i].def;
    host_preset(&trk[0], ENGI_PLAITS, pi);
    trk[0].p[P_AMODE] = 0, trk[0].p[P_DLY] = trk[0].p[P_REV] = 0, trk[0].p[P_VOICE] = V_POLY;
    trk[0].p[P_LD_PIT] = 0, trk[0].p[P_TRANS] = 0;
}
int main(void)
{
    static int32_t o[CTL * 2];
    const engine_t *en = ENGINES[ENGI_PLAITS];
    double tol = getenv("TOL") ? atof(getenv("TOL")) : 8.0;
    uint32_t pi, n, i, b, fails = 0;
    host_tracks_init();
    for (pi = 0; pi < en->npresets; pi++) {
        const preset_t *pr = &en->presets[pi];
        uint32_t md = (uint32_t)pr->e[0], pitched = md <= PX_CHORD;
        double worst = 0, t = (md == PX_VA && pr->e[1] < 64) ? 60.0 : tol, tp = 0;
        int32_t peak = 0;
        uint32_t freed = 0;
        if (getenv("ONLY") && !strstr(pr->name, getenv("ONLY")))
            continue;
        /* level: C4, held 1.2 s, then released */
        settle(o);
        load(pi);
        for (b = 0; b < 16; b++)
            mix_block(o, CTL);
        trk_note_on(&trk[0], 60, 100);
        for (b = 0; b < 1650u; b++) {
            mix_block(o, CTL);
            for (i = 0; i < CTL; i++) {
                int32_t a = (int32_t)((TAPL[i] + TAPR[i]) / 2);
                peak = a < 0 ? (-a > peak ? -a : peak) : (a > peak ? a : peak);
            }
        }
        trk_all_off(&trk[0]);
        for (b = 0; b < 6000u; b++) {
            mix_block(o, CTL);
            for (freed = 1, i = 0; i < NVOICE; i++)
                if (trk[0].v[i].active)
                    freed = 0;
            if (freed)
                break;
        }
        printf("plaits_test: %-11s %-9s peak %6d", pr->name, N_PX_MODEL[md], peak);
        if (peak < 1500 || peak > 60000) {
            printf("  LEVEL?");
            fails++;
        }
        if (!freed) {
            printf("  VOICE STUCK");
            fails++;
        }
        if (pitched) {
            for (n = 36; n <= 96; n += 6) {
                double fe = 440 * pow(2, ((int)n - 69) / 12.0), f, c;
                settle(o);
                load(pi);
                trk[0].p[P_SUS] = 127;   /* held steady for the measurement */
                for (b = 0; b < 16; b++)
                    mix_block(o, CTL);
                trk_note_on(&trk[0], n, 100);
                for (b = 0; b < 140; b++)
                    mix_block(o, CTL);
                for (b = 0; b < NX / CTL; b++) {
                    mix_block(o, CTL);
                    for (i = 0; i < CTL; i++)
                        x[b * CTL + i] = (TAPL[i] + TAPR[i]) / 2.0;
                }
                if (md == PX_ADD || md == PX_CHORD) {         /* the strongest harmonic / chord tone, measured near each */
                    static const int8_t CT[11][4] = {{0, 12, 0, 12}, {0, 7, 12, 7}, {0, 5, 7, 12}, {0, 3, 7, 12},
                                                     {0, 4, 7, 12}, {0, 3, 7, 10}, {0, 4, 7, 11}, {0, 3, 10, 14},
                                                     {0, 4, 11, 14}, {0, 2, 7, 12}, {0, 3, 6, 9}};
                    double bp = -1, pw, cand, ff;
                    int k, j;
                    c = 1e9;
                    for (k = 0; k < (md == PX_ADD ? 8 : 4); k++)
                        for (j = 0; j < (md == PX_ADD ? 1 : 2); j++) {
                            cand = md == PX_ADD ? fe * (k + 1) : fe * pow(2, (CT[(pr->e[1] * 11) >> 7][k] + 12 * j) / 12.0);
                            if (cand > 0.4 * FS)
                                continue;
                            ff = f0_of(cand, &pw);
                            if (pw > bp)
                                bp = pw, c = 1200 * log2(ff / cand);
                        }
                } else {
                    f = f0_of(fe, &tp);
                    c = 1200 * log2(f / fe);
                }
                if (getenv("VERB")) printf(" [%u:%+.0f]", n, c);
                if (fabs(c) > fabs(worst))
                    worst = c;
            }
            printf("  worst %+.1f cents", worst);
            if (fabs(worst) > t) {
                printf("  OUT OF TUNE (limit %.0f)", t);
                fails++;
            }
        }
        printf("\n");
    }
    printf("plaits_test: %u failed\n", fails);
    return (int)fails;
}

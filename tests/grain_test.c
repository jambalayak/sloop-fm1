/* SPDX-License-Identifier: GPL-3.0-only */
/* GRAIN (fx.c, PLAITS port): MIX 0 leaves the wet bus untouched (the golden renders show it bit for bit); MIX up adds a
 * cloud read from the delay line after the note has stopped: energy while the dry note is silent, no sample beyond the
 * limiter's range, for DENS / SIZE / PITCH / POS at their extremes. Exit status: failed checks. */
#define main hostsim_main
#include "hostsim.c"
#undef main
static double run(int mix, int dens, int size, int pitch, int pos, int32_t *peak)
{
    static int32_t o[CTL * 2];
    uint32_t b, i, k;
    double e = 0;
    host_tracks_init();
    for (k = 0; k < NPART; k++)
        trk_all_off(&trk[k]);
    for (b = 0; b < 400; b++)
        mix_block(o, CTL);
    for (k = 0; k < P_E0; k++)
        trk[0].p[k] = TP[k].def;
    host_preset(&trk[0], ENGI_PLAITS, 0);
    trk[0].p[P_DLY] = 127, trk[0].p[P_REV] = 0, trk[0].p[P_VOICE] = V_POLY;
    song.g[G_DMIX] = 0, song.g[G_DFDBK] = 40, song.g[G_GMIX] = mix, song.g[G_GDENS] = dens;
    song.g[G_GSIZE] = size, song.g[G_GPITCH] = pitch, song.g[G_GPOS] = pos;
    trk_note_on(&trk[0], 60, 110);
    for (b = 0; b < 700; b++)
        mix_block(o, CTL);
    trk_all_off(&trk[0]);
    *peak = 0;
    for (b = 0; b < 3500; b++) {
        mix_block(o, CTL);
        if (b > 1500)                                     /* (the note and its release are long over) */
            for (i = 0; i < CTL; i++) {
                int32_t l = o[i], a = l < 0 ? -l : l;
                e += (double)l * l;
                if (a > *peak) *peak = a;
            }
    }
    return e;
}
int main(void)
{
    int32_t pk, fails = 0, d, s, p, q;
    double e0 = run(0, 64, 50, 0, 40, &pk), e1 = run(100, 64, 50, 0, 40, &pk);
    printf("grain_test: off %.3g, on %.3g (peak %d)\n", e0, e1, pk);
    if (!(e1 > e0 * 4 + 1e3)) { printf("grain_test: NO CLOUD\n"); fails++; }
    for (d = 0; d <= 127; d += 127) for (s = 0; s <= 127; s += 127) for (p = -12; p <= 12; p += 12) for (q = 0; q <= 127; q += 127) {
        double e = run(127, d, s, p, q, &pk);
        printf("grain_test: dens %3d size %3d pitch %+3d pos %3d  energy %.3g peak %d\n", d, s, p, q, e, pk);
        if (pk > 32767 * 2 || e != e) { printf("  OUT OF RANGE\n"); fails++; }
    }
    printf("grain_test: %d failed\n", fails);
    return fails;
}

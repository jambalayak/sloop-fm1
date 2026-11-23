#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""The SLOOP logo: a four-colour sail (the four tracks: blue, green, yellow, orange) on the
mast of a 270-degree dial (the knobs of the UI), a hull, and a geometric monoline wordmark.

  tools/gen_logo.py OUT.h          firmware boot splash (RLE, 16-colour palette, RGB565)
  tools/gen_logo.py --assets DIR   sloop-logo.svg, sloop-logo.png, sloop-icon.svg/png, sloop-splash.png
"""
import math
import sys
from pathlib import Path

from PIL import Image, ImageDraw

BLUE, GREEN, YELLOW, ORANGE = (40, 124, 255), (30, 204, 112), (255, 198, 24), (255, 98, 26)
WHITE, BLACK = (242, 242, 242), (0, 0, 0)
HEX = lambda c: "#%02X%02X%02X" % c
SAIL = [BLUE, GREEN, YELLOW, ORANGE]

# ---- geometry (units: the icon is 240 x 240, the wordmark x-height is 72) -------------------
ICON = dict(r=106, w=12, mast_dx=-30, top=-80, bot=38, sail_w=96, gap=6, hull=64)
XH, SW = 72, 12                                  # wordmark x-height, stroke


def sail_bands(cx, cy, k):
    g = ICON
    mx, top, bot = cx + g["mast_dx"] * k, cy + g["top"] * k, cy + g["bot"] * k
    sx, h, wmax, gap = mx + 11 * k, bot - top, g["sail_w"] * k, g["gap"] * k
    bh = (h - 3 * gap) / 4
    out = []
    for i, c in enumerate(SAIL):
        y0 = top + i * (bh + gap)
        y1 = y0 + bh
        out.append((c, [(sx, y0), (sx + wmax * (y0 - top) / h, y0), (sx + wmax * (y1 - top) / h, y1), (sx, y1)]))
    return mx, top, bot, out


def rrect(d, box, r, fill):
    """rounded rectangle (ImageDraw.rounded_rectangle needs Pillow 8.2)"""
    x0, y0, x1, y1 = box
    r = min(r, (x1 - x0) / 2, (y1 - y0) / 2)
    d.rectangle([x0 + r, y0, x1 - r, y1], fill=fill)
    d.rectangle([x0, y0 + r, x1, y1 - r], fill=fill)
    for cx, cy in ((x0 + r, y0 + r), (x1 - r, y0 + r), (x0 + r, y1 - r), (x1 - r, y1 - r)):
        d.ellipse([cx - r, cy - r, cx + r, cy + r], fill=fill)


def draw_icon(d, ox, oy, k):
    g = ICON
    cx, cy, r, w = ox + 120 * k, oy + 120 * k, g["r"] * k, g["w"] * k
    d.arc([cx - r, cy - r, cx + r, cy + r], start=135, end=405, fill=WHITE, width=max(1, round(w)))
    for a in (135, 405):
        x = cx + (r - w / 2) * math.cos(math.radians(a))
        y = cy + (r - w / 2) * math.sin(math.radians(a))
        d.ellipse([x - w / 2, y - w / 2, x + w / 2, y + w / 2], fill=WHITE)
    mx, top, bot, bands = sail_bands(cx, cy, k)
    rrect(d, [mx - 5 * k, top - 4 * k, mx + 5 * k, bot + 14 * k], 5 * k, WHITE)
    for c, poly in bands:
        d.polygon(poly, fill=c)
    rrect(d, [cx - g["hull"] * k, bot + 24 * k, cx + g["hull"] * k, bot + 36 * k], 6 * k, WHITE)


def word_width(k):
    # A compact 5x7 pixel wordmark; keep it legible on the 240 px boot splash.
    cell = (XH * k / 7) * 0.78
    return 53 * cell


def draw_word(d, ox, oy, k, col=WHITE):
    glyphs = {
        "H": ("10001", "10001", "10001", "11111", "10001", "10001", "10001"),
        "O": ("01110", "10001", "10001", "10001", "10001", "10001", "01110"),
        "R": ("11110", "10001", "10001", "11110", "10100", "10010", "10001"),
        "A": ("01110", "10001", "10001", "11111", "10001", "10001", "10001"),
        "T": ("11111", "00100", "00100", "00100", "00100", "00100", "00100"),
        "I": ("11111", "00100", "00100", "00100", "00100", "00100", "11111"),
        "-": ("00000", "00000", "00000", "11111", "00000", "00000", "00000"),
        "1": ("00100", "01100", "00100", "00100", "00100", "00100", "01110"),
    }
    cell = (XH * k / 7) * 0.78
    x0 = ox
    y0 = oy + (XH * k - 7 * cell) / 2
    for ch in "HORATIO-1":
        pattern = glyphs[ch]
        for yy, bits in enumerate(pattern):
            for xx, bit in enumerate(bits):
                if bit == "1":
                    x = x0 + xx * cell
                    y = y0 + yy * cell
                    d.rectangle([x, y, x + cell, y + cell], fill=col)
        x0 += 6 * cell


def render(kind, W, H, k, bg=BLACK, ss=4):
    im = Image.new("RGB", (W * ss, H * ss), bg)
    d = ImageDraw.Draw(im)
    K = k * ss
    if kind == "h":                                   # icon + wordmark side by side
        draw_icon(d, 10 * K, (H * ss - 240 * K) / 2, K)
        draw_word(d, 270 * K, H * ss / 2 - 36 * K, K)
    elif kind == "v":                                 # icon over the wordmark (the boot splash)
        draw_icon(d, (W * ss - 240 * K) / 2, 0, K)
        draw_word(d, (W * ss - word_width(K)) / 2, 262 * K, K)
    else:
        draw_icon(d, (W * ss - 240 * K) / 2, (H * ss - 240 * K) / 2, K)
    return im.resize((W, H), Image.LANCZOS)


# ---- SVG (stroke centred on the path) ---------------------------------------------------------
def svg_icon(ox, oy):
    g, k = ICON, 1.0
    cx, cy, w = ox + 120, oy + 120, g["w"]
    r = g["r"] - w / 2
    a0, a1 = math.radians(135), math.radians(405)
    p0 = (cx + r * math.cos(a0), cy + r * math.sin(a0))
    p1 = (cx + r * math.cos(a1), cy + r * math.sin(a1))
    s = [f'<path d="M{p0[0]:.2f} {p0[1]:.2f} A{r:.2f} {r:.2f} 0 1 1 {p1[0]:.2f} {p1[1]:.2f}" fill="none" '
         f'stroke="{HEX(WHITE)}" stroke-width="{w}" stroke-linecap="round"/>']
    mx, top, bot, bands = sail_bands(cx, cy, k)
    s.append(f'<rect x="{mx - 5:.2f}" y="{top - 4:.2f}" width="10" height="{bot + 14 - top + 4:.2f}" rx="5" fill="{HEX(WHITE)}"/>')
    for c, poly in bands:
        s.append('<polygon points="' + " ".join(f"{x:.2f},{y:.2f}" for x, y in poly) + f'" fill="{HEX(c)}"/>')
    s.append(f'<rect x="{cx - g["hull"]:.2f}" y="{bot + 24:.2f}" width="{2 * g["hull"]}" height="12" rx="6" fill="{HEX(WHITE)}"/>')
    return s


def svg_word(ox, oy):
    w, xh, col = SW, XH, HEX(WHITE)
    base, x = oy + xh, ox
    ry, rx = (xh + w) / 4 - w / 2, 0.29 * xh - w / 2
    c1 = (x + 0.29 * xh, oy + (xh + w) / 4)
    c2 = (x + 0.29 * xh, base - (xh + w) / 4)

    def pt(c, a):
        a = math.radians(a)
        return c[0] + rx * math.cos(a), c[1] + ry * math.sin(a)
    s = []
    # top bowl 90 -> 335 (clockwise on screen), bottom bowl 270 -> 515
    for c, a, b in ((c1, 90, 335), (c2, 270, 515)):
        p, q = pt(c, a), pt(c, b)
        large = 1 if (b - a) > 180 else 0
        s.append(f'<path d="M{p[0]:.2f} {p[1]:.2f} A{rx:.2f} {ry:.2f} 0 {large} 1 {q[0]:.2f} {q[1]:.2f}" fill="none" '
                 f'stroke="{col}" stroke-width="{w}" stroke-linecap="round"/>')
    x += 2 * 0.29 * xh + 20
    s.append(f'<line x1="{x + w / 2:.2f}" y1="{oy - 32 + w / 2:.2f}" x2="{x + w / 2:.2f}" y2="{base - w / 2:.2f}" '
             f'stroke="{col}" stroke-width="{w}" stroke-linecap="round"/>')
    x += w + 20
    R = xh / 2
    for i in range(3):
        s.append(f'<circle cx="{x + R:.2f}" cy="{oy + R:.2f}" r="{R - w / 2:.2f}" fill="none" stroke="{col}" stroke-width="{w}"/>')
        if i < 2:
            x += 2 * R + 18
    s.append(f'<line x1="{x + w / 2:.2f}" y1="{oy + R:.2f}" x2="{x + w / 2:.2f}" y2="{base + 36 - w / 2:.2f}" '
             f'stroke="{col}" stroke-width="{w}" stroke-linecap="round"/>')
    return s


def svg(kind):
    if kind == "h":
        W, H = 760, 260
        body = svg_icon(10, 10) + ['<text x="270" y="158" fill="#F2F2F2" font-family="Arial, Helvetica, sans-serif" font-size="56" font-weight="700" letter-spacing="1.5">HORATIO-1</text>']
    else:
        W, H = 240, 240
        body = svg_icon(0, 0)
    return (f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {W} {H}" width="{W}" height="{H}" role="img" '
            f'aria-label="HORATIO-1">\n<rect width="{W}" height="{H}" fill="#000"/>\n' + "\n".join(body) + "\n</svg>\n")


# ---- firmware splash ---------------------------------------------------------------------------
SPLASH_W, SPLASH_H = 240, 188
# The compact HORATIO-1 pixel wordmark fits below the icon on the FM-1 splash.
SPLASH_K = 0.5
assert (262 + 72 + 6) * SPLASH_K <= SPLASH_H, "the HORATIO-1 wordmark must fit the splash"


def rgb565(c):
    r, g, b = c
    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)


def synthwave_splash():
    """Compact 240x188 HORATIO boot art: outrun ship, striped sun and neon sea."""
    W, H = 240, 188
    im = Image.new("RGB", (W, H), (4, 3, 24))
    d = ImageDraw.Draw(im)
    # Sixteen deliberately chosen RGB565-friendly colours.
    pal = [
        (4, 3, 24), (9, 7, 42), (17, 12, 68), (30, 18, 105),
        (57, 25, 142), (105, 25, 174), (174, 20, 190), (238, 35, 164),
        (255, 91, 139), (255, 151, 105), (255, 203, 116), (255, 235, 190),
        (13, 66, 145), (17, 139, 210), (24, 225, 235), (92, 245, 255)
    ]
    # Deep starfield and a warm, striped sunset.
    for y in range(0, 118):
        c = pal[1] if y < 42 else pal[2] if y < 82 else pal[3]
        d.line((0, y, W, y), fill=c)
    d.ellipse((58, 7, 182, 131), fill=pal[9], outline=pal[8], width=2)
    for y in range(45, 130, 8):
        d.rectangle((55, y, 185, y + 3), fill=pal[7] if y < 88 else pal[6])
    # Stars, kept sparse around the ship.
    stars = [(12,13),(31,29),(51,9),(198,18),(222,38),(17,68),(215,78),
             (37,91),(194,96),(229,12),(83,18),(165,28),(8,104),(232,106)]
    for x,y in stars:
        d.point((x,y), fill=pal[15])
        if (x+y) % 3 == 0:
            d.line((x-2,y,x+2,y), fill=pal[14])
            d.line((x,y-2,x,y+2), fill=pal[14])
    # Distant mountain silhouettes.
    d.polygon([(0,111),(18,91),(31,103),(48,94),(72,119),(0,126)], fill=pal[3])
    d.polygon([(240,111),(220,92),(205,104),(187,96),(166,119),(240,126)], fill=pal[3])
    d.line([(0,111),(18,91),(31,103),(48,94),(72,119)], fill=pal[5], width=1)
    d.line([(240,111),(220,92),(205,104),(187,96),(166,119)], fill=pal[5], width=1)
    # Horizon and luminous water.
    d.line((0,119,239,119), fill=pal[14], width=2)
    d.rectangle((0,121,239,148), fill=pal[1])
    for y in range(123,149,4):
        d.line((0,y,239,y), fill=pal[4] if y%8 else pal[6], width=1)
    for x in range(-30,280,24):
        d.line((120,121,x,151), fill=pal[4], width=1)
    # Ship rigging behind the sails.
    rig = pal[11]
    d.line((117,18,117,110), fill=rig, width=2)
    d.line((88,43,117,18,149,44), fill=rig, width=1)
    d.line((73,65,117,39,174,68), fill=pal[8], width=1)
    d.line((60,91,117,62,188,94), fill=rig, width=1)
    d.line((117,18,117,12,130,19), fill=pal[8], width=1)
    # Three dark sails with neon edges.
    d.polygon([(119,24),(146,35),(149,55),(119,49)], fill=pal[1], outline=pal[8])
    d.polygon([(115,42),(154,56),(166,83),(116,70)], fill=pal[1], outline=pal[7])
    d.polygon([(113,61),(166,82),(183,107),(111,91)], fill=pal[1], outline=pal[8])
    d.polygon([(111,42),(83,54),(72,75),(111,68)], fill=pal[2], outline=pal[7])
    d.polygon([(109,67),(71,81),(61,105),(109,91)], fill=pal[2], outline=pal[8])
    d.polygon([(111,91),(60,105),(52,120),(113,111)], fill=pal[2], outline=pal[7])
    # Hull, bow and deck.
    d.polygon([(47,113),(111,108),(188,103),(174,124),(145,132),(72,129),(52,123)],
              fill=pal[1], outline=pal[8])
    d.line((49,116,178,107), fill=pal[11], width=2)
    d.line((57,123,145,128), fill=pal[7], width=1)
    d.line((117,20,117,108), fill=pal[8], width=1)
    # Cyan wake and broken reflections.
    d.line((39,132,69,135,91,130,112,137,135,131,159,137,190,130), fill=pal[14], width=3)
    d.line((54,137,81,139,101,136), fill=pal[15], width=1)
    for x,y,w in [(15,139,23),(49,144,18),(83,141,31),(145,145,26),(190,139,34),(112,148,18)]:
        d.line((x,y,x+w,y), fill=pal[7] if y%2 else pal[14], width=1)
    # Lower dark grid, restrained so the title remains readable.
    d.rectangle((0,151,239,187), fill=pal[0])
    for y in (154,160,168,178,187):
        d.line((0,y,239,y), fill=pal[4], width=1)
    for x in range(-100,341,24):
        d.line((120,151,x,187), fill=pal[4], width=1)
    # Large compact HORATIO pixel wordmark.
    glyphs = {
        "H":("10001","10001","10001","11111","10001","10001","10001"),
        "O":("01110","10001","10001","10001","10001","10001","01110"),
        "R":("11110","10001","10001","11110","10100","10010","10001"),
        "A":("01110","10001","10001","11111","10001","10001","10001"),
        "T":("11111","00100","00100","00100","00100","00100","00100"),
        "I":("11111","00100","00100","00100","00100","00100","11111")
    }
    cell, gap = 3, 1
    word = "HORATIO"
    width = len(word) * (5*cell+gap)-gap
    ox, oy = (W-width)//2, 153
    for ch in word:
        for yy,row in enumerate(glyphs[ch]):
            for xx,b in enumerate(row):
                if b == "1":
                    x, y = ox+xx*cell, oy+yy*cell
                    d.rectangle((x,y,x+cell-1,y+cell-1), fill=pal[11])
                    if y+cell < H-1:
                        d.point((x,y+cell), fill=pal[7])
        ox += 5*cell+gap
    # Anchor emblem and a slim neon boot progress bar.
    d.line((120,178,120,184), fill=pal[11], width=1)
    d.line((116,180,124,180), fill=pal[11], width=1)
    d.arc((116,179,124,187), 0, 180, fill=pal[11], width=1)
    d.rounded_rectangle((43,184,197,187), radius=2, outline=pal[14], width=1)
    d.rectangle((46,185,133,186), fill=pal[7])
    return im, pal


def splash_header(path):
    im, cols = synthwave_splash()
    src = im.tobytes()
    px = []
    cache = {}
    for i in range(0, len(src), 3):
        p = (src[i], src[i + 1], src[i + 2])
        if p not in cache:
            cache[p] = min(range(16), key=lambda k: sum((p[j] - cols[k][j]) ** 2 for j in range(3)))
        px.append(cache[p])
    rle = bytearray()
    i = 0
    while i < len(px):
        j = i
        while j < len(px) and px[j] == px[i] and j - i < 16:
            j += 1
        rle.append(((j - i - 1) << 4) | px[i])
        i = j
    L = ["/* generated by tools/gen_logo.py: the HORATIO synthwave boot splash */", "#pragma once", "#include <stdint.h>",
         f"#define SLOOP_SPLASH_W {SPLASH_W}", f"#define SLOOP_SPLASH_H {SPLASH_H}",
         "static const uint16_t SLOOP_SPLASH_PAL[16] = {" + ", ".join(f"0x{rgb565(c):04X}" for c in cols) + "};",
         f"static const uint8_t SLOOP_SPLASH_RLE[{len(rle)}] = {{"]
    for k in range(0, len(rle), 24):
        L.append("    " + ", ".join(str(b) for b in rle[k:k + 24]) + ",")
    L.append("};")
    Path(path).write_text("\n".join(L) + "\n")
    print(f"logo: HORATIO synthwave splash {SPLASH_W}x{SPLASH_H}, {len(rle)} B RLE -> {path}")

def assets(out):
    out = Path(out)
    out.mkdir(parents=True, exist_ok=True)
    (out / "sloop-logo.svg").write_text(svg("h"))
    (out / "sloop-icon.svg").write_text(svg("i"))
    render("h", 1520, 520, 2.0).save(out / "sloop-logo.png")
    render("i", 512, 512, 512 / 240).save(out / "sloop-icon.png")
    render("v", SPLASH_W, SPLASH_H, SPLASH_K).resize((SPLASH_W * 2, SPLASH_H * 2), Image.NEAREST).save(out / "sloop-splash.png")
    print(f"logo: assets in {out}")


if __name__ == "__main__":
    if len(sys.argv) == 3 and sys.argv[1] == "--assets":
        assets(sys.argv[2])
    elif len(sys.argv) == 2:
        splash_header(sys.argv[1])
    else:
        sys.exit(__doc__)

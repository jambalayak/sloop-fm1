#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""The SLOOP logo: a four-colour sail (the four tracks: blue, green, yellow, orange) on the
mast of a 270-degree dial (the knobs of the UI), a hull, and a geometric monoline wordmark.

  tools/gen_logo.py OUT.h          firmware boot splash (RLE, 16-colour palette, RGB565)
  tools/gen_logo.py --assets DIR   sloop-logo.svg, sloop-logo.png, sloop-icon.svg/png, sloop-splash.png
"""
import base64
import math
import os
import sys
import zlib
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


def synthwave_splash_legacy():
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



# A compact, hand-finished 16-colour HORATIO frigate image.  The indexed pixels
# are zlib-compressed here so the build still emits the normal RGB565/RLE header.
_HORATIO_SPLASH_Z = """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"""
_HORATIO_SPLASH_PALETTE = [
    (3, 2, 20), (10, 7, 39), (24, 12, 65), (44, 17, 91),
    (73, 23, 119), (111, 29, 145), (154, 31, 157), (205, 35, 159),
    (244, 56, 151), (255, 105, 151), (255, 151, 139), (255, 194, 147),
    (255, 226, 183), (14, 67, 130), (27, 145, 188), (104, 220, 222),
]

def synthwave_splash():
    if os.environ.get("HORATIO_PLAITS", "0") != "1":
        return synthwave_splash_legacy()
    im = Image.frombytes("P", (240, 188), zlib.decompress(base64.b64decode(_HORATIO_SPLASH_Z)))
    flat = [channel for colour in _HORATIO_SPLASH_PALETTE for channel in colour]
    im.putpalette(flat + [0] * (768 - len(flat)))
    return im.convert("RGB"), _HORATIO_SPLASH_PALETTE


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

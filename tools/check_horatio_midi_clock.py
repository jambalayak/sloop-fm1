#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Guard the HORATIO/SLOOP external MIDI clock transport and timing contract.

This is a source-level regression test, not a substitute for testing a real
USB/TRS clock master against the FM-1 hardware.
"""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
seq = (ROOT / "firmware/src/seq.c").read_text()
uart = (ROOT / "firmware/src/midi_uart.c").read_text()
usb = (ROOT / "firmware/src/usb.c").read_text()

def require(label, condition):
    if not condition:
        raise SystemExit("FAIL: " + label)
    print("ok: " + label)

def section(source, start, end):
    a = source.find(start)
    b = source.find(end, a + len(start))
    if a < 0 or b < 0:
        raise SystemExit("FAIL: missing source section " + start)
    return source[a:b]

clock = section(seq, "static int mclk_on(void)", "/* everything that happens between")
event = section(clock, "static void mclk_event(", "static uint32_t mclk_adv(")
advance = section(clock, "static uint32_t mclk_adv(", "/* 2.7, GLO > MIDI 3 > CLK OUT")
output = section(clock, "static void clkout_block(", "/* everything that happens between")

require("clock uses MIDI's 24 PPQN", "#define MCLK_PULSE_U (BEAT_U / 24u)" in clock)
require("input follows the selected USB/TRS source", "src != (uint32_t)song.g[G_SYNC]" in event)
require("USB and TRS clock and transport are queued", 
        "b == 0xF8u || b == 0xFAu || b == 0xFBu || b == 0xFCu" in uart and
        "cin == 0xFu && (st == 0xF8u || st == 0xFAu || st == 0xFBu || st == 0xFCu)" in usb)
require("START resets clock phase while CONTINUE preserves the stopped phase",
        "if (st == 0xFAu) {" in event and
        "mclk.pos = mclk.done = 0;" in event and
        "mclk.have = 0;" in event and
        "else if (!song.playing)" in event and
        event.index("if (st == 0xFAu) {") < event.index("mclk.pos = mclk.done = 0;") and
        event.index("mclk.pos = mclk.done = 0;") < event.index("} else if (!song.playing)"))
require("START, CONTINUE and STOP reach transport handling",
        "transport_req = 3" in event and "else if (!song.playing)" in event and
        "if (st == 0xFCu)" in event and "transport_req = 2" in event)
require("clock times are accepted only in the 40..240 BPM beat range",
        "dt >= 250u && dt <= 1500u" in event and "clamp((int32_t)((t10 + 5u) / 10u), 40, 240)" in event)
require("tempo measurement averages up to eight beats", "#define MCLK_NB 8u" in clock and "mclk.nb < MCLK_NB" in clock)
require("interpolation never runs beyond the next incoming pulse",
        "if (el > mclk.iv_ms)" in advance and "off = MCLK_PULSE_U - 1u" in advance)
require("late sequencer blocks catch up at a bounded rate",
        "cap = n * 2u * (uint32_t)song.g[G_BPM]" in advance and "if (adv > cap)" in advance)
require("missing external clock falls back to internal tempo after 500 ms",
        "fm1_ms - mclk.last_ms < 500u" in clock and
        "mclk_on() && song.playing ? mclk_adv(n) : n * (uint32_t)song.g[G_BPM]" in seq)
require("clock output is suppressed while following external sync",
        "!song.g[G_CLKOUT] || song.g[G_SYNC]" in output)
require("internal clock output sends START, STOP and 24 PPQN pulses",
        "0xFAu" in output and "0xFCu" in output and
        "while (mco_u >= MCLK_PULSE_U)" in output and "0xF8u" in output)

print("HORATIO MIDI clock source contract passed.")

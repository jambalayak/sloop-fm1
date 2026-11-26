#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Guard HORATIO's three-live-track boundary and legacy project staging.

This is a source-contract regression check. The existing project_test covers
byte-level FUN1..FUN7 migration in the four-track SLOOP reference layout.
"""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
core = (ROOT / "firmware/src/core.h").read_text()
project = (ROOT / "firmware/src/project.c").read_text()


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


require("HORATIO has exactly three live tracks",
        "#define NPART 3" in core and
        "#define NTRK NPART                /* HORATIO live runtime: three synth tracks only */" in core)
require("SLOOP retains its four-track runtime and legacy drum index",
        "#define NTRK 4" in core and "#define TRK_DRUM 3" in core and
        "_Static_assert(NTRK > TRK_DRUM" in core)
require("project staging remains a separate four-track-compatible format",
        "typedef struct" in project and
        "proj_trk_t t[4]" in project)
capture = section(project, "static void proj_capture(project_t *p)", "/* a project's tracks")
apply = section(project, "static void proj_apply(const project_t *p, int all)", "/*")
require("capture serializes only the active runtime tracks",
        "for (i = 0; i < NTRK; i++)" in capture and
        "memcpy(&p->t[i].step" in capture)
require("legacy drum tune/decay uses project staging, not a live track pointer",
        "p->t[TRK_DRUM].e[l + 1u]" in project and
        "p->t[TRK_DRUM].e" in project and
        "drum_tw_put(p);" in capture)
require("HORATIO project loads force Plaits without reading legacy engine IDs",
        "#ifdef HORATIO_PLAITS\n        uint32_t e = ENGI_PLAITS;\n#else" in apply and
        "uint32_t e = k < NPART ? s->engine % NENGINES : 0u;" in apply)
require("project application writes only the three active runtime tracks",
        "for (k = 0; k < NTRK; k++)" in apply and
        "track_t *t = &trk[k];" in apply)
require("SLOOP-only drum implementation remains compile-time guarded",
        "#ifndef HORATIO_PLAITS" in project and
        "dsu_boot();" in project and "fm6_bank_boot();" in project)

print("HORATIO project/runtime boundary contract passed.")

#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Guard HORATIO's three-track FUN7 format and legacy project staging."""
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


require("HORATIO has three live synth tracks",
        "#define NPART 3" in core and
        "#define NTRK NPART" in core)
require("legacy SLOOP retains four live tracks and drum index 3",
        "#define NTRK 4" in core and "#define TRK_DRUM 3" in core and
        "_Static_assert(NTRK > TRK_DRUM" in core)
require("old FUN1..FUN6 working/staging layout remains four tracks",
        "#define PROJ_NTRK_LEGACY 4u" in project and
        "proj_trk_t t[PROJ_NTRK_LEGACY];" in project)
require("HORATIO FUN7 disk format stores exactly three synth tracks",
        "proj_trk_t t[NPART];" in project and
        "PROJ_MAGIC_V7" in project)
capture = section(project, "UI_SMALL static void proj_capture(project_t *p)", "/* a project's tracks")
apply = section(project, "static void proj_apply(const project_t *p, int all)", "#ifndef PROJ_HOST")
from_v7 = section(project, "UI_SMALL static int proj_from_v7(", "/* Capture the stable FUN7")
to_v7 = section(project, "UI_SMALL static void proj_to_v7(", "/* a format 4 project")
require("capture copies only NTRK live tracks into staging",
        "for (i = 0; i < NTRK; i++)" in capture and
        "memcpy(p->t[i].step, trk[i].step" in capture)
require("HORATIO disk writes serialize the three-track FUN7 record",
        "memcpy(v7->t, p->t, sizeof v7->t);" in to_v7 and
        "proj_to_v7(&proj_tmp.v7, p);" in project)
require("FUN7 import validates then copies three synth records",
        "v7->magic != PROJ_MAGIC_V7" in from_v7 and
        "for (i = 0; i < NPART; i++)" in from_v7 and
        "memcpy(&q->t[i], &v7->t[i]" in from_v7)
require("FUN7 import initializes the legacy staging drum record safely",
        "proj_trk_plain(&q->t[TRK_DRUM]);" in from_v7 and
        "memset(q->t[TRK_DRUM].dstep, 0" in from_v7)
require("HORATIO loads force Plaits rather than legacy engine IDs",
        "#ifdef HORATIO_PLAITS\n        uint32_t e = ENGI_PLAITS;\n#else" in apply)
require("HORATIO warm-boot comparison checks the compact FUN7 image",
        "n != (int)sizeof proj_tmp.v7 || !proj_v7_matches(&proj_tmp.v7, &proj_slot[i])" in project)
require("FUN7 match validation checks checksum and only the three serialized tracks",
        "v7->sum == proj_hash(v7, sizeof *v7 - 4u)" in project and
        "!memcmp(v7->t, p->t, sizeof v7->t)" in project)
require("project application writes only active runtime tracks",
        "for (k = 0; k < NTRK; k++)" in apply and
        "track_t *t = &trk[k];" in apply)
require("old drum tune/decay fields remain confined to the staging project",
        "p->t[TRK_DRUM].e[l + 1u]" in project and
        "const int16_t *e = p->t[TRK_DRUM].e;" in project)
require("legacy SLOOP drum and FM6 boot code remains guarded",
        "#ifndef HORATIO_PLAITS" in project and
        "dsu_boot();" in project and "fm6_bank_boot();" in project)

print("HORATIO project/FUN7 boundary contract passed.")

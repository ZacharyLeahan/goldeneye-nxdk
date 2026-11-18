"""Extract the platform-independent parts of the fork's GE32 extension for nxdk.

The Windows extension's SDL/GL hooks and binary addresses have no Xbox ABI.
The input response, inventory selection and wheel artwork do.
"""
from pathlib import Path


def generate(root: Path, emit):
    mod = root / "mods/the-finals"

    def section(name, start, end):
        text = (mod / name).read_text()
        first = text.index(start)
        return text[first:text.index(end, first + len(start))]

    aim = section("precision-aim.h", "typedef struct {double velocity", "static PrecisionAimState preciseAim;")
    aim += section("precision-aim.h", "static double aimIntegrate", "static int precisionAimCapture")
    emit("finals_aim.inc", section("precision-aim.h", "#define AIM_ADS_GAIN", "static struct") + aim)

    # Each split-screen player owns an instance of the Windows wheel state.
    layout = (mod / "wheel-layout.h").read_text()
    selection = section("weapon-wheel.c", "static void wheelUpdate(", "static void testAssert(")
    selection = "\n".join(line for line in selection.splitlines() if "if(logfile)" not in line)
    names = section("weapon-wheel.c", "static const char *names[]", '#include "wheel-layout.h"')
    emit("finals_items.inc", names)
    emit("finals_selection.inc", (layout + selection).replace("static void ", "void ")
         .replace("static float ", "float ").replace("static int ", "int "))

    art = section("wheel-render.h", "static void polygon", "static void draw(")
    draw = section("wheel-render.h", " float cx=vw", " glPopMatrix();glMatrixMode")
    emit("finals_art.inc", art.replace("static void ", "void ") +
         "\nvoid drawSectors(float vw, float vh) {\n" + draw + "}\n")

    emit("finals_font.inc", section("retro-ui.h", "static const unsigned char retroGlyphs", "static float retroWidth"))
    constants = section("jump-pad.h", "#define PAD_ITEM", "static volatile LONG")
    constants += section("combat.h", "#define MOVE_RUN_SCALE", "static ")
    emit("finals_constants.h", constants)

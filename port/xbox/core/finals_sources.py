"""Extract the platform-independent parts of the fork's GE32 extension for nxdk.

The Xbox build reuses the controller response, wheel and traversal algorithms.
It supplies native engine symbols and per-player state for the GE32 hooks.
"""
from pathlib import Path
import sys


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
    emit("finals_material.inc", section("jump-pad-render.h", " for(int y=0;y<32;y++)", " glPushClientAttrib"))
    emit("finals_held.inc", section("jump-pad-render.h", "static void padHeldFace", "static void drawPad"))
    constants = section("jump-pad.h", "#define PAD_ITEM", "static volatile LONG")
    constants += section("combat.h", "#define MOVE_RUN_SCALE", "static ")
    emit("finals_constants.h", constants)

    # Keep the GE32 route search, button window, curves and slide response in
    # the fork. Translate its executable-specific ABI to native engine fields.
    movement = section("mantle.h", "#define MANTLE_BUFFER", "static struct")
    mantle_state = section("mantle.h", "static struct", "static volatile LONG")
    slide_state = section("slide.h", "static struct", "static void slideButton")
    emit("finals_traversal_types.inc", movement +
         mantle_state.replace("static struct", "typedef struct").replace("} mantle;", "} XboxMantle;") +
         slide_state.replace("static struct", "typedef struct").replace("} slide;", "} XboxSlide;"))

    mantle = section("mantle.h", "static void mantlePress", "static int mantleFacesPlayer")
    mantle += section("mantle.h", "static int mantleTopAt", "static void mantleStop")
    mantle += section("mantle.h", "static int mantleVertical(int *useHeight", "static void mantlePublish")
    slide = section("slide.h", "static int slideGrounded", "static void slideBefore")
    slide += "static void slideBefore(int request){\n float dt=g_GlobalTimerDelta;\n"
    slide += section("slide.h", "slide.cooldown=fmaxf", "static void testSlideInput")
    source = mantle + slide
    fields = {"PF(140)": "field_70", "PF(144)": "stanHeight", "PF(152)": "field_7C",
              "PF(1864)": "field_488.collision_position.x", "PF(1872)": "field_488.collision_position.z",
              "PF(380)": "vv_theta", "PF(13940)": "standheight", "PP(1856)": "field_488.current_tile_ptr",
              "PP(200)": "prop", "PI(184)": "crouchpos", "PF(424)": "speedforwards", "PF(416)": "speedsideways"}
    for old, field in fields.items():
        source = source.replace(old, "g_CurrentPlayer->" + field)
    bindings = {
        "FN(int(*)(void*,float,float),0xd5e10)": "stanTestPointWithinTileBoundsMaybe",
        "FN(int(*)(void**,float,float,float,float),0xd6870)": "walkTilesBetweenPoints_NoCallback",
        "FN(float(*)(void*,float,float),0x1000+879136)": "stanGetPositionYValue",
        "*(float*)(base+0x303000+211436)": "g_GlobalTimerDelta",
        "void **tile": "StandTile **tile", "void *tile": "StandTile *tile",
        "void *landing": "StandTile *landing", "jumpAllowed()": "traversal->allowed",
        "openWheel||gadget.flight||slap.phase||padCarrying": "traversal->blocked",
        "openWheel||padCarrying||slap.phase": "traversal->blocked",
        "drop.active=jumpActive=0": "jumpActive=0", "InterlockedExchange(&mantleBusy,1);": "",
        "sprintLatched=sprintOn=0": "sprintOn=0",
        'if(logfile){fprintf(logfile,"slide start=%d entry=%.3f\\n",slide.starts,slide.entry);fflush(logfile);}': "",
        "jumpStandingOnFloor(g_CurrentPlayer->field_70,g_CurrentPlayer->stanHeight,g_CurrentPlayer->field_7C)": "xboxGrounded()",
    }
    for old, new in bindings.items():
        if old not in source:
            raise ValueError(f"Review fork traversal binding: {old}")
        source = source.replace(old, new)
    emit("finals_traversal.inc", source.replace("mantle.", "traversal->mantle.").replace("slide.", "traversal->slide."))


if __name__ == "__main__":
    root, output = map(Path, sys.argv[1:3])
    output.mkdir(parents=True, exist_ok=True)

    def write(name, source):
        path = output / name
        if not path.is_file() or path.read_text() != source:
            path.write_text(source)

    generate(root, write)

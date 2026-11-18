"""Prepare ABI-specific COFF inputs from the current upstream source checkout."""

from pathlib import Path
import json
import re
import sys

repo = Path(__file__).resolve().parents[3]
out = Path(sys.argv[1])
out.mkdir(parents=True, exist_ok=True)

def emit(name, source):
    path = out / name
    path.parent.mkdir(parents=True, exist_ok=True)
    if not path.is_file() or path.read_text() != source:
        path.write_text(source)


def adapt(source, before, after):
    if source.count(before) != 1:
        raise ValueError(f"Review upstream Xbox ABI adapter: {before}")
    return source.replace(before, after)


# Perfect Dark X's queue-retirement fix, applied to the configured SDK copy.
# The GPU DPC can observe a VBlank before the ISR selects the pending surface.
pbkit = Path(sys.argv[2]).read_text()
pbkit = pbkit.replace("for(i=0;i<5;i++) pb_BackBufferbReady[i]=0;",
                      "memset(pb_BackBufferbReady, 0, sizeof(pb_BackBufferbReady));")
pbkit = adapt(pbkit, "if (pb_BackBufferbReady[next]==1)",
              "if (pb_BackBufferbReady[next]==1 && (pb_debug_screen_active || "
              "(VIDEOREG(PCRTC_START) & 0x03ffffffu) == (pb_FBAddr[pb_BackBufferIndex[next]] & 0x03ffffffu)))")
pbkit += """
void xboxAcquireBackBuffer(void)
{
    // Scan the ready queue before checking scanout. The ISR can move a buffer
    // from queued to displayed, but only the render thread can queue it again.
    for (;;) {
        int queued = 0;
        for (int i = 0; i < 3; ++i) {
            if (pb_BackBufferbReady[i] && pb_BackBufferIndex[i] == pb_back_index)
                queued = 1;
        }
        if (!queued && (VIDEOREG(PCRTC_START) & 0x03ffffffu) !=
                       (pb_FBAddr[pb_back_index] & 0x03ffffffu)) return;
        pb_wait_for_vbl();
    }
}
"""
emit("pbkit_present.c", pbkit)

# All game units must see the same slot type, including quoted includes made
# from upstream's own source directory. Keep the PC allocation contract and
# derive the three aliased fields from the actual i386 Model layout.
slots_path = repo / "src/game/objecthandler.h"
slots = slots_path.read_text()
for before, after in (
    ("pad04[0x0C]", "pad04[offsetof(Model, obj) - 4]"),
    ("pad14[0x0C]", "pad14[offsetof(Model, datas) - offsetof(Model, obj) - sizeof(s32)]"),
    ("pad28[sizeof(struct Model) - 0x28]", "pad28[sizeof(Model) - offsetof(Model, datas) - sizeof(void *)]"),
):
    if slots.count(before) != 2:
        raise ValueError("Review upstream model slot layout")
    slots = slots.replace(before, after)
checks = []
for typ in ("struct AnimModelSlot", "struct ModelSlot"):
    for alias, field in (("unk02", "rwdatalen"), ("unk08", "obj"), ("unk10", "datas")):
        checks.append(f'_Static_assert(offsetof({typ}, {alias}) == offsetof(Model, {field}), "model slot {field}");')
    checks.append(f'_Static_assert(sizeof({typ}) == sizeof(Model), "model slot stride");')
slots = adapt(slots, "extern struct AnimModelSlot *g_AnimModelSlots;",
              "\n".join(checks) + "\nextern struct AnimModelSlot *g_AnimModelSlots;")
emit("objecthandler.h", "#include <stddef.h>\n" + slots)
emit("model_headers.json", json.dumps({"version": 0, "roots": [
    {"type": "file", "name": str(slots_path), "external-contents": str(out.resolve() / "objecthandler.h")}
]}))


for unit in ("lv", "spectrum", "image_bank", "objecthandler", "initunk_005450", "loadobjectmodel", "front", "bondview2"):
    src = (repo / f"src/game/{unit}.c").read_text()
    # COFF has no ELF weak aliases. Alias the references within the original TU.
    src = re.sub(r'^#pragma weak (\w+) = (\w+)$', r'#define \1 \2', src, flags=re.M)
    if unit in ("image_bank", "objecthandler", "initunk_005450"):
        # These PC paths resolve cart offsets and linker labels, independently of pointer size.
        src = src.replace("#if defined(__x86_64__)", "#if defined(PORT)")
    if unit == "lv":
        src = adapt(src, "DL = weaponRenderTracers(DL);",
                    "extern Gfx *xboxDrawWorldPads(Gfx *);\n            DL = xboxDrawWorldPads(DL);\n            DL = weaponRenderTracers(DL);")
        # Start the downstream boot sequence inside the normal game scheduler.
        src = adapt(src, "menu_update = MENU_FILE_SELECT;",
                    "menu_update = MENU_RAREWARE_LOGO;")
    if unit == "bondview2":
        src = adapt(src, "gunRenderFirstPersonGunModels(&gdl);",
                    "if (!xboxDrawHeldPad(&gdl)) gunRenderFirstPersonGunModels(&gdl);")
        entry = "void MoveBond(s8 stick_x, s8 stick_y, u16 buttons, u16 oldbuttons)"
        src = adapt(src, entry, '#include "port/xbox/core/finals_game.inc"\n\n' + entry)
        src = adapt(src, "bondviewProcessInput(stick_x, stick_y, buttons, oldbuttons);",
                    "xboxFinalsProcessInput(stick_x, stick_y, buttons, oldbuttons);")
        src = adapt(src, "bondviewUpdatePlayerY(use_stanHeight, sp390);",
                    "xboxFinalsVertical();\n    bondviewUpdatePlayerY(use_stanHeight, sp390);")
    if unit == "front":
        # Only Complex has converted multiplayer stage data in xbox.bin.
        # Use the stock disabled-tile rendering for every unavailable arena.
        default = "MP_stage_selected = MP_STAGE_TEMPLE;"
        if src.count(default) != 3:
            raise ValueError("Review upstream multiplayer default and fallback arenas")
        src = src.replace(default, "MP_stage_selected = MP_STAGE_COMPLEX;")
        check = "s32 check_if_mp_stage_unlocked(s32 stage)\n{\n    s32 players;"
        src = adapt(src, check, check + "\n    if (stage != MP_STAGE_COMPLEX) return FALSE;")
        src = adapt(src, "current_mp_stage_highlighted = MP_STAGE_RANDOM;",
                    "current_mp_stage_highlighted = MP_stage_selected;")
        src = adapt(src, """while (check_if_mp_stage_unlocked(current_mp_stage_highlighted) == 0)
            {
                current_mp_stage_highlighted = current_mp_stage_highlighted - 1;
            }""", """if (!check_if_mp_stage_unlocked(current_mp_stage_highlighted))
                current_mp_stage_highlighted = -1;""")
        src = adapt(src, "if (isGunBarrelInMode2())\n    {\n        frontChangeMenu(MENU_EYE_INTRO, TRUE);",
                    "if (isGunBarrelInMode2())\n    {\n        frontChangeMenu(is_first_time_on_main_menu ? MENU_EYE_INTRO : MENU_FILE_SELECT, TRUE);")
        marker = "void init_menu06_modeselect(void)\n{"
        src = adapt(src, marker, '#include "port/xbox/core/mode_menu.inc"\n\n' +
                    marker + "\n    xboxModeMessage = NULL;")
        # Leave dossier setup, the previous tab and actual menu transitions
        # upstream. Only the downstream mode rows and hit zones differ.
        start = src.index("    else if ((is_cheat_menu_available) && (275.0f <= cursor_v_pos))")
        end = src.index("    if (joyGetButtonsPressedThisFrame(PLAYER_1, B_BUTTON))", start)
        src = src[:start] + "    else xboxChooseMode();\n\n" + src[end:]
        start = src.index("    x = 0x96;", src.index("Gfx* constructor_menu06_modesel(Gfx* DL)\n{"))
        end = src.index("    DL = frontAddPreviousTabText(DL);", start)
        src = src[:start] + "    DL = xboxDrawModeRows(DL);\n" + src[end:]
        src = adapt(src, "cursor_v_pos = mode * 0x20 + 0xe2;",
                    "cursor_v_pos = mode == GAMEMODE_MULTI ? 290.0f : 226.0f;")
    if unit == "loadobjectmodel":
        # The PORT branch hardcodes LP64 setup-record sizes. The original
        # ILP32 switch walks the exact record layout emitted by assets.py.
        anchor = "sizepropdef(PropDefHeaderRecord *pdef)\n{\n#ifdef PORT"
        if src.count(anchor) != 1:
            raise ValueError("Review upstream sizepropdef layout")
        src = src.replace(anchor, anchor.replace("#ifdef PORT", "#if 0"))
    if unit == "spectrum":
        src = src.replace("#define spec_keyboard_row_caps_z_x_c_v spec_keyboard_buffer",
                          "#define spec_keyboard_row_caps_z_x_c_v spec_keyboard_buffer[0]")
    emit(f"{unit}.c", src)

rom = (repo / "port/src/romdata.c").read_text()
# The NT cart mapping and ILP32 fonts live in the Xbox host. Reuse the
# endian conversions that are independent of host pointer width.
start = rom.index('static u32 romdataBswap32(u32 v)\n{')
end = rom.index('u32 romdataFontPcSize(')
emit("cart_fixups.c", '#include "romdata.h"\n#include "system.h"\n' + rom[start:end])

symbols = dict(re.findall(r'^\.set (\w+), (0x[\da-fA-F]+)', (repo / "port/src/romassets_u.s").read_text(), re.M))
symbols.update(re.findall(r'^#define (ANIM_DATA_\w+) \(g_pc_animdata_base\[(0x[\da-f]+)u\]\)', (repo / "assets/animationtable_data.h").read_text(), re.M))
symbols.update(cfb_16="0x81800000", _bssSegmentEnd="0x81850000", animations_frame_buffer="0x81fffd30")
emit("cart_symbols.c", "".join(f'__asm__(".globl _{key}\\n.set _{key}, {value}");\n' for key, value in symbols.items()))

# Xbox DRAM has one native mapping. Preserve the original texture byte stream
# and resolve native addresses before interpreting display-list segment tags.
rsp = '#include "port/xbox/core/rsp_memory.h"\n' + (repo / "port/fast3d/gfx_pc.cpp").read_text()
rsp = adapt(rsp, "const uintptr_t a = (uintptr_t)addr;\n    if (a >= 0x10000000u",
            "const uintptr_t a = (uintptr_t)addr;\n    if (xboxGameRam(a)) return false;\n    if (a >= 0x10000000u")
rsp = adapt(rsp, "static inline void *seg_addr(uintptr_t w1) {",
            "static inline void *seg_addr(uintptr_t w1) {\n    w1 = xboxGfxAddress(w1);\n    if (xboxGameRam(w1)) return (void *)w1;")
rsp = adapt(rsp, "        case G_MW_SEGMENT:\n", "        case G_MW_SEGMENT:\n            data = xboxGfxAddress(data);\n")
# CI8 LoadBlock can exceed the cropped base mip. Stop at the last base texel.
rsp = adapt(rsp, "k < line_size_bytes; i++, k++, j++", "k < line_size_bytes && i < size_bytes; i++, k++, j++")
emit("gfx_pc.cpp", rsp)

from finals_sources import generate
generate(repo, lambda name, source: emit("finals/" + name, source))

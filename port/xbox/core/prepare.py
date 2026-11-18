"""Prepare ABI-specific COFF inputs from the current upstream source checkout."""

from pathlib import Path
import re
import sys

repo = Path(__file__).resolve().parents[3]
out = Path(sys.argv[1])
out.mkdir(parents=True, exist_ok=True)

def emit(name, source):
    path = out / name
    if not path.is_file() or path.read_text() != source:
        path.write_text(source)


def adapt(source, before, after):
    if source.count(before) != 1:
        raise ValueError(f"Review upstream Xbox ABI adapter: {before}")
    return source.replace(before, after)


for unit in ("lv", "spectrum", "image_bank", "objecthandler", "initunk_005450", "loadobjectmodel", "front"):
    src = (repo / f"src/game/{unit}.c").read_text()
    # COFF has no ELF weak aliases. Alias the references within the original TU.
    src = re.sub(r'^#pragma weak (\w+) = (\w+)$', r'#define \1 \2', src, flags=re.M)
    if unit in ("image_bank", "objecthandler", "initunk_005450"):
        # These PC paths resolve cart offsets and linker labels, independently of pointer size.
        src = src.replace("#if defined(__x86_64__)", "#if defined(PORT)")
    if unit == "lv":
        # Start the downstream boot sequence inside the normal game scheduler.
        src = adapt(src, "menu_update = MENU_FILE_SELECT;",
                    "menu_update = MENU_RAREWARE_LOGO;")
    if unit == "front":
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

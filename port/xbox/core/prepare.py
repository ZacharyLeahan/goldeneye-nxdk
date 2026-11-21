"""Prepare ABI-specific COFF inputs from the current upstream source checkout."""

from pathlib import Path
import json
import re
import subprocess
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


music = (repo / "src/music.c").read_text()
for track in range(1, 4):
    player = f"g_musicXTrack{track}SeqPlayer"
    anchor = f"while (alCSPGetState({player}))\n        ;"
    music = adapt(music, anchor, f"xboxWaitForSequenceStop({player});")
music = adapt(music, "void musicTrack1Play(s32 track)", "extern void xboxWaitForSequenceStop(ALCSPlayer *);\n\nvoid musicTrack1Play(s32 track)")
emit("music.c", music)


# A port can already be connected on title launch with no pending change
# bit. A failed enumeration also clears that bit. Retry connected ports
# without registered devices; a working controller must never be reset.
ohci = Path(sys.argv[3]).read_text()
poll = "if ((_ohci->HcRhPortStatus[i] & USBH_HcRhPortStatus_CSC_Msk) == 0)\n            continue;"
ohci = adapt(ohci, poll, """static uint32_t last_probe[OHCI_PORT_CNT];
        uint32_t status = _ohci->HcRhPortStatus[i];
        if (!(status & USBH_HcRhPortStatus_CSC_Msk)) {
            if (!(status & USBH_HcRhPortStatus_CCS_Msk) ||
                get_ticks() - last_probe[i] < 100 || ohci_find_device_by_port(i+1))
                continue;
        }
        last_probe[i] = get_ticks();""")
emit("ohci_retry.c", ohci)


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
pbkit = '#include "port/lan/frame_trace.h"\n#include "port/xbox/perf_hud.h"\nstatic unsigned ftSurfaceFrames[3];\n' + pbkit
pbkit = adapt(pbkit, "    //insert in push buffer the commands to trigger screen swapping at next VBlank",
              """    if (ftRecording()) {
        __atomic_store_n(&ftSurfaceFrames[pb_back_index], ftRenderId, __ATOMIC_RELEASE);
        ftRecord(FT_QUEUE, ftRenderId, pb_back_index);
    }
    //insert in push buffer the commands to trigger screen swapping at next VBlank""")
pbkit = adapt(pbkit, "            pb_BackBufferbReady[next]=1;",
              """            if (ftRecording()) ftRecord(FT_READY, __atomic_load_n(&ftSurfaceFrames[paramA], __ATOMIC_ACQUIRE), paramA);
            pb_BackBufferbReady[next]=1;""")
pbkit = adapt(pbkit, "if (pb_BackBufferbReady[pb_BackBufferNxtVBL]==1) VIDEOREG(PCRTC_START)=pb_FBAddr[pb_BackBufferIndex[pb_BackBufferNxtVBL]]&0x03FFFFFF;",
              """if (pb_BackBufferbReady[pb_BackBufferNxtVBL]==1) VIDEOREG(PCRTC_START)=pb_FBAddr[pb_BackBufferIndex[pb_BackBufferNxtVBL]]&0x03FFFFFF;
        unsigned hudEpoch = __atomic_load_n(&xboxHudScanEpoch, __ATOMIC_ACQUIRE);
        if (hudEpoch) xboxHudScanout(hudEpoch, VIDEOREG(PCRTC_START) & 0x03ffffffu);
        if (ftRecording()) {
            unsigned showing = VIDEOREG(PCRTC_START) & 0x03ffffffu;
            unsigned i = 0;
            while (i < 3 && (pb_FBAddr[i] & 0x03ffffffu) != showing) ++i;
            ftRecord(FT_SCAN, i < 3 ? __atomic_load_n(&ftSurfaceFrames[i], __ATOMIC_ACQUIRE) : 0,
                     ((uint64_t)ftRasterPhase() << 32) | i);
        }""")
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
def clang_path(path):
    # Native LLVM reads paths inside the JSON without MSYS argument conversion.
    if sys.platform in ("msys", "cygwin"):
        return subprocess.check_output(["cygpath", "-am", path], text=True).strip()
    return str(path)


emit("model_headers.json", json.dumps({"version": 0, "roots": [
    {"type": "file", "name": clang_path(slots_path),
     "external-contents": clang_path(out.resolve() / "objecthandler.h")}
]}))


sys.path.insert(0, str(repo / "port/lan"))
from adapt import adapt as adapt_lan
from mode_menu import generate as multiplayer_menu
sys.path.insert(0, str(repo / "port/lan/tests"))
from egypt_camera import generate as egypt_camera
spawn = egypt_camera((repo / "src/game/bondview_r.c").read_text())
spawn_choice = "rand_pad_index = bondviewGetRandomSpawnPadIndex();"
emit("bondview_r.c", adapt(spawn, spawn_choice, spawn_choice +
     "\n            extern int xboxCloseSpawnTest(void);\n            if (xboxCloseSpawnTest()) rand_pad_index = 0;"))
boss = adapt_lan("boss", (repo / "src/boss.c").read_text(), xbox=True)
# Enable the measured path only in the two qualified single-player missions.
boss = adapt(boss, "gdl = lvlRender(gdl);",
             """int solo = getPlayerCount() == 1 && !lanActive();
                            ftSetCampaignTuning(solo && (g_StageNum == LEVELID_JUNGLE || g_StageNum == LEVELID_STREETS),
                                                solo && g_StageNum == LEVELID_JUNGLE,
                                                solo && g_StageNum == LEVELID_STREETS);
                            gdl = lvlRender(gdl);""")
emit("boss.c", boss)

for unit in ("radar", "chraction", "lv", "bg", "model", "spectrum", "image_bank", "objecthandler", "initunk_005450", "loadobjectmodel", "front", "bondview2", "gun", "propobj", "chrprop", "mpmenu"):
    src = (repo / f"src/game/{unit}.c").read_text()
    if unit in ("radar", "chraction", "lv", "bg", "model", "bondview2", "gun", "propobj", "chrprop", "mpmenu"):
        src = adapt_lan(unit, src, xbox=True)
    # COFF has no ELF weak aliases. Alias the references within the original TU.
    src = re.sub(r'^#pragma weak (\w+) = (\w+)$', r'#define \1 \2', src, flags=re.M)
    if unit in ("image_bank", "objecthandler", "initunk_005450"):
        # These PC paths resolve cart offsets and linker labels, independently of pointer size.
        src = src.replace("#if defined(__x86_64__)", "#if defined(PORT)")
    if unit == "lv":
        src = 'void xboxLoadCheckpoint(int stage, const char *step);\n' + src
        for call in ("load_bg_file(g_CurrentStageToLoad);", "bodiesReset(stage);", "proplvreset2(stage);",
                     "init_path_table_links();", "modelmgrSetLevelResetting(FALSE);"):
            src = adapt(src, call, f'xboxLoadCheckpoint(stage, "{call.split("(")[0]}");\n    ' + call)
        # Start the downstream boot sequence inside the normal game scheduler.
        src = adapt(src, "menu_update = MENU_FILE_SELECT;",
                    "menu_update = MENU_RAREWARE_LOGO;")
    if unit == "bondview2":
        # Animation decelerates over several ticks. During a grab, the mantle
        # path owns horizontal displacement, including on a long frame.
        src = adapt(src, "start_collision_pos_x = g_CurrentPlayer->field_488.collision_position.f[0];",
                    "if (traversal->mantle.active) move_offset.x = move_offset.z = 0;\n        "
                    "start_collision_pos_x = g_CurrentPlayer->field_488.collision_position.f[0];")
    if unit == "front":
        # The Xbox local arena menu supports four physical controllers.
        for level, limit in (("BUNKER2", 3), ("ARCHIVES", 3), ("CAVERNS", 3), ("EGYPT", 2)):
            src = adapt(src, f"SP_LEVEL_{level}, 1, {limit}}}", f"SP_LEVEL_{level}, 1, 4}}")
        default = "MP_stage_selected = MP_STAGE_TEMPLE;"
        if src.count(default) != 3:
            raise ValueError("Review upstream multiplayer default and fallback arenas")
        src = src.replace(default, "MP_stage_selected = MP_STAGE_COMPLEX;")
        check = "if (multi_stage_setups[stage].unlock_after == -1)"
        src = adapt(src, check, "if (stage >= MP_STAGE_RANDOM && stage <= MP_STAGE_EGYPT)")
        src = adapt(src, "current_mp_stage_highlighted = MP_STAGE_RANDOM;",
                    "current_mp_stage_highlighted = MP_stage_selected;")
        src = adapt(src, "if (isGunBarrelInMode2())\n    {\n        frontChangeMenu(MENU_EYE_INTRO, TRUE);",
                    "if (isGunBarrelInMode2())\n    {\n        frontChangeMenu(is_first_time_on_main_menu ? MENU_EYE_INTRO : MENU_FILE_SELECT, TRUE);")
        src = multiplayer_menu(src)
    if unit == "loadobjectmodel":
        # The PORT branch hardcodes LP64 setup-record sizes. The original
        # ILP32 switch walks the exact record layout emitted by assets.py.
        anchor = "sizepropdef(PropDefHeaderRecord *pdef)\n{\n#ifdef PORT"
        if src.count(anchor) != 1:
            raise ValueError("Review upstream sizepropdef layout")
        src = src.replace(anchor, anchor.replace("#ifdef PORT", "#if 0"))
        src = adapt(src, "case PROPDEF_OBJECTIVE_COPY_ITEM:\n            return 1;",
                    "case PROPDEF_OBJECTIVE_COPY_ITEM:\n            return 3;")
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

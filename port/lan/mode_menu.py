"""Attach the shared multiplayer page to a build copy of the upstream dossier."""
from pathlib import Path
import sys


def generate(source, *, windows=False):
    def replace(before, after):
        nonlocal source
        if source.count(before) != 1:
            raise ValueError(f"Review multiplayer dossier adapter after upstream change: {before!r}")
        source = source.replace(before, after)

    entry = "void init_menu06_modeselect(void)\n{"
    replace(entry, '#include "port/lan/mode_menu.inc"\n\n' + entry + "\n    initMultiplayerPage();")
    hit_test = "    mission_difficulty_highlighted = DIFFICULTY_MULTI;\n    if (frontCheckCursorOnPreviousTab())"
    replace(hit_test, "    if (multiplayerPage != DOSSIER) { multiplayerMenuTick(); return; }\n" + hit_test)
    draw = "    x = 0x96;\n    y = 0xdc;"
    replace(draw, """    if (multiplayerPage != DOSSIER) {
        DL = drawMultiplayerPage(DL);
        DL = frontAddPreviousTabText(DL);
        return frontDrawCursor(DL);
    }
""" + draw)
    if windows:
        # Keep the twelve stock tiles in place and use the space below the grid.
        # The extra slot is local-only. LAN arena indices remain stable.
        entry = "IMG_MP_EGYPT, LEVELID_EGYPT, SP_LEVEL_EGYPT, 1, 2}"
        replace(entry, entry + """,
    {getStringID(LTITLE, TITLE_STR_133_STATPARK), getStringID(LTITLE, TITLE_STR_134_STAT),
     IMG_MP_STATUE, LEVELID_STATUE, -1, 2, 2}""")
        # MP_STAGE_SELECTED_MAX is the count of original slots including Random.
        entry = "void interface_menu12_mpstage(void)\n{"
        replace(entry, "enum { MP_STATUE_SLOT = MP_STAGE_SELECTED_MAX };\n\n" + entry)
        replace("current_mp_stage_highlighted = (row * 4) + column;",
                """current_mp_stage_highlighted = (row * 4) + column;
            if (check_if_mp_stage_unlocked(MP_STATUE_SLOT) &&
                cursor_v_pos >= 302 && cursor_v_pos <= 328 && cursor_h_pos >= 36 && cursor_h_pos <= 190)
                current_mp_stage_highlighted = MP_STATUE_SLOT;""")
        replace("randomGetNext() % (u32)(MP_STAGE_SELECTED_MAX - 1)",
                "randomGetNext() % (sizeof(multi_stage_setups) / sizeof(multi_stage_setups[0]) - 1)")
        anchor = "    DL = frontAddPreviousTabText(DL);\n    DL = frontDrawCursor(DL);"
        start = source.index("Gfx * constructor_menu12_mpstage(Gfx *DL)\n{")
        at = source.index(anchor, start)
        source = source[:at] + """    if (check_if_mp_stage_unlocked(MP_STATUE_SLOT)) {
        if (!tab_prev_highlight && current_mp_stage_highlighted == MP_STATUE_SLOT)
            DL = microcode_constructor_related_to_menus(DL, 36, 304, 190, 325, 50);
        DL = multiplayerLine(DL, 42, 306, langGet(multi_stage_setups[MP_STATUE_SLOT].folder_text_preset));
    }
""" + source[at:]
    return source


if __name__ == "__main__":
    src, dst = map(Path, sys.argv[1:])
    dst.write_text(generate(src.read_text(), windows=True))

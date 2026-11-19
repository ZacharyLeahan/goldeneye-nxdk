"""Attach the shared multiplayer page to a build copy of the upstream dossier."""
from pathlib import Path
import sys


def generate(source, *, xbox):
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
    if not xbox:
        # A lone player can enter Multiplayer to join a LAN session. Local
        # options still enforce the native minimum when that row is selected.
        replace("else if ((243.0f <= cursor_v_pos) && (joyGetControllerCount() >= 2))",
                "else if (243.0f <= cursor_v_pos)")
        gate = """    if (joyGetControllerCount() >= 2)
    {
        text_color = 0xFF;
    }
    else
    {
        text_color = 0x70;
    }"""
        replace(gate, "    text_color = 0xFF;")
        transition = "            gamemode = GAMEMODE_MULTI;\n            sndPlaySfx(g_musicSfxBufferPtr, DOOR_METAL_CLOSE_SFX, 0);"
        replace(transition, transition.replace("gamemode = GAMEMODE_MULTI;", "openMultiplayerPage(MULTIPLAYER);") + "\n            return;")
    return source


if __name__ == "__main__":
    src, dst = map(Path, sys.argv[1:])
    dst.write_text(generate(src.read_text(), xbox=False))

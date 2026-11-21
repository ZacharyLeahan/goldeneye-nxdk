"""Attach the shared multiplayer page to a build copy of the upstream dossier."""
from pathlib import Path
import sys


def generate(source):
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
    return source


if __name__ == "__main__":
    src, dst = map(Path, sys.argv[1:])
    dst.write_text(generate(src.read_text()))

"""Attach downstream LAN movement and viewport callbacks to build copies only."""
from pathlib import Path
import sys


def adapt(unit, source):
    def replace(before, after):
        nonlocal source
        if source.count(before) != 1:
            raise ValueError(f"Review LAN adapter after change to {unit}: {before}")
        source = source.replace(before, after)
    source = '#include "port/lan/lan.h"\n' + source
    if unit == "bondview2":
        entry = "void MoveBond(s8 stick_x, s8 stick_y, u16 buttons, u16 oldbuttons)"
        replace(entry, '#include "port/lan/pose.inc"\n' + entry)
        anchor = "bondviewUpdatePlayerY(use_stanHeight, sp390);"
        replace(anchor, anchor + "\n    lanMovePlayer();")
        for suffix, value in (("Height", "viGetY()"), ("Uly", "0")):
            entry = f"s16 bondviewGetCurrentPlayerViewport{suffix}(void)\n{{"
            replace(entry, entry + f"\n    if (lanActive()) return {value};")
    elif unit == "lv":
        replace('const char *smp = getenv("GE_STARTMP");',
                'const char *smp = lanIsHost() ? "2,2" : getenv("GE_STARTMP");')
        # The original renderer also ticks props per player. Preserve that work
        # and only submit the local camera's display list to the renderer.
        anchor = "set_cur_player(get_nth_player_from_shuffled(i));"
        replace(anchor, "Gfx *viewStart = DL;\n            " + anchor)
        anchor = "DL = mp_watch_menu_display(DL);"
        replace(anchor, anchor + "\n            if (lanActive() && get_cur_playernum() != 0) DL = viewStart;")
        replace("DL = viSetupScreensForNumPlayers(DL);",
                "if (!lanActive()) DL = viSetupScreensForNumPlayers(DL);")
    return source


if __name__ == "__main__":
    src, dst = map(Path, sys.argv[1:])
    dst.write_text(adapt(src.stem, src.read_text()))

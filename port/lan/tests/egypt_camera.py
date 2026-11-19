"""Select a native guest spawn for repeatable Windows-hosted Egypt profiling."""
from pathlib import Path
import sys


def generate(source):
    anchor = "rand_pad_index = bondviewGetRandomSpawnPadIndex();"
    if source.count(anchor) != 1:
        raise ValueError("Review the native multiplayer spawn selection")
    headers = '#include <ultra64.h>\n#include "game/lv.h"\n#include "port/lan/lan.h"\n'
    return headers + source.replace(anchor, anchor + """
            if (lanActive() && lvlGetCurrentStageToLoad() == LEVELID_EGYPT &&
                get_cur_playernum() == (lanIsHost() ? 1 : 0)) {
#ifdef __XBOX__
                extern int xboxEgyptTestActive(void);
                const char *pad = xboxEgyptTestActive() ? "0" : NULL;
#else
                const char *pad = getenv("GE_TRACE_EGYPT_PAD");
#endif
                if (pad && pad[0] >= '0' && pad[0] <= '9' && !pad[1] &&
                    pad[0] - '0' < startpadcount)
                    rand_pad_index = pad[0] - '0';
            }""")


if __name__ == "__main__":
    Path(sys.argv[2]).write_text(generate(Path(sys.argv[1]).read_text()))

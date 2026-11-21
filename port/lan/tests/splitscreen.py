"""Exercise independent local equipment wheels and grenade throws in the Windows game."""
import argparse
import os
from pathlib import Path
import re
import subprocess
import tempfile
import time

from firefight import game_data


ARENAS = {
    "Temple": 38, "Complex": 31, "Caves": 50, "Library": 48, "Basement": 45,
    "Stack": 46, "Facility": 34, "Bunker": 27, "Archives": 24, "Caverns": 39, "Egypt": 32, "Statue": 22,
}


def verify(binary, directory, log, arena, players):
    env = dict(os.environ, GE_STARTMP=f"{players},{list(ARENAS).index(arena) + 1}",
               GE_MPVIRT=str(players - 1), GE_INPUTSCRIPT_MS="1", GE_SPLIT_TRACE="1")
    for seat in range(players):
        start = 4400 + 550 * seat
        script = [f"{t}:L,SRIGHT" for t in range(start, start + 1230, 35)]
        script += [f"{start + 1280}:SNONE", f"{8100 + 750 * seat}:Z"]
        env["GE_INPUTSCRIPT" if seat == 0 else f"GE_INPUTSCRIPT_P{seat}"] = ";".join(script)
    with log.open("wb") as output:
        game = subprocess.Popen([str(binary)], cwd=directory, env=env,
                                stdout=output, stderr=subprocess.STDOUT)
    try:
        deadline = time.monotonic() + 45
        while time.monotonic() < deadline:
            if game.poll() is not None:
                raise RuntimeError(f"Game exited during {arena}: {log}")
            text = log.read_text(errors="replace")
            events = re.findall(r"Split seat (\d+) wheel (\d+) weapon (\d+) grenades (-?\d+) shots", text)
            opened = [int(seat) for seat, wheel, _, _ in events if wheel == "1"]
            # Each seat opens its own wheel, equips its own grenade, and consumes
            # one after the scheduled trigger. reserve + loaded is unchanged on equip.
            thrown = []
            for seat in range(players):
                states = [(wheel, gun, ammo) for who, wheel, gun, ammo in events if int(who) == seat]
                if ("0", "26", "3") in states and states[-1:] == [("0", "26", "2")]:
                    thrown.append(seat)
            confirmed = f"D416 split=1 players={players} stage={ARENAS[arena]}" in text
            launched = sorted(map(int, re.findall(r"Split grenade launched seat (\d+)", text)))
            if (confirmed and list(dict.fromkeys(opened)) == list(range(players)) and
                    len(thrown) == players and launched == list(range(players))):
                print(f"{arena}: {players} local wheels selected grenades and each seat spent one", flush=True)
                return
            time.sleep(.2)
        raise AssertionError(f"Split-screen firefight did not finish in {arena}: {log}")
    finally:
        if game.poll() is None:
            game.terminate()
        game.wait()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("binary", type=Path)
    parser.add_argument("data", type=Path)
    parser.add_argument("--arena", choices=ARENAS)
    args = parser.parse_args()
    logs = Path(tempfile.mkdtemp(prefix="ge007-split-logs-"))
    print(f"Test logs: {logs}", flush=True)
    with tempfile.TemporaryDirectory(prefix="ge007-split-") as tmp:
        directory = Path(tmp)
        game_data(args.data.resolve(), directory / "data")
        for arena in ([args.arena] if args.arena else ARENAS):
            verify(args.binary.resolve(), directory, logs / f"{arena}-2.log", arena, 2)
        if args.arena is None or args.arena == "Complex":
            verify(args.binary.resolve(), directory, logs / "Complex-4.log", "Complex", 4)

"""Exercise a fresh guest joining an existing match and a host restarting on a different arena."""
import argparse
import os
from pathlib import Path
import re
import socket
import subprocess
import tempfile
import time

from firefight import game_data


def wait_for(log, process, pattern, timeout=45):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        text = log.read_text(errors="replace")
        found = re.search(pattern, text, re.MULTILINE)
        if found:
            return found
        if process.poll() is not None:
            raise RuntimeError(f"Game exited while waiting for {pattern}: {log}")
        time.sleep(.15)
    raise AssertionError(f"Timeout waiting for {pattern}: {log}")


def verify(binary, data, root, logs, cold_pairs=0):
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
        sock.bind(("127.0.0.1", 0))
        port = str(sock.getsockname()[1])
    processes = []

    def start(role, number, script, extra=(), quit_frame=None):
        directory = root / role
        game_data(data, directory / "data")
        args = ["--lan-host"] if role == "host" else ["--lan-join", "127.0.0.1"]
        env = dict(os.environ, GE_LAN_TRACE="1", GE_INPUTSCRIPT_MS="1", GE_INPUTSCRIPT=script)
        if quit_frame:
            env["GE_QUITFRAME"] = str(quit_frame)
        log = logs / f"{role}-{number}.log"
        with log.open("wb") as output:
            process = subprocess.Popen([str(binary), *args, "--lan-port", port, *extra],
                                       cwd=directory, env=env, stdout=output, stderr=subprocess.STDOUT)
        processes.append(process)
        return log, process

    try:
        if cold_pairs:
            for number in range(1, cold_pairs + 1):
                pair = [start(role, number, "60000:A", quit_frame=145) for role in ("host", "guest")]
                for log, process in pair:
                    assert process.wait(timeout=30) == 0, log
                    text = log.read_text(errors="replace")
                    timers = re.findall(r"timer set: t=(\w+) start=100000us period=0us", text)
                    assert len(timers) >= 4 and text.count("timer fire: t=" + timers[0]) >= 4, log
                    assert "D416 split=1 players=2 stage=31" in text and "kernel heartbeat:" not in text, log
                print(f"Cold LAN pair {number}: four startup timer deliveries and Complex loaded on both seats",
                      flush=True)
            return
        host = start("host", 1, ";".join(f"{14500+i*450}:A" for i in range(20)))
        shots = ";".join(f"{4500+i*450}:Z" for i in range(19))
        guest = start("guest", 1, shots + ";14000:SDOWN;14700:SNONE;17000:Z")
        wait_for(*host, r"LAN seat 0 life 2 hp 1\.000 dead 0")
        wait_for(*host, r"LAN seat 1 life 1 hp 1\.000 dead 0 kills 1 shots \d+ weapon 4")
        # Leave enough time after the first kill and respawn for the scripted move.
        # Native reloads can consume trigger presses without producing shots.
        time.sleep(6)
        guest[1].terminate()
        guest[1].wait()
        original = re.findall(r"LAN seat 1 life 1 hp 1\.000 dead 0 kills 1 shots \d+ weapon 4 pistolAmmo (\d+)",
                              host[0].read_text(errors="replace"))
        ammo = int(original[-1])
        assert 0 < ammo < 49
        # Retreat from the victim so at least one native death drop remains
        # in the host's world when a fresh process reconnects.
        host_text = host[0].read_text(errors="replace")
        drops = set(re.findall(r"LAN created drop (\d+) item (\d+)", host_text))
        drops -= set(re.findall(r"LAN removed drop (\d+) item (\d+)", host_text))
        assert drops, "Expected a live death drop before restarting the guest"
        guest = start("guest", 2, "60000:A")
        saved = wait_for(*host, r"LAN restoring guest life 1 at ([-.\d]+) ([-.\d]+)")
        restored = wait_for(*guest, r"LAN placed guest life 1 at ([-.\d]+) ([-.\d]+)")
        assert saved.groups() == restored.groups()
        assert abs(float(saved[1])-1009.9) + abs(float(saved[2])-495) > 5
        wait_for(*guest, rf"LAN seat 0 life 1 hp 1\.000 dead 0 kills 1 shots \d+ weapon 4 pistolAmmo {ammo}\b")
        wait_for(*guest, r"LAN seat 1 life 2 hp 1\.000 dead 0")
        for generation, item in drops:
            wait_for(*guest, rf"LAN mirrored drop {generation} item {item}\b")
        print(f"Guest restarted: move, kill, {ammo} pistol rounds, opponent life and {len(drops)} world drops restored", flush=True)

        host[1].terminate()
        host[1].wait()
        host = start("host", 2, "60000:A", ("--lan-map", "Temple", "--lan-score-limit", "3"))
        new = wait_for(*host, r"LAN round (\d+) started arena 1 score limit 3")
        wait_for(*guest, rf"LAN round {new[1]} started arena 1 score limit 3")
        for log, proc in (host, guest):
            wait_for(log, proc, rf"(?s)LAN round {new[1]} started.*LAN seat 0 life 1 hp 1\.000 dead 0 kills 0 shots 0 weapon 4 pistolAmmo 49")
        print("Host restarted: surviving guest entered the new Temple round with fresh score, health and ammo", flush=True)
    finally:
        for process in processes:
            if process.poll() is None:
                process.terminate()
                process.wait()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("binary", type=Path)
    parser.add_argument("data", type=Path)
    parser.add_argument("--cold-pairs", type=int, default=0,
                        help="run this many fresh host/guest boots to stress the controller-init timer")
    args = parser.parse_args()
    if args.cold_pairs < 0:
        parser.error("--cold-pairs must be nonnegative")
    logs = Path(tempfile.mkdtemp(prefix="ge007-restart-logs-"))
    print(f"Test logs: {logs}", flush=True)
    with tempfile.TemporaryDirectory(prefix="ge007-restart-") as tmp:
        verify(args.binary.resolve(), args.data.resolve(), Path(tmp), logs, args.cold_pairs)

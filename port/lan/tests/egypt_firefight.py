"""Capture a two-seat Windows Egypt firefight and check the guest viewport."""
import argparse
from contextlib import ExitStack
import os
from pathlib import Path
import re
import shutil
import socket
import subprocess
import tempfile

from firefight import game_data
from frame_trace import summarize


def check_guest_image(path):
    with path.open("rb") as image:
        if image.readline().strip() != b"P6":
            raise ValueError(f"Unexpected framebuffer format in {path}")
        width, height = map(int, image.readline().split())
        if image.readline().strip() != b"255":
            raise ValueError(f"Unexpected component range in {path}")
        pixels = image.read()
    if len(pixels) != width * height * 3:
        raise ValueError(f"Truncated guest framebuffer in {path}")
    red, green, blue = pixels[::3], pixels[1::3], pixels[2::3]
    # Native start 0 faces a wall with a green ammo crate at lower center-left.
    # A corridor spawn or a controller tilted toward the ceiling can hide the sky bug.
    box = [y * width + x for y in range(88 * height // 100, 93 * height // 100)
           for x in range(44 * width // 100, 48 * width // 100)]
    if sum(green[i] > red[i] + 5 and green[i] > blue[i] + 3 for i in box) < len(box) // 2:
        raise ValueError(f"Guest camera does not face the ammo crate in {path}")
    yellow = sum(r == 255 and g == 255 and b == 0 for r, g, b in zip(red, green, blue))
    if yellow:
        raise ValueError(f"{yellow} solid-yellow pixels in {path}")


def run(binary, data, output):
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as reserve:
        reserve.bind(("127.0.0.1", 0))
        port = str(reserve.getsockname()[1])
    with tempfile.TemporaryDirectory(prefix="ge007-egypt-") as tmp, ExitStack() as files:
        games = []
        try:
            for role in ("host", "guest"):
                directory = Path(tmp) / role
                game_data(data, directory / "data")
                (directory / "data/ge007.ini").write_text("[Window]\nWidth = 640\nHeight = 480\n")
                env = dict(os.environ, GE_LAN_TRACE="1", GE_FRAME_TRACE="1", GE_RSEED="3",
                           GE_TRACE_EGYPT_PAD="0", GE_QUITFRAME="2020", GE_INPUTSCRIPT_MS="1",
                           GE_INPUTSCRIPT=";".join(f"{4000 + 650*i}:Z" for i in range(60)))
                args = ["--lan-host", "--lan-map", "Egypt"] if role == "host" else ["--lan-join", "127.0.0.1"]
                if role == "guest":
                    env["GE_PCDUMP"] = "1750-1990:60"
                log = files.enter_context((output / f"{role}.log").open("wb"))
                games.append(subprocess.Popen([binary, *args, "--lan-port", port], cwd=directory,
                                              env=env, stdout=log, stderr=subprocess.STDOUT))
            for game in games:
                if game.wait(timeout=90):
                    raise RuntimeError(f"Game exited with {game.returncode}, see {output}")
        finally:
            for game in games:
                if game.poll() is None:
                    game.kill()
                    game.wait()
        for role in ("host", "guest"):
            log = (output / f"{role}.log").read_text(errors="replace")
            shots = [max(map(int, re.findall(rf"LAN seat {seat} .*? shots (\d+)", log)), default=0)
                     for seat in range(2)]
            if "D416 split=1 players=2 stage=32" not in log or min(shots) < 1:
                raise ValueError(f"{role} did not record both seats firing in Egypt: {shots}")
            trace = output / f"{role}.bin"
            shutil.copyfile(Path(tmp) / role / "ge-frame.bin", trace)
            print(f"{role}, shots {shots}")
            summarize(trace)
        captures = Path(tmp) / "guest/ppm"
        for frame in range(1750, 1991, 60):
            capture = captures / f"frame_{frame:06d}.ppm"
            shutil.copyfile(capture, output / capture.name)
            check_guest_image(output / capture.name)
        print("Five guest frames passed the camera and yellow-fill checks.")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("binary", type=Path)
    parser.add_argument("data", type=Path)
    parser.add_argument("output", type=Path, help="parent directory for traces, screenshots and logs")
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    output = Path(tempfile.mkdtemp(prefix="egypt-", dir=args.output)).resolve()
    print(output, flush=True)
    run(args.binary.resolve(), args.data.resolve(), output)

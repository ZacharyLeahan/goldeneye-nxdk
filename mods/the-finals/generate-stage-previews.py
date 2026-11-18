"""Build the two local multiplayer thumbnails from a prepared GE32 install."""
import argparse
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess
import tempfile

WIDTH, HEIGHT, STRIDE = 68, 44, 72
STAGES = (("library", 4), ("stack", 6))


def thumbnail(path):
    bmp = path.read_bytes()
    if bmp[:2] != b"BM" or len(bmp) < 54:
        raise ValueError("Missing or incomplete game capture")
    offset = struct.unpack_from("<I", bmp, 10)[0]
    width, height, planes, depth, compression = struct.unpack_from("<iiHHI", bmp, 18)
    pitch = (width * 3 + 3) & ~3
    if (planes, depth, compression) != (1, 24, 0) or min(width, height) < 100 or len(bmp) < offset + height * pitch:
        raise ValueError("Unexpected game capture format")

    # A 68:44 crop below the radar and above the weapon/HUD. Box filtering
    # keeps railings and the ramp visible in the original-sized filmstrip.
    crop_width = min(width * 4 // 5, height * WIDTH * 7 // (HEIGHT * 10))
    crop_height = crop_width * HEIGHT // WIDTH
    left = (width - crop_width) // 2
    top = (height - crop_height) * 3 // 4
    luma = []
    for y in range(HEIGHT):
        for x in range(WIDTH):
            total = count = 0
            for row in range(top + y * crop_height // HEIGHT, top + (y + 1) * crop_height // HEIGHT):
                begin = offset + (height - 1 - row) * pitch
                for col in range(left + x * crop_width // WIDTH, left + (x + 1) * crop_width // WIDTH):
                    b, g, r = bmp[begin + col * 3:begin + col * 3 + 3]
                    total += 77 * r + 150 * g + 29 * b
                    count += 1
            luma.append(total // (256 * count))

    ordered = sorted(luma)
    black, white = ordered[len(ordered) // 100], ordered[-1 - len(ordered) // 100]
    if white - black < 16:
        raise ValueError("Game capture has insufficient detail")
    result = bytearray(STRIDE * HEIGHT)
    for i, value in enumerate(luma):
        y, x = divmod(i, WIDTH)
        # The filmstrip flips Y, and GE32 reads each I8 word most significant byte first.
        result[(HEIGHT - 1 - y) * STRIDE + (x ^ 3)] = max(0, min(255, (value - black) * 255 // (white - black)))
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("runtime", type=Path)
    root = parser.parse_args().runtime.resolve()
    with tempfile.TemporaryDirectory() as scratch:
        work = Path(scratch)
        for filename in ("GE32.exe", "weapon-wheel.dll", "SDL2.dll", "libgcc_s_seh-1.dll",
                         "libstdc++-6.dll", "libwinpthread-1.dll", "zlib1.dll"):
            shutil.copyfile(root / filename, work / filename)
        shutil.copytree(root / "data", work / "data", ignore=shutil.ignore_patterns("*.log", "*.eep", "*.bak", "*.i8"))
        ini = work / "data/ge007.ini"
        config = ini.read_text(encoding="utf-8")
        for key, value in {"Fullscreen": 0, "Width": 800, "Height": 600, "CrosshairPersistent": 0, "CrosshairHide": 1}.items():
            config = re.sub(rf"(?m)^{key}\s*=.*$", f"{key} = {value}", config)
        ini.write_text(config, encoding="utf-8")

        env = {k: v for k, v in os.environ.items() if not k.startswith(("GE_", "GE32_"))}
        env.update(GE_MPVIRT="1", GE32_SINGLEVIEW="1")
        frames = []
        for name, index in STAGES:
            capture = work / "stage-preview.bmp"
            capture.unlink(missing_ok=True)
            env.update(GE_STARTMP=f"2,{index}", GE32_STAGE_PREVIEW_CAPTURE=name)
            subprocess.run([str(work / "GE32.exe")], cwd=work, env=env, timeout=60, check=True)
            frames.append(thumbnail(capture))
        if frames[0] == frames[1]:
            raise ValueError("Both stage previews produced the same image")
        # Publish a complete pair only after both captures succeed.
        with tempfile.NamedTemporaryFile(dir=root / "data", delete=False) as tmp:
            staged = Path(tmp.name)
        try:
            staged.write_bytes(b"".join(frames))
            staged.replace(root / "data/mp-stage-previews.i8")
        finally:
            staged.unlink(missing_ok=True)


if __name__ == "__main__":
    main()

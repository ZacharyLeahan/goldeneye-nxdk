"""Build the downstream original Xbox executable and optionally a private XISO."""

import argparse
import hashlib
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import zipfile


HERE = Path(__file__).resolve().parent
REPO = HERE.parents[1]
ROM_NAME = "ge007.ntsc-final.z64"
ROM_SIZE = 12 * 1024 * 1024


def read_rom(path):
    if zipfile.is_zipfile(path):
        with zipfile.ZipFile(path) as archive:
            candidates = [entry for entry in archive.infolist()
                          if entry.filename.lower().endswith(".z64") and entry.file_size == ROM_SIZE]
            if len(candidates) != 1:
                raise ValueError("Expected one 12 MiB .z64 file in the ZIP.")
            data = archive.read(candidates[0])
    else:
        if path.stat().st_size != ROM_SIZE:
            raise ValueError("Expected a 12 MiB NTSC-U ROM.")
        data = path.read_bytes()
    expected = (REPO / "ge007.u.sha1").read_text().split()[0]
    if hashlib.sha1(data).hexdigest() != expected:
        raise ValueError("ROM checksum mismatch. This target requires the retail NTSC-U .z64.")
    return data


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rom", type=Path, help="NTSC-U .z64 or ZIP to package beside the XBE")
    parser.add_argument("--build-dir", type=Path, default=REPO / "build-xbox")
    args = parser.parse_args()
    if not os.environ.get("NXDK_DIR"):
        parser.error("Set NXDK_DIR to a prepared nxdk checkout (see DOWNSTREAM.md).")
    sdk = Path(os.environ["NXDK_DIR"]).absolute()
    build = args.build_dir.resolve()
    toolchain = sdk / "share" / "toolchain-nxdk.cmake"
    if not toolchain.is_file() or not (sdk / "lib" / "libc++.lib").is_file():
        parser.error("Build the nxdk libraries first (see DOWNSTREAM.md).")
    env = {**os.environ, "NXDK_DIR": str(sdk),
           "PATH": str(sdk / "bin") + os.pathsep + os.environ["PATH"]}
    try:
        if args.rom:
            data = read_rom(args.rom)
            xiso = shutil.which(str(sdk / "tools/extract-xiso/build/extract-xiso")) or shutil.which("extract-xiso")
            if not xiso:
                parser.error("Build nxdk's extract-xiso tool to package an image.")
        subprocess.run(["cmake", "-S", str(HERE), "-B", str(build),
                        f"-DCMAKE_TOOLCHAIN_FILE={toolchain}",
                        "-DCMAKE_BUILD_TYPE=Release", "-DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY"], env=env, check=True)
        subprocess.run(["cmake", "--build", str(build), "--parallel"], env=env, check=True)
        print(build / "default.xbe", flush=True)
        if args.rom:
            # Stage only the two disc files. Publish the image after successful packing.
            with tempfile.TemporaryDirectory(dir=build) as temp:
                staging = Path(temp) / "disc"
                staging.mkdir()
                (staging / ROM_NAME).write_bytes(data)
                shutil.copyfile(build / "default.xbe", staging / "default.xbe")
                image = Path(temp) / "ge007.iso"
                subprocess.run([xiso, "-c", str(staging), str(image)], env=env, check=True)
                image.replace(build / "ge007.iso")
            print(build / "ge007.iso", flush=True)
    except (OSError, ValueError, zipfile.BadZipFile, subprocess.CalledProcessError) as exc:
        parser.exit(1, f"{exc}\n")


if __name__ == "__main__":
    main()

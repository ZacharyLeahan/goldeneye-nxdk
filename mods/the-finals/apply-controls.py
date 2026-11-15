"""Apply the tested controller/aim profile without copying saves or ROM data."""
import argparse
import configparser
from pathlib import Path
import shutil

PROFILE = {
    "Video": {"CrosshairPersistent": "1", "FpsCap": "144"},
    "Input": {
        "PadPreset": "3", "PadPreset2": "3", "PadPreset3": "3", "PadPreset4": "3",
        "PadLookSensX": "65", "PadLookSensY": "60", "PadLookSmooth": "3",
        "PadLookInvertY": "1", "MouseInvertY": "1", "MouseRawInput": "1",
        "MouseSmoothing": "0", "AimMode": "1", "CrouchMode": "1",
        "PadDeadzone": "4000", "PadDeadzoneL": "4000", "PadDeadzoneR": "4000",
    },
}
for section in ("Input.Pad", "Input.Pad2", "Input.Pad3", "Input.Pad4"):
    PROFILE[section] = {"Aim": "lefttrigger", "Use": "y", "NextWeapon": "none", "Reload": "x", "Crouch": "b", "Gadget": "none"}

def apply(ini: Path):
    parser = configparser.ConfigParser(interpolation=None, strict=False)
    parser.optionxform = str
    if not ini.is_file():
        raise SystemExit("Launch GE32 once, close it, then supply its data/ge007.ini file.")
    parser.read(ini, encoding="utf-8")
    backup = ini.with_name("ge007.before-the-finals.ini")
    if not backup.exists():
        shutil.copy2(ini, backup)
    for section, values in PROFILE.items():
        if not parser.has_section(section):
            parser.add_section(section)
        for key, value in values.items():
            parser.set(section, key, value)
    with ini.open("w", encoding="utf-8", newline="") as out:
        parser.write(out)
    print("Controller profile applied; unrelated settings and saves preserved.")

if __name__ == "__main__":
    args = argparse.ArgumentParser(description=__doc__)
    args.add_argument("ini", type=Path, help="Path to the existing GE32 data/ge007.ini")
    apply(args.parse_args().ini)

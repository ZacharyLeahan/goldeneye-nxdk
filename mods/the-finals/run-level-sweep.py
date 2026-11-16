"""Run offline mechanics fixtures in a separate, already prepared GE32 runtime."""
import argparse
import json
import os
from pathlib import Path
import subprocess

MULTIPLAYER = [("Temple", 1), ("Complex", 2), ("Caves", 3), ("Library", 4),
               ("Basement", 5), ("Stack", 6), ("Facility", 7), ("Bunker", 8),
               ("Archives", 9), ("Caverns", 10), ("Egyptian", 11),
               ("Statue", 12), ("Cradle", 13), ("Runway", 14), ("Streets", 15),
               ("Surface", 16), ("Dam", 17), ("Depot", 18), ("Jungle", 19), ("Frigate", 20)]
MISSIONS = [("Dam", 33), ("Facility", 34), ("Runway", 35), ("Surface I", 36),
            ("Bunker I", 9), ("Silo", 20), ("Frigate", 26), ("Surface II", 43),
            ("Bunker II", 27), ("Statue", 22), ("Archives", 24), ("Streets", 29),
            ("Depot", 30), ("Train", 25), ("Jungle", 37), ("Control", 23),
            ("Caverns", 39), ("Cradle", 41), ("Aztec", 28), ("Egyptian", 32)]

# Recorded open-floor fixtures avoid a crowded cell and low jungle branches.
FIXTURES = {("mission", "Archives"): (-3136.105, 250.599, -4375.945, 0),
            ("multiplayer", "Jungle"): (-1219.783, 0, 980.159, 90)}

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("runtime", type=Path, help="Separate test runtime containing GE32.exe and sweep DLL")
    parser.add_argument("report", type=Path, help="Ignored output JSON; logs go beside it")
    parser.add_argument("--mode", choices=("all", "multiplayer", "mission"), default="all")
    parser.add_argument("--name", help="Run only this exact level name")
    args = parser.parse_args()
    runtime = args.runtime.resolve()
    if not (runtime / "GE32.exe").is_file():
        parser.error("Runtime must contain GE32.exe")
    args.report.parent.mkdir(parents=True, exist_ok=True)
    env_base = {k: v for k, v in os.environ.items() if not k.startswith("GE32_")}
    results = []
    groups = [("multiplayer", MULTIPLAYER), ("mission", MISSIONS)]
    for mode, maps in groups:
        if args.mode not in ("all", mode):
            continue
        for name, identifier in maps:
            if args.name and args.name != name:
                continue
            env = dict(env_base, GE32_LEVEL_SWEEP="1")
            if (mode, name) in FIXTURES:
                env["GE32_SWEEP_ANCHOR"] = ",".join(map(str, FIXTURES[mode, name]))
            command = [str(runtime / "GE32.exe")]
            if mode == "multiplayer":
                env["GE32_SWEEP_PRACTICE"] = str(identifier)
            else:
                command.append(f"-level_{identifier:02d}")
            logfile = runtime / "weapon-wheel.log"
            logfile.unlink(missing_ok=True)
            timed_out = False
            try:
                completed = subprocess.run(command, cwd=runtime, env=env, timeout=150,
                                           stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
                code = completed.returncode
            except subprocess.TimeoutExpired:
                timed_out, code = True, -1
            log = logfile.read_text(encoding="utf-8", errors="replace") if logfile.exists() else ""
            key = f"{mode}-{identifier:02d}"
            args.report.with_name(key + ".log").write_text(log, encoding="utf-8")
            expected = ({1: 38, 2: 31, 3: 50, 4: 48, 5: 45, 6: 46, 7: 34, 8: 27, 9: 24, 10: 39, 11: 32,
                         12: 22, 13: 41, 14: 35, 15: 29, 16: 36, 17: 33, 18: 30,
                         19: 37, 20: 26}[identifier]
                        if mode == "multiplayer" else identifier)
            result = dict(mode=mode, name=name, identifier=identifier, code=code,
                          timed_out=timed_out, passed=code == 0 and f"PASS LEVEL SWEEP stage={expected} " in log,
                          evidence=[line for line in log.splitlines()
                                    if any(word in line for word in ("PASS", "FAIL", "SKIP", "SWEEP start"))])
            results.append(result)
            args.report.write_text(json.dumps(results, indent=2), encoding="utf-8")
            print(f"{mode}: {name}: {'PASS' if result['passed'] else 'FAIL'}", flush=True)
    if not results:
        parser.error("No levels matched")
    raise SystemExit(0 if all(r["passed"] for r in results) else 1)

if __name__ == "__main__":
    main()

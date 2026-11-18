"""Replay the mantle survey in an isolated, prepared Windows game directory."""
import argparse
import os
import re
from pathlib import Path
import subprocess

MP_STAGES = [38, 31, 50, 48, 45, 46, 34, 27, 24, 39, 32, 22, 41, 35, 29, 36, 33, 30, 37, 26]


def read_routes(path):
    groups = {}
    for line in path.read_text().splitlines():
        if not line or line.startswith("#"):
            continue
        if line.startswith("[") and line.endswith("]"):
            current = groups[line[1:-1]] = []
        else:
            current.append(line)
    return groups


def replay(runtime, output_file, command, env, cases, stage):
    (runtime / "mantle-routes.txt").write_text("\n".join(cases) + "\n")
    log = runtime / "weapon-wheel.log"
    log.unlink(missing_ok=True)
    try:
        code = subprocess.run(command, cwd=runtime, env=env, timeout=160,
                              stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL).returncode
    except subprocess.TimeoutExpired:
        code = -1
    trace = log.read_text(errors="replace") if log.exists() else ""
    output_file.write_text(trace)
    complete = f"END routes={len(cases)} " in trace and trace.rstrip().endswith(f"stage={stage}")
    return code == 0 and complete, trace, complete


def with_prerequisite(case, cases):
    if case.split()[0] == "roof":
        return [next(row for row in cases if row.startswith("tap ")), case]
    return [case]


def second_press_landed(trace):
    pattern = r"^mantle (press|grab|land) stage=\d+ tick=(\d+) press=(\d+)"
    events = list(re.finditer(pattern, trace, re.MULTILINE))
    presses = [event for event in events if event[1] == "press"]
    grab = next((event for event in events if event[1] == "grab"), None)
    land = next((event for event in events if event[1] == "land"), None)
    if len(presses) != 2 or grab is None or land is None:
        return False
    first, second = presses
    flight = trace[first.start():second.start()]
    return (int(second[2]) - int(first[2]) > 18
            and "jump takeoff=" in flight and "jump landed=" not in flight
            and second.end() <= grab.start() < land.start() and grab[3] == second[3]
            and int(land[2]) > int(grab[2])
            and re.search(r"^PASS .* jumps=1 .* stable=1 ", trace, re.MULTILINE) is not None)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("runtime", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--setup", help="Select one setup, e.g. MP-7, Mission-34 or Barrier-7")
    parser.add_argument("--case", help="Replay one named case from a fresh load; requires --setup")
    parser.add_argument("--fresh", action="store_true", help="Start every case in a new game process")
    parser.add_argument("--second-press", action="store_true", help="Verify a fresh airborne A press caused the grab in each setup")
    args = parser.parse_args()
    runtime = args.runtime.resolve()
    manifest = "mantle-second-press.txt" if args.second_press else "mantle-routes.txt"
    groups = read_routes(Path(__file__).with_name(manifest))
    if args.setup and args.setup not in groups:
        parser.error("Unknown setup")
    if args.second_press and args.fresh:
        parser.error("--second-press already starts each route in a fresh game")
    if args.case and not args.setup:
        parser.error("--case requires --setup")
    if runtime == Path(__file__).resolve().parent or not (runtime / "GE32.exe").is_file():
        parser.error("Use an isolated directory containing GE32.exe")
    args.output.mkdir(parents=True, exist_ok=True)
    failed = False
    for name, cases in groups.items():
        if args.setup and name != args.setup:
            continue
        if name.startswith("Barrier-") and not args.setup:
            continue
        mode, number = name.split("-")
        env = {k: v for k, v in os.environ.items() if not k.startswith(("GE32_", "GE_"))}
        env.update(GE32_MANTLE_ROUTES="1", GE32_MANTLE_TEST="routes", GE_MPVIRT="1", GE32_SINGLEVIEW="1")
        command = [str(runtime / "GE32.exe")]
        if mode in ("MP", "Barrier"):
            stage = MP_STAGES[int(number) - 1]
            env["GE_STARTMP"] = "2," + number
        else:
            stage = int(number)
            command.append(f"-level_{stage:02d}")
        if args.second_press:
            env["GE32_MANTLE_TRACE"] = "1"
        if args.case:
            matches = [row for row in cases if row.split()[0] == args.case]
            if not matches:
                parser.error(f"Unknown case {args.case} in {name}")
            plan = with_prerequisite(matches[0], cases)
            ok, trace, _ = replay(runtime, args.output / f"{name}_{args.case}.log", command, env, plan, stage)
        elif args.fresh or (name == "MP-19" and not args.second_press):
            # Jungle practice can leave Bond unable to move after an earlier route.
            # Independent spawns preserve normal game rules and exercise every input.
            directory = args.output / (name + "_cases")
            directory.mkdir(exist_ok=True)
            verdicts = []
            ok = True
            finished = True
            for case in cases:
                label = case.split()[0]
                success, raw, complete = replay(runtime, directory / (label + ".log"), command, env,
                                                with_prerequisite(case, cases), stage)
                result = [line for line in raw.splitlines()
                          if line.startswith(tuple(f"{word} {label} " for word in ("PASS", "FAIL", "SKIP")))]
                verdicts.extend(result)
                ok &= success and len(result) == 1
                finished &= complete and len(result) == 1
            failures = sum(line.startswith("FAIL ") for line in verdicts)
            end = "END" if finished else "INCOMPLETE"
            trace = "\n".join(verdicts + [f"{end} routes={len(verdicts)} failures={failures} stage={stage}"]) + "\n"
            (args.output / (name + ".log")).write_text(trace)
        else:
            ok, trace, _ = replay(runtime, args.output / (name + ".log"), command, env, cases, stage)
        if args.second_press:
            ok &= second_press_landed(trace)
            print(name, "airborne second A verified" if ok else "airborne second A missed", flush=True)
        failed |= not ok
        print(name, trace.splitlines()[-1] if trace else "Missing trace", flush=True)
    return int(failed)


if __name__ == "__main__":
    raise SystemExit(main())

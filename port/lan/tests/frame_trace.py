"""Summarize the opt-in Egypt frame trace. All durations are wall time."""
import argparse
from bisect import bisect_left, bisect_right
from collections import Counter, defaultdict, deque
from pathlib import Path
from math import ceil
from statistics import mean, median
import struct

NAMES = ("", "vi", "gate", "game", "build", "seat", "vis", "props", "bg",
         "after_bg", "seat_end", "submit", "handoff", "render", "end",
         "queue", "ready", "scan", "acquired", "drain", "idle", "triangles", "shots", "view_draw", "list_begin", "list_end", "wake_mode", "wake_service", "tri4_cost",
         "cmd_queue", "wake_post", "irq_receive", "cmd_append", "wake_depth", "audio_start")


def read(path):
    data = path.read_bytes()
    magic, n, lost, size, hz = struct.unpack_from("<IIIIQ", data)
    if magic != 0x31464547 or size != 24 or lost or not hz or len(data) != 24 + n * size:
        raise ValueError("Incomplete, overflowing or incompatible trace")
    return hz, sorted(struct.iter_unpack("<QIIQ", data[24:]), key=lambda e: e[0])


def compare_view_draw(complete, scans, hz):
    """Assign each frame to its game-thread mode, including its first scanout."""
    first_scan, repeats = {}, Counter()
    for t, frame, surface in scans:
        if frame in first_scan:
            repeats[frame] += 1
        else:
            first_scan[frame] = t
    groups = defaultdict(list)
    shots = {r["render_id"]: r["shots"] for g,r in complete if "shots" in r}
    previous_frames = deque(maxlen=2)
    for g,r in complete:
        tag = g["view_draw"]
        rid = r["render_id"]
        if len(previous_frames) == 2 and all(previous == tag for previous in previous_frames):
            if not scans or (rid in first_scan and rid-1 in first_scan):
                groups[tag].append((g,r))
        previous_frames.append(tag)
    if len(groups) != 8 or any(tag // 2 != i for i,tag in enumerate(sorted(groups))):
        raise ValueError("A/B capture does not cover eight complete blocks")
    def report(label, frames):
        seat_ms = mean(g["seat_end",1] - g["seat",1] for g,r in frames)*1000/hz
        size = mean(g["bytes",1] for g,r in frames)
        result = f"{label} n={len(frames)} unused_ms={seat_ms:.4f} bytes={size:.0f}"
        if scans:
            elapsed = [first_scan[r["render_id"]]-first_scan[r["render_id"]-1] for g,r in frames]
            # A repeated observation holding the previous surface delays this frame.
            held = sum(repeats[r["render_id"]-1] for g,r in frames)
            p95 = sorted(elapsed)[ceil(.95*len(elapsed))-1]*1000/hz
            triangles = mean(r["triangles"] for g,r in frames)
            fired = sum(shots[r["render_id"]]-shots[r["render_id"]-1] for g,r in frames)
            result += (f" display_fps={hz*len(frames)/sum(elapsed):.2f} repeats={held}"
                       f" p95_ms={p95:.2f} tris={triangles:.0f} shots={fired}")
        print(result)
    for tag,frames in sorted(groups.items()):
        report(f"block {tag//2+1} {'B' if tag&1 else 'A'}", frames)
    for mode in (0,1):
        report("pooled " + ("B" if mode else "A"),
               [frame for tag,frames in groups.items() if tag%2 == mode for frame in frames])


def summarize(path, boot=False):
    hz, events = read(path)
    games, renders = defaultdict(dict), defaultdict(dict)
    submissions, handoffs = defaultdict(deque), defaultdict(deque)
    scans, gates = [], Counter()
    raster_samples = {}
    vi_posts = 0
    last_irq = (0, 0)
    received_frame = None
    wake_frames = deque()
    wake_depths, audio_starts = Counter(), []
    wake_services = Counter()
    wake_dispatches = []
    early_gate_times = []
    byte_counts = defaultdict(list)
    for t, kind, frame, arg in events:
        name = NAMES[kind]
        if name == "vi":
            vi_posts += 1
            continue
        if name == "gate":
            gates[(arg >> 1, bool(arg & 1))] += 1
            if arg & 1:
                early_gate_times.append(t)
        elif name in ("game", "build", "submit"):
            games[frame][name] = t
            if name == "submit":
                submissions[arg].append(frame)
        elif name in ("seat", "vis", "props", "bg", "after_bg", "seat_end"):
            seat = arg >> 32 if name == "seat_end" else arg
            games[frame][name, seat] = t
            if name == "seat_end":
                games[frame]["bytes", seat] = arg & 0xffffffff
                byte_counts[seat].append(arg & 0xffffffff)
        elif name == "cmd_queue":
            games[frame]["queued"] = t
        elif name == "wake_post":
            games[frame]["post"] = t
            if arg & 1:
                wake_frames.append(frame)
        elif name == "irq_receive":
            last_irq = (t, arg)
            received_frame = None
            if arg in (670, 671) and wake_frames:
                received_frame = wake_frames.popleft()
                games[received_frame]["wake_received"] = t
        elif name == "cmd_append":
            if submissions[arg]:
                g = games[submissions[arg][0]]
                g["append"] = t
                g["receiver"], g["interrupt"] = last_irq
        elif name == "wake_depth":
            wake_depths[last_irq[1], arg >> 1, bool(arg & 1)] += 1
            if received_frame is not None:
                games[received_frame]["mixed"] = bool(arg & 1)
        elif name == "audio_start":
            audio_starts.append(t)
        elif name == "wake_service":
            wake_services[arg] += 1
            if arg == 2:
                wake_dispatches.append(t)
        elif name == "wake_mode":
            games[frame][name] = arg
        elif name == "view_draw":
            games[frame][name] = arg
        elif name == "handoff":
            handoffs[arg].append((submissions[arg].popleft() if submissions[arg] else None, t))
        elif name == "render":
            r = renders[frame]
            r["render_id"] = frame
            r["game_id"], r["handoff"] = handoffs[arg].popleft() if handoffs[arg] else (None, t)
            r[name] = t
        elif name == "scan":
            raster_samples[t] = arg >> 32
            scans.append((t, frame, arg & 0xffffffff))
        elif name == "list_begin":
            renders[frame][name] = t
            renders[frame]["packed_tag"] = arg
        elif name == "tri4_cost":
            renders[frame]["tri4_count"] = arg >> 32
            renders[frame]["tri4_ticks"] = arg & 0xffffffff
        elif name == "list_end":
            renders[frame][name] = t
            renders[frame]["draw_ticks"] = arg
        elif name in ("shots", "triangles"):
            renders[frame][name] = arg
        else:
            renders[frame][name] = t
            if name in ("queue", "ready"):
                renders[frame][name + "_surface"] = arg
    # Capture can cut a frame at either end. Require complete joins in the interior.
    ids = sorted(renders)[2:-2]
    complete = []
    for rid in ids:
        r = renders[rid]
        g = games.get(r.get("game_id"), {})
        required = ["game", "submit"] if boot else ["game", "build", "submit"] + [(k, s) for s in (0, 1)
                    for k in ("seat", "vis", "props", "bg", "after_bg", "seat_end")]
        if not all(k in g for k in required) or not g["game"] <= g.get("build", g["game"]) <= g["submit"] <= r["handoff"] <= r["render"] <= r["end"]:
            raise ValueError(f"Broken game/task/render join for frame {rid}")
        if scans and not r["render"] <= r["queue"] <= r["ready"]:
            raise ValueError(f"Broken surface join for frame {rid}")
        complete.append((g, r))
    if not complete:
        raise ValueError("No complete frames")
    def ms(x): return 1000 * x / hz
    print(f"{len(complete)} complete frames; {len(events)} events; zero overflow")
    print("Software VI posts:", vi_posts, "gate visits (pending, too early):", dict(gates))
    has_lists = any("list_begin" in r for g,r in complete)
    if has_lists:
        totals, draws = [], []
        for g,r in complete:
            span = r["list_end"] - r["list_begin"]
            draw = r["draw_ticks"]
            if not 0 <= draw <= span or not r["render"] <= r["list_begin"] <= r["list_end"] <= r["end"]:
                raise ValueError("Invalid display-list or draw span")
            totals.append(span)
            draws.append(draw)
        print(f"Visible list: backend draw {ms(mean(draws)):.3f} ms, "
              f"list outside draw {ms(mean(totals)-mean(draws)):.3f} ms per frame")
    default_reuse = sum(r.get("packed_tag") == 1 for g, r in complete)
    if default_reuse:
        print(f"Default material reuse after vertex load: {default_reuse}/{len(complete)} frames")
    wake_test = any(g.get("wake_mode") for g,r in complete)
    if wake_test:
        print("Scheduler wakes (0 stale/mixed, 1 waiting for resource, 2 dispatched, 3 waiting for readiness):", dict(wake_services))
    compared_mode = {}
    if has_lists and (wake_test or any(r["packed_tag"] >= 16 for g,r in complete)):
        samples = defaultdict(list)
        last = deque(maxlen=2)
        first = {}
        scan_counts = Counter()
        for t,frame,_ in scans:
            first.setdefault(frame,t)
            scan_counts[frame] += 1
        for g,r in complete:
            tag = g.get("wake_mode", 0) if wake_test else r["packed_tag"]
            if tag >= 16 and len(last) == 2 and all(k == tag for k in last):
                samples[tag].append(r)
                compared_mode[r["render_id"]] = tag & 1
            last.append(tag)
        if len(samples) != 8:
            raise ValueError("Incomplete render comparison")
        gap_ends = []
        if len(scans) >= 2:
            period = median(b[0] - a[0] for a, b in zip(scans, scans[1:]))
            gap_ends = [b[0] for a, b in zip(scans, scans[1:]) if b[0] - a[0] > 1.5 * period]
        for mode in (0,1):
            rows = [r for tag,group in samples.items() if (tag & 1) == mode for r in group]
            total = mean(r["list_end"] - r["list_begin"] for r in rows)
            draw = mean(r["draw_ticks"] for r in rows)
            timing = ""
            if first:
                spans = [first[r["render_id"]]-first[r["render_id"]-1] for r in rows]
                repeats = sum(scan_counts[r["render_id"]-1]-1 for r in rows)
                # Attribute a missing interrupt sample to the same display interval as its repeats.
                missing_samples = sum(bisect_right(gap_ends, first[r["render_id"]]) -
                                      bisect_right(gap_ends, first[r["render_id"]-1]) for r in rows)
                ordered = sorted(spans)
                block_rates = []
                for tag, group in sorted(samples.items()):
                    if (tag & 1) == mode:
                        intervals = [first[r["render_id"]] - first[r["render_id"]-1] for r in group]
                        block_rates.append(f"{hz/mean(intervals):.2f}")
                timing = (f", displayed {hz/mean(spans):.2f} FPS, {repeats} observed repeats, "
                          f"{missing_samples} scanout gaps, p95 "
                          f"{ms(ordered[ceil(.95*len(spans))-1]):.2f} ms, p99 "
                          f"{ms(ordered[ceil(.99*len(spans))-1]):.2f} ms, longest "
                          f"{ms(ordered[-1]):.2f} ms, {mean(r['triangles'] for r in rows):.0f} tris/frame, "
                          f"block FPS [{', '.join(block_rates)}]")
            if wake_test:
                timing += f", submit to handoff {ms(mean(r['handoff'] - games[r['game_id']]['submit'] for r in rows)):.3f} ms"
            if wake_test and min(samples) >= 80:
                label = "Mixed command wake"
                def report_handoff(group, title):
                    parts = [[], [], [], []]
                    handlers = Counter()
                    for render in group:
                        game = games[render["game_id"]]
                        receive = max(game["queued"], game["receiver"])
                        edges = (game["submit"], game["queued"], receive, game["append"], render["handoff"])
                        if list(edges) != sorted(edges):
                            raise ValueError(f"Unordered command path in frame {render['render_id']}")
                        for part, a, b in zip(parts, edges, edges[1:]):
                            part.append(b-a)
                        handlers[game["interrupt"]] += 1
                    if group:
                        values = ", ".join(f"{ms(mean(part)):.3f}" for part in parts)
                        print(f"  {title} ({len(group)}): submit to queue, queue to servicing interrupt, "
                              f"interrupt to append, append to handoff (ms): {values}; handlers {dict(handlers)}")
                report_handoff(rows, f"Mode {mode} all")
                if first:
                    for long in (False, True):
                        clean = [r for r, span in zip(rows, spans) if (span > 1.5 * period) == long and
                                 bisect_right(gap_ends, first[r["render_id"]]) ==
                                 bisect_right(gap_ends, first[r["render_id"]-1])]
                        report_handoff(clean, "Long" if long else "Single refresh")
                print("  Wake snapshots (depth, mixed-pair):", {str((depth, mixed)): n for
                      (irq, depth, mixed), n in wake_depths.items() if irq == 670 + mode})
                dispatch = [games[r["game_id"]] for r in rows]
                delays = [g["wake_received"]-g["post"] for g in dispatch if "wake_received" in g]
                if delays:
                    print(f"  Wake post to receive {ms(mean(delays)):.3f} ms ({len(delays)} paired posts)")
                mixed = [g for g in dispatch if g.get("mixed") and g["append"] >= g["wake_received"]]
                if mixed:
                    print(f"  Mixed wake receive to graphics append {ms(mean(g['append'] - g['wake_received'] for g in mixed)):.3f} "
                          f"ms ({len(mixed)} frames)")
                audio = sum(bisect_left(audio_starts, games[r["game_id"]]["submit"]) -
                            bisect_left(audio_starts, games[r["game_id"]-1]["submit"]) for r in rows)
                print(f"  Audio starts across included submission intervals: {audio}")
            elif wake_test:
                label = "Early wake"
            elif min(samples) >= 64:
                label = "TRI4 projected positions"
                for r in rows:
                    if not 0 <= r["tri4_ticks"] < 0xffffffff or r["tri4_ticks"] > r["list_end"] - r["list_begin"] - r["draw_ticks"]:
                        raise ValueError(f"Invalid exclusive TRI4 span in frame {r['render_id']}")
                cost = ms(mean(r["tri4_ticks"] for r in rows))
                commands = mean(r["tri4_count"] for r in rows)
                timing += f", TRI4 exclusive {cost:.3f} ms, {commands:.1f} commands/frame"
                if commands:
                    timing += f", {1000 * cost / commands:.3f} us/command"
                if first:
                    slow = [r for r, span in zip(rows, spans) if span > 1.5 * period]
                    if slow:
                        with_gap = sum(bisect_right(gap_ends, first[r["render_id"]]) >
                                       bisect_right(gap_ends, first[r["render_id"]-1]) for r in slow)
                        timing += (f", {len(slow)} long intervals ({with_gap} span a scanout gap), "
                                   f"averaging {ms(mean(r['tri4_ticks'] for r in slow)):.3f} ms TRI4")
            elif min(samples) >= 48:
                label = "Material reuse after vertex load"
            else:
                label = "Packed"
            print(f"{label} {mode}: {len(rows)} frames, list excluding draw {ms(total-draw):.3f} ms, "
                  f"draw {ms(draw):.3f} ms{timing}")
            if 64 <= min(samples) < 80 and first:
                # A gap can make the first observation of a new surface late. Use only
                # intervals with consecutive refresh samples for deadline comparisons.
                for long_interval in (False, True):
                    group = [r for r, span in zip(rows, spans) if (span > 1.5 * period) == long_interval
                             and bisect_right(gap_ends, first[r["render_id"]]) ==
                                 bisect_right(gap_ends, first[r["render_id"] - 1])]
                    if not group:
                        continue
                    available = mean(first[r["render_id"]-1] + period - r["list_begin"] for r in group)
                    tri4 = mean(r["tri4_ticks"] for r in group)
                    draw = mean(r["draw_ticks"] for r in group)
                    remainder = mean(r["list_end"] - r["list_begin"] for r in group) - tri4 - draw
                    handoff = mean(r["handoff"] - games[r["game_id"]]["submit"] for r in group)
                    prior_work = mean(max(0, min(r["handoff"], renders[r["render_id"]-1]["end"]) -
                                          games[r["game_id"]]["submit"]) for r in group)
                    direct = 0
                    for r in group:
                        idx = bisect_right(wake_dispatches, r["handoff"]) - 1
                        if idx >= 0 and wake_dispatches[idx] >= games[r["game_id"]]["submit"] and r["handoff"] - wake_dispatches[idx] < hz / 2000:
                            direct += 1
                    print(f"  {'Long' if long_interval else 'Single-refresh'} ({len(group)} frames, no gaps): "
                          f"list start to target refresh {ms(available):.3f} ms, submit to handoff "
                          f"{ms(handoff):.3f} ms ({ms(prior_work):.3f} ms overlapping previous render), "
                          f"TRI4 {ms(tri4):.3f} ms, draw {ms(draw):.3f} ms, "
                          f"other list {ms(remainder):.3f} ms, {direct} handoffs within 0.5 ms of graphics wake dispatch")
    if boot:
        if scans:
            first, last = ids[0], ids[-1]
            middle = [(t,i,s) for t,i,s in scans if first <= i <= last]
            if not middle or any(renders[i]["queue_surface"] != s or renders[i]["ready_surface"] != s
                                 or t < renders[i]["ready"] for t,i,s in middle):
                raise ValueError("Boot scanout surface or ready timestamp mismatch")
            displayed = [i for _,i,_ in middle]
            if displayed != sorted(displayed) or set(displayed) != set(ids):
                raise ValueError("Missing or reordered boot frame")
            print(f"Verified {len(ids)} game/task/render/surface IDs at {len(middle)} scanouts")
        return
    for s in (0, 1):
        marks = ("seat", "vis", "props", "bg", "after_bg", "seat_end")
        spans = [(a, b, ms(mean(g[b, s]-g[a, s] for g, _ in complete)))
                 for a, b in zip(marks, marks[1:])]
        print(f"Seat {s} total {sum(v for _,_,v in spans):.3f} ms, "
              f"{mean(byte_counts[s]):.0f} list bytes/frame;", " ".join(f"{a}->{b} {v:.3f}" for a,b,v in spans))
    print("Mean ms: game->submit", round(ms(mean(g["submit"]-g["game"] for g,r in complete)),3),
          "submit->render",round(ms(mean(r["render"]-g["submit"] for g,r in complete)),3),
          "render->end", round(ms(mean(r["end"]-r["render"] for g,r in complete)),3))
    print("Submit->handoff", round(ms(mean(r["handoff"]-g["submit"] for g,r in complete)),3),
          "handoff->render", round(ms(mean(r["render"]-r["handoff"] for g,r in complete)),3))
    if len({g.get("view_draw", 0) for g,r in complete}) > 1:
        compare_view_draw(complete, scans, hz)
    if not scans:
        return
    for a,b in zip(("render","acquired","drain","idle","queue"),("acquired","drain","idle","queue","ready")):
        print(f"{a}->{b} {ms(mean(r[b]-r[a] for _,r in complete)):.3f} ms")
    print("Triangles/frame", round(mean(r["triangles"] for _,r in complete)),
          "shots during capture", complete[-1][1]["shots"]-complete[0][1]["shots"])
    valid = set(ids)
    joined = [j for j,(_,i,_) in enumerate(scans) if i in valid]
    if not joined:
        raise ValueError("No scanout correlated to a rendered frame")
    scans = scans[joined[0]:joined[-1]+1]
    for t,i,s in scans:
        if i not in valid or renders[i]["queue_surface"] != s or renders[i]["ready_surface"] != s or t < renders[i]["ready"]:
            raise ValueError(f"Unmatched scanout generation {i} on surface {s}")
    if len(scans) < 3:
        raise ValueError("No joined scanout")
    seen = set()
    reasons, held = Counter(), Counter()
    reasons_by_mode = defaultdict(Counter)
    late_ms, changes = [], []
    run = 1
    for idx in range(1,len(scans)):
        t,cur,s = scans[idx]
        prev = scans[idx-1][1]
        if cur != prev:
            if changes:
                held[run] += 1
            changes.append(t)
            run = 1
            seen.add(cur)
            if cur < prev: raise ValueError("Scanout went backwards")
            continue
        run += 1
        following = next((i for _,i,_ in scans[idx+1:] if i != cur),None)
        if following is None: continue
        r = renders[following]; g = games[r["game_id"]]
        if t < g["game"]: why = "before accepted game tick"
        elif t < g["submit"]: why = "next game frame under construction"
        elif t < r["handoff"]: why = "submitted, waiting for scheduler handoff"
        elif t < r["render"]: why = "handed off, waiting for render worker"
        elif t < r["acquired"]: why = "renderer acquiring back buffer"
        elif t < r["drain"]: why = "renderer processing display list and submitting draws"
        elif t < r["idle"]: why = "tail GPU drain"
        elif t < r["queue"]: why = "GPU idle, waiting to queue"
        elif t < r["ready"]: why = "queued, GPU completion not yet published"
        else: why = "GPU ready, still displaying previous surface"
        if has_lists and r["list_begin"] <= t < r["list_end"]:
            sample = raster_samples[t]
            if sample >> 2 == following and sample & 3 in (1, 2, 3):
                why = {1: "other display-list work" if "tri4_ticks" in r else "display-list work outside draw",
                       2: "draw_triangles callback", 3: "TRI4 outside draw"}[sample & 3]
            else:
                why = "display-list phase boundary or unmatched ISR sample"
        reasons[why] += 1
        if following in compared_mode:
            reasons_by_mode[compared_mode[following]][why] += 1
        late_ms.append(ms(r["ready"]-t))
    scan_times = [t for t,_,_ in scans]
    gaps = [b-a for a,b in zip(scan_times,scan_times[1:])]
    period = median(gaps)
    print("Observed gaps longer than 1.5 refreshes:", sum(gap > 1.5*period for gap in gaps))
    early = [bisect_left(scan_times,t) for t in early_gate_times if scan_times[0] < t <= scan_times[-1]]
    paired = {j for j in early if scans[j][1] == scans[j-1][1]}
    print("Early gate followed by a repeat at next observed refresh:", len(paired), "distinct repeats;", len(early), "gate rejections in interval")
    span = (scans[-1][0]-scans[0][0])/hz
    print("Scanouts",len(scans),"new frame rate",round(len(seen)/span,2),"Hz",round((len(scans)-1)/span,2))
    print("Refresh periods per displayed frame (complete intervals):",dict(sorted(held.items())))
    def percentiles(samples):
        ordered = sorted(samples)
        return [round(ordered[ceil(len(ordered)*p)-1], 3) for p in (.5, .95, .99)]
    if len(changes) >= 2:
        print("Display interval p50/p95/p99 ms:", percentiles([ms(b-a) for a,b in zip(changes,changes[1:])]))
    if late_ms:
        print("Next ready minus repeated refresh p50/p95/p99 ms:", percentiles(late_ms))
    print("Next frame at repeated refresh:",dict(reasons))
    for mode in sorted(set(compared_mode.values())):
        print(f"Repeated refreshes in comparison mode {mode}:", dict(reasons_by_mode[mode]))
    print("Interior rendered IDs absent from scanout:",len(set(range(scans[0][1]+1, scans[-1][1]))-seen))


if __name__ == "__main__":
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("trace", type=Path)
    ap.add_argument("--boot", action="store_true", help="validate boot frame identity without multiplayer seats")
    args = ap.parse_args()
    summarize(args.trace, args.boot)

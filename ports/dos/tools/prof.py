#!/usr/bin/env python3
"""prof.py: what a session costs on a Tandy 1000 EX (the model of m8088.py), per action and per routine.

  prof.py run [--build orig|ref|game ...] [--session NAME ...]     play sessions, write build/prof/<build>/<session>.json
  prof.py report --build B [--session S] [--top 30]               where the time goes
  prof.py actions --build B --session S                           what each key cost
  prof.py diff A B [--session S]                                  per-action medians, A against B
"""
from __future__ import annotations

import argparse
import json
import statistics
import sys
import time
from collections import defaultdict
from concurrent.futures import ProcessPoolExecutor
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import tandybox as T  # noqa: E402

PORT = HERE.parent
OUT = PORT / ("build/prof" + ("" if T.m8088.BUS == 5 else "-bus%d" % T.m8088.BUS))
SESSIONS = PORT / "sessions"
SUITE = ["walk", "combat", "panels", "spell", "menus", "intro"]
WATCH = ("_AnimateParticleBurst", "_AnimateConeOfCold", "_Dissolve", "TGA.DissolvePixels", "_RevealInStrips", "_GrowFrame")
WAITS = {"_WaitTicks", "_WaitRetrace", "_IsInVerticalRetrace", "WaitFrames", "_WaitFrames"}


def sec(c):
    return c / T.CLOCK


def profile(build: str, session: str, shots: bool = False, limit: int = 1200) -> dict:
    text = (SESSIONS / (session + ".keys")).read_text()
    probe = T.Session(text, None)
    save = probe.options.get("save")
    b = T.get_build(build, probe.options.get("program", "EOB.EXE"))
    m = T.TandyMachine(b, save=(T.ROOT / "tests/sessions" / save) if save else None, watch=WATCH)
    actions = []
    last = {}

    def snap():
        return dict(m.per)

    def on_action(label, t0, t1):
        now = snap()
        reads = [r for r in m.reads_log if t0 <= r[0] <= t1]
        routines = {m.routine_names[k]: v - last.get(k, 0) for k, v in now.items() if v != last.get(k, 0)}
        wait = sum(v for k, v in routines.items() if k in WAITS)
        actions.append({"label": label, "latency": t1 - t0, "start": t0, "wait": wait,
                        "routines": dict(sorted(routines.items(), key=lambda kv: -kv[1])[:60]),
                        "files": len(reads), "bytes": sum(r[3] for r in reads)})
        last.clear()
        last.update(now)

    shotdir = None
    if shots:
        shotdir = OUT / build / (session + "-shots")
        shotdir.mkdir(parents=True, exist_ok=True)
    s = T.Session(text, m, on_action=on_action, shots=shotdir)
    orig = s.service

    def service(mach):
        if s.t_key is not None and s.t_key == mach.insns:
            pass
        orig(mach)
    s.attach()
    t0 = time.time()
    # the baseline of an action starts when its key goes in
    keep = s.service

    def keyed(mach):
        before = s.t_key
        keep(mach)
        if s.t_key is not None and s.t_key != before:
            last.clear()
            last.update(snap())
    m.keys = type("K", (), {"service": staticmethod(keyed), "progress": staticmethod(s.progress)})
    reason = m.run_clocked(limit * T.CLOCK)
    res = {"build": build, "session": session, "reason": reason, "script_done": s.done(), "cycles": m.insns,
           "bus": m.bus_total, "actions": actions, "host_seconds": time.time() - t0, "files": len(m.reads_log),
           "routines": {m.routine_names[k]: v for k, v in sorted(m.per.items(), key=lambda kv: -kv[1])},
           "spans": {k: v for k, v in m.spans.items()}}
    out = OUT / build / (session + ".json")
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(json.dumps(res))
    return res


def load(build, session):
    return json.loads((OUT / build / (session + ".json")).read_text())


def kind(label: str) -> str:
    return label.split(" ")[0]


def report(build, sessions, top, skip=()):
    tot = defaultdict(int)
    allc = 0
    for s in sessions:
        r = load(build, s)
        acts = [a for a in r["actions"] if kind(a["label"]) not in skip]
        print("%-12s %3d actions, %8.2f s of latency, %s" % (s, len(acts), sec(sum(a["latency"] for a in acts)), r["reason"]))
        for a in acts:
            allc += a["latency"]
            for k, v in a["routines"].items():
                tot[k] += v
    print("\nroutines by cycles during actions (%.2f s):" % sec(allc))
    for k, v in sorted(tot.items(), key=lambda kv: -kv[1])[:top]:
        print("  %6.2f %%  %8.3f s  %s" % (100 * v / allc, sec(v), k))


def actions(build, session, detail):
    r = load(build, session)
    for a in r["actions"]:
        print("  %8.3f s  (wait %6.3f)  files %3d %8d bytes  %s" % (sec(a["latency"]), sec(a["wait"]), a["files"], a["bytes"], a["label"]))
        for k, v in list(a["routines"].items())[:detail]:
            print("        %7.3f s  %s" % (sec(v), k))


def groups(build, sessions):
    g = defaultdict(list)
    for s in sessions:
        for a in load(build, s)["actions"]:
            g[s + ":" + kind(a["label"])].append(a["latency"])
    return g


def diff(a, b, sessions):
    ga, gb = groups(a, sessions), groups(b, sessions)
    print("%-22s %5s %10s %10s %8s" % ("action", "n", a + " (s)", b + " (s)", "ratio"))
    for k in sorted(ga):
        if k in gb:
            x, y = statistics.median(ga[k]), statistics.median(gb[k])
            print("%-22s %5d %10.3f %10.3f %7.2fx" % (k, len(ga[k]), sec(x), sec(y), x / max(1, y)))


def spans(a, b, sessions):
    """The watched routines (a spell's particles, the intro's dissolves): the time of each run, from its first instruction to
    its last. A dissolve is the C routine (the shuffle) and, on the patched build, the register loop it calls, as one run."""
    def merged(r):
        out = {}
        for name, runs in r.items():
            key = "_Dissolve" if name == "TGA.DissolvePixels" else name
            out.setdefault(key, []).extend(runs)
        for runs in out.values():
            runs.sort()
            i = 0
            while i + 1 < len(runs):
                if runs[i + 1][0] - runs[i][1] < T.WATCH_GAP:
                    runs[i][1] = runs[i + 1][1]
                    runs[i][3] = runs[i + 1][3]
                    del runs[i + 1]
                else:
                    i += 1
        return out
    for s in sessions:
        ra, rb = merged(load(a, s).get("spans", {})), merged(load(b, s).get("spans", {}))
        for name in sorted(ra):
            x, y = ra[name], rb.get(name, [])
            for i, p in enumerate(x):
                q = y[i] if i < len(y) else None
                print("%-8s %-22s run %2d  %7.3f s  ->  %s" % (s, name, i, sec(p[1] - p[0]),
                      "%7.3f s   %.2fx" % (sec(q[1] - q[0]), (p[1] - p[0]) / max(1, q[1] - q[0])) if q else "(no run)"))


def _job(a):
    r = profile(*a)
    return a, r["reason"], r["script_done"], r["host_seconds"]


def main(argv=None):
    ap = argparse.ArgumentParser()
    ap.add_argument("cmd", choices=["run", "report", "actions", "diff", "spans"])
    ap.add_argument("rest", nargs="*")
    ap.add_argument("--build", action="append")
    ap.add_argument("--session", action="append")
    ap.add_argument("--top", type=int, default=30)
    ap.add_argument("--detail", type=int, default=0)
    ap.add_argument("--shots", action="store_true")
    ap.add_argument("--limit", type=int, default=1200)
    ap.add_argument("--skip", action="append", default=[])
    a = ap.parse_args(argv)
    sessions = a.session or SUITE
    if a.cmd == "run":
        jobs = [(b, s, a.shots, a.limit) for b in (a.build or ["orig"]) for s in sessions]
        with ProcessPoolExecutor(max_workers=6) as ex:
            for j, reason, done, host in ex.map(_job, jobs):
                print("%s/%s: %s (script %s) host %.0f s" % (j[0], j[1], reason, "done" if done else "NOT DONE", host), flush=True)
    elif a.cmd == "report":
        report(a.build[0], sessions, a.top, set(a.skip))
    elif a.cmd == "actions":
        for s in sessions:
            actions(a.build[0], s, a.detail)
    elif a.cmd == "diff":
        diff(a.rest[0], a.rest[1], sessions)
    elif a.cmd == "spans":
        spans(a.rest[0], a.rest[1], sessions)


if __name__ == "__main__":
    main()

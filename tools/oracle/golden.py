#!/usr/bin/env python3
"""golden.py: record and verify golden sessions (playbook 25, SPEC 6).

A session is a script in tests/sessions/<name>.keys (tools/oracle/input.py). Recording runs it twice;
the two runs must agree on every checkpoint, or nothing is written. The golden is
tests/sessions/golden/<name>.json (instructions, state digest and image hash of every checkpoint and of
the end of the run) with the screenshots beside it in tests/sessions/golden/<name>/ for people to look at.

  golden.py [--check] [NAME ...]      record (default) or verify; no NAME: every session
  golden.py --check --rebuilt [NAME ...]
                                      verify on the programs `make rebuild` linked (Phase 5, playbook 50):
                                      the same instructions, screens, sound and state digests, the state
                                      read in the original's form (tools/oracle/layout.py)
  golden.py --clock [--check] [NAME ...]
                                      record or verify the C-clock recording of an EOB.EXE session,
                                      tests/sessions/golden/<name>.clock.json (playbook 80, tools/oracle/cclock.py):
                                      the session is played once more with the clock attached and must still
                                      be its golden
Exit code 0 when everything recorded or verified, 1 otherwise. A rebuilt run that differs is played
again on the originals, and the first checkpoint whose state differs is reported with the memory that
differs (in the original's form).
"""
from __future__ import annotations

import argparse
import json
import shutil
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from oracle.machine import ROOT  # noqa: E402
from oracle.input import Script  # noqa: E402
from oracle import run, layout  # noqa: E402

SESSIONS = ROOT / "tests" / "sessions"
GOLDEN = SESSIONS / "golden"
KEEP = ("program", "stop", "instructions", "video_mode", "digest", "final_image", "script_done", "sound_digest", "adlib_writes")


def play(name: str, out: Path, memory: list | None = None) -> dict:
    """One run of the session; with `memory`, a copy of memory (in the original's form) at every
    state digest is appended to it, for `first_difference`."""
    text = (SESSIONS / ("%s.keys" % name)).read_text()
    opts = Script(text).options
    save = SESSIONS / opts["save"] if "save" in opts else None
    build = run.build_machine
    if memory is not None:
        def build_and_watch(*a, **k):
            m = build(*a, **k)
            digest = m.state_digest

            def watched(*aa, **kk):
                mem = m.uc.mem_read(0x500, 0xA0000 - 0x500)
                if m.canonical is not None:
                    m.canonical(m, mem)
                memory.append((m.insns, bytes(mem)))
                return digest(*aa, **kk)
            m.state_digest = watched
            return m
        run.build_machine = build_and_watch
    try:
        s = run.run_session(text, out=out, max_insns=int(opts.get("insns", 60_000_000)),
                            mouse="mouse" in opts, sound=opts.get("sound"), save=save,
                            program=opts.get("program", "EOB.EXE"))
    finally:
        run.build_machine = build
    g = {k: s[k] for k in KEEP}
    g["checkpoints"] = [{k: c[k] for k in ("kind", "name", "instructions", "digest", "image") if k in c}
                        for c in s["checkpoints"]]
    g["args"] = s["args"]
    return g


def play_clock(name: str, out: Path) -> tuple[dict, dict] | None:
    """The session with the C-unit clock attached: (its golden form, the clock recording); None for a
    session of another program (the C clock is EOB.EXE's for now)."""
    from oracle.cclock import CClock
    text = (SESSIONS / ("%s.keys" % name)).read_text()
    opts = Script(text).options
    if opts.get("program", "EOB.EXE") != "EOB.EXE":
        return None
    save = SESSIONS / opts["save"] if "save" in opts else None
    build = run.build_machine
    held = {}

    def build_and_clock(*a, **k):
        m = build(*a, **k)
        cc = held["cc"] = CClock(m)
        digest = m.state_digest

        def counted(*aa, **kk):
            held.setdefault("at", []).append([m.insns, cc.count])
            return digest(*aa, **kk)
        m.state_digest = counted
        return m
    run.build_machine = build_and_clock
    try:
        s = run.run_session(text, out=out, max_insns=int(opts.get("insns", 60_000_000)),
                            mouse="mouse" in opts, sound=opts.get("sound"), save=save)
    finally:
        run.build_machine = build
    g = {k: s[k] for k in KEEP}
    g["checkpoints"] = [{k: c[k] for k in ("kind", "name", "instructions", "digest", "image") if k in c}
                        for c in s["checkpoints"]]
    g["args"] = s["args"]
    rec = held["cc"].recording()
    names = [c["name"] for c in s["checkpoints"]] + ["end"]
    rec["checkpoints"] = [[n, i, c] for n, (i, c) in zip(names, held.get("at", []))]
    return g, {"session": name, **rec}


def write_clock(path: Path, rec: dict) -> None:
    """One line per entry, so a change reads as a diff."""
    lines = ["{"]
    keys = list(rec)
    for j, k in enumerate(keys):
        v = rec[k]
        end = "," if j < len(keys) - 1 else ""
        if isinstance(v, list):
            lines.append(' "%s": [' % k)
            lines += ["%s%s" % (json.dumps(x, separators=(",", ":")), "," if i < len(v) - 1 else "")
                      for i, x in enumerate(v)]
            lines.append(" ]%s" % end)
        else:
            lines.append(' "%s": %s%s' % (k, json.dumps(v), end))
    lines.append("}")
    path.write_text("\n".join(lines) + "\n")


def clock(name: str, verify: bool) -> bool:
    p = GOLDEN / ("%s.clock.json" % name)
    with tempfile.TemporaryDirectory() as tmp:
        r = play_clock(name, Path(tmp))
    if r is None:
        print("%s: not an EOB.EXE session, no C clock" % name)
        return True
    g, rec = r
    diff = differences(json.loads((GOLDEN / ("%s.json" % name)).read_text()), g)
    if diff:
        print("%s: the run with the clock is not its golden: %s" % (name, "; ".join(diff)))
        return False
    if verify:
        if not p.exists():
            print("%s: no clock recording (make golden CLOCK=1 NAME=%s)" % (name, name))
            return False
        old = json.loads(p.read_text())
        bad = [k for k in rec if old.get(k) != rec[k]]
        print("%s: clock %s" % (name, "OK" if not bad else "DIFFERS: " + ", ".join(bad)))
        return not bad
    write_clock(p, rec)
    print("%s: clock recorded: %d C-unit instructions of %d, %d interrupts, %d clock changes over %d accesses" % (
        name, rec["checkpoints"][-1][2], rec["checkpoints"][-1][1], len(rec["interrupts"]), len(rec["times"]),
        rec["accesses"]))
    return True


def differences(a: dict, b: dict) -> list[str]:
    out = ["%s: %r != %r" % (k, a.get(k), b.get(k)) for k in KEEP + ("args",) if a.get(k) != b.get(k)]
    for x, y in zip(a["checkpoints"], b["checkpoints"]):
        if x != y:
            out.append("checkpoint %s differs" % x["name"])
    if len(a["checkpoints"]) != len(b["checkpoints"]):
        out.append("%d checkpoints != %d" % (len(a["checkpoints"]), len(b["checkpoints"])))
    return out


def record(name: str) -> bool:
    with tempfile.TemporaryDirectory() as tmp:
        g1 = play(name, Path(tmp) / "1")
        g2 = play(name, Path(tmp) / "2")
        diff = differences(g1, g2)
        if diff:
            print("%s: two runs disagree, nothing recorded: %s" % (name, "; ".join(diff)))
            return False
        if g1["stop"].startswith(("unimplemented", "guest", "oracle")):
            print("%s: the run failed (%s), nothing recorded" % (name, g1["stop"]))
            return False
        GOLDEN.mkdir(parents=True, exist_ok=True)
        (GOLDEN / ("%s.json" % name)).write_text(json.dumps(g1, indent=1) + "\n")
        shots = GOLDEN / name
        shutil.rmtree(shots, ignore_errors=True)
        shutil.copytree(Path(tmp) / "1" / "shots", shots)
    print("%s: recorded, %d checkpoints, %d instructions, %s" % (
        name, len(g1["checkpoints"]), g1["instructions"], g1["stop"]))
    return True


def check(name: str) -> bool:
    p = GOLDEN / ("%s.json" % name)
    if not p.exists():
        print("%s: no golden (make golden NAME=%s)" % (name, name))
        return False
    with tempfile.TemporaryDirectory() as tmp:
        g = play(name, Path(tmp))
    diff = differences(json.loads(p.read_text()), g)
    print("%s: %s" % (name, "OK" if not diff else "DIFFERS: " + "; ".join(diff)))
    return not diff


def first_difference(name: str, rebuilt: layout.Layout) -> list[str]:
    """Play the session on the originals and on `rebuilt`, keeping memory at every state digest (each
    checkpoint, then the end); the first one that differs, and where (linear addresses, the rebuild's
    memory read in the original's form)."""
    runs = {}
    prev = layout.current()
    try:
        for lay in (layout.original(), rebuilt):
            layout.use(lay)
            mem: list = []
            with tempfile.TemporaryDirectory() as tmp:
                play(name, Path(tmp), memory=mem)
            runs[lay.name] = mem
    finally:
        layout.use(prev)
    a, b = runs["original"], runs[rebuilt.name]
    for i, ((ia, ma), (ib, mb)) in enumerate(zip(a, b)):
        if ia != ib:
            return ["state %d: at instruction %d on the rebuild, %d on the original" % (i, ib, ia)]
        if ma == mb:
            continue
        spans: list[list[int]] = []
        for k in (k for k in range(len(ma)) if ma[k] != mb[k]):
            if spans and k - spans[-1][1] <= 4:
                spans[-1][1] = k
            else:
                spans.append([k, k])
        out = ["state %d (instruction %d) is the first that differs: %d bytes in %d places" % (
            i, ia, sum(ma[k] != mb[k] for k in range(len(ma))), len(spans))]
        for x, y in spans[:16]:
            out.append("  %05X-%05X original %s, rebuild %s" % (
                x + 0x500, y + 0x500, ma[x:y + 1][:12].hex(" "), mb[x:y + 1][:12].hex(" ")))
        return out
    return ["memory is the same at every state; the registers or the screen differ"]


def check_rebuilt(name: str, lay: layout.Layout) -> bool:
    """Verify a golden recorded on the originals against the rebuilt programs."""
    p = GOLDEN / ("%s.json" % name)
    if not p.exists():
        print("%s: no golden (make golden NAME=%s)" % (name, name))
        return False
    prev = layout.use(lay)
    try:
        with tempfile.TemporaryDirectory() as tmp:
            g = play(name, Path(tmp))
    finally:
        layout.use(prev)
    diff = differences(json.loads(p.read_text()), g)
    print("%s (rebuilt): %s" % (name, "OK" if not diff else "DIFFERS: " + "; ".join(diff)))
    if diff:
        for line in first_difference(name, lay):
            print("  " + line)
    return not diff


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--check", action="store_true")
    ap.add_argument("--rebuilt", action="store_true", help="with --check: on the programs `make rebuild` linked")
    ap.add_argument("--clock", action="store_true", help="the C-clock recordings (playbook 80)")
    ap.add_argument("names", nargs="*")
    a = ap.parse_args(argv)
    if a.clock:
        names = a.names or sorted(p.stem for p in SESSIONS.glob("*.keys"))
        return 0 if all([clock(n, a.check) for n in names]) else 1
    if a.rebuilt and not a.check:
        print("--rebuilt only checks: goldens are recorded on the originals")
        return 1
    names = a.names or sorted(p.stem for p in SESSIONS.glob("*.keys"))
    if a.rebuilt:
        lay = layout.rebuilt()
        for n, p in sorted(lay.programs.items()):
            print("%s: %s; %d stubs with entries in another order; put back: %s" % (
                n, p.exe.relative_to(ROOT), len(p.entry_map), ", ".join(w for *_x, w in p.file_values)))
        ok = all([check_rebuilt(n, lay) for n in names])
    else:
        ok = all([(check if a.check else record)(n) for n in names])
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())

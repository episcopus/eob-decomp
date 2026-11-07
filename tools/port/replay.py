#!/usr/bin/env python3
"""replay.py: a golden session played on the port, headless, from the port's own image (playbook 80, M5).

For each session named (default: boot-title) it writes the port's recording from the golden's clock
recording (tests/sessions/golden/<name>.clock.json: the interrupts by access count, the oracle's
instruction count at the accesses, each checkpoint's place), makes a game directory with the pinned
files of eob/ only (as the oracle's runs see them, run.hide_unpinned), runs
`build/port/eob-port-headless --replay` (port/replay.c), and compares each checkpoint the port dumped:

  - the port's view of the state (tools/port/state.py) with the oracle's at the same place
    (the recording's `port`);
  - the screen with the golden's own image hash, for a `shot` (and with the oracle's for the others).

The first checkpoint that differs is reported with the ranges of memory that differ. Exit 0 when every
checkpoint of every session is the same.

  replay.py [--port build/port/eob-port-headless] [NAME ...]
"""
from __future__ import annotations

import argparse
import hashlib
import json
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))
from port import state  # noqa: E402

GOLDEN = ROOT / "tests" / "sessions" / "golden"
SESSIONS = ROOT / "tests" / "sessions"
WORK = ROOT / "build" / "port" / "replay"


def options(name: str) -> dict:
    from oracle.input import Script
    return Script((SESSIONS / ("%s.keys" % name)).read_text()).options


def recording(name: str, clock: dict, golden: dict) -> str:
    from oracle import run
    opts = options(name)
    for k in ("save", "clock", "program"):
        if k in opts and not (k == "program" and opts[k] == "EOB.EXE"):
            raise SystemExit("%s: option %s is not in the port's replay yet" % (name, k))
    args = run.command_line(golden["args"], "mouse" in opts, opts.get("sound"))
    lines = ["# %s, from tests/sessions/golden/%s.clock.json (tools/port/replay.py)" % (name, name),
             "cmdline  " + args.strip(), "mouse %d" % ("mouse" in opts), "adlib %d" % (opts.get("sound") == "adlib")]
    files = clock.get("interrupt_files") or [0] * len(clock["interrupts"])
    for e, f in zip(clock["interrupts"], files):
        if len(e) == 2:
            lines.append("irq0 %d %d" % (e[1], f))
        elif e[2] == "irq1":
            lines.append("irq1 %d %d %d" % (e[1], e[3], f))
        elif e[2] == "mouse":
            lines.append("mouse_event %d %d %d %d %d %d" % (e[1], *e[3:7], f))
        else:
            raise SystemExit("%s: an interrupt of kind %r" % (name, e[2]))
    lines += ["time %d %d" % (a, i) for a, i in clock["times"]]
    lines += ["ltime %d %d" % (a, i) for a, i in clock["library_times"]]
    lines += ["checkpoint %s %d %d" % (n, ev, acc) for n, ev, acc, _d, _s in clock["port"]]
    return "\n".join(lines) + "\n"


def game_dir(where: Path) -> Path:
    from oracle import run
    if where.exists():
        shutil.rmtree(where)
    where.mkdir(parents=True)
    keep = run.pinned()
    for f in (ROOT / "eob").iterdir():
        if f.is_file() and f.name.upper() in keep:
            shutil.copyfile(f, where / f.name)
    return where


def ranges(a: bytes, b: bytes, base: int, limit: int = 12) -> list[str]:
    out, i, n = [], 0, len(a)
    while i < n and len(out) < limit:
        if a[i] != b[i]:
            j = i
            while j < n and (a[j] != b[j] or (j + 1 < n and a[j + 1] != b[j + 1])):
                j += 1
            lin = base + i
            ds = lin - state.DGROUP * 16
            where = " (ds:%04X)" % ds if 0 <= ds < 0x10000 else ""
            out.append("%05X..%05X%s (%d bytes) port %s, oracle %s" % (lin, base + j, where, j - i, a[i:min(j, i + 8)].hex(),
                                                                       b[i:min(j, i + 8)].hex()))
            i = j
        i += 1
    return out


def replay(name: str, port: str) -> bool:
    clock = json.loads((GOLDEN / ("%s.clock.json" % name)).read_text())
    golden = json.loads((GOLDEN / ("%s.json" % name)).read_text())
    if "port" not in clock:
        print("%s: the clock recording has no port checkpoints (make golden CLOCK=1 NAME=%s)" % (name, name))
        return False
    work = WORK / name
    rec = work / "recording.txt"
    gd = game_dir(work / "game")
    dumps = work / "dumps"
    dumps.mkdir(parents=True, exist_ok=True)
    for f in dumps.iterdir():
        f.unlink()
    rec.write_text(recording(name, clock, golden))
    r = subprocess.run([port, "--replay", str(rec), str(gd), str(dumps)], capture_output=True, text=True)
    out = (r.stdout + r.stderr).strip()
    images = {c["name"]: c.get("image") for c in golden["checkpoints"]}
    images["end"] = golden.get("final_image")
    ok = True
    taken = 0
    for cp, _ev, _acc, digest, screen in clock["port"]:
        p = dumps / ("%s.bin" % cp)
        if not p.exists():
            ok = False
            break
        taken += 1
        raw = p.read_bytes()
        mem, dac, mode = raw[:0xB0000], raw[0xB0000:0xB0000 + 768], raw[0xB0000 + 768]
        ours = state.digest(mem)
        scr = state.screen(mem[0xA0000:0xA0000 + 64000], dac, mem) if mode == 0x13 else None
        whole = state.screen(mem[0xA0000:0xA0000 + 64000], dac) if mode == 0x13 else None
        bad = []
        if ours != digest:
            bad.append("state")
        if screen is not None and scr != screen:
            bad.append("screen (mode %02Xh)" % mode)
        note = ""
        if not bad and images.get(cp):
            note = " (the golden's screenshot too)" if whole == images[cp] else " (the screenshot but its tick-driven animations, tools/port/state.py)"
        print("%s: %-16s %s%s" % (name, cp, "the same" if not bad else "DIFFERS: " + ", ".join(bad), note))
        if bad:
            ok = False
            if "state" in bad:
                ref = WORK / name / "oracle-view" / ("%s.bin" % cp)
                if ref.exists():
                    for line in ranges(state.view(mem), ref.read_bytes(), state.LOW):
                        print("    " + line)
                else:
                    print("    (tools/port/replay.py --views %s writes the oracle's views to compare)" % name)
            break
    if r.returncode != 0 or taken < len(clock["port"]):
        ok = False
        print("%s: the port's replay ended with %d after %d of %d checkpoints:\n    %s" % (
            name, r.returncode, taken, len(clock["port"]), out.replace("\n", "\n    ")))
    return ok


def views(name: str) -> None:
    """The oracle's view at each checkpoint's place, into build/port/replay/<name>/oracle-view/, to name
    the bytes that differ (the recording keeps only their digests)."""
    import tempfile
    from oracle import run
    from oracle.cclock import CClock
    from oracle.input import Script
    out = WORK / name / "oracle-view"
    out.mkdir(parents=True, exist_ok=True)
    clock = json.loads((GOLDEN / ("%s.clock.json" % name)).read_text())
    build = run.build_machine
    held = {}

    def build_and_clock(*a, **k):
        m = build(*a, **k)
        cc = held["cc"] = CClock(m)
        resolve = cc._resolve

        def keep():
            if cc._pending:
                v = state.view(bytes(m.uc.mem_read(0, state.TOP)))
                for d in cc._pending:
                    d["view"] = v
            return resolve()
        cc._resolve = keep
        digest = m.state_digest

        def counted(*aa, **kk):
            cc.mark()
            return digest(*aa, **kk)
        m.state_digest = counted
        return m
    run.build_machine = build_and_clock
    try:
        text = (SESSIONS / ("%s.keys" % name)).read_text()
        opts = Script(text).options
        with tempfile.TemporaryDirectory() as tmp:
            run.run_session(text, out=Path(tmp), max_insns=int(opts.get("insns", 60_000_000)),
                            mouse="mouse" in opts, sound=opts.get("sound"))
        held["cc"].recording()
    finally:
        run.build_machine = build
    for (n, *_r), d in zip(clock["port"], held["cc"].marks):
        (out / ("%s.bin" % n)).write_bytes(d["view"])
    print("%s: the oracle's views in %s" % (name, out))


def probe(name: str, port: str, ks: list[int], by_access: bool = False) -> None:
    """Where between two checkpoints the port leaves the oracle: the port's view and the oracle's just
    before interrupt K is delivered (K interrupts delivered, as many accesses made as the recording has
    before it), for each K; the first K whose views differ is after the divergence. The views go to
    build/port/replay/<name>/probe/."""
    import tempfile
    from oracle import run
    from oracle.cclock import CClock
    from oracle.input import Script
    clock = json.loads((GOLDEN / ("%s.clock.json" % name)).read_text())
    golden = json.loads((GOLDEN / ("%s.json" % name)).read_text())
    work = WORK / name
    out = work / "probe"
    if out.exists():
        shutil.rmtree(out)
    out.mkdir(parents=True)
    if by_access:
        # just before access A: the interrupts recorded before it delivered on both sides
        places = {}
        for acc in ks:
            places[sum(1 for e in clock["interrupts"] if e[1] <= acc - 1)] = acc - 1
        ks = sorted(places)
    else:
        places = {k: clock["interrupts"][k][1] for k in ks}
    text = recording(name, clock, golden).split("\n")
    text = [l for l in text if not l.startswith("checkpoint ")]
    text += ["checkpoint probe-%d %d %d" % (k, k, places[k]) for k in sorted(ks)]
    rec = work / "probe.txt"
    rec.write_text("\n".join(text) + "\n")
    gd = game_dir(work / "game")
    subprocess.run([port, "--replay", str(rec), str(gd), str(out)], capture_output=True, text=True)
    oracle = {}
    build = run.build_machine

    def build_and_clock(*a, **k):
        m = build(*a, **k)
        cc = CClock(m)
        deliver = m.deliver_interrupt          # the clock's: a hardware interrupt adds an event

        def at_delivery(n, clear_if=True):
            k = len(cc.events)
            v = state.view(bytes(m.uc.mem_read(0, state.TOP))) if k in places and k not in oracle else None
            r = deliver(n, clear_if)
            if v is not None and len(cc.events) == k + 1:
                oracle[k] = v
            return r
        if by_access:
            want = {a: k for k, a in places.items()}
            read, write = m.port_read, m.port_write

            def look():
                k = want.get(cc.accesses)
                if k is not None and k not in oracle and len(cc.events) == k:
                    oracle[k] = state.view(bytes(m.uc.mem_read(0, state.TOP)))

            def port_read(p_, size):
                look()
                return read(p_, size)

            def port_write(p_, size, v):
                look()
                return write(p_, size, v)
            m.port_read, m.port_write = port_read, port_write
            for n_, fn in list(m.services.items()):
                def service(mach, fn=fn):
                    look()
                    return fn(mach)
                m.services[n_] = service
        else:
            m.deliver_interrupt = at_delivery
        return m
    run.build_machine = build_and_clock
    try:
        txt = (SESSIONS / ("%s.keys" % name)).read_text()
        opts = Script(txt).options
        with tempfile.TemporaryDirectory() as tmp:
            run.run_session(txt, out=Path(tmp), max_insns=int(opts.get("insns", 60_000_000)),
                            mouse="mouse" in opts, sound=opts.get("sound"))
    finally:
        run.build_machine = build
    for k in sorted(ks):
        p = out / ("probe-%d.bin" % k)
        if not p.exists():
            print("%s: probe %d: the port did not reach it" % (name, k))
            continue
        mine = state.view(p.read_bytes()[:0xB0000])
        same = mine == oracle.get(k)
        print("%s: %d interrupts delivered, before access %d: %s" % (name, k, places[k] + 1, "the same" if same else "DIFFERS"))
        if k in oracle:
            (out / ("oracle-%d.bin" % k)).write_bytes(oracle[k])
        if not same and k in oracle:
            for line in ranges(mine, oracle[k], state.LOW, 12):
                print("    " + line)


def probe_files(name: str, port: str, fs: list[int]) -> None:
    """The port's view and the oracle's just before the library's file call F (the oracle is run first,
    for the interrupts and accesses at each place)."""
    import tempfile
    from oracle import run
    from oracle.cclock import CClock
    from oracle.input import Script
    clock = json.loads((GOLDEN / ("%s.clock.json" % name)).read_text())
    golden = json.loads((GOLDEN / ("%s.json" % name)).read_text())
    work = WORK / name
    out = work / "probe"
    if out.exists():
        shutil.rmtree(out)
    out.mkdir(parents=True)
    want = {f - 1 for f in fs}
    found = {}
    build = run.build_machine

    def build_and_clock(*a, **k):
        m = build(*a, **k)
        cc = CClock(m)
        f21 = m.services[0x21]

        def svc(mach):
            if cc.files in want and cc.files not in found and 0x3C <= (mach.get("ax") >> 8) <= 0x44:
                found[cc.files] = (len(cc.events), cc.accesses, state.view(bytes(mach.uc.mem_read(0, state.TOP))))
            return f21(mach)
        m.services[0x21] = svc
        return m
    run.build_machine = build_and_clock
    try:
        txt = (SESSIONS / ("%s.keys" % name)).read_text()
        opts = Script(txt).options
        with tempfile.TemporaryDirectory() as tmp:
            run.run_session(txt, out=Path(tmp), max_insns=int(opts.get("insns", 60_000_000)),
                            mouse="mouse" in opts, sound=opts.get("sound"))
    finally:
        run.build_machine = build
    text = [l for l in recording(name, clock, golden).split("\n") if not l.startswith("checkpoint ")]
    text += ["checkpoint file-%d %d %d %d" % (f + 1, found[f][0], found[f][1], f) for f in sorted(found)]
    rec = work / "probe.txt"
    rec.write_text("\n".join(text) + "\n")
    gd = game_dir(work / "game")
    r = subprocess.run([port, "--replay", str(rec), str(gd), str(out)], capture_output=True, text=True)
    for f in sorted(found):
        p = out / ("file-%d.bin" % (f + 1))
        if not p.exists():
            print("%s: before file call %d: the port did not reach it (%s)" % (name, f + 1, r.stdout.strip().split("\n")[-1]))
            continue
        mine = state.view(p.read_bytes()[:0xB0000])
        same = mine == found[f][2]
        print("%s: before file call %d (%d interrupts, %d accesses): %s" % (name, f + 1, found[f][0], found[f][1],
                                                                             "the same" if same else "DIFFERS"))
        if not same:
            for line in ranges(mine, found[f][2], state.LOW, 12):
                print("    " + line)


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--port", default=str(ROOT / "build/port/eob-port-headless"))
    ap.add_argument("--views", action="store_true", help="write the oracle's views first, to name what differs")
    ap.add_argument("--probe", default="", help="K,K,...: compare the views just before interrupt K (bisecting)")
    ap.add_argument("--probe-access", default="", help="A,A,...: compare the views just before access A")
    ap.add_argument("--probe-file", default="", help="F,F,...: compare the views just before the library's file call F")
    ap.add_argument("names", nargs="*", default=["boot-title"])
    a = ap.parse_args(argv)
    if a.probe_file:
        for n in a.names:
            probe_files(n, a.port, [int(f) for f in a.probe_file.split(",")])
        return 0
    if a.probe or a.probe_access:
        for n in a.names:
            probe(n, a.port, [int(k) for k in (a.probe or a.probe_access).split(",")], by_access=bool(a.probe_access))
        return 0
    ok = True
    for n in a.names:
        if a.views:
            views(n)
        ok = replay(n, a.port) and ok
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())

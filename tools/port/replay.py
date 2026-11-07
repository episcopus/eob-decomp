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
    for e in clock["interrupts"]:
        if len(e) == 2:
            lines.append("irq0 %d" % e[1])
        elif e[2] == "irq1":
            lines.append("irq1 %d %d" % (e[1], e[3]))
        elif e[2] == "mouse":
            lines.append("mouse_event %d %d %d %d %d" % (e[1], *e[3:7]))
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
            out.append("%05X..%05X (%d bytes) port %s oracle-view" % (lin, base + j, j - i, a[i:min(j, i + 8)].hex()))
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
        scr = state.screen(mem[0xA0000:0xA0000 + 64000], dac) if mode == 0x13 else None
        want_scr = images.get(cp) or screen
        bad = []
        if ours != digest:
            bad.append("state")
        if want_scr is not None and scr != want_scr:
            bad.append("screen (mode %02Xh)" % mode)
        print("%s: %-16s %s" % (name, cp, "the same" if not bad else "DIFFERS: " + ", ".join(bad)))
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


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--port", default=str(ROOT / "build/port/eob-port-headless"))
    ap.add_argument("--views", action="store_true", help="write the oracle's views first, to name what differs")
    ap.add_argument("names", nargs="*", default=["boot-title"])
    a = ap.parse_args(argv)
    ok = True
    for n in a.names:
        if a.views:
            views(n)
        ok = replay(n, a.port) and ok
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())

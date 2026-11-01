#!/usr/bin/env python3
"""golden.py: record and verify golden sessions (playbook 25, SPEC 6).

A session is a script in tests/sessions/<name>.keys (tools/oracle/input.py). Recording runs it twice;
the two runs must agree on every checkpoint, or nothing is written. The golden is
tests/sessions/golden/<name>.json (instructions, state digest and image hash of every checkpoint and of
the end of the run) with the screenshots beside it in tests/sessions/golden/<name>/ for people to look at.

  golden.py [--check] [NAME ...]      record (default) or verify; no NAME: every session
Exit code 0 when everything recorded or verified, 1 otherwise.
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
from oracle import run  # noqa: E402

SESSIONS = ROOT / "tests" / "sessions"
GOLDEN = SESSIONS / "golden"
KEEP = ("program", "stop", "instructions", "video_mode", "digest", "final_image", "script_done", "sound_digest", "adlib_writes")


def play(name: str, out: Path) -> dict:
    text = (SESSIONS / ("%s.keys" % name)).read_text()
    opts = Script(text).options
    save = SESSIONS / opts["save"] if "save" in opts else None
    s = run.run_session(text, out=out, max_insns=int(opts.get("insns", 60_000_000)),
                        mouse="mouse" in opts, sound=opts.get("sound"), save=save,
                        program=opts.get("program", "EOB.EXE"))
    g = {k: s[k] for k in KEEP}
    g["checkpoints"] = [{k: c[k] for k in ("kind", "name", "instructions", "digest", "image") if k in c}
                        for c in s["checkpoints"]]
    g["args"] = s["args"]
    return g


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


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--check", action="store_true")
    ap.add_argument("names", nargs="*")
    a = ap.parse_args(argv)
    names = a.names or sorted(p.stem for p in SESSIONS.glob("*.keys"))
    ok = all([(check if a.check else record)(n) for n in names])
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())

#!/usr/bin/env python3
"""gate.py: the regression gate of the matcher (`make match-gate`, part of `make check`; playbook 30).

Three things must hold, or the gate fails:

  1. every unit in src/ matches (cmatch exits 0: no function that is neither a stub nor a draft
     differs);
  2. every function that re/symbols.toml records at L2 is still proven: some file of src/ defines it
     (not as a stub or a draft) and cmatch printed `ok` for it in this run;
  3. every negative fixture in tests/match/negative/ is rejected: cmatch reports a difference (a
     DIFFER line). A fixture that does not build, or that matches, fails the gate, because then it
     tests nothing. A fixture is a unit of src/ with one deliberate defect; its file name starts
     with the unit's id (`e0cf7-constant.C`), and its first comment says what the defect is.

So a change to the matcher (or to a unit) cannot quietly accept wrong code or lose a proven function.

  gate.py [-v]
"""
from __future__ import annotations

import argparse
import re
import subprocess
import sys
import tomllib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(Path(__file__).resolve().parent))
import cmatch  # noqa: E402

NEGATIVE = ROOT / "tests" / "match" / "negative"


def run(path: Path, unit: str | None = None) -> tuple[int, str]:
    cmd = [sys.executable, str(ROOT / "tools/match/cmatch.py"), str(path)]
    if unit:
        cmd += ["--unit", unit]
    r = subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True)
    return r.returncode, r.stdout + r.stderr


def address(seg, off: int) -> str:
    """The registry's spelling: 07A9:000A, ovl2:0110."""
    return "%s:%04X" % (seg.lower() if isinstance(seg, str) else "%04X" % seg, off)


def rel(p: Path) -> str:
    return str(p.relative_to(ROOT)) if p.is_relative_to(ROOT) else str(p)


def sources(folder: Path) -> list[Path]:
    return sorted(p for p in folder.iterdir() if p.suffix.upper() in (".C", ".ASM"))


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("-v", "--verbose", action="store_true")
    a = ap.parse_args(argv)
    problems, proven = [], set()
    for f in sources(ROOT / "src"):
        code, out = run(f)
        if code:
            problems.append("%s does not match:\n    %s" % (rel(f), "\n    ".join(
                l for l in out.splitlines() if not l.startswith("ok"))))
        funcs, _, defined, stubs = cmatch.parse_annotations(f.read_text(), f.suffix.upper() == ".ASM")
        drafts = set(cmatch.DRAFTS)
        ok = set(re.findall(r"^ok\s+(\S+)", out, re.M))
        for n in defined - stubs - drafts:
            if n in ok:
                proven.add(address(*funcs[n]))
        if a.verbose:
            print("%-22s %s" % (rel(f), "ok" if not code else "FAILS"))
    registry = tomllib.loads((ROOT / "re/symbols.toml").read_text())["function"]
    l2 = {f["addr"] for f in registry if f.get("level") == "L2"}
    lost = sorted(l2 - proven)
    if lost:
        problems.append("recorded at L2 in re/symbols.toml but not proven by src/ in this run: %s" % ", ".join(lost))
    fixtures = sources(NEGATIVE) if NEGATIVE.is_dir() else []
    for f in fixtures:
        code, out = run(f, f.stem.split("-")[0])
        differ = [l for l in out.splitlines() if l.startswith(("DIFFER", "MISSING"))]
        if code == 0 or not any(l.startswith("DIFFER") for l in differ):
            problems.append("negative fixture %s was not rejected with a difference (exit %d):\n    %s" % (
                rel(f), code, "\n    ".join(out.splitlines()[-6:])))
        elif a.verbose:
            print("%-40s rejected: %s" % (rel(f), differ[0][:90]))
    for p in problems:
        print("match-gate: " + p)
    print("match-gate: %d units, %d functions at L2 proven, %d negative fixtures rejected: %s" % (
        len(sources(ROOT / "src")), len(l2 & proven), len(fixtures) - sum(p.startswith("negative") for p in problems),
        "OK" if not problems else "FAILED"))
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())

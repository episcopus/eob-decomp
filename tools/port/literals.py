#!/usr/bin/env python3
"""literals.py: where the original keeps each string literal of the port's units (port/literals.json).

tools/port/tc2c.py gives a string literal the DGROUP address the original's code uses for it, so a
pointer to a literal the game stores or compares is the original's pointer. The matcher knows that
address exactly: proving a function, it compares each literal reference of our compiled code with the
original's and records both (CMATCH_LITERALS, tools/match/cmatch.py; make rebuild reads the same
record). This tool runs the matcher on each unit of port/units.txt, lines the record up with the
unit's literals in source order (the record's keys are offsets in our module's data, where the literal
pool may follow other data: the start that agrees with the most keys, as tools/rebuild/rebuild.py
finds it), and writes one row per literal: its bytes, its DGROUP offset, and "match" (the matcher saw
the reference) or "search" (no function refers to it, so tc2c's search by content placed it).

It needs the Turbo C++ toolchain (make tc-check), so it is run when src/ changes a unit's literals, and
its output is committed; make port only reads it, and tc2c stops when a unit's literals no longer
agree with the table.

  literals.py [--out port/literals.json] [UNIT.C ...]     (default: the units of port/units.txt)
"""
from __future__ import annotations

import argparse
import json
import os
import subprocess
import sys
import tempfile
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(Path(__file__).resolve().parent))

import tc2c  # noqa: E402


def recorded(unit: Path, work: Path) -> dict[int, int]:
    out = work / ("%s.json" % unit.stem)
    r = subprocess.run([sys.executable, str(ROOT / "tools" / "match" / "cmatch.py"), str(unit)], cwd=ROOT,
                       env=dict(os.environ, CMATCH_LITERALS=str(out)), capture_output=True, text=True)
    if r.returncode or not out.exists():
        raise SystemExit("literals: make match fails on %s:\n%s%s" % (unit, r.stdout[-2000:], r.stderr[-2000:]))
    return {int(k): v for k, v in json.loads(out.read_text()).items()}


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--out", type=Path, default=tc2c.LITERALS)
    ap.add_argument("units", nargs="*", type=Path)
    a = ap.parse_args(argv)
    units = a.units or [ROOT / "src" / n for n in
                        (ROOT / "port" / "units.txt").read_text().split("\n") if n.strip() and not n.startswith("#")]
    prog = tc2c.Program(ROOT / "eob" / "EOB.EXE", None)
    table, seen, agree, differ = {}, 0, 0, []
    with tempfile.TemporaryDirectory() as tmp:
        for p in units:
            u = tc2c.Unit(p, prog)
            lits = [b for _id, b in tc2c.unit_literals(u.ast())]
            if not lits:
                continue
            rec = recorded(p, Path(tmp))
            cum, k = [], 0
            for b in lits:
                cum.append(k)
                k += len(b) + 1
            votes = Counter(key - x for key in rec for x in cum if key >= x)
            base = max(votes.items(), key=lambda kv: (kv[1], -kv[0]))[0] if votes else 0
            found = tc2c.place_literals(lits, prog.dgroup)
            whole = "/*@data */" in p.read_text(errors="replace")
            if whole:
                # a unit that defines its whole data (step D2): the record's keys are offsets in that
                # data and its values the original's addresses, templates among them; the literals are
                # the pool's strings in source order, so each is the next address after the one before
                # that holds its bytes, the record's when the code refers to it
                held, nxt, exact = sorted(set(rec.values())), 0, []
                for b in lits:
                    want = b + b"\0"
                    at = next((v for v in held if v >= nxt and prog.dgroup[v:v + len(want)] == want), None)
                    if at is None:
                        at = prog.dgroup.find(want, nxt)
                        at = None if at < 0 else at
                        found[len(exact)] = at if at is not None else found[len(exact)]
                        exact.append(None)
                    else:
                        exact.append(at)
                    nxt = at + len(want) if at is not None else nxt
            rows = []
            for i, (b, x, f) in enumerate(zip(lits, cum, found)):
                at = exact[i] if whole else rec.get(base + x)
                if at is not None and prog.dgroup[at:at + len(b) + 1] != b + b"\0":
                    raise SystemExit("literals: %s: the matcher puts %r at ds:%04X, which holds other bytes"
                                     % (p.name, b, at))
                if at is None:
                    rows.append([b.hex(), f, "search"])
                    continue
                seen += 1
                if at == f:
                    agree += 1
                else:
                    differ.append("%s %r: search ds:%04X, matcher ds:%04X" % (p.stem, b, f, at))
                rows.append([b.hex(), at, "match"])
            table[u.name] = rows
    a.out.write_text(json.dumps({
        "about": "Where the original keeps each string literal of the port's units: [bytes in hex, DGROUP "
                 "offset, how it was found]. Written by tools/port/literals.py (make port-literals) from "
                 "the matcher's record; read by tools/port/tc2c.py.",
        "units": table}, indent=1, sort_keys=True) + "\n")
    total = sum(len(r) for r in table.values())
    print("literals: %d literals in %d units, %d placed by the matcher's record (%d where the search by "
          "content agrees), %d by the search" % (total, len(table), seen, agree, total - seen))
    for d in differ:
        print("  " + d)
    return 0


if __name__ == "__main__":
    sys.exit(main())

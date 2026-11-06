#!/usr/bin/env python3
"""datamap.py: the anchors of the data map, step D0 of playbook 80 ("Step D").

The program's initialised data (EOB.EXE ds:01FE to 84B2) is the `_DATA` of its modules in link order,
each a module's initialised globals, its templates and then its string pool. This lists, in address
order, every anchor that says whose data a byte is:

  - pool: a unit's string literals as the matcher placed them (port/literals.json), the end of its data;
  - reloc: a run of the original's data relocations; TLINK writes the relocation table module by
    module, so a run follows the code relocations of the module whose data it is, or of the module
    before a data-only one (playbook 50, item 15);
  - empty: a code segment of size 0 among the resident code (build/re/eob.json), most likely a
    data-only module's;
  - names: the globals src/ declares in each stretch between anchors, by the units that declare them.

What lies between two anchors of different units is a boundary D0 still has to place.

  datamap.py [--lo 01FE] [--hi 84B2]
"""
from __future__ import annotations

import argparse
import collections
import json
import re
import sys
import tomllib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
EXE = ROOT / "eob" / "EOB.EXE"
DGROUP = 0x212E


def lin(addr: str) -> int:
    s, o = addr.split(":")
    return int(s, 16) * 16 + int(o, 16)


def resident_units() -> list[tuple[int, int, str]]:
    """(linear start, linear end, id) of EOB.EXE's units with code in the image"""
    us = tomllib.loads((ROOT / "re/units.toml").read_text())["unit"]
    out = []
    for u in us:
        if u["program"] != "EOB.EXE" or ":" not in u["start"] or u["start"].startswith("ovl"):
            continue
        a = lin(u["start"])
        out.append((a, a + u["bytes"], u["id"]))
    return sorted(out)


def relocations(exe: bytes) -> list[int]:
    """the original's relocation table, in its order, as linear addresses in the load module"""
    n = int.from_bytes(exe[6:8], "little")
    at = int.from_bytes(exe[0x18:0x1A], "little")
    out = []
    for i in range(n):
        o = int.from_bytes(exe[at + 4 * i:at + 4 * i + 2], "little")
        s = int.from_bytes(exe[at + 4 * i + 2:at + 4 * i + 4], "little")
        out.append(s * 16 + o)
    return out


def reloc_runs(relocs: list[int], units: list, lo: int, hi: int) -> list[tuple[int, int, str, int]]:
    """(first ds offset, last ds offset + 2, the unit whose code relocations the run follows, count)"""
    dg = DGROUP * 16

    def unit_of(x):
        return next((u for a, b, u in units if a <= x < b), None)
    runs, last = [], "C0"
    for x in relocs:
        if x >= dg:
            d = x - dg
            if lo <= d < hi:
                if runs and runs[-1][2] == last and runs[-1][4]:
                    runs[-1][0].append(d)
                else:
                    runs.append([[d], None, last, None, True])
            continue
        u = unit_of(x)
        if u:
            if runs:
                runs[-1][4] = False
            last = u
    return [(min(r[0]), max(r[0]) + 2, r[2], len(r[0])) for r in runs]


def pools() -> dict[str, tuple[int, int, int]]:
    """unit -> (first literal, end of the last, count)"""
    lits = json.loads((ROOT / "port/literals.json").read_text())["units"]
    out = {}
    for u, ls in lits.items():
        if not ls:
            continue
        a = min(l[1] for l in ls)
        b = max(l[1] + len(bytes.fromhex(l[0])) for l in ls)
        out[u.lower()] = (a, b, len(ls))
    return out


def declared() -> dict[int, set]:
    """ds offset -> the EOB.EXE units of src/ that declare a global there"""
    out = collections.defaultdict(set)
    for f in sorted((ROOT / "src").glob("E*.*")):
        if f.suffix.upper() not in (".C", ".ASM"):
            continue
        for m in re.finditer(r"/\*@ ds:([0-9A-Fa-f]{4}) \*/|;\s*@ ds:([0-9A-Fa-f]{4})", f.read_text(errors="replace")):
            out[int(m.group(1) or m.group(2), 16)].add(f.stem.lower())
    return out


def empty_segments() -> list[tuple[int, int]]:
    p = ROOT / "build/re/eob.json"
    if not p.exists():
        return []
    segs = json.loads(p.read_text())["segments"]
    return [(s["para"], s["end_offset"]) for s in segs if not s["size"] and s["kind_name"] == "code"]


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--lo", default="01FE")
    ap.add_argument("--hi", default="84B2")
    a = ap.parse_args(argv)
    lo, hi = int(a.lo, 16), int(a.hi, 16)
    exe = EXE.read_bytes()
    units = resident_units()
    anchors = []                                     # (ds offset, end, kind, unit, detail)
    for s, e, u, n in reloc_runs(relocations(exe), units, lo, hi):
        anchors.append((s, e, "reloc", u, "%d relocations, after %s's code" % (n, u)))
    for u, (s, e, n) in pools().items():
        if lo <= s < hi:
            anchors.append((s, e, "pool", u, "%d literals" % n))
    anchors.sort()
    names = declared()
    print("empty code segments (data-only modules?): %s" % ", ".join(
        "%04X:%04X" % (p, o) for p, o in empty_segments()) or "build/re/eob.json missing: make overlays")
    pos = lo
    for s, e, kind, u, detail in anchors:
        if s > pos:
            between = sorted(x for x in names if pos <= x < s)
            if between:
                by = collections.Counter(v for x in between for v in names[x])
                print("  ds:%04X-%04X  %d names, declared by %s" % (
                    pos, s, len(between), ", ".join("%s %d" % kv for kv in by.most_common(5))))
        print("ds:%04X-%04X %-5s %-6s %s" % (s, e, kind, u, detail))
        pos = max(pos, e)
    print("  ds:%04X-%04X  to the end" % (pos, hi))
    return 0


if __name__ == "__main__":
    sys.exit(main())

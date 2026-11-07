#!/usr/bin/env python3
"""datamap.py: the anchors of the data map, step D0 of playbook 80 ("Step D").

The program's initialised data (EOB.EXE ds:008F to 84B2) is the `_DATA` of its modules in link order,
each a module's initialised globals, its templates and then its string pool. This lists, in address
order, every anchor that says whose data a byte is:

  - pool: a unit's string literals as the matcher placed them (port/literals.json), the end of its data;
  - reloc: a run of the original's data relocations; TLINK writes the relocation table module by
    module, so a run follows the code relocations of the module whose data it is, or of the module
    before a data-only one (playbook 50, item 15);
  - empty: a code segment of size 0 among the resident code (build/re/eob.json), most likely a
    data-only module's;
  - names: the globals src/ declares in each stretch between anchors, by the units that declare them.

What lies between two anchors of different units is a boundary D0 has to place; --map places the ones
the rules settle (see build_map) and lists the rest.

  datamap.py [--lo 008F] [--hi 84B2]          the anchors in address order
  datamap.py --map                            each module's span, its boundaries and their evidence
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
    return [(min(r[0]), max(r[0]) + 2, r[2], len(r[0]), r[0]) for r in runs]


def records(xs: list[int]) -> list[list[int]]:
    """A run's relocations cut where the table's order rises: Turbo C writes each data record's fixups
    from the highest address down, so a rise starts a new record (about every 1 KB) or a new module;
    a record never spans two modules."""
    if len(xs) > 1 and all(b > a for a, b in zip(xs, xs[1:])):
        return [list(xs)]                            # TASM's order: one module, written upward
    out = []
    for x in xs:
        if out and x < out[-1][-1]:
            out[-1].append(x)
        else:
            out.append([x])
    return out


def pools() -> dict[str, tuple[int, int, int]]:
    """unit -> (first literal, end of the last with its NUL, count)"""
    lits = json.loads((ROOT / "port/literals.json").read_text())["units"]
    out = {}
    for u, ls in lits.items():
        if not ls:
            continue
        a = min(l[1] for l in ls)
        b = max(l[1] + len(bytes.fromhex(l[0])) + 1 for l in ls)
        out[u.lower()] = (a, b, len(ls))
    return out


def declared() -> dict[int, set]:
    """ds offset -> the EOB.EXE units of src/ that declare a global there"""
    out = collections.defaultdict(set)
    for f in sorted((ROOT / "src").glob("E*.*")):
        if f.suffix.upper() not in (".C", ".ASM"):
            continue
        # every annotation, with or without text after the address (`/*@ ds:009E: records ... */`), as
        # the matcher reads them
        for m in re.finditer(r"(?:/\*|;)\s*@\s*ds:([0-9A-Fa-f]{4})\b", f.read_text(errors="replace")):
            out[int(m.group(1), 16)].add(f.stem.lower())
    return out


def empty_segments() -> list[tuple[int, int]]:
    p = ROOT / "build/re/eob.json"
    if not p.exists():
        return []
    segs = json.loads(p.read_text())["segments"]
    return [(s["para"], s["end_offset"]) for s in segs if not s["size"] and s["kind_name"] == "code"]


OVERLAYS_AFTER = "e177d"   # the overlays' data follows E177D's: their pools (EOV1 ds:645B .. EOV6 ds:7F46)
                           # come after E177D's (ds:5E2C) and before E19BC's (ds:7FEC), and the one run
                           # of data relocations after E177D's code covers them (their own code
                           # relocations are in the FBOV data, not the table)


def link_order() -> list[str]:
    """the modules in the original's link order, lower case: the resident units by address, a
    data-only module (`dSSSS`, from the empty code segment at SSSS:OOOO) before the unit that starts
    where its empty segment is, and the seven overlays, in number order, after E177D"""
    us = tomllib.loads((ROOT / "re/units.toml").read_text())["unit"]
    res = sorted((lin(u["start"]), 1, u["id"]) for u in us if u["program"] == "EOB.EXE" and ":" in u["start"]
                 and not u["start"].startswith("ovl") and u["kind"] not in ("startup", "library"))
    last = max(a for a, _, _ in res)
    res += [(p * 16 + o, 0, "d%04x" % p) for p, o in empty_segments() if p * 16 + o <= last]
    out = [m for _, _, m in sorted(res)]
    ovl = sorted((u["id"] for u in us if u["program"] == "EOB.EXE" and u["start"].startswith("ovl")),
                 key=lambda i: int(i[3:]))
    k = out.index(OVERLAYS_AFTER) + 1
    return out[:k] + ovl + out[k:]


def code_relocators(relocs: list[int], units: list) -> set:
    """the units with code relocations in the table (the overlays' are in their FBOV data)"""
    dg = DGROUP * 16
    out = set()
    for x in relocs:
        if x < dg:
            u = next((u for a, b, u in units if a <= x < b), None)
            if u:
                out.add(u)
    return out


CLUSTER = 3


def build_map(lo: int, hi: int) -> dict:
    """Each module's span of initialised data, and the boundaries between modules.

    Hard anchors: a unit's pool (its literals), and the records of a run of data relocations that
    follows a unit with code relocations right before a module with code relocations (one owner).
    A run that follows a unit with code-less modules after it (the data-only modules, the overlays)
    is assigned record by record: a candidate keeps a record when its pool does not end before the
    record (a pool closes its module's data) and the modules placed so far leave room for it in link
    order; one candidate left places it. Soft anchors: a name only one unit declares, between hard
    spans whose order allows that unit, the largest set whose owners run in link order.

    Boundaries, between two modules placed one after the other in link order (`between` the
    modules the order puts between them, with no anchor):
      settled: the first ends with its pool and nothing is between: the second starts at the pool's
        end, rounded up to even (`_DATA` is word aligned), its unnamed leading bytes its own;
      settled [H]: the same with modules between, taken to have no initialised data;
      touching: the second's first anchor is at the first's last, rounded up to even;
      open: anything else, with the range the boundary is in.
    """
    exe = EXE.read_bytes()
    units = resident_units()
    relocs = relocations(exe)
    order = link_order()
    pos = {m: i for i, m in enumerate(order)}
    coded = code_relocators(relocs, units)
    spans: dict = {}
    pool_end = {}

    def add(m, a, b, why):
        s = spans.setdefault(m, [a, b, []])
        s[0], s[1] = min(s[0], a), max(s[1], b)
        s[2].append(why)
    kind = {f.stem.lower(): "c" if f.suffix.upper() == ".C" else "asm"   # by the source in src/
            for f in (ROOT / "src").glob("E*.*") if f.suffix.upper() in (".C", ".ASM")}
    image = exe[int.from_bytes(exe[8:10], "little") * 16 + DGROUP * 16:]
    names = declared()

    def string_at(x):
        """the end (after its NUL) of a printable NUL-terminated string at ds:x, else 0"""
        e = image.find(b"\0", x)
        t = image[x:e]
        return e + 1 if t and all(32 <= c < 127 or c in (6, 9, 10, 13) for c in t) else 0
    multi = []
    for a, b, after, n, xs in reloc_runs(relocs, units, lo, hi):
        cands = [after]
        for m in order[pos[after] + 1:] if after in pos else []:
            if m in coded:
                break
            cands.append(m)
        # the direction tells the assembler from the compiler: Turbo C writes a module's data
        # fixups downward (every C unit's run falls), TASM upward [H: by the one rising run, not by
        # assembling a test file]; a data-only module may be either
        if len(xs) > 1 and len(cands) > 1:
            up = all(y > x for x, y in zip(xs, xs[1:]))
            down = all(y < x for x, y in zip(xs, xs[1:])) or not up and sum(y < x for x, y in zip(xs, xs[1:])) > len(xs) // 2
            not_kind = "c" if up else "asm" if down else None
            narrowed = [c for c in cands if kind.get(c) != not_kind]
            if narrowed and narrowed != cands:
                why_dir = " (written %s: %s)" % ("upward, by TASM" if up else "downward, by Turbo C",
                                                "/".join(narrowed))
                cands = narrowed
            else:
                why_dir = ""
        else:
            why_dir = ""
        if len(cands) == 1:
            add(cands[0], a, b, "relocations ds:%04X-%04X (%d)%s" % (a, b, n, why_dir))
        else:
            multi += [(min(r), max(r) + 2, len(r), cands) for r in records(xs)]
    for u, (a, b, n) in pools().items():
        if lo <= a < hi and u in pos:
            add(u, a, b, "pool ds:%04X-%04X (%d literals)" % (a, b, n))
            pool_end[u] = b

    def fits(c, a, b, pending=(), name=False):
        if c in pool_end and b > pool_end[c] and not (name and a >= pool_end[c] - 1 and string_at(a)):
            return False                             # past the pool only a string the unit declares
        if c in pool_end and name and a >= pool_end[c] and any(
                pool_end[c] <= y < a for y in names if c not in names[y]):
            return False
        for ra, rb, _, cands in pending:             # a record not placed yet still bounds the order:
            if all(pos[k] > pos[c] for k in cands) and b > ra:       # whichever candidate owns it
                return False
            if all(pos[k] < pos[c] for k in cands) and a < rb:
                return False
        for m, (s0, s1, _) in spans.items():
            if m == c:
                continue
            if pos[m] < pos[c] and s1 > a or pos[m] > pos[c] and s0 < b:
                return False
        return True

    def place_records():
        left = []
        for a, b, n, cands in multi:
            ok = [c for c in cands if fits(c, a, b)]
            if len(ok) == 1:
                add(ok[0], a, b, "relocations ds:%04X-%04X (%d, a record of the run after %s's code)" % (
                    a, b, n, cands[0]))
            else:
                left.append((a, b, n, ok or cands))
        return left
    multi = place_records()
    conflicts = []
    hard = sorted((v[0], v[1], m) for m, v in spans.items())
    soft = []
    for x in sorted(names):
        if not (lo <= x < hi) or len(names[x]) != 1:
            continue
        (u,) = names[x]
        if u not in pos or any(a0 <= x < b0 for a0, b0, _ in hard):
            continue
        if fits(u, x, x + 1, multi, name=True):
            soft.append((x, u))
        else:
            conflicts.append((x, u))
    # a name alone is weak (a unit declares the other modules' data it reads, the driver's slots at
    # ds:802A are declared one by one by their callers): keep it when it is one of a cluster of at
    # least CLUSTER names of its unit with no other unit's name between, or when nothing but its own
    # names lies between it and its unit's hard span
    groups, cur = [], []
    for x, u in soft:
        if cur and cur[-1][1] == u:
            cur.append((x, u))
        else:
            cur = [(x, u)]
            groups.append(cur)
    strong = []
    for g in groups:
        u = g[0][1]
        near = u in spans and any(not any(min(spans[u][1], g[0][0]) <= y < max(spans[u][0], g[-1][0])
                                          for y, v in soft if v != u) for _ in (0,))
        if len(g) >= CLUSTER or near:
            strong += g
        else:
            conflicts += g
    soft = strong
    best, back = [1] * len(soft), [-1] * len(soft)   # the longest chain with owners in link order
    for i in range(len(soft)):
        for j in range(i):
            if pos[soft[j][1]] <= pos[soft[i][1]] and best[j] + 1 > best[i]:
                best[i], back[i] = best[j] + 1, j
    keep, i = set(), max(range(len(soft)), key=lambda k: best[k], default=-1)
    while i >= 0:
        keep.add(i)
        i = back[i]
    for k, (x, u) in enumerate(soft):
        if k in keep:
            if u in pool_end and x >= pool_end[u] - 1:      # a string the unit declares extern: its pool
                e = string_at(x)
                add(u, x, e, "string ds:%04X-%04X declared only by %s (its pool goes on)" % (x, e, u))
                pool_end[u] = max(pool_end[u], e)
            else:
                add(u, x, x + 1, "name ds:%04X declared only by %s" % (x, u))
        else:
            conflicts.append((x, u))
    multi = place_records()
    seq = sorted(spans, key=lambda m: pos[m])
    bounds = []
    for m1, m2 in zip(seq, seq[1:]):
        e1, s2 = spans[m1][1], spans[m2][0]
        between = order[pos[m1] + 1:pos[m2]]
        cut = (e1 + 1) & ~1
        inside = [r for r in multi if e1 <= r[0] < s2]
        if s2 < e1:
            kind = "overlap"
        elif inside:
            kind = "open"
        elif pool_end.get(m1) == e1 and cut <= s2:
            kind = "settled [H]" if between else "settled"
        elif cut >= s2:
            kind = "touching"
        else:
            kind = "open"
        bounds.append({"after": m1, "before": m2, "end": e1, "start": s2, "cut": cut if kind != "open" else None,
                       "kind": kind, "between": between,
                       "records": ["ds:%04X-%04X (%d relocations): %s" % (r[0], r[1], r[2], "/".join(r[3]))
                                   for r in inside]})
    return {"order": order, "spans": spans, "bounds": bounds, "conflicts": conflicts, "pending": multi}


def resolve(lo: int, hi: int) -> list[dict]:
    """The map: each module with initialised data, its [start, end) in link order covering [lo, hi)
    with no gap, how each end was placed, and what says so. A boundary no anchor settles has no
    relocation inside it (the image cannot tell where it is: any even cut gives the same bytes and
    the same relocation order), so it is `free`, placed by the default rule: the stretch goes to the
    module before, unless the module after declares names in it, which then start at the first of
    them (rounded down to even)."""
    m = build_map(lo, hi)
    order, spans = m["order"], m["spans"]
    pos = {x: i for i, x in enumerate(order)}
    names = declared()
    seq = sorted(spans, key=lambda k: pos[k])
    cuts = {}                                        # module -> (start, how, why)
    rel = {x - DGROUP * 16 for x in relocations(EXE.read_bytes())}
    for b in m["bounds"]:
        m1, m2 = b["after"], b["before"]
        if b["kind"] in ("settled", "settled [H]", "touching"):
            why = {"settled": "%s's pool ends at ds:%04X; _DATA is word aligned",
                   "settled [H]": "%s's pool ends at ds:%04X; the modules between are taken to have no data",
                   "touching": "%s's last anchor ends at ds:%04X"}[b["kind"]] % (m1, b["end"])
            cuts[m2] = (b["cut"], b["kind"], why)
        else:
            # the next module's first anchor, or the far pointer whose relocated segment word it is (d1b82's
            # run starts at ds:8106, the segment of the pointer at ds:8104)
            limit = b["start"] - 2 if b["start"] in rel else b["start"]
            own = [x for x in sorted(names) if b["end"] <= x < limit and names[x] == {m2}]
            at = (own[0] & ~1) if own else limit & ~1
            at = max(at, (b["end"] + 1) & ~1)
            cuts[m2] = (at, "free", "no relocation between ds:%04X and ds:%04X; %s" % (
                b["end"], limit, "the first name only %s declares" % m2 if own else "the stretch goes to %s" % m1))
    # a module the map took to have no data, between a pool's end and the next module's names, that alone
    # declares a name there (E14EF's gHoverSelectsOption, ds:5D1A, after E1414's pool and before E1547's
    # names): the stretch from the pool's end to the next module's first name is its own
    for b in [b for b in m["bounds"] if b["kind"] == "settled [H]"]:
        m1, m2 = b["after"], b["before"]
        between = order[pos[m1] + 1:pos[m2]]
        # the next module's first name, or its first anchor: a run of relocations starts at the far
        # pointer whose segment word is the first relocated one, two bytes before it
        first_name = min([x for x in names if x >= b["cut"] and m2 in names[x]], default=None)
        stop = min([x for x in [first_name] if x is not None] + [spans[m2][0]] +
                   [r - 2 for r in rel if r >= b["cut"]])
        lone = [(x, next(iter(names[x]))) for x in sorted(names)
                if b["cut"] <= x < stop and len(names[x]) == 1 and next(iter(names[x])) in between]
        if not lone or any(stop & ~1 > x >= lone[0][0] for x in rel):
            continue
        mod = lone[0][1]
        if any(u != mod for _, u in lone):
            continue
        rest = between[:between.index(mod)]
        cuts[mod] = (b["cut"], "settled [H]" if rest else "settled", ("%s's pool ends at ds:%04X; " % (m1, b["end"])) + (
            "the modules between are taken to have no data" if rest else "_DATA is word aligned"))
        cuts[m2] = (stop & ~1, "free", "no relocation between ds:%04X and ds:%04X; %s" % (
            lone[-1][0], stop, "the first name only %s declares" % m2 if stop == first_name
            else "%s's first far pointer" % m2 if any(r - 2 == stop for r in rel) else "%s's first anchor" % m2))
        spans[mod] = [b["cut"], stop & ~1, ["name ds:%04X declared only by %s" % (x, mod) for x, _ in lone]]
        seq.insert(seq.index(m2), mod)
    last = seq[-1]                                   # a pool closes its module: what follows the last
    if spans[last][1] < hi:                          # pool goes to a module after it in link order
        later = order[pos[last] + 1:]
        own = [(x, next(iter(names[x]))) for x in sorted(names) if spans[last][1] <= x < hi and len(names[x]) == 1
               and next(iter(names[x])) in later]
        nxt = own[0][1] if own else later[0]
        cut = (spans[last][1] + 1) & ~1
        spans[nxt] = [cut, hi, ["names ds:%s declared only by %s" % (",".join("%04X" % x for x, u in own if u == nxt), nxt)]
                      if own else []]
        cuts[nxt] = (cut, "settled [H]", "%s's pool ends at ds:%04X; the rest, to the library's data, taken as %s's" % (
            last, spans[last][1], nxt))
        seq.append(nxt)
    out = []
    first = seq[0]
    cuts.setdefault(first, ((lo + 1) & ~1, "settled", "the start-up code's _DATA ends at ds:%04X" % lo))
    for i, mod in enumerate(seq):
        start, how, why = cuts[mod]
        end = cuts[seq[i + 1]][0] if i + 1 < len(seq) else hi
        out.append({"id": mod, "start": start, "end": end, "start_by": how, "start_why": why,
                    "evidence": spans[mod][2]})
    return out


def write_toml(rows: list[dict], path: Path) -> None:
    lines = ["# The data map of EOB.EXE (playbook 80, step D0): each module's initialised data in DGROUP, in",
             "# link order, from the start-up code's _DATA to the library's. Written by `make datamap WRITE=1`",
             "# (tools/rebuild/datamap.py). start_by: settled (the previous module's pool ends there), settled [H]",
             "# (the same, the modules between taken to have no data), touching (the anchors meet), free (no",
             "# relocation inside the stretch, so the image cannot tell; placed by the default rule).",
             "# A module of the form dSSSS is a data-only module, from the empty code segment at SSSS. A module's",
             "# end is the next one's start: the pad byte after an odd end of a pool is counted with it.", ""]
    names = declared()
    for r in rows:
        r["notes"] = ["ds:%04X is declared by %s, not by this module" % (x, ", ".join(sorted(names[x])))
                      for x in sorted(names) if r["start"] <= x < r["end"] and r["id"] not in names[x]
                      and not r["id"].startswith("d")]
    for r in rows:
        lines += ["[[module]]", 'id = "%s"' % r["id"], 'start = "ds:%04X"' % r["start"], 'end = "ds:%04X"' % r["end"],
                  "bytes = %d" % (r["end"] - r["start"]), 'start_by = "%s"' % r["start_by"],
                  'start_why = "%s"' % r["start_why"],
                  "evidence = [%s]" % ", ".join('"%s"' % e for e in r["evidence"])]
        if r["notes"]:
            lines.append("notes = [%s]" % ", ".join('"%s"' % e for e in r["notes"]))
        lines.append("")
    path.write_text("\n".join(lines))


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--lo", default="008F")
    ap.add_argument("--hi", default="84B2")
    ap.add_argument("--map", action="store_true", help="each module's span and the boundaries")
    ap.add_argument("--write", action="store_true", help="write the map to re/datamap.toml")
    a = ap.parse_args(argv)
    lo, hi = int(a.lo, 16), int(a.hi, 16)
    if a.write:
        rows = resolve(lo, hi)
        write_toml(rows, ROOT / "re/datamap.toml")
        tally = collections.Counter(r["start_by"] for r in rows)
        print("datamap: %d modules, ds:%04X-%04X; boundaries %s; re/datamap.toml written" % (
            len(rows), lo, hi, ", ".join("%d %s" % (n, k) for k, n in sorted(tally.items()))))
        return 0
    if a.map:
        m = build_map(lo, hi)
        pos = {x: i for i, x in enumerate(m["order"])}
        for mod, (s0, s1, why) in sorted(m["spans"].items(), key=lambda kv: pos[kv[0]]):
            print("%-6s ds:%04X-%04X  %s" % (mod, s0, s1, "; ".join(why[:2]) +
                                             (" (+%d)" % (len(why) - 2) if len(why) > 2 else "")))
        print()
        tally = collections.Counter()
        for b in m["bounds"]:
            tally[b["kind"]] += 1
            if b["kind"] != "settled":
                print("%-11s %s | %s  ds:%04X..%04X%s" % (b["kind"], b["after"], b["before"], b["end"], b["start"],
                                                         ("  between: " + " ".join(b["between"])) if b["between"] else ""))
                for r in b["records"]:
                    print("            a record of relocations at %s" % r)
        print("boundaries: %s" % ", ".join("%d %s" % (n, k) for k, n in sorted(tally.items())))
        print("names kept out (another unit's data): %d" % len(m["conflicts"]))
        return 0
    exe = EXE.read_bytes()
    units = resident_units()
    anchors = []                                     # (ds offset, end, kind, unit, detail)
    for s, e, u, n, _ in reloc_runs(relocations(exe), units, lo, hi):
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

#!/usr/bin/env python3
"""names.py: what in src/ is still named by its address, and where the files disagree (Phase 6,
playbook 65).

  make names                  a line per unit: placeholder functions, globals, struct fields, unknowns
  make names UNIT=<id>        that unit's placeholders, each with the files that share it
  make names SHARED=1         what to name first: placeholder globals and functions declared by the
                              most files, and the structs whose files give one offset two names

A placeholder is what rename.PLACEHOLDER says it is (f19EC_0016, Ovl2_0110, g811B, gText7BA6). A
global or a function is reported once per address, under the unit that defines it when one does
(its owner), otherwise under each file that declares it. A struct field is a placeholder when its
name is its offset (m0D, b0, w12) or a placeholder (unk, field); padding (pad04) is not counted. Field
offsets are computed from the declarations: Turbo C packs structs on bytes without -a, and -ml makes
an unqualified data pointer far (4 bytes). A field this tool cannot size stops the offsets of its
struct there.
"""
from __future__ import annotations

import argparse
import re
import sys
import tomllib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools/match"))
import rename  # noqa: E402

SRC = ROOT / "src"
FIELD_PLACEHOLDER = re.compile(r"^(?:[mbwd]_?[0-9A-Fa-f]{1,3}|unk\w*|field\d*|f[0-9A-F]{2,})$")
PAD = re.compile(r"^(?:pad|_pad|gap|res|reserved)\w*$")
STRUCT = re.compile(r"^(struct|union)\s+(\w+)\s*\{([^{}]*)\}\s*;", re.M)
SIZES = {"char": 1, "signed char": 1, "unsigned char": 1, "int": 2, "unsigned": 2, "unsigned int": 2,
         "short": 2, "unsigned short": 2, "signed int": 2, "long": 4, "unsigned long": 4, "signed long": 4}
FIELD = re.compile(r"^\s*((?:unsigned|signed|(?:struct|union)\s+\w+|\w+)(?:\s+(?:char|int|short|long))?)\s+"
                   r"((?:far\s*\*|near\s*\*|\*)\s*)?(\w+)\s*((?:\[[^\]]+\])*)\s*;")
FUNC_FIELD = re.compile(r"^\s*[\w\s*]+\(\s*(far|near)?\s*\*\s*(\w+)\s*((?:\[[^\]]+\])*)\)\s*\([^;]*\)\s*;")


def comments_out(text: str) -> str:
    return re.sub(r"/\*.*?\*/|//[^\n]*", lambda m: re.sub(r"[^\n]", " ", m.group(0)), text, flags=re.S)


def struct_fields(text: str) -> list[tuple[str, list[tuple[int | None, str]]]]:
    """(struct name, [(offset or None, field name)]) of every struct the file defines."""
    out, sizes = [], {}
    for m in STRUCT.finditer(comments_out(text)):
        at, fields, union = 0, [], m.group(1) == "union"
        widest = 0
        for line in re.split(r";(?![^()]*\))", m.group(3)):
            f, fp = FIELD.match(line + ";"), FUNC_FIELD.match(line + ";")
            if not f and not fp:
                if line.strip():
                    at = None
                continue
            if fp:                                      # a function pointer: far unless `near` (-ml)
                name, dims, size = fp.group(2), fp.group(3), 2 if fp.group(1) == "near" else 4
            else:
                base, ptr, name, dims = f.groups()
                base = re.sub(r"\s+", " ", base)
                if ptr:
                    size = 2 if ptr.startswith("near") else 4
                else:
                    size = SIZES.get(base) or sizes.get(base)
            count = 1
            for d in re.findall(r"\[([^\]]+)\]", dims):
                try:
                    count *= int(eval(d, {"__builtins__": {}}))   # 0x16 - 0x04: constants only
                except Exception:
                    count = None
                    break
            if union:
                fields.append((0, name))
                widest = max(widest, size * count) if size and count and widest is not None else None
                continue
            fields.append((at, name))
            at = at + size * count if at is not None and size and count else None
        sizes["%s %s" % (m.group(1), m.group(2))] = widest if union else at
        out.append((m.group(2), fields))
    return out


def is_placeholder(name: str, addr: str) -> bool:
    """rename.PLACEHOLDER, or a name that ends with its own address (gDCAA, g159, gIconX5721): the
    pattern needs a digit and four hex digits, these escape it."""
    if rename.PLACEHOLDER.match(name):
        return True
    off = addr.split(":")[-1].lstrip("0")
    return len(off) >= 2 and name.lstrip("_").upper().endswith(off.upper()) and \
        re.match(r"^_?g?[A-Za-z]*[a-z]?%s$" % re.escape(off), name, re.I) is not None


def units() -> dict:
    return {u["id"]: u for u in tomllib.loads((ROOT / "re/units.toml").read_text())["unit"]}


def inventory() -> dict:
    files = rename.sources()
    texts = {p: p.read_text() for p in files}
    by_addr: dict[str, dict[Path, str]] = {}
    for p in files:
        for n, addr in rename.bindings(p).items():
            by_addr.setdefault(addr, {})[p] = n[1:] if p.suffix.upper() == ".ASM" and n.startswith("_") else n
    owner = {}
    for p in files:
        asm = p.suffix.upper() == ".ASM"
        for line in texts[p].splitlines():
            m = re.search(r"/\*@\s*(ds:[0-9A-Fa-f]{1,4}|ovl\d+:[0-9A-Fa-f]{1,4}|[0-9A-Fa-f]{4}:[0-9A-Fa-f]{4})\b", line) \
                or (asm and re.search(r";\s*@\s*(ds:[0-9A-Fa-f]{1,4}|[0-9A-Fa-f]{4}:[0-9A-Fa-f]{4})\b", line))
            if not m or re.search(r"\bextern\b|\bextrn\b", line, re.I) or "stub" in line:
                continue
            addr = rename.PREFIX.get(rename.program_of(p), "") + rename.norm(m.group(1))
            owner.setdefault(addr, p)
    places: dict[Path, dict[str, list]] = {p: {"functions": [], "globals": [], "fields": []} for p in files}
    shared = []
    for addr, names in by_addr.items():
        if any(not is_placeholder(n, addr) for n in names.values()):
            continue
        far_data = all(re.match(r"_?g[A-Z0-9]", n) for n in names.values())   # gSheet2067 at 2067:0000
        kind = "globals" if addr.split("/")[-1].startswith("ds:") or far_data else "functions"
        where = [owner[addr]] if addr in owner else sorted(names)
        for p in where:
            places[p][kind].append((addr, names[p] if p in names else next(iter(names.values())), len(names)))
        shared.append((len(names), kind, addr, sorted(set(names.values())), sorted(q.stem for q in names)))
    structs: dict[tuple[str, int], dict[str, set]] = {}
    for p in files:
        if p.suffix.upper() != ".C":
            continue
        for sname, fields in struct_fields(texts[p]):
            whole = len(fields) == 1          # one array over the whole record: a byte view, not a layout
            for at, fname in fields:
                if PAD.match(fname):
                    continue
                if FIELD_PLACEHOLDER.match(fname):
                    places[p]["fields"].append((sname, at, fname))
                if at is not None and not whole and not re.search(r"^union\s+%s\b" % sname, texts[p], re.M):
                    structs.setdefault((sname, at), {}).setdefault(fname, set()).add(p.stem)
    clashes = {k: v for k, v in structs.items() if len(v) > 1}
    unknown = {p: texts[p].count("Purpose unknown") for p in files}
    return {"files": files, "places": places, "shared": shared, "clashes": clashes, "unknown": unknown}


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--unit")
    ap.add_argument("--shared", action="store_true")
    a = ap.parse_args(argv)
    inv = inventory()
    us = units()
    if a.shared:
        print("Placeholder globals and functions by the number of files that declare them (name these first):")
        for n, kind, addr, names, stems in sorted(inv["shared"], key=lambda s: (-s[0], s[2]))[:60]:
            if n < 3:
                break
            print("  %2d files  %-9s %-14s %-24s %s" % (n, kind[:-1], addr, "/".join(names), " ".join(stems)))
        print("\nStruct offsets the files name differently (one record, one vocabulary):")
        for (sname, at), names in sorted(inv["clashes"].items()):
            print("  struct %-12s +%02X  %s" % (sname, at, "; ".join(
                "%s (%s)" % (f, " ".join(sorted(ps))) for f, ps in sorted(names.items(), key=lambda x: -len(x[1])))))
        return 0
    if a.unit:
        p = next((q for q in inv["files"] if q.stem.lower() == a.unit.lower()), None)
        if not p:
            print("names: no file for unit %s in src/" % a.unit)
            return 1
        pl = inv["places"][p]
        print("%s (%s): %d placeholder functions, %d globals, %d struct fields, %d 'Purpose unknown'" % (
            p.name, us.get(p.stem.lower(), {}).get("program", "?"), len(pl["functions"]), len(pl["globals"]),
            len(pl["fields"]), inv["unknown"][p]))
        for kind in ("functions", "globals"):
            for addr, name, n in sorted(pl[kind]):
                print("  %-9s %-14s %-24s declared by %d file%s" % (kind[:-1], addr, name, n, "s" if n > 1 else ""))
        for sname, at, fname in pl["fields"]:
            print("  field     struct %s +%s %s" % (sname, "%02X" % at if at is not None else "?", fname))
        return 0
    tot = [0, 0, 0, 0]
    print("%-12s %-10s %5s %7s %6s %7s" % ("file", "program", "funcs", "globals", "fields", "unknown"))
    for p in inv["files"]:
        pl = inv["places"][p]
        row = [len(pl["functions"]), len(pl["globals"]), len(pl["fields"]), inv["unknown"][p]]
        tot = [x + y for x, y in zip(tot, row)]
        if any(row):
            print("%-12s %-10s %5d %7d %6d %7d" % (p.name, us.get(p.stem.lower(), {}).get("program", "?"), *row))
    print("%-12s %-10s %5d %7d %6d %7d" % ("total", "", *tot))
    print("names: %d function and %d global addresses have only placeholder names (a global no unit "
          "defines counts in every file that declares it)" % tuple(
              sum(1 for s in inv["shared"] if s[1] == k) for k in ("functions", "globals")))
    print("names: %d struct offsets named two ways across files (make names SHARED=1)" % len(inv["clashes"]))
    return 0


if __name__ == "__main__":
    sys.exit(main())

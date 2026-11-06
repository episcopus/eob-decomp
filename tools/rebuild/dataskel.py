#!/usr/bin/env python3
"""dataskel.py: draft a unit's data as C definitions (`make data-skeleton UNIT=`; playbook 80, step D).

The unit's span comes from re/datamap.toml (make datamap). Its bytes are the original's, read from
the load module, and cut into items at:

  - every name some file of src/ declares in the span (`extern signed char gX[]; /*@ ds:341E */`),
    typed by that declaration (the unit's own file first);
  - every literal and initialiser template the code of the unit owns (port/literals.json, the
    matcher's record): those are the code's, written by the functions, so the draft leaves them out
    and says where they fall (Turbo C writes globals and templates in source order, the string pool
    last: a definition goes in the file before the function whose template follows it);
  - every far pointer target inside the span that has no name, and every run of far pointers
    (a relocated segment word at +2): an unnamed piece is `gXXXX`, to be named from its readers.

A far pointer is written by the name at its target: a function by its registry name (an overlay
function through its stub entry), data by the declared name at or below it. What the draft cannot
type (a struct, an item whose size is not a multiple of its element) is written as bytes with a
comment, for the unit's D2 to write as its type. The draft is printed and kept in
build/skeleton/<UNIT>-data.C; `make match` with `/*@data */` in the unit's file proves it.

  dataskel.py UNIT
"""
from __future__ import annotations

import argparse
import json
import re
import struct
import sys
import tomllib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools" / "re"))
sys.path.insert(0, str(ROOT / "tools" / "match"))
from mzutil import read_mz  # noqa: E402
import cmatch  # noqa: E402

DGROUP = 0x212E
DECL = re.compile(r"/\*@\s*ds:([0-9A-Fa-f]{4})\b|;\s*@\s*ds:([0-9A-Fa-f]{4})\b")
SIZES = {"char": 1, "signed char": 1, "unsigned char": 1, "int": 2, "signed int": 2, "unsigned": 2,
         "unsigned int": 2, "short": 2, "unsigned short": 2, "signed": 2, "long": 4, "unsigned long": 4}


class Decl:
    def __init__(self, name: str, off: int, unit: str, text: str):
        self.name, self.off, self.unit, self.text = name, off, unit, text
        self.base, self.ptr, self.array, self.fptr = parse_type(name, text)

    def elem(self) -> int | None:
        if self.ptr or self.fptr:
            return 4
        return SIZES.get(self.base)


def parse_type(name: str, text: str) -> tuple[str, bool, bool, bool]:
    """(base type, element is a pointer, is an array, is a function pointer) of a C declaration."""
    t = re.sub(r"\bextern\b|\bstatic\b|\bconst\b|\bvolatile\b|;.*$|=.*$", " ", text).strip()
    fptr = bool(re.search(r"\(\s*(?:far\s*|near\s*)?\*\s*%s\b" % re.escape(name), t))
    array = bool(re.search(r"\b%s\s*\)?\s*\[" % re.escape(name), t))
    head = t[:t.find(name)] if name in t else t
    ptr = "*" in head.replace("(", "")
    base = re.sub(r"\bfar\b|\bnear\b|\bhuge\b|\*|\(", " ", head)
    return " ".join(base.split()), ptr and not fptr, array, fptr


def declarations(unit: str) -> dict[int, Decl]:
    """ds offset -> the declaration to type it by: the unit's own file's, else the first file's."""
    out: dict[int, Decl] = {}
    # C first (typed), the unit's own file before the others; then assembly (`EXTRN _gX:WORD`, typed by
    # its size), the unit's own last (its `_gX label byte` says nothing of the type)
    def order(f):
        asm, own = f.suffix.upper() == ".ASM", f.stem.lower() == unit
        return (asm, own if asm else not own, f.name)
    files = sorted((ROOT / "src").glob("*.*"), key=order)
    for f in files:
        if f.suffix.upper() not in (".C", ".ASM") or not f.stem.upper().startswith(("E", "D")):
            continue
        asm = f.suffix.upper() == ".ASM"
        for line in f.read_text(errors="replace").splitlines():
            m = cmatch.ANNOT.search(line)
            if not m or not m.group(1).lower().startswith("ds:"):
                continue
            try:
                _, data, _, _ = cmatch.parse_annotations(line, asm)
            except SystemExit:
                continue
            for name, off in data.items():
                if asm and name.startswith("_"):
                    name = name[1:]                  # C's name of an assembly label (_gTunesOn)
                if off not in out:
                    out[off] = Decl(name, off, f.stem.lower(), asm_decl(name, line) if asm else c_decl(line))
    return out


def c_decl(line: str) -> str:
    """The declaration of an annotated C line, without its comments: `extern int gX;`."""
    text = re.sub(r"/\*.*?\*/", " ", line)
    text = re.sub(r"/\*.*$", " ", text)
    return " ".join(text.split()).split(";")[0].split("=")[0].strip() + ";"     # a definition: its head


def asm_decl(name: str, line: str) -> str:
    """A C declaration for an assembly one, by its size: BYTE, WORD, DWORD (a far pointer)."""
    m = re.search(r":\s*(BYTE|WORD|DWORD)\b|\b(db|dw|dd)\b", line, re.I)
    kind = (m.group(1) or {"db": "BYTE", "dw": "WORD", "dd": "DWORD"}[m.group(2).lower()]).upper() if m else "BYTE"
    c = {"BYTE": "unsigned char", "WORD": "unsigned", "DWORD": "void far *"}[kind]
    return "extern %s%s%s;" % (c, "" if c.endswith("*") else " ", name.lstrip("_"))


def function_decls() -> dict[str, str]:
    """name -> an extern declaration of it with its annotation, from the files of src/."""
    out = {}
    for f in sorted((ROOT / "src").glob("*.C")):
        for line in f.read_text(errors="replace").splitlines():
            m = cmatch.ANNOT.search(line)
            if m and not m.group(1).lower().startswith("ds:") and line.lstrip().startswith("extern") and "(" in line:
                n = re.search(r"\(?\s*(?:far\s*)?\*?\s*([A-Za-z_]\w*)\s*\)?\s*\(", line.split("/*")[0])
                if n and n.group(1) not in out:
                    out[n.group(1)] = "%s /*@ %s */" % (c_decl(line), m.group(1))
    return out


def function_names() -> tuple[dict, dict]:
    """(linear address -> name, (overlay, code offset) -> name) of the registry's named functions."""
    lin, ovl = {}, {}
    for f in tomllib.loads((ROOT / "re/symbols.toml").read_text())["function"]:
        if not f.get("name"):
            continue
        s, o = f["addr"].split(":")
        if s.lower().startswith("ovl"):
            ovl[(int(s[3:]), int(o, 16))] = f["name"]
        else:
            lin[int(s, 16) * 16 + int(o, 16)] = f["name"]
    return lin, ovl


def stub_entries() -> dict:
    """(stub paragraph, entry offset) -> (overlay, code offset), from make overlays."""
    p = ROOT / "build/re/eob.json"
    man = json.loads(p.read_text()) if p.exists() else {"overlays": []}
    return {(o["stub_para"], 0x20 + 5 * e["index"]): (o["overlay"], e["code_offset"])
            for o in man["overlays"] for e in o["entries"]}


def draft(unit: str) -> str:
    mods = {m["id"]: m for m in tomllib.loads((ROOT / "re/datamap.toml").read_text())["module"]}
    if unit not in mods:
        raise SystemExit("dataskel: %s has no span in re/datamap.toml" % unit)
    mod = mods[unit]
    lo, hi = int(mod["start"][3:], 16), int(mod["end"][3:], 16)
    mz = read_mz((ROOT / "eob/EOB.EXE").read_bytes())
    img, base = mz["image"], DGROUP * 16
    relocs = {s * 16 + o - base for o, s in mz["relocs"] if lo <= s * 16 + o - base < hi}
    byte = lambda o: img[base + o]
    word = lambda o: struct.unpack_from("<H", img, base + o)[0]
    decls = declarations(unit)
    lits = json.loads((ROOT / "port/literals.json").read_text())["units"]
    code_owned = sorted((l[1], l[1] + len(bytes.fromhex(l[0])) + 1) for k, v in lits.items() if k.lower() == unit
                        for l in v if lo <= l[1] < hi)
    funcs, ovl_funcs = function_names()
    stubs = stub_entries()
    # cuts: names, code-owned pieces, far pointer runs, unnamed targets inside the span
    targets = {}                                     # unnamed target -> the pointee type of a table at it
    for r in relocs:
        if word(r) == DGROUP and lo <= word(r - 2) < hi:
            d = next((decls[o] for o in decls if o <= r - 2 < o + 4 * 64 and decls[o].ptr), None)
            targets[word(r - 2)] = d.base if d and SIZES.get(d.base) else None
    # a scalar (not an array) ends after its element: what follows to the next name is unnamed
    ends = {o + decls[o].elem() for o in decls if lo <= o < hi and not decls[o].array and decls[o].elem()}
    cuts = {lo, hi} | {o for o in decls if lo <= o < hi} | {e for e in ends if e < hi} | {a for a, _ in code_owned} | \
           {b for _, b in code_owned if b < hi}
    named = {o for o in decls if lo <= o < hi}
    for t in targets:
        if not any(a <= t < b for a, b in code_owned):
            cuts.add(t)
    for r in sorted(relocs):                         # a far pointer run starts and ends a piece
        if r - 2 >= lo and not any(o for o in named if r - 2 < o < r + 2):
            prev = r - 4 in relocs
            nxt = r + 4 in relocs
            if not prev and r - 2 not in named:
                cuts.add(r - 2)
            if not nxt and r + 2 < hi:
                cuts.add(r + 2)
    cuts = sorted(c for c in cuts if lo <= c <= hi)
    out = ["/*@data */", "/* the data of %s, %s to %s (%d bytes, %d relocations): drafted by make data-skeleton "
           "from the original's bytes; each item at its annotation, in address order. */" % (
               unit, mod["start"], mod["end"], hi - lo, len(relocs))]
    needs: dict[str, str] = {}
    fdecls = function_decls()
    own_file = next((f for f in (ROOT / "src").glob(unit.upper() + ".C")), None)    # (a C draft)
    mine = set(cmatch.parse_annotations(own_file.read_text(), own_file.suffix.upper() == ".ASM")[0]) if own_file else set()

    def name_at(t: int) -> str:
        """A far pointer's data target, by the name at or below it."""
        cand = [o for o in list(decls) + [c for c in cuts if lo <= c < hi] if o <= t]
        if not cand:
            return "(void far *)MK_FP(_DS, 0x%04X)" % t
        o = max(cand)
        n = decls[o].name if o in decls else "g%04X" % o
        if o in decls and decls[o].unit != unit and not decls[o].unit.startswith(unit):
            needs[n] = "%s /*@ ds:%04X */" % (decls[o].text.strip() or n, o)
        if o in decls and not decls[o].array:
            n = "&" + n                              # a scalar: its address
        return n if t == o else "(char far *)%s + %d" % (n, t - o)

    def pointer(o: int) -> str:
        if o + 2 not in relocs:
            return "0" if word(o) == 0 and word(o + 2) == 0 else "(void far *)0x%04X%04XL /* no relocation */" % (
                word(o + 2), word(o))
        s, off = word(o + 2), word(o)
        if s == DGROUP:
            return name_at(off)
        if (s, off) in stubs:
            n = ovl_funcs.get(stubs[(s, off)])
            if n and n not in mine:
                needs[n] = fdecls.get(n, "extern void far %s(void); /*@ ovl%d:%04X */" % ((n,) + stubs[(s, off)]))
            return n or "0 /* ovl%d:%04X, no named function */" % stubs[(s, off)]
        n = funcs.get(s * 16 + off)
        if n and n not in mine:
            needs[n] = fdecls.get(n, "extern void far %s(void); /*@ %04X:%04X */" % (n, s, off))
        return n or "0 /* %04X:%04X, no named function */" % (s, off)

    for a, b in zip(cuts, cuts[1:]):
        if a >= hi:
            break
        own = next(((p, q) for p, q in code_owned if p == a), None)
        if own:
            text = img[base + a:base + b - 1]
            out.append("/* ds:%04X-%04X: the code's (a literal or template, %r): written by its function */" % (
                a, b, text[:40].decode("latin-1")))
            continue
        d = decls.get(a)
        n = b - a
        if d and d.elem() and n % d.elem() == 0 and (d.ptr or d.fptr or not any(a <= r < b for r in relocs)):
            e = d.elem()
            k = n // e
            if d.ptr or d.fptr:
                vals = [pointer(a + 4 * i) for i in range(k)]
            elif e == 1:
                raw = img[base + a:base + b]
                if d.base == "char" and raw.endswith(b"\0") and b"\0" not in raw[:-1] and all(32 <= c < 127 for c in raw[:-1]) and len(raw) > 1:
                    vals = None
                    s = raw[:-1].decode("latin-1").replace("\\", "\\\\").replace('"', '\\"')
                    out.append('%s %s[%d] = "%s"; /*@ ds:%04X */' % (d.base, d.name, n, s, a) if d.array else
                               '%s %s = %d; /*@ ds:%04X */' % (d.base, d.name, raw[0], a))
                    continue
                vals = [("%d" % struct.unpack("b", bytes([c]))[0]) if d.base in ("signed char", "char") else "0x%02X" % c for c in raw]  # char is signed (no -K)
            elif e == 2:
                vals = [("%d" % struct.unpack_from("<h", img, base + a + 2 * i)[0]) if "unsigned" not in d.base
                        else "0x%04X" % word(a + 2 * i) for i in range(k)]
            else:
                vals = [("%dL" % struct.unpack_from("<l", img, base + a + 4 * i)[0]) if "unsigned" not in d.base
                        else "0x%08XUL" % struct.unpack_from("<L", img, base + a + 4 * i)[0] for i in range(k)]
            decl = d.text.strip()
            out.append(render(d, k, vals, a))
            continue
        name = d.name if d else "g%04X" % a
        if not d and b == hi and n == 1 and byte(a) == 0 and a % 2 and (a in {q for _, q in code_owned}):
            out.append("/* ds:%04X: the pad byte to the next module (word alignment) */" % a)
            continue
        t = targets.get(a)
        if not d and t and n % SIZES[t] == 0 and not any(a <= r < b for r in relocs):
            out.append(render(Decl(name, a, unit, "%s %s[];" % (t, name)), n // SIZES[t],
                              scalars(img, base, a, n, t), a))
            continue
        if not d and n % 4 == 0 and all(a + 4 * i + 2 in relocs for i in range(n // 4)):
            out.append(render(None, n // 4, [pointer(a + 4 * i) for i in range(n // 4)], a, "void far *%s" % name))
            continue
        why = ""
        if d:
            why = " /* declared `%s` (%s): write it as its type */" % (" ".join(d.text.split()), d.unit)
        rel = [r for r in relocs if a <= r < b]
        if rel:
            why += " /* relocated words at %s: far pointers to write by name */" % ", ".join("ds:%04X" % r for r in rel)
        raw = img[base + a:base + b]
        vals = ["0x%02X" % c for c in raw]
        out.append(braces("unsigned char %s[%d]" % (name, n), vals, a, why))
    if needs:
        out[2:2] = ["/* what the pointers name that the file does not declare: */"] + sorted(needs.values())
    return "\n".join(out) + "\n"


def scalars(img: bytes, base: int, a: int, n: int, t: str) -> list[str]:
    e = SIZES[t]
    fmt = {1: "b", 2: "h", 4: "l"}[e]
    if "unsigned" in t:
        fmt = fmt.upper()
    vs = struct.unpack_from("<%d%s" % (n // e, fmt), img, base + a)
    suffix = ("UL" if "unsigned" in t else "L") if e == 4 else ""
    return [("0x%0*X" % (2 * e, v) if "unsigned" in t else "%d" % v) + suffix for v in vs]


def braces(head: str, vals: list[str], at: int, tail: str = "", per: int = 12) -> str:
    """`head = { values };` with the annotation on the first line, where the matcher reads it."""
    if len(vals) <= per:
        return "%s = { %s }; /*@ ds:%04X */%s" % (head, ", ".join(vals), at, tail)
    rows = ",\n    ".join(", ".join(vals[i:i + per]) for i in range(0, len(vals), per))
    return "%s = { /*@ ds:%04X */%s\n    %s\n};" % (head, at, tail, rows)


def render(d: Decl | None, k: int, vals: list, at: int, head: str | None = None) -> str:
    if head is None:
        text = re.sub(r"\bextern\s+", "", " ".join(d.text.split())).rstrip(";").strip()
        text = re.sub(r"\s*;.*$", "", text)
        if d.array:
            text = re.sub(r"(\b%s\s*\)?\s*)\[\s*\w*\s*\]" % re.escape(d.name), r"\g<1>[%d]" % k, text, count=1)
        head = text
    if d is not None and not d.array:
        return "%s = %s; /*@ ds:%04X */" % (head, vals[0], at)
    if head.endswith("]") is False and "[" not in head:
        head += "[%d]" % k
    return braces(head, vals, at, per=4 if any(len(v) > 8 for v in vals) else 12)


def assembled(unit: str) -> bool:
    """The module was assembled: its data relocations rise in the table (TASM writes a module's data
    fixups upward, Turbo C downward; checked by assembling and compiling test files, 2026-11-06)."""
    mod = {m["id"]: m for m in tomllib.loads((ROOT / "re/datamap.toml").read_text())["module"]}[unit]
    lo, hi = int(mod["start"][3:], 16), int(mod["end"][3:], 16)
    mz = read_mz((ROOT / "eob/EOB.EXE").read_bytes())
    xs = [s * 16 + o - DGROUP * 16 for o, s in mz["relocs"]]
    xs = [x for x in xs if lo <= x < hi]
    return len(xs) > 1 and all(q > p for p, q in zip(xs, xs[1:]))


def draft_asm(unit: str) -> str:
    """The data of a module that was assembled, as TASM source: each declared name a PUBLIC label (C's
    `_` added), words and far pointers by its declaration, the rest bytes."""
    mod = {m["id"]: m for m in tomllib.loads((ROOT / "re/datamap.toml").read_text())["module"]}[unit]
    lo, hi = int(mod["start"][3:], 16), int(mod["end"][3:], 16)
    mz = read_mz((ROOT / "eob/EOB.EXE").read_bytes())
    img, base = mz["image"], DGROUP * 16
    relocs = {s * 16 + o - base for o, s in mz["relocs"] if lo <= s * 16 + o - base < hi}
    word = lambda o: struct.unpack_from("<H", img, base + o)[0]
    decls = declarations(unit)
    funcs, ovl_funcs = function_names()
    stubs = stub_entries()
    names = {o: d for o, d in decls.items() if lo <= o < hi}
    ext, body = {}, []
    for t in sorted({word(r - 2) for r in relocs if word(r) == DGROUP and lo <= word(r - 2) < hi} - set(names)):
        names[t] = Decl("g%04X" % t, t, unit, "unsigned char g%04X[];" % t)
    # an unnamed start, and an unnamed run of far pointers, get a label by their address (no PUBLIC)
    labels = {lo} | {r - 2 for r in relocs if r - 4 not in relocs}
    labels = {x for x in labels if x not in names and not any(o < x < o + (names[o].elem() or 1) for o in names)}
    k = lo
    while k < hi:
        if k in names:
            d = names[k]
            body += ["", "        PUBLIC  _%s ; @ ds:%04X" % (d.name, k), "_%s label byte" % d.name]
        elif k in labels:
            body += ["", "g%04X label byte ; ds:%04X, to name from its readers" % (k, k)]
        if k + 2 in relocs:
            s_, off = word(k + 2), word(k)
            if s_ == DGROUP:
                o = max(x for x in list(decls) + list(names) if x <= off)
                d = names.get(o) or decls[o]
                if not lo <= o < hi:
                    ext["_" + d.name] = ("BYTE", "ds:%04X" % o)
                body.append("        dd      _%s%s" % (d.name, "" if o == off else "+%d" % (off - o)))
            else:
                if (s_, off) in stubs:
                    ov, co = stubs[(s_, off)]
                    n, at = ovl_funcs.get((ov, co)), "ovl%d:%04X" % (ov, co)
                else:
                    n, at = funcs.get(s_ * 16 + off), "%04X:%04X" % (s_, off)
                if not n:
                    body.append("        dd      0 ; %s, no named function: name it" % at)
                else:
                    sym = n if n.startswith("_") else "_" + n
                    ext[sym] = ("FAR", at)
                    body.append("        dd      %s" % sym)
            k += 4
            continue
        nxt = min([x for x in list(names) + list(labels) if x > k] + [r - 2 for r in relocs if r - 2 > k] + [hi])
        d = names.get(max([x for x in names if x <= k], default=-1))
        e = d.elem() if d else None
        if d and (d.ptr or d.fptr) and (nxt - k) % 4 == 0:
            ps = [struct.unpack_from("<L", img, base + k + 4 * i)[0] for i in range(min((nxt - k) // 4, 4))]
            body.append("        dd      " + ", ".join("%08Xh" % v if v else "0" for v in ps))
            k += 4 * len(ps)
        elif e == 2 and (nxt - k) % 2 == 0:
            ws = [word(k + 2 * i) for i in range(min((nxt - k) // 2, 8))]
            body.append("        dw      " + ", ".join("%04Xh" % w if w > 9 else "%d" % w for w in ws))
            k += 2 * len(ws)
        else:
            bs = img[base + k:base + min(nxt, k + 12)]
            body.append("        db      " + ", ".join("%03Xh" % b for b in bs))
            k += len(bs)
    out = [".MODEL LARGE", "; @data", "; the data of %s, %s to %s (%d bytes, %d relocations): drafted by make data-skeleton"
           % (unit, mod["start"], mod["end"], hi - lo, len(relocs)),
           "; from the original's bytes. Assembled: its relocations rise in the table, as TASM writes them."]
    out += ["        EXTRN   %s:%s ; @ %s" % (n, kind, at) for n, (kind, at) in sorted(ext.items())]
    out += ["", ".DATA"] + body + ["", ".CODE", "; no code: the module's empty code segment (the original's segment table lists it)", "END"]
    return "\n".join(out) + "\n"


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("unit")
    ap.add_argument("--c", action="store_true", help="draft C even when the relocations say it was assembled")
    a = ap.parse_args(argv)
    unit = a.unit.lower()
    asm = assembled(unit) and not a.c
    text = draft_asm(unit) if asm else draft(unit)
    out = ROOT / "build/skeleton" / ("%s-data.%s" % (unit.upper(), "ASM" if asm else "C"))
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(text)
    print(text, end="")
    print("dataskel: %s (paste the definitions into the unit's file in source order, then make match)" % out.relative_to(ROOT))
    return 0


if __name__ == "__main__":
    sys.exit(main())

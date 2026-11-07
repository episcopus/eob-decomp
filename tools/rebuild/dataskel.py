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
import clayout  # noqa: E402

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


class Fallback(Exception):
    """The bytes cannot be written as the type (a relocation where a number is, a union's other
    member, a near pointer that is not null): the draft writes them as bytes."""


PRINTABLE = set(range(32, 127)) | {9, 10, 13}


def c_string(raw: bytes) -> str | None:
    """A C string literal for raw (without its NUL), when it reads as text."""
    if not raw or any(c not in PRINTABLE for c in raw):
        return None
    out = []
    for c in raw:
        ch = chr(c)
        out.append({"\\": "\\\\", '"': '\\"', "\r": "\\r", "\n": "\\n", "\t": "\\t"}.get(ch, ch))
    return '"%s"' % "".join(out)


def own_prototypes(f: Path | None) -> dict[str, str]:
    """name -> a prototype of each function the unit's file defines (`int F(int a);`), from the
    definition's head, so a table before the definition can name it."""
    out: dict[str, str] = {}
    if not f:
        return out
    lines = f.read_text(errors="replace").split("\n")
    for i, line in enumerate(lines):
        m = re.match(r"^/\*@\s*(?:ovl\d+|[0-9A-Fa-f]{4}):[0-9A-Fa-f]{4}(?:\s+\w+)?\s*\*/\s*(.+)$", line)
        if not m or "(" not in m.group(1):
            continue
        head, j = m.group(1), i
        while head.count("(") > head.count(")") and j + 1 < len(lines):
            j += 1
            head += " " + lines[j].strip()
        if head.rstrip().endswith(";"):
            continue
        n = re.search(r"(\w+)\s*\(", head)
        if n:
            out[n.group(1)] = re.sub(r"/\*.*?\*/", "", " ".join(head.split())).strip() + ";"
    return out


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
    owned_at = {a: b for a, b in code_owned}
    funcs, ovl_funcs = function_names()
    stubs = stub_entries()
    own_file = next((f for f in (ROOT / "src").glob(unit.upper() + ".C")), None)    # (a C draft)
    mine = set(cmatch.parse_annotations(own_file.read_text(), False)[0]) if own_file else set()
    protos = own_prototypes(own_file)
    others = [f for f in sorted((ROOT / "src").glob("*.C")) if f != own_file]
    T = clayout.Types(([own_file] if own_file else []) + [ROOT / "src/EOB.H"] + others)

    def ctype_of(d: Decl):
        try:
            return T.variable(d.text)[1]
        except (ValueError, KeyError, SyntaxError, ZeroDivisionError):
            return None

    # the items: the declared names, then the unnamed targets of far pointers, typed by what points
    # at them (a table's element, a struct's pointer field), to a fixpoint (a node's `next` names the
    # next node)
    items: dict[int, tuple[str, object, Decl | None]] = {}     # start -> (name, CType or None, decl)
    for o, d in decls.items():
        if lo <= o < hi:
            items[o] = (d.name, ctype_of(d), d)

    def strong_after(k: int) -> int:
        return min([x for x in items if x > k] + [a for a, _ in code_owned if a > k] +
                   [b for _, b in code_owned if b > k and b <= hi] + [hi])

    def extent(o: int) -> int:
        name, t, d = items[o]
        nxt = strong_after(o)
        if t is not None and t.size and not (t.kind == "array" and t.count == 0):
            return min(o + t.size, nxt) if o + t.size <= nxt else nxt
        return nxt

    def leaf_at(t, off: int):
        """The scalar member of type t at offset off (an open array repeats its element)."""
        while t is not None and t.kind in ("array", "struct", "union"):
            if t.kind == "array":
                e = t.elem
                if not e.size:
                    return None
                off %= e.size
                t = e
            else:
                f = [(n, fo, ft) for n, fo, ft in t.fields if fo <= off < fo + ft.size]
                if not f:
                    return None
                _, fo, ft = f[0]
                off -= fo
                t = ft
        return t if off == 0 else None

    def pointee(leaf):
        if leaf is None or leaf.kind != "ptr":
            return None
        try:
            return T.base(leaf.base)
        except KeyError:
            return None

    strings: dict[int, tuple[int, str]] = {}      # pool strings a table points at: start -> (end, literal)
    for _ in range(8):
        added = False
        for r in sorted(relocs):
            if word(r) != DGROUP or not lo <= word(r - 2) < hi:
                continue
            t = word(r - 2)
            if t in items or t in strings:
                continue
            o = max([x for x in items if x <= r - 2], default=None)
            want = None
            if o is not None and r - 2 < extent(o):
                want = pointee(leaf_at(items[o][1], r - 2 - o))
            if any(a <= t < b for a, b in code_owned) or any(x < t < extent(x) for x in items):
                continue
            # a string the code never names, past the table: a literal of the table's initialiser
            end = img.find(b"\0", base + t) - base
            lit = c_string(img[base + t:base + end])
            if lit and t > r and (want is None or want.kind == "int" and want.size == 1) and end + 1 <= strong_after(t):
                strings[t] = (end + 1, lit)
                added = True
                continue
            items[t] = ("g%04X" % t, want if want is not None and want.kind != "int" else
                        (want and clayout.CType("array", 0, elem=want, count=0)), None)
            added = True
        if not added:
            break

    out = ["/*@data */", "/* the data of %s, %s to %s (%d bytes, %d relocations): drafted by make data-skeleton "
           "from the original's bytes; each item at its annotation, in address order. */" % (
               unit, mod["start"], mod["end"], hi - lo, len(relocs))]
    needs: dict[str, str] = {}
    fdecls = function_decls()
    emitted: dict[int, tuple[str, object]] = {}   # start -> (name, CType) as written

    def spelling(t) -> str:
        while t is not None and t.kind == "array":
            t = t.elem
        return t.spelling() if t is not None else "unsigned char"

    def data_ref(t: int, want) -> str:
        if t in strings:
            return strings[t][1]
        cand = [x for x in items if x <= t] + [x for x in decls if x <= t and not lo <= x < hi]
        if not cand:
            return "(void far *)MK_FP(_DS, 0x%04X)" % t
        o = max(cand)
        if o in items:
            name, ct, d = items[o]
        else:
            d = decls[o]
            name, ct = d.name, ctype_of(d)
            needs[name] = "%s /*@ ds:%04X */" % (d.text.strip() or name, o)
        if o in emitted:
            ct = emitted[o][1]
        is_array = ct is not None and ct.kind == "array"
        if o not in emitted and ct is None and d is not None:
            is_array = d.array
        e = ct.elem.size if is_array and ct is not None and ct.elem and ct.elem.size else None
        if t == o:
            ref = name if is_array else "&" + name
        elif e and (t - o) % e == 0:
            ref = "&%s[%d]" % (name, (t - o) // e)
        else:
            ref = "(char far *)%s%s + %d" % ("" if is_array else "&", name, t - o)
            return ref if want is None or (want.kind == "int" and want.size == 1) else "(void far *)(%s)" % ref
        got = spelling(ct.elem if is_array and ct is not None else ct) if ct is not None else None
        if want is not None and got is not None and want.spelling() and want.spelling() != got and want.spelling() != "void":
            return "(void far *)%s" % ref
        return ref

    def func_ref(s: int, off: int) -> str:
        if (s, off) in stubs:
            n = ovl_funcs.get(stubs[(s, off)])
            at = "ovl%d:%04X" % stubs[(s, off)]
        else:
            n = funcs.get(s * 16 + off)
            at = "%04X:%04X" % (s, off)
        if not n:
            return "0 /* %s, no named function */" % at
        if n in mine:
            if n in protos:
                needs[n] = protos[n]
        else:
            needs[n] = fdecls.get(n, "extern void far %s(void); /*@ %s */" % (n, at))
        return n

    def pointer(o: int, leaf) -> str:
        if o in relocs or o + 1 in relocs or o + 3 in relocs:
            raise Fallback()
        if leaf is not None and leaf.size == 2:
            if word(o) or o + 2 in relocs:
                raise Fallback()                 # a near pointer: a number, written as bytes
            return "0"
        if o + 2 not in relocs:
            if word(o) == 0 and word(o + 2) == 0:
                return "0"
            return "(void far *)0x%04X%04XL /* no relocation */" % (word(o + 2), word(o))
        s, off = word(o + 2), word(o)
        if s == DGROUP:
            return data_ref(off, pointee(leaf) if leaf is not None and leaf.kind == "ptr" else None)
        return func_ref(s, off)

    def number(o: int, t) -> str:
        if any(o <= r < o + t.size for r in relocs) or any(o <= r - 2 < o + t.size for r in relocs if r - 2 >= o):
            raise Fallback()
        v = int.from_bytes(img[base + o:base + o + t.size], "little", signed=t.signed)
        if t.signed or v < 10:
            return "%d" % v + ("L" if t.size == 4 else "")
        return "0x%0*X" % (2 * t.size, v) + ("UL" if t.size == 4 else "")

    def value(t, o: int, n: int | None = None) -> str:
        """The initialiser of n bytes at o as t (n for an open array)."""
        if t.kind == "int":
            return number(o, t)
        if t.kind in ("ptr", "fptr"):
            return pointer(o, t)
        if t.kind == "array":
            e = t.elem
            k = t.count or (n // e.size)
            if e.kind == "int" and e.size == 1:
                raw = img[base + o:base + o + k]
                z = raw.find(b"\0")
                if z > 0 and not raw[z:].strip(b"\0") and not any(o <= r < o + k for r in relocs):
                    s = c_string(raw[:z])
                    if s:
                        return s
            return "{ " + ", ".join(value(e, o + i * e.size) for i in range(k)) + " }"
        if t.kind == "struct":
            return "{ " + ", ".join(value(ft, o + fo) for _, fo, ft in t.fields) + " }"
        if t.kind == "union":
            _, fo, ft = t.fields[0]
            rest = img[base + o + ft.size:base + o + t.size]
            if rest.strip(b"\0") or any(o + ft.size <= r + dr < o + t.size for r in relocs for dr in (-2, 0, 1)):
                raise Fallback()
            return "{ " + value(ft, o) + " }"
        raise Fallback()

    def head_of(name: str, t, d: Decl | None, k: int | None) -> str:
        if d is not None:
            text = re.sub(r"\bextern\s+", "", " ".join(d.text.split())).rstrip(";").strip()
            text = re.sub(r"\s*;.*$", "", text)
            if k is not None:
                text = re.sub(r"(\b%s\s*\)?\s*)\[\s*\]" % re.escape(name), r"\g<1>[%d]" % k, text, count=1)
            return text
        if t.kind == "array":
            return "%s %s[%d]" % (spelling(t), name, k)
        if t.kind == "ptr":
            return "%s far *%s" % (t.base, name)
        return "%s %s" % (t.spelling(), name)

    def write_item(a: int, b: int, name: str, t, d: Decl | None) -> bool:
        n = b - a
        if t is None:
            return False
        if t.kind == "array" and t.count == 0:
            e = t.elem
            if not e.size or n % e.size:
                return False
            k = n // e.size
            full = clayout.CType("array", n, elem=e, count=k)
        else:
            if t.size != n:
                return False
            full, k = t, (t.count if t.kind == "array" else None)
        try:
            if full.kind == "array" and full.elem.kind not in ("int",) or full.kind in ("struct", "union"):
                if full.kind == "array":
                    vals = [value(full.elem, a + i * full.elem.size) for i in range(k)]
                else:
                    vals = None
                    v = value(full, a)
            else:
                vals = None
                v = value(full, a)
        except Fallback:
            return False
        h = head_of(name, full, d, k if (t.kind == "array" and (t.count == 0 or d is None)) else None)
        if vals is None and v.startswith("{ ") and (full.kind == "array" or len(v) > 90):
            vals = split_inits(v)
        if vals is not None:
            per = 1 if any(len(x) > 30 for x in vals) else (4 if any(len(x) > 8 for x in vals) else 12)
            out.append(braces(h, vals, a, per=per))
        else:
            out.append("%s = %s; /*@ ds:%04X */" % (h, v, a))
        emitted[a] = (name, full)
        return True

    def write_bytes(a: int, b: int, name: str, why: str = "") -> None:
        n = b - a
        if n % 4 == 0 and all(a + 4 * i + 2 in relocs for i in range(n // 4)):
            try:
                vals = [pointer(a + 4 * i, None) for i in range(n // 4)]
                out.append(braces("void far *%s[%d]" % (name, n // 4), vals, a, why,
                                  per=4 if any(len(v) > 8 for v in vals) else 12))
                emitted[a] = (name, None)
                return
            except Fallback:
                pass
        rel = [r for r in relocs if a <= r < b]
        if rel:
            why += " /* relocated words at %s: far pointers to write by name */" % ", ".join("ds:%04X" % r for r in sorted(rel))
        out.append(braces("unsigned char %s[%d]" % (name, n), ["0x%02X" % c for c in img[base + a:base + b]], a, why))
        emitted[a] = (name, clayout.CType("array", n, elem=clayout.CType("int", 1, name="unsigned char"), count=n))

    # pass 1 decides each item's written type; references to items written as bytes then cast, so the
    # draft is made twice and the second pass knows every item's type
    for _pass in (0, 1):
        out[2:] = []
        needs.clear()
        k = lo
        while k < hi:
            if k in owned_at:
                b = owned_at[k]
                text = img[base + k:base + b - 1]
                if b == hi and b - k == 1 and byte(k) == 0:
                    out.append("/* ds:%04X: the pad byte to the next module (word alignment) */" % k)
                else:
                    out.append("/* ds:%04X-%04X: the code's (a literal or template, %r): written by its function */" % (
                        k, b, text[:40].decode("latin-1")))
                k = b
                continue
            if k in strings:
                b, lit = strings[k]
                out.append("/* ds:%04X-%04X: the string pool, %s (a table's literal) */" % (k, b, lit if len(lit) < 50 else lit[:46] + '..."'))
                k = b
                continue
            if k in items:
                name, t, d = items[k]
                b = extent(k)
                if not write_item(k, b, name, t, d):
                    why = " /* declared `%s` (%s): write it as its type */" % (" ".join(d.text.split()), d.unit) if d else ""
                    write_bytes(k, b, name, why)
                k = b
                continue
            # an unnamed stretch: to the next item, owned piece or string, cut at far pointer runs
            b = min([x for x in list(items) + list(owned_at) + list(strings) if x > k] + [hi])
            run = k + 2 in relocs
            j = k
            while j < b and ((j + 2 in relocs) == run):
                j += 4 if run else 1
            j = min(j, b)
            if j == k:
                j = b
            if not run and j - k == 1 and b == hi and byte(k) == 0 and k % 2:
                out.append("/* ds:%04X: the pad byte to the next module (word alignment) */" % k)
            else:
                write_bytes(k, j, "g%04X" % k)
            k = j
    if needs:
        out[2:2] = ["/* what the pointers name that the file does not declare before them: */"] + sorted(needs.values())
    return "\n".join(out) + "\n"


def split_inits(v: str) -> list[str]:
    """The top-level members of `{ a, { b, c }, "d,e" }`."""
    inner = v.strip()[2:-2]
    out, depth, cur, q = [], 0, [], False
    for i, c in enumerate(inner):
        if c == '"' and (i == 0 or inner[i - 1] != "\\"):
            q = not q
        if not q:
            if c == "{":
                depth += 1
            elif c == "}":
                depth -= 1
            if c == "," and depth == 0:
                out.append("".join(cur).strip())
                cur = []
                continue
        cur.append(c)
    out.append("".join(cur).strip())
    return out


def scalars(img: bytes, base: int, a: int, n: int, t: str) -> list[str]:
    e = SIZES[t]
    fmt = {1: "b", 2: "h", 4: "l"}[e]
    if "unsigned" in t:
        fmt = fmt.upper()
    vs = struct.unpack_from("<%d%s" % (n // e, fmt), img, base + a)
    suffix = ("UL" if "unsigned" in t else "L") if e == 4 else ""
    return [("0x%0*X" % (2 * e, v) if "unsigned" in t else "%d" % v) + suffix for v in vs]


def braces(head: str, vals: list[str], at: int, tail: str = "", per: int = 12) -> str:
    """`head = { values };` with the annotation on the first line, where the matcher reads it; the
    values wrapped at 100 columns (per=1: one to a line, for a table of structs)."""
    one = "%s = { %s }; /*@ ds:%04X */%s" % (head, ", ".join(vals), at, tail)
    if per != 1 and len(one) - len(tail) <= 100:
        return one
    rows, cur = [], ""
    for v in vals:
        if cur and (per == 1 or len(cur) + len(v) + 2 > 96):
            rows.append(cur)
            cur = ""
        cur = v if not cur else cur + ", " + v
    rows.append(cur)
    return "%s = { /*@ ds:%04X */%s\n    %s\n};" % (head, at, tail, ",\n    ".join(rows))


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

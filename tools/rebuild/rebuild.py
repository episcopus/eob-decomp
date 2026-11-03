#!/usr/bin/env python3
"""rebuild.py: build a program from src/ with the pinned toolchain and compare it with the original
(Phase 5, playbook 50).

  rebuild.py START1.EXE|INTRO.EXE [-v]

The units of the program (re/units.toml, not its start-up code or runtime library) are compiled
and assembled from working copies of their files in build/rebuild/<program>/, then linked by TLINK
with the start-up code and the libraries of the program's memory model, in address order: the
original link order, since each module follows the one before it. An INTRO.EXE unit proven as an
EOB.EXE twin (`make twin`) is built from its twin's file, whose addresses are EOB.EXE's: they are
translated, a function's by its offset in the unit, a global's by the relabelling the twin proof
finds. Overlay units are linked with TLINK's /o (VROOMM). What only a full link needs, the copies
get here and src/ does not:

  - one name per address. `make match` ties everything by address, so a C file may call an
    assembly routine `PutChar` that the assembly defines as `PutChar` (C looks for `_PutChar`), and
    the function the start-up code calls is whatever its file named it. In the copies every address
    gets one symbol: its defining file's, `_name` when C defines or calls it, `main` for the function
    the start-up code calls;
  - the data. src/ declares most globals as externs at their DGROUP offsets (`/*@ ds:00A8 */`), so
    nothing defines them; the modules bring only what they define (string literals, assembly data).
    A probe link shows where each module's data falls; each one is found in the original by its
    bytes (pieces of it vote for one place), and the gaps between are filled by generated modules
    (FILLnn.ASM) holding the original's bytes, with a public at each declared offset. The program's
    uninitialised data (_BSS, past the end of the load image) is reserved the same way.

Then the load image, the relocation table and (when those agree) the whole file are compared with
the original's, and every range that differs is reported with the unit or region it lies in. Exit
code 0 when the image and the relocations are identical. The working files stay in
build/rebuild/<program>/ (OUT.EXE, OUT.MAP, the copies, the fillers).
"""
from __future__ import annotations

import argparse
import json
import re
import shutil
import sys
import tomllib
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools" / "re"))
sys.path.insert(0, str(ROOT / "tools" / "match"))
import tc_toolchain as tc  # noqa: E402
from mzutil import read_mz  # noqa: E402
import cmatch  # noqa: E402
import rename  # noqa: E402

# the start-up code and libraries of each program (whitepaper 2.1, 2.6)
PROGRAMS = {"START1.EXE": {"model": "S", "libs": ["CS"]},
            "INTRO.EXE": {"model": "L", "libs": ["OVERLAY", "CL"]},
            "EOB.EXE": {"model": "L", "libs": ["OVERLAY", "CL"]}}
DATA_DEF = re.compile(r"^\s*([A-Za-z_]\w*)\s+(?:db|dw|dd|label)\b", re.M | re.I)


def lin(addr: str) -> int:
    s, o = addr.split(":")
    return int(s, 16) * 16 + int(o, 16)


def all_units() -> list[dict]:
    return tomllib.loads((ROOT / "re/units.toml").read_text())["unit"]


def own_units(program: str) -> list[dict]:
    us = [u for u in all_units() if u["program"] == program and u["kind"] not in ("startup", "library")]
    return sorted(us, key=lambda u: (u["start"].startswith("ovl"), lin(u["start"]) if ":" in u["start"] and
                                     not u["start"].startswith("ovl") else int(u["start"][3:].split(":")[0])))


class Space:
    """Addresses of a program; a twin file's EOB.EXE addresses translated into it."""

    def __init__(self, program: str):
        self.program = program
        units = all_units()
        self.by_id = {u["id"]: u for u in units}
        self.twin_of = {u["twin"]: u for u in units if u["program"] == program and u.get("twin")}
        self.eob = [u for u in units if u["program"] == "EOB.EXE" and ":" in u["start"] and not u["start"].startswith("ovl")]
        self.data: dict[int, int] = {}                 # EOB ds offset -> this program's (twins)
        self.far: dict[tuple, tuple] = {}              # EOB (seg, off) -> this program's, from the twin proofs
        self.seg: dict[int, int] = {}                  # EOB segment -> this program's, from the twin proofs
        self.lib = {p: self.library(p) for p in ("EOB.EXE", program)}
        man = json.loads((ROOT / "build/re" / ("%s.json" % program.split(".")[0].lower())).read_text())
        self.stubs = {o["stub_para"]: (o["overlay"], {0x20 + 5 * e["index"]: e["code_offset"] for e in o["entries"]})
                      for o in man.get("overlays", [])}
        self.segments = man.get("segments", [])
        if program != "EOB.EXE" and self.twin_of:
            import twin
            pairs = Counter()
            E, I = twin.Program("EOB.EXE"), twin.Program(program)
            us, fs = twin.registry()
            for e_id, iu in self.twin_of.items():
                why, seen = twin.check(e_id, iu["id"], E, I, us, fs)
                for (kind, x), y in seen.items():
                    if kind == "far":
                        bx, by = bytes.fromhex(x), bytes.fromhex(y)
                        self.far[(int.from_bytes(bx[2:], "little"), int.from_bytes(bx[:2], "little"))] = (
                            int.from_bytes(by[2:], "little"), int.from_bytes(by[:2], "little"))
                    elif kind == "seg":
                        self.seg[x] = y
                    if kind in ("disp", "imm") and len(x) == 4:
                        pairs[(int.from_bytes(bytes.fromhex(x), "little"), int.from_bytes(bytes.fromhex(y), "little"))] += 1
            self.pairs = sorted(pairs)

    @staticmethod
    def library(program: str) -> tuple[range, dict]:
        """The library unit's linear range and its publics by address (rtl_compare)."""
        u = next((u for u in all_units() if u["program"] == program and u["kind"] == "library"), None)
        rtl = {"EOB.EXE": "rtl_compare.json", "INTRO.EXE": "rtl_intro.json", "START1.EXE": "rtl_start1.json"}[program]
        syms = json.loads((ROOT / "build/re" / rtl).read_text())["tc100"].get("symbols", {}) if (ROOT / "build/re" / rtl).exists() else {}
        by = {}
        for name, a in sorted(syms.items()):
            by.setdefault(a, name)
        Space.aliases.setdefault(program, {})
        for name, a in syms.items():
            Space.aliases[program].setdefault(a, set()).add(name)
        return (range(lin(u["start"]), lin(u["start"]) + u["bytes"]) if u else range(0)), by

    aliases: dict = {}                                 # program -> library address -> every public there

    _ref = None

    @classmethod
    def reference(cls) -> tuple[bytes, dict]:
        """The large-model reference link of every library module (rtl_compare): its image and publics."""
        if cls._ref is None:
            d = ROOT / "build/re/rtl/tc100/l"
            text = (d / "r.map").read_text("latin-1")
            text = text[text.index("Publics by Value"):]
            pubs = {}
            for m in re.finditer(r"^\s*([0-9A-F]{4}):([0-9A-F]{4})\s+(?:idle\s+)?(\S+)\s*$", text, re.M):
                pubs.setdefault(int(m.group(1), 16) * 16 + int(m.group(2), 16), m.group(3))
            cls._ref = (read_mz((d / "r.exe").read_bytes())["image"], pubs)
        return cls._ref

    def library_name(self, program: str, a: int) -> str | None:
        """A library function rtl_compare did not name: its code found in the reference link, address
        fields aside (EOB.EXE 0000:60BD, INTRO.EXE 0000:549D is _close)."""
        img = read_mz((ROOT / "eob" / program).read_bytes())["image"]
        ref, pubs = self.reference()
        from capstone import CS_ARCH_X86, CS_MODE_16, Cs
        md = Cs(CS_ARCH_X86, CS_MODE_16)
        md.detail = True
        code, mask = img[a:a + 48], []
        for i in md.disasm(code, 0):
            skip = set()
            if i.bytes[0] in (0x9A, 0xEA):
                skip = set(range(1, 5))
            elif i.mnemonic == "call" or i.mnemonic.startswith("j"):
                skip = set(range(i.imm_offset, i.imm_offset + i.imm_size)) if i.imm_size else set()
            elif i.disp_size == 2:
                skip = set(range(i.disp_offset, i.disp_offset + 2))
            mask += [k not in skip for k in range(i.size)]
        n = len(mask)
        head = bytes(code[:next((k for k, m in enumerate(mask) if not m), n)])[:8]
        hits = []
        for m in re.finditer(re.escape(head), ref) if len(head) >= 4 else []:
            r = m.start()
            if all(not mask[k] or ref[r + k] == code[k] for k in range(n)):
                hits.append(r)
        return pubs.get(hits[0]) if len(hits) == 1 else None

    def resolve(self, seg, off, eob_file: bool) -> tuple[str, str]:
        """("addr", this program's address) or ("sym", a library public)."""
        if not isinstance(seg, str):
            a = seg * 16 + off
            if eob_file:
                rng, by = self.lib["EOB.EXE"]
                if a in rng:
                    name = by.get(a) or self.library_name("EOB.EXE", a)
                    if name:
                        return "sym", name
            key = self.func(seg, off, eob_file)
            if key.startswith("EOB:") and (seg, off) in self.far:
                s2, o2 = self.far[(seg, off)]
                key = "%04X:%04X" % (s2, o2)
            if not key.startswith("EOB:"):
                s2, o2 = (int(x, 16) for x in key.split(":"))
                rng, by = self.lib[self.program]
                if s2 * 16 + o2 in rng:
                    name = by.get(s2 * 16 + o2) or self.library_name(self.program, s2 * 16 + o2)
                    if name:
                        return "sym", name
                if s2 in self.stubs and o2 in self.stubs[s2][1]:
                    return "addr", "ovl%d:%04X" % (self.stubs[s2][0], self.stubs[s2][1][o2])
            return "addr", key
        return "addr", self.func(seg, off, eob_file)

    def func(self, seg, off, eob_file: bool) -> str:
        if isinstance(seg, str):
            return "%s:%04X" % (seg.lower(), off)
        a = seg * 16 + off
        if not eob_file or self.program == "EOB.EXE":
            return "%04X:%04X" % (seg, off)
        e = next((u for u in self.eob if lin(u["start"]) <= a < lin(u["start"]) + u["bytes"]), None)
        i = self.twin_of.get(e["id"]) if e else None
        if not i:
            return "EOB:%04X:%04X" % (seg, off)        # no twin here: stays unresolved, reported by TLINK
        t = lin(i["start"]) + a - lin(e["start"])
        para = int(i["segment"], 16)
        return "%04X:%04X" % (para, t - para * 16)

    def ds(self, off: int, eob_file: bool) -> int | None:
        if not eob_file or self.program == "EOB.EXE":
            return off
        near = [(abs(x - off), y - x) for x, y in self.pairs if abs(x - off) <= 64]
        if not near:
            return None
        best = min(d for d, _ in near)
        delta = Counter(dl for d, dl in near if d <= best + 8).most_common(1)[0][0]
        return (off + delta) & 0xFFFF


def sources(program: str) -> list[tuple[Path, bool, dict]]:
    """(file, written with EOB.EXE's addresses, unit) in link order: by the lowest address each
    file defines, overlay units last."""
    out = []
    for k, u in enumerate(own_units(program)):
        stem = (u.get("twin") or u["id"]).upper()
        for p in sorted((ROOT / "src").glob(stem + ".*")):
            if p.suffix.upper() in (".C", ".ASM"):
                funcs, _, defined, stubs = cmatch.parse_annotations(p.read_text(), p.suffix.upper() == ".ASM")
                own = [funcs[n] for n in defined - stubs]
                if own:
                    first = min(o if isinstance(s, str) else s * 16 + o for s, o in own)
                    out.append(((k, first), p, bool(u.get("twin")) or program == "EOB.EXE", u))
    return [(p, eob, u) for _, p, eob, u in sorted(out, key=lambda t: t[0])]


def find_main(program: str, img: bytes, relocs: set) -> str | None:
    """The function the start-up code calls: the first call from it into the program's own code."""
    c0 = next(u for u in all_units() if u["program"] == program and u["kind"] == "startup")
    lo, hi = lin(c0["start"]), lin(c0["start"]) + c0["bytes"]
    from capstone import CS_ARCH_X86, CS_MODE_16, Cs
    own = [(lin(u["start"]), lin(u["start"]) + u["bytes"]) for u in own_units(program)
           if not u["start"].startswith("ovl")]
    for i in Cs(CS_ARCH_X86, CS_MODE_16).disasm(img[lo:hi], lo):
        t = None
        if i.bytes[0] == 0xE8:
            t = i.address + 3 + int.from_bytes(i.bytes[1:3], "little", signed=True)
        elif i.bytes[0] == 0x9A and i.address + 3 in relocs:
            t = int.from_bytes(i.bytes[3:5], "little") * 16 + int.from_bytes(i.bytes[1:3], "little")
        if t is not None and any(a <= t < b for a, b in own):
            para = next(int(u["segment"], 16) for u in own_units(program) if not u["start"].startswith("ovl")
                        and lin(u["start"]) <= t < lin(u["start"]) + u["bytes"])
            return "%04X:%04X" % (para, t - para * 16)
    return None


class Copy:
    def __init__(self, src: Path, eob: bool, unit: dict, w: Path):
        self.src, self.eob, self.unit = src, eob, unit
        self.asm = src.suffix.upper() == ".ASM"
        self.path = w / (src.stem.upper() + (".ASM" if self.asm else ".C"))
        self.text = src.read_text()
        self.funcs, self.data, self.defined, self.stubs = cmatch.parse_annotations(self.text, self.asm)
        self.fardata = set(cmatch.FARDATA)              # externs at seg:off that are data (far objects)
        self.data_defined = set(DATA_DEF.findall(self.text)) if self.asm else {
            n for n in self.data if not re.search(r"\bextern\b[^;]*\b%s\b" % re.escape(n), self.text)}


def make_copies(program: str, w: Path, space: Space, main_addr: str | None) -> list[Copy]:
    copies = [Copy(p, eob, u, w) for p, eob, u in sources(program)]
    stems = Counter(c.path.stem for c in copies)
    for c in copies:                                    # S_TEXT.C and S_TEXT.ASM: two objects, two names
        if c.asm and stems[c.path.stem] > 1:
            c.path = w / (c.path.stem[:7] + "A.ASM")
    fsym, dsym, c_uses = {}, {}, set()
    for c in copies:                                    # each function address's symbol
        for n in c.defined - c.stubs:
            a = space.func(*c.funcs[n], c.eob)
            bare = n[1:] if c.asm and n.startswith("_") else n
            fsym[a] = "main" if a == main_addr else bare
            if not c.asm:
                c_uses.add(a)
        if not c.asm:
            c_uses.update(space.resolve(*c.funcs[n], c.eob)[1] for n in c.funcs)
    seen = {}
    for a in sorted(fsym):                              # two addresses, one name (two drivers' SetVoicePitch):
        key = fsym[a].lower()                           # the later one carries its address, in the copies only
        if key in seen and fsym[a] != "main":
            fsym[a] = "%s_%s" % (fsym[a], a.replace(":", "_"))
        seen[key] = a
    for c in copies:                                    # data an assembly or C module defines itself
        for n in c.data_defined & set(c.data):
            d = space.ds(c.data[n], c.eob)
            if d is not None:
                dsym[d] = n if c.asm else "_" + n
    unresolved, far = [], {}
    asm_labels = {n: c for c in copies if c.asm for n in DATA_DEF.findall(c.text)}
    for c in copies:
        text = c.text
        for n, (seg, off) in c.funcs.items():
            if n in c.fardata and space.resolve(seg, off, c.eob)[1] in fsym:
                pass                                    # a far object that is a routine (Int3FHandler): by address
            elif n in c.fardata:                        # a far object: placed below
                bare = n[1:] if c.asm and n.startswith("_") else n
                if bare not in asm_labels or asm_labels[bare] is c:
                    s2 = seg if (isinstance(seg, str) or not c.eob or program == "EOB.EXE") else space.seg.get(seg, seg)
                    far.setdefault((s2, off), set()).add(n if c.asm else "_" + n)
                continue
            kind, a = space.resolve(seg, off, c.eob)
            if kind == "sym":                           # a library function, by its library name, unless
                mine = n if c.asm else "_" + n          # the source already uses one of its names (remove/unlink)
                if any(mine in names_at for names_at in Space.aliases.get("EOB.EXE" if c.eob else program, {}).values()):
                    continue
                want = a if c.asm else (a[1:] if a.startswith("_") else a)
                if n != want:
                    text, _ = rename.replace_word(text, n, want, c.asm)
                continue
            if a not in fsym:
                continue
            want = ("_" + fsym[a] if (a in c_uses or fsym[a] == "main") else fsym[a]) if c.asm else fsym[a]
            if n != want:
                text, _ = rename.replace_word(text, n, want, c.asm)
        for n, off in c.data.items():
            d = space.ds(off, c.eob)
            if d is None:
                if n not in c.data_defined:
                    unresolved.append("%s %s ds:%04X" % (c.src.name, n, off))
                continue
            if d in dsym and n not in c.data_defined:
                want = dsym[d] if c.asm else dsym[d].lstrip("_")
                if n != want:
                    text, _ = rename.replace_word(text, n, want, c.asm)
        c.text_out = text
    # a far object an assembly module defines (gMovedSeg, a word in I0923.ASM's code): exported to C
    c_far = {n[1:] for c in copies if not c.asm for n in (("_" + m) for m in c.fardata)}
    for name in sorted(c_far & set(asm_labels)):
        d = asm_labels[name]
        d.text_out, _ = rename.replace_word(d.text_out, name, "_" + name, True)
        d.text_out = re.sub(r"(^\s*\.CODE\b[^\n]*\n)", r"\1        PUBLIC  _%s\n" % name, d.text_out, count=1, flags=re.M | re.I)
    for c in copies:
        # a large-model assembly unit in segment 0000 (INTRO.EXE's AdLib driver) is part of _TEXT, with
        # the start-up code and the library; TASM's .CODE would name its segment after the file
        if c.asm and PROGRAMS[program]["model"] == "L" and c.unit["start"].startswith("0000:"):
            c.text_out = re.sub(r"^(\s*\.CODE)\s*$", r"\1 _TEXT", c.text_out, flags=re.M | re.I)
        c.path.write_text(c.text_out.replace("\r\n", "\n").replace("\n", "\r\n"))
    if unresolved:
        print("rebuild: globals of twin files not placed in %s: %s" % (program, "; ".join(unresolved[:8])))
    # every global's symbols, by this program's DGROUP offset (generated labels for the undefined ones)
    names: dict[int, list[str]] = {}
    for c in copies:
        _, data, _, _ = cmatch.parse_annotations(c.text_out, c.asm)
        for n, off in data.items():
            d = space.ds(off, c.eob)
            if d is None or d in dsym:
                continue
            sym = n if c.asm else "_" + n
            if sym not in names.setdefault(d, []):
                names[d].append(sym)
    return copies, names, far


def far_objects(program: str, space: "Space", far: dict, image: bytes) -> str | None:
    """FAR.ASM: the far objects src/ declares outside DGROUP. A data segment of the original's (0E6C,
    640 bytes in INTRO.EXE) is its bytes, class FAR_DATA, which TLINK places after the code; the two
    overlay-information segments TLINK fills (the start-up code's _OVERLAY_ and the overlay manager's
    _OVRDATA_, in that order in the segment table) get a label at their start."""
    if not far:
        return None
    segs = space.segments
    info = sorted({sg["para"] for sg in segs if sg["kind_name"] not in ("code", "data", "overlay-stub")
                   and sg["para"] > max(x["para"] for x in segs if x["kind_name"] == "code" and x["size"])
                   and sg["para"] < max(x["para"] for x in segs if x["kind_name"] == "data")})
    named = {info[0]: ("_OVERLAY_", None), info[1]: ("_OVRDATA_", "_OVRGROUP_")} if len(info) >= 2 else {}
    out = []
    for (seg, off), syms in sorted(far.items(), key=lambda kv: (str(kv[0][0]), kv[0][1])):
        if isinstance(seg, str):
            continue
        out.append((seg, off, sorted(syms)))
    lines, groups = [], []
    pubs = ["PUBLIC %s" % s for _, _, ss in out for s in ss]
    for seg in sorted({x[0] for x in out}):
        here = [x for x in out if x[0] == seg]
        if seg in named:
            name, grp = named[seg]
            if grp:
                groups.append("%s GROUP %s" % (grp, name))
            lines += ["%s SEGMENT PARA PUBLIC 'OVRINFO'" % name] + ["%s label byte" % s for _, o, ss in here for s in ss if o == 0]
            lines.append("%s ENDS" % name)
            continue
        size = next((x["size"] for x in segs if x["para"] == seg and x["kind_name"] == "data" and x["size"]), None)
        if size is None:
            print("rebuild: far objects at %04X (%s) are in no data segment of the original" % (seg, ", ".join(s for _, _, ss in here for s in ss)))
            continue
        lines.append("FAR_%04X SEGMENT PARA PUBLIC 'FAR_DATA'" % seg)
        pos = 0
        for o in sorted({o for _, o, _ in here}) + [size]:
            chunk = image[seg * 16 + pos:seg * 16 + o]
            lines += ["db " + ",".join("%03Xh" % b for b in chunk[k:k + 16]) for k in range(0, len(chunk), 16)]
            if o < size:
                lines += ["%s label byte" % s for _, oo, ss in here if oo == o for s in ss]
            pos = o
        lines.append("FAR_%04X ENDS" % seg)
    return "\r\n".join(groups + pubs + lines + ["END"]) + "\r\n"


def filler(tag: str, image: bytes, base: int, lo: int, hi: int, names: dict, init: bool) -> str:
    """A module of the original's DGROUP bytes [lo, hi) (or reserved, for _BSS), a public at each name."""
    seg = "_DATA" if init else "_BSS"
    pub = sorted(o for o in names if lo <= o < hi)
    lines = ["DGROUP GROUP _DATA, _BSS"] + ["PUBLIC %s" % s for o in pub for s in names[o]]
    other = "_BSS SEGMENT BYTE PUBLIC 'BSS'\r\n_BSS ENDS" if init else "_DATA SEGMENT BYTE PUBLIC 'DATA'\r\n_DATA ENDS"
    lines.append(other)
    lines.append("%s SEGMENT BYTE PUBLIC '%s'" % (seg, "DATA" if init else "BSS"))
    pos = lo
    for o in pub + [hi]:
        if o > pos:
            if init:
                chunk = image[base + pos:base + o]
                lines += ["db " + ",".join("%03Xh" % b for b in chunk[k:k + 16]) for k in range(0, len(chunk), 16)]
            else:
                lines.append("db %d dup (?)" % (o - pos))
        if o < hi:
            lines += ["%s label byte" % s for s in names[o]]
        pos = max(pos, o)
    lines += ["%s ENDS" % seg, "END"]
    return "\r\n".join(lines) + "\r\n"


def build_objs(w: Path, copies: list[Copy]) -> None:
    for c in copies:
        if c.asm:
            r = tc.run(tc.manifest()["pinned_asm"], "TASM", ["/mx", c.path.name], cwd=w)
        else:
            m = cmatch.FLAGS.search(c.text_out)
            flags = m.group(1).split() if m else tc.manifest()["cflags"].split()
            args = ["-c", "-ID:\\INCLUDE"] + [f for f in flags if f != "-B"] + [c.path.name]
            r = tc.run("tc100", "TCC", args, cwd=w)
            # an extern renamed to its library name may clash with the header's own prototype of it
            # (E1C0E.C's remove, the library's unlink, with dos.h's `unlink(const char *)`): the copy's
            # declaration goes, the header's stands, and the code is the same
            for name in re.findall(r"Type mismatch in redeclaration of '(\w+)'", r.stdout.decode("latin-1")):
                text = c.path.read_text()
                text = re.sub(r"^extern[^;\n]*\b%s\s*\([^;]*;[^\n]*$" % re.escape(name), "/* (the header declares %s) */" % name,
                              text, count=1, flags=re.M)
                c.path.write_text(text)
                r = tc.run("tc100", "TCC", args, cwd=w)
        if not any((w / n).exists() for n in (c.path.stem + ".OBJ", c.path.stem.lower() + ".obj")):
            raise SystemExit("rebuild: %s does not build:\n%s" % (c.path.name, r.stdout.decode("latin-1")[-1500:]))


def assemble(w: Path, name: str, text: str) -> None:
    (w / (name + ".ASM")).write_text(text)
    r = tc.run(tc.manifest()["pinned_asm"], "TASM", ["/mx", name + ".ASM"], cwd=w)
    out = r.stdout.decode("latin-1")
    if not re.search(r"Error messages:\s+None", out):
        raise SystemExit("rebuild: %s.ASM: %s" % (name, out[-800:]))


def link(program: str, w: Path, copies: list[Copy], fillers: dict[int, str], tail: str | None):
    """TLINK the start-up code, the modules in order (a filler after the module it follows, the
    overlay units under /o) and the libraries."""
    cfg = PROGRAMS[program]
    objs, overlaid = [], False
    for k in sorted(fillers):
        if k < 0:
            objs.append(fillers[k])
    for i, c in enumerate(copies):
        ovl = c.unit["start"].startswith("ovl")
        if ovl != overlaid:
            objs.append("/o" if ovl else "/o-")
            overlaid = ovl
        objs.append(c.path.stem)
        if i in fillers:
            if overlaid:
                objs.append("/o-")
                overlaid = False
            objs.append(fillers[i])
    if overlaid:
        objs.append("/o-")
    if (w / "FAR.OBJ").exists() or (w / "far.obj").exists():
        objs.append("FAR")
    if (w / "EXTREF.OBJ").exists() or (w / "extref.obj").exists():
        objs.append("EXTREF")
    if tail:
        objs.append(tail)
    for f in ("OUT.EXE", "OUT.MAP", "out.exe", "out.map"):
        (w / f).unlink(missing_ok=True)
    # a DOS command line holds 127 characters: the objects go in a response file, one per line
    resp = ["D:\\LIB\\C0%s" % cfg["model"]] + objs
    (w / "OUT.LNK").write_text(" +\r\n".join(resp) + "\r\nOUT\r\nOUT\r\n" +
                               "+".join("D:\\LIB\\" + l for l in cfg["libs"]) + "\r\n")
    r = tc.run("tc100", "TLINK", ["/c", "/m", "/s", "@OUT.LNK"], cwd=w)
    log = r.stdout.decode("latin-1")
    exe = next((w / n for n in ("OUT.EXE", "out.exe") if (w / n).exists()), None)
    if not exe or re.search(r"^Error", log, re.M):
        return None, log
    return exe, log


def dgroup_contribs(w: Path) -> tuple[int, list]:
    """Our DGROUP frame and its contributions (offset, length, segment, module) in map order."""
    text = next((w / n for n in ("OUT.MAP", "out.map") if (w / n).exists())).read_text("latin-1")
    rows = re.findall(r"^\s*([0-9A-F]{4}):([0-9A-F]{4}) ([0-9A-F]{4}) C=(\w+)\s+S=(\S+)\s+G=(\S+)\s+M=(\S+)", text, re.M)
    dg = next(int(f, 16) for f, o, l, c, s, g, m in rows if g == "DGROUP")
    out = [((int(f, 16) - dg) * 16 + int(o, 16), int(l, 16), s, m.upper()) for f, o, l, c, s, g, m in rows if g == "DGROUP"]
    return dg, out


def locate(ours: bytes, orig: bytes, start: int, lo: int, hi: int) -> int | None:
    """Where our bytes [lo, hi) are in the original, at or after `start`: pieces vote for one shift."""
    votes = Counter()
    step = 8 if hi - lo > 64 else 1
    size = min(32, hi - lo)
    for k in range(lo, max(lo + 1, hi - size + 1), step):
        piece = ours[k:k + size]
        if len(piece) < 4 or len(set(piece)) < min(4, len(piece)):
            continue
        hits = [m.start() for m in re.finditer(re.escape(piece), orig[start:])]
        if len(hits) == 1:
            votes[start + hits[0] - k] += 1
    return lo + votes.most_common(1)[0][0] if votes else None


def missing_library(program: str, w: Path, space: "Space") -> str | None:
    """A library module the original links and ours does not: the first library module whose place
    differs, with ours earlier, follows one; the original's code there, found in the reference link,
    names it (INTRO.EXE's GETCWD, which nothing calls: an assembly module's unused EXTRN pulled it)."""
    rtl = {"EOB.EXE": "rtl_compare.json", "INTRO.EXE": "rtl_intro.json", "START1.EXE": "rtl_start1.json"}[program]
    found = json.loads((ROOT / "build/re" / rtl).read_text())["tc100"]["found"]
    text = next((w / n for n in ("OUT.MAP", "out.map") if (w / n).exists())).read_text("latin-1")
    rows = re.findall(r"^\s*([0-9A-F]{4}):([0-9A-F]{4}) ([0-9A-F]{4}) C=CODE\s+S=(\S+)\s+G=\S+\s+M=(\S+)", text, re.M)
    ours = {m.upper(): (int(f, 16) * 16 + int(a, 16), int(l, 16)) for f, a, l, sg, m in rows}
    prev_end = None
    for name, at in sorted(found.items(), key=lambda kv: kv[1]):
        if name.upper() not in ours or ours[name.upper()][1] <= 2:
            continue
        o, l = ours[name.upper()]
        if o == at:
            prev_end = at + l
            continue
        if o < at and prev_end is not None:
            return space.library_name(program, prev_end)
        return None
    return None


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("program")
    ap.add_argument("-v", "--verbose", action="store_true")
    ap.add_argument("--extern", action="append", default=[], help=argparse.SUPPRESS)
    a = ap.parse_args(argv)
    program = a.program.upper()
    raw = (ROOT / "eob" / program).read_bytes()
    orig = read_mz(raw)
    oimg, orel = orig["image"], {s * 16 + o for o, s in orig["relocs"]}
    odg = cmatch.dgroup_of(ROOT / "eob" / program)
    w = ROOT / "build" / "rebuild" / program.split(".")[0].lower()
    shutil.rmtree(w, ignore_errors=True)
    w.mkdir(parents=True)
    space = Space(program)
    copies, names, far = make_copies(program, w, space, find_main(program, oimg, orel))
    if far and a.verbose:
        print("rebuild: far objects to place: %s" % ", ".join("%s at %s:%04X" % ("/".join(sorted(v)), k[0] if isinstance(k[0], str) else "%04X" % k[0], k[1]) for k, v in sorted(far.items(), key=str)))
    build_objs(w, copies)
    if a.extern:                                        # library modules nothing calls (see missing_library)
        assemble(w, "EXTREF", "\r\n".join([".MODEL LARGE" if PROGRAMS[program]["model"] == "L" else ".MODEL SMALL"] +
                                           ["EXTRN %s:FAR" % x for x in a.extern] + ["END"]) + "\r\n")
    far_mod = far_objects(program, space, far, oimg)
    if far_mod:
        assemble(w, "FAR", far_mod)
    syms = [s for o in sorted(names) for s in names[o]]
    probe = "\r\n".join(["DGROUP GROUP _DATA, _BSS"] + ["PUBLIC %s" % s for s in syms] +
                        ["_DATA SEGMENT BYTE PUBLIC 'DATA'", "_DATA ENDS", "_BSS SEGMENT BYTE PUBLIC 'BSS'"] +
                        ["%s label byte" % s for s in syms] +
                        ["_BSS ENDS", "END"]) + "\r\n"
    assemble(w, "PROBE", probe)
    exe, log = link(program, w, copies, {}, "PROBE")
    if not exe:
        print(log[-3000:])
        return 1
    pimg = read_mz(exe.read_bytes())["image"]
    pdg, contribs = dgroup_contribs(w)
    odata = oimg[odg * 16:]
    image_end = len(oimg) - odg * 16
    mods = {c.path.stem.upper() for c in copies}
    ovl = {c.path.stem.upper() for c in copies if c.unit["start"].startswith("ovl")}
    c0_end = next(o + l for o, l, s, m in contribs if s == "_DATA" and m.startswith("C0"))
    mine = [(o, l, m.split(".")[0]) for o, l, s, m in contribs if l and s == "_DATA" and m.split(".")[0] in mods]
    # where each module's data is in the original: the resident ones in link order, an overlay
    # module's anywhere (its code is not in the image, so only its data shows where it was linked)
    placed, cursor = {}, c0_end
    for o, l, stem in mine:
        if stem in ovl:
            continue
        at = locate(pimg[pdg * 16:], odata, cursor, o, o + l)
        if at is None:
            print("rebuild: %s's data (%d bytes) is not in the original" % (stem, l))
            return 1
        placed[stem], cursor = (at, l), at + l
    for o, l, stem in mine:
        if stem in ovl:
            at = locate(pimg[pdg * 16:], odata, c0_end, o, o + l)
            if at is None:
                print("rebuild: %s's data (%d bytes) is not in the original" % (stem, l))
                return 1
            placed[stem] = (at, l)
    # an overlay module goes after the resident module whose data precedes its own (INTRO.EXE: IOV0
    # after I0892, its main module)
    resident = [c for c in copies if c.path.stem.upper() not in ovl]
    for c in [c for c in copies if c.path.stem.upper() in ovl]:
        if c.path.stem.upper() not in placed:
            resident.append(c)
            continue
        at = placed[c.path.stem.upper()][0]
        before = [r for r in resident if r.path.stem.upper() in placed and placed[r.path.stem.upper()][0] < at]
        k = resident.index(max(before, key=lambda r: placed[r.path.stem.upper()][0])) + 1 if before else len(resident)
        resident.insert(k, c)
    copies[:] = resident
    # the library's data: its contributions found by their bytes past the program's
    lib = [(o, l) for o, l, s, m in contribs if l and s == "_DATA" and not m.startswith("C0")
           and m.split(".")[0] not in mods and m not in ("PROBE", "FAR")]
    lib_at, end = None, max((a + l for a, l in placed.values()), default=c0_end)
    for o, l in lib:
        if l >= 16:
            at = locate(pimg[pdg * 16:], odata, end, o, o + l)
            if at is not None:
                lib_at = at - (o - lib[0][0])
                break
    if lib_at is None:
        lib_at = end
    # the fillers: the original's bytes between the start-up code's data, each module's and the library's
    order = [c.path.stem.upper() for c in copies]
    fillers, prev_end, prev_idx, k = {}, c0_end, -1, 0
    for stem in [x for x in order if x in placed]:
        at, l = placed[stem]
        if at > prev_end:
            fillers[prev_idx] = "FILL%02d" % k
            assemble(w, fillers[prev_idx], filler(fillers[prev_idx], oimg, odg * 16, prev_end, at, names, True))
            k += 1
        elif at < prev_end:
            print("rebuild: %s's data (ds:%04X) overlaps the module before it (to ds:%04X)" % (stem, at, prev_end))
        prev_end, prev_idx = at + l, order.index(stem)
    if lib_at > prev_end:
        fillers[prev_idx] = "FILL%02d" % k
        assemble(w, fillers[prev_idx], filler(fillers[prev_idx], oimg, odg * 16, prev_end, lib_at, names, True))
    # the uninitialised globals: past the image (START1.EXE), or, when the image holds all of DGROUP
    # (INTRO.EXE, its stack segment after it), the ones past every data filler and the library's data:
    # Turbo C makes them communal variables, which TLINK places after every module's _BSS (c_common)
    filled = [(c0_end, lib_at)]
    common = sorted(o for o in names if o >= image_end or (image_end > max(names, default=0) and o >= lib_at
                                                        and not any(lo <= o < hi for lo, hi in filled)))
    tail = None
    if common:
        dg_size = next((x["size"] for x in space.segments if x["para"] == odg), None) or (max(common) + 4)
        hi = max(common) + 4 if common[0] >= image_end else dg_size
        seg = "_BSS" if common[0] >= image_end else "c_common"
        text = filler("BSS", oimg, odg * 16, common[0], hi, names, False)
        if seg != "_BSS":
            text = text.replace("DGROUP GROUP _DATA, _BSS", "DGROUP GROUP _DATA, _BSS, c_common").replace(
                "_BSS SEGMENT BYTE PUBLIC 'BSS'\r\n", "_BSS SEGMENT BYTE PUBLIC 'BSS'\r\n_BSS ENDS\r\nc_common SEGMENT BYTE PUBLIC 'BSS'\r\n", 1)
            text = re.sub(r"^_BSS ENDS\r\nEND", "c_common ENDS\r\nEND", text, flags=re.M)
        assemble(w, "BSS", text)
        tail = "BSS"
    exe, log = link(program, w, copies, fillers, tail)
    if not exe:
        print(log[-3000:])
        return 1
    extra = missing_library(program, w, space)
    if extra and extra not in a.extern and len(a.extern) < 8:
        print("rebuild: the original links the library's %s, which nothing here calls: an EXTRN for it" % extra)
        return main((argv or sys.argv[1:]) + ["--extern", extra])
    ours = read_mz(exe.read_bytes())
    img, rel = ours["image"], {s * 16 + o for o, s in ours["relocs"]}
    print("rebuild: %s from %d files; data placed for %d modules, %d fillers, the library's data at ds:%04X" % (
        program, len(copies), len(placed), len(fillers), lib_at))
    if a.verbose:
        print("rebuild: link order %s" % " ".join(c.path.stem for c in copies))
    rc = compare(program, img, rel, oimg, orel, odg, a.verbose)
    if rc == 0 and exe.read_bytes() == raw:
        print("rebuild: %s: the whole file is identical, header included (%d bytes)" % (program, len(raw)))
    elif rc == 0:
        print("rebuild: %s: the load image and relocations are identical; the rest of the file differs" % program)
    return rc


def compare(program, img, rel, oimg, orel, dg, verbose) -> int:
    regions = [(lin(u["start"]), lin(u["start"]) + u["bytes"], u["id"]) for u in all_units()
               if u["program"] == program and ":" in u["start"] and not u["start"].startswith("ovl")]

    def where(x):
        if x >= dg * 16:
            return "DGROUP ds:%04X" % (x - dg * 16)
        return next(("%s +%04X" % (r, x - a) for a, b, r in regions if a <= x < b), "%05X" % x)
    diffs, k, n = [], 0, max(len(img), len(oimg))
    while k < n:
        if k < len(img) and k < len(oimg) and img[k] == oimg[k]:
            k += 1
            continue
        j = k
        while j < n and not (j < len(img) and j < len(oimg) and img[j] == oimg[j]):
            j += 1
        diffs.append((k, j))
        k = j
    print("rebuild: image %d bytes, original %d; relocations %d, original %d" % (len(img), len(oimg), len(rel), len(orel)))
    for x, y in diffs[:40 if verbose else 12]:
        print("  differs %05X-%05X (%d bytes) at %s: ours %s, original %s" % (
            x, y, y - x, where(x), img[x:min(y, x + 8)].hex(" "), oimg[x:min(y, x + 8)].hex(" ")))
    if len(diffs) > (40 if verbose else 12):
        print("  ... %d ranges in all" % len(diffs))
    if rel != orel:
        print("  relocations only ours: %s; only the original's: %s" % (
            " ".join("%05X" % x for x in sorted(rel - orel)[:8]), " ".join("%05X" % x for x in sorted(orel - rel)[:8])))
    same = not diffs and rel == orel
    print("rebuild: %s %s" % (program, "IDENTICAL" if same else "differs"))
    return 0 if same else 1


if __name__ == "__main__":
    sys.exit(main())

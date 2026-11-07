#!/usr/bin/env python3
"""rebuild.py: build a program from src/ with the pinned toolchain and compare it with the original
(Phase 5, playbook 50).

  rebuild.py START1.EXE|INTRO.EXE|EOB.EXE [-v]
  rebuild.py CGA.OVL|EGA.OVL|MCGA.OVL|TGA.OVL [-v]     a graphics driver from src/drivers/ (driver.py)

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
build/rebuild/<program>/ (<PROGRAM>.EXE and .MAP, the copies, the fillers).
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

# the start-up code and libraries of each program (whitepaper 2.1, 2.6); `library_units` are units the
# map took for Westwood code that are a library module: e1e8b (and INTRO.EXE's twin i0dd9) is
# OVERLAY.LIB's overlay manager OVRMAN, its code segment _OVRTEXT_ (the library's copy and the
# original's segment differ only in address fields, 2026-11-03), so the library brings it
PROGRAMS = {"START1.EXE": {"model": "S", "libs": ["CS"], "library_units": []},
            "INTRO.EXE": {"model": "L", "libs": ["OVERLAY", "CL"], "library_units": ["i0dd9"]},
            "EOB.EXE": {"model": "L", "libs": ["OVERLAY", "CL"], "library_units": ["e1e8b"]}}
DATA_DEF = re.compile(r"^\s*([A-Za-z_]\w*)\s+(?:db|dw|dd|label)\b", re.M | re.I)


def lin(addr: str) -> int:
    s, o = addr.split(":")
    return int(s, 16) * 16 + int(o, 16)


def all_units() -> list[dict]:
    return tomllib.loads((ROOT / "re/units.toml").read_text())["unit"]


def own_units(program: str) -> list[dict]:
    us = [u for u in all_units() if u["program"] == program and u["kind"] not in ("startup", "library")
          and u["id"] not in PROGRAMS.get(program, {}).get("library_units", [])]
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
        self.values: dict[str, dict] = {}              # EOB unit -> {(disp or imm, EOB value): this program's}
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
                    if kind in ("disp", "imm", "cs"):
                        self.values.setdefault(e_id, {})[(kind, int.from_bytes(bytes.fromhex(x), "little"))] = \
                            int.from_bytes(bytes.fromhex(y), "little")
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

    def ds(self, off: int, eob_file: bool, twin: str | None = None) -> int | None:
        if not eob_file or self.program == "EOB.EXE":
            return off
        exact = self.values.get(twin, {})               # the twin proof's own value for this unit, when the
        for kind in ("disp", "imm"):                    # unit's code uses the offset as it is (a nearby
            if (kind, off) in exact:                    # pair can belong to the next segment's data)
                return exact[(kind, off)]
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
    # a data-only module (src/D1AAD.ASM: the data map's d1aad, re/datamap.toml): the original's empty code
    # segment at its address is its own, so it goes before the unit that starts there, in place of the
    # empty module empty_segments would make
    man = ROOT / "build" / "re" / ("%s.json" % program.split(".")[0].lower())
    segs = json.loads(man.read_text()).get("segments", []) if man.exists() else []
    units = own_units(program)
    for p in sorted((ROOT / "src").glob("D*.*")):
        m = re.fullmatch(r"D([0-9A-F]{4})", p.stem.upper())
        sg = next((x for x in segs if m and x["kind_name"] == "code" and not x["size"] and x["para"] == int(m.group(1), 16)), None)
        if p.suffix.upper() not in (".C", ".ASM") or not sg:
            continue
        k = next((k for k, u in enumerate(units) if ":" in u["start"] and not u["start"].startswith("ovl")
                  and lin(u["start"]) >= sg["start"]), len(units))
        u = {"id": p.stem.lower(), "program": program, "kind": "data", "bytes": 0,
             "start": "%04X:%04X" % (sg["para"], sg["start"] - sg["para"] * 16)}
        out.append(((k, -1), p, program == "EOB.EXE", u))
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
        self.text = src.read_text() if self.asm else cmatch.expand(src.read_text())   # src/EOB.H inlined
        self.funcs, self.data, self.defined, self.stubs = cmatch.parse_annotations(self.text, self.asm)
        self.fardata = set(cmatch.FARDATA)              # externs at seg:off that are data (far objects)
        # /*@data */: the file defines its unit's whole _DATA (playbook 80, step D); it is linked as it
        # is, at its span of re/datamap.toml, and no filler holds that span
        self.whole_data = bool(cmatch.OWN)
        self.defdata = dict(cmatch.DEFDATA)
        # a C global is defined by a top-level line without extern (EOV5.C has both for gTable7B89)
        self.data_defined = set(DATA_DEF.findall(self.text)) if self.asm else {
            n for n in self.data if re.search(r"^(?!extern\b)[A-Za-z_][^;\n(]*\b%s\b\s*(\[[^\]]*\])?\s*[=;]" % re.escape(n), self.text, re.M)}
        self.data_defined |= set(self.defdata)


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
            d = space.ds(c.data[n], c.eob, c.unit.get("twin"))
            if d is not None:
                dsym[d] = n if c.asm else "_" + n
    unresolved, far, c_alias = [], {}, {}
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
            d = space.ds(off, c.eob, c.unit.get("twin"))
            if d is None:
                if n not in c.data_defined:
                    unresolved.append("%s %s ds:%04X" % (c.src.name, n, off))
                continue
            if d in dsym and n not in c.data_defined:
                want = dsym[d] if c.asm else dsym[d].lstrip("_")
                if n != want:
                    text, _ = rename.replace_word(text, n, want, c.asm)
                if not c.asm and not dsym[d].startswith("_"):     # assembly data that C names with
                    c_alias.setdefault(d, set()).add("_" + want)  # its underscore (below)
        c.text_out = text
    names_lit: dict[int, list[str]] = {}
    # a far object an assembly module defines (gMovedSeg, a word in I0923.ASM's code): exported to C
    c_far = {n[1:] for c in copies if not c.asm for n in (("_" + m) for m in c.fardata)}
    for name in sorted(c_far & set(asm_labels)):
        d = asm_labels[name]
        d.text_out, _ = rename.replace_word(d.text_out, name, "_" + name, True)
        d.text_out = re.sub(r"(^\s*\.CODE\b[^\n]*\n)", r"\1        PUBLIC  _%s\n" % name, d.text_out, count=1, flags=re.M | re.I)
    # a segment by a symbol in it: a function of this program, else a far object (alias_far may rename it)
    seg_names: dict[int, tuple[str, str]] = {}
    for (sg, off), syms in sorted(far.items(), key=str):
        if not isinstance(sg, str):
            seg_names.setdefault(sg, (sorted(syms)[0], "BYTE"))
    for a, n in sorted(fsym.items()):
        if ":" in a and not a.startswith("ovl"):
            seg_names[int(a.split(":")[0], 16)] = ("_" + n if (a in c_uses or n == "main") else n, "FAR")
    space.seg_names = seg_names
    # a far pointer to an overlay function: (stub segment, entry offset) -> the function
    space.stub_names = {(para, e): ("_" + fsym[k] if k in c_uses else fsym[k])
                        for para, (ov, ents) in space.stubs.items() for e, co in ents.items()
                        for k in ["ovl%d:%04X" % (ov, co)] if k in fsym}
    # an overlay function the original's stub table has no entry for was not public: TLINK gives every
    # public of an overlay module one (EOB.EXE's overlay 0 has 30 entries for 35 functions, overlay 1 6
    # for 16). The copy makes it static, which Turbo C compiles to the same code; src/ keeps it public,
    # as the matcher finds each function by its public
    entries = {}
    for para, (ov, ents) in space.stubs.items():
        entries.setdefault(ov, set()).update(ents.values())
    for c in copies:
        m = re.match(r"ovl(\d+):", c.unit["start"])
        if c.asm or not m or int(m.group(1)) not in entries:
            continue
        ov = int(m.group(1))
        for n in sorted(c.defined - c.stubs):
            seg, off = c.funcs[n]
            if isinstance(seg, str) and off not in entries[ov]:
                c.text_out, k = re.subn(r"^(/\*@\s*ovl%d:%04X\s*\*/\s*)(?!static\b)" % (ov, off), r"\1static ",
                                        c.text_out, count=1, flags=re.M | re.I)
                if k:
                    c.statics = getattr(c, "statics", set()) | {n}
    for c in copies:
        if not c.whole_data:                        # (a whole-data module keeps its literals, in place)
            relabel_literals(c, space, w, names_lit)
            for d in define_between_templates(c, space):
                dsym[d] = "_" + next((n for n, o in c.data.items() if o == d), "__d%04X" % d)
        for o, lab in getattr(c, "labels", {}).items():
            if "_" + lab not in names_lit.setdefault(o, []):
                names_lit[o].append("_" + lab)
        if c.asm:
            src_prog = "EOB.EXE" if c.eob else program
            img = space.image if src_prog == program else read_mz((ROOT / "eob" / src_prog).read_bytes())["image"]
            own_data_segment(c, space, img, cmatch.dgroup_of(ROOT / "eob" / src_prog))
    for c in copies:
        # a large-model assembly unit in segment 0000 (INTRO.EXE's AdLib driver) is part of _TEXT, with
        # the start-up code and the library; TASM's .CODE would name its segment after the file
        large = c.asm and PROGRAMS[program]["model"] == "L"
        if c.asm and c.eob and program != "EOB.EXE":
            relabel_numbers(c, space.values.get(c.unit.get("twin"), {}))
        if c.asm and c.eob:
            relabel_segments(c, space, seg_names)
        elif not c.asm:
            relabel_far_constants(c, space, seg_names)
            if c.unit["start"].startswith("ovl"):
                relabel_overlay_segments(c, space, seg_names)
        if c.asm:                                   # DGROUP's paragraph, which src/ spells as a number
            para = cmatch.dgroup_of(ROOT / "eob" / ("EOB.EXE" if c.eob else program))   # (212Eh): the group
            c.text_out = re.sub(r"^(\s*mov\s+\w+\s*,\s*)0*%Xh\b" % para, r"\g<1>DGROUP", c.text_out, flags=re.M | re.I)
        if large and c.unit["start"].startswith("0000:") and not re.search(r"DGROUP|@data|\.DATA", c.text_out, re.I):
            # without .MODEL, whose own empty code segment the original does not have (TLINK's segment
            # table would list it)
            c.text_out = re.sub(r"^\s*\.MODEL\b[^\n]*$", "", c.text_out, count=1, flags=re.M | re.I)
            c.text_out = re.sub(r"^\s*\.CODE\s*$", "_TEXT SEGMENT WORD PUBLIC 'CODE'\n        ASSUME  cs:_TEXT",
                                c.text_out, count=1, flags=re.M | re.I)
            c.text_out = re.sub(r"@code\b", "_TEXT", c.text_out, flags=re.I)
            c.text_out = re.sub(r"^(\s*END\b[^\n]*)\s*\Z", "_TEXT ENDS\n\\1\n", c.text_out, flags=re.M | re.I)
        elif large and c.unit["start"].startswith("0000:"):
            c.text_out = re.sub(r"^(\s*\.CODE)\s*$", r"\1 _TEXT", c.text_out, flags=re.M | re.I)
        elif large and not c.unit["start"].startswith("ovl") and re.search(r"^\s*_TEXT\s+SEGMENT\b", c.text_out, re.M | re.I):
            # an explicit _TEXT in a unit with a segment of its own (E1BB1.ASM): TLINK would put it in
            # the start-up code's segment; its own is <STEM>_TEXT, as .CODE names it
            c.text_out = re.sub(r"\b_TEXT\b", "%s_TEXT" % c.path.stem.upper(), c.text_out)
            # its EXTRNs of far routines go before it: TASM would take one declared inside the segment
            # as in it, and TLINK overflows on the offset (E1BB1.ASM's calls to E19BC.C)
            seg = re.search(r"^[ \t]*%s_TEXT\s+SEGMENT\b[^\n]*\n" % c.path.stem.upper(), c.text_out, re.M | re.I)
            ext = re.findall(r"^[ \t]*EXTRN\s+\w+\s*:\s*FAR\b[^\n]*\n", c.text_out[seg.end():], re.M | re.I)
            body = re.sub(r"^[ \t]*EXTRN\s+\w+\s*:\s*FAR\b[^\n]*\n", "", c.text_out[seg.end():], flags=re.M | re.I)
            c.text_out = c.text_out[:seg.start()] + "".join(ext) + seg.group(0) + body
        # where the original's segment begins: TLINK's segment table records it (the low four bits of
        # its start, after alignment), and the map can count an alignment pad as a unit's first byte
        # (src/ then has it as `db 0`: E1BBE.ASM, I0CDA.ASM). The pad goes from the copy, and .CODE's
        # word alignment makes it; a segment that begins at an odd address (E1948.ASM at 084A:0005 in
        # INTRO.EXE) was byte aligned, so it is declared BYTE before .MODEL (TASM 2.0 then keeps it).
        if large and not c.unit["start"].startswith("0000:") and not c.unit["start"].startswith("ovl"):
            u = lin(c.unit["start"])
            sg = next((x for x in space.segments if x["kind_name"] == "code" and x["size"] and x["start"] <= u < x["start"] + x["size"]), None)
            begin = sg["para"] * 16 + sg["flags"] if sg else u
            if begin > u:
                c.text_out, dropped = drop_pad(c.text_out, begin - u)
                if not dropped:
                    print("rebuild: %s: the original's segment begins at %05X, after %d bytes of the unit; no pad found" % (c.path.name, begin, begin - u))
            elif u % 2:
                seg = "%s_TEXT" % c.path.stem.upper()
                c.text_out = re.sub(r"^(\s*\.MODEL\b)", "%s SEGMENT BYTE PUBLIC 'CODE'\n%s ENDS\n\\1" % (seg, seg),
                                    c.text_out, count=1, flags=re.M | re.I)
        c.path.write_text(c.text_out.replace("\r\n", "\n").replace("\n", "\r\n"))
    if unresolved:
        print("rebuild: globals of twin files not placed in %s: %s" % (program, "; ".join(unresolved[:8])))
    # every global's symbols, by this program's DGROUP offset (generated labels for the undefined ones)
    names: dict[int, list[str]] = {}
    for c in copies:
        _, data, _, _ = cmatch.parse_annotations(c.text_out, c.asm)
        for n, off in data.items():
            d = space.ds(off, c.eob, c.unit.get("twin"))
            if d is None or d in dsym:
                continue
            sym = n if c.asm else "_" + n
            if sym not in names.setdefault(d, []):
                names[d].append(sym)
    # an assembly label that C code names with its underscore (E1948.ASM's gTandyOn, C's _gTandyOn,
    # one address): the copy gives the C name a label of its own at the definition. Only at that
    # address: E17A6.ASM's gSpriteHeight is a word in its code, E1BBE.ASM's _gSpriteHeight ds:8182
    for d, syms in c_alias.items():
        for s in syms:
            if s not in names.setdefault(d, []):
                names[d].append(s)
    for c in copies:
        for lab in sorted(c.data_defined) if c.asm else ():
            if lab not in c.data:                   # a label of the code (E17A6.ASM's)
                continue
            at = space.ds(c.data[lab], c.eob, c.unit.get("twin"))
            # (a twin's label whose INTRO.EXE address no proof gives, gTandyOn there: by its name)
            hit = [d for d, syms in names.items() if "_" + lab in syms and at in (d, None)]
            if not hit or "_" + lab in c.data_defined:
                continue
            c.text_out = re.sub(r"^(%s\s+(db|dw|dd)\b)" % re.escape(lab), r"_%s label byte\n\1" % lab, c.text_out, count=1, flags=re.M | re.I)
            c.text_out = re.sub(r"^(\s*\.MODEL\b[^\n]*\n)", r"\g<1>        PUBLIC  _%s\n" % lab, c.text_out, count=1, flags=re.M | re.I)
            c.path.write_text(c.text_out.replace("\r\n", "\n").replace("\n", "\r\n"))
            for d in hit:
                names[d].remove("_" + lab)
                if not names[d]:
                    del names[d]
    for o2, syms in names_lit.items():
        for sy in syms:
            if sy not in names.setdefault(o2, []):
                names[o2].append(sy)
    return copies, names, far


def alias_far(program: str, w: Path, exe: Path, copies: list, far: dict) -> dict:
    """Name the far objects that lie in a library's segment by that library's public; the rest stay."""
    if not far:
        return far
    import fbov
    orig = json.loads((ROOT / "build/re" / ("%s.json" % program.split(".")[0].lower())).read_text())["segments"]
    ours = fbov.manifest(exe.read_bytes())[0]["segments"]
    text = map_text(w)
    text = text[text.index("Publics by Value"):] if "Publics by Value" in text else ""
    pubs = sorted((int(m.group(1), 16) * 16 + int(m.group(2), 16), m.group(3))
                  for m in re.finditer(r"^\s*([0-9A-F]{4}):([0-9A-F]{4})\s+(?:idle\s+)?(\S+)\s*$", text, re.M))
    def data_before_dgroup(tab):                    # the non-empty data segments before DGROUP, in order
        dg = max(x["para"] for x in tab if x["kind_name"] == "data" and x["size"] > 1024)
        return [x for x in tab if x["kind_name"] == "data" and x["size"] and x["para"] < dg]
    od, wd = data_before_dgroup(orig), data_before_dgroup(ours)
    rest = {}
    for (seg, off), syms in far.items():
        k = next((i for i, x in enumerate(od) if not isinstance(seg, str) and x["para"] == seg), None)
        if k is None or k >= len(wd):
            rest[(seg, off)] = syms
            continue
        lo = wd[k]["para"] * 16
        hit = [(a, n) for a, n in pubs if lo <= a <= lo + off and a < lo + wd[k]["size"]]
        if not hit:
            rest[(seg, off)] = syms
            continue
        a, pub = max(hit)
        delta = lo + off - a
        for c in copies:
            for sym in syms:
                name = sym[1:] if not c.asm and sym.startswith("_") else sym
                if c.asm and delta == 0 and re.search(r"\bSEG\s+%s\b" % re.escape(sym), c.text_out):
                    c.text_out = re.sub(r"\b%s\b" % re.escape(sym), pub, c.text_out)   # relabel_segments'
                    c.path.write_text(c.text_out.replace("\r\n", "\n").replace("\n", "\r\n"))
                    continue
                if c.asm or not re.search(r"\b%s\b" % re.escape(name), c.text_out):
                    continue
                m = re.search(r"^extern\s+(.*?)\s+far\s+%s\s*(\[\s*\])?\s*;[^\n]*$" % re.escape(name), c.text_out, re.M)
                if not m:
                    continue
                cname = pub[1:] if pub.startswith("_") else pub
                typ, arr = m.group(1), bool(m.group(2))
                expr = ("((%s far *)((char far *)%s + %d))" if arr else "(*(%s far *)((char far *)%s + %d))") % (typ, cname, delta)
                c.text_out = c.text_out[:m.start()] + "extern char far %s[];\n#define %s %s" % (cname, name, expr) + c.text_out[m.end():]
                c.path.write_text(c.text_out.replace("\r\n", "\n").replace("\n", "\r\n"))
                print("rebuild: %s is the library's %s+%d (%s)" % (name, pub, delta, c.path.name))
    return rest


ASM_DEF = re.compile(r"^(\s*)(\w+)?(\s+)(db|dw|dd)(\s+)(.*?)(\s*;.*)?$", re.I)


HEXNUM = re.compile(r"\b(?:(0*[0-9][0-9A-Fa-f]*)h|([0-9]+))\b")


def relabel_numbers(c: "Copy", values: dict) -> None:
    """A twin assembly file spells some addresses as EOB.EXE's numbers (`mov ax,ds:[8496h]`): each
    number on a code line that the twin proof relabels (a displacement inside brackets, an immediate
    outside them) takes this program's value. Numbers the two programs share are not in the proof."""
    out = []
    for line in c.text_out.split("\n"):
        code, sep, comment = line.partition(";")
        if re.match(r"\s*(\w+\s+)?(db|dw|dd|EQU)\b", code, re.I) or not code.strip():
            out.append(line)
            continue
        def sub(m, code=code):
            inside = code[:m.start()].count("[") > code[:m.start()].count("]")
            if inside and re.search(r"\bbp\b", code[code.rfind("[", 0, m.start()):code.find("]", m.end()) + 1], re.I):
                return m.group(0)                   # a frame offset ([bp+6]), never an address
            kind = ("cs" if re.search(r"\bcs:", code, re.I) else "disp") if inside else "imm"
            if m.group(2) and not inside:           # a decimal outside brackets is a count, not an address
                return m.group(0)
            y = values.get((kind, int(m.group(1), 16) if m.group(1) else int(m.group(2))))
            return m.group(0) if y is None else "0%Xh" % y
        out.append(HEXNUM.sub(sub, code) + sep + comment)
    c.text_out = "\n".join(out)


def drop_pad(text: str, n: int) -> tuple[str, bool]:
    """Take the n pad bytes off the first statement after .CODE when it is zeros (`db 0`, or E1D9A.ASM's
    `dw 0,0,0,0`, a pad and seven bytes of scratch); labels before it stay where they are."""
    m = re.search(r"^\s*(\.CODE\b|\w+_TEXT\s+SEGMENT\b)[^\n]*\n", text, re.M | re.I)
    if not m:
        return text, False
    pos = m.end()
    for line in re.finditer(r"[^\n]*\n", text[pos:]):
        code = line.group(0).split(";")[0].strip()
        if not code or re.match(r"(PUBLIC|EXTRN|ASSUME)\b|\w+\s+LABEL\b", code, re.I):
            continue
        z = re.fullmatch(r"(db|dw)\s+(0+h?(?:\s*,\s*0+h?)*)", code, re.I)
        if not z:
            return text, False
        total = (z.group(2).count(",") + 1) * (1 if z.group(1).lower() == "db" else 2)
        if total < n:
            return text, False
        at = pos + line.start()
        new = "        db      %d dup (0)\n" % (total - n) if total > n else ""
        return text[:at] + new + text[at + len(line.group(0)):], True
    return text, False


def relabel_segments(c: "Copy", space: "Space", seg_names: dict) -> None:
    """An EOB.EXE segment that a twin assembly file loads as a number (`mov bx,1A16h`, a word the
    original relocates) becomes SEG of a symbol in this program's segment, so TLINK relocates it."""
    need = set()
    own = space.program == "EOB.EXE"                # EOB.EXE's own files: its segments as they are
    def sub(m):
        x = int(m.group(2), 16)
        y = x if own else space.seg.get(x)
        if y is None or y not in seg_names:
            return m.group(0)
        need.add(seg_names[y])
        return "%sSEG %s" % (m.group(1), seg_names[y][0])
    out = []
    for line in c.text_out.split("\n"):
        code, sep, comment = line.partition(";")
        out.append(re.sub(r"^(\s*(?:\w+:\s*)?mov\s+\w+\s*,\s*)0*([0-9][0-9A-Fa-f]*)h\b", sub, code, flags=re.I) + sep + comment)
    c.text_out = "\n".join(out)
    for name, kind in sorted(need):
        if not re.search(r"^\s*(EXTRN|PUBLIC)\b[^;\n]*\b%s\b|^%s\s+(PROC|LABEL)\b" % (re.escape(name), re.escape(name)), c.text_out, re.M | re.I):
            c.text_out = re.sub(r"^(\s*\.MODEL\b[^\n]*\n)", r"\g<1>        EXTRN   %s:%s\n" % (name, kind), c.text_out, count=1, flags=re.M | re.I)


def relabel_library_segments(w: Path, copies: list, space: "Space") -> int:
    """A segment of the library's that an own assembly file loads as a number (E1A72.ASM's `mov
    ax,20B5h`, the overlay manager's _OVRDATA_, where it keeps the EXE's file handle): after the probe
    link, which puts the code and far segments where the original has them, SEG of a public there."""
    text = map_text(w)
    text = text[text.index("Publics by Value"):] if "Publics by Value" in text else ""
    lib = {}
    for m in re.finditer(r"^\s*([0-9A-F]{4}):([0-9A-F]{4})\s+(?:idle\s+)?(\S+)\s*$", text, re.M):
        para = int(m.group(1), 16)
        if para not in space.seg_names and para != space.dgroup:
            lib.setdefault(para, m.group(3))
    n = 0
    for c in copies:
        if not c.asm or c.unit.get("twin"):
            continue
        need = set()
        def sub(m):
            x = int(m.group(2), 16)
            if x not in lib:
                return m.group(0)
            need.add(lib[x])
            return "%sSEG %s" % (m.group(1), lib[x])
        out = []
        for line in c.text_out.split("\n"):
            code, sep, comment = line.partition(";")
            out.append(re.sub(r"^(\s*(?:\w+:\s*)?mov\s+\w+\s*,\s*)0*([0-9][0-9A-Fa-f]{3})h\b", sub, code, flags=re.I) + sep + comment)
        if not need:
            continue
        c.text_out = "\n".join(out)
        for name in sorted(need):
            c.text_out = re.sub(r"^(\s*\.MODEL\b[^\n]*\n)", r"\g<1>        EXTRN   %s:BYTE\n" % name, c.text_out, count=1, flags=re.M | re.I)
        c.path.write_text(c.text_out.replace("\r\n", "\n").replace("\n", "\r\n"))
        print("rebuild: %s loads the library's segment of %s" % (c.path.name, ", ".join(sorted(need))))
        n += 1
    return n


def relabel_overlay_segments(c: "Copy", space: "Space", seg_names: dict) -> None:
    """An overlay's segment numbers are TLINK's segment-table indexes times 8 (its fixups are relocated
    by the table, not by paragraph): EOV3.C's `MK_FP(0x1d8, 0)` is entry 59, the far object gMapSheet
    at 1F1E:0000, which the copy names, so Turbo C writes the same code with the fixup. (EOV5.C's
    `gScrollHook = 0xac; g8119 = 0x2d0;` is entry 90, overlay 5's stub: a far pointer to its own
    MessagePanel through entry 0ACh, which Turbo C writes only for a function of another module; for
    one of its own it writes CS and the function's offset. It stays as src/ has it: the same bytes,
    without the three fixups.)"""
    segs = space.segments
    def entry(v):
        return segs[v // 8] if v % 8 == 0 and v // 8 < len(segs) else None
    names = set()
    def mkfp(m):
        sg = entry(int(m.group(1), 16))
        if not sg or sg["para"] not in seg_names or seg_names[sg["para"]][1] != "BYTE":
            return m.group(0)
        sym = seg_names[sg["para"]][0].lstrip("_")
        names.add(sym)
        off = int(m.group(2), 0)
        return "((void far *)%s)" % sym if not off else "((void far *)((char far *)%s + %d))" % (sym, off)
    text = re.sub(r"\bMK_FP\(\s*0x([0-9A-Fa-f]+)\s*,\s*(0x[0-9A-Fa-f]+|\d+)\s*\)", mkfp, c.text_out)
    if names:
        decl = "".join("extern char far %s[];\n" % n for n in sorted(names)
                       if not re.search(r"^extern[^;\n]*\b%s\b" % re.escape(n), text, re.M))
        last = max((m.end() for m in re.finditer(r"^#include[^\n]*\n", text, re.M)), default=0)
        text = text[:last] + decl + text[last:]
    c.text_out = text


def relabel_far_constants(c: "Copy", space: "Space", seg_names: dict) -> None:
    """A far pointer src/ writes as a number (E0BD2.C's `(char far *)0x1F280000L`, the item-name table
    gItemNames at 1F28:0000, a word the original relocates): in the copy, the far object there, so
    TLINK relocates it. Only a segment of a far object (not of code) is taken."""
    names = {}
    def sub(m):
        seg, off = int(m.group(2), 16), int(m.group(3), 16)
        if seg not in seg_names or seg_names[seg][1] != "BYTE":
            return m.group(0)
        sym = seg_names[seg][0].lstrip("_")
        names[sym] = True
        return "(%s far *)((char far *)%s + 0x%X)" % (m.group(1).strip(), sym, off) if off else \
               "(%s far *)%s" % (m.group(1).strip(), sym)
    text = re.sub(r"\(\s*([A-Za-z_][\w\s]*?)\s+far\s*\*\s*\)\s*0x([0-9A-Fa-f]{4})([0-9A-Fa-f]{4})L\b", sub, c.text_out)
    if names:
        decl = "".join("extern char far %s[];\n" % n for n in sorted(names)
                       if not re.search(r"^extern[^;\n]*\b%s\b" % re.escape(n), text, re.M))
        last = max((m.end() for m in re.finditer(r"^#include[^\n]*\n", text, re.M)), default=0)
        c.text_out = text[:last] + decl + text[last:]


def own_data_segment(c: "Copy", space: "Space", image: bytes, odg: int) -> None:
    """An assembly module's own data segment (E1948.ASM's SND: the PC speaker driver's tables, right
    after every module's _DATA, at ds:88A6 in EOB.EXE and ds:26DA in INTRO.EXE): byte aligned, class
    DATA, so TLINK puts it there; the padding src/ keeps before its first variable (to give match the
    original's offsets from the paragraph) goes, and each `?` takes the original's bytes."""
    m = re.search(r"^(\w+)\s+SEGMENT\s+PARA\s+PUBLIC\s+'(\w+)'(.*?)^\1\s+ENDS", c.text_out, re.M | re.S | re.I)
    if not m or m.group(1).upper().endswith("_TEXT") or m.group(2).upper() in ("CODE", "DATA", "BSS"):
        return
    body, out, started, pos, first = m.group(3), [], False, None, None
    for line in body.split("\n"):
        mm = ASM_DEF.match(line)
        ann = re.search(r"@\s*ds:([0-9A-Fa-f]{1,4})", line)
        if mm and ann and not started:
            started, pos = True, int(ann.group(1), 16)
            first = pos   # in the file's own program: a twin's values are
                                                          # its EOB.EXE twin's (the same in both programs)
        if not started:
            continue
        if mm and pos is not None:
            kind, operand = mm.group(4).lower(), mm.group(6)
            size = {"db": 1, "dw": 2, "dd": 4}[kind]
            dm = re.match(r"(\d+)\s+dup\s*\(\s*\?\s*\)", operand, re.I)
            n = int(dm.group(1)) * size if dm else (size if operand.strip() == "?" else None)
            if n is None:
                out.append(line)
                pos = None                          # an initialised value we cannot size: stop filling
                continue
            data = image[odg * 16 + pos:odg * 16 + pos + n]
            vals = ["0%0*Xh" % (2 * size, int.from_bytes(data[k:k + size], "little")) for k in range(0, n, size)]
            for k in range(0, len(vals), 16):       # the type stays (the code's operands need it); short lines
                out.append("%s%s%s%s%s%s%s" % (mm.group(1), (mm.group(2) or "") if k == 0 else "",
                                               mm.group(3) if k == 0 else " " * (len(mm.group(2) or "") + len(mm.group(3))),
                                               kind, mm.group(5), ",".join(vals[k:k + 16]),
                                               (mm.group(7) or "")[:60] if k == 0 else ""))
            pos += n
        else:
            out.append(line)
    new = "%s SEGMENT BYTE PUBLIC 'DATA'\n%s\n%s ENDS" % (m.group(1), "\n".join(out), m.group(1))
    c.text_out = c.text_out[:m.start()] + new + c.text_out[m.end():]
    if first is not None:                           # the frame src/ loads as a number (29B8h, the
        frame = odg + first // 16                   # paragraph TLINK gave the segment) is the segment
        c.text_out = re.sub(r"^(\s*mov\s+\w+\s*,\s*)0*%Xh\b" % frame, r"\g<1>%s" % m.group(1),
                            c.text_out, flags=re.M | re.I)


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


C_LIT = re.compile(r'/\*.*?\*/|//[^\n]*|\'(?:[^\'\\\n]|\\.)*\'|"(?:[^"\\\n]|\\.)*"(?:\s*"(?:[^"\\\n]|\\.)*")*', re.S)
ESC = {"n": 10, "r": 13, "t": 9, "a": 7, "b": 8, "f": 12, "v": 11, "\\": 92, '"': 34, "'": 39, "?": 63}


def c_string_bytes(lit: str) -> bytes:
    """The bytes of a C string literal (adjacent ones joined), escapes decoded, without the NUL."""
    out = bytearray()
    for part in re.findall(r'"((?:[^"\\\n]|\\.)*)"', lit):
        k = 0
        while k < len(part):
            ch = part[k]
            if ch != "\\":
                out.append(ord(ch) & 0xFF)
                k += 1
                continue
            nxt = part[k + 1]
            if nxt == "x":
                m = re.match(r"[0-9A-Fa-f]{1,2}", part[k + 2:])
                out.append(int(m.group(0), 16))
                k += 2 + len(m.group(0))
            elif nxt in "01234567":
                m = re.match(r"[0-7]{1,3}", part[k + 1:])
                out.append(int(m.group(0), 8) & 0xFF)
                k += 1 + len(m.group(0))
            else:
                out.append(ESC.get(nxt, ord(nxt)))
                k += 2
    return bytes(out)


def literals_at_original(c: "Copy", space: "Space", w: Path) -> dict[int, int] | None:
    """Where the original keeps each string literal of a C file: make match records it for every
    literal a function refers to (CMATCH_LITERALS), by its offset in our module's data."""
    out = w / ("%s.LIT" % c.path.stem)
    import os
    import subprocess
    r = subprocess.run([sys.executable, str(ROOT / "tools/match/cmatch.py"), str(c.src)], cwd=ROOT,
                       env=dict(os.environ, CMATCH_LITERALS=str(out)), capture_output=True, text=True)
    if not out.exists():
        return None
    got = {int(k): v for k, v in json.loads(out.read_text()).items()}
    return {k: space.ds(v, c.eob, c.unit.get("twin")) for k, v in got.items()}


def relabel_literals(c: "Copy", space: "Space", w: Path, names: dict) -> int:
    """Each string literal of a C copy becomes a reference to a label at the original's address of it
    (__s1A07), so the module brings no data of its own and the program's data comes whole from the
    original (the fillers); the code is the same, an offset in DGROUP either way."""
    c.lit_where, c.lit_gone = {}, {}
    if c.asm:
        return 0
    where = literals_at_original(c, space, w)
    if not where:
        return 0
    c.lit_where = where                             # (a template's too: an initialiser F_SCOPY@ copies)
    text = c.text_out
    lits = []                                       # (match, bytes) of each string literal, in order
    for m in C_LIT.finditer(text):
        tok = m.group(0)
        if not tok.startswith('"'):
            continue
        line_start = text.rfind("\n", 0, m.start()) + 1
        if text[line_start:m.start()].lstrip().startswith("#"):      # #include "x.h"
            continue
        lits.append((m, c_string_bytes(tok)))
    # the pool follows the module's other data (its initialised globals, then the templates F_SCOPY@
    # copies: E122E.C's 149 bytes of them): its start is the one that puts the most literals where
    # the matcher saw them
    cum, k = [], 0
    for _, b in lits:
        cum.append(k)
        k += len(b) + 1
    votes = Counter(key - x for key in where for x in cum if key >= x)
    base = max(votes.items(), key=lambda kv: (kv[1], -kv[0]))[0] if votes else 0
    # a literal no function refers to (the matcher saw none) is where the pool puts it, when the
    # original has its bytes there
    shift = Counter(where[base + x] - base - x for x in cum if where.get(base + x) is not None).most_common(1)
    data = space.image[space.dgroup * 16:]
    out, pos, n, used = [], 0, 0, set()
    for (m, b), x in zip(lits, cum):
        at = where.get(base + x)
        if at is None and shift:
            guess = base + x + shift[0][0]
            at = guess if data[guess:guess + len(b) + 1] == b + b"\0" else None
        if at is None:
            continue
        c.lit_gone[base + x] = len(b) + 1          # gone from the module's data
        label = "__s%04X" % at
        out += [text[pos:m.start()], label]
        pos = m.end()
        used.add(at)
        n += 1
    out.append(text[pos:])
    decl = "".join("extern char __s%04X[];\n" % a for a in sorted(used))
    body = "".join(out)
    last_inc = max((mm.end() for mm in re.finditer(r"^#include[^\n]*\n", body, re.M)), default=0)
    c.text_out = body[:last_inc] + decl + body[last_inc:]
    for a in used:
        if "___s%04X" % a not in names.setdefault(a, []):
            names[a].append("___s%04X" % a)
    return n


AUTO_INIT = re.compile(r"^[ \t]+[A-Za-z_][^;=(\n]*(?:\(\s*far\s*\*\s*\w+\s*\[[^\]]*\]\s*\)\s*\([^)]*\)\s*|\w+\s*(?:\[[^\]]*\]\s*)+)=\s*[{\x22]", re.M)


def define_between_templates(c: "Copy", space: "Space") -> list:
    """Globals the original module defined between two of its initialiser templates (E122E.C's g5A40,
    8 bytes between the tables at ds:5A18 and ds:5A48): src/ declares them extern, so the copy's data
    would close up. Each becomes a definition with the original's bytes, before the function whose
    template follows it (Turbo C writes a module's data in source order, its string pool last). The
    ones before the first item (but a byte that makes it odd) or after the last stay in the fillers. Their
    ds offsets."""
    where, gone = getattr(c, "lit_where", {}), getattr(c, "lit_gone", {})
    keys = sorted(k for k in set(where) - set(gone) if where[k] is not None)
    if not keys:
        return []
    # the module's data items in source order: the templates of initialised locals and the globals it
    # defines (EOV5.C's gTable7B89, between its templates); a gap goes before the item that follows it
    items = [(m.start(), True) for m in AUTO_INIT.finditer(c.text_out)]
    items += [(m.start(), False) for n in c.data_defined for m in [re.search(
        r"^(?!extern\b)[A-Za-z_][^;\n(]*\b%s\b\s*(\[[^\]]*\])?\s*=" % re.escape(n), c.text_out, re.M)] if m]
    items.sort()
    if len(items) != len(keys):
        print("rebuild: %s: %d data items, %d in the source; data between them not defined" % (c.path.name, len(keys), len(items)))
        return []
    data = space.image[space.dgroup * 16:]
    mine = {off: n for n, off in c.data.items()}
    done, text, retype = [], c.text_out, []
    c.lit_added = {}                                # src offset of the item after a gap -> bytes added
    c.labels = {}                                   # ds offset -> a label a definition points at
    dg = space.dgroup * 16
    relocs = {x - dg for x in getattr(space, "relocs", ())}
    # where the module's data begins: Turbo C writes a module's data fixups from the highest address
    # down, so the data relocations that follow its own in the original's table, lower still, are its
    # too (EOV5.C's gText7B23, the far pointers at ds:7B23 and ds:7B27 before its first template), and
    # with them everything up to its first item (gCellDef7B2B); then a byte when that is odd, as a C
    # module's data is word aligned (EOV1.C's first template is at ds:63CB, the byte before it its own)
    first = where[keys[0]]
    seq = [x - dg for x in getattr(space, "reloc_seq", ())]
    own = [i for i, r in enumerate(seq) if first <= r < where[keys[-1]]]
    start = first
    if own:
        j = max(own) + 1
        while j < len(seq) and 0 <= seq[j] < start and seq[j] < seq[j - 1]:
            start = max((o for o in mine if o <= seq[j]), default=seq[j] - 2)
            j += 1
    for i in range(len(keys) - 1, -1, -1):         # from the end, so earlier positions stay valid
        lo = where[keys[i - 1]] + keys[i] - keys[i - 1] if i else start & ~1
        hi = where[keys[i]]
        if hi <= lo:
            continue
        c.lit_added[keys[i]] = hi - lo
        cuts = sorted({lo} | {o for o in mine if lo < o < hi}) + [hi]
        defs = []
        for a, b in zip(cuts, cuts[1:]):
            n = mine.get(a, "__d%04X" % a)
            decl = re.search(r"^extern\s+([^;]*?)\b%s\s*(\[[^\]]*\])?\s*;" % re.escape(n), text, re.M)
            typ = decl.group(1).strip() if decl else "char"
            word = re.fullmatch(r"(unsigned\s+|signed\s+)?(int|short)|unsigned", typ)
            size = 2 if word else 1
            signed = word and "unsigned" not in typ or (not word and "signed" in typ and "unsigned" not in typ)
            vals = [int.from_bytes(data[k:k + size], "little", signed=bool(signed)) for k in range(a, b, size)]
            if any(k in relocs for k in range(a, b - 1)):
                # far pointers (EOV5.C's gText7BA6, menu tables of 212E:7D23 and on, which src/ declares
                # as char arrays and uses only by address): pointers to labels at their strings, so the
                # copy has the original's relocations; the copy's extern takes the type
                ptrs = [(int.from_bytes(data[k:k + 2], "little"), int.from_bytes(data[k + 2:k + 4], "little"))
                        for k in range(a, b, 4)]
                if (b - a) % 4 or any(k + 2 not in relocs for k in range(a, b, 4)) or any(s != space.dgroup for _, s in ptrs):
                    print("rebuild: %s: %s at ds:%04X holds relocated words that are not far pointers into DGROUP" % (c.path.name, n, a))
                else:
                    for o, _ in ptrs:
                        c.labels[o] = "__s%04X" % o
                    defs.append("".join("extern char __s%04X[];\n" % o for o, _ in sorted(set(ptrs))) +
                                "char far *%s[] = { %s };" % (n, ", ".join("(char far *)__s%04X" % o for o, _ in ptrs)))
                    if decl:
                        retype.append((decl.group(0), "extern char far *%s[];" % n))
                    done.append(a)
                    continue
            if decl and not decl.group(2):          # a scalar (EOV0.C's g5EB0)
                defs.append("%s %s = %d;" % (typ, n, vals[0]))
            else:
                defs.append("%s %s[] = { %s };" % (typ, n, ", ".join(str(v) for v in vals)))
            done.append(a)
        # before the next item: a global's line, or the head of the function that holds a template
        # (the lines above its `{` back to a comment, a declaration or a blank line)
        head, template = items[i]
        if template:
            brace = text.rindex("\n{", 0, head) + 1
            head = text.rindex("\n", 0, brace - 1) + 1
            while head > 0:
                prev = text[text.rindex("\n", 0, head - 1) + 1:head - 1]
                if not prev.strip() or prev.rstrip().endswith((";", "}", "*/")):
                    break
                head = text.rindex("\n", 0, head - 1) + 1
        text = text[:head] + "/* defined here in the original's data (make rebuild) */\n" + "\n".join(defs) + "\n" + text[head:]
    for old, new in retype:
        text = text.replace(old, new, 1)
    c.text_out = text
    return done


def filler(tag: str, image: bytes, base: int, lo: int, hi: int, names: dict, init: bool,
           relocs: set | None = None, seg_names: dict | None = None, dgroup: int | None = None,
           order: list | None = None, stub_names: dict | None = None) -> str:
    """A module of the original's DGROUP bytes [lo, hi) (or reserved, for _BSS), a public at each name.
    A word the original relocates (the segment of a far pointer) is a fixup: DGROUP, or SEG of a
    function in that code segment; a far pointer to an overlay function (its stub segment and entry,
    EOB.EXE ds:3BBE) is `dd` of the function, whose entry TLINK writes."""
    seg = "_DATA" if init else "_BSS"
    pub = sorted(o for o in names if lo <= o < hi)
    fix = {}                                        # start -> (bytes, statement, the relocated word)
    ext = set()
    for k in range(lo, hi - 1) if init and relocs else ():
        if base + k in relocs:
            v = int.from_bytes(image[base + k:base + k + 2], "little")
            e = int.from_bytes(image[base + k - 2:base + k], "little")
            if v == dgroup:
                fix[k] = (2, "dw DGROUP", k)
            elif seg_names and v in seg_names:
                fix[k] = (2, "dw SEG %s" % seg_names[v][0], k)
                ext.add(seg_names[v])
            elif stub_names and (v, e) in stub_names and k - 2 >= lo:
                fix[k - 2] = (4, "dd %s" % stub_names[(v, e)], k)
                ext.add((stub_names[(v, e)], "FAR"))
            else:
                print("rebuild: %s: the relocated word at ds:%04X (%04X) is no segment we know" % (tag, k, v))
    lines = ["DGROUP GROUP _DATA, _BSS"] + ["PUBLIC %s" % s for o in pub for s in names[o]]
    lines += ["EXTRN %s:%s" % (n, kind) for n, kind in sorted(ext)]
    other = "_BSS SEGMENT BYTE PUBLIC 'BSS'\r\n_BSS ENDS" if init else "_DATA SEGMENT BYTE PUBLIC 'DATA'\r\n_DATA ENDS"
    lines.append(other)
    lines.append("%s SEGMENT BYTE PUBLIC '%s'" % (seg, "DATA" if init else "BSS"))
    pos = lo
    for o in pub + [hi]:
        if o > pos:
            if init:
                at = pos
                for f in sorted(x for x in fix if pos <= x < o) + [o]:
                    chunk = image[base + at:base + f]
                    lines += ["db " + ",".join("%03Xh" % b for b in chunk[k:k + 16]) for k in range(0, len(chunk), 16)]
                    if f < o:                       # a fixup now, or (in the original's order) at the end
                        lines.append("ORG %05Xh" % (f + fix[f][0] - lo) if order else fix[f][1])
                        at = f + fix[f][0]
            else:
                lines.append("db %d dup (?)" % (o - pos))
        if o < hi:
            lines += ["%s label byte" % s for s in names[o]]
        pos = max(pos, o)
    if order:                                       # the fixups in the original's order (TLINK writes
        start = {r: f for f, (_, _, r) in fix.items()}   # the relocation table in fixup order)
        for r in order:
            if r in start:
                lines += ["ORG %05Xh" % (start[r] - lo), fix[start[r]][1]]
        lines.append("ORG %05Xh" % (hi - lo))
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


def empty_segments(w: Path, space: "Space", copies: list) -> dict[int, list]:
    """The original's segment table lists three empty code segments in INTRO.EXE (0892:0000, 0A19:0000,
    0AED:000D), each between two of our modules: a module of the original with no code of its own (a
    data-only module, its data now in a filler [H]). Each becomes a module with one empty segment, linked
    after the module that ends where it is."""
    out: dict[int, list] = {}
    for k, sg in enumerate(x for x in space.segments if x["kind_name"] == "code" and x["size"] == 0):
        at = sg["start"]
        if any(c.unit.get("kind") == "data" and lin(c.unit["start"]) == at for c in copies):
            continue                                # a data-only module of src/ brings it
        i = next((i for i, c in enumerate(copies) if ":" in c.unit["start"] and not c.unit["start"].startswith("ovl")
                  and lin(c.unit["start"]) + c.unit["bytes"] == at), None)
        if i is None:                               # the overlay stubs' segment (_1STUB_) is TLINK's
            if at < max((lin(c.unit["start"]) + c.unit["bytes"] for c in copies if ":" in c.unit["start"]
                         and not c.unit["start"].startswith("ovl")), default=0):
                print("rebuild: the original's empty code segment at %04X:%04X follows none of our modules" % (sg["para"], at - sg["para"] * 16))
            continue
        name = "EMPTY%d" % k
        assemble(w, name, "%s_TEXT SEGMENT BYTE PUBLIC 'CODE'\r\n%s_TEXT ENDS\r\nEND\r\n" % (name, name))
        out.setdefault(i, []).append(name)
    return out


def data_owners(oseq: list, copies: list, odg: int, lo: int, hi: int, idx: int, verbose: bool) -> list:
    """The modules that own the data [lo, hi): TLINK writes the relocation table module by module, so
    each run of the original's data relocations follows the code relocations of the module whose data
    it is (I0892.C's), or of the module before a data-only one (the empty code segment after E1948.ASM).
    Each piece goes after that module, its fixups in the original's order. [(copy index, lo, hi, order)]"""
    owner = {}
    for i, c in enumerate(copies):
        if ":" in c.unit["start"] and not c.unit["start"].startswith("ovl"):
            u = lin(c.unit["start"])
            owner[(u, u + c.unit["bytes"])] = i
    def who(x):
        return next((i for (p, q), i in owner.items() if p <= x < q), None)
    dg = odg * 16
    runs, last = [], -1                              # (owner, [ds offsets in order])
    for x in oseq:
        if x >= dg:
            d = x - dg
            if lo <= d < hi:
                if runs and runs[-1][0] == last:
                    runs[-1][1].append(d)
                else:
                    runs.append((last, [d]))
            continue
        i = who(x)
        if i is not None:
            last = i
        elif x < min((p for p, q in owner), default=0):
            last = -1                                # the start-up code
    if not runs:
        return [(idx, lo, hi, None)]
    runs.sort(key=lambda r: min(r[1]))
    if any(r1[0] > r2[0] for r1, r2 in zip(runs, runs[1:])) or runs[0][0] < idx:
        print("rebuild: the data relocations of ds:%04X-%04X do not follow the link order; one filler" % (lo, hi))
        return [(idx, lo, hi, [x - dg for x in oseq if lo <= x - dg < hi and x >= dg])]   # in the original's order
    merged = []
    for o, ds in runs:
        if merged and merged[-1][0] == o:
            merged[-1][1].extend(ds)
        else:
            merged.append((o, list(ds)))
    # a cut goes before the far pointer whose segment the first relocation is, at an even offset: the
    # C module between two pieces has a word-aligned _DATA (EOB.EXE's ds:0201, the segment of 212E:1EC9)
    def cut(ds):
        return max(lo, (min(ds) - 2) & ~1)
    cuts = [lo] + [cut(ds) for _, ds in merged[1:]] + [hi]
    out = []
    if merged[0][0] != idx and cut(merged[0][1]) > lo:   # data before the first relocation stays
        out.append((idx, lo, cut(merged[0][1]), None))
        cuts[0] = cut(merged[0][1])
    for k, (o, ds) in enumerate(merged):
        out.append((o, cuts[k], cuts[k + 1], ds))
        if verbose:
            print("rebuild: data ds:%04X-%04X after %s, %d relocations" % (cuts[k], cuts[k + 1], copies[o].path.stem if o >= 0 else "C0", len(ds)))
    return out


def data_spans(program: str) -> dict[str, int]:
    """Where each module's data begins in the original (re/datamap.toml, EOB.EXE only), by the stem of
    its file: a module whose file defines its whole data (/*@data */) is placed there."""
    p = ROOT / "re" / "datamap.toml"
    if program != "EOB.EXE" or not p.exists():
        return {}
    return {m["id"].upper(): int(m["start"][3:], 16) for m in tomllib.loads(p.read_text())["module"]}


def by_template(c: "Copy", pdata: bytes, odata: bytes, o: int, l: int) -> int | None:
    """Where a module's data is in the original, by the original's address of the initialiser templates
    in it (EOV0.C's `char taken[4] = {0, 0, 0, 0}`, four zeros that no bytes place; E122E.C's tables of
    far pointers, whose words the link changes): the matcher compared each one and records it with the
    literals, by its offset in the module's data before the literals went. Every template agrees."""
    ats = set()
    for k in sorted(set(getattr(c, "lit_where", {})) - set(getattr(c, "lit_gone", {}))):
        if c.lit_where[k] is None:
            return None
        ats.add(c.lit_where[k] - (k - sum(n for g, n in c.lit_gone.items() if g < k)
                                  + sum(n for g, n in getattr(c, "lit_added", {}).items() if g <= k)))
    return ats.pop() if len(ats) == 1 else None


def alias_library_data(w: Path, copies: list, names: dict, pdg: int, lib: list, lib_at: int) -> int:
    """A global src/ names in the library's data (E07C8.C's g8894 is the library's _argv, ds:8894): the
    copies call it by the library's public there (the library's data moves as a whole), and a header
    that declares that public with its own type gets it under another name. The globals renamed."""
    if not lib:
        return 0
    text = (w / "PROBE.TXT").read_text("latin-1")
    text = text[text.index("Publics by Value"):]
    lo, hi = lib[0][0], max(o + l for o, l in lib)
    pubs = {}
    for m in re.finditer(r"^\s*([0-9A-F]{4}):([0-9A-F]{4})\s+(?:idle\s+)?(\S+)\s*$", text, re.M):
        off = (int(m.group(1), 16) - pdg) * 16 + int(m.group(2), 16)
        if lo <= off < hi:
            pubs.setdefault(off + lib_at - lo, m.group(3))
    n = 0
    for c in copies:
        for name, off in c.data.items():
            pub = pubs.get(off) if not c.unit.get("twin") else None
            if pub is None or name in c.data_defined:
                continue
            want = pub if c.asm else pub[1:]
            if want == name:
                continue
            text, _ = rename.replace_word(c.text_out, name, want, c.asm)
            if not c.asm and re.search(r"^#include", text, re.M):   # the header's own declaration aside
                first = re.search(r"^#include", text, re.M).start()
                last = max(m.end() for m in re.finditer(r"^#include[^\n]*\n", text, re.M))
                text = (text[:first] + "#define %s __hdr%s\n" % (want, want) + text[first:last] +
                        "#undef %s\n" % want + text[last:])
            c.text_out = text
            c.path.write_text(text.replace("\r\n", "\n").replace("\n", "\r\n"))
            print("rebuild: %s is the library's %s (ds:%04X, %s)" % (name, pub, off, c.path.name))
            names.pop(off, None)
            n += 1
    return n


def mods_of(copies: list) -> set:
    return {c.path.stem.upper() for c in copies}


def program_defines(w: Path, pimg: bytes, oimg: bytes, pdg: int, contribs: list, mine: set) -> list:
    """The library's DGROUP publics whose references in the original's code all point somewhere else
    than the library's data moved as a whole (the shift most publics share): the program defines them."""
    text = map_text(w)
    text = text[text.index("Publics by Value"):] if "Publics by Value" in text else ""
    lib = [(o, l, m) for o, l, sg, m in contribs if l and m.split(".")[0] not in mine | {"C0", "PROBE"}]
    pubs = []
    for m in re.finditer(r"^\s*([0-9A-F]{4}):([0-9A-F]{4})\s+(?:idle\s+)?(\S+)\s*$", text, re.M):
        off = (int(m.group(1), 16) - pdg) * 16 + int(m.group(2), 16)
        if any(o <= off < o + l for o, l, _ in lib):
            pubs.append((off, m.group(3)))
    refs = {}
    for k in range(0, min(pdg * 16, len(oimg)) - 1):
        x = int.from_bytes(pimg[k:k + 2], "little")
        refs.setdefault(x, []).append(int.from_bytes(oimg[k:k + 2], "little"))
    shifts, seen = Counter(), {}
    for off, sym in pubs:
        ys = [y for y in refs.get(off, []) if y != off]
        if len(ys) >= 2 and len(set(ys)) == 1:
            seen[sym] = (off, ys[0])
            shifts[ys[0] - off] += 1
    if not shifts:
        return []
    common, _ = shifts.most_common(1)[0]
    start = lib[0][0] + common                      # the library's data in the original
    return [(y, sym) for sym, (off, y) in sorted(seen.items()) if y - off != common and y < start]


def map_text(w: Path) -> str:
    """TLINK's map of the last link in w."""
    return next(iter(list(w.glob("*.MAP")) + list(w.glob("*.map")))).read_text("latin-1")


def link(program: str, w: Path, copies: list[Copy], fillers: dict[int, list], tail: str | None,
         empty: dict[int, list] | None = None):
    """TLINK the start-up code, the modules in order (a filler, and an empty code segment, after the
    module it follows, the overlay units under /o) and the libraries."""
    cfg = PROGRAMS[program]
    objs, overlaid = [], False
    after = {k: list((empty or {}).get(k, [])) + list(fillers.get(k, [])) for k in set(fillers) | set(empty or {})}
    for k in sorted(after):
        if k < 0:
            objs += after[k]
    for i, c in enumerate(copies):
        ovl = c.unit["start"].startswith("ovl")
        if ovl != overlaid:
            objs.append("/o" if ovl else "/o-")
            overlaid = ovl
        objs.append(c.path.stem)
        if i in after:
            if overlaid:
                objs.append("/o-")
                overlaid = False
            objs += after[i]
    if overlaid:
        objs.append("/o-")
    if (w / "FAR.OBJ").exists() or (w / "far.obj").exists():
        objs.append("FAR")
    if (w / "EXTREF.OBJ").exists() or (w / "extref.obj").exists():
        objs.append("EXTREF")
    if tail:
        objs.append(tail)
    out = program.split(".")[0].upper()          # TLINK writes the name into the overlay data (_EXEINFO_)
    for f in list(w.glob("*.EXE")) + list(w.glob("*.exe")) + list(w.glob("*.MAP")) + list(w.glob("*.map")):
        f.unlink()
    # a DOS command line holds 127 characters: the objects go in a response file, one per line
    resp = ["D:\\LIB\\C0%s" % cfg["model"]] + objs
    (w / "OUT.LNK").write_text(" +\r\n".join(resp) + "\r\n%s\r\n%s\r\n" % (out, out) +
                               "+".join("D:\\LIB\\" + l for l in cfg["libs"]) + "\r\n")
    r = tc.run("tc100", "TLINK", ["/c", "/m", "/s", "@OUT.LNK"], cwd=w)
    log = r.stdout.decode("latin-1")
    exe = next(iter(list(w.glob("*.EXE")) + list(w.glob("*.exe"))), None)
    if not exe or re.search(r"^Error", log, re.M):
        return None, log
    return exe, log


def dgroup_contribs(w: Path) -> tuple[int, list]:
    """Our DGROUP frame and its contributions (offset, length, segment, module) in map order."""
    text = map_text(w)
    rows = re.findall(r"^\s*([0-9A-F]{4}):([0-9A-F]{4}) ([0-9A-F]{4}) C=(\w+)\s+S=(\S+)\s+G=(\S+)\s+M=(\S+)", text, re.M)
    dg = next(int(f, 16) for f, o, l, c, s, g, m in rows if g == "DGROUP")
    out = [((int(f, 16) - dg) * 16 + int(o, 16), int(l, 16), s, m.upper()) for f, o, l, c, s, g, m in rows if g == "DGROUP"]
    return dg, out


def library_bss(pimg: bytes, oimg: bytes, pdg: int, contribs: list, mine: set, code: range) -> int | None:
    """Where the library's _BSS starts in the original: the library's code addresses it."""
    lib = [(o, l) for o, l, s, m in contribs if s == "_BSS" and l and m.split(".")[0] not in mine]
    if not lib:
        return None
    return shifted(pimg, oimg, lib[0][0], max(o + l for o, l in lib), code)


def shifted(pimg: bytes, oimg: bytes, lo: int, hi: int, code: range, least: int = 4) -> int | None:
    """Where our DGROUP range [lo, hi) of the probe link is in the original: each word of the code that
    points into it votes for the shift to the original's word there."""
    votes = Counter()
    for k in range(code.start, min(code.stop, len(pimg), len(oimg)) - 1):
        x, y = int.from_bytes(pimg[k:k + 2], "little"), int.from_bytes(oimg[k:k + 2], "little")
        if lo <= x < hi and y != x:                 # the probe has no fillers: its BSS lies elsewhere
            votes[y - x] += 1
    if not votes:
        return None
    (shift, n), *rest = votes.most_common(2)
    # few real references (EOB.EXE's library BSS: three, the rest chance words of code): a clear winner
    clear = n >= 3 and n > 2 * (rest[0][1] if rest else 0)
    return (lo + shift) & 0xFFFF if n >= least or clear else None


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
    text = map_text(w)
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
    if program.endswith(".OVL"):                    # a graphics driver: one module, built and compared whole
        import driver
        return driver.main([program] + (["-v"] if a.verbose else []))
    raw = (ROOT / "eob" / program).read_bytes()
    orig = read_mz(raw)
    oimg, orel = orig["image"], {s * 16 + o for o, s in orig["relocs"]}
    odg = cmatch.dgroup_of(ROOT / "eob" / program)
    w = ROOT / "build" / "rebuild" / program.split(".")[0].lower()
    shutil.rmtree(w, ignore_errors=True)
    w.mkdir(parents=True)
    space = Space(program)
    space.image, space.dgroup, space.relocs = oimg, odg, orel
    space.reloc_seq = [sg * 16 + o for o, sg in orig["relocs"]]
    copies, names, far = make_copies(program, w, space, find_main(program, oimg, orel))
    if far and a.verbose:
        print("rebuild: far objects to place: %s" % ", ".join("%s at %s:%04X" % ("/".join(sorted(v)), k[0] if isinstance(k[0], str) else "%04X" % k[0], k[1]) for k, v in sorted(far.items(), key=str)))
    build_objs(w, copies)
    if a.extern:                                        # library modules nothing calls (see missing_library)
        assemble(w, "EXTREF", "\r\n".join(["EXTRN %s:FAR" % x for x in a.extern] + ["END"]) + "\r\n")   # no segment
    def probe(far_defined=()):
        """PROBE.ASM: a label for every name of the data, all at one place, so the link shows where
        each module's own data falls; the far objects too, until FAR.ASM defines them."""
        syms = [s for o in sorted(names) for s in names[o]]
        syms += sorted({x for v in far.values() for x in v} - set(syms) - set(far_defined))
        assemble(w, "PROBE", "\r\n".join(["DGROUP GROUP _DATA, _BSS"] + ["PUBLIC %s" % s for s in syms] +
                                         ["_DATA SEGMENT BYTE PUBLIC 'DATA'", "_DATA ENDS", "_BSS SEGMENT BYTE PUBLIC 'BSS'"] +
                                         ["%s label byte" % s for s in syms] + ["_BSS ENDS", "END"]) + "\r\n")
    probe()
    exe, log = link(program, w, copies, {}, "PROBE")
    if not exe:
        print(log[-3000:])
        return 1
    # far objects: one in a library's segment (the overlay manager's data, INTRO.EXE 0E6C) is named from
    # the library's public that covers it, in our own link (the segments map by their index in TLINK's
    # segment table); Westwood's own far data is a generated module of the original's bytes
    westwood = alias_far(program, w, exe, copies, far)
    far_mod = far_objects(program, space, westwood, oimg) if westwood else None
    if far_mod:
        assemble(w, "FAR", far_mod)
    if westwood != far or far_mod:
        probe({x for v in westwood.values() for x in v} if far_mod else ())
        build_objs(w, copies)
        exe, log = link(program, w, copies, {}, "PROBE")
        if not exe:
            print(log[-3000:])
            return 1
    if relabel_library_segments(w, copies, space):  # (with the far data in place)
        build_objs(w, copies)
        exe, log = link(program, w, copies, {}, "PROBE")
        if not exe:
            print(log[-3000:])
            return 1
    pimg = read_mz(exe.read_bytes())["image"]
    pdg, contribs = dgroup_contribs(w)
    # a library variable the program defines itself (INTRO.EXE's _ovrbuffer, the overlay buffer size,
    # in Westwood's data at ds:1A28): the original's library code addresses it there, so the filler
    # names it, and TLINK does not link the library's default (OVRBUFF)
    own = program_defines(w, pimg, oimg, pdg, contribs, mods_of(copies))
    if own:
        for v, sym in own:
            print("rebuild: the program defines the library's %s itself, at ds:%04X" % (sym, v))
            names.setdefault(v, []).append(sym)
        probe({x for v in westwood.values() for x in v} if far_mod else ())
        exe, log = link(program, w, copies, {}, "PROBE")
        if not exe:
            print(log[-3000:])
            return 1
        pimg = read_mz(exe.read_bytes())["image"]
        pdg, contribs = dgroup_contribs(w)
    shutil.copy(next(iter(list(w.glob("*.MAP")) + list(w.glob("*.map")))), w / "PROBE.TXT")   # kept, for the reader
    odata = oimg[odg * 16:]
    image_end = len(oimg) - odg * 16
    mods = {c.path.stem.upper() for c in copies}
    ovl = {c.path.stem.upper() for c in copies if c.unit["start"].startswith("ovl")}
    c0_end = next(o + l for o, l, s, m in contribs if s == "_DATA" and m.startswith("C0"))
    mine = [(o, l, m.split(".")[0]) for o, l, s, m in contribs if l and s == "_DATA" and m.split(".")[0] in mods]
    # where each module's data is in the original: the resident ones in link order, an overlay
    # module's anywhere (its code is not in the image, so only its data shows where it was linked)
    placed, cursor = {}, c0_end
    copy_of = {c.path.stem.upper(): c for c in copies}
    spans = data_spans(program)
    eob_spans = data_spans("EOB.EXE") if program != "EOB.EXE" else {}

    def twin_place(stem: str) -> int | None:
        """A twin's whole data (an EOB.EXE file with /*@data */ linked into INTRO.EXE): where the twin
        proof's address map puts the start of its EOB.EXE span."""
        c = copy_of[stem]
        if not c.whole_data or stem not in eob_spans or not c.unit.get("twin"):
            return None
        return space.ds(eob_spans[stem], True, c.unit["twin"])

    for o, l, stem in mine:
        if stem in ovl:
            continue
        at = spans.get(stem) if copy_of[stem].whole_data else None
        at = twin_place(stem) if at is None else at
        at = by_template(copy_of[stem], pimg[pdg * 16:], odata, o, l) if at is None else at
        at = locate(pimg[pdg * 16:], odata, cursor, o, o + l) if at is None else at
        if at is None:
            print("rebuild: %s's data (%d bytes) is not in the original" % (stem, l))
            return 1
        placed[stem], cursor = (at, l), at + l
    for o, l, stem in mine:
        if stem in ovl:
            at = spans.get(stem) if copy_of[stem].whole_data else None
            at = by_template(copy_of[stem], pimg[pdg * 16:], odata, o, l) if at is None else at
            at = locate(pimg[pdg * 16:], odata, c0_end, o, o + l) if at is None else at
            if at is None:
                print("rebuild: %s's data (%d bytes) is not in the original" % (stem, l))
                return 1
            placed[stem] = (at, l)
    # an overlay module goes after the resident module whose data precedes its own (INTRO.EXE: IOV0
    # after I0892, its main module)
    resident = [c for c in copies if c.path.stem.upper() not in ovl]
    def ovl_no(c):
        return int(re.match(r"ovl(\d+)", c.unit["start"]).group(1))
    # where each resident module's data begins in the original: its own, else the first of the data
    # whose relocations follow its code's (E1414.C's ds:5BAA, which puts EOV0 after it)
    pos = {stem: at for stem, (at, _) in placed.items()}
    spans = [(lin(c.unit["start"]), lin(c.unit["start"]) + c.unit["bytes"], c.path.stem.upper()) for c in copies
             if ":" in c.unit["start"] and not c.unit["start"].startswith("ovl")]
    last = None
    for x in [sg * 16 + o for o, sg in orig["relocs"]]:
        if x >= odg * 16:
            if last and last not in pos:
                pos[last] = x - odg * 16
            continue
        last = next((s for p0, p1, s in spans if p0 <= x < p1), last)
    later = []
    for c in [c for c in copies if c.path.stem.upper() in ovl]:
        if c.path.stem.upper() not in placed:
            later.append(c)
            continue
        at = placed[c.path.stem.upper()][0]
        before = [r for r in resident if pos.get(r.path.stem.upper(), 1 << 20) < at]
        k = max(resident.index(r) for r in before) + 1 if before else len(resident)
        resident.insert(k, c)
    for c in sorted(later, key=ovl_no):
        # no data of its own to show its place: after the overlay before it, as TLINK numbers the
        # overlays in link order (EOB.EXE's EOV2 after EOV1, EOV4 after EOV3)
        prev = [r for r in resident if r.path.stem.upper() in ovl and ovl_no(r) < ovl_no(c)]
        k = resident.index(max(prev, key=ovl_no)) + 1 if prev else len(resident)
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
    if alias_library_data(w, copies, names, pdg, lib, lib_at):
        build_objs(w, copies)
    # the fillers: the original's bytes between the start-up code's data, each module's and the library's
    order = [c.path.stem.upper() for c in copies]
    ranges, prev_end, prev_idx = [], c0_end, -1
    for stem in [x for x in order if x in placed]:
        at, l = placed[stem]
        if at > prev_end:
            ranges.append((prev_idx, prev_end, at))
        elif at < prev_end:
            print("rebuild: %s's data (ds:%04X) overlaps the module before it (to ds:%04X)" % (stem, at, prev_end))
        prev_end, prev_idx = at + l, order.index(stem)
    if lib_at > prev_end:
        ranges.append((prev_idx, prev_end, lib_at))
    oseq = [sg * 16 + o for o, sg in orig["relocs"]]
    fillers: dict[int, list] = {}
    for idx, lo, hi in ranges:
        for i2, a2, b2, fix_order in data_owners(oseq, copies, odg, lo, hi, idx, a.verbose):
            name = "FILL%02d" % sum(len(v) for v in fillers.values())
            fillers.setdefault(i2, []).append(name)
            assemble(w, name, filler(name, oimg, odg * 16, a2, b2, names, True, orel, space.seg_names, odg, fix_order,
                                     space.stub_names))
    # the uninitialised globals: past the image (START1.EXE), or, when the image holds all of DGROUP
    # (INTRO.EXE, its stack segment after it), the ones past every data filler and the library's data.
    # Turbo C puts them in their module's _BSS, so they come before the library's _BSS (INTRO.EXE's
    # library code addresses its BSS from ds:2E30, after them); the last one ends where that starts
    filled = [(c0_end, lib_at)]
    lib_bss = library_bss(pimg, oimg, pdg, contribs, mods | {"C0"}, space.lib[program][0])
    # where _BSS starts in the original: the start-up code clears it from there (C0's bdata)
    p_bss = next(o for o, l, s, m in contribs if s == "_BSS")
    c0 = next(u for u in all_units() if u["program"] == program and u["kind"] == "startup")
    bss_at = shifted(pimg, oimg, p_bss, p_bss + 1, range(lin(c0["start"]), lin(c0["start"]) + c0["bytes"]), 1)
    bss_at = p_bss if bss_at is None else bss_at
    if a.verbose:
        print("rebuild: _BSS at ds:%04X in the probe, ds:%04X in the original" % (p_bss, bss_at))
    if a.verbose:
        print("rebuild: the library's BSS at ds:%s in the original; image end %04X, data end %04X, names %04X-%04X" % (
            "%04X" % lib_bss if lib_bss is not None else "?", image_end, lib_at, min(names), max(names)))
    common = sorted(o for o in names if (o >= image_end or (image_end > max(names, default=0) and o >= lib_at
                                                         and not any(lo <= o < hi for lo, hi in filled)))
                    and bss_at <= o and (lib_bss is None or o < lib_bss))
    tail = None
    if common:
        hi = lib_bss if lib_bss is not None else max(common) + 4
        assemble(w, "BSS", filler("BSS", oimg, odg * 16, common[0], hi, names, False))
        tail = "BSS"
    exe, log = link(program, w, copies, fillers, tail, empty_segments(w, space, copies))
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
    compare_overlays(raw, exe.read_bytes(), oimg, img, orig["relocs"], ours["relocs"])
    if rc == 0 and exe.read_bytes() == raw:
        print("rebuild: %s: the whole file is identical, header included (%d bytes)" % (program, len(raw)))
    elif rc == 0:
        print("rebuild: %s: the load image and relocations are identical; the rest of the file differs" % program)
    return rc


def compare_overlays(raw: bytes, built: bytes, oimg: bytes, img: bytes, oseq: list, wseq: list) -> None:
    """The relocation table's order, and the overlays (FBOV): each one's code and fixups. A far pointer
    that differs only in its stub entry (the entry's offset in the same stub) is counted apart: TLINK
    numbers an overlay's entries from its module's public order, which Turbo C takes from a hash of the
    function names, so the original names set it (playbook 50, 'Left on')."""
    import fbov
    mo, mw = fbov.manifest(raw)[0], fbov.manifest(built)[0]
    print("rebuild: the relocation table is %s" % ("in the original's order" if list(oseq) == list(wseq)
                                                   else "in another order"))
    if not mo.get("overlays"):
        return
    stubs = {o["stub_para"] for o in mo["overlays"]}
    orel = {sg * 16 + o for o, sg in oseq}
    lo, hi = min(stubs) * 16, max(o["stub_para"] * 16 + 0x20 + 5 * o["entry_count"] for o in mo["overlays"])
    entry = table = 0
    other, k = [], 0
    while k < min(len(img), len(oimg)):
        if img[k] == oimg[k]:
            k += 1
            continue
        if lo <= k < hi:
            table += 1
        elif any(s + 2 in orel and s <= k < s + 2 and int.from_bytes(oimg[s + 2:s + 4], "little") in stubs
                 for s in (k - 1, k)):
            entry += 1
        else:
            other.append(k)
        k += 1
    print("rebuild: of the image's differing bytes, %d are stub entries in far pointers, %d in the stub "
          "table, %d elsewhere%s" % (entry, table, len(other), " (%s; the link date is in _EXEINFO_)" % " ".join(
              "%05X" % x for x in other) if 0 < len(other) <= 8 else ""))
    # in an overlay's code a segment is its segment-table index times 8 (the fixups say where)
    stub_idx = {8 * i for i, sg in enumerate(mo["segments"]) if sg["kind_name"] == "overlay-stub" and sg["size"]}
    bo, bw = read_mz(raw)["tail"][16:], read_mz(built)["tail"][16:]
    for x, y in zip(mo["overlays"], mw["overlays"]):
        cx = bo[x["record_offset"]:x["record_offset"] + x["code_bytes"]]
        cy = bw[y["record_offset"]:y["record_offset"] + y["code_bytes"]]
        if len(cx) != len(cy):
            print("rebuild: overlay %d: %d bytes of code, the original %d" % (x["overlay"], len(cy), len(cx)))
            continue
        fx, fy = set(x["fixups"]), set(y["fixups"])
        diff = [k for k, (a, b) in enumerate(zip(cx, cy)) if a != b]
        ent = [k for k in diff if any(s + 2 in fx and s <= k < s + 2 and int.from_bytes(cx[s + 2:s + 4], "little") in stub_idx
                                      for s in (k - 1, k))]
        nd = len(diff)
        print("rebuild: overlay %d: %s%s" % (x["overlay"], "code identical" if not nd else "%d code bytes differ%s" % (
              nd, ", all stub entries" if len(ent) == nd else " (%d of them stub entries)" % len(ent)),
              "" if fx == fy else "; fixups only the original's at %s, only ours at %s" % (
                  " ".join("%04X" % f for f in sorted(fx - fy)) or "-", " ".join("%04X" % f for f in sorted(fy - fx)) or "-")))


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

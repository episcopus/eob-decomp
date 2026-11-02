#!/usr/bin/env python3
"""cmatch.py: prove a C or assembly unit against the original (L2; SPEC 4.2, playbook 30).

The source names the original address of everything it defines or uses, in a comment on the line
of the declaration or definition (`;` comments in assembly):

    extern char gFlag; /*@ ds:F3B9 */                  a global: offset in DGROUP
    extern void far *ListAdd(...); /*@ 1658:001E */    a callee: seg:off (a function in an overlay
                                                        is called through its stub entry, 2106:002A)
    /*@ 0CF7:0007 */ void ListInit(...)                a function of this unit
    /*@ ovl6:0123 */ void f(...)                       a function of overlay 6 (offset in its code)
    extern unsigned char far gReg; /*@ 0000:3B36 */    a far object in another segment (data in an
                                                        assembly unit's code segment): mov ax,seg gReg
                                                        then es:[gReg]; EXTRN _gReg:BYTE in assembly
    /*@ 08E6:00C3 stub */ void f00C3(...) {}           a sibling not decompiled yet (called near by
                                                        the functions of this file; not compared)
    /*@ 0ACD:091C draft */ void f(...)                 written but not matching yet: compared and
                                                        reported, not a failure, never L2
    /*@flags -ml -O -Z -G */                          the file's switches (default: cflags); -B: compiled
                                                        through the assembler (TCC -S, then TASM /ml /jNOSMART)
    PUBLIC Blit ; @ 19EA:0000                          the same in assembly

How it proves: the unit is compiled once (TCC with `cflags` of tools/re/tc_toolchain.toml, or TASM
for .ASM) and linked twice with C0L, a generated assembly stub that defines every extern it names,
and CL.LIB. Link B pads the stub's data and code by 111h bytes, so the addresses in our code move
between A and B and the constants do not (code by 113h, so that the unit's offset in its paragraph
changes whatever the segment alignment). Over each function, byte by byte:

  - a byte that is the same in A and B must be the original's byte;
  - a field (the word starting at a byte that changed, or a relocated segment word) must name
    the same thing as the original's word at that place:
      moved by 111h+    a global (same name, same offset in it) or a string literal of this
                        file (same text)
      moved by little   an offset into this unit's own code (jump tables, labels): the same
                        distance from the function's start
      relocation        DGROUP, this unit's segment, the segment of the far pointer before it, or
                        the segment of a far object (its own stub segment FAR_SSSS)
      moved by 111h+    from FAR_BASE up: the offset of a far object (same name, same offset in it)
  - a near call (E8) reaches the function of this file with the same name;
  - a far pointer (offset word, then a relocated segment word) to a function names it: an
    annotated function, or a runtime library function, located in the original through
    build/re/rtl_compare.json (`symbols`), which our link does not move.

  cmatch.py FILE [--program eob/EOB.EXE] [--flags="-ml -O -Z"] [--write] [--show NAME] [--unit ID]
--write records level L2 and the name of every matched function in re/symbols.toml.
Exit code 0 when every annotated function defined in the file matches.
"""
from __future__ import annotations

import argparse, json, re, shutil, struct, sys, tomllib
from pathlib import Path

from capstone import CS_ARCH_X86, CS_MODE_16, Cs

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools" / "re"))
from mzutil import read_mz  # noqa: E402
import tc_toolchain as tc  # noqa: E402

MD = Cs(CS_ARCH_X86, CS_MODE_16)
ANNOT = re.compile(r"(?:/\*|;)\s*@\s*(ds:[0-9A-Fa-f]{1,4}|ovl\d+:[0-9A-Fa-f]{1,4}|[0-9A-Fa-f]{4}:[0-9A-Fa-f]{4})(\s+stub|\s+draft)?\s*(?:\*/)?")
FLAGS = re.compile(r"/\*@flags\s+([^*]+?)\s*\*/")
DGROUP = {"EOB.EXE": 0x212E}
KEYWORDS = {"void", "char", "int", "long", "short", "unsigned", "signed", "far", "near", "huge",
            "extern", "static", "struct", "union", "const", "volatile", "interrupt", "cdecl", "pascal"}
ASM_WORDS = {"public", "extrn", "proc", "far", "near", "endp", "label", "byte", "word", "dword", "db", "dw", "dd"}
PAD = 0x111       # data pad in link B
PAD_CODE = 0x113  # code pad: with byte or word alignment the unit's offset in its paragraph changes


# ---------------------------------------------------------------- the source

DRAFTS: set = set()
FARDATA: set = set()   # externs annotated seg:off that are data, not functions: far objects in another segment
FAR_BASE = 0x8000
# -B: units compiled through the assembler. Their bytes come from TASM without SMART encoding: with
# SMART, TASM 2.0 shortens `or bx,1` to 83 and changes other forms the original has (e14ef, e19ec)
TASM_OPTS = (__import__("os").environ.get("TASM_OPTS") or "/ml /jNOSMART").split()
# experiments: another installed assembler release for -B and the stubs (tools/re/tc_toolchain.toml)
ASM_RELEASE = __import__("os").environ.get("ASM_RELEASE") or None      # their offsets in our stub start here, apart from anything in DGROUP


def parse_annotations(src: str, asm: bool = False) -> tuple[dict, dict, set, set]:
    """funcs: name -> (seg, off) or ("ovlN", off); data: name -> DS offset; defined: this file's
    functions; stubs: those of them marked `stub` (a sibling not decompiled yet: not compared).
    Functions marked `draft` (written, not matching yet) go to DRAFTS: compared, not failures."""
    funcs, data, defined, stubs = {}, {}, set(), set()
    DRAFTS.clear()
    FARDATA.clear()
    for line in src.splitlines():
        m = ANNOT.search(line)
        if not m:
            continue
        decl = ANNOT.sub("", line)
        if asm:
            names = [n for n in re.findall(r"[A-Za-z_@$?][\w@$?]*", decl) if n.lower() not in ASM_WORDS]
            is_def = not re.search(r"\bextrn\b", decl, re.I)
        else:
            fp = re.search(r"\(\s*(?:far\s*|near\s*)?\*\s*([A-Za-z_]\w*)\s*\)", decl)  # function pointer
            names = [fp.group(1)] if fp else [n for n in re.findall(r"([A-Za-z_]\w*)\s*(?:\(|\[|;|=|,)", decl)
                                              if n not in KEYWORDS]
            is_def = not decl.strip().startswith("extern") and not decl.rstrip().endswith(";")
        if not names:
            raise SystemExit("cmatch: no name on annotated line: %s" % line.strip())
        name, addr = names[0], m.group(1)
        if addr.lower().startswith("ds:"):
            data[name] = int(addr[3:], 16)
            continue
        seg, off = addr.split(":")
        funcs[name] = (seg.lower(), int(off, 16)) if seg.lower().startswith("ovl") else (int(seg, 16), int(off, 16))
        if not is_def and not seg.lower().startswith("ovl"):
            # far data: `extern unsigned char far gX; /*@ 0000:3B36 */`, `EXTRN _gX:BYTE ; @ 0000:3B36`
            if (asm and re.search(r":\s*(BYTE|WORD|DWORD)\b", decl, re.I)) or (
                    not asm and not re.search(r"\b%s\s*\(" % re.escape(name), decl) and not fp):
                FARDATA.add(name)
        if is_def:
            defined.add(name)
            if m.group(2) and m.group(2).strip() == "stub":
                stubs.add(name)
            elif m.group(2):
                DRAFTS.add(name)
    return funcs, data, defined, stubs


def stub_source(funcs: dict, data: dict, defined: set, pad: bool, cname: bool = True) -> str:
    """TASM source defining every extern: data in DGROUP, functions as far returns."""
    sym = (lambda n: "_" + n) if cname else (lambda n: n)
    out = [".MODEL LARGE", ".DATA"]
    if pad:
        out.append("padd db %d dup (0)" % PAD)
    for n in sorted(data):
        out += ["PUBLIC %s" % sym(n), "%s db 256 dup (0)" % sym(n)]
    out.append(".CODE")
    if pad:
        out.append("padc db %d dup (90h)" % PAD_CODE)
    for n in sorted(set(funcs) - defined - FARDATA):
        out += ["PUBLIC %s" % sym(n), "%s PROC FAR" % sym(n), "ret", "%s ENDP" % sym(n)]
    # far data: one segment per original segment (FAR_SSSS), the objects at offsets that are unique
    # across them (FAR_BASE + 256 * i); the pad build moves them by PAD, as it moves DGROUP
    i = 0
    for seg in sorted({funcs[n][0] for n in FARDATA}):
        out.append("FAR_%04X SEGMENT PARA PUBLIC 'FAR_DATA'" % seg)
        names = sorted(n for n in FARDATA if funcs[n][0] == seg)
        out.append("db %d dup (0)" % (FAR_BASE + 256 * i + (PAD if pad else 0)))
        for n in names:
            out += ["PUBLIC %s" % sym(n), "%s db 256 dup (0)" % sym(n)]
            i += 1
        out.append("FAR_%04X ENDS" % seg)
    out += ["PUBLIC _main", "_main PROC FAR", "xor ax,ax", "ret", "_main ENDP", "END"]
    return "\r\n".join(out) + "\r\n"


# ---------------------------------------------------------------- our build

class Build:
    """One link of our unit: image, relocations, publics, our unit's segment and data."""

    def __init__(self, w: Path, exe: str):
        mz = read_mz((w / exe).read_bytes())
        self.image = mz["image"]
        self.relocs = {s * 16 + o for o, s in mz["relocs"]}
        self.publics, self.dgroup, self.unit, self.lits = {}, None, None, (0, 0)
        self.far_frames: dict[int, int] = {}       # frame of a FAR_SSSS stub segment -> SSSS
        for line in (w / exe.replace(".EXE", ".MAP")).read_text("latin-1").splitlines():
            m = re.match(r"\s*([0-9A-F]{4}):([0-9A-F]{4}) ([0-9A-F]{4}) C=(\w+)\s+S=(\S+)\s+G=(\S+)\s+M=(\S+)", line)
            if m:
                frame, off, ln = (int(m.group(i), 16) for i in (1, 2, 3))
                cls, seg, grp, mod = m.group(4), m.group(5), m.group(6), m.group(7).upper()
                if grp == "DGROUP" and self.dgroup is None:
                    self.dgroup = frame
                if re.fullmatch(r"FAR_[0-9A-F]{4}", seg):
                    self.far_frames[frame] = int(seg[4:], 16)
                if cls == "CODE" and mod.startswith("U."):
                    self.unit = (frame, off, ln)
                if mod.startswith("U.") and seg == "_DATA":
                    self.lits = (off, off + ln)
                continue
            m = re.match(r"\s*([0-9A-F]{4}):([0-9A-F]{4})\s+(?:idle\s+)?(\S+)\s*$", line)
            if m and m.group(3) != "Abs":
                self.publics[m.group(3)] = int(m.group(1), 16) * 16 + int(m.group(2), 16)
        self.by_linear = {}
        for n, v in sorted(self.publics.items()):
            self.by_linear.setdefault(v, n)

    def word(self, lin: int) -> int:
        return struct.unpack_from("<H", self.image, lin)[0]

    def cstr(self, off: int) -> bytes:
        p = self.dgroup * 16 + off
        return self.image[p:self.image.index(b"\0", p)]

    def name(self, n: str) -> str | None:
        return ("_" + n) if ("_" + n) in self.publics else (n if n in self.publics else None)


def link(w: Path, release: str, flags: list, src_name: str, funcs, data, defined, asm: bool) -> tuple[Build, Build]:
    if asm:
        r = tc.run((ASM_RELEASE or tc.manifest()["pinned_asm"]), "TASM", ["/mx", src_name], cwd=w)
    else:
        if "-B" in flags:
            # compiled through the assembler, as TCC -B does (inline asm forces it): TCC writes U.ASM
            # (-S) and TASM assembles it; TCC cannot start TASM itself under emu2. TCC's own assembler
            # merges identical jump tails that the original keeps (e14ef: four case tails)
            r = tc.run(release, tc.manifest()["release"][release].get("compiler", "TCC"), ["-S", "-ID:\\INCLUDE"] + [f for f in flags if f != "-B"] + [src_name], cwd=w)
            if (w / "U.ASM").exists():
                r = tc.run((ASM_RELEASE or tc.manifest()["pinned_asm"]), "TASM", TASM_OPTS + ["U.ASM"], cwd=w)
        else:
            r = tc.run(release, tc.manifest()["release"][release].get("compiler", "TCC"), ["-c", "-ID:\\INCLUDE"] + flags + [src_name], cwd=w)
    out = r.stdout.decode("latin-1")
    failed = re.search(r"\*\*Error\*\*", out) if (asm or "-B" in flags) else re.search(r"^Error ", out, re.M)
    if not (w / "U.OBJ").exists() or failed:
        raise SystemExit(out)
    builds = []
    for v, pad in (("A", False), ("B", True)):
        (w / ("STUB%s.ASM" % v)).write_text(stub_source(funcs, data, defined, pad, cname=not asm))
        r = tc.run((ASM_RELEASE or tc.manifest()["pinned_asm"]), "TASM", ["/mx", "STUB%s.ASM" % v], cwd=w)
        if not (w / ("STUB%s.OBJ" % v)).exists():
            raise SystemExit(r.stdout.decode("latin-1"))
        r = tc.run(release, "TLINK", ["/c", "/m", "/s", "D:\\LIB\\C0L+STUB%s+U,P%s,P%s,D:\\LIB\\CL" % (v, v, v)], cwd=w)
        if not (w / ("P%s.EXE" % v)).exists():
            raise SystemExit(r.stdout.decode("latin-1"))
        builds.append(Build(w, "P%s.EXE" % v))
    return builds[0], builds[1]


# ---------------------------------------------------------------- the original

class Original:
    """What a function is compared with: the load module, or one overlay's code (with its fixups)."""

    def __init__(self, program: Path, dgroup: int, overlay: int | None = None):
        mz = read_mz(program.read_bytes())
        self.load_module, self.dgroup, self.overlay = mz["image"], dgroup, overlay
        if overlay is None:
            self.image = mz["image"]
            self.relocs = {s * 16 + o for o, s in mz["relocs"]}
        else:
            stem = program.stem.lower()
            man = json.loads((ROOT / "build" / "re" / ("%s.json" % stem)).read_text())
            self.image = (ROOT / "build" / "re" / ("%s-overlays" % stem) / ("ovl%02d.bin" % overlay)).read_bytes()
            self.relocs = set(man["overlays"][overlay]["fixups"])
            # an overlay's segment fixups hold a byte offset into TLINK's segment table (index * 8)
            self.segtable = [s["para"] for s in man["segments"]]
        lib = ROOT / "build" / "re" / ("rtl_compare.json" if program.name.upper() == "EOB.EXE" else "rtl_intro.json")
        self.library = json.loads(lib.read_text())["tc100"].get("symbols", {}) if lib.exists() else {}

    def word(self, p: int) -> int:
        return struct.unpack_from("<H", self.image, p)[0]

    def seg(self, p: int) -> int:
        """The paragraph a segment word names (overlay fixups go through the segment table)."""
        v = self.word(p)
        if self.overlay is not None and p in self.relocs and v % 8 == 0 and v // 8 < len(self.segtable):
            return self.segtable[v // 8]
        return v

    def cstr(self, off: int) -> bytes:
        p = self.dgroup * 16 + off
        return self.load_module[p:self.load_module.index(b"\0", p)]


# ---------------------------------------------------------------- the comparison

def compare(fname, a: Build, b: Build, at_a: int, at_b: int, length: int, orig: Original, at_o: int,
            unit_frame: int | None, unit_base: int, funcs: dict, data: dict) -> str | None:
    """None if the range matches; otherwise the first difference."""
    ufa, uoa, _ = a.unit
    uob = b.unit[1]
    data_a = {a.name(n): (a.publics[a.name(n)] - a.dgroup * 16, d) for n, d in data.items() if a.name(n)}
    func_of = {a.name(n): n for n in funcs if a.name(n)}
    far_a = {}                                       # offset in its stub segment -> (name, original seg:off)
    for n in FARDATA:
        if a.name(n):
            lin = a.publics[a.name(n)]
            frame = max(f for f in a.far_frames if f * 16 <= lin)
            far_a[lin - frame * 16] = (n, funcs[n])

    def fail(k, what):
        return why(fname, k, what, a, at_a, orig, at_o)

    near = near_calls(a, at_a, length)
    k, want_seg = 0, {}
    while k < length:
        pa, pb, po = at_a + k, at_b + k, at_o + k
        if k in near:  # a near call to a function of this unit: the same function by name
            pub = a.by_linear.get(near[k])
            if pub not in func_of:
                return fail(k, "near call to %05X, not a function of this file" % near[k])
            seg, off = funcs[func_of[pub]]
            target = off if isinstance(seg, str) else seg * 16 + off
            got = (po + 2 + struct.unpack_from("<h", orig.image, po)[0]) & 0xFFFFF
            if isinstance(seg, str):
                got &= 0xFFFF
            if got != target:
                return fail(k, "near call to %s, original reaches %05X" % (func_of[pub], got))
            k += 2
            continue
        if pa + 2 in a.relocs and pa not in a.relocs and pa + 1 not in a.relocs:
            # the offset of a far pointer: a function (ours: annotated or library), or data
            target = a.word(pa + 2) * 16 + a.word(pa)
            pub = a.by_linear.get(target)
            if pub in func_of:
                seg, off = funcs[func_of[pub]]
                if orig.word(po) != off:
                    return fail(k, "far pointer to %s: original offset %04X, %s is at %04X" % (
                        func_of[pub], orig.word(po), func_of[pub], off))
                want_seg[k + 2] = None if isinstance(seg, str) else seg
                k += 2
                continue
            if pub in orig.library:
                lin = orig.library[pub]
                if orig.seg(po + 2) * 16 + orig.word(po) != lin:
                    return fail(k, "call to %s: original reaches %04X:%04X, %s is at %05X" % (
                        pub.lstrip("_"), orig.seg(po + 2), orig.word(po), pub.lstrip("_"), lin))
                want_seg[k + 2] = orig.seg(po + 2)
                k += 2
                continue
        if pa in a.relocs:
            va, vo = a.word(pa), orig.seg(po)
            if po not in orig.relocs:
                return fail(k, "segment word %04X has no relocation in the original" % va)
            if k in want_seg:
                want = want_seg.pop(k)
            elif va == a.dgroup:
                want = orig.dgroup
            elif va == ufa:
                want = unit_frame
            elif va in a.far_frames:                 # the segment of a far object (FARDATA)
                want = a.far_frames[va]
            else:
                return fail(k, "segment word %04X not explained" % va)
            if want is not None and vo != want:
                return fail(k, "segment %04X, original %04X" % (want, vo))
            k += 2
            continue
        if a.image[pa] == b.image[pb]:
            if a.image[pa] != orig.image[po]:
                return fail(k, "byte %02X, original %02X" % (a.image[pa], orig.image[po]))
            k += 1
            continue
        va, vb, vo = a.word(pa), b.word(pb), orig.word(po)
        delta = (vb - va) & 0xFFFF
        if PAD <= delta < PAD + 16:  # DGROUP moved by the pad, plus the alignment of later segments
            g = max(((off, n) for n, (off, _) in data_a.items() if off <= va), default=None)
            if g and va - g[0] < 256:
                want = data_a[g[1]][1] + (va - g[0])
                if vo != want:
                    return fail(k, "%s+%d is ds:%04X, original ds:%04X" % (g[1].lstrip("_"), va - g[0], want, vo))
            elif a.lits[0] <= va < a.lits[1]:
                if a.cstr(va) != orig.cstr(vo):
                    return fail(k, "literal %r, original %r" % (a.cstr(va), orig.cstr(vo)))
            elif va >= FAR_BASE and far_a and max((o for o in far_a if o <= va), default=-1) >= va - 255:
                o = max(o for o in far_a if o <= va)
                n, (seg, off) = far_a[o]
                if vo != (off + va - o) & 0xFFFF:
                    return fail(k, "%s+%d is %04X:%04X, original offset %04X" % (n, va - o, seg, off + va - o, vo))
            else:
                return fail(k, "address %04X not explained" % va)
        elif delta == (uob - uoa) & 0xFFFF:  # an offset into this unit's code: compared from the function start
            ours_fn = at_a - ufa * 16
            theirs_fn = at_o if unit_frame is None else at_o - unit_frame * 16
            if (va - ours_fn) & 0xFFFF != (vo - theirs_fn) & 0xFFFF:
                return fail(k, "code offset %+d from the function, original %+d" % (
                    (va - ours_fn) & 0xFFFF, (vo - theirs_fn) & 0xFFFF))
        else:
            return fail(k, "field %04X moved by %04X: not explained" % (va, delta))
        k += 2
    return None


def near_calls(a: Build, at_a: int, length: int) -> dict[int, int]:
    """Offsets (from the function start) of the displacement of every near call, with its target.
    Decoding stops at the first jump table (`jmp cs:[bx+table]`), which follows the code."""
    out, stop = {}, at_a + length
    for i in MD.disasm(a.image[at_a:at_a + length], at_a):
        if i.address >= stop:
            break
        if i.bytes[:3] == b"\x2e\xff\xa7":  # jmp word ptr cs:[bx+disp16]
            stop = min(stop, a.unit[0] * 16 + struct.unpack_from("<H", i.bytes, 3)[0])
        if i.bytes[0] == 0xE8:
            out[i.address + 1 - at_a] = i.address + 3 + struct.unpack_from("<h", i.bytes, 1)[0]
    return out


def side_by_side(a: Build, at_a: int, length: int, orig: Original, at_o: int) -> None:
    import itertools
    fmt = lambda i: "%04X %-12s %s %s" % (i.address, i.bytes.hex(), i.mnemonic, i.op_str)
    x = [fmt(i) for i in MD.disasm(a.image[at_a:at_a + length], 0)]
    y = [fmt(i) for i in MD.disasm(orig.image[at_o:at_o + length], 0)]
    for p, q in itertools.zip_longest(x, y, fillvalue=""):
        print("  %-52s | %s" % (p[:52], q))


def why(fname, k, what, a: Build, at_a, orig: Original, at_o) -> str:
    def at(img, base):
        for i in MD.disasm(img[base:base + k + 16], 0):
            if i.address <= k < i.address + i.size:
                return "%s %s" % (i.mnemonic, i.op_str)
        return "?"
    return "%s +%04X: %s (ours %s; original %s)" % (fname, k, what, at(a.image, at_a), at(orig.image, at_o))


# ---------------------------------------------------------------- main

def unit_of(path: Path, unit: str | None = None) -> dict | None:
    """The registry entry of the file's unit: `unit`, or the file name (src/E0CF7.C is e0cf7)."""
    p = ROOT / "re" / "units.toml"
    units = tomllib.loads(p.read_text()).get("unit", []) if p.exists() else []
    return next((u for u in units if u["id"] == (unit or path.stem).lower()), None)


def gaps_match(A, B, orig, covered: list, u: dict) -> bool:
    """The unit's bytes outside its functions (a start marker, padding, data between them) equal
    the original's, and do not move between our two links."""
    frame, off = (int(x, 16) for x in u["start"].split(":"))
    ours, theirs = A.unit[0] * 16 + A.unit[1], frame * 16 + off
    inside = set()
    for at, n in covered:
        inside.update(range(at - ours, at - ours + n))
    shift = (B.unit[0] * 16 + B.unit[1]) - ours
    for k in range(A.unit[2]):
        if k in inside:
            continue
        if A.image[ours + k] != B.image[ours + shift + k] or A.image[ours + k] != orig.image[theirs + k]:
            return False
    return True


def write_levels(names_by_addr: dict, flags: str, whole_unit: str | None = None) -> int:
    """Set name, level = "L2" and match_flags on the registry entries of matched functions. When the
    whole unit matched (every function, and the bytes between them), its entries that are not
    functions of the source (a start marker, padding the map took for a function) get L2 too, with
    a note."""
    p = ROOT / "re" / "symbols.toml"
    blocks = p.read_text().split("[[function]]\n")
    n = 0
    for i, b in enumerate(blocks):
        m = re.search(r'^addr = "(\w+):([0-9A-F]{4})"$', b, re.M)
        if not m or not re.search(r'^unit = "e', b, re.M):
            continue
        s = m.group(1)
        key = (s.lower() if s.lower().startswith("ovl") else int(s, 16), int(m.group(2), 16))
        name = names_by_addr.get(key)
        if not name:
            if whole_unit and re.search(r'^unit = "%s"$' % re.escape(whole_unit), b, re.M) \
                    and not re.search(r'^level = ', b, re.M):
                blocks[i] = b.rstrip("\n") + '\nlevel = "L2"\nnotes = "not a function: bytes of the unit outside its ' \
                    'functions (a marker or padding), matched with the whole unit"\n\n'
                n += 1
            continue
        b = re.sub(r'^(name|level|match_flags) = .*\n', "", b, flags=re.M)
        blocks[i] = b.rstrip("\n") + '\nname = "%s"\nlevel = "L2"\nmatch_flags = "%s"\n\n' % (name, flags)
        n += 1
    p.write_text("[[function]]\n".join(blocks))
    return n


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("file")
    ap.add_argument("--program", default=str(ROOT / "eob" / "EOB.EXE"))
    ap.add_argument("--flags", default=None)
    ap.add_argument("--release", default=None)
    ap.add_argument("--write", action="store_true")
    ap.add_argument("--show", metavar="NAME", help="print ours and the original side by side")
    ap.add_argument("--unit", help="the unit, when the file name is not its id (negative fixtures)")
    a = ap.parse_args()
    path = Path(a.file)
    asm = path.suffix.upper() == ".ASM"
    release = a.release or tc.manifest()["pinned"]
    src = path.read_text()
    fl = FLAGS.search(src)
    flags = (a.flags if a.flags is not None else fl.group(1) if fl else tc.manifest()["cflags"]).split()
    funcs, data, defined, stubs = parse_annotations(src, asm)
    w = ROOT / "build" / "match" / path.stem
    shutil.rmtree(w, ignore_errors=True)
    w.mkdir(parents=True)
    src_name = "U.ASM" if asm else "U.C"
    (w / src_name).write_text(src.replace("\r\n", "\n").replace("\n", "\r\n"))
    A, B = link(w, release, flags, src_name, funcs, data, defined, asm)
    program = Path(a.program)
    if a.write and a.unit:
        print("cmatch: --write records levels from src/ only, not from a file named with --unit")
        return 2
    u = unit_of(path, a.unit)
    ovl = u.get("overlay") if u else None
    if ovl is None:
        o = next((funcs[n][0] for n in defined if isinstance(funcs[n][0], str)), None)
        ovl = int(o[3:]) if o else None
    orig = Original(program, DGROUP.get(program.name.upper(), 0x212E), ovl)
    unit_frame, unit_base = (None, 0)
    if u and ovl is None:
        unit_frame, unit_base = (int(x, 16) for x in u["start"].split(":"))
    ours_end = A.unit[0] * 16 + A.unit[1] + A.unit[2]
    starts = sorted(A.publics[A.name(n)] for n in defined if A.name(n))
    bad, matched, covered = 0, [], []
    for n in sorted(defined - stubs, key=lambda n: A.publics.get(A.name(n) or "", 1 << 30)):
        pub = A.name(n)
        if not pub:
            print("MISSING %s: not public in our build (static?)" % n)
            bad += 1
            continue
        at_a, at_b = A.publics[pub], B.publics[pub]
        length = min([s for s in starts if s > at_a] + [ours_end]) - at_a
        seg, off = funcs[n]
        at_o = off if isinstance(seg, str) else seg * 16 + off
        err = compare(n, A, B, at_a, at_b, length, orig, at_o, unit_frame, unit_base, funcs, data)
        covered.append((at_a, length))
        if a.show == n:
            side_by_side(A, at_a, length, orig, at_o)
        if err and n in DRAFTS:
            print("draft   " + err)
            continue
        print("ok      %s (%d bytes)" % (n, length) if not err else "DIFFER  " + err)
        bad += bool(err)
        if not err:
            matched.append(n)
    drafts = sum(1 for n in DRAFTS if n not in matched)
    whole = False
    if u and not bad and not stubs and not drafts:
        whole = A.unit[2] == u["bytes"] and ovl is None and gaps_match(A, B, orig, covered, u)
        print("unit %s %s: ours %d bytes, the unit %d" % (
            u["id"], "complete" if whole else "not complete", A.unit[2], u["bytes"]))
    print("cmatch: %d of %d functions match%s%s" % (
        len(defined - stubs) - bad - drafts, len(defined - stubs),
        ", %d drafts" % drafts if drafts else "", ", %d stubs" % len(stubs) if stubs else ""))
    if a.write and program.name.upper() == "EOB.EXE":
        n = write_levels({funcs[k]: k for k in matched}, "tasm /mx" if asm else " ".join(flags),
                         u["id"] if whole else None)
        print("cmatch: recorded L2 for %d functions in re/symbols.toml" % n)
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())

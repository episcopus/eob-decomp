"""layout.py: which programs a run loads, and how to read their memory in the original's form
(SPEC 8.4, playbook 50; poolrad-decomp's tools/oracle/layout.py, adapted).

Every golden was recorded on the originals in eob/. A rebuild (`make rebuild`, Phase 5) has the same
load image but for the differences playbook 50 accepts (the human, 2026-11-03 and 2026-11-04):

  - the link date, three bytes of `_EXEINFO_`;
  - the overlay stub order: TLINK numbers an overlay module's stub entries from its public order, which
    follows the original function names, so the entries of each stub are the original's in another order,
    and every far pointer to an entry (in data, in resident code, in an overlay's code) names the
    function by its entry offset in that stub;
  - EOB.EXE overlay 5's hook: `gScrollHook = 0xac; g8119 = 0x2d0;` (EOV5.C) has three fixups in the
    original and none in the rebuild.

The oracle has no hooks at the original's addresses (unlike poolrad's), so a run only needs to load the
rebuilt files; the state digest (machine.state_digest, the whole of conventional memory) is what moves.
`Layout.canonical` rewrites a copy of memory into the original's form: each stub's entries back in the
original's order, and each far pointer to a stub entry (offset, then the stub's segment) to the
original's entry for the same function. What the rebuild's file says and the original's does not is
put back where memory still holds the rebuild's value (`file_values`): the link date, and what follows
from overlay 5's three fixups in the overlay data (overlay 5 is 16 bytes shorter there): its stub's
fixup size, overlay 6's stub's record offset (on disk, and as the file offset the overlay manager makes
of it), the overlay manager's copy of the FBOV size. The hook itself is not rewritten: a run that
reaches it shows as a difference (golden.first_difference), a fact about the rebuild, never hidden.

  original()   the pinned eob/EOB.EXE and eob/INTRO.EXE (what every golden is)
  rebuilt()    build/rebuild/eob/eob.exe and build/rebuild/intro/intro.exe, served under their own names
               in the game directory, so the overlay manager (which reads its own file) and INTRO.EXE's
               load of EOB.EXE (INT 21h 4B03h) see the rebuild
"""
from __future__ import annotations

import struct
import sys
from dataclasses import dataclass, field
from pathlib import Path

from .machine import ROOT, LOAD_SEG

GAME = ROOT / "eob"
REBUILD = ROOT / "build" / "rebuild"
PROGRAMS = ("EOB.EXE", "INTRO.EXE")
MEM_BASE = 0x500                    # machine.state_digest hashes 0500..9FFFF


def _fbov():
    sys.path.insert(0, str(ROOT / "tools" / "re"))
    import fbov
    return fbov


@dataclass
class Program:
    """What `canonical` needs of one program: its stubs and where the rebuild's differ."""
    name: str
    exe: Path
    # stub paragraph (load-relative) -> {rebuilt entry offset: original entry offset}, entries that moved
    entry_map: dict[int, dict[int, int]] = field(default_factory=dict)
    # stub paragraph -> number of entries
    entry_count: dict[int, int] = field(default_factory=dict)
    image_len: int = 0
    # what the rebuild's file says and the original's does not, outside stub entries: (load-relative
    # offset, ours, the original's), put back where memory still holds ours (the link date; the stub
    # headers' record offsets and fixup sizes and the overlay manager's copy of the FBOV size, which follow
    # from overlay 5's three fixups)
    file_values: list[tuple[int, bytes, bytes, str]] = field(default_factory=list)


@dataclass
class Layout:
    name: str = "original"
    programs: dict[str, Program] = field(default_factory=dict)
    notes: list[str] = field(default_factory=list)

    @property
    def is_original(self) -> bool:
        return self.name == "original"

    def exe(self, program: str) -> Path:
        p = self.programs.get(program.upper())
        return p.exe if p else GAME / program

    def serve(self, m) -> None:
        """Serve this layout's files under the programs' names in the game directory."""
        if self.is_original:
            return
        for name, p in self.programs.items():
            m.vfs.alias(name, p.exe)

    def canonical(self, m, mem: bytearray) -> None:
        """Rewrite `mem` (machine memory from MEM_BASE) into the original's form, for every image of a
        rebuilt program the run loaded: the program itself (at LOAD_SEG) and each one loaded with
        INT 21h 4B03h (INTRO.EXE loading EOB.EXE: at its relocation factor)."""
        if self.is_original:
            return
        loads = [(Path(m.exe).name.upper(), LOAD_SEG, LOAD_SEG)] + list(getattr(m, "image_loads", []))
        for i, (name, seg, base) in enumerate(loads):
            p = self.programs.get(name)
            if p is None:
                continue
            lo, hi = seg * 16, seg * 16 + p.image_len
            later = loads[i + 1:]
            if any(s * 16 < hi and lo < s * 16 + (self.programs[n].image_len if n in self.programs else 1)
                   for n, s, _f in later):
                continue                     # a later load wrote over this image (drivers: only their start)
            _canonical(p, base, mem)


def _canonical(p: Program, base: int, mem: bytearray) -> None:
    def at(lin: int) -> int:
        return lin - MEM_BASE
    for off, ours, theirs, _why in p.file_values:
        k = at(base * 16 + off)
        if mem[k:k + len(ours)] == ours:
            mem[k:k + len(ours)] = theirs
    for para, moved in p.entry_map.items():
        seg = (base + para) & 0xFFFF
        if mem[at(seg * 16):at(seg * 16) + 2] != b"\xcd\x3f":
            continue                         # not (or no longer) this program's stub
        # the stub's entries, in the original's order
        t = at(seg * 16 + 0x20)
        ents = bytes(mem[t:t + 5 * p.entry_count[para]])
        for ours, theirs in moved.items():
            i, j = (ours - 0x20) // 5, (theirs - 0x20) // 5
            mem[t + 5 * j:t + 5 * j + 5] = ents[5 * i:5 * i + 5]
        # far pointers to its entries: (offset, segment) with the stub's segment
        key = struct.pack("<H", seg)
        k = mem.find(key, 2)
        while k != -1:
            if k >= 2:
                w = mem[k - 2] | (mem[k - 1] << 8)
                if w in moved:
                    mem[k - 2:k] = struct.pack("<H", moved[w])
            k = mem.find(key, k + 1)


def original() -> Layout:
    return Layout(programs={n: Program(n, GAME / n) for n in PROGRAMS})


def _built(name: str) -> Path:
    d = REBUILD / name.split(".")[0].lower()
    for f in d.glob("*"):
        if f.name.upper() == name:
            return f
    raise FileNotFoundError("no rebuilt %s in %s: run `make rebuild PROGRAM=%s` first" % (name, d, name))


def program(name: str, exe: Path, orig: Path | None = None) -> Program:
    """The stubs of `exe` against the original's: TLINK places each stub at the same paragraph (the load
    images are the same size and their segments line up, `make rebuild`), each entry names an offset in
    the overlay's code, and the entries of a stub are the original's in another order."""
    fbov = _fbov()
    from mzutil import read_mz          # tools/re (bytes in), put on the path by _fbov
    orig = orig or GAME / name
    a, b = orig.read_bytes(), exe.read_bytes()
    ma, mb = fbov.manifest(a)[0], fbov.manifest(b)[0]
    p = Program(name, exe)
    if len(ma["overlays"]) != len(mb["overlays"]):
        raise ValueError("%s: %d overlays, the original %d" % (exe, len(mb["overlays"]), len(ma["overlays"])))
    for x, y in zip(ma["overlays"], mb["overlays"]):
        if x["stub_para"] != y["stub_para"] or x["entry_count"] != y["entry_count"]:
            raise ValueError("%s: overlay %d's stub is not the original's (%04X, %d entries; the original "
                             "%04X, %d)" % (exe, x["overlay"], y["stub_para"], y["entry_count"],
                                            x["stub_para"], x["entry_count"]))
        theirs = {e["code_offset"]: 0x20 + 5 * e["index"] for e in x["entries"]}
        if len(theirs) != len(x["entries"]) or sorted(theirs) != sorted(e["code_offset"] for e in y["entries"]):
            raise ValueError("%s: overlay %d's stub entries are not the original's functions" % (exe, x["overlay"]))
        moved = {0x20 + 5 * e["index"]: theirs[e["code_offset"]] for e in y["entries"]
                 if theirs[e["code_offset"]] != 0x20 + 5 * e["index"]}
        p.entry_count[x["stub_para"]] = x["entry_count"]
        if moved:
            p.entry_map[x["stub_para"]] = moved
    # bytes that differ outside the stub entries and far pointers to them: the link date in _EXEINFO_
    # and the stub headers' fields that describe the overlay data (playbook 50)
    ia, ib = read_mz(a)["image"], read_mz(b)["image"]
    if len(ia) != len(ib):
        raise ValueError("%s: load image of %d bytes, the original %d" % (exe, len(ib), len(ia)))
    p.image_len = len(ib)
    rel = {seg * 16 + off for off, seg in read_mz(a)["relocs"]}
    stub_segs = set(p.entry_count)
    header = {q * 16 + k for q in stub_segs for k in range(0x20)}
    entries = {q * 16 + 0x20 + k for q, n in p.entry_count.items() for k in range(5 * n)}
    other = []
    for k in range(len(ia)):
        if ia[k] == ib[k] or k in entries:
            continue
        if k not in header and any(s + 2 in rel and int.from_bytes(ia[s + 2:s + 4], "little") in stub_segs
                                   for s in (k - 1, k)):
            continue
        if other and other[-1][0] + len(other[-1][1]) == k:
            other[-1] = (other[-1][0], other[-1][1] + ib[k:k + 1], other[-1][2] + ia[k:k + 1])
        else:
            other.append((k, ib[k:k + 1], ia[k:k + 1]))
    for k, ours, theirs in other:
        if k not in header:
            p.file_values.append((k, ours, theirs, "the link date (_EXEINFO_)"))
    # a stub header's fields that describe the overlay data, whole: the record's offset in the FBOV data
    # (+04, a dword the overlay manager turns into the file offset at start-up: plus the load module's
    # size and the FBOV header's 16 bytes, seen 2026-11-04) and the code and fixup sizes (+08, +0A)
    body = read_mz(a)["size"] + 16
    if read_mz(b)["size"] + 16 != body:
        raise ValueError("%s: the overlay data starts at %X, the original's at %X" % (exe, read_mz(b)["size"] + 16, body))
    for n, q in enumerate(sorted(stub_segs)):
        for f, fmt, what in ((4, "<I", "record offset"), (8, "<H", "code bytes"), (0xA, "<H", "fixup bytes")):
            x, y = struct.unpack_from(fmt, ia, q * 16 + f)[0], struct.unpack_from(fmt, ib, q * 16 + f)[0]
            if x == y:
                continue
            why = "overlay %d's stub %s" % (n, what)
            p.file_values.append((q * 16 + f, struct.pack(fmt, y), struct.pack(fmt, x), why))
            if f == 4:
                p.file_values.append((q * 16 + f, struct.pack(fmt, y + body), struct.pack(fmt, x + body),
                                      why + " as a file offset"))
    dated = sum(len(o) for k, o, _t in other if k not in header)
    if dated != 3:
        raise ValueError("%s: %d image bytes differ beyond the stubs (the link date is 3): %s" % (
            exe, dated, " ".join("%05X" % k for k, _o, _t in other[:8])))
    # the overlay manager keeps the FBOV header's size (OVRDATA's dword in DGROUP; EOB.EXE ds:84B2)
    fa, fb = read_mz(a)["tail"][4:8], read_mz(b)["tail"][4:8]
    if fa != fb:
        at = ovrdata_size_word(exe)
        p.file_values.append((at, fb, fa, "the overlay manager's copy of the FBOV size"))
    return p


def ovrdata_size_word(exe: Path) -> int:
    """Load-relative address of OVRDATA's dword in DGROUP, from the rebuild's link map: where the
    overlay manager keeps the FBOV size it reads at start-up (EOB.EXE 212E:84B2, seen 2026-11-04: CDC0h
    in the original, CDB0h in the rebuild, the FBOV header's size field of each)."""
    import re
    mp = next((f for f in exe.parent.glob("*") if f.name.upper() == exe.stem.upper() + ".MAP"), None)
    if mp is None:
        raise FileNotFoundError("no link map beside %s" % exe)
    m = re.search(r"^\s*([0-9A-F]{4}):([0-9A-F]{4}) 0004 C=DATA\s+S=_DATA\s+G=DGROUP\s+M=OVRDATA\b",
                  mp.read_text(errors="replace"), re.M)
    if m is None:
        raise ValueError("%s: no OVRDATA dword in DGROUP" % mp)
    return int(m.group(1), 16) * 16 + int(m.group(2), 16)


def rebuilt() -> Layout:
    """The programs `make rebuild` linked (both: INTRO.EXE loads EOB.EXE)."""
    return Layout(name="rebuilt", programs={n: program(n, _built(n)) for n in PROGRAMS})


# --- the active layout ---------------------------------------------------------------------------------
_current: Layout = original()


def current() -> Layout:
    return _current


def use(layout: Layout) -> Layout:
    """Make `layout` the one runs use; returns the previous one."""
    global _current
    prev, _current = _current, layout
    return prev

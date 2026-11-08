#!/usr/bin/env python3
"""introsrc.py: INTRO.EXE's own C units as units of the port (playbook 80, M7, "The intro").

INTRO.EXE links 23 of EOB.EXE's units again (`make twin`: its graphics, input, file and memory routines,
the same object modules at other addresses), so the port does not build them a second time: the intro's
own code, src/I0892.C (its start-up and hand-off) and src/IOV0.C (the playback), runs on the ones the game
already has. tc2c reads a copy of those two files, written here, in which

  - every function they define is named Intro_<name> (Main, StartupGame and others are the game's too), at
    a code segment no EOB address uses (+6000h; the overlay's entries at 6E96:), and their externs of
    functions lose their INTRO.EXE addresses (a call goes by name) but for the library's;
  - a data address the twin units use is the EOB address of the same variable, from the twin proof
    (tools/match/twin.py): the graphics driver's table, the page table, the font, and the rest;
  - any other data address, and every string literal, is the same offset in a segment of the intro's own,
    TC_INTRO_SEG (tcport.h; B000 here, plus the load segment): port/intro.c copies INTRO.EXE's data there.

src/ is not changed.

  introsrc.py OUT_DIR
"""
from __future__ import annotations

import codecs
import re
import shutil
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path[:0] = [str(ROOT / "tools" / "match"), str(ROOT / "tools" / "re")]

CODE = 0x6000           # INTRO code segment + this: no EOB code address
DATA = 0xB000           # tc_load_seg (1000h) + this: TC_INTRO_SEG, C000h
FAR_AT = 0x8000         # far data of other segments, one word apart from here in TC_INTRO_SEG
UNITS = ("I0892", "IOV0")


def intro_dgroup() -> bytes:
    d = (ROOT / "eob" / "INTRO.EXE").read_bytes()
    last, pages, _n, hdr = struct.unpack_from("<HHHH", d, 2)
    h = hdr * 16
    size = pages * 512 - (512 - last if last else 0) - h
    para = struct.unpack_from("<H", d, h + 1)[0]        # C0's first instruction: mov dx, DGROUP
    return d[h + para * 16:h + size]


def ds_map() -> dict[int, int]:
    """INTRO DGROUP offset -> EOB DGROUP offset, for what the twin units address."""
    import twin
    units, funcs = twin.registry()
    E, I = twin.Program("EOB.EXE"), twin.Program("INTRO.EXE")
    m: dict[int, int] = {}
    for i, u in units.items():
        if u["program"] != "INTRO.EXE" or not u.get("twin"):
            continue
        why, seen = twin.check(u["twin"], i, E, I, units, funcs)
        if why:
            raise SystemExit("introsrc: %s is not its twin: %s" % (i, why))
        for (kind, x), y in seen.items():
            if kind in ("disp", "imm") and len(x) == 4:
                m[int.from_bytes(bytes.fromhex(y), "little")] = int.from_bytes(bytes.fromhex(x), "little")
    m[0x1A14] = 0x3074      # gOverlayFiles: RunOverlayRequest (E1C00) reads a name in the request's own segment
    return m


def mapped(m: dict[int, int], keys: list[int], a: int) -> int | None:
    """a's EOB offset: known, or between two near known ones of one displacement (a table)."""
    if a in m:
        return m[a]
    below = [k for k in keys if k < a][-1:]
    above = [k for k in keys if k > a][:1]
    if below and above and above[0] - below[0] <= 0x40 and m[below[0]] - below[0] == m[above[0]] - above[0]:
        return a + m[below[0]] - below[0]
    return None


TOKEN = re.compile(r'/\*.*?\*/|//[^\n]*|"(?:\\.|[^"\\\n])*"(?:\s*"(?:\\.|[^"\\\n])*")*|\'(?:\\.|[^\'\\])*\'', re.S)


def literal_bytes(tok: str) -> bytes:
    out = b""
    for part in re.findall(r'"((?:\\.|[^"\\\n])*)"', tok):
        out += codecs.escape_decode(part.encode("latin-1"))[0]
    return out


def rewrite(src: str, dgroup: bytes, m: dict[int, int], defined: set[str]) -> str:
    keys = sorted(m)
    lits: list[str] = []
    far: dict[str, int] = {}

    def literal(match: re.Match) -> str:
        tok = match.group(0)
        if not tok.startswith('"'):
            return tok
        line_start = src.rfind("\n", 0, match.start()) + 1
        if src[line_start:match.start()].lstrip().startswith("#"):
            return tok
        b = literal_bytes(tok) + b"\0"
        at = dgroup.find(b)
        if at < 0:
            raise SystemExit("introsrc: the literal %r is not in INTRO.EXE's data" % b)
        name = "gIntroLit_%d" % len(lits)
        lits.append("extern char far %s[]; /*@ %04X:%04X */\n" % (name, DATA, at))
        return name

    text = TOKEN.sub(literal, src)

    def annot(match: re.Match) -> str:
        kind = match.group(1)
        if kind.lower().startswith("ovl"):                    # the overlay's functions
            return "/*@ %04X:%s" % (0x6E96, match.group(2))
        if kind.lower() == "ds":
            a = int(match.group(2), 16)
            e = mapped(m, keys, a)
            return "/*@ ds:%04X" % e if e is not None else "/*@ %04X:%04X" % (DATA, a)
        return match.group(0)

    text = re.sub(r"/\*@ *(ovl0|ds):([0-9A-Fa-f]{4})", annot, text)

    out = []
    for line in text.split("\n"):
        ann = re.search(r"/\*@ *([0-9A-Fa-f]{4}):([0-9A-Fa-f]{4})", line)
        if ann and not line.lstrip().startswith("/*@flags"):
            seg, off = int(ann.group(1), 16), ann.group(2)
            before = line[:ann.start()]
            if seg not in (DATA, 0x6E96):
                if before.lstrip().startswith("extern"):
                    if re.search(r"\b\w+\s*\(", before) and "(far *" not in before:
                        if seg != 0:                          # a call by name; the library keeps its mark
                            line = line[:ann.start()] + "/* " + line[ann.start() + 3:]
                    else:                                     # far data of another segment
                        at = far.setdefault(ann.group(0), FAR_AT + 2 * len(far))
                        line = line[:ann.start()] + "/*@ %04X:%04X" % (DATA, at) + line[ann.end():]
                else:                                         # a definition
                    line = line[:ann.start()] + "/*@ %04X:%s" % (seg + CODE, off) + line[ann.end():]
        out.append(line)
    text = "\n".join(out)
    text = re.sub(r"^extern int Ovl0Main\(.*$", "", text, flags=re.M)
    for intro, game in (("ClipRect", "CopyRegion"), ("CloseFileHandle", "close")):      # the same routines (twin e19f3; the library)
        text = re.sub(r"\b%s\b" % intro, game, text)
    text = text.replace("Ovl0Main(argc, argv)", "Intro_RunIntro()")
    for name in sorted(defined):
        text = re.sub(r"\b%s\b" % re.escape(name), "Intro_" + name, text)
    return "".join(lits) + text


def main(argv=None) -> int:
    argv = argv if argv is not None else sys.argv[1:]
    out = Path(argv[0])
    out.mkdir(parents=True, exist_ok=True)
    import cmatch
    dgroup = intro_dgroup()
    m = ds_map()
    for u in UNITS:
        src = (ROOT / "src" / (u + ".C")).read_text(encoding="latin-1")
        _f, _d, defined, _s = cmatch.parse_annotations(src)
        (out / (u + ".C")).write_text(rewrite(src, dgroup, m, defined), encoding="latin-1")
    shutil.copyfile(ROOT / "src" / "EOB.H", out / "EOB.H")
    return 0


if __name__ == "__main__":
    sys.exit(main())

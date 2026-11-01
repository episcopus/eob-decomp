#!/usr/bin/env python3
"""disasm.py: annotated disassembly of a range of a program's load module.

Far calls are shown as seg:off with the target's function index; DS offsets of string literals
are decoded from DGROUP.

  disasm.py EXE SEG [--start OFF] [--len N] [--dgroup 212E]
"""
from __future__ import annotations

import argparse, json, os, re, struct, sys

from capstone import CS_ARCH_X86, CS_MODE_16, Cs

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from mzutil import read_mz  # noqa: E402

MD = Cs(CS_ARCH_X86, CS_MODE_16)


def cstring(img: bytes, at: int) -> str | None:
    end = img.find(b"\0", at, at + 80)
    s = img[at:end] if end > at else b""
    if len(s) >= 3 and all(32 <= c < 127 or c in (9, 10, 13) for c in s):
        return s.decode("latin-1").encode("unicode_escape").decode()
    return None


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("exe")
    ap.add_argument("seg")
    ap.add_argument("--start", type=lambda s: int(s, 16), default=0)
    ap.add_argument("--len", type=int, default=None)
    ap.add_argument("--dgroup", type=lambda s: int(s, 16), default=0x212E)
    a = ap.parse_args()
    mz = read_mz(open(a.exe, "rb").read())
    img, relocs = mz["image"], {s * 16 + o for o, s in mz["relocs"]}
    stem = os.path.splitext(os.path.basename(a.exe))[0].lower()
    man = os.path.join("build", "re", stem + ".json")
    dgroup_img = img
    if a.seg.lower().startswith("ovl"):
        n = int(a.seg[3:])
        img = open(os.path.join("build", "re", stem + "-overlays", "ovl%02d.bin" % n), "rb").read()
        relocs = set(json.load(open(man))["overlays"][n]["fixups"])
        a.seg = 0
        man = ""
    else:
        a.seg = int(a.seg, 16)
    seg_end = None
    if os.path.exists(man):
        for s in json.load(open(man))["segments"]:
            if s["para"] == a.seg and s["kind"] == 1 and s["size"]:
                seg_end = s["start"] + s["size"] - a.seg * 16
    base = a.seg * 16
    end = a.start + a.len if a.len else (seg_end or a.start + 256)
    for i in MD.disasm(img[base + a.start:base + end], a.start):
        note = []
        lin = base + i.address
        if any(lin + k in relocs for k in range(i.size)):
            note.append("reloc")
        for m in re.finditer(r"0x([0-9a-f]+)", i.op_str):
            v = int(m.group(1), 16)
            s = cstring(dgroup_img, a.dgroup * 16 + v) if v >= 0x10 else None
            if s and ("[" in i.op_str or i.mnemonic in ("mov", "push")):
                note.append('ds:%04x "%s"' % (v, s))
        if i.mnemonic in ("ret", "retf"):
            note.append("-----")
        print("%04X:%04X  %-16s %-6s %-28s %s" % (a.seg, i.address, i.bytes.hex(), i.mnemonic, i.op_str, " ".join(note)))
    return 0


if __name__ == "__main__":
    sys.exit(main())

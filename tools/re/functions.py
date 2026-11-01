#!/usr/bin/env python3
"""functions.py: enumerate the functions of a program's code segments (playbook 10).

Evidence for a function start, strongest first:
  ovl     an entry of an overlay stub (the overlay manager calls it)
  far     the target of a far call (9A, segment word relocated) or of a far pointer pushed
          as two immediates (push seg relocated, push off)
  near    the target of a near call (E8) from code already known to be a function
  frame   a Borland prologue (55 8B EC: push bp; mov bp,sp) right after a return
  segment the start of a code segment (one module each, except segment 0)
A function's size is the distance to the next start in the same segment (it includes padding
and any data between the two); `make match` corrects it once the function is matched.

  functions.py EXE MANIFEST.json [-o build/re/<prog>-functions.json]
"""
from __future__ import annotations

import argparse, json, os, struct, sys
from collections import defaultdict

from capstone import CS_ARCH_X86, CS_MODE_16, Cs

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from mzutil import read_mz  # noqa: E402

MD = Cs(CS_ARCH_X86, CS_MODE_16)
RANK = {"ovl": 0, "far": 1, "near": 2, "frame": 3, "segment": 4}


def code_ranges(manifest: dict) -> list[tuple[int, int, int]]:
    """(start, end, frame paragraph) of every non-empty code segment, linear in the load module."""
    return [(s["start"], s["start"] + s["size"], s["para"]) for s in manifest["segments"]
            if s["kind"] == 1 and s["size"]]


def scan(mz: dict, manifest: dict) -> dict:
    img, relocs = mz["image"], {s * 16 + o for o, s in mz["relocs"]}
    ranges = code_ranges(manifest)
    seg_of = {}
    for a, b, para in ranges:
        seg_of[para] = seg_of.get(para, []) + [(a, b)]

    def in_code(lin):
        return any(a <= lin < b for a, b, _ in ranges)

    starts = defaultdict(set)
    for a, b, para in ranges:
        starts[a].add("segment")
    # far calls and far pointers: 9A off seg (seg relocated), 68 seg ... (push imm16 relocated)
    for a, b, _ in ranges:
        for i in MD.disasm(img[a:b], a):
            if i.bytes[0] == 0x9A and i.address + 3 in relocs:
                off, seg = struct.unpack_from("<HH", i.bytes, 1)
                t = seg * 16 + off
                if in_code(t):
                    starts[t].add("far")
            elif i.mnemonic == "call" and i.bytes[0] == 0xE8:
                t = i.address + i.size + struct.unpack_from("<h", i.bytes, 1)[0]
                if a <= t < b:
                    starts[t].add("near")
            elif i.mnemonic == "ret" or i.mnemonic == "retf":
                n = i.address + i.size
                if n + 3 <= b and img[n:n + 3] == b"\x55\x8b\xec":
                    starts[n].add("frame")
    # overlays: entries are offsets into the overlay's own code
    ovl = []
    for o in manifest["overlays"]:
        for e in o["entries"]:
            ovl.append({"overlay": o["overlay"], "offset": e["code_offset"], "entry": e["index"]})
    return {"resident": starts, "ranges": ranges, "overlay_entries": ovl}


def overlay_functions(manifest: dict, ovl_dir: str) -> list[dict]:
    out = []
    for o in manifest["overlays"]:
        code = open(os.path.join(ovl_dir, "ovl%02d.bin" % o["overlay"]), "rb").read()
        st = defaultdict(set)
        st[0].add("segment")
        for e in o["entries"]:
            st[e["code_offset"]].add("ovl")
        for i in MD.disasm(code, 0):
            if i.bytes[0] == 0xE8:
                t = i.address + i.size + struct.unpack_from("<h", i.bytes, 1)[0]
                if 0 <= t < len(code):
                    st[t].add("near")
            elif i.mnemonic in ("ret", "retf"):
                n = i.address + i.size
                if code[n:n + 3] == b"\x55\x8b\xec":
                    st[n].add("frame")
        keys = sorted(st)
        for k, n in zip(keys, keys[1:] + [len(code)]):
            out.append({"overlay": o["overlay"], "offset": k, "size": n - k,
                        "evidence": sorted(st[k], key=RANK.get)})
    return out


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("exe")
    ap.add_argument("manifest")
    ap.add_argument("-o", "--out")
    a = ap.parse_args()
    mz = read_mz(open(a.exe, "rb").read())
    m = json.load(open(a.manifest))
    s = scan(mz, m)
    funcs = []
    for start, end, para in s["ranges"]:
        keys = sorted(k for k in s["resident"] if start <= k < end)
        for k, n in zip(keys, keys[1:] + [end]):
            funcs.append({"linear": k, "seg": para, "offset": k - para * 16, "size": n - k,
                          "evidence": sorted(s["resident"][k], key=RANK.get)})
    ovl_dir = os.path.splitext(a.manifest)[0] + "-overlays"
    ofuncs = overlay_functions(m, ovl_dir) if m["overlays"] else []
    out = {"program": os.path.basename(a.exe), "resident": funcs, "overlay": ofuncs}
    if a.out:
        json.dump(out, open(a.out, "w"), indent=1)
    by = defaultdict(int)
    for f in funcs + ofuncs:
        by[f["evidence"][0]] += 1
    print("%s: %d resident and %d overlay functions (first evidence: %s)" % (
        out["program"], len(funcs), len(ofuncs), ", ".join("%s %d" % (k, by[k]) for k in sorted(by, key=RANK.get))))
    return 0


if __name__ == "__main__":
    sys.exit(main())

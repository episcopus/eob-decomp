#!/usr/bin/env python3
"""rebuild.py: build a program from src/ with the pinned toolchain and compare it with the original
(Phase 5, playbook 50).

  rebuild.py START1.EXE [-v]

The units of the program (re/units.toml, not its start-up code or runtime library) are compiled
and assembled from working copies of their files in build/rebuild/<program>/, then linked by TLINK
with the start-up code and the library of the program's memory model, in address order: the
original link order, since each module follows the one before it. Two things only a full link
needs, the copies get here and src/ does not:

  - one name per address. `make match` ties everything by address, so a C file may call an
    assembly routine `PutChar` that the assembly defines as `PutChar` (C looks for `_PutChar`), and
    the function the start-up code calls is whatever its file named it. In the copies every address
    gets one name: its defining file's, `_` added in assembly for a C caller, `main` for the
    function the start-up code calls (rename.py rewrites the copies, by address);
  - the data. src/ declares every global as an extern at its DGROUP offset (`/*@ ds:00A8 */`), so
    nothing defines one. A generated module, DATA.ASM, holds the original's bytes of the program's
    own data (between the start-up code's and the library's), with a public at each declared
    offset, and reserves its uninitialised data (_BSS). A probe link finds the library's data in
    the original by its bytes, which bounds the program's.

Then the load image and the relocation table are compared with the original's, and every range
that differs is reported with the unit or region it lies in; when they agree, the whole file
(header included) is compared too. Exit code 0 when the image and the relocations are identical.
The working files stay in build/rebuild/<program>/ (OUT.EXE, OUT.MAP, DATA.ASM).

Supported so far: START1.EXE (small model, no overlays; identical 2026-11-03). EOB.EXE and
INTRO.EXE need the large model's link (C0L, CL.LIB, OVERLAY.LIB) and VROOMM overlays (playbook 50).
"""
from __future__ import annotations

import argparse
import json
import re
import shutil
import sys
import tomllib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools" / "re"))
sys.path.insert(0, str(ROOT / "tools" / "match"))
import tc_toolchain as tc  # noqa: E402
from mzutil import read_mz  # noqa: E402
import cmatch  # noqa: E402
import rename  # noqa: E402

MODEL = {"START1.EXE": "S"}                     # the small model: C0S and CS.LIB (whitepaper 2.6)


def units_of(program: str) -> list[dict]:
    us = tomllib.loads((ROOT / "re/units.toml").read_text())["unit"]
    us = [u for u in us if u["program"] == program and u["kind"] not in ("startup", "library")]
    return sorted(us, key=lambda u: cmatch_linear(u["start"]))


def cmatch_linear(addr: str) -> int:
    s, o = addr.split(":")
    return int(s, 16) * 16 + int(o, 16)


def sources(program: str) -> list[Path]:
    """The program's source files in link order: by the lowest address each one defines."""
    files = []
    for u in units_of(program):
        for p in sorted((ROOT / "src").glob(u["id"].upper() + ".*")):
            if p.suffix.upper() in (".C", ".ASM"):
                asm = p.suffix.upper() == ".ASM"
                funcs, _, defined, stubs = cmatch.parse_annotations(p.read_text(), asm)
                own = [funcs[n] for n in defined - stubs]
                if own:
                    files.append((min(cmatch_linear("%04X:%04X" % (s, o)) for s, o in own), p))
    return [p for _, p in sorted(files)]


def copies(program: str, w: Path, main_addr: str) -> list[Path]:
    """Working copies M1.C, M2.ASM ... with one name per address."""
    out = []
    for i, p in enumerate(sources(program), 1):
        q = w / ("M%d%s" % (i, p.suffix.upper()))
        q.write_text(p.read_text())
        out.append(q)
    # each address's name: its defining file's (main for the start-up code's callee)
    names, c_callers = {}, set()
    for q in out:
        asm = q.suffix == ".ASM"
        funcs, _, defined, stubs = cmatch.parse_annotations(q.read_text(), asm)
        for n in defined - stubs:
            a = rename.norm("%04X:%04X" % funcs[n])
            names[a] = "main" if a == main_addr else (n[1:] if asm and n.startswith("_") else n)
        if not asm:
            c_callers.update(rename.norm("%04X:%04X" % funcs[n]) for n in funcs)
    for q in out:
        asm = q.suffix == ".ASM"
        text = q.read_text()
        for n, a in rename.bindings(q).items():
            if a not in names or a.startswith("ds:"):
                continue
            want = names[a]
            if asm:
                want = ("_" + want) if (a in c_callers or n.startswith("_")) else want
            if n != want:
                text, _ = rename.replace_word(text, n, want, asm)
        q.write_text(text.replace("\r\n", "\n").replace("\n", "\r\n"))
    return out


def data_names(files: list[Path]) -> dict[int, list[str]]:
    """DGROUP offset -> the public names the copies use for it (`_x` for C, as written in assembly)."""
    out: dict[int, list[str]] = {}
    for q in files:
        asm = q.suffix == ".ASM"
        _, data, _, _ = cmatch.parse_annotations(q.read_text(), asm)
        for n, off in data.items():
            sym = n if asm else "_" + n
            if sym not in out.setdefault(off, []):
                out[off].append(sym)
    return out


def data_module(names: dict[int, list[str]], image: bytes, dgroup: int, data: tuple, bss: tuple) -> str:
    """DATA.ASM: the original bytes of [data) with a public at each named offset; [bss) reserved."""
    # explicit data segments only: a simplified .MODEL would add an empty word-aligned _TEXT, and
    # TLINK would pad the code before the next module to an even address
    pub = sorted(o for o in names if data[0] <= o < data[1] or bss[0] <= o < bss[1])
    lines = ["DGROUP GROUP _DATA, _BSS"] + ["PUBLIC %s" % s for o in pub for s in names[o]]
    lines.append("_DATA SEGMENT WORD PUBLIC 'DATA'")

    def block(lo, hi, init):
        pos, body = lo, []
        for o in [o for o in pub if lo <= o < hi] + [hi]:
            if o > pos:
                if init:
                    chunk = image[dgroup * 16 + pos:dgroup * 16 + o]
                    for k in range(0, len(chunk), 16):
                        body.append("db " + ",".join("%03Xh" % b for b in chunk[k:k + 16]))
                else:
                    body.append("db %d dup (?)" % (o - pos))
            if o < hi:
                body += ["%s label byte" % s for s in names[o]]
            pos = o
        return body
    lines += block(*data, True) + ["_DATA ENDS", "_BSS SEGMENT WORD PUBLIC 'BSS'"]
    lines += block(*bss, False) + ["_BSS ENDS", "END"]
    return "\r\n".join(lines) + "\r\n"


def link(program: str, w: Path, files: list[Path], data_asm: str | None) -> tuple[bytes, set, str]:
    model = MODEL.get(program, "L")
    objs = []
    for q in files:
        if q.suffix == ".C":
            flags = (cmatch.FLAGS.search(q.read_text()).group(1).split() if cmatch.FLAGS.search(q.read_text())
                     else tc.manifest()["cflags"].split())
            r = tc.run("tc100", "TCC", ["-c", "-ID:\\INCLUDE"] + flags + [q.name], cwd=w)
        else:
            r = tc.run(tc.manifest()["pinned_asm"], "TASM", ["/mx", q.name], cwd=w)
        if not (w / (q.stem + ".OBJ")).exists() and not (w / (q.stem.lower() + ".obj")).exists():
            raise SystemExit("rebuild: %s does not build:\n%s" % (q.name, r.stdout.decode("latin-1")[-1500:]))
        objs.append(q.stem)
    if data_asm is not None:
        (w / "DATA.ASM").write_text(data_asm)
        r = tc.run(tc.manifest()["pinned_asm"], "TASM", ["/mx", "DATA.ASM"], cwd=w)
        if "Error" in r.stdout.decode("latin-1").split("Error messages:")[-1][:12]:
            raise SystemExit("rebuild: DATA.ASM: %s" % r.stdout.decode("latin-1")[-800:])
        objs.insert(0, "DATA")
    for f in ("OUT.EXE", "OUT.MAP"):
        (w / f).unlink(missing_ok=True)
    r = tc.run("tc100", "TLINK", ["/c", "/m", "/s", "D:\\LIB\\C0%s+%s,OUT,OUT,D:\\LIB\\C%s" % (model, "+".join(objs), model)], cwd=w)
    log = r.stdout.decode("latin-1")
    exe = w / "OUT.EXE" if (w / "OUT.EXE").exists() else w / "out.exe"
    if not exe.exists() or "Error" in log:
        return b"", set(), log
    mz = read_mz(exe.read_bytes())
    return mz["image"], {s * 16 + o for o, s in mz["relocs"]}, log


def find(hay: bytes, needle: bytes, start: int = 0) -> int:
    return hay.find(needle, start) if needle else -1


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("program")
    ap.add_argument("-v", "--verbose", action="store_true")
    a = ap.parse_args(argv)
    program = a.program.upper()
    orig = read_mz((ROOT / "eob" / program).read_bytes())
    oimg, orel = orig["image"], {s * 16 + o for o, s in orig["relocs"]}
    dg = cmatch.dgroup_of(ROOT / "eob" / program)
    w = ROOT / "build" / "rebuild" / program.split(".")[0].lower()
    shutil.rmtree(w, ignore_errors=True)
    w.mkdir(parents=True)
    main_addr = rename.norm(min(u["start"] for u in units_of(program)))   # the first unit's first function
    files = copies(program, w, main_addr)
    names = data_names(files)
    # the probe: every global defined as a zero-size label, so the library's data falls right after
    # the start-up code's; its bytes, found in the original, bound the program's own data
    syms = [s for o in sorted(names) for s in names[o]]
    probe = "\r\n".join(["DGROUP GROUP _DATA, _BSS"] + ["PUBLIC %s" % s for s in syms] +
                          ["_DATA SEGMENT WORD PUBLIC 'DATA'", "_DATA ENDS", "_BSS SEGMENT WORD PUBLIC 'BSS'"] +
                          ["%s label byte" % s for s in syms] + ["_BSS ENDS", "END"]) + "\r\n"
    pimg, _, log = link(program, w, files, probe)
    if not pimg:
        print(log[-2500:])
        return 1
    pmap = (w / "OUT.MAP").read_text("latin-1") if (w / "OUT.MAP").exists() else (w / "out.map").read_text("latin-1")
    pdg = int(re.search(r"^\s*([0-9A-F]{4}):[0-9A-F]{4} [0-9A-F]{4} C=DATA\s+S=_DATA\s+G=DGROUP", pmap, re.M).group(1), 16)
    seg = {}
    for m in re.finditer(r"^\s*([0-9A-F]{4}):([0-9A-F]{4}) ([0-9A-F]{4}) C=(\w+)\s+S=(\S+)\s+G=(\S+)\s+M=(\S+)", pmap, re.M):
        fr, off, ln = (int(m.group(i), 16) for i in (1, 2, 3))
        seg.setdefault((m.group(5), m.group(7).upper()), (fr * 16 + off - pdg * 16, ln))
    c0_data = sum(ln for (s, mod), (o, ln) in seg.items() if s == "_DATA" and mod == "C0.ASM")
    lib_data = sorted((o, ln, mod) for (s, mod), (o, ln) in seg.items() if s == "_DATA" and ln and o >= c0_data
                      and not mod.startswith(("M", "DATA")))
    # the shift of the library's data: pieces of it found once in the original agree on one distance
    # (pointers in it move with it, so not every piece is found; short ones are found anywhere)
    o0 = lib_data[0][0]
    o1 = max(o + ln for o, ln, _ in lib_data)
    pdata, odata = pimg[pdg * 16:pdg * 16 + o1], oimg[dg * 16:]
    from collections import Counter
    votes = Counter()
    for k in range(o0, o1 - 32, 8):
        piece = pdata[k:k + 32]
        if len(set(piece)) < 6:
            continue
        hits = [m.start() for m in re.finditer(re.escape(piece), odata)]
        if len(hits) == 1 and hits[0] >= k:
            votes[hits[0] - k] += 1
    if not votes:
        print("rebuild: the library's data is not in the original's DGROUP")
        return 1
    shift, n = votes.most_common(1)[0]
    at = o0 + shift
    if a.verbose:
        print("rebuild: library data shifted by %04X (%d of %d pieces agree)" % (shift, n, sum(votes.values())))
    image_end = len(oimg) - dg * 16
    bss_names = [o for o in names if o >= image_end]
    bss = (image_end, max(bss_names) + 4 if bss_names else image_end)
    final = data_module(names, oimg, dg, (c0_data, at), bss)
    img, rel, log = link(program, w, files, final)
    if not img:
        print(log[-2500:])
        return 1
    print("rebuild: %s from %d files, data %04X to %04X from the original, uninitialised %04X to %04X" % (
        program, len(files), c0_data, at, bss[0], bss[1]))
    rc = compare(program, img, rel, oimg, orel, dg, a.verbose)
    ours = (w / "OUT.EXE") if (w / "OUT.EXE").exists() else (w / "out.exe")
    if rc == 0 and ours.read_bytes() == (ROOT / "eob" / program).read_bytes():
        print("rebuild: %s: the whole file is identical, header included (%d bytes)" % (program, len(ours.read_bytes())))
    elif rc == 0:
        print("rebuild: %s: the load image and relocations are identical; the EXE header differs" % program)
    return rc


def compare(program, img, rel, oimg, orel, dg, verbose) -> int:
    units = sorted(tomllib.loads((ROOT / "re/units.toml").read_text())["unit"], key=lambda u: u.get("start", ""))
    regions = [(cmatch_linear(u["start"]), cmatch_linear(u["start"]) + u["bytes"], u["id"]) for u in units
               if u["program"] == program and ":" in u["start"] and not u["start"].startswith("ovl")]

    def where(lin):
        if lin >= dg * 16:
            return "DGROUP ds:%04X" % (lin - dg * 16)
        return next(("%s +%04X" % (r, lin - a) for a, b, r in regions if a <= lin < b), "%05X" % lin)
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
    for a, b in diffs[:40 if verbose else 12]:
        print("  differs %05X-%05X (%d bytes) at %s: ours %s, original %s" % (
            a, b, b - a, where(a), img[a:min(b, a + 8)].hex(" "), oimg[a:min(b, a + 8)].hex(" ")))
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

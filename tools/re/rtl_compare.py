#!/usr/bin/env python3
"""rtl_compare.py: which Turbo C++ release's runtime library is in a program (whitepaper 2.1).

For each release and memory model: link one program that references every module of the
model's C library (and OVERLAY.LIB), read TLINK's map for each module's code, then look for
each module in the target program. A module is found when every instruction is the same
except address fields (displacements, immediates, call and jump targets), which differ
between two links. The release whose modules are all found is the one that built the program.

  rtl_compare.py [--target eob/EOB.EXE] [--model l] [--releases tc100,tc101] [-o build/re/rtl_compare.json]
"""
from __future__ import annotations

import argparse, json, os, re, shutil, sys
from pathlib import Path

from capstone import CS_ARCH_X86, CS_MODE_16, Cs

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from mzutil import read_mz  # noqa: E402
import tc_toolchain as tc  # noqa: E402

ROOT = tc.ROOT
WORK = ROOT / "build" / "re" / "rtl"
MODEL_LIB = {"s": "CS", "c": "CC", "m": "CM", "l": "CL", "h": "CH"}
MATH_LIB = {"s": "MATHS", "c": "MATHC", "m": "MATHM", "l": "MATHL", "h": "MATHH"}
IDENT = re.compile(r"^[A-Za-z_@$?][A-Za-z0-9_@$?]*$")
MD = Cs(CS_ARCH_X86, CS_MODE_16)
MD.detail = True


def publics_by_module(lst: str) -> dict[str, list[str]]:
    mods, cur = {}, None
    for line in lst.splitlines():
        m = re.match(r"^(\S+)\s+size = (\d+)", line)
        if m:
            cur = m.group(1)
            mods[cur] = []
        elif cur and line.startswith("\t"):
            mods[cur] += [n for n in re.split(r"\s{2,}", line.strip()) if IDENT.match(n)]
    return mods


def build(release: str, model: str) -> tuple[Path, dict]:
    """Link the reference program; return its directory and the publics referenced per module."""
    w = WORK / release / model
    shutil.rmtree(w, ignore_errors=True)
    w.mkdir(parents=True)
    lib = MODEL_LIB[model]
    mods = {}
    for name in (lib, "OVERLAY"):
        tc.run(release, "TLIB", ["D:\\LIB\\%s.LIB" % name, ",%s.LST" % name], cwd=w)
        for mod, pubs in publics_by_module((w / ("%s.LST" % name)).read_text("latin-1")).items():
            if pubs:
                mods[mod] = pubs[0]
    asm = ["IDEAL", "MODEL LARGE", "CODESEG"]
    asm += ["EXTRN %s:FAR" % p for p in sorted(set(mods.values()))]
    asm += ["DD %s" % p for p in sorted(set(mods.values()))]
    asm += ["END"]
    (w / "REFS.ASM").write_text("\r\n".join(asm) + "\r\n")
    (w / "M.C").write_text("int main(void) { return 0; }\r\n")
    tasm = "tasm20" if (tc.TOOLCHAIN / "tasm20").exists() else "tasm201"
    r = tc.run(tasm, "TASM", ["/mx", "REFS.ASM"], cwd=w)
    if not (w / "REFS.OBJ").exists():
        raise SystemExit("rtl_compare: TASM failed:\n" + r.stdout.decode("latin-1")[-800:])
    tc.run(release, "TCC", ["-m" + model, "-c", "-ID:\\INCLUDE", "M.C"], cwd=w)
    libs = "D:\\LIB\\OVERLAY+D:\\LIB\\EMU+D:\\LIB\\%s+D:\\LIB\\%s" % (MATH_LIB[model], lib)
    r = tc.run(release, "TLINK", ["/c", "/s", "D:\\LIB\\C0%s" % model.upper() + "+M+REFS,R,R," + libs], cwd=w)
    if not (w / "R.EXE").exists():
        raise SystemExit("rtl_compare: TLINK failed:\n" + r.stdout.decode("latin-1")[-1500:])
    return w, mods


def module_code(w: Path) -> list[tuple[str, int, int]]:
    """(module, linear start, length) of every code contribution in TLINK's detailed map."""
    out = []
    for line in (w / "R.MAP").read_text("latin-1").splitlines():
        m = re.match(r"\s*([0-9A-F]{4}):([0-9A-F]{4}) ([0-9A-F]{4}) C=CODE\s+S=\S+\s+G=\S+\s+M=(\S+)", line)
        if m and int(m.group(3), 16):
            out.append((m.group(4).upper(), int(m.group(1), 16) * 16 + int(m.group(2), 16), int(m.group(3), 16)))
    return out


def same_insn(a, b) -> bool:
    if a.bytes == b.bytes:
        return True
    if a.size != b.size or a.mnemonic != b.mnemonic:
        return False
    cut = [o for o in (a.disp_offset, a.imm_offset) if o] or [a.size]
    return a.bytes[:min(cut)] == b.bytes[:min(cut)]


def matches(code: bytes, target: bytes, at: int) -> bool:
    ours = list(MD.disasm(code, 0))
    if sum(i.size for i in ours) != len(code):
        return False  # data inside the module: fall back to the decoded prefix only
    theirs = list(MD.disasm(target[at:at + len(code) + 16], 0))
    return len(theirs) >= len(ours) and all(same_insn(a, b) for a, b in zip(ours, theirs))


def fixed_bytes(code: bytes) -> list[int]:
    """Offsets of the bytes that are not address fields, as decoded from our side."""
    keep = []
    for i in MD.disasm(code, 0):
        cut = min([o for o in (i.disp_offset, i.imm_offset) if o] or [i.size])
        keep += range(i.address, i.address + cut)
    return keep


def find(code: bytes, target: bytes, limit: int) -> int | None:
    keep = fixed_bytes(code)
    probe = [(k, code[k]) for k in keep[:48]]  # cheap filter before the full comparison
    key = code[:4] if keep[:4] == [0, 1, 2, 3] else code[:1]
    p = target.find(key, 0, limit)
    while p >= 0:
        if all(target[p + k] == v for k, v in probe if p + k < len(target)) and matches(code, target, p):
            return p
        p = target.find(key, p + 1, limit)
    return None


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--target", default=str(ROOT / "eob" / "EOB.EXE"))
    ap.add_argument("--model", default="l")
    ap.add_argument("--releases", default="tc100,tc101")
    ap.add_argument("-o", "--out", default=str(ROOT / "build" / "re" / "rtl_compare.json"))
    a = ap.parse_args()
    target = read_mz(open(a.target, "rb").read())["image"]
    limit = len(target)
    report = {}
    for rel in a.releases.split(","):
        w, _ = build(rel, a.model)
        image = read_mz((w / "R.EXE").read_bytes())["image"]
        found, missing = {}, []
        for mod, start, length in module_code(w):
            at = find(image[start:start + length], target, limit)
            if at is None:
                missing.append(mod)
            else:
                found[mod] = at
        report[rel] = {"model": a.model, "found": found, "missing": missing}
        print("%s -m%s: %d of %d library modules found in %s" % (
            rel, a.model, len(found), len(found) + len(missing), os.path.basename(a.target)))
    Path(a.out).parent.mkdir(parents=True, exist_ok=True)
    json.dump(report, open(a.out, "w"), indent=1)
    return 0


if __name__ == "__main__":
    sys.exit(main())

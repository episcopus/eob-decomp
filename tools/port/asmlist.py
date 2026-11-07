#!/usr/bin/env python3
"""asmlist.py: the data the assembly units keep in their code segments, for the port's image
(port/asmdata.json; playbook 80, M5).

The hand-written C of port/asm reads its unit's tables and variables where the original has them, in
the unit's code segment (E1AAD's key tables at 1AAD:0186, E1D8F's saved vector at 1D8F:0002, ...).
The port's image is generated from src/ (decision 6), so these bytes come from the assembly source:
TASM assembles each unit with the matcher's switches (`/mx`) and a listing (`/l`); every `db`, `dw`
and `dd` line of the unit's code segment gives its offset and its bytes there, and the unit's code
begins at its start in re/units.toml (1AAD:000C: TASM's offset 0070h of kKeyBuf is 1AAD:007C). Each
run of data lines is checked against EOB.EXE's load module before it is written; a word whose value
TASM marks relocatable within the segment (`r`) has the unit's start added, as the linker does.

port/asmdata.json is committed, so `make port` needs no assembler (as port/literals.json); make
port-asmdata writes it again and needs Turbo Assembler (docs/setup/SETUP-tc.md).

  asmlist.py [--out port/asmdata.json] [UNIT.ASM ...]   (default: every assembly unit of EOB.EXE with
                                                        C in port/asm)
"""
from __future__ import annotations

import argparse
import json
import re
import shutil
import sys
import tempfile
import tomllib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools" / "re"))
from mzutil import read_mz  # noqa: E402

DATA = re.compile(r"^(?:\w+\s+)?(db|dw|dd)\b", re.I)


def units() -> dict[str, dict]:
    return {u["id"]: u for u in tomllib.loads((ROOT / "re/units.toml").read_text())["unit"] if u["program"] == "EOB.EXE"}


def listing(src: Path) -> list[str]:
    import tc_toolchain as tc
    with tempfile.TemporaryDirectory() as tmp:
        w = Path(tmp)
        shutil.copyfile(src, w / src.name.upper())
        r = tc.run(tc.manifest()["pinned_asm"], "TASM", ["/mx", "/l", src.name.upper()], cwd=w)
        lst = next((p for p in w.iterdir() if p.suffix.lower() == ".lst"), None)
        if lst is None or b"Error messages:    None" not in r.stdout:
            raise SystemExit("%s: TASM failed:\n%s" % (src.name, r.stdout.decode("latin-1")))
        return lst.read_text(errors="replace").split("\n")


def object_bytes(field: str, base: int) -> bytes:
    """The bytes of a listing's object-code field: `40 4B 4C` bytes, `0110` a word, `00000000` a
    doubleword (their values, high digits first), `0100*(00)` a repeat, `0012r` relocatable."""
    out = bytearray()
    for tok in field.replace("+", " ").split():
        m = re.fullmatch(r"([0-9A-F]+)\*\(([0-9A-F ]+)\)", tok)
        if m:
            out += bytes.fromhex(m.group(2).replace(" ", "")) * int(m.group(1), 16)
            continue
        m = re.fullmatch(r"([0-9A-F]+)(r?)", tok)
        if not m:
            raise ValueError(tok)
        v, n = int(m.group(1), 16), len(m.group(1)) // 2
        if m.group(2):
            v += base
        out += (v & ((1 << (8 * n)) - 1)).to_bytes(n, "little")
    return bytes(out)


def code_data(src: Path, unit: dict) -> list[tuple[int, int, bytes]]:
    seg = int(unit["segment"], 16)
    base = int(unit["start"].split(":")[1], 16)
    # the listing's columns, tabs expanded: the line number, the offset at 8, the object code from 14,
    # the source from 37; a long object field goes on below with `+` at its end
    lines = [l.expandtabs(8) for l in listing(src)]
    runs: list[list] = []
    in_code = False
    i = 0
    while i < len(lines):
        l = lines[i]
        i += 1
        if not re.match(r"\s*\d+ ", l) or len(l) < 8:
            continue
        off = l[8:12] if re.fullmatch(r"[0-9A-F]{4}", l[8:12]) else None
        field, source = l[14:37].strip(), l[37:].split(";")[0].strip()
        if re.match(r"\.CODE\b", source, re.I):
            in_code = True
            continue
        if re.match(r"(\.DATA\b|\.FARDATA\b|\w+\s+SEGMENT\b|END\b)", source, re.I):
            in_code = False
            continue
        if not in_code or off is None or not DATA.match(source):
            continue
        while field.endswith("+") and i < len(lines) and re.match(r"\s*\d+ {7}", lines[i]):
            field = field[:-1] + " " + lines[i][14:37].strip()
            i += 1
        b = object_bytes(field, base)
        at = base + int(off, 16)
        if runs and runs[-1][1] + len(runs[-1][2]) == at:
            runs[-1][2] += b
        else:
            runs.append([seg, at, bytearray(b)])
    return [(s, o, bytes(b)) for s, o, b in runs]


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--out", default=str(ROOT / "port/asmdata.json"))
    ap.add_argument("files", nargs="*")
    a = ap.parse_args(argv)
    us = units()
    files = [Path(f) for f in a.files] or sorted(
        ROOT / "src" / (p.stem + ".ASM") for p in (ROOT / "port/asm").glob("E*.c"))
    mz = read_mz((ROOT / "eob/EOB.EXE").read_bytes())
    img = mz["image"]
    out, bad = {}, 0
    for f in files:
        u = us.get(f.stem.lower())
        if u is None:
            print("%s: not a unit of EOB.EXE" % f.name)
            bad += 1
            continue
        runs = code_data(f, u)
        rows = []
        for seg, off, b in runs:
            lin = seg * 16 + off
            if img[lin:lin + len(b)] != b:
                k = next(j for j in range(len(b)) if img[lin + j] != b[j])
                print("%s: %04X:%04X (%d bytes) is not EOB.EXE's at +%d: %02X, the original %02X" % (
                    f.name, seg, off, len(b), k, b[k], img[lin + k]))
                bad += 1
                continue
            rows.append(["%04X" % seg, "%04X" % off, b.hex()])
        out[f.stem.upper()] = rows
        print("%-12s %3d runs, %5d bytes" % (f.name, len(rows), sum(len(r[2]) // 2 for r in rows)))
    Path(a.out).write_text(json.dumps({"about": "the data of the assembly units in their code segments, from TASM's "
                                       "listing, each run checked against EOB.EXE (tools/port/asmlist.py, "
                                       "make port-asmdata)", "units": out}, indent=1) + "\n")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())

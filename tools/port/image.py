#!/usr/bin/env python3
"""image.py: the port's generated data image against EOB.EXE's (step D3, playbook 80; `make port-image`).

The port writes its units' definitions into an empty arena (`eob-port-headless --data-image`, tc2c's
tc2c_data) and saves DGROUP; this compares it with DGROUP of EOB.EXE's load module, loaded and
relocated at the same segment (1000h). Compared:

  - the program's initialised data, ds:0090 to 84B2, module by module (re/datamap.toml);
  - E1948's driver segment, ds:88A0 to 8A6C (its own SNDDATA segment, src/E1948.ASM);
  - the uninitialised data, ds:8A6C to the end of the load module's DGROUP: zero in both;
  - the library variables the port's runtime keeps in DGROUP, which it writes itself
(tc_library_data). And the code segments below DGROUP: the assembly units' data there (port/asmdata.json,
tools/port/asmlist.py), each run the same as EOB.EXE's, and no other byte. The rest of the start-up code's data (ds:0000 to 008F) and of the library's
(ds:84B2 to 88A0) is listed, not compared: no function of the port reads it. Exit 0 when every compared
byte is the same.
"""
from __future__ import annotations

import argparse
import subprocess
import sys
import tomllib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools" / "re"))
from mzutil import read_mz  # noqa: E402

DGROUP = 0x212E
LOAD = 0x1000
SND = (0x88A0, 0x8A6C)
LIBRARY = [(0x0000, 0x0090, "the start-up code's data (C0)"), (0x84B2, 0x88A0, "the library's data")]
# the library variables the runtime writes (tc_library_data, port/runtime/tcdos.c): _openfd and _fmode,
# rand's seed, tzset's timezone, daylight and tzname
RUNTIME = [(0x8758, 0x8782, "_openfd, _fmode"), (0x8784, 0x8788, "rand's seed"),
           (0x8810, 0x8818, "tzname"), (0x8818, 0x881E, "timezone, daylight")]


def original_image() -> bytes:
    mz = read_mz((ROOT / "eob/EOB.EXE").read_bytes())
    img = bytearray(mz["image"])
    for o, s in mz["relocs"]:
        p = s * 16 + o
        v = int.from_bytes(img[p:p + 2], "little")
        img[p:p + 2] = ((v + LOAD) & 0xFFFF).to_bytes(2, "little")
    return bytes(img)


def original_dgroup() -> bytes:
    d = original_image()[DGROUP * 16:DGROUP * 16 + 0x10000]
    return d + b"\0" * (0x10000 - len(d))


def code_segments(path: Path) -> int:
    """The code segments the port writes (step M5): the assembly units' data of port/asmdata.json, each
    run the same as EOB.EXE's, and nothing else (the port has no code bytes). How many differ."""
    import json
    ours, orig = path.read_bytes(), original_image()
    runs = [(int(sg, 16) * 16 + int(of, 16), len(hx) // 2, u) for u, rows in
            json.loads((ROOT / "port/asmdata.json").read_text())["units"].items() for sg, of, hx in rows]
    import re
    sys.path.insert(0, str(ROOT / "tools" / "port"))
    from asmdata import module_data
    far = ROOT / "src/far/FARDATA.ASM"
    for seg in re.findall(r"^FAR_([0-9A-F]{4})\s+SEGMENT\b", far.read_text(errors="replace"), re.M):
        runs.append((int(seg, 16) * 16, len(module_data(far, 0, "FAR_" + seg)[0]), "Westwood's far data " + seg))
    bad, inside = 0, bytearray(len(ours))
    for lin, n, u in runs:
        inside[lin:lin + n] = b"\1" * n
        if ours[lin:lin + n] != orig[lin:lin + n]:
            bad += 1
            print("DIFFER  %s's code-segment data at %05X (%d bytes)" % (u, lin + LOAD * 16, n))
    stray = [i for i in range(len(ours)) if ours[i] and not inside[i]]
    if stray:
        bad += 1
        print("DIFFER  the code segments hold %d bytes no assembly unit's data put there, the first at %05X"
              % (len(stray), stray[0] + LOAD * 16))
    print("port-image: the assembly units' data in their code segments and Westwood's far data, %d runs of %d bytes: %s" % (
        len(runs), sum(n for _, n, _ in runs), "the same" if not bad else "%d differ" % bad))
    return bad


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--port", default=str(ROOT / "build/port/eob-port-headless"))
    ap.add_argument("--out", default=str(ROOT / "build/port/dgroup.bin"))
    a = ap.parse_args(argv)
    r = subprocess.run([a.port, "--data-image", a.out], capture_output=True, text=True)
    if r.returncode:
        print(r.stdout + r.stderr, end="")
        return r.returncode
    ours, orig = Path(a.out).read_bytes(), original_dgroup()
    mods = tomllib.loads((ROOT / "re/datamap.toml").read_text())["module"]
    ranges = [(int(m["start"][3:], 16), int(m["end"][3:], 16), m["id"]) for m in mods]
    ranges += [(SND[0], SND[1], "e1948 (its driver segment)"), (SND[1], 0x10000, "the uninitialised data")]
    ranges += [(lo, hi, "the runtime's " + what) for lo, hi, what in RUNTIME]
    bad = 0
    for lo, hi, name in ranges:
        diff = [o for o in range(lo, hi) if ours[o] != orig[o]]
        if diff:
            bad += 1
            o = diff[0]
            print("DIFFER  %-28s ds:%04X-%04X: %d bytes differ, the first at ds:%04X (ours %02X, original %02X)"
                  % (name, lo, hi, len(diff), o, ours[o], orig[o]))
    for lo, hi, what in LIBRARY:
        print("not compared  ds:%04X-%04X: %s, but for the runtime's variables" % (lo, hi, what))
    n = sum(hi - lo for lo, hi, _ in ranges)
    print("port-image: %d ranges, %d bytes compared: %s" % (len(ranges), n, "the same" if not bad else "%d differ" % bad))
    bad += code_segments(Path(a.out + ".code"))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())

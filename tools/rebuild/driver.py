#!/usr/bin/env python3
"""driver.py: build a graphics driver (CGA.OVL, EGA.OVL, MCGA.OVL, TGA.OVL) from src/drivers/ and compare it
with the original, byte for byte, header included (SPEC O1 since 0.2, whitepaper 2.5).

  driver.py MCGA.OVL [-v] [--source FILE]       (make rebuild PROGRAM=MCGA.OVL)

The driver is one assembly module, src/drivers/<NAME>.ASM: TASM 2.0 assembles it (/mx, with a listing) and
TLINK 3.0 links it alone, with no start-up code, library or stack (TLINK's "No stack" warning is expected:
the originals have SS:SP 0000:0000). The result is compared with eob/<NAME>.OVL: the header (relocations and
sizes) and every byte of the image. A difference is reported at the source line that produced it (from the
listing), so a wrong instruction shows where it is. Exit code 0 when the whole file is identical. The
working files stay in build/rebuild/<name>/ (D.ASM, D.LST, D.EXE).

This is the proof of a driver: `make match` proves functions of EOB.EXE and INTRO.EXE linked with C0L
(re/units.toml), and a driver is a program of its own, so it is built and compared whole like `make rebuild`
does for START1.EXE.
"""
from __future__ import annotations

import argparse
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools" / "re"))
import tc_toolchain as tc  # noqa: E402

DRIVERS = ("CGA.OVL", "EGA.OVL", "MCGA.OVL", "TGA.OVL")
TASM_OPTS = ["/mx"]
TIMEOUT = 120        # seconds: TASM 2.0 with a listing never ends on some errors inside a REPT block


def run(release: str, tool: str, args: list[str], cwd: Path) -> str:
    """tc.run with a time limit; returns the tool's output (a timeout is reported in it)."""
    target = tc.TOOLCHAIN / release
    env = dict(os.environ, EMU2_DRIVE_C=str(cwd), EMU2_DRIVE_D=str(target), EMU2_DEFAULT_DRIVE="C")
    try:
        r = subprocess.run([str(tc.EMU2), str(target / "BIN" / (tool + ".EXE"))] + args, env=env, cwd=cwd,
                           capture_output=True, stdin=subprocess.DEVNULL, timeout=TIMEOUT)
        return r.stdout.decode("latin-1")
    except subprocess.TimeoutExpired as e:
        return (e.stdout or b"").decode("latin-1") + "\n**Error** %s did not finish in %d s\n" % (tool, TIMEOUT)


def source_of(name: str) -> Path:
    return ROOT / "src" / "drivers" / (name.split(".")[0].upper() + ".ASM")


def build(name: str, source: Path, w: Path) -> tuple[bytes | None, str]:
    """Assemble and link `source` in `w`; returns (the linked file or None, the tools' output)."""
    shutil.rmtree(w, ignore_errors=True)
    w.mkdir(parents=True)
    (w / "D.ASM").write_bytes(source.read_text().replace("\r\n", "\n").replace("\n", "\r\n").encode("latin-1"))
    log = run(tc.manifest()["pinned_asm"], "TASM", TASM_OPTS + ["/l", "D.ASM"], cwd=w)
    if "**Error**" in log or "*Warning*" in log or not (w / "D.OBJ").exists():
        return None, log
    log += run(tc.manifest()["pinned"], "TLINK", ["D.OBJ,D.EXE"], cwd=w)
    exe = w / "D.EXE"
    if re.search(r"^(Fatal|Error):|\*\*Error\*\*", log, re.M) or not exe.exists():
        return None, log
    return exe.read_bytes(), log


def listing(w: Path) -> list[tuple[int, str]]:
    """(offset, source line) for each listing line that has an offset, in order."""
    out = []
    lst = w / "D.LST"
    if not lst.exists():
        return out
    for line in lst.read_text(encoding="latin-1").splitlines():
        m = re.match(r"\s*\d+\s+([0-9A-F]{4})\s", line)
        if m:
            out.append((int(m.group(1), 16), line.rstrip()))
    return out


def line_at(lines: list[tuple[int, str]], off: int) -> str:
    best = ""
    for o, text in lines:
        if o <= off:
            best = text
        else:
            break
    return re.sub(r"\s+", " ", best).strip()


def compare(name: str, ours: bytes, orig: bytes, w: Path, limit: int = 12) -> list[str]:
    """What differs, in words: the header fields, then the image at the source lines."""
    out = []
    hdr = int.from_bytes(orig[8:10], "little") * 16
    ohdr = int.from_bytes(ours[8:10], "little") * 16 if len(ours) >= 10 else 0
    if ours[:hdr] != orig[:hdr]:
        fields = ["signature", "last page bytes", "pages", "relocations", "header paragraphs", "minalloc",
                  "maxalloc", "ss", "sp", "checksum", "ip", "cs", "relocation table", "overlay"]
        for i, f in enumerate(fields):
            a, b = ours[2 * i:2 * i + 2], orig[2 * i:2 * i + 2]
            if a != b:
                out.append("header %s: ours %s, original %s" % (f, a.hex(), b.hex()))
        if ours[0x1C:hdr] != orig[0x1C:hdr]:
            out.append("header bytes 1Ch to %Xh (the TLINK signature and relocations) differ" % hdr)
    a, b = ours[ohdr:], orig[hdr:]
    if len(a) != len(b):
        out.append("image %d bytes, original %d" % (len(a), len(b)))
    lines = listing(w)
    n, i = 0, 0
    while i < min(len(a), len(b)) and n < limit:
        if a[i] != b[i]:
            out.append("image %04X: ours %s, original %s  | %s" % (i, a[i:i + 6].hex(), b[i:i + 6].hex(),
                                                                    line_at(lines, i)))
            n += 1
            while i < min(len(a), len(b)) and a[i] != b[i]:
                i += 1
        i += 1
    return out


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("driver", help="CGA.OVL, EGA.OVL, MCGA.OVL or TGA.OVL")
    ap.add_argument("-v", "--verbose", action="store_true")
    ap.add_argument("--source", help="the source to build (default src/drivers/<NAME>.ASM)")
    a = ap.parse_args(argv)
    name = a.driver.upper()
    if name not in DRIVERS:
        print("driver: %s is not one of %s" % (a.driver, ", ".join(DRIVERS)))
        return 2
    source = Path(a.source) if a.source else source_of(name)
    if not source.exists():
        print("driver: no source %s" % source.relative_to(ROOT) if source.is_relative_to(ROOT) else source)
        return 2
    orig = (ROOT / "eob" / name).read_bytes()
    w = ROOT / "build" / "rebuild" / name.split(".")[0].lower()
    ours, log = build(name, source, w)
    if a.verbose or ours is None:
        print(log.strip())
    if ours is None:
        print("driver: %s does not assemble and link" % name)
        return 1
    if ours == orig:
        print("driver: %s from %s: %d bytes, the whole file is identical" % (
            name, source.relative_to(ROOT) if source.is_relative_to(ROOT) else source, len(orig)))
        return 0
    for line in compare(name, ours, orig, w):
        print("driver: " + line)
    print("driver: %s differs from the original" % name)
    return 1


if __name__ == "__main__":
    sys.exit(main())

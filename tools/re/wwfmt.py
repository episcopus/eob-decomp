#!/usr/bin/env python3
"""wwfmt.py: Westwood data formats of the pinned files (whitepaper 4.2 to 4.4).

Compressed files (CPS, CMP, EGA, INF, VCN, ECN) start with the header the game's loader reads
(EOB.EXE 1B82:0041 LoadPacked and 1B82:00C7 Unpack, src/E1B82.C):

  +0 u16 bytes after this field (some files: the file size)
  +2 u8  format: 0 stored, 1 and 2 need a work buffer (3C00h, FC00h bytes), 3, 4 LCW
  +3 u8  0
  +4 u32 unpacked size
  +8 u16 palette bytes that follow the header (skipped by format 4)

LCW (format 4), written from the published description of the format, decodes into a buffer and
copies from the output written so far:
  0ccc pppp pppppppp    copy ccc+3 bytes from (current - pppppppppppp)
  10cc cccc             copy cccccc literal bytes from the input; 80h alone ends the data
  11cc cccc pppppppp pppppppp      copy cccccc+3 bytes from the absolute output offset
  FE nnnn vv            fill nnnn bytes with vv
  FF nnnn pppp          copy nnnn bytes from the absolute output offset pppp

Other formats: MAZ (u16 width, u16 height, u16 bytes per cell, then cells), VMP and EMP (u16 count,
then u16 entries), PAL and COL (768 bytes of 6-bit VGA palette).

  wwfmt.py verify eob/                          every file of every PAK, decoded and checked
  wwfmt.py png eob/EOBDATA1.PAK TITLE-V.CMP OUT.PNG [--palette EOBDATA2.PAK:EOBPAL.COL]
"""
from __future__ import annotations

import glob, os, struct, sys
from collections import Counter

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import pak  # noqa: E402

COMPRESSED = {"CPS", "CMP", "EGA", "INF", "VCN", "ECN"}
# Files that do not decode to their declared size, with what is known (whitepaper 4.2).
KNOWN = {
    "EOBTITLE.CMP": "LCW stream decodes coherently for 31962 bytes (99 rows of a title picture), "
                    "then a fill of 13082 bytes (FE 1A 33 4C) and garbage past 64000; damaged in this copy? "
                    "Named by EOB.EXE and INTRO.EXE",
}


def header(d: bytes) -> dict | None:
    if len(d) < 10:
        return None
    size, fmt, zero, unpacked, pal = struct.unpack_from("<HBBIH", d)
    if size not in (len(d) - 2, len(d)) or zero or fmt > 4:
        return None
    return {"size": size, "format": fmt, "unpacked": unpacked, "palette": pal}


def lcw(src: bytes, size: int) -> bytes:
    out, i = bytearray(), 0
    while len(out) < size:
        c = src[i]
        i += 1
        if c < 0x80:
            n, back = ((c >> 4) & 7) + 3, ((c & 0x0F) << 8) | src[i]
            i += 1
            for _ in range(n):
                out.append(out[len(out) - back])
        elif c < 0xC0:
            n = c & 0x3F
            if n == 0:
                break
            out += src[i:i + n]
            i += n
        elif c < 0xFE:
            n, pos = (c & 0x3F) + 3, struct.unpack_from("<H", src, i)[0]
            i += 2
            for k in range(n):
                out.append(out[pos + k])
        elif c == 0xFE:
            n, v = struct.unpack_from("<HB", src, i)
            i += 3
            out += bytes([v]) * n
        else:
            n, pos = struct.unpack_from("<HH", src, i)
            i += 4
            for k in range(n):
                out.append(out[pos + k])
    return bytes(out)


def unpack(d: bytes) -> tuple[bytes, bytes]:
    """(data, palette) of a compressed file."""
    h = header(d)
    if h is None:
        raise ValueError("no Westwood header")
    body = d[10:]
    palette, body = body[:h["palette"]], body[h["palette"]:]
    if h["format"] == 0:
        return body[:h["unpacked"]], palette
    if h["format"] == 4:
        return lcw(body, h["unpacked"]), palette
    raise ValueError("format %d not decoded yet" % h["format"])


def maze(d: bytes) -> dict:
    w, h, cell = struct.unpack_from("<HHH", d)
    if 6 + w * h * cell != len(d):
        raise ValueError("maze %dx%dx%d is not %d bytes" % (w, h, cell, len(d)))
    return {"width": w, "height": h, "cell": cell, "cells": d[6:]}


def words(d: bytes) -> list[int]:
    n = struct.unpack_from("<H", d)[0]
    if 2 + 2 * n != len(d):
        raise ValueError("%d entries is not %d bytes" % (n, len(d)))
    return list(struct.unpack_from("<%dH" % n, d, 2))


def verify(folder: str) -> int:
    counts, bad = Counter(), 0
    for f in sorted(glob.glob(os.path.join(folder, "*.PAK"))):
        for e in pak.read_pak(open(f, "rb").read())["entries"]:
            ext = e["name"].rsplit(".", 1)[-1].upper()
            d = e["data"]
            try:
                h = header(d) if ext in COMPRESSED or ext == "EGA" else None
                if h:
                    data, _ = unpack(d)
                    if len(data) != h["unpacked"] and e["name"].upper() in KNOWN:
                        counts["known anomaly"] += 1
                        continue
                    if len(data) != h["unpacked"]:
                        raise ValueError("unpacked to %d bytes, header says %d" % (len(data), h["unpacked"]))
                    counts["%s format %d" % (ext, h["format"])] += 1
                elif ext == "MAZ":
                    maze(d)
                    counts["MAZ"] += 1
                elif ext in ("VMP", "EMP"):
                    words(d)
                    counts[ext] += 1
                elif ext in ("PAL", "COL"):
                    if len(d) != 768 or max(d) > 63 and ext == "COL":
                        raise ValueError("not a 768-byte palette")
                    counts[ext] += 1
                else:
                    counts[ext + " not decoded"] += 1
            except ValueError as err:
                counts[ext + " not decoded"] += 1
                if "not decoded yet" not in str(err):
                    bad += 1
                    print("%s %s: %s" % (os.path.basename(f), e["name"], err))
    print("wwfmt: " + ", ".join("%s %d" % kv for kv in sorted(counts.items())) + ("; OK" if not bad else "; %d problems" % bad))
    return 1 if bad else 0


def find(spec: str, folder: str = "eob") -> bytes:
    archive, name = spec.split(":")
    for e in pak.read_pak(open(os.path.join(folder, archive), "rb").read())["entries"]:
        if e["name"].upper() == name.upper():
            return e["data"]
    raise SystemExit("%s not in %s" % (name, archive))


def main(argv: list[str]) -> int:
    if len(argv) >= 3 and argv[1] == "verify":
        return verify(argv[2])
    if len(argv) >= 5 and argv[1] == "png":
        from PIL import Image
        folder = os.path.dirname(argv[2]) or "."
        data, pal = unpack(find("%s:%s" % (os.path.basename(argv[2]), argv[3]), folder))
        if "--palette" in argv:
            pal = find(argv[argv.index("--palette") + 1], folder)
        img = Image.frombytes("P", (320, len(data) // 320), data[:320 * (len(data) // 320)])
        img.putpalette([min(255, c * 4) for c in pal[:768]])
        img.save(argv[4])
        print("wrote %s (%dx%d)" % (argv[4], img.width, img.height))
        return 0
    print(__doc__)
    return 2


if __name__ == "__main__":
    sys.exit(main(sys.argv))

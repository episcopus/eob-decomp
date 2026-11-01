#!/usr/bin/env python3
"""eobfmt.py: Eye of the Beholder's own data files, the ones without the Westwood header (whitepaper 4.4).

Each decoder checks the invariants that hold for every pinned file. Layouts tagged [V] are read by
the routine named; read sizes marked "trace" come from the oracle's file reads during the golden
session load-walk (playbook 25); field meanings tagged [H] are not proven yet.

  FONT6.FNT, FONT8.FNT   [V] 1A47:0005: u16 bytes after this field; u16 offsets[128] of the glyphs
                         from the file start; u8 height, u8 width at 102h; glyphs from 104h,
                         height * ((width + 7) / 8) bytes each, one bit per pixel, MSB left
  TEXT.DAT               u16 offsets of NUL-terminated strings; the table ends at the first one
  ITEM.DAT               trace (ovl2:0088): u16 count, count records of 14 bytes; u16 count,
                         count names of 35 bytes (NUL-padded)
  ITEMTYPE.DAT           trace (ovl2:013C): u16 count, count records of 16 bytes
  <wall set>.DAT         trace (ovl3:0096): u16 count, count decorations of 52 bytes; u16 count,
                         count rectangles of 8 bytes (u16 x, y, w, h) [H]; a decoration's first 10
                         bytes index the rectangles (FFh: none) [H]
  ITEMRMP.VGA, .EGA      trace (ovl2:0484): 1600 bytes read whole; [H] 100 rows of 16 colours
  ADLIB.DAT, PCGAMESN.DAT [V] 177D:000A: u8 map[120] (game sound to driver sound; 96h and up
                         ignored, 177D:0070), then the driver's data (at most 32C8h bytes): u16
                         offsets, the first one the size of the table
  PCSOUND.DAT, SOUND.DAT the same layout [H]; EOB.EXE names them only in its PAK catalogue
  EOBDATA.SAV            trace (ovl3:160A): 6 characters of 243 bytes (u8 index, u8 in party,
                         char name[11], ...), 64 bytes, 500 items of 14 bytes, 12 level states of
                         2040 bytes, 3 bytes, 96 bytes
  LEVELS.TMP             12 level states of 2040 bytes (the game reads one at a time, ovl3:1194)

  eobfmt.py verify eob/                every pinned file of these kinds, decoded and checked
  eobfmt.py save FILE                  the sections of a saved game; checks the round trip
"""
from __future__ import annotations

import glob, os, struct, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import pak  # noqa: E402

WALL_SETS = {"BRICK", "BLUE", "GREEN", "XANATHA", "DROW"}
SOUND_DATA = {"ADLIB.DAT", "PCGAMESN.DAT", "PCSOUND.DAT", "SOUND.DAT"}
SOUND_DATA_MAX = 0x32C8                  # the buffer 177D:004C allocates
SAVE_SECTIONS = (("characters", 6 * 243), ("party", 64), ("items", 500 * 14),
                 ("levels", 12 * 2040), ("unknown3", 3), ("unknown96", 96))
LEVEL_STATE = 2040


class FormatError(ValueError):
    pass


def need(cond: bool, what: str) -> None:
    if not cond:
        raise FormatError(what)


def u16(d: bytes, p: int) -> int:
    return struct.unpack_from("<H", d, p)[0]


def font(d: bytes) -> dict:
    need(len(d) > 0x104 and u16(d, 0) + 2 == len(d), "size field is not the file size - 2")
    height, width = d[0x102], d[0x103]
    per = height * ((width + 7) // 8)
    need(0x104 + 128 * per == len(d), "128 glyphs of %d bytes do not fill the file" % per)
    offs = struct.unpack_from("<128H", d, 2)
    need(all(o == 0x104 + i * per for i, o in enumerate(offs)), "glyph offsets out of order")
    return {"height": height, "width": width, "glyphs": [d[o:o + per] for o in offs]}


def text(d: bytes) -> list[bytes]:
    first = u16(d, 0)
    need(first % 2 == 0 and 0 < first <= len(d), "bad table size")
    offs = struct.unpack_from("<%dH" % (first // 2), d)
    need(list(offs) == sorted(offs) and offs[-1] < len(d), "offsets out of order")
    out = []
    for o in offs:
        end = d.find(b"\0", o)
        need(end >= 0, "string at %d not terminated" % o)
        out.append(d[o:end])
    return out


def records(d: bytes, p: int, size: int) -> tuple[list[bytes], int]:
    n = u16(d, p)
    p += 2
    need(p + n * size <= len(d), "%d records of %d bytes overrun the file" % (n, size))
    return [d[p + i * size:p + (i + 1) * size] for i in range(n)], p + n * size


def items(d: bytes) -> dict:
    recs, p = records(d, 0, 14)
    names, p = records(d, p, 35)
    need(p == len(d), "%d bytes after the names" % (len(d) - p))
    return {"items": recs, "names": [n.split(b"\0")[0] for n in names]}


def item_types(d: bytes) -> list[bytes]:
    recs, p = records(d, 0, 16)
    need(p == len(d), "%d bytes after the records" % (len(d) - p))
    return recs


def decorations(d: bytes) -> dict:
    decs, p = records(d, 0, 52)
    rects, p = records(d, p, 8)
    need(p == len(d), "%d bytes after the rectangles" % (len(d) - p))
    for k, r in enumerate(decs):
        need(all(s == 0xFF or s < len(rects) for s in r[:10]), "decoration %d names a missing rectangle" % k)
    return {"decorations": decs, "rects": [struct.unpack("<4H", r) for r in rects]}


def remap(d: bytes) -> list[bytes]:
    need(len(d) == 1600, "not 1600 bytes")
    return [d[i:i + 16] for i in range(0, 1600, 16)]


def sound_data(d: bytes) -> dict:
    need(len(d) > 122, "too short")
    body = d[120:]
    first = u16(body, 0)
    need(first % 2 == 0 and first < len(body), "bad offset table")
    offs = struct.unpack_from("<%dH" % (first // 2), body)
    need(all(o < len(body) for o in offs), "an offset past the end")
    return {"map": d[:120], "offsets": offs, "data": body, "fits": len(body) <= SOUND_DATA_MAX}


def save(d: bytes) -> dict:
    need(len(d) == sum(n for _, n in SAVE_SECTIONS), "not %d bytes" % sum(n for _, n in SAVE_SECTIONS))
    out, p = {}, 0
    for name, n in SAVE_SECTIONS:
        out[name] = d[p:p + n]
        p += n
    chars = [out["characters"][i * 243:(i + 1) * 243] for i in range(6)]
    need(all(c[0] == i for i, c in enumerate(chars)), "character records out of order")
    out["names"] = [c[2:13].split(b"\0")[0] for c in chars if c[1]]
    out["level_states"] = [out["levels"][i * LEVEL_STATE:(i + 1) * LEVEL_STATE] for i in range(12)]
    return out


def save_bytes(s: dict) -> bytes:
    return b"".join(s[name] for name, _ in SAVE_SECTIONS)


def levels(d: bytes) -> list[bytes]:
    need(len(d) % LEVEL_STATE == 0, "not a whole number of level states")
    return [d[i:i + LEVEL_STATE] for i in range(0, len(d), LEVEL_STATE)]


def decode(name: str, d: bytes):
    """The decoded form of one file, or None when this module does not handle its kind."""
    n = name.upper()
    base, _, ext = n.rpartition(".")
    if ext == "FNT":
        return font(d)
    if n == "TEXT.DAT":
        return text(d)
    if n == "ITEM.DAT":
        return items(d)
    if n == "ITEMTYPE.DAT":
        return item_types(d)
    if base in WALL_SETS and ext == "DAT":
        return decorations(d)
    if base == "ITEMRMP":
        return remap(d)
    if n in SOUND_DATA:
        return sound_data(d)
    if n == "EOBDATA.SAV":
        return save(d)
    if n == "LEVELS.TMP":
        return levels(d)
    return None


def verify(folder: str) -> int:
    counts, bad = {}, 0
    files = [(os.path.basename(f), open(f, "rb").read()) for f in sorted(glob.glob(os.path.join(folder, "*.FNT")))]
    for f in sorted(glob.glob(os.path.join(folder, "*.PAK"))):
        files += [(e["name"], e["data"]) for e in pak.read_pak(open(f, "rb").read())["entries"]]
    for name, d in files:
        try:
            if decode(name, d) is not None:
                kind = name.upper().rpartition(".")[2]
                counts[kind] = counts.get(kind, 0) + 1
        except FormatError as e:
            bad += 1
            print("%s: %s" % (name, e))
    print("eobfmt: " + ", ".join("%s %d" % kv for kv in sorted(counts.items())) + ("; OK" if not bad else "; %d problems" % bad))
    return 1 if bad else 0


def main(argv: list[str]) -> int:
    if len(argv) >= 3 and argv[1] == "verify":
        return verify(argv[2])
    if len(argv) >= 3 and argv[1] == "save":
        d = open(argv[2], "rb").read()
        s = save(d)
        print("party: %s" % ", ".join(n.decode("latin-1") for n in s["names"]))
        for name, n in SAVE_SECTIONS:
            print("  %-10s %6d bytes" % (name, n))
        print("round trip: %s" % ("OK" if save_bytes(s) == d else "DIFFERS"))
        return 0 if save_bytes(s) == d else 1
    print(__doc__)
    return 2


if __name__ == "__main__":
    sys.exit(main(sys.argv))

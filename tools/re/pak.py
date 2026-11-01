#!/usr/bin/env python3
"""pak.py: list, verify and extract Westwood .PAK archives (whitepaper 4.1).

Layout: a directory of entries (u32 file offset, NUL-terminated name), then the files back to back.
The directory ends at the first file's offset. Its last 4 bytes are a terminator with no name,
which the pinned files fill three ways: the file size (EOBDATA1, 5), zero (EOBDATA3, 4, 6) or
other bytes (EOBDATA2: 0C 00 10 04). The terminator is kept as raw bytes so that `build_pak`
gives the original file back. An empty name also ends the directory (not seen in the pinned files).

  python3 tools/re/pak.py verify eob/                # every .PAK: directory, tiling, round trip
  python3 tools/re/pak.py list eob/EOBDATA3.PAK
  python3 tools/re/pak.py extract eob/EOBDATA3.PAK build/re/pak/EOBDATA3
"""
from __future__ import annotations

import glob, os, struct, sys


def read_pak(d: bytes) -> dict:
    if len(d) < 4:
        raise ValueError("too short for a PAK directory")
    first = struct.unpack_from("<I", d, 0)[0]
    if not 5 <= first <= len(d):
        raise ValueError("first offset %d outside the file" % first)
    entries, p = [], 0
    while True:
        if p + 4 >= first:  # 4 bytes left before the data: a bare terminator
            term, p = d[p:first], first
            break
        off = struct.unpack_from("<I", d, p)[0]
        end = d.find(b"\0", p + 4, first)
        if end < 0:
            raise ValueError("name at %d is not terminated inside the directory" % (p + 4))
        name = d[p + 4:end]
        if not name:
            term = d[p:end + 1]
            p = end + 1
            break
        entries.append({"name": name.decode("latin-1"), "offset": off})
        p = end + 1
    if p != first:
        raise ValueError("%d bytes between the terminator and the first file" % (first - p))
    if not entries or entries[0]["offset"] != first:
        raise ValueError("first entry does not start right after the directory")
    for a, b in zip(entries, entries[1:] + [{"offset": len(d)}]):
        if not a["offset"] <= b["offset"] <= len(d):
            raise ValueError("%s: offsets are not ascending inside the file" % a["name"])
        a["size"] = b["offset"] - a["offset"]
        a["data"] = d[a["offset"]:b["offset"]]
    return {"entries": entries, "terminator": term}


def build_pak(entries: list[dict], terminator: bytes) -> bytes:
    """Inverse of read_pak: the same names, data and terminator give the same bytes."""
    dirlen = sum(4 + len(e["name"]) + 1 for e in entries) + len(terminator)
    out, off = bytearray(), dirlen
    for e in entries:
        out += struct.pack("<I", off) + e["name"].encode("latin-1") + b"\0"
        off += len(e["data"])
    out += terminator
    for e in entries:
        out += e["data"]
    return bytes(out)


def describe_terminator(term: bytes, size: int) -> str:
    if len(term) in (4, 5) and term[4:] in (b"", b"\0"):
        v = struct.unpack_from("<I", term)[0]
        return "file size" if v == size else "zero" if v == 0 else "other (%s)" % term.hex()
    return "%d raw bytes %s" % (len(term), term.hex())


def main(argv: list[str]) -> int:
    if len(argv) < 3 or argv[1] not in ("verify", "list", "extract"):
        print(__doc__)
        return 2
    cmd, target = argv[1], argv[2]
    if cmd == "verify":
        files = sorted(glob.glob(os.path.join(target, "*.PAK")))
        nfiles = bad = 0
        for f in files:
            d = open(f, "rb").read()
            try:
                pak = read_pak(d)
            except ValueError as e:
                print("%s: %s" % (f, e)); bad += 1; continue
            nfiles += len(pak["entries"])
            if build_pak(pak["entries"], pak["terminator"]) != d:
                print("%s: round trip differs" % f); bad += 1
        print("pak: %d archives, %d files, %s" % (len(files), nfiles, "OK" if not bad else "%d problems" % bad))
        return 1 if bad or not files else 0
    d = open(target, "rb").read()
    pak = read_pak(d)
    if cmd == "list":
        for e in pak["entries"]:
            print("%8d %8d  %s" % (e["offset"], e["size"], e["name"]))
        print("terminator: %s" % describe_terminator(pak["terminator"], len(d)))
        return 0
    out = argv[3] if len(argv) > 3 else os.path.join("build", "re", "pak", os.path.basename(target))
    os.makedirs(out, exist_ok=True)
    for e in pak["entries"]:
        open(os.path.join(out, e["name"]), "wb").write(e["data"])
    print("extracted %d files to %s" % (len(pak["entries"]), out))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))

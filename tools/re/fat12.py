#!/usr/bin/env python3
"""fat12.py: list and extract every file of a FAT12 floppy image (sub-directories included).

  python3 tools/re/fat12.py list DISK.IMG
  python3 tools/re/fat12.py extract DISK.IMG OUTDIR
"""
from __future__ import annotations

import os, struct, sys


class Fat12:
    def __init__(self, img: bytes):
        self.img = img
        self.bps, self.spc, rsv, nfats, self.nroot, total, _, spf = struct.unpack_from("<HBHBHHBH", img, 11)
        self.fat = rsv * self.bps
        self.root = (rsv + nfats * spf) * self.bps
        self.data = self.root + self.nroot * 32
        self.csize = self.bps * self.spc

    def next_cluster(self, c: int) -> int:
        v = struct.unpack_from("<H", self.img, self.fat + c * 3 // 2)[0]
        return v >> 4 if c & 1 else v & 0xFFF

    def chain(self, c: int) -> bytes:
        out, seen = bytearray(), set()
        while 2 <= c < 0xFF8 and c not in seen:
            seen.add(c)
            p = self.data + (c - 2) * self.csize
            out += self.img[p:p + self.csize]
            c = self.next_cluster(c)
        return bytes(out)

    def entries(self, raw: bytes, path: str = ""):
        for i in range(0, len(raw) - 31, 32):
            e = raw[i:i + 32]
            if e[0] == 0:
                break
            if e[0] == 0xE5 or e[11] == 0x0F or e[11] & 0x08:  # deleted, long name, volume label
                continue
            name = e[:8].decode("latin-1").rstrip()
            ext = e[8:11].decode("latin-1").rstrip()
            full = path + name + ("." + ext if ext else "")
            if name in (".", ".."):
                continue
            tm, dt, clus, size = struct.unpack_from("<HHHI", e, 22)
            date = "%04d-%02d-%02d" % (1980 + (dt >> 9), (dt >> 5) & 15, dt & 31)
            if e[11] & 0x10:
                yield from self.entries(self.chain(clus), full + "/")
            else:
                yield {"path": full, "size": size, "date": date, "data": self.chain(clus)[:size]}

    def files(self):
        return list(self.entries(self.img[self.root:self.data]))


def main(argv: list[str]) -> int:
    if len(argv) < 3 or argv[1] not in ("list", "extract"):
        print(__doc__)
        return 2
    fs = Fat12(open(argv[2], "rb").read())
    for f in fs.files():
        if argv[1] == "list":
            print("%8d %s %s" % (f["size"], f["date"], f["path"]))
        else:
            p = os.path.join(argv[3], *f["path"].split("/"))
            os.makedirs(os.path.dirname(p), exist_ok=True)
            open(p, "wb").write(f["data"])
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))

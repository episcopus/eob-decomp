"""E17A6.c (the graphics support segment in C) against the original, by Direct calls on the bench
(playbook 80, M3). Each routine is called with chosen and random arguments and states, the original
then the C on the same state, comparing memory, the registers returned and the devices:

- DirectionFromPoints and Distance over every pair of a grid of cells, random 10-bit cells and random
  words (Distance takes any word; DirectionFromPoints any word below 2000h, past which its `div`
  raises INT 0);
- FindNeighbour with targets that each offset hits, misses and random pair indexes;
- SetPlaneTables from random tables, at offsets that wrap;
- ExpandNibbles and BuildMask on random images (BuildMask through random driver tables);
- TranslateImage in modes 0 to 4 and others, random headers (counts of 0 included: 64 KB loops),
  random tables;
- BlitSpriteDefault (Tandy and MCGA), BlitSpriteCga and BlitSpriteEga on random sprites: odd
  widths, remainders 0, 1 and 2, row counts that drop rows, EGA rows over all 16 planes;
- DrawTile in the CGA, Tandy and MCGA/VGA branches (and graphics choices past 4, which take the MCGA
  branch) on random tile maps with the mirrored and drawn-over flags, the destination at random
  offsets; the EGA branch stops in the C (tc_fail 74) and is expected to.

Exit status 1 if any call differs or does not run.

  e17a6.py [LIB] [SCALE]
"""
import random
import struct
import sys
from collections import Counter
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools" / "port"))
import hybrid

LIB = Path(sys.argv[1]) if len(sys.argv) > 1 else hybrid.LIB
SCALE = float(sys.argv[2]) if len(sys.argv) > 2 else 1.0
DG = hybrid.DGROUP
CS = hybrid.LOAD_SEG + 0x17A6
rng = random.Random(0x17A6)
bad, seen = [], Counter()
d = hybrid.Direct(lib=LIB)

SRC = 0x5000            # source buffers (5000:0000 .. 7FFF:FFFF is free on the loaded bench)
DST = 0x8000            # destination buffers (the calls' stack is at 9000:FFF0)


def n(k):
    return max(1, int(k * SCALE))


def w16(seg, off, v): d.write(seg, off, struct.pack("<H", v & 0xFFFF))


def fp(seg, off): return ((seg << 16) | (off & 0xFFFF), 4)


def rbytes(k): return bytes(rng.randrange(256) for _ in range(k))


def fill(seg, size):
    """random bytes over size bytes from seg:0000, in 64 KB pieces"""
    while size > 0:
        k = min(size, 0x10000)
        d.write(seg, 0, rng.randbytes(k))
        seg += 0x1000
        size -= k


def call(name, *args, expect="same"):
    r = d.call(name, *args)
    st = r["status"]
    seen[(name, st[:40])] += 1
    if st != expect or (expect == "C stopped" and "EGA" not in r.get("why", "")):
        bad.append(dict(r, args=[hex(a) if isinstance(a, int) else (hex(a[0]), a[1]) for a in args]))
    return r


# ---- the cell helpers
cells = [0, 1, 0x1F, 0x20, 0x21, 0x3FF, 0x3E0, 0x210, 0x1EF, 0x22F, 0x200]
for a in cells:
    for b in cells:
        call("DirectionFromPoints", a, b)
        call("Distance", a, b)
for _ in range(n(400)):
    a, b = rng.randrange(0x400), rng.randrange(0x400)
    if rng.random() < 0.3:                      # near cells
        b = (a + rng.choice([-66, -65, -64, -63, -62, -33, -32, -31, -2, -1, 0, 1, 2, 31, 32, 33, 62, 64, 66])) & 0x3FF
    call("DirectionFromPoints", a, b)
    call("Distance", a, b)
for _ in range(n(200)):
    call("DirectionFromPoints", rng.randrange(0x2000), rng.randrange(0x2000))
    call("Distance", rng.randrange(0x10000), rng.randrange(0x10000))

near = [struct.unpack("<h", d.read(DG, 0x7FCC + 2 * i, 2))[0] for i in range(8)]
ahead = [struct.unpack("<h", d.read(DG, 0x7FDC + 2 * i, 2))[0] for i in range(4)]
for _ in range(n(300)):
    a = rng.randrange(0x400)
    c = rng.random()
    if c < 0.4:
        b = (a + rng.choice(near)) & 0x3FF
    elif c < 0.7:
        b = (a + rng.choice(ahead)) & 0x3FF
    else:
        b = rng.randrange(0x400)
    call("FindNeighbour", a, b, rng.choice([0, 1, 0, 1, 2, 3, rng.randrange(0x10000)]))

# ---- SetPlaneTables
for _ in range(n(40)):
    o1 = rng.choice([0, 0x10, rng.randrange(0x10000), 0xFFF8, 0xFFFF])
    o2 = rng.choice([0x100, rng.randrange(0x10000), 0xFFF1])
    d.write(SRC, o1, rbytes(16))
    d.write(SRC + 0x1000, o2, rbytes(16))
    call("SetPlaneTables", fp(SRC, o1), fp(SRC + 0x1000, o2))

# ---- ExpandNibbles
for k in range(n(6)):
    so = rng.choice([0, 0x1234, 0xFFFD, rng.randrange(0x10000)])
    do = rng.choice([0, 1, rng.randrange(0x10000)])
    fill(SRC, 0x20000)
    call("ExpandNibbles", fp(SRC, so), fp(DST, do))

# ---- BuildMask
for k in range(n(6)):
    toff = rng.choice([0, 0x80CE, rng.randrange(0x10000), 0xFE80])
    tseg = SRC + 0x2000
    fill(tseg, 0x10000)
    w16(DG, 0x80CE, toff)
    w16(DG, 0x80D0, tseg)
    io = rng.choice([0, 0x1000, rng.randrange(0x10000)])
    fill(DST, 0x10000)
    call("BuildMask", fp(DST, io))


# ---- TranslateImage
def table(mode):
    t = bytearray(rbytes(256))
    if mode == 2:                               # the stream ends at a translated byte with 20h
        for i in range(256):
            if rng.random() < 0.15:
                t[i] |= 0x20
            else:
                t[i] &= ~0x20
    return bytes(t)


for k in range(n(300)):
    mode = rng.choice([0, 0, 1, 2, 2, 3, 3, 3, 4, rng.randrange(0x10000)])
    h, w = rng.randrange(1, 40), rng.randrange(1, 40)
    if rng.random() < 0.03:
        h = 0                                   # a count of 0: the loop runs 65536 times
    so = rng.choice([0, rng.randrange(0x10000)])
    src = rbytes(4) + bytes([w, h]) + rbytes(min(0x8000, 4 * w * h + 0x400))
    d.write(SRC, so, src)
    to = rng.choice([0, 0x100, rng.randrange(0x10000)])
    d.write(SRC + 0x1000, to, table(mode))
    call("TranslateImage", fp(SRC, so), fp(DST, rng.randrange(0x10000)), fp(SRC + 0x1000, to), mode)


# ---- the sprite unpackers
def sprite_plain(rows, width):
    hdr = bytes([rng.randrange(256), rows, width, rng.randrange(256)])
    return hdr + rbytes(min(0xF000, (rows + 2) * (width * 4 + 2) + 32))


for k in range(n(160)):
    choice = rng.choice([1, 3, 3, 4])
    w16(DG, 0x8102, choice)
    rows = rng.choice([rng.randrange(1, 60), rng.randrange(60, 255), 0xFF, 0, 1, 2, 3])
    width = rng.choice([rng.randrange(1, 64), rng.randrange(1, 255), 1, 2, 3, 0])
    if width == 0 and rows not in (1, 2, 3):
        rows = 2                                # 65536 groups a row: keep it short
    so = rng.choice([0, rng.randrange(0x10000)])
    d.write(SRC, so, sprite_plain(rows, width))
    call("BlitSpriteDefault", fp(SRC, so), fp(DST, rng.randrange(0x10000)))

for k in range(n(160)):
    rows = rng.choice([rng.randrange(1, 60), rng.randrange(60, 256), 1, 2, 3])
    width = rng.choice([rng.randrange(2, 64), rng.randrange(2, 255), 2, 3, 4, 5, 1])
    if width == 1:
        rows = rng.choice([1, 2, 3])            # 65536 groups a row: keep it short
    so = rng.choice([0, rng.randrange(0x10000)])
    d.write(SRC, so, sprite_plain(rows, width) + rbytes(min(0x8000, rows * width * 2)))
    do = rng.choice([0, rng.randrange(0x10000)])
    d.write(DST, 0, rbytes(0x100))
    call("BlitSpriteCga", fp(SRC, so), fp(DST, do))


def ega_row(width):
    """a source row: per column some (pixel, flags) words, bit 7 of the flags on a column's last
    word, bits 7 and 6 on the row's last"""
    out = bytearray()
    cols = rng.choice([width, width, rng.randrange(1, width + 1)])
    planes = rng.sample(range(16), rng.choice([1, 2, 3, 5, 16]))
    for c in range(cols):
        words = [(rng.choice([0, rng.randrange(1, 256), rng.randrange(1, 256)]),
                  rng.choice(planes) | (rng.randrange(4) << 4)) for _ in range(rng.choice([1, 1, 2, 3]))]
        px, fl = words[-1]
        words[-1] = (px, fl | 0x80 | (0x40 if c == cols - 1 else 0))
        for px, fl in words:
            out += bytes([px, fl])
    return bytes(out)


for k in range(n(160)):
    rows = rng.choice([rng.randrange(1, 40), 1, 2, 3, 4])
    width = rng.choice([rng.randrange(3, 41), 3, 4, 5, 40])
    data = bytes([rng.randrange(256), rows, width, rng.randrange(256)])
    for _ in range(rows):
        data += ega_row(width)
    so = rng.choice([0, rng.randrange(0xC000)])
    d.write(SRC, so, data)
    call("BlitSpriteEga", fp(SRC, so), fp(DST, rng.randrange(0x10000)))


# ---- DrawTile
def tile_word(layer2=False):
    c = rng.random()
    if c < 0.15 and not layer2:
        return 0                                # the second layer's word is taken
    v = rng.randrange(0x1000)
    if rng.random() < 0.3:
        v |= 0x4000                             # mirrored
    if rng.random() < 0.2:
        v |= 0x8000                             # drawn over another
    return v


for k in range(n(120)):
    choice = rng.choice([3, 3, 3, 4, 0, 0, 1, 1, 5, 0xFFFF])
    w16(DG, 0x8102, choice)
    m = b"".join(struct.pack("<H", tile_word()) for _ in range(330))
    m += b"".join(struct.pack("<H", tile_word(True)) for _ in range(330))
    d.write(DG, 0xADB0, m)
    fill(SRC, 0x30000)
    if rng.random() < 0.5:
        d.write(CS, 0x3AF, rbytes(32))          # the plane tables, as SetPlaneTables leaves them
    dst = rng.choice([0, rng.randrange(0x10000)])
    w16(DG, 0xEFC2, dst)
    w16(DG, 0xEFC4, DST)
    call("DrawTile", fp(SRC, rng.choice([0, rng.randrange(0x10000)])))

w16(DG, 0x8102, 2)
call("DrawTile", fp(SRC, 0), expect="C stopped")

for key, k in sorted(seen.items()):
    print(key, k)
for b in bad[:10]:
    print(b)
print("%d calls, %d not as expected" % (sum(seen.values()), len(bad)))
sys.exit(1 if bad else 0)

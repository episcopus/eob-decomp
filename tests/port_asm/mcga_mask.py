"""MCGA.OVL's masked and remapped shape draws in C (port/drivers/mcga_mask.c) against the original's
(playbook 80, M4): each called by hybrid.Direct on the bench of mcga_bench.py, the original then the C on
the same state, comparing memory (the driver's variables, its cs:00D2 to cs:00F5 and cs:01F6 to cs:01FC
included), the screen at A000h and the registers. No golden reaches these four routines, so this test is
their proof.

  - DrawShapeMasked and DrawShapeMaskedFlipped: random format 0 shapes (1 to 40 rows, 1 to 10 columns,
    pixels and transparent runs of 1 to the row's end; a width 0 shape where it is safe), page 4's
    mask planes filled with sparse and dense random bits and page 2 with random pixels, every depth
    from 0 to 9 and the signed edges (-1, 7FFFh, 8000h); drawn on pages 0, 2 (the background itself),
    4 (the mask planes themselves), 6 and the page numbers 1 and 21h (taken AND 1Fh), at positions
    inside the region, across each edge of it, entirely outside, at odd and even x, at negative and
    huge coordinates; under the whole screen, smaller regions, a one-column region, regions past the
    screen and empty or inverted ones; and a shape whose pixels run past its segment's end;
  - DrawShapeRemapped and DrawShapeFlippedRemapped: random 4-bit shapes (format 1 to 255, 1 to 40 rows,
    0 to 10 columns) through random remap tables set by SetRemapTable (with colour 0, the transparent
    one, among them), on the same pages, positions and regions, so that each odd and even clip at the
    left and the right is reached; a gNibbleSwap table changed and put back; a shape across its
    segment's end; DrawShapeRemapped with format 0 shapes, where the original returns with the caller's
    registers changed and the C stops with the reason (checked: "C stopped"). Not
    DrawShapeFlippedRemapped with a format 0 shape: the original's retf then leaves the driver
    (mcga_mask.c).

Shapes with 0 rows are not drawn unless a clip ends the call first: the original then draws 65536 rows.

Exit status 1 if a call differs or does not run.

  mcga_mask.py [LIB]
"""
import sys

from mcga_bench import Driver, SCRATCH

t = Driver(sys.argv[1] if len(sys.argv) > 1 else None)
rng = t.rng
WRAP = 0x8800               # 8800:8000 is SCRATCH:0000; 8800:0000 is free memory below it

REGIONS = [(0, 40, 0, 200), (5, 30, 20, 150), (1, 39, 1, 199), (10, 11, 50, 60), (0, 40, 100, 101),
           (3, 45, 0, 230), (20, 10, 0, 200), (0, 40, 120, 80), (0, 0, 0, 200), (0, 40, 0, 0),
           (0x2001, 0x2028, 0, 200), (0xFFFE, 2, 0xFFF0, 20)]
PAGES = (0, 2, 4, 6, 1, 0x21)
DEPTHS = tuple(range(10)) + (0xFFFF, 0x7FFF, 0x8000)


def fill(page, density=None):
    """page's 64000 bytes random; with density, each bit set with about that chance"""
    if density is None:
        data = rng.randbytes(64000)
    else:
        data = bytes(sum(1 << b for b in range(8) if rng.random() < density) for _ in range(64000))
    t.write(t.page_seg(page), 0, data)


def masked_shape(rows, cols):
    """a format 0 shape: header (byte 1 rows, byte 2 columns), then each row's cols * 8 pixels as
    colours 1 to 255 and transparent runs (0, count) that end inside the row"""
    out = bytearray([rng.randrange(256), rows, cols, rng.randrange(256)])
    style = rng.random()
    for _ in range(rows):
        left = cols * 8
        while left:
            if rng.random() < style * 0.6:
                n = rng.randint(1, min(left, rng.choice((3, 16, 255))))
                out += bytes((0, n))
                left -= n
            else:
                out.append(rng.randrange(1, 256))
                left -= 1
    return bytes(out)


def remap_shape(rows, cols, fmt=None):
    """a 4-bit shape: format, rows, columns, 17 bytes the draw passes, then rows * cols * 4 bytes"""
    fmt = rng.randrange(1, 256) if fmt is None else fmt
    out = bytearray([fmt, rows, cols]) + rng.randbytes(17)
    zero = rng.random() * 0.5
    for _ in range(rows * cols * 4):
        hi = 0 if rng.random() < zero else rng.randrange(16)
        lo = 0 if rng.random() < zero else rng.randrange(16)
        out.append(hi << 4 | lo)
    return bytes(out)


def place(shape):
    """the shape at a random offset of SCRATCH; its far pointer"""
    off = rng.randrange(0, 0x8000 - len(shape))
    t.write(SCRATCH, off, shape)
    return t.far(SCRATCH, off)


def positions(rows, cols, region):
    """x, y: inside the region, across each edge, outside, odd and even, huge"""
    left, right, top, bottom = (v if v < 0x8000 else v - 0x10000 for v in region)
    w, h = cols * 8, rows
    l, r = left * 8, right * 8
    xs = [l, l + 1, l + 3, l - w // 2, l - w // 2 + 1, l - w + 1, l - w, l - w - 1, r - w, r - w + 1,
          r - w // 2, r - w // 2 - 1, r - 1, r, r + 5, (l + r) // 2, (l + r) // 2 + 1,
          rng.randrange(-80, 400), -32768, 32767, -1, 0x7FF0]
    ys = [top, top + 1, top - h // 2, top - h + 1, top - h, top - 1, bottom - h, bottom - h // 2,
          bottom - 1, bottom, (top + bottom) // 2, rng.randrange(-50, 260), -32768, 32767, -h * 3]
    out = [(x, rng.choice(ys)) for x in xs] + [(rng.choice(xs), y) for y in ys]
    return out


def masked_calls(n_shapes, per_shape):
    for _ in range(n_shapes):
        region = rng.choice(REGIONS)
        t.call("SetRegion", *region)
        rows, cols = rng.randint(1, 40), rng.randint(1, 10)
        shape = place(masked_shape(rows, cols))
        cases = positions(rows, cols, region)
        rng.shuffle(cases)
        for x, y in cases[:per_shape]:
            name = rng.choice(("DrawShapeMasked", "DrawShapeMaskedFlipped"))
            t.call(name, rng.choice(PAGES), shape, x, y, rng.choice(DEPTHS))


def remap_calls(n_shapes, per_shape, flipped_too=True):
    for _ in range(n_shapes):
        region = rng.choice(REGIONS)
        t.call("SetRegion", *region)
        if rng.random() < 0.5:
            t.write(SCRATCH, 0x7FE0, bytes(0 if rng.random() < 0.25 else rng.randrange(1, 256)
                                           for _ in range(16)))
            t.call("SetRemapTable", t.far(SCRATCH, 0x7FE0))
        rows, cols = rng.randint(1, 40), rng.randint(0, 10)
        shape = place(remap_shape(rows, cols))
        cases = positions(rows, cols, region)
        rng.shuffle(cases)
        for x, y in cases[:per_shape]:
            names = ("DrawShapeRemapped", "DrawShapeFlippedRemapped") if flipped_too else ("DrawShapeRemapped",)
            t.call(rng.choice(names), rng.choice(PAGES), shape, x, y)


# ---- the masked draws
fill(2)
fill(0)
fill(6)
for density in (0.02, 0.15, None):
    fill(4, density)
    masked_calls(20, 14)
fill(4, 0.1)
for name in ("DrawShapeMasked", "DrawShapeMaskedFlipped"):
    t.call("SetRegion", 0, 40, 0, 200)
    for depth in DEPTHS:                            # every depth on the same shape and place
        shape = place(masked_shape(30, 6))
        t.call(name, 0, shape, 100, 80, depth)
        t.call(name, 6, shape, 101, 81, depth)
    zero = place(bytes((0, 5, 0, 0)))                         # width 0, five rows: nothing drawn
    t.call(name, 0, zero, 10, 10, 3)
    t.call(name, 0, zero, 320, 199, 3)
    t.call(name, 0, place(bytes((0, 0, 4, 0))), 10, -5, 1)      # 0 rows above the region: no draw
    for x in (-8, -1, 0, 1, 7, 311, 312, 313, 319):           # a one-pixel run at each column edge
        t.call(name, 0, place(masked_shape(3, 1)), x, 5, rng.choice(DEPTHS))
# pixels past the segment's end: the shape at 8800:FFE0, its rows going on at 8800:0000
for name in ("DrawShapeMasked", "DrawShapeMaskedFlipped"):
    s = masked_shape(12, 4)
    t.write(WRAP, 0xFFE0, s[:0x20])
    t.write(WRAP, 0x0000, s[0x20:])
    for x, y in ((40, 40), (-9, -3), (300, 190)):
        t.call(name, 2, t.far(WRAP, 0xFFE0), x, y, 2)

# ---- the remapped draws
for _ in range(3):
    t.write(SCRATCH, 0x7FE0, bytes(rng.randrange(256) for _ in range(16)))
    t.call("SetRemapTable", t.far(SCRATCH, 0x7FE0))
    remap_calls(15, 14)
t.call("SetRegion", 0, 40, 0, 200)
for name in ("DrawShapeRemapped", "DrawShapeFlippedRemapped"):
    for cols in (0, 1, 2, 9):                       # each parity of each clip
        shape = place(remap_shape(7, cols))
        for x in (-cols * 8 - 1, -cols * 8 + 1, -9, -8, -3, -2, -1, 0, 1, 2, 3, 319 - cols * 8,
                  320 - cols * 8, 321 - cols * 8, 316, 317, 318, 319, 320):
            for y in (-8, -6, -1, 0, 3, 194, 195, 199, 200):
                t.call(name, rng.choice(PAGES), shape, x, y)
t.call("SetRegion", 10, 12, 50, 60)                 # both edges cutting the shape, each parity
for name in ("DrawShapeRemapped", "DrawShapeFlippedRemapped"):
    for cols in (3, 4):
        shape = place(remap_shape(9, cols))
        for x in range(96 - cols * 8, 82):
            t.call(name, rng.choice(PAGES), shape, x, rng.choice((45, 50, 55)))
t.call("SetRegion", 0, 40, 0, 200)
for fmt in (0, 0, 0):                               # the broken exit: the C stops with the reason
    r = t.d.call("MCGA.DrawShapeRemapped", rng.choice(PAGES), place(remap_shape(5, 3, fmt)), 50, 50)
    t.seen[("DrawShapeRemapped format 0", r["status"])] += 1
    if r["status"] != "C stopped" or "format 0" not in r.get("why", ""):
        t.bad.append(r)
saved = t.read(t.seg, 0x00F6, 256)                  # another nibble swap table
t.write(t.seg, 0x00F6, rng.randbytes(256))
remap_calls(4, 10)
t.write(t.seg, 0x00F6, saved)
s = remap_shape(6, 5)                               # across the segment's end
t.write(WRAP, 0xFFF0, s[:0x10])
t.write(WRAP, 0x0000, s[0x10:])
for name in ("DrawShapeRemapped", "DrawShapeFlippedRemapped"):
    for x, y in ((40, 40), (-9, -3), (291, 197), (3, 0)):
        t.call(name, 2, t.far(WRAP, 0xFFF0), x, y)

sys.exit(t.report())

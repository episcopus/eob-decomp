"""MCGA.OVL's DrawShape and DrawShapeFlipped in C (port/drivers/mcga_shape.c) against the original's
(playbook 80, M4): each called by hybrid.Direct on the bench of mcga_bench.py, the original then the C on
the same state, comparing memory (the driver's variables at cs:00D2 to cs:00F5 included), the screen
(A000h) and the registers.

  - both formats: format 0 (8-bit pixels, 0 and a count a transparent run) with runs inside rows, at
    row ends, crossing rows, runs of 0 pixels, and malformed streams; format 4 (a 16-byte colour
    table, two pixels a byte) with colours 0 in the table; widths 0 to 12 columns, rows 0 (the 65536
    rows the source draws) to 60;
  - shapes made of random pixels and shapes cut from the title's art (page 2 as the game left it);
  - positions inside the region, across each of its edges, entirely outside, at odd and even x, at
    negative and huge coordinates (-32768, 32767, FFFFh);
  - regions set by SetRegion: the whole screen, inner ones, one column, empty, reversed, past the
    screen's bottom (the row table read past its end) and past its right;
  - pages 0 (the screen), 2, 4, 6, and page numbers taken AND 1Fh;
  - shapes whose bytes wrap at the end of their segment (8000:FFxx onto 8000:0000).

Exit status 1 if a call differs or does not run.

  mcga_shape.py [LIB]
"""
import sys

from mcga_bench import Driver

t = Driver(sys.argv[1] if len(sys.argv) > 1 else None)
rng = t.rng
SHAPES = 0x8000             # the shapes' segment, random bytes around them. Not the scratch of
                            # mcga_bench.py: a run walked past a shape's end can read 64 KB on, and in
                            # the scratch's segment it would read the calls' stack (9000:FFF0 down),
                            # which the original's pushes fill and the C's do not
EDGE_X = (-32768, -400, -100, -17, -9, -8, -7, -2, -1, 0, 1, 2, 3, 7, 8, 9, 311, 312, 313, 318, 319, 320,
          321, 400, 32767, 0xFFFF)
EDGE_Y = (-32768, -300, -50, -13, -2, -1, 0, 1, 2, 9, 10, 11, 150, 189, 190, 191, 198, 199, 200, 201, 255,
          32767, 0xFFFF)
REGIONS = ((0, 40, 0, 200), (2, 38, 10, 190), (5, 6, 50, 60), (1, 39, 1, 199), (10, 10, 20, 100),
           (20, 10, 100, 20), (0, 40, 0, 255), (30, 50, 150, 230), (0, 40, 100, 101), (39, 40, 199, 200),
           (0xFFFF, 3, 0xFFF0, 40))


def art(rows, width):
    """rows lines of width pixels cut from page 2 (the title's art as the game left it)"""
    x0, y0 = rng.randrange(320 - width + 1 if width <= 320 else 1), rng.randrange(200 - rows + 1)
    seg = t.page_seg(2)
    return [list(t.read(seg, (y0 + j) * 320 + x0, width)) for j in range(rows)]


def random_pixels(rows, width):
    out = []
    for _ in range(rows):
        row, k = [], 0
        while k < width:
            n = rng.randrange(1, 12)
            v = 0 if rng.random() < 0.4 else None
            for _ in range(n):
                row.append(0 if v == 0 else rng.randrange(1, 256))
            k += n
        out.append(row[:width])
    return out


def format0(pixels, cols, across=False, empty_runs=False):
    """A format-0 shape: header, then the pixels with runs of 0 coded as 0 and a count (one run for at
    most 255 pixels). With across, the runs go on past the ends of the rows."""
    data = bytearray([0, len(pixels) & 0xFF, cols, rng.randrange(256)])
    streams = [sum(pixels, [])] if across else pixels
    for s in streams:
        k = 0
        while k < len(s):
            if s[k] == 0:
                n = 1
                while k + n < len(s) and s[k + n] == 0 and n < 255:
                    n += 1
                data += bytes((0, n))
                k += n
            else:
                data.append(s[k])
                k += 1
            if empty_runs and rng.random() < 0.05:
                data += bytes((0, 0))
    return data


def format4(pixels, cols, table):
    """A format-4 shape: header (first byte not 0), its 16 colours, then two pixels a byte"""
    data = bytearray([rng.randrange(1, 256), len(pixels) & 0xFF, cols, rng.randrange(256)]) + bytes(table)
    for row in pixels:
        row = [p & 0x0F for p in row] + [0] * (cols * 8 - len(row))
        for k in range(0, cols * 8, 2):
            data.append((row[k] << 4) | row[k + 1])
    return data


def shape(fmt, rows=None, cols=None, source=None):
    if rows is None:
        rows = rng.choice((1, 2, 3, 5, 8, 13, 20, 33, 47, 60))
    if cols is None:
        cols = rng.choice((0, 1, 1, 2, 3, 4, 5, 7, 12))
    source = source or rng.choice(("random", "art"))
    pixels = random_pixels(rows, cols * 8) if source == "random" else art(rows, cols * 8)
    if fmt == 0:
        return format0(pixels, cols, across=rng.random() < 0.3, empty_runs=rng.random() < 0.2)
    table = [0] + [rng.randrange(256) for _ in range(15)]
    for k in rng.sample(range(16), rng.randrange(4)):
        table[k] = 0
    return format4(pixels, cols, table)


def place(data, wrap=False):
    """Writes a shape in the scratch (random bytes after it) and returns its far pointer argument"""
    if wrap:
        off = 0x10000 - rng.randrange(1, min(len(data), 0xF0) + 1)
        off = min(off, 0xFFFD)                 # the header's word is read inside the segment
        head = 0x10000 - off
        t.write(SHAPES, off, data[:head])
        t.write(SHAPES, 0, data[head:] + bytes(rng.randrange(256) for _ in range(64)))
        return t.far(SHAPES, off)
    off = rng.randrange(0, 0xF000 - len(data))
    t.write(SHAPES, off, data + bytes(rng.randrange(256) for _ in range(256)))
    return t.far(SHAPES, off)


def draw(fmt, page=None, x=None, y=None, **kw):
    data = shape(fmt, **{k: v for k, v in kw.items() if k in ("rows", "cols", "source")})
    p = place(data, wrap=kw.get("wrap", False))
    if page is None:
        page = rng.choice((0, 2, 4, 6, 0, 2, 0x22, 0xFFE4))
    if x is None:
        x = rng.choice(EDGE_X + tuple(rng.randrange(-80, 330) for _ in range(6)))
    if y is None:
        y = rng.choice(EDGE_Y + tuple(rng.randrange(-70, 210) for _ in range(6)))
    for name in ("DrawShape", "DrawShapeFlipped"):
        t.call(name, page, p, x, y)


t.fill_page(0)                                      # page 2 keeps the title's art; 4 and 6 as they are
t.write(SHAPES, 0, bytes(rng.randrange(256) for _ in range(0x10000)))

# ---- every region, both formats, positions at the edges and random
for region in REGIONS:
    t.call("SetRegion", *region)
    for _ in range(40):
        draw(rng.choice((0, 4)))

# ---- the whole screen and an inner region, every edge position for each format
for region in ((0, 40, 0, 200), (3, 37, 20, 180)):
    t.call("SetRegion", *region)
    left, right, top, bottom = region[0] * 8, region[1] * 8, region[2], region[3]
    for fmt in (0, 4):
        for x in (left - 17, left - 9, left - 8, left - 1, left, left + 1, right - 9, right - 8, right - 7,
                  right - 1, right, right + 1):
            draw(fmt, x=x, cols=rng.choice((1, 2, 3)))
        for y in (top - 61, top - 5, top - 1, top, top + 1, bottom - 7, bottom - 1, bottom, bottom + 1):
            draw(fmt, y=y, rows=rng.choice((3, 8, 60)))
        for x, y in ((left - 3, top - 3), (right - 5, bottom - 5), (left - 4, bottom - 2), (right - 3, top - 2)):
            draw(fmt, x=x, y=y, rows=8, cols=2)          # across two edges at once

# ---- the title's art, whole screen
t.call("SetRegion", 0, 40, 0, 200)
for _ in range(40):
    draw(rng.choice((0, 4)), source="art", x=rng.randrange(-40, 330), y=rng.randrange(-40, 210))

# ---- 0 columns, and 0 rows (65536 rows: in format 4 of one column; in format 0 of no columns, since
# 65536 rows of runs walk on through random bytes past the original's budget)
for region in ((0, 40, 0, 200), (2, 30, 30, 150)):
    t.call("SetRegion", *region)
    for fmt in (0, 4):
        draw(fmt, cols=0, x=rng.randrange(0, 300), y=rng.randrange(0, 190))
        draw(fmt, cols=0, x=-5, y=-3)
        draw(fmt, rows=0, cols=fmt // 4, x=rng.randrange(0, 300), y=rng.randrange(40, 190), page=2)
        draw(fmt, rows=0, cols=fmt // 4, x=-3, y=100, page=4)

# ---- malformed format 0: random bytes after the header
t.call("SetRegion", 0, 40, 0, 200)
for _ in range(12):
    rows, cols = rng.randrange(1, 10), rng.randrange(0, 4)
    data = bytearray([0, rows, cols, 0]) + bytes(rng.choice((0, 0, 1, 2, 7, 200, 255)) if rng.random() < 0.5
                                                 else rng.randrange(256) for _ in range(rows * cols * 8 + 64))
    p = place(data)
    for name in ("DrawShape", "DrawShapeFlipped"):
        t.call(name, 2, p, rng.choice((-9, -3, 0, 5, 300, 317)), rng.choice((-4, 0, 50, 196)))

# ---- shapes whose bytes wrap at the end of their segment
for region in ((0, 40, 0, 200), (1, 39, 5, 195)):
    t.call("SetRegion", *region)
    for _ in range(10):
        draw(rng.choice((0, 4)), wrap=True)

sys.exit(t.report())

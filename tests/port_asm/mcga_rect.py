"""MCGA.OVL's rectangles, cells, tiles and lines in C (port/drivers/mcga_rect.c) against the original's
(playbook 80, M4): each called by hybrid.Direct on the bench of mcga_bench.py, the original then the C on
the same state, comparing memory, the screen and the value returned.

  - SaveRect, SaveRectFromPage, RestoreRect and RestoreRectToPage: columns, lines, widths and heights
    inside, on and past every edge, zero, negative and huge; each page as gDrawPage (and a page number
    past 1Fh); buffers at a segment's start and near its end (the copy wrapping in the segment);
  - CopyRegion: every argument on and past its edges, negative lines (the unsigned row cut), the cut at
    line 199, a row count of 0 (65536 rows wrapping in the segment), pages 0, 2, 4, 6 and 22h;
  - DrawCell8x8 and ReadCell8x8: columns and lines past each edge, each page, pointers near a
    segment's end; MirrorCell16 on random cells, its destination wrapping in its segment;
  - SetTileSet, SetTileFormat (square sizes below 3, others, negative ones) and DrawIndexedTile in the
    1 by 1 and 2 by 2 modes and mode 4 (nothing drawn), tile numbers 0 to FFFFh, columns and lines past
    the screen (unclamped: the line indexes the row offsets as it is);
  - DrawLine: every octant, horizontal runs from odd and even pixels, vertical lines, single points,
    lines clipped on each side and corner, lines wholly outside, the longest horizontal and vertical
    spans (-32768 to 32767), with the screen as the clip rectangle and with three others;
  - FillRect: corners on and past every edge, reversed, single pixels, the colour's high byte;
  - XorRect: every pair of starting and ending bits, widths 1 to 24 and the screen, reversed corners,
    corners past the screen.

Exit status 1 if a call differs or does not run.

  mcga_rect.py [LIB]
"""
import sys

from mcga_bench import Driver, SCRATCH

t = Driver(sys.argv[1] if len(sys.argv) > 1 else None)
rng = t.rng
EDGE = (-32768, -2, -1, 0, 1, 7, 8, 38, 39, 40, 41, 192, 193, 199, 200, 201, 318, 319, 320, 321, 32767,
        0xFFFF)
COLS = (-32768, -1, 0, 1, 38, 39, 40, 41, 0x7FFF)
LINES = (-32768, -1, 0, 1, 7, 191, 192, 193, 198, 199, 200, 201, 0x7FFF)
COUNTS_C = (0, 1, 2, 39, 40, 41, -1, 0x8000)
COUNTS_R = (0, 1, 2, 199, 200, 201, -1, 0x8000)
BUF = t.page_seg(6)                       # buffers the C writes go to a page, which the test compares
                                          # (9000h, the scratch segment, is the calls' stack, masked)


def pick(edges, lo, hi):
    return rng.choice(edges + (rng.randrange(lo, hi),))


def random_bytes(n):
    return bytes(rng.randrange(256) for _ in range(n))


for page in (0, 2, 4, 6):
    t.fill_page(page)
t.write(SCRATCH, 0, random_bytes(0x8000))

# ---- SaveRect, SaveRectFromPage, RestoreRect, RestoreRectToPage
for draw_page in (0, 2, 6, 4, 0x22):
    t.set_draw_page(draw_page)
    for _ in range(40):
        col, y = pick(COLS, 0, 40), pick(LINES, 0, 200)
        cols, rows = pick(COUNTS_C, 1, 41), pick(COUNTS_R, 1, 201)
        t.call("SaveRect", col, y, cols, rows, t.far(BUF, 0))
        t.call("SaveRectFromPage", col, y, cols, rows, t.far(BUF, 0))
        col, y = pick(COLS, 0, 40), pick(LINES, 0, 200)
        cols, rows = pick(COUNTS_C, 1, 41), pick(COUNTS_R, 1, 201)
        t.call("RestoreRect", col, y, cols, rows, t.far(SCRATCH, rng.randrange(0x100)))
        t.call("RestoreRectToPage", col, y, cols, rows, t.far(SCRATCH, rng.randrange(0x100)))
    for off in (0xFFF0, 0xFF01, 0xFE00):                 # a buffer that wraps in its segment
        t.call("SaveRect", 3, 50, 4, 9, t.far(BUF, off))
        t.call("SaveRectFromPage", 30, 190, 40, 30, t.far(BUF, off))
        t.call("RestoreRect", 39, 199, 2, 3, t.far(BUF, off))
        t.call("RestoreRectToPage", 0, 0, 5, 7, t.far(BUF, off))
    t.call("SaveRect", 0, 0, 40, 200, t.far(BUF, 0))     # the whole screen
    t.call("RestoreRectToPage", 0, 0, 40, 200, t.far(BUF, 0))
    t.call("SaveRectFromPage", -5, -5, 0x7FFF, 0x7FFF, t.far(BUF, 0))
    t.call("RestoreRect", 0, 0, 0xFFFF, 0xFFFF, t.far(SCRATCH, 0))

# ---- CopyRegion
t.set_draw_page(0)
for _ in range(150):
    t.call("CopyRegion", pick(COLS, 0, 40), pick(LINES, 0, 200), pick(COLS, 0, 40), pick(LINES, 0, 200),
           pick(COUNTS_C[1:], 1, 41), pick(COUNTS_R[1:], 1, 201),
           rng.choice((0, 2, 4, 6, 0x22)), rng.choice((0, 2, 4, 6)))
for dst_y in (-5, -1, 0, 1, 150, 190, 198, 199):        # the row cut against the destination line
    for rows in (1, 2, 3, 9, 10, 11, 50, 200, 0x7FFF, 0xFFFF):
        t.call("CopyRegion", 2, 7, 5, dst_y, 3, rows, 2, 0)
for cols in (0, 1, 40, 41, 0xFFFF, 0x8000):
    t.call("CopyRegion", 0, 0, 0, 0, cols, 10, 6, 2)
t.call("CopyRegion", 0, 0, 0, 0, 40, 200, 2, 0)          # whole pages
t.call("CopyRegion", 0, 0, 0, 0, 40, 200, 0, 6)
t.call("CopyRegion", 10, 20, 12, 30, 1, 0, 2, 0)         # 0 rows: 65536 of them
t.call("CopyRegion", 39, 199, 0, 0, 2, 0, 6, 0)

# ---- DrawCell8x8, ReadCell8x8, MirrorCell16
for draw_page in (0, 2, 6, 0x21):
    t.set_draw_page(draw_page)
    for _ in range(30):
        t.call("DrawCell8x8", t.far(SCRATCH, rng.randrange(0x7000)), pick(COLS, 0, 40), pick(LINES, 0, 200))
    t.call("DrawCell8x8", t.far(BUF, 0xFFF8), 5, 5)       # the source wraps in its segment
    t.call("DrawCell8x8", t.far(BUF, 0xFFE3), 39, 199)
for page in (0, 2, 4, 6, 0x20):
    for _ in range(25):
        t.call("ReadCell8x8", page, t.far(BUF, rng.randrange(0xFA00)), pick(COLS, 0, 40), pick(LINES, 0, 200))
    t.call("ReadCell8x8", page, t.far(BUF, 0xFFF4), 0, 0)
    t.call("ReadCell8x8", page, t.far(BUF, 0xFFFF), 39, 192)
for _ in range(25):
    t.call("MirrorCell16", t.far(SCRATCH, rng.randrange(0x7F00)), t.far(BUF, rng.randrange(0xFF00)))
for src, dst in ((0xFFC0, 0xFFF0), (0x0000, 0xFFFC), (0x0001, 0xFFF9), (0x1000, 0x0001)):
    t.call("MirrorCell16", t.far(BUF, src), t.far(BUF, dst))

# ---- SetTileSet, SetTileFormat, DrawIndexedTile
PIXELS, COLOURS, MAP = 0x1000, 0x6000, 0x7000
t.call("SetTileSet", t.far(SCRATCH, PIXELS), t.far(SCRATCH, COLOURS), t.far(SCRATCH, MAP))
t.call("SetTileSet", t.far(0x9100, 3), t.far(SCRATCH, COLOURS + 5), t.far(SCRATCH, MAP))
for w, h in ((0, 0), (-1, -1), (-2, -2), (0x8000, 0x8000), (3, 3), (1, 2), (2, 1), (5, 7), (0x7FFF, 0x7FFF),
             (2, 2), (1, 1)):
    t.call("SetTileFormat", w, h, rng.randrange(0x10000), rng.randrange(0x10000))
for fmt in ((1, 1), (2, 2), (3, 3), (4, 2)):
    t.call("SetTileFormat", fmt[0], fmt[1], 0, 0)
    for _ in range(25):
        tile = rng.choice((0, 1, 255, 0x100, 0x7FFF, 0xFFFF, rng.randrange(0x100)))
        t.call("DrawIndexedTile", tile, rng.choice((0, 1, 37, 38, 39, 40, rng.randrange(40))),
               rng.choice((0, 1, 183, 184, 191, 192, 199, rng.randrange(200), 250, 0xFFFF)))
    t.write(SCRATCH, MAP, random_bytes(0x100))
    t.write(SCRATCH, COLOURS, random_bytes(0x1000))
t.call("SetTileSet", t.far(SCRATCH, PIXELS), t.far(SCRATCH, 0xFFF8), t.far(SCRATCH, 0xFF80))
t.call("SetTileFormat", 2, 2, 0, 0)
for tile in (0, 0x7F, 0x80, 0xFF):                      # a colour row and a map that wrap in the segment
    t.call("DrawIndexedTile", tile, 3, 4)

# ---- DrawLine
CLIPS = ((0, 0, 319, 199), (10, 20, 300, 180), (100, 50, 101, 52), (-50, -30, 400, 250))


def lines():
    out = []
    for _ in range(40):                                  # inside, any direction
        out.append((rng.randrange(320), rng.randrange(200), rng.randrange(320), rng.randrange(200)))
    for _ in range(40):                                  # through and past the edges
        out.append((rng.randrange(-600, 900), rng.randrange(-500, 700), rng.randrange(-600, 900),
                    rng.randrange(-500, 700)))
    for _ in range(10):                                  # long ones
        out.append((rng.randrange(-15000, 15000), rng.randrange(-15000, 15000),
                    rng.randrange(-15000, 15000), rng.randrange(-15000, 15000)))
    cx, cy = 160, 100
    for dx, dy in ((50, 10), (10, 50), (-10, 50), (-50, 10), (-50, -10), (-10, -50), (10, -50), (50, -10),
                   (30, 30), (-30, 30), (30, -30), (-30, -30), (1, 0), (0, 1), (1, 1), (-1, 1)):
        out.append((cx, cy, cx + dx, cy + dy))          # the eight octants and the diagonals
    for x1, x2 in ((0, 319), (319, 0), (1, 2), (1, 1), (2, 2), (2, 3), (3, 8), (4, 9), (5, 5), (100, 103)):
        out.append((x1, 77, x2, 77))                     # horizontal: odd and even starts and lengths
    for y1, y2 in ((0, 199), (199, 0), (5, 5), (10, 11), (11, 10)):
        out.append((33, y1, 33, y2))                     # vertical and single points
    out += [(-32768, 100, 32767, 100), (32767, 40, -32768, 40), (100, -32768, 100, 32767),
            (200, 32767, 200, -32768), (-10, -10, -5, -5), (400, 300, 500, 250), (-20, 100, 30, 100),
            (300, 50, 330, 50), (50, -5, 50, 5), (60, 195, 60, 205), (-30, -20, 340, 220), (340, -20, -30, 220),
            (-1, 0, 319, 200), (0, -1, 320, 199), (160, -100, 160, 300), (-100, 100, 400, 100)]
    return out


for clip in CLIPS:
    t.call("SetLineClip", *clip)
    for draw_page in (0, 2):
        t.set_draw_page(draw_page)
        for x1, y1, x2, y2 in lines():
            t.call("DrawLine", x1, y1, x2, y2, rng.randrange(0x10000))
t.call("SetLineClip", 0, 0, 319, 199)
t.set_draw_page(6)
for x1, y1, x2, y2 in lines()[:40]:
    t.call("DrawLine", x1, y1, x2, y2, rng.randrange(256))

# ---- FillRect
for draw_page in (0, 2, 6):
    t.set_draw_page(draw_page)
    for _ in range(80):
        t.call("FillRect", pick(EDGE, 0, 320), pick(EDGE, 0, 200), pick(EDGE, 0, 320), pick(EDGE, 0, 200),
               rng.randrange(0x10000))
    for x1, y1, x2, y2 in ((0, 0, 319, 199), (5, 5, 5, 5), (10, 10, 9, 20), (10, 20, 30, 19), (-5, -5, -1, -1),
                           (318, 198, 319, 199), (319, 199, 320, 200), (100, 50, 0xFFFF, 60), (0, 0, 0, 0)):
        t.call("FillRect", x1, y1, x2, y2, rng.randrange(0x10000))

# ---- XorRect
for draw_page in (0, 2, 6):
    t.set_draw_page(draw_page)
    for a in range(8):                                   # every starting and ending bit
        for b in range(8):
            for groups in (0, 1, 2):
                x1 = 8 * rng.randrange(1, 30) + a
                x2 = x1 - a + 8 * groups + b
                t.call("XorRect", x1, 10 + a, x2, 12 + b, rng.randrange(256))
    for w in range(1, 25):
        x1 = rng.randrange(0, 290)
        t.call("XorRect", x1 + w - 1, 150, x1, 151 + w % 3, rng.randrange(0x10000))
    for _ in range(40):
        t.call("XorRect", pick(EDGE, 0, 320), pick(EDGE, 0, 200), pick(EDGE, 0, 320), pick(EDGE, 0, 200),
               rng.randrange(0x10000))
    t.call("XorRect", 0, 0, 319, 199, 0xFF)
    t.call("XorRect", 319, 199, 0, 0, 0x5A)
    t.call("XorRect", -100, -100, 1000, 1000, 1)

sys.exit(t.report())

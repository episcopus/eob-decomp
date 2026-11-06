"""MCGA.OVL's sprite cutters and text in C (port/drivers/mcga_cell.c) against the original's (playbook 80,
M4): each called by hybrid.Direct on the bench of mcga_bench.py, the original then the C on the same
state, comparing memory, the screen and DX:AX.

  - GetSpriteCellRle (0C8D) and GetSpriteCell (0DD9), cutting from the page after the draw page (draw
    pages 0, 1 and 2): pages of random pixels, of few colours (fewer than 16, exactly 16, 17 and more,
    one), of zero runs short, of 255, 256, 257 and over 511 bytes (the RLE split and its truncation), all
    zero; cells at each edge of the page and past it (col 39 and past, y 199 and past, cols and rows 1,
    40, 200, the rows 0 that make 256, cols 0 that make a 65536-byte row); odd pixel counts. Each block is
    given back with MemFree (on the bench as well), so the game's heap stays as it was; the MemFree calls
    are compared too. The cutters' MemAlloc runs with DS as the caller's.
  - DrawMaskedCell16 (0F56): random sheets, cell numbers 0 to 1023 (the << 7 and the sheet's offset
    wrap; no cell reaches the calls' stack, 9000:FF00 up, where the original's pushes differ from the
    C's), every column and line edge, draw pages 0 and 2, lines past the row table.
  - DrawText (2543) with the current font as the game has it (ds:FE4E) and fonts built here (widths 0 to
    25 and 255, heights 0, 1, 8, 16, 200, 201): every character 1 to 255 (the & 7Fh), CR (0Dh), lines
    that wrap at x 320, text running off the bottom, x and y negative, 0, at and past the edges,
    colour and background 0 and not, draw pages 0 and 2, an empty string.

Exit status 1 if a call differs or does not run.

  mcga_cell.py [LIB]
"""
import struct
import sys

from mcga_bench import Driver, DG, SCRATCH, CUR_FONT

t = Driver(sys.argv[1] if len(sys.argv) > 1 else None)
rng = t.rng


def cut(name, col, y, cols, rows):
    r = t.call(name, col, y, cols, rows, t.far(SCRATCH, 0x7000))
    v = r.get("value")
    if v:
        f = t.d.call("MemFree", (v, 4))
        t.seen[("MemFree", f["status"][:40])] += 1
        if f["status"] != "same":
            t.bad.append(dict(f, args=[hex(v)]))
    return r


def page_bytes(kind):
    if kind == "random":
        return bytes(rng.randrange(256) for _ in range(64000))
    if kind == "few":                                   # fewer than 16 colours
        pal = [rng.randrange(256) for _ in range(rng.randrange(1, 12))]
        return bytes(rng.choice(pal) for _ in range(64000))
    if kind == "sixteen":
        pal = rng.sample(range(256), 16)
        return bytes(rng.choice(pal) for _ in range(64000))
    if kind == "seventeen":
        pal = rng.sample(range(256), 17)
        return bytes(rng.choice(pal) for _ in range(64000))
    if kind == "one":
        return bytes([rng.randrange(1, 256)]) * 64000
    if kind == "zero":
        return bytes(64000)
    if kind == "runs":                                  # zero runs of every length class
        out = bytearray()
        while len(out) < 64000:
            n = rng.choice((1, 2, 3, 7, 254, 255, 256, 257, 300, 510, 511, 512, 600, 1000))
            out += bytes(n)
            out += bytes(rng.randrange(1, 256) for _ in range(rng.randrange(1, 6)))
        return bytes(out[:64000])
    if kind == "sparse":                                # mostly zero, a few colours
        return bytes(rng.randrange(1, 256) if rng.random() < 0.05 else 0 for _ in range(64000))
    raise ValueError(kind)


SIZES = ((1, 1), (1, 8), (2, 16), (5, 7), (40, 1), (40, 200), (3, 200), (39, 199), (1, 0), (2, 0),
         (8, 33), (0x101, 2), (0x328, 0x102), (100, 3), (255, 2), (64, 1))     # low bytes only: 0x101 is 1 col
PLACES = ((0, 0), (39, 199), (39, 0), (0, 199), (20, 100), (35, 180), (40, 0), (41, 50), (0, 200),
          (0, 201), (10, 0xFFFF), (0xFFFF, 10), (100, 150))

# ---- the cutters
for draw in (1, 0, 2):
    t.set_draw_page(draw)
    for kind in ("random", "few", "sixteen", "seventeen", "one", "zero", "runs", "sparse"):
        t.fill_page(draw + 1, page_bytes(kind))
        for name in ("GetSpriteCellRle", "GetSpriteCell"):
            for _ in range(4):
                col, y = rng.choice(PLACES)
                cols, rows = rng.choice(SIZES)
                cut(name, col, y, cols, rows)
            cut(name, rng.randrange(40), rng.randrange(200), rng.randrange(1, 41), rng.randrange(1, 201))
    t.d.clear_logs()

t.set_draw_page(0)
t.fill_page(1, page_bytes("runs"))
for name in ("GetSpriteCellRle", "GetSpriteCell"):
    for col, y in ((0, 0), (39, 199), (13, 77)):
        for cols, rows in ((1, 1), (2, 3), (3, 1), (40, 200), (1, 0), (100, 4), (255, 1)):
            cut(name, col, y, cols, rows)
t.fill_page(1, page_bytes("zero"))
cut("GetSpriteCellRle", 0, 0, 0, 1)                     # a 65536-byte row of zeros: one run, its length 0
cut("GetSpriteCellRle", 5, 5, 0, 3)
cut("GetSpriteCell", 0, 0, 0, 1)                        # 65536 pixels: 32768 bytes
t.fill_page(1, page_bytes("few"))
cut("GetSpriteCell", 3, 9, 0, 1)

# ---- DrawMaskedCell16
for draw in (0, 2):
    t.set_draw_page(draw)
    t.fill_page(draw)
    for _ in range(25):
        sheet = rng.choice((0x80, 0x100, 0x4000, 0x7000))   # a cell never reaches the calls' stack at
        t.write(SCRATCH, sheet, bytes(rng.randrange(256) for _ in range(0x1000)))   # 9000:FF00 and up
        for _ in range(3):
            mask = rng.choice((0, 1, 7, 31, rng.randrange(32), 0x1FF, 0x200 + rng.randrange(32)))
            cell = rng.choice((0, 1, 7, 31, rng.randrange(32), 0x1FF))
            col, y = rng.choice(((0, 0), (38, 184), (39, 199), (20, 100), (rng.randrange(39), rng.randrange(185)),
                                 (39, 184), (0, 185), (37, 199)))
            t.call("DrawMaskedCell16", mask, cell, t.far(SCRATCH, sheet), col, y)
t.write(SCRATCH, 0, bytes(rng.randrange(256) for _ in range(0x1000)))
t.call("DrawMaskedCell16", 0x1FF, 0x1FE, t.far(SCRATCH, 0x1000), 3, 4)    # the cell offsets wrap to 0F80, 0F00
t.call("DrawMaskedCell16", 0x200, 0x3FF, t.far(SCRATCH, 0x180), 3, 4)     # to 0180, 0100
t.write(SCRATCH, 0x2000, bytes([0x00, 0xF0, 0x0F, 0xFF] * 32 * 2))
t.write(SCRATCH, 0x2100, bytes(range(256)) * 2)
for mask in (0x40, 0x41):
    for cell in (0x42, 0x43):
        t.call("DrawMaskedCell16", mask, cell, t.far(SCRATCH, 0), 5, 5)
t.set_draw_page(0)
for y in (200, 201, 255, 0x7FFF, 0xFFFF):               # gRowOffsets read past its 201 words
    t.call("DrawMaskedCell16", 0, 1, t.far(SCRATCH, 0x100), 1, y)

# ---- DrawText
game_font = t.read(DG, CUR_FONT, 4)
TEXT = 0x6000


def text(s, off=TEXT):
    t.write(SCRATCH, off, s + b"\0")
    return t.far(SCRATCH, off)


def draw(s, x, y, colour, back):
    return t.call("DrawText", text(s), x, y, colour, back)


def build_font(width, height, off=0x3000):
    """a font at SCRATCH:off: 128 glyph offsets at +2, the height at +102h, the width at +103h, the glyphs
    after (rows of (width + 7) / 8 bytes: four glyphs, one blank, shared by the 128 characters)"""
    row = max(1, (min(width, 24) + 7) // 8)
    size = row * (height if height else 256)
    data = bytearray(0x104)
    glyphs = bytes(rng.randrange(256) for _ in range(3 * size)) + bytes(size)
    for c in range(128):                                # four glyphs, one of them blank
        struct.pack_into("<H", data, 2 + 2 * c, 0x104 + (c % 4) * size)
    data[0x102], data[0x103] = height, width
    t.write(SCRATCH, off, bytes(data) + bytes(glyphs))
    t.write(DG, CUR_FONT, struct.pack("<HH", off, SCRATCH))


ALL = bytes(range(1, 256))
XS = (0, 1, -1, -100, 7, 300, 312, 313, 314, 319, 320, 321, 0x7FFF, -32768)
YS = (0, 1, -1, -50, 150, 190, 192, 193, 199, 200, 201, 0x7FFF, -32768)
for page in (0, 2):
    t.set_draw_page(page)
    t.fill_page(page)
    t.write(DG, CUR_FONT, game_font)
    for k in range(0, len(ALL), 40):
        draw(ALL[k:k + 40], rng.randrange(-10, 330), rng.randrange(-10, 205), rng.randrange(256),
             rng.choice((0, rng.randrange(256))))
    draw(ALL, 0, 0, 15, 0)
    draw(ALL, 0, 100, 0, 9)
    draw(ALL, 13, 7, 0, 0)
    draw(b"", 10, 10, 1, 2)
    draw(b"\r", 10, 10, 1, 2)
    draw(b"AB\rCD\r\rEF", 100, 50, 7, 8)
    draw(b"\r" * 40 + b"X", 10, 10, 1, 2)               # CR past the bottom, then a character
    draw(b"wrap " * 30, 250, 20, 12, 0)
    draw(b"bottom " * 80, 0, 180, 12, 3)
    for _ in range(25):
        n = rng.randrange(1, 30)
        s = bytes(rng.choice(ALL) for _ in range(n))
        draw(s, rng.choice(XS), rng.choice(YS), rng.randrange(256), rng.choice((0, rng.randrange(256))))
    for x in XS:
        draw(b"Hi\rthere", x, 100, 5, 6)
    for y in YS:
        draw(b"Hi\rthere", 100, y, 5, 0)
    for width in tuple(range(0, 26)) + (255,):
        for height in (1, 8, 16):
            build_font(width, height)
            draw(bytes(rng.choice(ALL) for _ in range(12)) + b"\r" + ALL[60:70], rng.choice((0, 3, 290)),
                 rng.choice((0, 50, 190)), rng.randrange(1, 256), rng.choice((0, rng.randrange(1, 256))))
    t.d.clear_logs()
t.set_draw_page(0)
for width, height in ((8, 0), (6, 200), (6, 201), (9, 199), (24, 100)):
    build_font(width, height)
    draw(b"Ab\rc", 0, 0, 3, 4)
    draw(b"Ab\rc", 0, 1, 3, 0)
t.write(DG, CUR_FONT, game_font)

sys.exit(t.report())

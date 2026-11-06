"""MCGA.OVL's entry, link and small exports in C (port/drivers/mcga.c) against the original's (playbook 80,
M4): each called by hybrid.Direct on the bench of mcga_bench.py, the original then the C on the same
state, comparing memory, the screen and the registers each answers in.

  - DriverEntry (AX, BX and the carry) and LinkDriver on tables of imports and exports, one with a
    missing export (the carry);
  - CopyPage between pages; ClearPage on each page; SwapPageWithScreen;
  - GetPixel and PutPixel inside, on and past every edge, negative coordinates, each draw page;
  - SaveDrawState and RestoreDrawState in sequences; SetRegion, SetLineClip, SetRemapTable, SetPalette
    and SaveRectSize with random and edge values; CopyToPlanar.

Exit status 1 if a call differs or does not run.

  mcga.py [LIB]
"""
import sys

from mcga_bench import Driver, DG, SCRATCH

t = Driver(sys.argv[1] if len(sys.argv) > 1 else None)
rng = t.rng
EDGE = (-32768, -2, -1, 0, 1, 7, 8, 39, 40, 41, 199, 200, 201, 318, 319, 320, 321, 32767, 0xFFFF)

# ---- the pages
for page in (0, 2, 4, 6, 3, 7):
    t.fill_page(page)
t.call("CopyPage", t.page_seg(2), t.page_seg(4))
t.call("CopyPage", t.page_seg(0), t.page_seg(6))
t.call("CopyPage", t.page_seg(4), t.page_seg(0))
t.call("CopyPage", t.page_seg(2), t.page_seg(2))
for page in (0, 2, 4, 6, 1, 0x21, 0xFFE4):             # the page number is taken AND 1Fh
    t.set_draw_page(page)
    t.call("ClearPage")
    t.fill_page(page & 0x1F)
for page in (2, 4, 0, 0x22):
    t.call("SwapPageWithScreen", page)

# ---- pixels
for page in (0, 2, 4):
    t.set_draw_page(page)
    for _ in range(60):
        x = rng.choice(EDGE + (rng.randrange(320),))
        y = rng.choice(EDGE + (rng.randrange(200),))
        t.call("GetPixel", x, y)
        t.call("PutPixel", x, y, rng.randrange(0x10000))
    for x, y in ((0, 0), (319, 199), (319, 0), (0, 199)):
        t.call("PutPixel", x, y, rng.randrange(256))
        t.call("GetPixel", x, y)

# ---- the region, the clip rectangle, the remap table, the palette, the sizes
for _ in range(10):
    t.call("SetRegion", *(rng.choice(EDGE + (rng.randrange(0x10000),)) for _ in range(4)))
    for _ in range(rng.randrange(1, 4)):
        t.call(rng.choice(("SaveDrawState", "RestoreDrawState")))
t.call("SetRegion", 0, 40, 0, 200)
t.call("SaveDrawState")
t.call("RestoreDrawState")
for _ in range(6):
    t.call("SetLineClip", *(rng.choice(EDGE + (rng.randrange(0x10000),)) for _ in range(4)))
t.call("SetLineClip", 0, 0, 319, 199)
for _ in range(5):
    t.write(SCRATCH, 0x100, bytes(rng.randrange(256) for _ in range(16)))
    t.call("SetRemapTable", t.far(SCRATCH, 0x100))
t.call("SetRemapTable", t.far(SCRATCH, 0xFFF8))           # across the segment's end
for _ in range(3):
    t.write(SCRATCH, 0x200, bytes(rng.randrange(64) for _ in range(768)))
    t.call("SetPalette", t.far(SCRATCH, 0x200))
for cols in EDGE[:12] + (rng.randrange(0x10000),):
    for rows in (0, 1, 8, 199, 200, 201, -1, 0x7FFF):
        t.call("SaveRectSize", cols, rows)
t.call("CopyToPlanar", t.far(SCRATCH, 0), 0x4000, 8000)

# ---- entry and link
t.call("DriverEntry", 0x27, 0x1A16, t.seg)
table = 0x9400
real = t.read(DG, 0x802A, 47 * 4)
t.write(DG, table, real)
t.call("LinkDriver", t.far(DG, table))                   # the game's table as linked: all imports
for _ in range(4):
    slots = bytearray(real)
    for i in range(7, 47):                               # some exports asked for, some given
        if rng.random() < 0.5:
            slots[4 * i:4 * i + 4] = bytes(4)
    t.write(DG, table, slots)
    t.call("LinkDriver", t.far(DG, table))
saved = t.read(t.seg, 0x001F + 4 * 12, 4)
t.write(t.seg, 0x001F + 4 * 12, bytes(4))                # an export the driver does not have
slots = bytearray(real)
slots[4 * 19:4 * 20] = bytes(4)
t.write(DG, table, slots)
t.call("LinkDriver", t.far(DG, table))
t.write(t.seg, 0x001F + 4 * 12, saved)
t.write(t.seg, 0x0003, real[:28])                        # the game's imports back in the driver

sys.exit(t.report())

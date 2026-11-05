import sys; sys.path.insert(0, '.')
from figlib import *

c = Canvas(760, 420)
c.section(0, 11, "THE PAGE TABLE gPageSegs (ds:FC0C): EIGHT SEGMENT WORDS, FOUR PAIRS, FILLED PER GRAPHICS CHOICE")

LX = 176        # first data column
CW = 146        # column width
RH = 34         # row height
Y0 = 50
heads = ["0  CGA", "1  Tandy", "2  EGA", "3  VGA / MCGA"]
for i, h in enumerate(heads):
    c.text(LX + i * CW + CW / 2 - 2, Y0 - 10, h, size=11.5, weight=700, color=INK, anchor="middle")
roles = [
    ("0", "screen"), ("1", "screen twin"), ("2", "page 2, draw target"), ("3", "art page"),
    ("4", "page 4"), ("5", "staging page"), ("6", "page 6"), ("7", "wall blocks"),
]
for r, (n, t) in enumerate(roles):
    y = Y0 + r * RH
    c.text(0, y + RH / 2 + 4, "slot " + n, size=11, weight=700, color=INK, mono=False)
    c.text(42, y + RH / 2 + 4, t, size=10.5, color=INK2)
V, M = "teal", "ochre"
def cell(col, r0, nr, kind, l1, l2=None):
    x = LX + col * CW; y = Y0 + r0 * RH
    tc, sc, fc = PAL[kind]
    c.rect(x + 1, y + 1, CW - 2, nr * RH - 2, fill=fc, stroke=sc, sw=1.1, rx=3)
    cy = y + nr * RH / 2
    if l2:
        c.text(x + CW / 2, cy - 1, l1, size=11, weight=700, color=tc, anchor="middle")
        c.text(x + CW / 2, cy + 12, l2, size=9.5, color=INK2, anchor="middle")
    else:
        c.text(x + CW / 2, cy + 4, l1, size=11, weight=700, color=tc, anchor="middle")
# CGA
cell(0, 0, 2, V, "B800", "screen, 16 KB lines")
cell(0, 2, 2, M, "64,000 bytes", "one segment")
cell(0, 4, 2, M, "32,000 bytes", "one segment")
cell(0, 6, 2, M, "52,000 bytes", "one segment")
# Tandy
cell(1, 0, 2, V, "B800", "screen, banked")
cell(1, 2, 2, M, "memtop", "below video [H]")
cell(1, 4, 2, M, "32,576 bytes", "one segment")
cell(1, 6, 2, M, "52,000 bytes", "one segment")
# EGA
cell(2, 0, 1, V, "A000", "page 0, visible")
cell(2, 1, 1, "grey", "not set", "stays 0")
cell(2, 2, 1, V, "A200", "video RAM page")
cell(2, 3, 1, M, "64,000 bytes")
cell(2, 4, 1, V, "A200")
cell(2, 5, 1, M, "32,000 bytes")
cell(2, 6, 1, V, "A200")
cell(2, 7, 1, M, "52,000 bytes")
# VGA
cell(3, 0, 2, V, "A000", "64,000 bytes, mode 13h")
cell(3, 2, 2, M, "65,000 bytes", "one segment")
cell(3, 4, 2, M, "32,000 bytes", "one segment")
cell(3, 6, 2, M, "52,000 bytes", "one segment")
c.legend(LX, Y0 + 8 * RH + 18, [("teal", "video memory (even slot)"), ("ochre", "main memory from MemAlloc (odd slot)")])
# rules
ry = Y0 + 8 * RH + 40
for t in [
    "GetPageBuffer(k) is the far pointer to offset 0 of gPageSegs[k | 1]; the drivers turn a page number n into gPageSegs[n and 1Fh].",
    "In the CGA, Tandy and VGA tables the two slots of a pair name the same segment, so copying page 3 to page 2 is skipped (CopyPage tests that the segments differ). EGA is the only mode where page 2 (A200, planar video RAM) and the buffer in slot 3 are different memory.",
]:
    for l in wrap(t, 750, 10.5):
        c.text(0, ry, l, size=10.5, color=INK2); ry += 13.5
    ry += 3
c.h = ry + 2
c.save("figs/gfx_pages.svg")

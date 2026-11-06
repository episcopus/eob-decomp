import sys; sys.path.insert(0, '.')
from figlib import *

# BuildViewBlockMap (ovl4:044B): the rectangles it copies into the 22 x 15 block map, in code order.
P = [  # (slot, kind, x, y, w, h, row)   kind: S side wall (left/right), F front wall
    (0, "S", -2, 3, 3, 5, 0), (6, "S", 21, 3, 3, 5, 0),
    (1, "S", 2, 3, 3, 5, 0), (5, "S", 17, 3, 3, 5, 0),
    (2, "S", 8, 3, 1, 5, 0), (4, "S", 13, 3, 1, 5, 0),
    (1, "F", -4, 3, 6, 5, 0), (5, "F", 20, 3, 6, 5, 0), (2, "F", 2, 3, 6, 5, 0), (4, "F", 14, 3, 6, 5, 0), (3, "F", 8, 3, 6, 5, 0),
    (7, "S", 0, 3, 2, 6, 1), (11, "S", 20, 3, 2, 6, 1), (8, "S", 6, 2, 2, 8, 1), (10, "S", 14, 2, 2, 8, 1),
    (8, "F", -4, 2, 10, 8, 1), (10, "F", 16, 2, 10, 8, 1), (9, "F", 6, 2, 10, 8, 1),
    (12, "S", 3, 1, 3, 12, 2), (14, "S", 16, 1, 3, 12, 2),
    (12, "F", -13, 1, 16, 12, 2), (14, "F", 19, 1, 16, 12, 2), (13, "F", 3, 1, 16, 12, 2),
    (15, "S", 0, 0, 3, 15, 3), (17, "S", 19, 0, 3, 15, 3),
]
kinds = ["grey", "teal", "ochre", "plum"]
rowname = ["3 ahead", "2 ahead", "1 ahead", "own row"]
B = 21
GX, GY = 16, 30
c = Canvas(760, GY + 15 * B + 36)
c.section(0, 11, "WHERE THE WALL PIECES LAND IN THE 22 x 15 BLOCK MAP (EACH SQUARE 8 x 8 PIXELS)")
c.rect(GX, GY, 22 * B, 15 * B, fill="#F1EDE2", stroke=GREY, sw=1, rx=0)
for gx in range(1, 22):
    c.line(GX + gx * B, GY, GX + gx * B, GY + 15 * B, stroke="#E2DCCB", sw=0.5)
for gy in range(1, 15):
    c.line(GX, GY + gy * B, GX + 22 * B, GY + gy * B, stroke="#E2DCCB", sw=0.5)
for slot, kind, x, y, w, h, row in P:
    k = kinds[row]
    tc, sc, fc = PAL[k]
    x0 = max(x, 0); x1 = min(x + w, 22)
    if x1 <= x0:
        continue
    rx, ry, rw, rh = GX + x0 * B, GY + y * B, (x1 - x0) * B, h * B
    if kind == "F":
        c.rect(rx, ry, rw, rh, fill=SOLID[k], stroke="none", sw=0, rx=0, opacity=0.10)
        c.rect(rx, ry, rw, rh, fill="none", stroke=SOLID[k], sw=1.8, rx=0)
        c.text(rx + 4, ry + 13, "F%d" % slot, size=11, weight=700, color=tc)
    else:
        c.rect(rx, ry, rw, rh, fill=SOLID[k], stroke="none", sw=0, rx=0, opacity=0.22)
        c.rect(rx, ry, rw, rh, fill="none", stroke=SOLID[k], sw=1.1, rx=0, dash="4 2")
        if rw >= 30:
            c.text(rx + 4, ry + rh - 5, "S%d" % slot, size=10.5, weight=700, color=tc)
        elif slot not in (0, 6):
            c.text(rx + rw / 2, ry + rh + 11, "S%d" % slot, size=9.5, weight=700, color=tc, anchor="middle")
for gx in (0, 5, 10, 15, 20):
    c.text(GX + gx * B + B / 2, GY - 4, str(gx), size=9.5, color=MUTED, anchor="middle")
for gy in (0, 5, 10):
    c.text(GX - 1, GY + gy * B + B / 2 + 3, str(gy), size=9.5, color=MUTED, anchor="end")

RX = GX + 22 * B + 24
yy = GY + 2
c.section(RX, yy, "READING IT", color="#14504A"); yy += 18
c.legend(RX, yy, [("grey", "3 ahead"), ("teal", "2 ahead")]); yy += 16
c.legend(RX, yy, [("ochre", "1 ahead"), ("plum", "own row")]); yy += 20
c.text(RX, yy, "solid frame: front wall (F) of a slot", size=10.5, color=INK2); yy += 14
c.text(RX, yy, "dashed frame: side wall (S) of a slot", size=10.5, color=INK2); yy += 20
for t in [
    "Pieces are copied in the order of the rows above, far to near, sides before fronts in each row. A later copy overwrites an earlier one; zero words are skipped, so a hole in a nearer piece shows what lies behind it.",
    "Left-hand sides are copied; right-hand sides are mirrored copies (columns reversed, bit 4000h toggled) at the mirror position. Front walls are copied only.",
    "Pieces past the edge are clipped by column. Slots 0 and 6 have no front piece, and neither has the party's row: the wall straight ahead is F13, 128 by 96 pixels.",
]:
    for l in wrap(t, 232, 10.5):
        c.text(RX, yy, l, size=10.5, color=INK2); yy += 13.2
    yy += 5
c.h = max(c.h, yy)
c.save("figs/gfx_view_blocks.svg")

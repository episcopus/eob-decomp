import sys; sys.path.insert(0, '.')
from figlib import *

c = Canvas(760, 360)
c.section(0, 11, "A .VMP FILE: 2,916 WORDS = ONE BACKGROUND + SIX WALL SETS")

# top bar, to scale: 330 + 6 * 431 words
W = 756
tot = 2916
def mx(w): return 2 + W * w / tot
y = 30
c.rect(mx(0), y, mx(330) - mx(0), 30, fill=PAL["ochre"][2], stroke=PAL["ochre"][1], sw=1.2, rx=0)
c.text((mx(0) + mx(330)) / 2, y + 13, "background", size=11.5, weight=700, color=PAL["ochre"][0], anchor="middle")
c.text((mx(0) + mx(330)) / 2, y + 25, "22 x 15 words", size=10.5, color=INK2, anchor="middle")
kinds = ["teal", "navy", "teal", "navy", "teal", "navy"]
for i in range(6):
    a = 330 + 431 * i
    b = a + 431
    k = kinds[i]
    c.rect(mx(a), y, mx(b) - mx(a), 30, fill=PAL[k][2], stroke=PAL[k][1], sw=1.2, rx=0)
    c.text((mx(a) + mx(b)) / 2, y + 13, "set %d" % (i + 1), size=11.5, weight=700, color=PAL[k][0], anchor="middle")
    c.text((mx(a) + mx(b)) / 2, y + 25, "431 words", size=10.5, color=INK2, anchor="middle")
for w, lab in [(0, "0"), (330, "330"), (761, "761"), (2916, "2916")]:
    c.text(mx(w), y - 4, lab, size=10, color=MUTED, anchor="middle" if 0 < w < 2916 else ("start" if w == 0 else "end"), mono=True)
c.text(mx(330) + 4, y + 46, "gWallVmpSet[type] picks set 1 to 6 (0: the wall type has no wall graphics); the background is always the one at the start", size=10.5, color=MUTED)

# zoom: one set as 9 rectangles
pieces = [  # (offset, w, h, label, used)
    (0, 3, 15, "3 x 15", "side, party row (slots 15, 17)"),
    (45, 3, 12, "3 x 12", "side, 1 ahead (12, 14)"),
    (81, 2, 8, "2 x 8", "side, 2 ahead inner (8, 10)"),
    (97, 1, 5, "1 x 5", "side, 3 ahead inner (2, 4)"),
    (102, 3, 5, "3 x 5", "side, 3 ahead outer (0, 1, 5, 6)"),
    (117, 2, 6, "2 x 6", "side, 2 ahead outer (7, 11)"),
    (129, 6, 5, "6 x 5", "front, 3 ahead (1 to 5)"),
    (159, 10, 8, "10 x 8", "front, 2 ahead (8, 9, 10)"),
    (239, 16, 12, "16 x 12", "front, 1 ahead (12, 13, 14)"),
]
y2 = 150
c.section(0, y2 - 40, "ONE SET (431 WORDS) SPLITS INTO NINE RECTANGLES OF BLOCK WORDS")
c.text(0, y2 - 22, "word offset inside the set, to scale", size=10.5, color=MUTED)
for i, (o, w, h, lab, used) in enumerate(pieces):
    k = "navy" if "side" in used else "teal"
    x0 = 2 + W * o / 431
    x1 = 2 + W * (o + w * h) / 431
    c.rect(x0, y2, x1 - x0, 34, fill=PAL[k][2], stroke=PAL[k][1], sw=1.1, rx=0)
    if x1 - x0 > 40:
        c.text((x0 + x1) / 2, y2 + 21, lab, size=11, weight=700, color=PAL[k][0], anchor="middle")
    if x1 - x0 > 24 or o in (0,):
        c.text(x0 + 1, y2 + 46, "%d" % o, size=9.5, color=MUTED, mono=True)
c.text(2 + W, y2 + 46, "431", size=9.5, color=MUTED, mono=True, anchor="end")

# table under: one row per piece
ty = y2 + 66
c.text(0, ty, "rectangle", size=10.5, weight=700, color=INK)
c.text(200, ty, "x2", size=10.5, weight=700, color=INK)
c.text(250, ty, "words", size=10.5, weight=700, color=INK)
c.text(300, ty, "pixels", size=10.5, weight=700, color=INK)
c.text(380, ty, "copied at (x, y), in blocks", size=10.5, weight=700, color=INK)
c.text(620, ty, "mirrored copy at", size=10.5, weight=700, color=INK)
c.line(0, ty + 5, 758, ty + 5, stroke=RULE)
left = {
    0: ("(0, 0)", "(19, 0)"), 45: ("(3, 1)", "(16, 1)"), 81: ("(6, 2)", "(14, 2)"),
    97: ("(8, 3)", "(13, 3)"), 102: ("(-2, 3), (2, 3)", "(21, 3), (17, 3)"),
    117: ("(0, 3)", "(20, 3)"), 129: ("(-4, 3) (2, 3) (8, 3) (14, 3) (20, 3)", "none"),
    159: ("(-4, 2) (6, 2) (16, 2)", "none"), 239: ("(-13, 1) (3, 1) (19, 1)", "none"),
}
for i, (o, w, h, lab, used) in enumerate(pieces):
    yy = ty + 19 + i * 17
    c.text(0, yy, used, size=10.5, color=INK2)
    c.text(200, yy, "0x%02X" % o, size=10.5, color=INK2, mono=True)
    c.text(250, yy, "%d" % (w * h), size=10.5, color=INK2, mono=True)
    c.text(300, yy, "%d x %d" % (w * 8, h * 8), size=10.5, color=INK2, mono=True)
    l, r = left[o]
    c.text(380, yy, l, size=10.5, color=INK2, mono=True)
    c.text(620, yy, r if o != 129 else "none", size=10.5, color=INK2, mono=True)
c.h = ty + 19 + 9 * 17 + 6
c.save("figs/gfx_vmp_layout.svg")

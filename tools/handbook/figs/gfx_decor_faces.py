import sys; sys.path.insert(0, '.')
from figlib import *

c = Canvas(760, 400)
c.section(0, 11, "WALL DECORATIONS: TEN POSITIONS IN A RECORD, TWO FACES PER SLOT")

# (A face = gWallFaces[2s], drawn second; B face = gWallFaces[2s+1], drawn first)
faces = {
    0: (None, None), 1: (("F", 3), ("R", 9)), 2: (("F", 3), ("R", 7)), 3: (("F", 3), None), 4: (("F", 3), ("L", 7)),
    5: (("F", 3), ("L", 9)), 6: (None, None),
    7: (("F", 2), ("R", 8)), 8: (("F", 2), ("R", 6)), 9: (("F", 2), None), 10: (("F", 2), ("L", 6)), 11: (("F", 2), ("L", 8)),
    12: (("F", 1), ("R", 5)), 13: (("F", 1), None), 14: (("F", 1), ("L", 5)),
    15: (None, ("R", 4)), 16: (("F", 0), None), 17: (None, ("L", 4)),
}
rows = [[0, 1, 2, 3, 4, 5, 6], [7, 8, 9, 10, 11], [12, 13, 14], [15, 16, 17]]
rl = ["3 ahead", "2 ahead", "1 ahead", "own row"]
W, H = 60, 58
X0, Y0 = 62, 30
col = {"F": "teal", "R": "navy", "L": "plum"}
for ri, row in enumerate(rows):
    y = Y0 + ri * (H + 6)
    off = (7 - len(row)) * (W + 4) / 2
    c.text(X0 - 6, y + H / 2 + 4, rl[ri], size=10.5, color=MUTED, anchor="end")
    for i, s in enumerate(row):
        x = X0 + off + i * (W + 4)
        c.rect(x, y, W, H, fill=CARD, stroke=RULE, sw=1, rx=5)
        c.text(x + W / 2, y + 13, "slot %d" % s, size=10, weight=700, color=INK, anchor="middle")
        A, B = faces[s]
        for j, (lab, f) in enumerate([("B", B), ("A", A)]):
            yy = y + 20 + j * 18
            if f is None:
                c.text(x + W / 2, yy + 11, "no face", size=9, color=MUTED, anchor="middle")
                continue
            k = col[f[0]]
            c.rect(x + 5, yy, W - 10, 15, fill=PAL[k][2], stroke=PAL[k][1], sw=1, rx=3)
            nm = {"F": "front", "R": "right", "L": "left"}[f[0]]
            c.text(x + W / 2, yy + 11, "%s %d" % (nm, f[1]), size=10, weight=600, color=PAL[k][0], anchor="middle")
# legend
LX = X0 + 7 * (W + 4) + 8
c.text(LX, Y0 + 8, "each chip: which side of the", size=10.5, color=INK2)
c.text(LX, Y0 + 21, "cell's wall bytes is read, and", size=10.5, color=INK2)
c.text(LX, Y0 + 34, "which of the ten entries of", size=10.5, color=INK2)
c.text(LX, Y0 + 47, "the record is drawn", size=10.5, color=INK2)
c.text(LX, Y0 + 68, "upper chip: face B (first)", size=10.5, color=MUTED)
c.text(LX, Y0 + 81, "lower chip: face A (second)", size=10.5, color=MUTED)
c.text(LX, Y0 + 102, "left faces use the mirrored", size=10.5, color=MUTED)
c.text(LX, Y0 + 115, "form of the same entry", size=10.5, color=MUTED)

# record layouts
y3 = Y0 + 4 * (H + 6) + 22
c.section(0, y3 - 8, "THE .DAT RECORDS (BRICK.DAT: 60 DECORATIONS, 177 RECTANGLES)")
segs = [("10 bytes", "rectangle index per entry, FFh = none", 200, "teal"), ("1", "next", 38, "grey"), ("1", "flags", 38, "grey"),
        ("10 x i16", "x of each entry", 240, "ochre"), ("10 x i16", "y of each entry", 240, "ochre")]
# scale: 52 bytes over 756
sc = 756 / 52.0
x = 0
for lab, sub, w_, k in [("bytes 0-9", "rectangle index, FFh none", 10, "teal"), ("10", "next", 1, "grey"), ("11", "flags", 1, "grey"),
                        ("12-31", "x[10], words", 20, "ochre"), ("32-51", "y[10], words", 20, "ochre")]:
    ww = w_ * sc
    c.rect(x, y3, ww, 40, fill=PAL[k][2], stroke=PAL[k][1], sw=1.1, rx=0)
    if ww > 60:
        c.text(x + ww / 2, y3 + 16, lab, size=11, weight=700, color=PAL[k][0], anchor="middle")
        c.text(x + ww / 2, y3 + 31, sub, size=10, color=INK2, anchor="middle")
    else:
        c.text(x + ww / 2, y3 + 14, lab, size=9.5, weight=700, color=PAL[k][0], anchor="middle")
    x += ww
c.text(330, y3 + 54, "next: chain to the next record, 0 ends the chain", size=10.5, color=INK2)
c.text(330, y3 + 67, "flags: bit 0 mirrors, bit 1 mirrors on odd parity,", size=10.5, color=INK2)
c.text(330, y3 + 80, "bit 2 picks the x base (face A only)", size=10.5, color=INK2)
c.text(0, y3 + 54, "52 bytes a decoration", size=10.5, color=INK2)
c.text(0, y3 + 67, "then: 8 bytes a rectangle (x, y, w, h)", size=10.5, color=INK2)
c.text(0, y3 + 80, "x, w in 8-px columns; y, h in lines", size=10.5, color=INK2)
c.h = y3 + 92
c.save("figs/gfx_decor_faces.svg")

import sys; sys.path.insert(0, '.')
from figlib import *

c = Canvas(760, 380)
c.section(0, 11, "MONSTER SHAPES: WHICH VIEW, AND HOW BIG")

A0 = [4, -2, 1, 3, 3, 4, -2, 1, 1, 3, 4, -2, -2, 1, 3, 4]
B1 = [4, -3, 1, 2, 2, 4, -3, 1, 1, 2, 4, -3, -3, 1, 2, 4]
A1 = [-4, -3, -1, 2, 2, -4, -3, -1, -1, 2, -4, -3, -3, -1, 2, -4]
tables = [("gMonsterFrames, walk phase 0", A0, 0, 0), ("gMonsterFrames, phase 1", B1, 1, 0),
          ("gMonsterFramesMirrorStep, phase 1", A1, 1, 1)]
cs = 30
def grid(x, y, data, title):
    c.text(x, y - 6, title, size=10.5, weight=700, color=INK)
    c.text(x + 40 + 2 * cs, y + 4 * cs + 16, "monster orient 0..3", size=9.5, color=MUTED, anchor="middle")
    for r in range(4):
        c.text(x + 34, y + r * cs + cs / 2 + 4, "dir %d" % r, size=9.5, color=MUTED, anchor="end")
        for q in range(4):
            v = data[r * 4 + q]
            k = "plum" if v < 0 else "teal"
            c.rect(x + 40 + q * cs, y + r * cs, cs, cs, fill=PAL[k][2], stroke=PAL[k][1], sw=1, rx=0)
            c.text(x + 40 + q * cs + cs / 2, y + r * cs + cs / 2 + 5, "%d" % v, size=12, weight=700, color=PAL[k][0], anchor="middle")
grid(0, 40, A0, "phase 0, both tables")
grid(185, 40, B1, "phase 1, table for most monsters")
grid(370, 40, A1, "phase 1, flag 2 monsters (mirror step)")
c.text(0, 40 + 4 * cs + 38, "index = phase * 16 + party dir * 4 + monster orient. Sprite 1 to 4 pick a drawing, a negative number its mirror image; the attack shapes 5 and 6", size=10.5, color=INK2)
c.text(0, 40 + 4 * cs + 52, "replace the table value in the nearest row when the monster's attack frame byte is FEh or FFh.", size=10.5, color=INK2)

# right: legend text
c.legend(560, 48, [("teal", "as drawn"), ("plum", "mirrored")])
for i, l in enumerate(wrap("Shape 1 is the front, 4 the back, 2 and 3 the side in two walk phases (looked at in KOBOLD.CPS); 5 and 6 are attack poses.", 190, 10.5)):
    c.text(560, 72 + i * 13, l, size=10.5, color=MUTED)

# sizes
y0 = 262
c.section(0, y0 - 14, "ONE SHAPE, SHRUNK ONCE PER ROW OF DISTANCE (BlitSpriteDefault)")
sizes = [("1 ahead (row 2)", 0, 56, 96), ("2 ahead (row 1)", 1, 40, 64), ("3 ahead (row 0)", 2, 32, 43)]
sc = 0.62
bx = 44
base = y0 + 96 * sc + 14
for lab, n, w, h in sizes:
    k = ["plum", "ochre", "teal"][n]
    c.rect(bx, base - h * sc, w * sc, h * sc, fill=PAL[k][2], stroke=PAL[k][1], sw=1.3, rx=0)
    c.text(bx + w * sc / 2, base + 14, lab, size=10.5, weight=700, color=PAL[k][0], anchor="middle")
    c.text(bx + w * sc / 2, base + 27, "%d x %d px" % (w, h), size=10.5, color=INK2, anchor="middle", mono=True)
    c.text(bx + w * sc / 2, base + 40, "%d shrinks" % n, size=10, color=MUTED, anchor="middle")
    bx += 150
tx = 480
yy = y0 + 4
for l in [
    "rows out = 2 * (rows + 1) / 3",
    "columns out = ceil(2 * columns / 3), in 8 pixel columns",
    "per row: copy, copy, skip a row",
    "per 6 pixels: keep pixels 0, 1, 3, 4",
    "",
    "gMonsterShrinkSteps[gViewSlotRow]: 2, 1, 0 for rows 0, 1, 2 (the party's row never draws monsters).",
    "The result goes to a scratch block (gScratchBuf), so the shape sheet is never changed.",
]:
    if l == "":
        yy += 4; continue
    mono = l.startswith(("rows", "col", "per"))
    for ll in wrap(l, 270, 10.5, mono=mono):
        c.text(tx, yy, ll, size=10.5, color=INK2 if mono else MUTED, mono=mono); yy += 13.2
c.h = max(base + 52, yy + 6)
c.save("figs/gfx_monster_shrink.svg")

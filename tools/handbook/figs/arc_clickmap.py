import sys; sys.path.insert(0, '.')
from figlib import *

S = 1.6
OX, OY = 0, 0
c = Canvas(760, 200 * S + 8)
def R(x, y, w, h, kind, fillop=0.55, dash=None, sw=1.1, label=None, lsize=9.5, lcolor=None, fill=None):
    if fillop == 0.0:
        c.rect(OX + x * S, OY + y * S, w * S, h * S, fill='none', stroke=SOLID[kind], sw=sw, rx=0, dash=dash)
    else:
        c.rect(OX + x * S, OY + y * S, w * S, h * S, fill=fill or MID[kind], stroke=SOLID[kind], sw=sw, rx=0, dash=dash, opacity=fillop)
    if label:
        c.text(OX + (x + w / 2) * S, OY + (y + h / 2) * S + 3.5, label, size=lsize, weight=700, color=lcolor or PAL[kind][0], anchor="middle")
# screen
c.rect(0, 0, 320 * S, 200 * S, fill="#FFFDF8", stroke=GREY, sw=1.4, rx=0)
# message window (text window 7)
R(8, 179, 272, 18, "grey", fillop=0.4, dash="4 3", label="text window 7: messages", lsize=9.5)
# view
R(24, 8, 128, 96, "teal", 0.35, label=None)
c.text(OX + 56 * S, OY + 52 * S, "3D view 128 x 96", size=11, weight=700, color=PAL["teal"][0], anchor="middle")
c.text(OX + 56 * S, OY + 52 * S + 13, "click: ClickWallAhead", size=9.2, color=PAL["teal"][0], anchor="middle")
# throw
R(0, 8, 88, 48, "red", 0.0, dash="4 3", sw=1.2)
R(88, 8, 88, 48, "red", 0.0, dash="4 3", sw=1.2)
c.text(OX + 3, OY + 8 * S + 12, "throw", size=9, color=PAL["red"][0])
c.text(OX + 88 * S + 3, OY + 8 * S + 12, "throw", size=9, color=PAL["red"][0])
# floor items
for (x, y, w, h, n) in ((0, 72, 88, 29, 2), (89, 72, 88, 29, 3), (0, 102, 88, 18, 0), (89, 102, 88, 18, 1)):
    R(x, y, w, h, "ochre", 0.45, label="floor %d" % n, lsize=9.5)
# movement
mv = [(3, 128, "turn L", "kp7"), (24, 128, "forward", "kp8"), (45, 128, "turn R", "kp9"),
      (3, 144, "step L", "kp4"), (24, 144, "back", "kp2 kp5"), (45, 144, "step R", "kp6")]
for x, y, t, k in mv:
    R(x, y, 21, 16, "navy", 0.5)
    c.text(OX + (x + 10.5) * S, OY + (y + 8) * S - 1, t, size=8.6, weight=700, color=PAL["navy"][0], anchor="middle")
    c.text(OX + (x + 10.5) * S, OY + (y + 8) * S + 9, k, size=8.2, color=PAL["navy"][0], anchor="middle")
# party panel
cols = [184, 256]
rows_ = [2, 54, 106]
for ri, ry in enumerate(rows_):
    for ci, cx in enumerate(cols):
        n = ri * 2 + ci
        R(cx, ry, 63, 50, "plum", 0.18, sw=1.4)
        R(cx, ry + 8, 33, 33, "plum", 0.45, label="face", lsize=9)
        R(cx + 32, ry + 8, 31, 33, "teal", 0.45, label="hands", lsize=9)
        R(cx, ry, 63, 8, "red", 0.45)
        c.text(OX + (cx + 2) * S, OY + (ry + 6.2) * S, "F%d" % (n + 1), size=8.5, weight=700, color=PAL["red"][0])
# camp
R(289, 177, 31, 21, "ochre", 0.6, label="camp", lsize=10)
c.text(OX + 304.5 * S, OY + 190 * S + 9, "C", size=9, color=PAL["ochre"][0], anchor="middle", weight=700)
# scale ticks
c.text(0, 200 * S + 6, "", size=8)
# legend
LX = 320 * S + 18
c.text(LX, 14, "Regions of node list 1b", size=11.5, weight=700, color=INK)
items = [
    ("navy", "movement arrows", "MoveForward, MoveBackward, MoveLeft, MoveRight, TurnLeft, TurnRight"),
    ("plum", "63 x 50 portrait box", "SelectClickedMember: F1 to F6"),
    ("plum", "face, 33 x 33", "OpenInventory: I"),
    ("teal", "hands, 31 x 33", "ClickHandSlot: left swaps with the item on the pointer, right uses it"),
    ("red", "name strip, 63 x 8", "PickOrSwapMember: right button or Shift+F1 to F6"),
    ("ochre", "floor, four cells", "ClickFloorItem: pick up or drop at 4 spots of 2 cells"),
    ("red", "dashed, 88 x 48 halves", "ThrowMouseItem: throw the pointer's item"),
    ("ochre", "camp icon", "RunCampMenu (ovl1): C"),
]
y = 30
for k, t, b in items:
    c.rect(LX, y - 8, 12, 10, fill=MID[k], stroke=SOLID[k], sw=1, rx=1)
    c.text(LX + 18, y + 1, t, size=10.3, weight=700, color=INK)
    for j, l in enumerate(wrap(b, 205, 9.6)):
        c.text(LX + 18, y + 13 + 11 * j, l, size=9.6, color=INK2)
    y += 13 + 11 * len(wrap(b, 205, 9.6)) + 8
c.save("figs/arc_clickmap.svg")

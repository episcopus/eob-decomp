import sys; sys.path.insert(0, '.')
from figlib import *

c = Canvas(760, 424)
c.section(0, 11, "THE CELL AS FOUR QUARTERS: WHERE THE PARTY STANDS AND WHOM A BLOW REACHES")

# data (ds:1A1B gViewToCellPos, ds:329B gTargetPosOrder), position 0 NW, 1 NE, 2 SW, 3 SE, 4 centre
party = {0: [0, 1, 2, 3], 1: [1, 3, 0, 2], 2: [3, 2, 1, 0], 3: [2, 0, 3, 1]}
target = {0: ([2, 3, 0, 1], [3, 2, 1, 0]), 1: ([0, 2, 1, 3], [2, 0, 3, 1]),
          2: ([1, 0, 3, 2], [0, 1, 2, 3]), 3: ([3, 1, 0, 2], [1, 3, 2, 0])}
names = ["facing north", "facing east", "facing south", "facing west"]
arrows = {0: (0, -1), 1: (1, 0), 2: (0, 1), 3: (-1, 0)}
G = 62
X0, DX = 178, 146
QX = {0: (0, 0), 1: (1, 0), 2: (0, 1), 3: (1, 1)}

def grid(x, y, labels, fill, centre=None, face=None, hi=None):
    for q, (cx, cy) in QX.items():
        f = fill
        if hi is not None and q in hi:
            f = hi[q]
        c.rect(x + cx * G / 2, y + cy * G / 2, G / 2, G / 2, fill=f, stroke=GREY, sw=1, rx=0)
        if q in labels:
            c.text(x + cx * G / 2 + G / 4, y + cy * G / 2 + G / 4 + 5, labels[q], size=13, weight=700, color=INK, anchor="middle")
    if centre is not None:
        c.circle(x + G / 2, y + G / 2, 9, fill=CARD, stroke=SOLID["navy"], sw=1.2)
        c.text(x + G / 2, y + G / 2 + 4, centre, size=11, weight=700, color=PAL["navy"][0], anchor="middle")
    if face:
        dx, dy = face
        ax, ay = x + G / 2 + dx * (G / 2 + 18), y + G / 2 + dy * (G / 2 + 18)
        bx, by = x + G / 2 + dx * (G / 2 + 4), y + G / 2 + dy * (G / 2 + 4)
        c.arrow([(bx - dx * 0, by - dy * 0), (ax, ay)], color=SOLID["red"], sw=1.6)

rows = [
    ("The party's cell", "member number in each quarter", 40),
    ("Cell ahead, member 0", "the left-hand front member: the order in which a blow looks for a monster", 190),
    ("Cell ahead, member 1", "the right-hand front member", 310),
]
for title, sub, y in rows:
    c.text(0, y + 24, title, size=12, weight=700, color=INK)
    for i, ln in enumerate(wrap(sub, 150, 10.5)):
        c.text(0, y + 40 + i * 13, ln, size=10.5, color=MUTED)

for d in range(4):
    x = X0 + d * DX
    c.text(x + G / 2, 30, names[d], size=12, weight=700, color=PAL["red"][0], anchor="middle")
    # party cell
    labels = {party[d][m]: str(m) for m in range(4)}
    grid(x, 50, labels, "#E1E5F2", face=arrows[d])
    c.text(x + G / 2, 50 + G + 34, "members 4, 5 behind 2, 3", size=9.5, color=MUTED, anchor="middle")
    # cell ahead
    for r, order in enumerate(target[d]):
        lab = {q: str(k + 1) for k, q in enumerate(order)}
        y = 190 + r * 120
        grid(x, y, lab, "#F5E0D9", centre="0", face=None)
        near = {0: "south", 1: "west", 2: "north", 3: "east"}[d]
        if r == 0:
            c.text(x + G / 2, y + G + 14, f"near side: {near}", size=9.5, color=MUTED, anchor="middle")

c.text(0, 412, "The centre (0) is searched first. Quarters: 0 north-west, 1 north-east, 2 south-west, 3 south-east of the north-up map. Tables ds:1A1B and ds:329B.", size=10.5, color=MUTED)
c.h = 424
c.save("figs/cmb_cellgrid.svg")

import sys; sys.path.insert(0, '.')
from figlib import *

# The 18 view slots: gViewCellOffsets (ds:199D) for facing north, gViewDrawOrder (ds:7278).
c = Canvas(760, 330)
c.section(0, 11, "THE 18 CELLS THE VIEW LOOKS AT (PARTY FACING NORTH, SLOT = INDEX IN THE TABLES)")

rows = [  # (row label, dy, slots, dx list)
    ("3 ahead", -3, [0, 1, 2, 3, 4, 5, 6], [-3, -2, -1, 0, 1, 2, 3]),
    ("2 ahead", -2, [7, 8, 9, 10, 11], [-2, -1, 0, 1, 2]),
    ("1 ahead", -1, [12, 13, 14], [-1, 0, 1]),
    ("own row", 0, [15, 16, 17], [-1, 0, 1]),
]
order = [0, 6, 1, 5, 2, 4, 3, 7, 11, 8, 10, 9, 12, 14, 13, 15, 17, 16]
rank = {s: i for i, s in enumerate(order)}
CW, CH = 54, 54
X0, Y0 = 70, 28
cols = {-3: 0, -2: 1, -1: 2, 0: 3, 1: 4, 2: 5, 3: 6}
kinds = {0: "grey", 1: "teal", 2: "ochre", 3: "plum"}
off = [-99, -98, -97, -96, -95, -94, -93, -66, -65, -64, -63, -62, -33, -32, -31, -1, 0, 1]
for ri, (lab, dy, slots, dxs) in enumerate(rows):
    y = Y0 + ri * (CH + 4)
    c.text(X0 - 6, y + CH / 2 + 4, lab, size=10.5, color=MUTED, anchor="end")
    for s, dx in zip(slots, dxs):
        x = X0 + cols[dx] * (CW + 4)
        k = kinds[ri]
        c.box(x, y, CW, CH, k, None, None)
        c.text(x + CW / 2, y + 22, str(s), size=18, weight=700, color=PAL[k][0], anchor="middle")
        c.text(x + CW / 2, y + 38, "%+d" % off[s], size=10.5, color=INK2, anchor="middle", mono=True)
        c.text(x + CW / 2, y + 50, "draw #%d" % (rank[s] + 1), size=9.5, color=MUTED, anchor="middle")
# party marker on slot 16
x16 = X0 + cols[0] * (CW + 4)
y16 = Y0 + 3 * (CH + 4)
c.path(f"M{x16+CW-15},{y16+12} L{x16+CW-9},{y16+3} L{x16+CW-3},{y16+12} Z", fill=SOLID["red"], stroke=SOLID["red"])

# axis labels
c.text(X0, Y0 + 4 * (CH + 4) + 12, "left of the party", size=10.5, color=MUTED)
c.text(X0 + 7 * (CW + 4) - 4, Y0 + 4 * (CH + 4) + 12, "right of the party", size=10.5, color=MUTED, anchor="end")

# right-hand explanation
RX = 500
c.section(RX, Y0 + 6, "WHAT EACH CELL NUMBER IS", color="#14504A")
yy = Y0 + 24
for l in wrap("cell id = (gPartyCell + gViewCellOffsets[gPartyDir * 18 + slot]) AND 3FFh; the +n under a slot is that offset for facing north on the 32-wide map (-32 is one row up).", 258, 11.5):
    c.text(RX, yy, l, size=11.5, color=INK2); yy += 15
yy += 6
c.text(RX, yy, "Slot 0 offset by facing", size=11.5, weight=700, color=INK); yy += 16
for d, nm, v in [(0, "north", -99), (1, "east", -93), (2, "south", 99), (3, "west", 93)]:
    c.text(RX, yy, "dir %d %-5s" % (d, nm), size=11, color=INK2, mono=True)
    c.text(RX + 110, yy, "%+d" % v, size=11, color=INK2, mono=True); yy += 15
yy += 8
c.text(RX, yy, "Draw order (gViewDrawOrder)", size=11.5, weight=700, color=INK); yy += 16
c.text(RX, yy, "0 6 1 5 2 4 3", size=11, color=INK2, mono=True); c.text(RX + 112, yy, "far row, outside in", size=10.5, color=MUTED); yy += 15
c.text(RX, yy, "7 11 8 10 9", size=11, color=INK2, mono=True); yy += 15
c.text(RX, yy, "12 14 13", size=11, color=INK2, mono=True); yy += 15
c.text(RX, yy, "15 17 16", size=11, color=INK2, mono=True); c.text(RX + 112, yy, "party's own row, last", size=10.5, color=MUTED); yy += 20
for l in wrap("The far rows come first so that nearer cells overwrite them. Rows hold 7, 5, 3 and 3 cells.", 258, 11):
    c.text(RX, yy, l, size=11, color=MUTED); yy += 14
c.h = max(Y0 + 4 * (CH + 4) + 22, yy + 6)
c.save("figs/gfx_view_slots.svg")

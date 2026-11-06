import sys; sys.path.insert(0, '.')
from figlib import *

c = Canvas(760, 394)
c.section(0, 11, "ONE MAZE CELL: 4 BYTES IN THE FILE, 9 IN MEMORY (gMaze, ds:B894, 1,024 CELLS = 9,216 BYTES)")
BW = 80
X0 = 20
Y = 62
H = 40

# file bracket over bytes 0..3
c.path(f"M{X0},{Y-10} L{X0},{Y-16} L{X0+4*BW},{Y-16} L{X0+4*BW},{Y-10}", stroke=SOLID["teal"], sw=1.2)
c.text(X0 + 2 * BW, Y - 22, "LEVELn.MAZ: 4 bytes a cell, copied in by LoadMaze", size=11, color=PAL["teal"][0], anchor="middle", weight=600)

fields = [
    (0, 1, "teal", "N", "side 0"),
    (1, 1, "teal", "E", "side 1"),
    (2, 1, "teal", "S", "side 2"),
    (3, 1, "teal", "W", "side 3"),
    (4, 1, "plum", "count | mask", "+4"),
    (5, 2, "ochre", "item list head (word)", "+5"),
    (7, 2, "navy", "script offset (word)", "+7"),
]
for off, n, kind, label, off_s in fields:
    x = X0 + off * BW
    tc, sc, fc = PAL[kind]
    c.rect(x, Y, n * BW, H, fill=fc, stroke=sc, sw=1.3, rx=0)
    if n == 1 and kind == "teal":
        c.text(x + BW / 2, Y + 25, label, size=16, weight=700, color=tc, anchor="middle")
    else:
        c.text(x + n * BW / 2, Y + 24, label, size=11.5, weight=700, color=tc, anchor="middle")
for i in range(9):
    c.text(X0 + i * BW + BW / 2, Y + H + 15, "+%d" % i, size=10.5, color=MUTED, anchor="middle", mono=True)

# explanations
TY = Y + H + 36
cols = [
    (0, 4, "teal", "Four wall types",
     "One byte per side, in direction order: 0 north, 1 east, 2 south, 3 west. The value indexes the four per-kind tables of the level (flags, picture, handler, decoration chain). 0 is open space."),
    (4, 1, "plum", "+4",
     "See below."),
    (5, 2, "ochre", "Items",
     "Head of a circular list of the items lying in the cell (an index into gItems, 0 = none)."),
    (7, 2, "navy", "Script",
     "Offset of the cell's script in the level file (0 = none)."),
]
for off, n, kind, title, body in cols:
    x = X0 + off * BW
    w = n * BW
    tc, sc, fc = PAL[kind]
    c.line(x + 2, Y + H + 22, x + 2, TY - 8, stroke=sc, sw=0.8)
    c.text(x + 6, TY + 6, title, size=11.5, weight=700, color=tc)
    yy = TY + 22
    for l in wrap(body, w - 8, 10.5):
        c.text(x + 6, yy, l, size=10.5, color=INK2)
        yy += 13.5

# byte 4 decode
BY = 236
c.line(0, BY - 12, 760, BY - 12, stroke=RULE)
c.section(0, BY + 4, "BYTE +4 BIT BY BIT, AND THE EVENTS THE MASK SWITCHES ON")
bx = 20
bw = 64
labels = [("7", "0x10", "navy"), ("6", "8", "navy"), ("5", "4", "navy"), ("4", "2", "navy"), ("3", "1", "navy"),
          ("2", "", "plum"), ("1", "", "plum"), ("0", "", "plum")]
yb = BY + 16
for i, (b, mode, kind) in enumerate(labels):
    x = bx + i * bw
    tc, sc, fc = PAL[kind]
    c.rect(x, yb, bw, 30, fill=fc, stroke=sc, sw=1.2, rx=0)
    c.text(x + bw / 2, yb + 13, "bit " + b, size=10.5, color=MUTED, anchor="middle")
    if mode:
        c.text(x + bw / 2, yb + 26, "mode " + mode, size=11, weight=700, color=tc, anchor="middle")
c.text(bx + 2.5 * bw, yb + 50, "trigger mask: a set bit enables the event", size=11, color=PAL["navy"][0], anchor="middle", weight=600)
c.text(bx + 6.5 * bw, yb + 50, "monster count, 0 to 4 in practice", size=11, color=PAL["plum"][0], anchor="middle", weight=600)

# modes legend at right
mx = 548
rows = [
    ("1", "the party enters the cell"),
    ("2", "the party leaves it"),
    ("4", "an item is put into it"),
    ("8", "an item is taken from it"),
    ("0x10", "a thrown item flies into it"),
]
c.text(mx, BY + 28, "mode", size=10.5, color=MUTED, weight=600)
c.text(mx + 50, BY + 28, "event (RunCellScript)", size=10.5, color=MUTED, weight=600)
yy = BY + 44
for m, e in rows:
    c.text(mx, yy, m, size=11, color=PAL["navy"][0], weight=700, mono=True)
    c.text(mx + 50, yy, e, size=11, color=INK2)
    yy += 15
yy = BY + 126
for l in wrap("Enabled whatever the mask: 0x20 (the level timer, cell 0 only), 0x40 (a mouse click on the wall) and 0x80 (nothing raises it). The key 'activate' uses 0xFF. LoadLevel ORs the file's flag byte into +4 and stores the script offset at +7 for each entry of the level file's trigger table.", 735, 10.5):
    c.text(0, yy, l, size=10.5, color=MUTED)
    yy += 13.5
c.save("figs/wld_cell.svg")

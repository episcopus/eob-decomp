import sys; sys.path.insert(0, '.')
from figlib import *

c = Canvas(760, 452)
W = 760
# ---- row 1: EOBDATA.SAV to scale ----
c.section(0, 11, "EOBDATA.SAV, 33,101 BYTES, TO SCALE")
TOT = 33101
parts = [("6 characters", 1458, "plum"), ("hdr", 64, "navy"), ("500 items", 7000, "ochre"), ("12 level states", 24480, "teal"), ("", 3, "grey"), ("", 96, "grey")]
x = 0
y1, h1 = 22, 30
ends = []
for lab, n, k in parts:
    w = W * n / TOT
    tc, sc, fc = PAL[k]
    c.rect(x, y1, w, h1, fill=fc, stroke=sc, sw=1, rx=0)
    if n > 3000:
        c.text(x + w / 2, y1 + 19, "%s: %s bytes" % (lab, format(n, ",")), size=11.5, weight=700, color=tc, anchor="middle")
    ends.append((x, w))
    x += w
c.text(ends[0][0] + 2, y1 + h1 + 13, "characters 1,458", size=10, color=PAL["plum"][0])
c.text(120, y1 + h1 + 13, "header 64 (ds:FBC8)", size=10, color=PAL["navy"][0])
c.text(760, y1 + h1 + 13, "3 + 96 bytes (sound flags, spell-item buffer)", size=10, color=MUTED, anchor="end")
# trapezoid to row 2
lx0, lx1 = ends[3][0], ends[3][0] + ends[3][1]
y2 = 118
c.path(f"M{lx0:.1f},{y1+h1} L0,{y2-2} M{lx1:.1f},{y1+h1} L{W},{y2-2}", stroke=GREY, sw=0.8, dash="3 2")
# ---- row 2: LEVELS.TMP 12 slots ----
c.text(0, y2 - 8, "LEVELS.TMP = the same 12 slots, 2,040 bytes each; slot n starts at (n - 1) * 0x7F8", size=11, color=PAL["teal"][0], weight=700)
h2 = 30
sw_ = W / 12
for i in range(12):
    tc, sc, fc = PAL["teal"]
    c.rect(i * sw_, y2, sw_, h2, fill=fc, stroke=sc, sw=1, rx=0)
    c.text(i * sw_ + sw_ / 2, y2 + 20, str(i + 1), size=12, weight=700, color=tc, anchor="middle")
# trapezoid to row 3 from slot 2
y3 = 214
c.path(f"M{sw_:.1f},{y2+h2} L0,{y3-2} M{2*sw_:.1f},{y2+h2} L{W},{y3-2}", stroke=GREY, sw=0.8, dash="3 2")
c.text(0, y3 - 8, "one slot, 2,040 bytes (0x7F8)", size=11, color=PAL["teal"][0], weight=700)
h3 = 40
a = W * 0x4B0 / 0x7F8
tc, sc, fc = PAL["navy"]
c.rect(0, y3, a, h3, fill=fc, stroke=sc, sw=1.2, rx=0)
# used part example: level 1 = 549 bytes
used = W * 550 / 0x7F8
c.rect(0, y3, used, h3, fill=MID["navy"], stroke=sc, sw=1.2, rx=0)
c.text(used / 2, y3 + 25, "LCW", size=12, weight=700, color=tc, anchor="middle")
c.text(used + 8, y3 + 18, "wall bytes of 1,024 cells, packed;", size=11, color=INK2)
c.text(used + 8, y3 + 32, "at most 0x4B0 = 1,200 bytes", size=11, color=INK2)
tc2, sc2, fc2 = PAL["red"]
c.rect(a, y3, W - a, h3, fill=fc2, stroke=sc2, sw=1.2, rx=0)
c.text(a + (W - a) / 2, y3 + 18, "30 monsters, raw", size=12, weight=700, color=tc2, anchor="middle")
c.text(a + (W - a) / 2, y3 + 32, "30 x 28 = 840 bytes (0x348)", size=11, color=INK2, anchor="middle")
c.text(0, y3 + h3 + 14, "+0x000", size=10, color=MUTED, mono=True)
c.text(a, y3 + h3 + 14, "+0x4B0", size=10, color=MUTED, mono=True)
c.text(W, y3 + h3 + 14, "+0x7F8", size=10, color=MUTED, mono=True, anchor="end")
c.text(used, y3 + h3 + 14, "level 1 of the test save: 550 bytes", size=10, color=MUTED, anchor="middle")

# ---- flows ----
y4 = 312
c.line(0, y4 - 10, W, y4 - 10, stroke=RULE)
c.section(0, y4 + 4, "WRITE AND READ (ovl3:0FD1 PackLevelState, ovl3:109D UnpackLevelState)")
bw_, bh_ = 168, 62
bx = [0, 196, 392, 588]
items = [
    ("teal", "gMaze", "the first 4 bytes of each of the 1,024 cells go to a 4,096-byte scratch buffer"),
    ("navy", "CompressLcw", "pack to the slot; over 1,200 bytes prints 'Map compression failure' and quits"),
    ("red", "gMonsters", "copied whole to slot +0x4B0"),
    ("plum", "on load", "Unpack4 the walls, copy the monsters back, then add one to the count of every cell a monster stands in"),
]
for (k, t, b), x in zip(items, bx):
    c.rect(x, y4 + 14, bw_, bh_ + 18, fill=PAL[k][2], stroke=PAL[k][1], sw=1.1, rx=6)
    c.text(x + 8, y4 + 32, t, size=12, weight=700, color=PAL[k][0], mono=True)
    yy = y4 + 47
    for l in wrap(b, bw_ - 16, 10.5):
        c.text(x + 8, yy, l, size=10.5, color=INK2)
        yy += 13
for x in bx[:3]:
    c.arrow([(x + bw_ + 2, y4 + 55), (x + 196 - 3, y4 + 55)], color=GREY)
c.text(0, y4 + 116, "Not in a slot: items (the 500 records carry their level and cell), the per-level flags and the party position (the 64-byte header).", size=10.5, color=MUTED)
c.save("figs/wld_state.svg")

import sys; sys.path.insert(0, '.')
from figlib import *

c = Canvas(760, 560)
c.section(0, 11, "THE TWELVE LEVELS: WHAT EACH LEVEL FILE CONTAINS (counts from the files)")
cols = [("lvl", 0), ("wall set", 30), ("sprite sheets", 104), ("monsters", 232), ("items", 296), ("triggers", 346), ("timer", 410), ("stairs / falls out", 468)]
hdr_y = 36
for h, x in cols:
    c.text(x, hdr_y, h, size=10, color=MUTED, weight=600)
c.line(0, hdr_y + 6, 760, hdr_y + 6, stroke=RULE)
data = [
    (1, "brick", "kobold, leech", 24, 15, 38, "off", "L2"),
    (2, "brick", "zombie, skeleton", 27, 26, 84, "80", "L1, L3, L7"),
    (3, "brick", "kuotoa, flind", 29, 38, 93, "100", "L2, L4 (+2 falls)"),
    (4, "blue", "spider", 15, 30, 84, "150", "L3, L5 (+4), L7"),
    (5, "blue", "dwarf, spider", 23, 28, 104, "50", "L4, L6 (+41), L7"),
    (6, "blue", "kenku, mage", 28, 45, 72, "50", "L5, L7, L10"),
    (7, "drow", "drowelf, skelwar", 29, 37, 90, "50", "L2 4 5 6 8 (+3) 9 11"),
    (8, "drow", "drider, hellhnd", 28, 30, 74, "50", "L7, L9 (+6), L10"),
    (9, "drow", "rust, disbeast", 22, 32, 70, "80", "L7, L8, L10 (+2)"),
    (10, "green", "shindia, mantis", 14, 17, 56, "80", "L6, L8, L9, L11 (+1)"),
    (11, "green", "xorn, mflayer", 22, 23, 37, "50", "L7, L12"),
    (12, "xanatha", "xanath, golem", 8, 29, 69, "50", "L11"),
]
kinds = {"brick": "ochre", "blue": "navy", "drow": "plum", "green": "teal", "xanatha": "red"}
y = hdr_y + 24
for r in data:
    k = kinds[r[1]]
    c.rect(0, y - 14, 760, 20, fill=PAL[k][2] if r[0] % 2 else CARD, stroke="none", rx=0, opacity=0.8)
    c.text(0, y, str(r[0]), size=11.5, weight=700, color=PAL[k][0], mono=True)
    c.text(30, y, r[1], size=11, color=INK2, weight=600)
    c.text(104, y, r[2], size=11, color=INK2)
    for (v, x) in zip(r[3:7], [232, 296, 346, 410]):
        c.text(x, y, str(v), size=11, color=INK2, mono=True)
    c.text(468, y, r[7], size=10.5, color=INK2)
    y += 22
yt = y + 2
for j, l in enumerate(wrap("monsters: live records in the file (slots 1 to 29); items: records of ITEM.DAT placed on the level; triggers: cells with a script; timer: steps between level-timer events (mode 2; level 1 off); stairs / falls out: levels reached by LOAD LEVEL, (+n) = n falling cells.", 740, 9.8)):
    c.text(0, yt + 8 + j * 12, l, size=9.8, color=MUTED)

# link graph
GY = yt + 160
c.line(0, GY - 112, 760, GY - 112, stroke=RULE)
c.section(0, GY - 96, "HOW THE LEVELS CONNECT (every LOAD LEVEL in the scripts: 125 of them)")
nx = [40 + i * 62 for i in range(12)]
# straight stair links (adjacent)
for i in range(11):
    c.line(nx[i], GY, nx[i + 1], GY, stroke=SOLID["teal"], sw=2.2)
# hub arcs above: 7 to 2,4,5,9,11 and 6-10, 8-10
def arc(a, b, up, color, dash=None, sw=1.5, h=None):
    x1, x2 = nx[a - 1], nx[b - 1]
    hh = h if h is not None else 18 + 9 * abs(b - a)
    ym = GY + (-hh if up else hh)
    c.path(f"M{x1},{GY} Q{(x1+x2)/2},{ym*2-GY if False else ym} {x2},{GY}", stroke=color, sw=sw, dash=dash)
for t in (2, 4, 5, 9, 11):
    arc(7, t, True, SOLID["plum"], h=14 + 7 * abs(7 - t))
arc(6, 10, False, SOLID["ochre"], h=36)
arc(8, 10, False, SOLID["ochre"], h=22)
# falls: dashed small arcs below between adjacent pairs
for a, n in [(3, 2), (4, 4), (5, 41), (7, 3), (8, 6), (9, 2), (10, 1)]:
    x1, x2 = nx[a - 1], nx[a]
    c.path(f"M{x1+6},{GY+6} Q{(x1+x2)/2},{GY+26} {x2-6},{GY+6}", stroke=SOLID["red"], sw=1.3, dash="3 2")
    c.text((x1 + x2) / 2, GY + 30, str(n), size=9.5, color=PAL["red"][0], anchor="middle")
for i, x in enumerate(nx):
    k = kinds[data[i][1]]
    c.circle(x, GY, 12, fill=PAL[k][2], stroke=PAL[k][1], sw=1.5)
    c.text(x, GY + 4, str(i + 1), size=11.5, weight=700, color=PAL[k][0], anchor="middle")
c.legend(0, GY + 76, [("teal", "stairs between neighbours"), ("plum", "level 7 links to 2, 4, 5, 9, 11 (portals?)"), ("ochre", "L6 and L8 to L10"), ("red", "falls (number of cells), one level down", "3 2")])
c.h = GY + 90
c.save("figs/wld_levels.svg")

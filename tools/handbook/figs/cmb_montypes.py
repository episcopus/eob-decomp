import sys; sys.path.insert(0, '.')
from figlib import *

c = Canvas(760, 560)
c.section(0, 11, "THE 22 MONSTER TYPES (gMonsterTypes, ds:1A3B, 27 bytes each)")

# type, levels, AC, THAC0, HD (-1: 1d4 hp), attacks, XP, resist %, flags
T = [
 (0, "1", 7, 18, -1, "1d4", 15, 0, "H1 H2"),
 (1, "1", 7, 17, 2, "1d4", 120, 0, "H2"),
 (2, "2", 5, 17, 1, "1d6", 65, 0, "E U move"),
 (3, "2", 8, 15, 2, "1d8", 65, 0, "U"),
 (4, "3", 4, 16, 2, "1d4+1 x2", 175, 20, "R"),
 (5, "3", 5, 17, 3, "1d8", 65, 0, "H1 H2 move"),
 (6, "4 5", 6, 19, 2, "1d8", 650, 0, "H2 P"),
 (7, "5", 3, 12, 7, "1d8", 730, 0, "H1 H2"),
 (8, "7", 5, 7, 10, "1d8+6", 1500, 0, "E U move"),
 (9, "9", 4, 15, 6, "1d8 x2", 975, 0, "H1 H2"),
 (10, "10", 5, 13, 6, "1d8 1d4 1d4", 1400, 0, "H1 H2 Z move"),
 (11, "none", 4, 13, 5, "1d4 x2", 1250, 0, "H1"),
 (12, "7", 4, 15, 4, "1d8+4", 650, 60, "H1 Z"),
 (13, "12", -1, 7, 10, "3d8", 10000, 100, "H2"),
 (14, "none", -1, 7, 9, "3d4", 2000, 0, "H1 H2 R"),
 (15, "8", 3, 13, 7, "1d6", 5000, 60, "H2 R"),
 (16, "6", 5, 13, 7, "1d6", 5000, 0, "H2 R"),
 (17, "11", 5, 11, 8, "5d20", 8000, 90, "R"),
 (18, "9", 2, 15, 5, "1d1", 270, 0, "H2 T"),
 (19, "11", -2, 13, 8, "6d4", 4000, 50, "H2"),
 (20, "8", 4, 13, 7, "1d10", 975, 40, "H2"),
 (21, "none", 0, 5, 35, "1d100", 14000, 100, "R"),
]
cols = [(0, "type"), (40, "level"), (92, "AC"), (126, "THAC0"), (176, "HD"), (234, "attacks"), (330, "XP"), (396, "resists"), (462, "flags")]
ty = 26
c.rect(0, ty, 760, 19, fill="#ECE7DD", stroke="none", rx=0)
for x, h in cols:
    c.text(x + 6, ty + 13, h, size=10.5, weight=700, color=INK2)
rh = 18.5
for i, r in enumerate(T):
    y = ty + 19 + i * rh
    c.rect(0, y, 760, rh, fill="#FFFDF8" if i % 2 else "#F4F0E6", stroke="none", rx=0)
    vals = [str(r[0]), r[1], str(r[2]), str(r[3]), ("1d4 hp" if r[4] == -1 else str(r[4])), r[5], f"{r[6]:,}".replace(",", " "), (f"{r[7]}%" if r[7] else "-"), r[8]]
    for (x, h), v in zip(cols, vals):
        c.text(x + 6, y + 13, v, size=10.5, color=INK, weight=700 if h == "type" else 400)
yb = ty + 19 + len(T) * rh + 16
yy = c.para(0, yb, "level: the dungeon levels whose LEVELn.INF has the type among its 30 starting records (decoded from the files); none: in no starting record, so placed by a script or unused [?]. AC and THAC0 are descending (lower AC is harder to hit). HD: hit points are RollDice(HD, 8) (1d4 for -1); XP is what the whole party shares. Flags: H1 Hold Person works, H2 Hold Monster works, U undead (turnable), E halves damage from edged weapons, move: may swing after moving, P poison, Z paralysis, R ranged or spell attack, T destroys a metal item. Resists is the Magic Resistance percentage, a roll of 1d100 below it stops a spell.", 740, size=10.5, color=MUTED)
c.h = yy + 4
c.save("figs/cmb_montypes.svg")

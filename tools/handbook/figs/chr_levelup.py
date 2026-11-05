import sys, math; sys.path.insert(0, '.')
from figlib import *

c = Canvas(760, 470)
c.section(0, 11, "Experience: from a kill to a new level")

# ---- flow ----
Y = 26
bw, gap = 132, 20
items = [
    ("navy", "award", ["a dead monster gives its type's word at +13 (m13); a script opcode gives a word"]),
    ("navy", "AddPartyExperience", ["0DE8:0C2B. Divide by the members with hp above -10 (MemberIs 3)"]),
    ("teal", "AddExperience", ["0DE8:0B45. Divide by gClassSlotCount[class]: 1, 2 or 3"]),
    ("ochre", "each slot with a class", ["experience[slot] += share; test it against the table"]),
    ("red", "LevelUpSlot", ["levels[slot]++, one hit die of that slot added to hp and maxHp"]),
]
for i, (k, t, b) in enumerate(items):
    x = i * (bw + gap)
    c.box(x, Y, bw, 104, k, t, b, size=10.5, tsize=11.5)
    if i:
        c.arrow([(x - gap, Y + 52), (x, Y + 52)], color=GREY)
c.text(0, Y + 124, "The test is experience[slot] > table[levels[slot]]: strictly greater, and only one level per award. A table entry of FFFFFFFF ends advancement.", size=10.5, color=INK2)

# ---- ladder ----
LY = 190
c.section(0, LY, "gExpTables: experience needed for each level (log scale), one table per base class")
AX0, AX1 = 150, 690
def px(v):
    return AX0 + (math.log10(v) - 3.0) / 3.0 * (AX1 - AX0)
rows = [
    ("FIGHTER", "teal", [2000, 4000, 8000, 16000, 32000, 64000, 125000, 250000, 500000, 750000]),
    ("MAGE", "navy", [2500, 5000, 10000, 20000, 40000, 60000, 90000, 135000, 250000, 375000]),
    ("CLERIC", "ochre", [1500, 3000, 6000, 13000, 27500, 55000, 110000, 225000, 450000]),
    ("THIEF", "plum", [1250, 2500, 5000, 10000, 20000, 40000, 70000, 110000, 160000, 220000]),
    ("PALADIN, RANGER", "red", [2250, 4500, 9000, 18000, 36000, 75000, 150000, 300000, 600000, 900000]),
]
top = LY + 34
RH = 38
# decade gridlines
for v, lab in [(1000, "1,000"), (10000, "10,000"), (100000, "100,000"), (1000000, "1,000,000")]:
    x = px(v)
    c.line(x, top - 8, x, top + RH * len(rows) - 4, stroke=RULE, sw=1)
    c.text(x, top - 12, lab, size=10, color=MUTED, anchor="middle")
x5 = px(5000)
c.line(x5, top - 8, x5, top + RH * len(rows) - 4, stroke="#A8452F", sw=1.2, dash="4 3")
for i, (name, kind, th) in enumerate(rows):
    y = top + i * RH + 14
    tc, sc, fc = PAL[kind]
    c.text(0, y + 4, name, size=11.5, weight=700, color=tc)
    c.line(AX0, y, AX1, y, stroke=sc, sw=1.4)
    for j, v in enumerate(th):
        x = px(v)
        c.circle(x, y, 3.2, fill=SOLID[kind], stroke="none")
        c.text(x, y + 15, str(j + 2), size=9.5, color=tc, anchor="middle")
    c.text(px(th[-1]) + 8, y - 5, "max %d" % (len(th) + 1), size=9.5, color=MUTED)
c.text(x5 + 5, top + RH * len(rows) + 8, "5000 XP at creation, divided by the number of classes", size=10.5, color="#80311F")
c.text(0, top + RH * len(rows) + 8, "dots: the level reached when the total passes the mark", size=10.5, color=MUTED)
c.save("figs/chr_levelup.svg")

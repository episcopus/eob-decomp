import sys; sys.path.insert(0, '.')
from figlib import *

c = Canvas(760, 440)
c.section(0, 11, "EXPERIENCE: FROM A KILL TO A NEW LEVEL")

def tbox(x, y, w, kind, title, lines, size=10.5, h=None):
    out = []
    for l in lines:
        out += wrap(l, w - 32, size)
    hh = h or (10 + 17 + len(out) * size * 1.3 + 8)
    c.box(x, y, w, hh, kind, title, out, size=size, tsize=12, align="left")
    return y + hh

bw, gap = 172, 24
specs = [
    ("red", "KillMonster", ["08E6:0EC3. hp at 0 or below: drops go on its cell (first item 1 time in 2, second always); XP = the type's value, 15 to 14000"]),
    ("teal", "AddPartyExperience", ["0DE8:0C2B. amount / number of members passing MemberIs 3 (present, not dead; unconscious count)"]),
    ("teal", "AddExperience", ["0DE8:0B45. share / number of classes of the member (1, 2 or 3); each class slot adds the share to its own total"]),
    ("plum", "LevelUpSlot", ["0DE8:0C82. total above the slot's threshold: level + 1, hp and maxHp += one hit die plus CON bonus, divided by the class count; one level per award"]),
]
H = 0
for i, (k, t, l) in enumerate(specs):
    x = i * (bw + gap)
    H = max(H, tbox(x, 24, bw, k, t, l, h=118) - 24)
    if i:
        c.arrow([(x - gap, 24 + 59), (x, 24 + 59)])

# thresholds
ty = 24 + 118 + 22
c.text(0, ty, "Experience a class slot must exceed to reach level n (ds:35BC tables)", size=12, weight=700, color=INK)
cls = [
    ("fighter", [2000, 4000, 8000, 16000, 32000, 64000, 125000, 250000, 500000, 750000]),
    ("paladin, ranger", [2250, 4500, 9000, 18000, 36000, 75000, 150000, 300000, 600000, 900000]),
    ("mage", [2500, 5000, 10000, 20000, 40000, 60000, 90000, 135000, 250000, 375000]),
    ("cleric", [1500, 3000, 6000, 13000, 27500, 55000, 110000, 225000, 450000, None]),
    ("thief", [1250, 2500, 5000, 10000, 20000, 40000, 70000, 110000, 160000, 220000]),
]
x0, cw = 112, 64
for j in range(10):
    c.text(x0 + j * cw + cw / 2, ty + 18, str(j + 2), size=11, weight=700, color=MUTED, anchor="middle")
c.text(x0 - 8, ty + 18, "level", size=10.5, color=MUTED, anchor="end")
def fmt(v):
    if v is None: return "none"
    return f"{v:,}".replace(",", " ")
for r, (name, vals) in enumerate(cls):
    y = ty + 26 + r * 22
    c.text(0, y + 14, name, size=11.5, weight=700, color=INK)
    for j, v in enumerate(vals):
        f = "#ECE7DD" if v is None else (MID["teal"] if j < 4 else "#7DBBB0" if j < 7 else SOLID["teal"])
        tcol = MUTED if v is None else (INK if j < 7 else "#FFFFFF")
        c.rect(x0 + j * cw, y, cw - 2, 20, fill=f, stroke="none", rx=2)
        c.text(x0 + j * cw + (cw - 2) / 2, y + 14, fmt(v), size=10.5, weight=600, color=tcol, anchor="middle")
yy = ty + 26 + 5 * 22 + 14
yy = c.para(0, yy, "The numbers agree with AD&D 2nd edition's [H]; the limit of 11 (cleric 10) is where each table ends in -1 [V]. A multi-class member levels each slot on its own total, so a fighter/mage needs the share twice: 650 points of a kill among 4 living members are 162 each, 81 to each of his two classes.", 756, size=10.5, color=MUTED)
c.h = yy + 4
c.save("figs/cmb_xp.svg")

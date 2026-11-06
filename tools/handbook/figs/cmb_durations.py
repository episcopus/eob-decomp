import sys; sys.path.insert(0, '.')
from figlib import *
import math

c = Canvas(760, 520)
c.section(0, 11, "HOW LONG THINGS LAST (log scale; 1 tick = 1/18.2 s; the common unit 546 ticks = 30 s)")

items = [  # name, lo s, hi s, kind, note
    ("Hold (monster)", 12, 29, "plum", "15 updates"),
    ("Shocking Grasp, Vampiric Touch", 30, 30, "red", ""),
    ("poison tick, every", 30, 30, "grey", "5 hp"),
    ("Aid, Detect Magic, Prayer", 60, 60, "navy", ""),
    ("Protection from Evil", 90, 90, "navy", ""),
    ("Haste", 120, 120, "navy", ""),
    ("Shield, Magical Vestment", 150, 150, "navy", ""),
    ("paralysis", 150, 150, "plum", ""),
    ("Giant Strength potion", 150, 240, "ochre", "(1d4 + 4) x 30 s"),
    ("Bless", 180, 180, "navy", ""),
    ("Flame Blade", 120, 270, "red", "(4 + level / 2) x 30 s"),
    ("Protection from Evil 10'", 300, 3000, "navy", "level x 5 min"),
    ("Slow Poison", 3600, 19800, "teal", "(1 + level) x 30 min"),
]
LX, X0, X1 = 0, 238, 740
lo_v, hi_v = 10, 20000
def px(v):
    return X0 + (X1 - X0) * (math.log10(v) - math.log10(lo_v)) / (math.log10(hi_v) - math.log10(lo_v))
ay = 30
ticks = [(10, "10 s"), (30, "30 s"), (60, "1 min"), (180, "3 min"), (600, "10 min"), (1800, "30 min"), (3600, "1 h"), (10800, "3 h")]
rh = 24
H = ay + 10 + len(items) * rh
for v, lab in ticks:
    c.line(px(v), ay + 6, px(v), H, stroke=RULE, dash="2 3")
    c.text(px(v), ay, lab, size=10, color=MUTED, anchor="middle")
for i, (name, lo, hi, kind, note) in enumerate(items):
    y = ay + 12 + i * rh
    tc, sc, fc = PAL[kind]
    c.text(LX, y + 14, name, size=11, color=INK)
    if hi == lo:
        c.circle(px(lo), y + 9, 5, fill=SOLID[kind])
        if note:
            c.text(px(lo) + 10, y + 13, note, size=10, color=MUTED)
    else:
        c.rect(px(lo), y + 3, px(hi) - px(lo), 12, fill=fc, stroke=sc, sw=1.1, rx=4)
        c.text(px(hi) + 8 if px(hi) < 560 else px(lo) - 8, y + 13, note, size=10, color=MUTED, anchor="start" if px(hi) < 560 else "end")

ny = H + 20
c.text(0, ny, "No timer at all:", size=12, weight=700, color=INK)
nots = [
    ("Armor", "absorbs caster level + 8 points of damage, then ends (CharRec +DF)"),
    ("Stoneskin", "turns away the next 1d4 + level / 2 monster hits (CharRec +E0)"),
    ("Invisibility", "until the member attacks or casts an attack spell, or is dispelled"),
    ("Fear, turned undead", "monsters keep running until something hits them"),
]
for i, (a, b) in enumerate(nots):
    c.text(0, ny + 20 + i * 16, a, size=11, weight=700, color=PAL["navy"][0])
    c.text(150, ny + 20 + i * 16, b, size=11, color=INK2)
c.h = ny + 20 + len(nots) * 16 + 4
c.save("figs/cmb_durations.svg")

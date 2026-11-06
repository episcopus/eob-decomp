import sys; sys.path.insert(0, '.')
from figlib import *

c = Canvas(760, 470)
c.section(0, 11, "One rest, in game hours: what CampLoop (ovl6:0000) does")

AX0, AX1 = 120, 744
HOURS = 25.0
def hx(h):
    return AX0 + h / HOURS * (AX1 - AX0)

lanes = [
    ("every tick", "teal"),
    ("spells", "plum"),
    ("every hour", "red"),
    ("every 8 hours", "ochre"),
    ("every 24 hours", "navy"),
]
LY0, LH = 40, 50
for i, (nm, k) in enumerate(lanes):
    y = LY0 + i * LH
    tc, sc, fc = PAL[k]
    c.rect(0, y, 760, LH - 6, fill=fc, stroke="none", rx=4, opacity=0.55)
    c.text(8, y + 22, nm, size=11.5, weight=700, color=tc)

# lane 0: ticks, six per hour
y = LY0
tc, sc, fc = PAL["teal"]
for t in range(0, 24 * 6 + 1):
    x = hx(t / 6.0)
    h = 10 if t % 6 == 0 else 5
    c.line(x, y + 36 - h, x, y + 36, stroke=sc, sw=0.8)
c.text(AX0 + 4, y + 12, "a tick is a sixth of an hour: counters[i]++, memorise one spell, healers cast, then the stop tests", size=10, color=INK2)

# lane 1: spells
y = LY0 + LH
tc, sc, fc = PAL["plum"]
c.rect(hx(0), y + 8, hx(49 / 6.0) - hx(0), 22, fill="#FFFFFF", stroke=sc, sw=1.1, rx=3)
c.text((hx(0) + hx(49 / 6.0)) / 2, y + 23, "nothing until counters > 48 (49 ticks)", size=10, color=tc, anchor="middle", weight=600)
c.text(hx(49 / 6.0) + 6, y + 18, "then one spell per tick, lowest slot first:", size=10, color=INK2)
c.text(hx(49 / 6.0) + 6, y + 31, "a level n spell needs n ticks since the last one", size=10, color=INK2)

# lane 2: hours
y = LY0 + 2 * LH
tc, sc, fc = PAL["red"]
for h in range(1, 25):
    x = hx(h)
    c.line(x, y + 6, x, y + 16, stroke=sc, sw=1.1)
    c.circle(x, y + 6, 2.6, fill=SOLID["red"])
c.text(AX0 + 4, y + 12, "", size=10)
c.text(hx(1) - 4, y + 34, "hours + 1; clock + 7FF8h ticks; each poisoned member takes 10 damage", size=10, color=INK2)

# lane 3: 8h
y = LY0 + 3 * LH
tc, sc, fc = PAL["ochre"]
for h in (8, 16, 24):
    x = hx(h)
    c.line(x, y + 4, x, y + 38, stroke=sc, sw=1.6)
    c.circle(x, y + 4, 3.5, fill=SOLID["ochre"])
c.text(hx(0) + 4, y + 18, "paladins get Lay on Hands back;", size=10, color=INK2)
c.text(hx(0) + 4, y + 31, "fed members below maxHp gain 1 hp", size=10, color=INK2)
c.text(hx(8) + 6, y + 31, "most wounded member chosen again", size=10, color=INK2)

# lane 4: 24h
y = LY0 + 4 * LH
tc, sc, fc = PAL["navy"]
c.line(hx(24), y + 4, hx(24), y + 38, stroke=sc, sw=2.2)
c.circle(hx(24), y + 4, 4, fill=SOLID["navy"])
c.text(hx(24) - 8, y + 20, "a member with food 0 loses 1 hp", size=10, color=INK2, anchor="end")
c.text(hx(24) - 8, y + 33, "(then 'Your party is starving' is asked)", size=10, color=INK2, anchor="end")

# axis
AYY = LY0 + 5 * LH + 6
c.line(AX0, AYY, AX1, AYY, stroke=GREY, sw=1.3)
for h in range(0, 25):
    x = hx(h)
    c.line(x, AYY, x, AYY + (6 if h % 8 else 9), stroke=GREY, sw=1)
    if h % 4 == 0:
        c.text(x, AYY + 22, str(h), size=10.5, color=MUTED, anchor="middle", mono=True)
c.text(AX0 - 6, AYY + 22, "hours", size=10.5, color=MUTED, anchor="end")

# stop conditions
SY = AYY + 46
c.text(0, SY, "The loop goes on while there is a reason to rest and none to stop:", size=11.5, weight=700, color="#14504A")
c.box(0, SY + 10, 360, 64, "teal", "reasons to go on", ["a living, fed member still has a spell to regain, or", "someone is wounded and the party agreed to heal"], size=10.5, tsize=12)
c.box(380, SY + 10, 360, 64, "red", "reasons to stop", ["a key is pressed; a monster is within distance 1;", "a prompt (poison, starving, injured) is answered no"], size=10.5, tsize=12)
c.save("figs/chr_rest.svg")

import sys; sys.path.insert(0, '.')
from figlib import *

c = Canvas(760, 300)
c.section(0, 11, "HOW LONG A HAND STAYS BUSY (ticks after the click)")
X0, SC = 150, 10.5          # x of tick 0, units per tick
def tx(t): return X0 + t * SC

# axis
ay = 40
c.line(tx(0), ay, tx(58), ay, stroke=GREY)
for t in range(0, 59, 6):
    c.line(tx(t), ay - 3, tx(t), ay + 3, stroke=GREY)
    c.text(tx(t), ay - 7, str(t), size=10.5, color=MUTED, anchor="middle")
for s in (1, 2, 3):
    t = s * 18.2
    c.line(tx(t), ay + 3, tx(t), 268, stroke=RULE, dash="2 3")
    c.text(tx(t), 282, f"{s} s", size=10.5, color=MUTED, anchor="middle")
c.text(tx(0), 282, "0", size=10.5, color=MUTED, anchor="middle")

def row(y, label, sub, segs, marks):
    c.text(0, y + 13, label, size=12, weight=700, color=INK)
    c.text(0, y + 27, sub, size=10.5, color=MUTED)
    for (a, b, kind, txt) in segs:
        tc, sc, fc = PAL[kind]
        c.rect(tx(a), y, (b - a) * SC, 26, fill=fc, stroke=sc, sw=1.1, rx=3)
        c.text((tx(a) + tx(b)) / 2, y + 17, txt, size=11, color=tc, anchor="middle")
    for (t, s, yy) in marks:
        c.text(tx(t), y + 40 + yy, s, size=10, color=MUTED, anchor="middle")

row(56, "Attack", "hit, miss or hack", 
    [(0, 18, "red", "label shown"), (18, 54, "ochre", "hand shaded, label gone")],
    [(0, "click", 0), (18, "type 2/3", 0), (54, "type 4/5: free", 0)])
row(114, "Attack, hasted", "effect flag 8", 
    [(0, 18, "red", "label shown"), (18, 36, "ochre", "shaded")],
    [(0, "click", 0), (18, "type 2/3", 0), (36, "free", 0)])
row(172, "Cannot attack", "CAN'T REACH, NO AMMO", 
    [(0, 18, "red", "label shown")],
    [(0, "click", 0), (18, "type 0/1: free", 0)])
row(222, "Hit taken", "lastDamage on the portrait", 
    [(0, 18, "plum", "damage number")],
    [(0, "DamageCharacter", 0), (18, "type 6: cleared", 0)])
c.save("figs/cmb_cooldown.svg")

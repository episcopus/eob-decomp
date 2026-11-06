import sys; sys.path.insert(0, '.')
from figlib import *

MAXHP = 50
c = Canvas(760, 380)
c.section(0, 11, "Hit points and condition of one member (example maxHp = 50, hp is a signed byte)")

X0, X1 = 20, 740
lo, hi = -10, MAXHP
def px(v):
    return X0 + (v - lo) / float(hi - lo + 1) * (X1 - X0)
AY = 78
H = 30
# bands
bands = [(-10, -10, "red", "dead"), (-9, 0, "plum", "unconscious"), (1, 9, "ochre", "wounded"), (10, MAXHP, "teal", "well")]
for a, b, k, t in bands:
    tc, sc, fc = PAL[k]
    c.rect(px(a), AY, px(b + 1) - px(a), H, fill=SOLID[k] if a == -10 else fc, stroke=sc, sw=1.2, rx=0)
    if b - a >= 6:
        c.text((px(a) + px(b + 1)) / 2, AY + 19, t, size=11.5, weight=700, color=tc, anchor="middle")
c.text(px(-10) + 16, AY - 7, "-10: dead", size=10.5, weight=700, color="#80311F")
c.arrow([(px(-10) + 5, AY - 10), (px(-10) + 5, AY - 1)], color=SOLID["red"])
# ticks
for v in [-10, 0, 1, 10, 25, 50]:
    x = px(v) + (px(v + 1) - px(v)) / 2
    c.line(x, AY + H, x, AY + H + 5, stroke=GREY)
    c.text(x, AY + H + 17, str(v), size=10, color=MUTED, anchor="middle", mono=True)

# bar colour states
c.text(X0, AY - 26, "ratio bar value = hp + 10 against maxHp + 10 (DrawMemberHp, 1547:033F)", size=10.5, color=INK2)
by = AY + H + 34
c.text(X0, by, "bar colour state 2 when hp <= 0, state 1 when (maxHp + 10) / 3 > hp + 10, else state 0", size=10.5, color=INK2)
c.text(X0, by + 15, "here: state 1 for hp 1 to 9, because (50 + 10) / 3 = 20 > hp + 10", size=10.5, color=MUTED)

# MemberIs table
TY = 196
c.section(0, TY - 6, "MemberIs(who, mask): every bit of the mask must hold (0D1C:0670)")
bits = [("1", "status bit 0: the slot holds a character"), ("2", "hp > -10: not dead"), ("4", "hp > 0: not unconscious"),
        ("8", "status bit 2 clear: not paralysed"), ("16", "status bit 1 clear: not poisoned")]
for i, (b, t) in enumerate(bits):
    y = TY + 4 + i * 20
    c.pill(0, y + 8, b, "navy", size=10.5, solid=True, mono=True)
    c.text(40, y + 12, t, size=10.5, color=INK2)
uses = [("1", "drawing, picking a member to swap or drop"), ("3", "healing, experience shares, the +1 hit point of a long rest"),
        ("5", "eating, camp spells, the party-wiped test: conscious"), ("13", "using items (1 + 4 + 8)")]
c.text(400, TY + 16, "common masks", size=10.5, weight=700, color=MUTED)
for i, (m, t) in enumerate(uses):
    y = TY + 32 + i * 20
    c.pill(400, y + 8, m, "teal", size=10.5, solid=True, mono=True)
    c.text(436, y + 12, t, size=10.5, color=INK2)

# status byte
SY = 330
c.text(0, SY - 6, "status byte (+01):", size=10.5, weight=700, color=MUTED)
st = [("bit 0", "present", "grey"), ("bit 1", "poisoned (timer type 8)", "red"), ("bit 2", "paralysed (timer type 9)", "red")]
x = 128
for t, d, k in st:
    c.pill(x, SY - 10, t, k, size=10.5, solid=True)
    w = text_width(d, 10.5) + 20
    c.text(x + 44, SY - 6, d, size=10.5, color=INK2)
    x += 44 + text_width(d, 10.5) + 22
c.text(0, SY + 18, "Damage stops at -10 (DamageCharacter keeps only status bit 0 then); a dead member stays in the party until a temple heals or resurrects.", size=10.5, color=MUTED)
c.save("figs/chr_status.svg")

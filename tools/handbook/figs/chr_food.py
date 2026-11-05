import sys; sys.path.insert(0, '.')
from figlib import *

c = Canvas(760, 296)
c.section(0, 11, "Food: one byte per character, 0 to 100")

X0, X1 = 20, 740
def fx(v):
    return X0 + v / 100.0 * (X1 - X0)
Y = 44
H = 32
zones = [(0, 0, "red", ""), (1, 19, "red", "bar state 2 (below 20)"), (20, 32, "ochre", "state 1"), (33, 100, "teal", "state 0 (33 and up): normal")]
for a, b, k, t in zones:
    tc, sc, fc = PAL[k]
    c.rect(fx(a), Y, fx(b + 1) - fx(a) if b > a else 4, H, fill=SOLID[k] if a == 0 and b == 0 else fc, stroke=sc, sw=1.2, rx=0)
    if t:
        c.text((fx(a) + fx(b + 1)) / 2, Y + 20, t, size=10.5, weight=600, color=tc, anchor="middle")
for v in (0, 20, 33, 100):
    c.line(fx(v) + (2 if v == 0 else 0), Y + H, fx(v) + (2 if v == 0 else 0), Y + H + 6, stroke=GREY)
    c.text(fx(v) + (2 if v == 0 else 0), Y + H + 18, str(v), size=10.5, color=MUTED, anchor="middle", mono=True)

# what moves it
MY = 126
c.text(0, MY, "down: one per minute of play", size=11.5, weight=700, color="#80311F")
c.text(0, MY + 16, "Effect 14 (DecayMemberState, 1414:0C82) runs every 1080 ticks, about 59 s at 18.2 ticks a second,", size=10.5, color=INK2)
c.text(0, MY + 30, "and takes 1 from every present member who has food and hp above -10: 100 lasts about 99 minutes.", size=10.5, color=INK2)
c.text(0, MY + 44, "A rest advances the clock without running the effect, so a whole rest costs one decrement at most.", size=10.5, color=INK2)

UY = MY + 76
c.text(0, UY, "up", size=11.5, weight=700, color="#14504A")
ups = [("Rations", "+25"), ("Iron Rations", "+50 (one stack +100)"), ("Kenku Egg", "+10 to +40"), ("Potion of Vitality", "set to 100"), ("Create Food", "all six set to 100")]
x = 0
for i, (t, v) in enumerate(ups):
    w = 140
    c.box(i * (w + 10), UY + 8, w, 42, "teal", t, [v], size=10, tsize=11)
c.text(0, UY + 62, "EatFood (0ACD:07E0) takes a ration or egg from the pointer (item types 31 and 49), adds its value and clamps at 100.", size=10.5, color=INK2)

DY = UY + 90
c.text(0, DY, "at 0", size=11.5, weight=700, color="#80311F")
c.text(0, DY + 16, "No spells are memorised or prayed for in a rest, the member cannot be picked for Memorize or Pray, no hit point is regained,", size=10.5, color=INK2)
c.text(0, DY + 30, "and in a rest of 24 hours the member loses 1 hit point. Walking around at food 0 has no effect at all.", size=10.5, color=INK2)
c.save("figs/chr_food.svg")

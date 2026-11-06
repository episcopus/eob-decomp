import sys; sys.path.insert(0, '.')
from figlib import *

# The 16 timed effects of ds:5BA8 (E1414.C) and the clock that drives them.
rows = [
    # idx, handler, period ticks, period text, role, initial state, kind
    (0,  "BlinkSelected",        "9",     "0.5 s",   "blink the member being moved", "off", "world"),
    (1,  "MoveDDEffects",        "3",     "0.16 s",  "step every flying item and missile", "on", "world"),
    (2,  "UpdateMonsterGroup 0", "20*",   "1.1 s*",  "monsters of update group 0", "on", "mon"),
    (3,  "UpdateMonsterGroup 1", "20*",   "1.1 s*",  "group 1 (offset by half a period)", "on", "mon"),
    (4,  "UpdateMonsterGroup 2", "20*",   "1.1 s*",  "group 2", "on", "mon"),
    (5,  "UpdateMonsterGroup 3", "20*",   "1.1 s*",  "group 3 (offset by half a period)", "on", "mon"),
    (6,  "RunCharEffects 0",     "var",   "var",     "member 0: the 10 timed slots", "off", "mem"),
    (7,  "RunCharEffects 1",     "var",   "var",     "member 1", "off", "mem"),
    (8,  "RunCharEffects 2",     "var",   "var",     "member 2", "off", "mem"),
    (9,  "RunCharEffects 3",     "var",   "var",     "member 3", "off", "mem"),
    (10, "RunCharEffects 4",     "var",   "var",     "member 4", "off", "mem"),
    (11, "RunCharEffects 5",     "var",   "var",     "member 5", "off", "mem"),
    (12, "StepDoor",             "5",     "0.27 s",  "move the door that is opening or closing", "off", "world"),
    (13, "CheckFarTable",        "10",    "0.55 s",  "wall animation phase", "on", "world"),
    (14, "DecayMemberState",     "1080",  "59 s",    "food of each member drops by 1", "on", "food"),
    (15, "RandomiseMonsterOffsets", "25", "1.4 s",   "jitter the drawn position of monsters", "on", "world"),
]
KIND = {"world": "grey", "mon": "red", "mem": "teal", "food": "ochre"}

c = Canvas(760, 470)
c.section(0, 11, "THE CLOCK AND THE SIXTEEN TIMED EFFECTS (gEffects, ds:5BA8)")

# --- top strip: the pipeline ---
y0 = 22
c.box(0, y0, 150, 46, "navy", "INT 1Ch", ["18.2 times a second"], size=11.5, tsize=12.5)
c.box(180, y0, 200, 46, "navy", "gGameTicks  ds:8492", ["counts only while", "gTimerEnableFlags bit 1 is set"], size=11, tsize=12.5)
c.box(410, y0, 160, 46, "teal", "RunEffects", ["each pass of RunMapLoop"], size=11.5, tsize=12.5)
c.box(600, y0, 160, 46, "teal", "fn(arg), then re-arm", ["expiry = now + period"], size=11, tsize=12.5)
for a, b in ((150, 180), (380, 410), (570, 600)):
    c.arrow([(a, y0 + 23), (b, y0 + 23)])

# --- table ---
ty = 84
cols = [(0, "#"), (26, "handler"), (206, "period (ticks)"), (290, "seconds"), (352, "what it does"), (650, "at start")]
c.rect(0, ty, 760, 20, fill="#ECE7DD", stroke="none", rx=0)
for x, h in cols:
    c.text(x + 6, ty + 14, h, size=11, weight=700, color=INK2)
rh = 21
for i, (idx, fn, pt, ps, role, ini, kind) in enumerate(rows):
    y = ty + 20 + i * rh
    tc, sc, fc = PAL[KIND[kind]]
    c.rect(0, y, 760, rh, fill=fc if i % 2 == 0 else CARD, stroke="none", rx=0)
    c.rect(0, y, 4, rh, fill=SOLID[KIND[kind]], stroke="none", rx=0)
    c.text(10, y + 15, str(idx), size=11.5, weight=700, color=tc)
    c.text(32, y + 15, fn, size=11, mono=True, color=INK)
    c.text(206 + 6, y + 15, pt, size=11.5, color=INK2)
    c.text(290 + 6, y + 15, ps, size=11.5, color=INK2)
    c.text(352 + 6, y + 15, role, size=11.5, color=INK2)
    c.text(650 + 6, y + 15, "armed" if ini == "on" else "disarmed", size=11, color=MUTED)
yb = ty + 20 + 16 * rh + 14
c.text(0, yb, "* the period of entries 2 to 5 is overwritten by each level file (LoadLevel): 15 to 35 ticks in the shipped levels; entries 3 and 5 are offset by half a period.", size=10.5, color=MUTED)
c.text(0, yb + 14, "var: the period is the time to the member's earliest timer (RunCharEffects); the entry is disarmed when all ten slots are empty.", size=10.5, color=MUTED)
c.legend(0, yb + 34, [("grey", "world"), ("red", "monsters"), ("teal", "party members"), ("ochre", "food")])
c.h = yb + 46
c.save("figs/cmb_effects.svg")

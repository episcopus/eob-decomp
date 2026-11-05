import sys, math; sys.path.insert(0, '.')
from figlib import *

c = Canvas(760, 372)
c.section(0, 11, "One pass of RunMapLoop (0ACD:0E8A)")
items = [
    ("PollMapInput", "one key or mouse event if one waits (never blocks); node walk and callback; HandleMapKey when the mouse is off", "navy"),
    ("RunEffects", "each due entry of the 16-entry timer table runs its handler; then TickLevelScriptTimer", "teal"),
    ("DrawViewFrame(1)", "only when an action set gViewDirty", "plum"),
    ("ambient sound", "levels 1 to 3 only: (1d10+3) x 18 ticks apart, sound 1Eh at a cell 0 to 11 from the party", "ochre"),
    ("PlaySoundDefault(0)", "plays the sound recorded by the previous call (one behind)", "ochre"),
    ("TurnUndead", "levels 2 and 7 only: a cleric or paladin may turn the undead ahead", "red"),
]
y = 24
LW = 372
for i, (t, b, k) in enumerate(items):
    ls = wrap(b, LW - 40, 11)
    h = 27 + 13.5 * len(ls)
    tc, sc, fc = PAL[k]
    c.rect(0, y, LW, h, fill=fc, stroke=sc, sw=1.1, rx=5)
    c.badge(14, y + 15, str(i + 1), k, r=8)
    c.text(30, y + 18, t, size=12.5, weight=700, color=tc, mono=True)
    for j, l in enumerate(ls):
        c.text(30, y + 33 + 13.5 * j, l, size=11, color=INK2)
    y += h + 7
c.text(0, y + 8, "Then the loop starts again at once: there is no wait, no HLT; the", size=11, color=MUTED)
c.text(0, y + 22, "pace comes from the tick counter, not from the loop.", size=11, color=MUTED)

# right panel: the timer table
RX = 410
c.section(RX, 11, "The 16 timed effects (ds:5BA8)")
rows = [
    ("MoveDDEffects", 3, "thrown items and spells in flight", "navy"),
    ("StepDoor", 5, "a door that is opening or closing", "navy*"),
    ("BlinkSelected", 9, "frame of the member being moved", "navy*"),
    ("CheckFarTable", 10, "animated walls (teleporter) flip", "navy"),
    ("UpdateMonsterGroup x4", 20, "monster groups 0 to 3", "red"),
    ("RandomiseMonsterOffsets", 25, "jitter of drawn monsters", "red"),
    ("RunCharEffects x6", 50, "poison, spells, timers of each member", "teal*"),
    ("DecayMemberState", 1080, "food of each member", "teal"),
]
def bx(v):
    return RX + 4 + (math.log10(v) / math.log10(1100)) * 150
by = 30
for name, v, note, k in rows:
    c.text(RX, by + 12, name, size=11, weight=600, color=INK, mono=False)
    w = bx(v) - (RX + 4)
    kk = k.rstrip('*')
    c.rect(RX + 4, by + 17, w, 8, fill=(MID[kk] if k.endswith('*') else SOLID[kk]), stroke=SOLID[kk], sw=1, rx=2)
    c.text(RX + 4 + w + 6, by + 25, "%d (%.1f s)" % (v, v / 18.2), size=10.5, color=MUTED)
    c.text(RX, by + 39, note, size=10.5, color=MUTED)
    by += 49
c.text(RX, by + 4, "Pale bars start disarmed and are armed on demand. Period in ticks", size=10.5, color=MUTED)
c.text(RX, by + 17, "(1 tick = 1/18.2 s), log scale; LoadLevel resets the monster periods.", size=10.5, color=MUTED)
c.h = max(y + 30, by + 26)
c.save("figs/arc_tick.svg")

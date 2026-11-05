import sys; sys.path.insert(0, '.')
from figlib import *

c = Canvas(760, 520)
c.section(0, 11, "THE TIMED-EFFECT SLOTS OF A MEMBER: ARM, TICK, EXPIRE")

def tbox(x, y, w, kind, title, lines, size=10.5, h=None):
    out = []
    for l in lines:
        out += wrap(l, w - 32, size)
    hh = h or (10 + 17 + len(out) * size * 1.3 + 8)
    c.box(x, y, w, hh, kind, title, out, size=size, tsize=12, align="left")
    return y + hh

# left: ten slots (an example)
c.text(0, 40, "CharRec +AD and +D5", size=12, weight=700, color=INK)
c.text(0, 54, "ten slots per member (example values)", size=10.5, color=MUTED)
ex = [("-26", "Bless", "9840"), ("8", "poison", "10386"), ("-6", "Shield", "12480"), ("2", "attack", "9114"), ("", "", ""), ("", "", ""), ("", "", ""), ("", "", ""), ("", "", ""), ("", "", "")]
c.rect(0, 62, 66, 18, fill="#ECE7DD", stroke="none", rx=0)
c.text(4, 75, "type", size=10, weight=700, color=INK2)
c.text(70, 75, "expires at", size=10, weight=700, color=INK2)
for i, (t, nm, tk) in enumerate(ex):
    y = 82 + i * 21
    k = "teal" if t else "grey"
    tc, sc, fc = PAL[k]
    c.rect(0, y, 64, 19, fill=fc, stroke=sc, sw=1, rx=3)
    c.rect(68, y, 100, 19, fill=fc, stroke=sc, sw=1, rx=3)
    if t:
        c.text(6, y + 14, t, size=11, weight=700, color=tc, mono=True)
        c.text(30, y + 14, nm, size=9.5, color=MUTED)
        c.text(74, y + 14, tk, size=11, color=INK2, mono=True)
    else:
        c.text(6, y + 14, "0", size=11, color=MUTED, mono=True)
        c.text(74, y + 14, "0 = empty", size=10.5, color=MUTED)
c.text(0, 82 + 10 * 21 + 14, "tick counts in gGameTicks;", size=10, color=MUTED)
c.text(0, 82 + 10 * 21 + 27, "saved games store them relative", size=10, color=MUTED)
c.text(0, 82 + 10 * 21 + 40, "(AgeCharTimers, ovl3:123D)", size=10, color=MUTED)

# middle: arm and tick
xb, wb = 196, 290
m1 = tbox(xb, 24, wb, "teal", "Arm: ApplyCharEffect  1414:05ED", [
    "(who, interval, type, firstEmpty = 1)",
    "time = gGameTicks + interval; the member's effect (6 + who)",
    "not armed: period = interval, slot 0 = (type, time), arm it",
    "armed: pull its expiry back to time if that is earlier; reuse the slot that already has this type, else the first empty slot",
    "all ten full and no such type: slot = -1, the stores land in the bytes before the arrays (QUIRK 1414:0755)"])
m2 = tbox(xb, m1 + 12, wb, "plum", "Tick: RunCharEffects  1414:07CA", [
    "RunEffects calls it for effect 6 + who",
    "for every slot with time <= gGameTicks: clear it, then by type (right)",
    "afterwards: period = the smallest remaining time - gGameTicks; with none left, period = -1 and the entry is disarmed"])
m3 = tbox(xb, m2 + 12, wb, "ochre", "Re-arm after a load or a drop", [
    "RearmCharEffects 1414:0B2E recomputes the same period for every present member; ClearCharEffectSlot clears all slots of one type"])
c.arrow([(xb + wb / 2, m1), (xb + wb / 2, m1 + 12)])
c.arrow([(xb + wb / 2, m2), (xb + wb / 2, m2 + 12)], dash="3 3")
c.arrow([(168, 120), (xb, 120)], both=True)

# right: types
xr, wr = 512, 248
c.text(xr, 40, "Slot types (CharRec +D5)", size=12, weight=700, color=INK)
types = [
    ("-n", "spell n runs out: message \"%s's %s spell expires.\", gSpells[n].remove(rec)", "navy"),
    ("0, 1", "hand 0 or 1: flip its busy bit, clear its label", "red"),
    ("2, 3", "clear the label, arm type 4 or 5 for 36 ticks (18 if hasted)", "red"),
    ("4, 5", "flip the hand's busy bit: the hand is free", "red"),
    ("6", "clear lastDamage (the number on the portrait)", "grey"),
    ("7", "giant strength ends: strength and percentile back to base", "ochre"),
    ("8", "poison tick: 5 damage unless Slow Poison, re-arm 546 ticks; stops when the status bit is gone", "plum"),
    ("9", "paralysis ends, with its message", "plum"),
]
y = 52
for k, t, kind in types:
    lines = wrap(t, wr - 52, 10.5)
    h = max(24, 8 + len(lines) * 13.5)
    tc, sc, fc = PAL[kind]
    c.rect(xr, y, wr, h, fill=fc, stroke=sc, sw=1, rx=4)
    c.text(xr + 8, y + 16, k, size=11.5, weight=700, color=tc, mono=True)
    for i, l in enumerate(lines):
        c.text(xr + 46, y + 16 + i * 13.5, l, size=10.5, color=INK2)
    y += h + 5
c.h = max(m3, y, 82 + 10 * 21 + 54) + 10
c.save("figs/cmb_slots.svg")

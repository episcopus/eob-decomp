import sys; sys.path.insert(0, '.')
from figlib import *

c = Canvas(760, 505)
c.section(0, 11, "PlaySoundAtCell: record now, play at the next call")

# ---- left: the call sequence ----
LX, LW = 0, 360
c.text(LX, 36, "Calls as the oracle traced them (load-walk, party at cell 490)", size=11, color=MUTED)
hdr = ["call", "plays now", "records"]
cx = [LX, LX + 112, LX + 252]
rows = [
    ("Default(0)", "nothing yet", "driver 0, att 0"),
    ("AtCell(75, 309)", "nothing (0 is not > 0)", "driver 133, att 14"),
    ("AtCell(75, 245)", "133 at att 14", "driver 133, att 15"),
    ("AtCell(75, 555)", "133 at att 15", "driver 133, att 2"),
    ("Default(0)", "133 at att 2", "driver 0, att 0"),
]
y = 56
c.rect(LX, y - 14, LW, 20, fill=PANEL, rx=3)
for x, h in zip(cx, hdr):
    c.text(x + 6, y, h, size=10.5, weight=700, color=PAL["navy"][0])
for r in rows:
    y += 24
    c.line(LX, y - 15, LX + LW, y - 15, stroke=RULE)
    for x, t, i in zip(cx, r, range(3)):
        c.text(x + 6, y, t, size=10.5, mono=(i == 0), color=INK2)
c.line(LX, y + 9, LX + LW, y + 9, stroke=RULE)
c.para(LX, y + 28, "PlaySoundDefault(0) runs once per pass of the map loop (5,954 times in the load-walk session). "
       "Sound 0 maps to driver sound 0, which is never played by this path, so it flushes the sound recorded before it.",
       LW, size=10.5, color=MUTED)

# ---- right: the steps of one play ----
RX, RW = 392, 368
steps = [
    ("1", "spin until gBusyCountdown is 0"),
    ("2", "put the previous sound's two bytes back (+3, +1)"),
    ("3", "save this sound's +3 (L, the first LEVEL2 operand) and +1 (priority)"),
    ("4", "if att > 0: +3 := L + ((3Fh - L) * (att - 1) >> 4), +1 := 20h - att"),
    ("5", "QueueSound(id); gBusyCountdown := 0Bh"),
    ("6", "att := cell ? Distance(party, cell) : 0; record map[n]"),
]
c.text(RX, 36, "One play when the recorded att is below 10h (else nothing at all)", size=11, color=MUTED)
y = 46
for n, t in steps:
    ls = wrap(t, RW - 34, 11)
    h = 8 + 14 * len(ls)
    c.rect(RX, y, RW, h, fill=CARD, stroke=PAL["teal"][1], sw=1, rx=4)
    c.badge(RX + 14, y + h / 2, n, "teal")
    for k, l in enumerate(ls):
        c.text(RX + 30, y + 16 + 14 * k, l, size=11, color=INK2)
    y += h + 5

# ---- bottom: what the distance does ----
by = 262
c.section(0, by, "Distance and what it does to a sound whose first level operand is 5", color=PAL["teal"][0])
c.text(0, by + 18, "Distance = larger of dx, dy + half the smaller (cells). Bars: the carrier level offset after step 4 (0 loud, 3Fh silent).",
       size=10.5, color=MUTED)
L = 5
n = 16
bx0, bw, gap = 30, 36, 6
base = by + 146
hmax = 108
for a in range(n):
    lv = L if a == 0 else L + ((0x3f - L) * (a - 1) >> 4)
    h = hmax * lv / 63.0
    x = bx0 + a * (bw + gap)
    tc, sc, fc = PAL["teal"]
    if a == 0:
        tc, sc, fc = PAL["grey"]
    c.rect(x, base - h, bw, h, fill=fc, stroke=sc, sw=1, rx=2)
    c.text(x + bw / 2, base - h - 5, "%d" % lv, size=10.5, color=INK2, anchor="middle")
    c.text(x + bw / 2, base + 14, "%d" % a, size=11, weight=600, color=INK, anchor="middle")
    pr = 0x20 - a if a else 40
    c.text(x + bw / 2, base + 29, ("%d" % pr), size=10.5, color=MUTED, anchor="middle")
c.text(0, base + 14, "att", size=10.5, color=MUTED)
c.text(0, base + 29, "prio", size=10.5, color=MUTED)
c.text(bx0 + n * (bw + gap) - 2, base + 14, "10h and up", size=10.5, color=MUTED, anchor="start") if False else None
c.para(0, base + 50, "att 0 leaves the sound as the file has it (here a priority of 40 is just an example). "
       "From att 1 up the priority is 20h - att whatever the file said, so a nearer sound can interrupt a farther one on the same voice. "
       "From att 16 the sound is dropped.", 758, size=10.5, color=MUTED)
c.save("figs/snd_atcell.svg")

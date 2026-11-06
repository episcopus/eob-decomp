import sys; sys.path.insert(0, '.')
from figlib import *

c = Canvas(760, 462)
TC, TS, TF = PAL["teal"]
OC, OS, OF = PAL["ochre"]
NC, NS, NF = PAL["navy"]

# ---------------- panel 1: AdLib ----------------
c.section(0, 11, "AdLib driver: INT 08h on PIT channel 0, divisor 0555h (1,365), 874 Hz [H clock 1,193,182 Hz]")
x0, u = 20, 14.5          # one pass = 14.5 units; 48 passes = 696
y = 66
for p in range(49):
    x = x0 + p * u
    big = p % 12 == 0
    c.line(x, y + (0 if big else 8), x, y + 18, stroke=TS if not big else INK, sw=1.0 if not big else 1.6)
c.text(0, y + 12, "pass", size=10.5, color=MUTED, anchor="start") if False else None
c.text(x0, y - 8, "0", size=10.5, color=MUTED, anchor="middle")
for p in (12, 24, 36, 48):
    c.text(x0 + p * u, y - 8, str(p), size=10.5, color=MUTED, anchor="middle")
# driver tick markers
y2 = 106
c.text(0, y2 - 6, "every 12th pass: SoundDriverTick = ProcessSoundQueue, UpdateVoices, TickTempo (72.8 Hz)", size=11, color=TC, weight=600)
for p in (12, 24, 36, 48):
    x = x0 + p * u
    c.rect(x - 22, y2, 44, 24, fill=TF, stroke=TS, sw=1.2, rx=4)
    c.text(x, y2 + 16, "tick", size=11, weight=700, color=TC, anchor="middle")
# bios chain
y3 = 160
c.text(0, y3 - 8, "every 48th pass: jump to the BIOS INT 08h (the clock, then INT 1Ch: 18.2 Hz); the other passes send the EOI themselves", size=11, color=NC, weight=600)
x = x0 + 48 * u
c.rect(x - 70, y3, 70, 24, fill=NF, stroke=NS, sw=1.2, rx=4)
c.text(x - 35, y3 + 16, "BIOS chain", size=11, weight=700, color=NC, anchor="middle")
# busy countdown
y4 = 214
c.text(0, y4 - 8, "gBusyCountdown: PlaySoundAtCell sets 0Bh (11); each pass counts it down, so it is 0 one pass before the next tick", size=11, color=OC, weight=600)
for k in range(12):
    x = x0 + k * u
    h = 4 + (11 - k) * 2.2 if k < 12 else 2
    c.rect(x + 1, y4 + 30 - h, u - 2, h, fill=OF, stroke=OS, sw=1, rx=1)
c.text(x0 + 0.5 * u, y4 + 44, "11", size=10.5, color=MUTED, anchor="middle")
c.text(x0 + 11.5 * u, y4 + 44, "0", size=10.5, color=MUTED, anchor="middle")
c.text(x0 + 13 * u, y4 + 26, "pass 11: busy byte clear; pass 12: the driver tick reads the queue and the patched script bytes", size=10.5, color=MUTED)

# ---------------- panel 2: speaker ----------------
c.section(0, 285, "Speaker and Tandy driver: PIT channel 0 at divisor 1000h (4,096), 291 Hz [H]", color=PAL["ochre"][0])
u2 = 8.8
x0 = 20
y = 320
for p in range(81):
    x = x0 + p * u2
    big = p % 16 == 0
    c.line(x, y + (0 if big else 8), x, y + 18, stroke=OS if not big else INK, sw=1.0 if not big else 1.6)
for p in (0, 16, 32, 48, 64, 80):
    c.text(x0 + p * u2, y - 8, str(p), size=10.5, color=MUTED, anchor="middle")
c.text(x0 + 82 * u2 - 70, y - 8, "passes", size=10.5, color=MUTED) if False else None
# SoundTick every pass: band
c.rect(x0, y + 26, 80 * u2, 16, fill=OF, stroke=OS, sw=1, rx=2)
c.text(x0 + 40 * u2, y + 38, "SoundTick on every pass: one note or command per voice", size=11, color=OC, anchor="middle", weight=600)
# chains
for p in (16, 32, 48, 64, 80):
    x = x0 + p * u2
    c.rect(x - 10, y + 50, 20, 14, fill=NF, stroke=NS, sw=1, rx=2)
c.text(x0 + 6, y + 86, "blue: every 16th pass chains to the BIOS (18.2 Hz)", size=10.5, color=NC)
# echo voices
for p, lab in ((0, "voice 0 starts"), (35, "voice 1 starts (pass 35)"), (70, "voice 2 starts (pass 70)")):
    x = x0 + p * u2
    c.arrow([(x, y + 110), (x, y + 68)], color=SOLID["plum"])
    c.text(x + 4, y + 124, lab, size=10.5, color=PAL["plum"][0], anchor="start" if p < 70 else "end")

c.save("figs/snd_timer.svg")

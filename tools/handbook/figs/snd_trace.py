import sys; sys.path.insert(0, '.')
from figlib import *

c = Canvas(760, 380)
c.section(0, 11, "One short sound, byte by byte: driver sound 139, voice 3 (slots 08h, 0Bh)")
cols = [(0, "script"), (112, "what the driver does"), (350, "register writes (register value)"), (680, "ms")]
y = 40
c.rect(0, y - 15, 758, 22, fill=PANEL, rx=3)
for x, t in cols:
    c.text(x + 6, y, t, size=10.5, weight=700, color=PAL["navy"][0])
rows = [
    ("teal", "03 00", ["queued; the next driver tick calls", "ClearVoice, then VoiceSilence"],
     ["68 FF  6B FF  88 FF  8B FF", "B3 00  B3 20"], "0 to 2.7"),
    ("grey", "AB 14", ["LEVEL2: level offset 14h (the attenuation", "patched it to 26h in this play)"],
     ["no write"], ""),
    ("plum", "90 76", ["PATCH 76h: LoadVoicePatch, the carrier", "level is 00h + 26h"],
     ["28 00  2B 00  C3 0E  E8 00  EB 00", "48 00  4B 26  68 80  6B 2A", "88 00  8B 06"], "3.2 to 10.3"),
    ("ochre", "12 18", ["note, octave 1, semitone 2: F-number 15Ah,", "block 1, key on; wait 24 units"],
     ["A3 5A  B3 05  B3 25"], "10.9 to 12.2"),
    ("red", "88", ["END, 24 units later: VoiceKeyOff", "(speed FFh: one unit a driver tick)"],
     ["B3 05"], "315.5"),
]
y += 14
for kind, sc, did, wr, ms in rows:
    tc, sc_, fc = PAL[kind]
    h = max(len(did), len(wr)) * 15 + 14
    c.rect(0, y, 758, h, fill=fc, stroke=sc_, sw=1, rx=4)
    c.text(10, y + 20, sc, size=12.5, weight=700, color=tc, mono=True)
    for k, l in enumerate(did):
        c.text(118, y + 20 + 15 * k, l, size=10.5, color=INK2)
    for k, l in enumerate(wr):
        c.text(356, y + 20 + 15 * k, l, size=11, color=INK, mono=True)
    c.text(686, y + 20, ms, size=10.5, color=INK2)
    y += h + 6
c.para(0, y + 14, "Write times are the oracle's: it charges 20 us for each status-port read, so a write costs 540 to 640 us there. On a real card the 4 plus 23 reads "
       "are about 30 us. The wait itself is counted in driver ticks, not in time spent writing. The B3 05 at the end is the same register as the "
       "key-on write, with bit 5 cleared.", 758, size=10.5, color=MUTED)
c.save("figs/snd_trace.svg")

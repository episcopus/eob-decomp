import sys; sys.path.insert(0, '.')
from figlib import *

c = Canvas(760, 380)
OC, OS, OF = PAL["ochre"]
PC_, PS_, PF_ = PAL["plum"]
c.section(0, 11, "The speaker and Tandy driver: a song stream, the note table and the Tandy echo", color=OC)

# ---- panel A: a stream ----
c.text(0, 36, "A song: sound 1 of pcgamesn.dat", size=11.5, weight=700, color=OC)
bytesA = [("E2", "set"), ("01", "x1"), ("20", "A2"), ("00", ""), ("2C", "A3"), ("00", ""), ("38", "A4"), ("00", ""), ("FF", "end")]
x = 0
for i, (b, lab) in enumerate(bytesA):
    kind = "navy" if i < 2 else ("teal" if i % 2 == 0 else "grey") if i < 8 else "red"
    if i == 8: kind = "red"
    tc, sc, fc = PAL[kind]
    c.rect(x, 46, 26, 24, fill=fc, stroke=sc, sw=1, rx=3)
    c.text(x + 13, 62, b, size=11, mono=True, anchor="middle", weight=700, color=tc)
    x += 28
c.para(0, 92, "E2 01: length multiplier 1. Then three (note, duration) pairs: note byte 20h, 2Ch, 38h are notes 32, 44 and 56 of the table "
       "(110, 220 and 440 Hz), each with duration byte 0. A duration byte d lasts (d + 1) x multiplier - 1 passes, and a stored 0 still sounds for 2 passes (7 ms): "
       "a rising blip of about 21 ms. FFh ends the voice.", 244, size=10.5, color=MUTED)
c.text(0, 218, "Bytes in a stream", size=11.5, weight=700, color=OC)
cmds = [("00", "rest"), ("01-5F", "note index"), ("60-7F", "voice stays silent for good"), ("E2 n", "length multiplier := n (0 means 1)"),
        ("E8 n / E9 n", "multiplier minus / plus n"), ("E6 w", "note at frequency w Hz"), ("CD n, CE d", "repeat counter, loop back d bytes"),
        ("D1 n, D2 d", "second repeat counter and loop"), ("other 80-FF", "end of the voice")]
y = 236
for k, (a, b) in enumerate(cmds):
    c.text(0, y + 15 * k, a, size=10.5, mono=True, color=INK)
    c.text(78, y + 15 * k, b, size=10.5, color=INK2)

# ---- panel B: hardware ----
BX = 270
c.text(BX, 36, "A note on the hardware", size=11.5, weight=700, color=OC)
def stack(x, y, items, w, size=10.5):
    for it in items:
        y = c.para(x, y, it, w, size=size, color=INK2) + 4
    return y
c.rect(BX, 46, 226, 120, fill=OF, stroke=OS, sw=1.2, rx=5)
c.text(BX + 9, 64, "PC speaker, one voice", size=11.5, weight=700, color=OC)
stack(BX + 9, 80, ["table word = 1193180 / frequency (index 1: 18.35 Hz; index 95: 4,187 Hz)",
                   "out 42h: low byte, then high byte (channel 2 is in mode 3 from 0B6h written to 43h)",
                   "on: port 61h bits 0 and 1 set; off: cleared. Index 0 is a rest."], 208)
c.rect(BX, 176, 226, 150, fill=PF_, stroke=PS_, sw=1.2, rx=5)
c.text(BX + 9, 194, "Tandy, SN76489 on port C0h", size=11.5, weight=700, color=PC_)
stack(BX + 9, 210, ["table word = 111860 / frequency, at most 1023 (indexes 1 to 3 are clamped); index 95 is 27",
                    "tone: 1 cc 0 dddd (voice cc, low 4 bits), then 0 dddddd (high 6 bits)",
                    "volume: 1 cc 1 vvvv with the voice's value XOR 0Fh, 0 loudest [H]"], 208)
# ---- panel C: echo ----
CX = 518
c.text(CX, 36, "Tandy: one song, three voices", size=11.5, weight=700, color=PC_)
lane_y = [58, 124, 190]
delays = [0, 35, 70]
vols = [15, 10, 5]
u = 0.6  # units per pass
for k in range(3):
    y = lane_y[k]
    c.text(CX, y + 12, "voice %d" % k, size=10.5, weight=600, color=INK2)
    c.text(CX, y + 26, "volume %d" % vols[k], size=10.5, color=MUTED)
    c.rect(CX + 62, y, 180, 50, fill="#FFFFFF", stroke=RULE, sw=1, rx=3)
    start = CX + 62 + delays[k] * 1.0 + 4
    # three note blocks of 2 passes after delay, drawn wider for legibility
    c.line(CX + 62 + delays[k], y + 4, CX + 62 + delays[k], y + 46, stroke=PAL["plum"][1], sw=1, dash="3 2")
    for n in range(3):
        c.rect(start + n * 14, y + 10 + (2 - n) * 6 - 4, 12, 20, fill=MID["plum"] if k else PF_, stroke=PS_, sw=1, rx=2)
    h = 4 + vols[k] * 1.9
    c.rect(CX + 62 + 150, y + 46 - h, 12, h, fill=SOLID["plum"], stroke=PS_, sw=0.8, rx=1)
c.text(CX + 62, 252, "starts: pass 0, 35, 70", size=10.5, color=MUTED)
c.para(CX, 276, "All three voices read the same stream from byte 0: the song has no voice parts. StartSound sets position 0 for voices 0 to 2, and "
       "holds voices 1 and 2 back with the counters 23h and 46h. The volumes are the data image's: 15, 10, 5 (voice 3, 0, is never played). "
       "The result is an echo, quieter each time [I]. On the PC speaker gVoiceCount is 1, so only voice 0 sounds.", 242, size=10.5, color=MUTED)
c.save("figs/snd_speaker.svg")

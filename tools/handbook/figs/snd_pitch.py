import sys; sys.path.insert(0, '.')
from figlib import *

c = Canvas(760, 376)
c.section(0, 11, "From a note byte to registers A0h and B0h, and from a duration to ticks")

def bits(x, y, labels, w, kinds, h=30, title=None):
    xx = x
    for (lab, n), kind in zip(labels, kinds):
        tc, sc, fc = PAL[kind]
        ww = w * n
        c.rect(xx, y, ww, h, fill=fc, stroke=sc, sw=1.2, rx=3)
        c.text(xx + ww / 2, y + 20, lab, size=11, weight=700, color=tc, anchor="middle")
        xx += ww
    return xx

# note byte
c.text(0, 36, "note byte (the first byte of a note, below 80h)", size=11, weight=600, color=INK2)
bits(0, 44, [("octave", 4), ("semitone", 4)], 24, ["teal", "plum"])
c.text(0, 92, "bit 7 is clear; high nibble 0 to 7, low nibble 0 to 11", size=10.5, color=MUTED)
c.para(0, 114, "SetVoicePitch (4A3A): octave nibble + transpose (+07, 10h per octave), semitone + detune (+12, wraps at 12), "
       "F-number from the table + fine tune (+24), then the bend (+42) is added or subtracted through a 32-byte curve of gLevelCurvePtrs.", 330, size=10.5, color=INK2)

# registers
c.text(390, 36, "the registers it writes", size=11, weight=600, color=INK2)
c.text(390, 56, "A0h + voice", size=10.5, mono=True, color=MUTED)
bits(480, 42, [("F-number low 8 bits", 8)], 34, ["ochre"], h=24)
c.text(390, 92, "B0h + voice", size=10.5, mono=True, color=MUTED)
x = 480
for lab, n, kind in (("-", 2, "grey"), ("key", 1, "red"), ("block", 3, "teal"), ("F hi", 2, "ochre")):
    tc, sc, fc = PAL[kind]
    w = 34 * n
    c.rect(x, 78, w, 24, fill=fc, stroke=sc, sw=1.2, rx=3)
    c.text(x + w / 2, 94, lab, size=10.5, weight=700, color=tc, anchor="middle")
    x += w
c.text(480, 118, "bits 7 6  5  4 3 2  1 0", size=10.5, color=MUTED, mono=True)
c.para(390, 138, "Bit 5 is the key-on bit: VoiceKeyOn sets it, VoiceKeyOff clears it. The block is the octave; the F-number has 10 bits. On the OPL2 [H] f = F x 49,716 / 2^(20 - block) Hz.", 360, size=10.5, color=INK2)

# F-number table
c.text(0, 186, "gFNumbers (3E31): 12 words, one octave", size=11, weight=600, color=INK2)
fn = [0x134, 0x147, 0x15a, 0x16f, 0x184, 0x19c, 0x1b4, 0x1ce, 0x1e9, 0x207, 0x225, 0x246]
mx = max(fn)
for i, f in enumerate(fn):
    x = i * 62
    h = 70 * f / mx
    c.rect(x + 2, 262 - h, 54, h, fill=PAL["ochre"][2], stroke=PAL["ochre"][1], sw=1, rx=2)
    c.text(x + 29, 262 - h - 4, "%03X" % f, size=10.5, mono=True, color=INK, anchor="middle", weight=700)
    c.text(x + 29, 278, "%.1f" % (f * 49716 / 2 ** 16), size=10.5, color=INK2, anchor="middle")
    c.text(x + 29, 292, "semitone %d" % i, size=10, color=MUTED, anchor="middle") if i in (0, 11) else None
c.para(0, 312, "Under each bar: Hz at block 4 [H]. The steps are about 5.95 percent, a semitone; semitone 0 is near a B flat, not a C.", 700, size=10.5, color=MUTED)

# timing
c.para(0, 336, "A duration d counts down only when the voice's speed accumulator carries: units per driver tick = speed / 256, a tick is 13.7 ms (72.8 Hz), so d units last d x 256 / speed ticks. "
       "A speed of FFh is one unit per tick; the intro's three pieces set 168, 234 and 180.", 700, size=10.5, color=INK2)
c.save("figs/snd_pitch.svg")

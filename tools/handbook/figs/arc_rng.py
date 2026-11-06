import sys; sys.path.insert(0, '.')
from figlib import *

c = Canvas(760, 440)
# left: LCG
c.rect(0, 0, 366, 214, fill=PAL["navy"][2], stroke=PAL["navy"][1], sw=1.2, rx=6)
c.text(10, 20, "rand(): the C runtime's generator", size=13, weight=700, color=PAL["navy"][0])
c.text(356, 20, "0000:55B7", size=11, color=PAL["navy"][0], anchor="end", mono=True)
c.text(10, 42, "state: one 32-bit word, ds:8784 (image value 1)", size=11, color=INK2)
c.box(10, 54, 346, 34, "navy", None, None, fill=CARD)
c.text(183, 76, "seed = seed x 015A4E35h + 1   (mod 2^32)", size=12, weight=600, color=PAL["navy"][0], anchor="middle", mono=True)
c.box(10, 98, 346, 34, "navy", None, None, fill=CARD)
c.text(183, 120, "result = (seed >> 16) and 7FFFh      0 to 32767", size=12, weight=600, color=PAL["navy"][0], anchor="middle", mono=True)
c.para(10, 152, "Seeded once, in StartupGame (07C8:0199): srand(time(NULL)) stores the low 16 bits of the clock's seconds as the low word and 0 as the high word. So there are 65,536 possible games of dice, and a seed repeats about every 18 hours.", 346, size=10.8, color=INK2)
# right: shift mix
c.rect(394, 0, 366, 214, fill=PAL["plum"][2], stroke=PAL["plum"][1], sw=1.2, rx=6)
c.text(404, 20, "RandomByte: a 24-bit shift mix", size=13, weight=700, color=PAL["plum"][0])
c.text(750, 20, "1D8C:0000", size=11, color=PAL["plum"][0], anchor="end", mono=True)
c.text(404, 42, "state: 3 bytes b0 b1 b2 at ds:8484 (image value 34 12 76)", size=11, color=INK2)
bx = 404
for i, (n, x) in enumerate((("b2", 404), ("b1", 480), ("b0", 556))):
    c.box(x, 56, 64, 30, "plum", n, None, fill=CARD, tsize=12)
c.text(404, 104, "b2 and b1 shift left, b0 right; the bits that fall out feed the next byte.", size=10.5, color=INK2)
c.text(404, 118, "bit 1 of b0 enters b2; bit 7 of b2 enters b1; b0 takes a bit from", size=10.5, color=INK2)
c.text(404, 132, "(b0 >> 2) - b0 - carry. The result is  b0 xor b1  (one byte).", size=10.5, color=INK2)
c.para(404, 154, "Never seeded: every run walks the same 139,810-step cycle from the same start, so effects that use it look the same each time. Only two callers, both in the spell burst effect (E0EF3.C).", 346, size=10.8, color=INK2)
# consumers
c.section(0, 244, "Who uses them")
cons = [
    ("RollDice(count, sides, bonus)", "082C:0458", "sum of count rolls, each  rand x sides / 32767 + 1  (a 1 + sides is folded to 1), plus the bonus, which is a char: 0FFh means -1. About 90 call sites in 15 units: to-hit, damage, saves, hit points, 4d6 minus the lowest for abilities, 1d10+3 for the ambient sound.", "navy"),
    ("RandomRange(lo, hi)", "1AAA:0005", "rand x (hi - lo + 1) / 32767 + lo, drawn again when it exceeds hi. Particle speeds and spreads of spell bursts, the shuffle of the tile animation in overlay 5.", "navy"),
    ("RandomByte()", "1D8C:0000", "the low two bits or one bit of the result: the edge a burst particle starts on, whether it mirrors.", "plum"),
]
y = 256
for t, a, b, k in cons:
    h = 44 if len(b) < 130 else 60
    c.rect(0, y, 760, h, fill=PAL[k][2], stroke=PAL[k][1], sw=1, rx=5)
    c.text(10, y + 18, t, size=11.5, weight=700, color=PAL[k][0], mono=True)
    c.text(750, y + 18, a, size=10.5, color=PAL[k][0], anchor="end", mono=True)
    for j, l in enumerate(wrap(b, 740, 10.6)[:3]):
        c.text(10, y + 32 + 12 * j, l, size=10.6, color=INK2)
    y += h + 6
c.text(0, y + 10, "First values, run in the oracle on the original: rand from the image seed 346, 130, 10982, 1090, 11656; after srand(1234), RollDice(1, 6, 0)", size=10.5, color=MUTED)
c.text(0, y + 24, "gives 1, 6, 5, 5, 6, 3, 5, 4; RandomByte gives 62, 196, 85, 196, 63, 165, 39, 120 (each equals the Python model of the formulas above).", size=10.5, color=MUTED)
c.h = y + 36
c.save("figs/arc_rng.svg")

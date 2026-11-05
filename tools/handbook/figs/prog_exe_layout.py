import sys; sys.path.insert(0, '.')
from figlib import *

c = Canvas(760, 372)
c.section(0, 11, "EOB.EXE, 264,480 bytes, to scale")
TOT = 264480.0
def px(b): return 760 * b / TOT
parts = [
    ("MZ header + 2,504 relocations", 10240, "grey", None),
    ("code, 53 segments", 127435, "teal", None),
    ("stubs, asm data", 8469, "ochre", None),
    ("DGROUP 65,508", 65508, "navy", None),
    ("", 140, "grey", None),
    ("FBOV overlay data 52,688", 52688, "plum", None),
]
x = 0.0; by, bh = 34, 44
pos = []
for (n, b, k, _) in parts:
    w = px(b)
    tc, sc, fc = PAL[k]
    c.rect(x, by, w, bh, fill=fc, stroke=sc, sw=1.1, rx=0)
    pos.append((x, w, n, b, k))
    x += w
for (x0, w, n, b, k) in pos:
    if w > 60:
        tc = PAL[k][0]
        words = wrap(n, w - 10, 11.5, weight=600)
        for i, l in enumerate(words):
            c.text(x0 + w / 2, by + 20 + (i - (len(words) - 1) / 2) * 13, l, size=11.5, weight=600, color=tc, anchor="middle")
# offsets
for off, lab in ((0, "0"), (10240, "10,240"), (144400 - 10240 + 10240 - 0, "")):
    pass
c.text(0, by - 6, "0", size=10, color=MUTED)
c.text(px(10240) + 4, by - 6, "10,240", size=10, color=MUTED)
c.text(px(10240 + 201552), by - 6, "211,792", size=10, color=MUTED, anchor="middle")
c.text(759, by - 6, "264,480", size=10, color=MUTED, anchor="end")
# load module bracket
lx0, lx1 = px(10240), px(10240 + 201552)
c.line(lx0, by + bh + 8, lx1, by + bh + 8, stroke=SOLID["teal"], sw=1.5)
c.line(lx0, by + bh + 4, lx0, by + bh + 12, stroke=SOLID["teal"], sw=1.5)
c.line(lx1, by + bh + 4, lx1, by + bh + 12, stroke=SOLID["teal"], sw=1.5)
c.text((lx0 + lx1) / 2, by + bh + 25, "load module: 201,552 bytes, what DOS loads at 1000:0000", size=11, color=PAL["teal"][0], anchor="middle", weight=600)

# zoom segment 0
zy = 150
c.section(0, zy - 8, "Segment 0000 (_TEXT), 31,386 bytes: startup, a sound driver, the runtime")
z0, z1 = 0, 760
seg0 = [("C0L start-up", 524, "grey"), ("Westwood AdLib driver (E_TEXT.ASM), 19,996 bytes", 19996, "plum"), ("Borland runtime library, 10,866 bytes", 10866, "grey")]
x = 0.0
for (n, b, k) in seg0:
    w = 760 * b / 31386.0
    tc, sc, fc = PAL[k]
    c.rect(x, zy, w, 40, fill=fc, stroke=sc, sw=1.1, rx=0)
    if w > 100:
        c.text(x + w / 2, zy + 24, n, size=11.5, weight=600, color=tc, anchor="middle")
    x += w
c.text(760 * 524 / 31386.0 + 4, zy + 56, "020C", size=10, color=MUTED, mono=True)
c.text(0, zy + 70, "C0L: Borland start-up code, 524 bytes", size=10.5, color=MUTED)
c.text(760 * (524 + 19996) / 31386.0, zy + 56, "5028", size=10, color=MUTED, mono=True, anchor="middle")
c.text(759, zy + 56, "7A9A", size=10, color=MUTED, mono=True, anchor="end")

# the other segments
c.section(0, 260, "The 53 code segments: one per Westwood source module (62 units in all)")
y = c.para(0, 282, "resident code 127,435 bytes in 53 segments, 31,386 of them segment 0; seven stub segments (20F6 to 212A) call into overlay code", 690, size=11.5, color=INK2)
y = c.para(0, y + 4, "overlays: 7 segments, 125 entry points, 50,641 bytes of code, stored after the load module and loaded on demand by the VROOMM manager", 690, size=11.5, color=INK2)
y = c.para(0, y + 4, "DGROUP at paragraph 212E: typed constants, then variables, ending at 212E:FFE4; the stack lives past it (312D:0080)", 690, size=11.5, color=INK2)
y = c.para(0, y + 10, "Sizes are those of sections 2.1.2, 2.2 and 2.3; the 8,469 bytes between the code and DGROUP are what is left after subtracting them from the load module.", 700, size=10.5, color=MUTED)
c.h = y + 8
c.save("figs/prog_exe_layout.svg")

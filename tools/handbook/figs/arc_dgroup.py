import sys; sys.path.insert(0, '.')
from figlib import *

c = Canvas(760, 420)
W = 744.0
X0 = 8
H = 26

legend = []

def mark(px, y, lo, hi, k, num):
    pass

def strip(y, base, size, spans, title, nstart):
    c.text(X0, y - 40, title, size=11, weight=700, color=PAL["teal"][0])
    px = lambda off: X0 + W * (off - base) / size
    c.rect(X0, y, W, H, fill=CARD, stroke=GREY, sw=1, rx=0)
    lastx, level = -99, 0
    n = nstart
    for (lo, hi, k, lab) in spans:
        x0, x1 = px(lo), px(hi + 1)
        c.rect(x0, y, max(1.5, x1 - x0), H, fill=MID[k], stroke=SOLID[k], sw=1, rx=0)
        xm = (x0 + x1) / 2
        level = (level + 1) % 2 if xm - lastx < 18 else 0
        lastx = xm
        by = y - 10 - 14 * level
        c.line(xm, y, xm, by + 7, stroke=GREY, sw=0.8)
        c.badge(xm, by, str(n), k, r=7, size=8.5)
        legend.append((n, "%04X" % lo, lab))
        n += 1
    for off in range(0, size + 1, 0x2000):
        x = px(base + off)
        c.line(x, y + H, x, y + H + 4, stroke=GREY)
        c.text(x, y + H + 15, "%04X" % (base + off) if base + off <= 0xFFFF else "", size=9, color=MUTED, anchor=("start" if off == 0 else ("end" if off == size else "middle")), mono=True)
    return px, n

s1 = [
    (0x0000, 0x009E, "grey", "C0 start-up data: the copyright string and the two runtime messages"),
    (0x009F, 0x01FE, "ochre", "14 text-window regions (16 bytes), 8 option panels (16 bytes)"),
    (0x01FF, 0x110F, "teal", "file table: 203 records of 19 bytes (section 3.5)"),
    (0x1110, 0x3A61, "navy", "tables and strings: names, spells, monsters, walls"),
    (0x3A62, 0x5069, "plum", "screen node records on a 60-byte grid (section 5.10)"),
    (0x506A, 0x547D, "red", "the eight node pointer tables"),
    (0x547E, 0x7FFF, "navy", "view geometry, camp and dialog data, strings, file names"),
]
px1, n = strip(58, 0, 0x8000, s1, "Initialised data, ds:0000 to 7FFF", 1)
s2 = [
    (0x8026, 0x81EA, "ochre", "graphics driver entry points (802A), flags, mouse and video state"),
    (0x8484, 0x84B4, "red", "clocks and the 24-bit RNG state (8484, 8488 to 849E)"),
    (0x8784, 0x8787, "red", "rand() seed, the C runtime's"),
    (0x88A0, 0x8A66, "ochre", "PC speaker and Tandy driver data"),
    (0x8A82, 0x8DC9, "red", "gMonsters: 30 records of 28 bytes (zero in the file)"),
    (0x8DD0, 0xA927, "teal", "gItems: 500 records of 14 bytes"),
    (0xA9F0, 0xAD7F, "teal", "gItemTypes: 57 records of 16 bytes"),
    (0xB894, 0xD293, "navy", "gMaze: 1,024 cells of 9 bytes"),
    (0xDD2C, 0xDDDF, "plum", "gProjectiles: 10 records of 18 bytes"),
    (0xEFD4, 0xF2F3, "plum", "gDecorShapes: 200 far pointers"),
    (0xF580, 0xFB31, "teal", "gChars: 6 records of 243 bytes"),
    (0xFBC8, 0xFC07, "red", "the 64-byte block of the saved game (section 5.11)"),
    (0xFC0C, 0xFC1B, "ochre", "page segment table: 8 words"),
    (0xFEEA, 0xFEFD, "teal", "file handle slots: 10 words"),
]
y2 = 156
px2, n = strip(y2, 0x8000, 0x8000, s2, "Initialised up to 8A56, then uninitialised: ds:8000 to FFE4", n)
bx = px2(0x8A57)
c.line(bx, y2 - 6, bx, y2 + H, stroke=SOLID["red"], sw=1.4, dash="3 2")
c.text(bx + 6, y2 + H + 30, "8A57: from here the file holds zeros", size=9.8, color=PAL["red"][0])
# legend in two columns
ly = 226
col = [legend[:11], legend[11:]]
for ci, items in enumerate(col):
    x = ci * 380
    for i, (num, off, lab) in enumerate(items):
        y = ly + i * 17
        c.badge(x + 8, y - 3, str(num), "grey", r=7, size=8.5)
        c.text(x + 20, y, off, size=10, color=INK2, mono=True)
        for t in wrap(lab, 316, 10.2)[:1]:
            c.text(x + 56, y, t, size=10.2, color=INK2)
c.h = ly + 11 * 17 + 4
c.save("figs/arc_dgroup.svg")

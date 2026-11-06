import sys; sys.path.insert(0, '.')
from figlib import *

c = Canvas(760, 372)
c.section(0, 11, "The packed-file header and the five formats Unpack understands")
hy = 36
fields = [("u16", "size of the rest", 120), ("u8", "format", 70), ("u8", "0", 40), ("u32", "unpacked size", 120), ("u16", "palette bytes", 110), ("...", "palette, then data", 150)]
x = 0
for (t, n, w) in fields:
    k = "navy" if t != "..." else "grey"
    tc, sc, fc = PAL[k]
    c.rect(x, hy, w, 46, fill=fc, stroke=sc, rx=0)
    c.text(x + w / 2, hy + 18, n, size=11, weight=600, color=tc, anchor="middle")
    c.text(x + w / 2, hy + 35, t, size=10.5, color=MUTED, anchor="middle", mono=True)
    x += w
for (xx, lab) in ((0, "+0"), (120, "+2"), (190, "+3"), (230, "+4"), (350, "+8")):
    c.text(xx + 2, hy - 5, lab, size=10, mono=True, color=MUTED)
c.text(420, hy - 5, "(src+2: unpacked size, src+6: palette; 1B82:0041)", size=10, color=MUTED)
fy = 118
rows = [
    ("0", "stored", "copied from src+8", "Unpack0", "grey"),
    ("1", "12-bit LZW", "3 bytes for 2 codes; dictionary of (offset, length); 3C00h work buffer", "5 EGA pictures", "plum"),
    ("2", "Unpack2", "decoded with an FC00h work buffer; no pinned file uses it", "", "grey"),
    ("3", "Unpack3", "no pinned file uses it", "", "grey"),
    ("4", "LCW (Format 80)", "back reference, literal run, absolute copy, fill; palette before data", "130 files", "teal"),
]
c.text(0, fy - 6, "format", size=10.5, color=MUTED, weight=600)
c.text(40, fy - 6, "method", size=10.5, color=MUTED, weight=600)
c.text(180, fy - 6, "how the data is coded", size=10.5, color=MUTED, weight=600)
c.text(760, fy - 6, "used by", size=10.5, color=MUTED, weight=600, anchor="end")
for i, (f, m, d, u, k) in enumerate(rows):
    y = fy + i * 24
    tc, sc, fc = PAL[k]
    c.rect(0, y, 760, 22, fill=fc if i % 2 == 0 else CARD, stroke="none", rx=2)
    c.badge(14, y + 11, f, k, r=8)
    c.text(40, y + 15, m, size=11.5, weight=600, color=tc)
    c.text(180, y + 15, d, size=10.5, color=INK2)
    c.text(752, y + 15, u, size=10.5, color=MUTED, anchor="end")
ly = 280
c.section(0, ly - 8, "LCW commands (the first byte decides)", color=PAL["teal"][0])
ops = [
    ("00 to 7F", "copy from behind", "n = ((c SHR 4) AND 7) + 3 bytes; distance = 12 bits (low nibble of c, next byte)", "teal"),
    ("80 to BF", "literal run", "n = c AND 3F bytes follow; 80 itself ends the data", "navy"),
    ("C0 to FD", "copy from the output", "n = (c AND 3F) + 3 bytes from an absolute u16 position", "plum"),
    ("FE", "fill", "u16 count, u8 value: the byte repeated", "ochre"),
    ("FF", "long absolute copy", "u16 count, u16 position", "red"),
]
for i, (r, n, d, k) in enumerate(ops):
    y = ly + i * 22
    tc, sc, fc = PAL[k]
    c.rect(0, y, 70, 19, fill=SOLID[k], stroke="none", rx=3)
    c.text(35, y + 13.5, r, size=10, mono=True, weight=700, color="#FFFFFF", anchor="middle")
    c.text(82, y + 14, n, size=11, weight=600, color=tc)
    c.text(230, y + 14, d, size=10.5, color=INK2)
c.h = ly + 5 * 22 + 4
c.save("figs/data_unpack.svg")

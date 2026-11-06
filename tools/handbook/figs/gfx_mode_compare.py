import sys; sys.path.insert(0, '.')
from figlib import *

c = Canvas(760, 560)
c.section(0, 11, "THE FOUR GRAPHICS CHOICES SIDE BY SIDE")

LX, CW = 118, 160
heads = [("0 CGA", "CGA.OVL, BIOS mode 4", "grey"), ("1 Tandy", "TGA.OVL", "ochre"), ("2 EGA", "EGA.OVL, mode 0Dh", "navy"), ("3 VGA / MCGA", "MCGA.OVL, mode 13h", "teal")]
for i, (h, s, k) in enumerate(heads):
    x = LX + i * CW
    c.rect(x + 2, 22, CW - 4, 36, fill=PAL[k][2], stroke=PAL[k][1], sw=1.2, rx=5)
    c.text(x + CW / 2, 38, h, size=12.5, weight=700, color=PAL[k][0], anchor="middle")
    c.text(x + CW / 2, 51, s, size=10, color=INK2, anchor="middle")

# row 1: eight pixels as stored
y = 70
c.text(0, y + 24, "8 pixels as", size=10.5, weight=700, color=INK)
c.text(0, y + 37, "bytes on screen", size=10.5, weight=700, color=INK)
def px_strip(col, nbits, nbytes, planes=1):
    x0 = LX + col * CW + 6
    w = CW - 16
    k = ["grey", "ochre", "navy", "teal"][col]
    if planes == 1:
        bw = w / nbytes
        for b in range(nbytes):
            c.rect(x0 + b * bw, y + 8, bw, 22, fill=PAL[k][2], stroke=PAL[k][1], sw=1, rx=0)
        pw = w / 8
        for p in range(8):
            c.line(x0 + p * pw, y + 30, x0 + p * pw, y + 36, stroke=PAL[k][1], sw=0.8)
        c.text(x0 + w / 2, y + 45, "%d byte%s, %d bits a pixel" % (nbytes, "" if nbytes == 1 else "s", nbits), size=9.5, color=PAL[k][0], anchor="middle", weight=600)
    else:
        for p in range(planes):
            c.rect(x0, y + 4 + p * 8, w, 7, fill=PAL[k][2], stroke=PAL[k][1], sw=0.8, rx=0)
        c.text(x0 + w / 2, y + 45, "1 byte in each of 4 planes", size=9.5, color=PAL[k][0], anchor="middle", weight=600)
px_strip(0, 2, 2)
px_strip(1, 4, 4)
px_strip(2, 1, 1, planes=4)
px_strip(3, 8, 8)

rows = [
    ("line layout", ["80 bytes; even lines B800:0000, odd lines B800:2000", "160 bytes; line y in bank y mod 4 (2000h apart), row y / 4", "40 bytes a plane, the four planes share A000:y * 40; writes use write mode 2 and the bit mask", "320 bytes, A000:y * 320"]),
    ("colours", ["4; 16 art colours become two CGA colours in a checkerboard (32-byte map)", "16; video array palette registers 10h to 1Fh", "16; attribute controller registers, colours above 7 at 10h to 17h", "256; INT 10h AX=1012h loads all 768 DAC bytes"]),
    ("wall files", [".ECN + .EMP: BRICK has 1,484 blocks; blocks converted to CGA pixels by BuildMask", ".ECN + .EMP; blocks used as stored", ".ECN + .EMP; blocks copied into video RAM planes from A400 up, where DrawTile reads them", ".VCN + .VMP: BRICK has 1,575 blocks, 2 colour tables"]),
    ("DrawTile", ["own CGA path (17A6:063E)", "own Tandy path (17A6:0856)", "EGA path with the sequencer and graphics controller (17A6:0446)", "table lookups into page 2 (17A6:0ADA)"]),
    ("a shape cell", ["CGA pixels followed by a transparency mask", "two 4-bit pixels a byte, no mask: colour 0 keeps the screen", "(mask, colour) pair for each colour of each 8-pixel group", "4-bit pixels with a 16-colour table of their own"]),
    ("page 0", ["16 KB at B800", "B800, 4 banks", "A000, 8,000 bytes a plane", "A000, 64,000 bytes"]),
    ("UI and flash colours", ["2, 9, 0Dh, 0Eh ...; hit flash row ds:1CE3", "the same as EGA; row ds:1CD3", "2, 9, 0Dh, 0Eh ... (SetUiPalette); row ds:1CD3", "84h, 82h, 87h, 0B4h ...; row ds:1CD3 (everything but 0 to white)"]),
]
yy = y + 58
for label, cells in rows:
    hmax = 0
    wr = []
    for t in cells:
        ls = wrap(t, CW - 16, 9.8)
        wr.append(ls)
        hmax = max(hmax, len(ls))
    h = hmax * 12 + 10
    c.line(0, yy - 4, 758, yy - 4, stroke=RULE)
    c.text(0, yy + 9, label, size=10.5, weight=700, color=INK)
    for i, ls in enumerate(wr):
        for j, l in enumerate(ls):
            c.text(LX + i * CW + 8, yy + 9 + j * 12, l, size=9.8, color=INK2)
    yy += h + 4
c.h = yy + 4
c.save("figs/gfx_mode_compare.svg")

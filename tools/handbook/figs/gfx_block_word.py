import sys; sys.path.insert(0, '.')
from figlib import *

c = Canvas(760, 470)
c.section(0, 11, "THE BLOCK WORD, THE VCN FILE AND ONE 8 BY 8 BLOCK")

# (a) the 16-bit block word
x0, y0 = 0, 30
bits = [("15", "overlay", "red", 70), ("14", "mirror", "ochre", 70), ("13 .. 0", "block number", "teal", 270)]
x = x0
for lab, what, k, w in bits:
    c.box(x, y0, w, 44, k, what, None, tsize=12.5)
    c.text(x + w / 2, y0 - 4, lab, size=10.5, color=MUTED, anchor="middle", mono=True)
    x += w
c.text(x + 8, y0 + 18, "one u16 per 8 by 8 block of the", size=11, color=INK2)
c.text(x + 8, y0 + 32, "view, two layers of 22 x 15", size=11, color=INK2)
yy = y0 + 62
rules = [
    ("0", "layer 1 is empty here: show the background block (layer 2), drawn through colour table A"),
    ("n", "draw block n of the .VCN, opaque, through table B"),
    ("n | 4000h", "the same block with its columns reversed (a mirrored copy)"),
    ("8000h | n", "draw the background block first, then block n over it; pixels of colour 0 let the background show"),
]
for w_, t in rules:
    c.text(0, yy, w_, size=11.5, weight=700, color=INK, mono=True)
    c.text(100, yy, t, size=11.5, color=INK2)
    yy += 17

# (b) VCN file
y1 = 192
c.section(0, y1 - 10, "A .VCN FILE (VGA; THE EGA FILE .ECN HAS THE SAME FRAME)")
parts = [("u16", "count", 44, "grey"), ("16 bytes", "table A", 90, "ochre"), ("16 bytes", "table B", 90, "ochre"),
         ("count x 32 bytes", "8 by 8 blocks, 4 bits a pixel", 520, "teal")]
x = 0
for a, b, w, k in parts:
    c.box(x, y1, w, 44, k, a, [b], tsize=11.5, size=10.5)
    x += w
c.text(0, y1 + 62, "offsets 0, 2, 12h, 22h. BRICK.VCN: 1,575 blocks (50,434 bytes), BLUE 1,512, DROW 1,456, GREEN 1,523, XANATHA 1,305.", size=10.5, color=MUTED)
c.text(0, y1 + 76, "BRICK.ECN has 1,484 blocks and tables of zeros: the EGA and CGA blocks carry the screen's own colours.", size=10.5, color=MUTED)

# (c) block bytes
y2 = 306
c.section(0, y2 - 10, "ONE BLOCK: 8 ROWS OF 4 BYTES, HIGH NIBBLE FIRST (LEFT)")
cs = 36
for r in range(8):
    for b in range(4):
        bx, by = b * cs * 1.0 + 0, y2 + r * 15
        c.rect(bx * 1 + 0, by, cs * 1, 13, fill=PAL["teal"][2] if (r + b) % 2 == 0 else CARD, stroke=PAL["teal"][1], sw=0.8, rx=0)
        c.text(bx + cs / 2, by + 10, "%d,%d" % (2 * b, 2 * b + 1), size=9, color=INK2, anchor="middle")
c.text(0, y2 + 8 * 15 + 14, "pixels 0..7 of a row: bytes 0 to 3, high nibble left", size=10.5, color=MUTED)
c.text(0, y2 + 8 * 15 + 28, "block n starts 2 n paragraphs after gWallBlocks", size=10.5, color=MUTED)
# lookup arrows
ax = 160
c.arrow([(ax, y2 + 20), (ax + 70, y2 + 20)], color=GREY)
c.box(ax + 74, y2, 120, 34, "ochre", "colour table", ["16 entries"], size=10.5, tsize=12)
c.arrow([(ax + 194, y2 + 20), (ax + 244, y2 + 20)], color=GREY)
c.box(ax + 248, y2, 150, 34, "navy", "palette index", ["byte written to page"], size=10.5, tsize=12)
c.text(ax + 74, y2 + 54, "Tables of BRICK.VCN:", size=10.5, weight=700, color=INK)
c.text(ax + 74, y2 + 69, "A: 00 E7 E5 E6 E4 E3 E2 E0 E1 0C F5 F4 F3 F2 F1 F6", size=10.5, color=INK2, mono=True)
c.text(ax + 74, y2 + 84, "B: 00 E4 E5 E6 0C E7 E3 E2 E1 E0 F4 F5 F2 F3 F0 F1", size=10.5, color=INK2, mono=True)
c.text(ax + 74, y2 + 103, "A colours a nibble of the background layer, B of the wall layer; both keep 0 as 0.", size=10.5, color=MUTED)
c.text(ax + 74, y2 + 117, "The level's .PAL gives the 256 palette entries those indices name.", size=10.5, color=MUTED)
c.h = y2 + 140
c.save("figs/gfx_block_word.svg")

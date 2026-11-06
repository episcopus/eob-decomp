import sys; sys.path.insert(0, '.')
from figlib import *

c = Canvas(760, 372)
c.section(0, 11, "THE SHAPE: THE SPRITE FORMAT DrawShape READS")

def hdr(x, y, first, k):
    labs = [(first, "format"), ("rows", "height"), ("cols", "width / 8"), ("rows", "copy")]
    for i, (v, t) in enumerate(labs):
        c.rect(x + i * 52, y, 52, 34, fill=PAL["grey"][2], stroke=PAL["grey"][1], sw=1, rx=0)
        c.text(x + i * 52 + 26, y + 15, v, size=12, weight=700, color=PAL["grey"][0], anchor="middle", mono=True)
        c.text(x + i * 52 + 26, y + 28, t, size=9.5, color=INK2, anchor="middle")
    for i, t in enumerate(["+0", "+1", "+2", "+3"]):
        c.text(x + i * 52 + 26, y - 4, t, size=9.5, color=MUTED, anchor="middle", mono=True)

# format 0: 8-bit RLE
y1 = 44
c.text(0, y1 - 2, "Format 0: one byte a pixel, runs of colour 0 coded (GetSpriteCellRle: the character portraits)", size=11.5, weight=700, color=INK)
hdr(0, y1 + 14, "00", "grey")
c.rect(208, y1 + 14, 548, 34, fill=PAL["teal"][2], stroke=PAL["teal"][1], sw=1, rx=0)
c.text(482, y1 + 29, "rows of pixels: a byte 0 is followed by a count n: n transparent pixels", size=11.5, weight=700, color=PAL["teal"][0], anchor="middle")
c.text(482, y1 + 43, "(runs over 255 are split with FFh); any other byte is a palette index", size=10.5, color=INK2, anchor="middle")
c.text(0, y1 + 64, "Fragment of a row: 05 07 07 00 04 12 12 = pixels 5, 7, 7, four transparent, then 18, 18", size=10.5, color=MUTED, mono=True)

# format 1: 4-bit
y2 = 150
c.text(0, y2 - 2, "Format 1: four bits a pixel with its own colour table (GetSpriteCell: items, doors, decorations, monsters, UI)", size=11.5, weight=700, color=INK)
hdr(0, y2 + 14, "01", "grey")
c.rect(208, y2 + 14, 110, 34, fill=PAL["ochre"][2], stroke=PAL["ochre"][1], sw=1, rx=0)
c.text(263, y2 + 29, "16 bytes", size=11.5, weight=700, color=PAL["ochre"][0], anchor="middle")
c.text(263, y2 + 43, "colour table", size=9.5, color=INK2, anchor="middle")
c.rect(318, y2 + 14, 438, 34, fill=PAL["teal"][2], stroke=PAL["teal"][1], sw=1, rx=0)
c.text(537, y2 + 29, "rows x (cols * 4) bytes, two pixels a byte", size=11.5, weight=700, color=PAL["teal"][0], anchor="middle")
c.text(537, y2 + 43, "high nibble is the left pixel; code 0 is transparent", size=10.5, color=INK2, anchor="middle")
c.text(0, y2 + 64, "Size: 4 + 16 + rows * cols * 4 bytes. A 2-column, 16-row item icon is 148 bytes (16 x 16 pixels).", size=10.5, color=MUTED)

# what draws read
y3 = 252
c.section(0, y3 - 6, "WHO READS WHICH BYTE")
rows = [
    ("DrawShape, driver", "+0 picks the format; +1 rows, +2 columns (x 8 pixels); +3 is skipped"),
    ("the game, placing a shape", "the position is the bottom centre: x less (+2 * 8) / 2, y less +1 (DrawSlotMonsters, DrawSlotItems)"),
    ("pointer (SetMousePointerShape)", "height is +3, width is +2 plus one column (the shape may start between columns)"),
    ("clipping", "to the region SetRegion set (columns, lines); x is made even first (DrawShapeClipped)"),
    ("flipped and remapped forms", "DrawShapeFlipped writes right to left; the remapped ones send each nibble through a 16-colour row"),
]
yy = y3 + 12
for a, b in rows:
    c.text(0, yy, a, size=10.5, weight=700, color=INK2)
    for l in wrap(b, 470, 10.5):
        c.text(250, yy, l, size=10.5, color=INK2); yy += 13
    yy += 3
c.h = yy + 4
c.save("figs/gfx_shape_format.svg")

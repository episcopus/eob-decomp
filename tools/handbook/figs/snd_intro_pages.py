import sys; sys.path.insert(0, '.')
from figlib import *

c = Canvas(760, 316)
c.section(0, 11, "How a scene gets a picture to the screen (VGA)")

steps = [
    ("grey", "PAK file", ["one .CMP, LCW, 64,000 bytes unpacked"]),
    ("grey", "slot 5", ["LoadPackedImage reads the packed bytes here"]),
    ("teal", "slot 3", ["Unpack4 writes the 320 by 200 picture here"]),
    ("ochre", "heap", ["gMakeImage cuts rectangles into shapes (runs of 0 are transparent)"]),
    ("plum", "page 2 or 4", ["DrawShape draws a shape, clipped to the region"]),
    ("navy", "page 0", ["ClipRect copies a rectangle: the screen"]),
]
w, g = 106, 24
y = 34
for i, (kind, t, b) in enumerate(steps):
    x = i * (w + g)
    c.box(x, y, w, 88, kind, t, b, size=10.5, tsize=12)
    if i:
        c.arrow([(x - g + 1, y + 44), (x - 1, y + 44)], color=GREY)
c.para(0, 146, "Two other routes: CopyPage copies a whole page, and Dissolve moves pixels one at a time with GetPixel and PutPixel.", 700, size=10.5, color=MUTED)

# page table
c.section(0, 182, "gPageSegs after StartupGame, graphics choice 3 (I0892.C:337-342)", color=PAL["teal"][0])
pages = [
    ("pages 0, 1", "the screen, A000:0000", "navy"),
    ("pages 2, 3", "buffer A, MemAlloc 64,000 bytes", "teal"),
    ("pages 4, 5", "buffer B, 64,000 bytes", "plum"),
    ("pages 6, 7", "buffer C, 64,512 bytes", "grey"),
]
pw = 178
for i, (t, d, kind) in enumerate(pages):
    x = i * (pw + 15)
    c.box(x, 196, pw, 54, kind, t, [d], size=10.5, tsize=12)
c.para(0, 272, "Each pair of slots holds one segment on VGA: the even slot is the page a draw goes to, the odd slot the memory copy that "
       "LoadPackedImage and gMakeImage use (E19EC.C). A scene copies its picture to page 4 with CopyPage(3, 4) and then reveals it in pieces with "
       "ClipRect(..., 4, 0), rectangle by rectangle, one step per wait.", 700, size=10.5, color=MUTED)
c.save("figs/snd_intro_pages.svg")

import sys; sys.path.insert(0, '.')
from figlib import *

c = Canvas(760, 380)
c.section(0, 11, "WHO WRITES WHERE: THE THREE PAGES THE GAME USES (VGA NUMBERING)")

# three page columns
PX = {"disk": 20, "p3": 250, "p2": 430, "p0": 620}
def pagebox(x, w, kind, title, sub):
    c.box(x, 30, w, 54, kind, title, [sub], size=10, tsize=12)
pagebox(0, 150, "grey", "files", "CPS, EGA, VCN, DAT ...")
pagebox(230, 150, "ochre", "page 3: art page", "gArtPage; pictures and sheets")
pagebox(410, 150, "teal", "page 2: draw page", "gDrawPage = 2; composed frames")
pagebox(590, 160, "navy", "page 0: the screen", "A000; gDrawPage = 0")
for k in (305, 485, 670):
    pass
# lifelines
for x in (75, 305, 485, 670):
    c.line(x, 84, x, 340, stroke=RULE, sw=1.2, dash="3 3")
def msg(x1, x2, y, text, color=GREY, above=True):
    c.arrow([(x1, y), (x2, y)], color=color, label=text, lsize=10, ldy=-5, lcolor=INK2)
msg(75, 305, 112, "LoadPacked + Unpack", PAL["grey"][1])
c.arrow([(305, 150), (386, 150)], color=PAL["ochre"][1], label="GetSpriteCell", lsize=10, ldy=-5, lcolor=INK2)
c.box(390, 134, 190, 32, "ochre", "shape cells in far memory", ["MemAlloc'd blocks, one per cell"], size=9.5, tsize=11)
msg(305, 670, 200, "LoadScreenImage, mode 0: CopyPage(3, 0)", PAL["navy"][1])
msg(305, 485, 226, "mode 2: CopyPage(3, 2); skipped on VGA", PAL["teal"][1])
c.box(410, 244, 150, 36, "teal", "DrawTile, DrawShapeClipped(2)", ["the view, 176 x 120 at (0, 0)"], size=9.5, tsize=10.5)
msg(485, 670, 304, "CopyRegion(0, 0, 0, 0, 22, 120, 2, 0): the view", PAL["red"][1])
msg(485, 670, 328, "a party block: composed at x 8 or 80, copied to x + 176", PAL["plum"][1])
c.text(0, 352, "Direct to page 0: the pointer, the compass, text windows, the hand slots (while gDrawPage is 0).", size=10.5, color=INK2)
c.text(0, 366, "Panels that need an undo (the inventory) keep a pristine copy in the right half of page 2 and restore from it.", size=10.5, color=INK2)
c.h = 378
c.save("figs/gfx_page_traffic.svg")

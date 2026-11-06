import sys; sys.path.insert(0, '.')
from figlib import *

c = Canvas(760, 318)
c.section(0, 11, "The three programs and what they read")

# program boxes
def prog(x, name, kind, lines, w=210):
    c.box(x, 26, w, 104, kind, name, lines, size=11.5, tsize=15, title_mono=True)

prog(0, "START1.EXE", "ochre", ["8,652 bytes, small model", "setup menu: graphics, sound, mouse", "edited by the fan patch (v1.9)"])
prog(275, "INTRO.EXE", "navy", ["88,032 bytes, large model", "the introduction, 147 s", "23 units shared with EOB.EXE"])
prog(550, "EOB.EXE", "teal", ["264,480 bytes, large model", "the game: 62 units, 7 overlays", "title menu, party, 12 levels"])
c.arrow([(212, 78), (273, 78)], color=SOLID["ochre"], label="spawnl", lpos=0.5, ldy=-6)
c.arrow([(487, 78), (548, 78)], color=SOLID["navy"], label="load + jump", lpos=0.5, ldy=-6)
c.text(0, 148, "arguments: graphics, sound, mouse, \"homie\"", size=10.5, color=MUTED)
c.text(275, 148, "DOS 4B03h (load overlay) at 1000:0000", size=10.5, color=MUTED)

# data row
c.section(0, 182, "Files the game loads at run time")
cols = [
    ("EOBDATA1..6.PAK", "6 archives, 193 files", "wall sets, pictures, scripts, mazes, sound data", "teal"),
    ("*.OVL", "4 drivers", "MCGA, CGA, EGA, TGA: one is loaded", "plum"),
    ("FONT6, FONT8", "2 fonts", "6 by 6 and 8 by 8, 128 glyphs", "grey"),
    ("ITEM.DAT ...", "item and type tables", "432 items, 57 item types", "navy"),
    ("EOBDATA.SAV", "saved game", "33,101 bytes + LEVELS.TMP", "ochre"),
]
w = (760 - 4 * 10) / 5
for i, (t, s1, s2, k) in enumerate(cols):
    x = i * (w + 10)
    c.box(x, 196, w, 84, k, t, [s1, s2], size=10.5, tsize=11.5, title_mono=True)
c.arrow([(655, 132), (655, 194)], color=GREY, dash="3 2")
c.text(0, 304, "All three programs: Turbo C++ 1.00, TLINK 3.0, overlays in the VROOMM format appended to the EXE; sound and graphics drivers by TASM 2.0.", size=11, color=MUTED)
c.h = 312
c.save("figs/brief_programs.svg")

import sys; sys.path.insert(0, '.')
from figlib import *

c = Canvas(760, 410)
c.section(0, 11, "A WORKED SCRIPT: LEVEL 1, CELL 754 (x 18, y 23), THE STAIRS DOWN, FLAGS 0xA8 = MODES 1, 4 AND 0x10")

def node(x, y, w, h, kind, off, title, lines):
    c.rect(x, y, w, h, fill=PAL[kind][2], stroke=PAL[kind][1], sw=1.2, rx=6)
    c.text(x + 8, y + 16, off, size=10, color=MUTED, mono=True)
    c.text(x + 40, y + 16, title, size=11.5, weight=700, color=PAL[kind][0])
    yy = y + 31
    for l in lines:
        for w_ in wrap(l, w - 18, 10.5):
            c.text(x + 8, yy, w_, size=10.5, color=INK2)
            yy += 12.5

A = (0, 30, 252, 40)
B = (0, 100, 252, 40)
C = (0, 170, 252, 66)
D = (0, 266, 252, 92)
E = (290, 100, 240, 118)
F = (560, 130, 200, 100)
node(*A, "navy", "719", "if mode == 1", [])
node(*B, "navy", "726", "if dir == 1  (facing east)", [])
node(*C, "teal", "733", "if level flag 31 is clear", ["OVERLAY 10 (the find-word panel), then set flag 31: the first descent only"])
node(*D, "red", "746", "going down", ["TEXT 'going down...', SOUND 79,", "LOAD LEVEL 2, cell 755, facing east,", "JUMP 858 (the script stops in LOAD LEVEL)"])
node(*E, "ochre", "776", "the wrong way", ["TEXT 'you can't go that way.', SOUND 29, then four tests 1 == 0, 1 == 1, 1 == 2, 1 == 3, each with a MOVE of the party to a neighbour: only the second is true, so the party goes to 753, west"])
node(560, 130, 200, 100, "plum", "858", "if mode == 4", ["an item dropped here: the same four tests, so the item moves to 753"])
END = (560, 270, 200, 40)
c.rect(*END[:4], fill=PAL["grey"][2], stroke=PAL["grey"][1], sw=1.2, rx=6)
c.text(END[0] + 8, END[1] + 16, "917", size=10, color=MUTED, mono=True)
c.text(END[0] + 40, END[1] + 16, "END", size=11.5, weight=700, color=PAL["grey"][0])
# arrows
c.arrow([(126, 71), (126, 99)], color=GREY, label="yes", lpos=0.5, ldx=14, ldy=3)
c.arrow([(126, 141), (126, 169)], color=GREY, label="yes", ldx=14, ldy=3)
c.arrow([(126, 237), (126, 265)], color=GREY, label="yes", ldx=14, ldy=3)
c.arrow([(253, 120), (289, 120)], color=GREY, label="no", ldx=0, ldy=-5)
c.arrow([(253, 50), (660, 50), (660, 129)], color=GREY, label="no", lpos=0.2, ldy=-5)
c.arrow([(531, 175), (559, 175)], color=GREY)
c.arrow([(660, 231), (660, 269)], color=GREY)
c.arrow([(253, 312), (559, 296)], color=GREY, curve=False)
c.arrow([(126, 359), (126, 359)], color=GREY) if False else None
c.text(0, 384, "Entering facing east goes down; entering any other way is bounced back out. 432 tests in the 12 level files have the 'literal == literal' shape: 108 groups", size=10.5, color=MUTED)
c.text(0, 398, "of four in 54 scripts, each followed by a MOVE. The first operand is a constant where the design seems to call for the party's facing.", size=10.5, color=MUTED)
c.save("figs/wld_stairs.svg")

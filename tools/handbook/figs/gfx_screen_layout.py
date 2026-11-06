import sys; sys.path.insert(0, '.')
from figlib import *

S = 1.55
OX, OY = 2, 22
c = Canvas(760, int(OY + 200 * S + 52))
c.section(0, 11, "THE GAME SCREEN, 320 x 200 (PAGE 0), WITH THE RECTANGLES THE CODE USES")

def R(x, y, w, h, fill="none", stroke=GREY, sw=1, dash=None, rx=0, op=None):
    c.rect(OX + x * S, OY + y * S, w * S, h * S, fill=fill, stroke=stroke, sw=sw, dash=dash, rx=rx, opacity=op)
def T(x, y, s, size=9.5, color=INK2, anchor="start", weight=400):
    c.text(OX + x * S, OY + y * S, s, size=size, color=color, anchor=anchor, weight=weight)

R(0, 0, 320, 200, fill="#F1EDE2", stroke=INK, sw=1.4)
# view
R(0, 0, 176, 120, fill=PAL["teal"][2], stroke=PAL["teal"][1], sw=1.4)
T(88, 52, "3D view", 13, PAL["teal"][0], "middle", 700)
T(88, 66, "region 5: 0, 0, 176 x 120", 10, INK2, "middle")
T(88, 79, "composed on page 2, copied", 9.5, INK2, "middle")
T(88, 90, "as 22 columns x 120 lines", 9.5, INK2, "middle")
# arrows
for r_ in range(2):
    for q in range(3):
        R(3 + 21 * q + 0 * q + q * 0, 128 + 16 * r_, 21, 16, fill=PAL["navy"][2], stroke=PAL["navy"][1], sw=1)
# fix the x spacing: buttons at x = 3, 24, 45
T(34, 166, "six move buttons", 9.5, PAL["navy"][0], "middle", 700)
T(34, 176, "21 x 16 at x 3, 24, 45", 9, INK2, "middle")
# compass
R(70, 121, 106, 54, fill=PAL["ochre"][2], stroke=PAL["ochre"][1], sw=1, dash="3 2")
T(123, 143, "compass", 11, PAL["ochre"][0], "middle", 700)
T(123, 154, "3 shapes, redrawn", 9, INK2, "middle")
T(123, 164, "when the facing changes", 9, INK2, "middle")
# message
R(8, 179, 272, 18, fill=PAL["grey"][2], stroke=PAL["grey"][1], sw=1.2)
T(144, 191, "message window: text region 7, 8, 179, 272 x 18", 10, INK2, "middle")
# camp
R(289, 177, 31, 21, fill=PAL["red"][2], stroke=PAL["red"][1], sw=1)
T(304.5, 190, "CAMP", 9, PAL["red"][0], "middle", 700)
# party blocks
xs = [184, 256]; ys = [2, 54, 106]
n = 0
for ry_ in ys:
    for rx_ in xs:
        R(rx_, ry_, 64, 50, fill=PAL["plum"][2], stroke=PAL["plum"][1], sw=1.1)
        if n == 0:
            R(rx_, ry_ + 9, 32, 32, fill=CARD, stroke=PAL["plum"][1], sw=0.9)
            T(rx_ + 16, ry_ + 28, "portrait", 8.5, INK2, "middle")
            R(rx_ + 32, ry_ + 9, 31, 16, fill=CARD, stroke=PAL["plum"][1], sw=0.9)
            R(rx_ + 32, ry_ + 25, 31, 16, fill=CARD, stroke=PAL["plum"][1], sw=0.9)
            T(rx_ + 47, ry_ + 20, "hand 0", 8, INK2, "middle")
            T(rx_ + 47, ry_ + 36, "hand 1", 8, INK2, "middle")
            T(rx_ + 2, ry_ + 8, "name", 8.5, INK2)
            R(rx_ + 15, ry_ + 44, 38, 3, fill=SOLID["teal"], stroke="none")
        else:
            T(rx_ + 32, ry_ + 29, "member %d" % n, 9.5, PAL["plum"][0], "middle", 600)
        n += 1
R(176, 0, 144, 168, fill="none", stroke=SOLID["red"], sw=1.3, dash="5 3")
T(0, 212, "plum: the six party blocks, each 64 x 50, at x 184 and 256, y 2, 54 and 106", 10, PAL["plum"][0], "start", 700)
T(0, 222, "red dashes: the area (176, 0, 144 x 168) the inventory and character panels redraw", 10, SOLID["red"], "start", 700)

# explanation column
TX = OX + 320 * S + 18
yy = OY + 4
c.section(TX, yy, "HOW TO READ IT", color="#14504A"); yy += 18
for t in [
    "Every rectangle is a number in the code: the view and the text windows are rows of the region table (ds:009F), the hit rectangles are screen nodes (the 38 records of gTable1b), the member blocks come from tables at ds:5D1F to 5D4F.",
    "x is in pixels here. The region table and several routines count x in 8-pixel columns: the view is 22 columns wide, a member block 8.",
    "A member block is built on page 2 at x 8 or 80 and copied to page 0 at x + 176 (the drawing routines add 0xB0 while gDrawPage is 0).",
]:
    for l in wrap(t, 200, 10.5):
        c.text(TX, yy, l, size=10.5, color=INK2); yy += 13.2
    yy += 5
c.save("figs/gfx_screen_layout.svg")

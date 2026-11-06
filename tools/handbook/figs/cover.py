import sys; sys.path.insert(0, '.')
from figlib import *
import random

random.seed(7)
c = Canvas(760, 470)
# a corridor seen from the party: nested depth slots, left and right walls, floor and ceiling
cx, cy = 380, 205
depths = [(380, 330), (290, 244), (212, 174), (152, 122), (106, 84), (72, 56)]   # half width, half height by depth
cols = ["#DCEBE7", "#CFE3DE", "#C0D9D2", "#B2CEC6", "#A5C4BC", "#9ABAB1"]
# back wall
hw, hh = depths[-1]
c.rect(cx - hw, cy - hh, 2 * hw, 2 * hh, fill="#8FB0A7", stroke=SOLID["teal"], sw=1.2, rx=0)
for i in range(len(depths) - 1):
    (w0, h0), (w1, h1) = depths[i], depths[i + 1]
    # left wall
    c.path(f"M{cx-w0},{cy-h0} L{cx-w1},{cy-h1} L{cx-w1},{cy+h1} L{cx-w0},{cy+h0} Z", fill=cols[i], stroke=SOLID["teal"], sw=1.1)
    c.path(f"M{cx+w0},{cy-h0} L{cx+w1},{cy-h1} L{cx+w1},{cy+h1} L{cx+w0},{cy+h0} Z", fill=cols[i], stroke=SOLID["teal"], sw=1.1)
    # ceiling and floor
    c.path(f"M{cx-w0},{cy-h0} L{cx-w1},{cy-h1} L{cx+w1},{cy-h1} L{cx+w0},{cy-h0} Z", fill="#E8F1EE" if i % 2 == 0 else "#DCE9E5", stroke=SOLID["teal"], sw=1.1)
    c.path(f"M{cx-w0},{cy+h0} L{cx-w1},{cy+h1} L{cx+w1},{cy+h1} L{cx+w0},{cy+h0} Z", fill="#D3E4DF" if i % 2 == 0 else "#C8DCD6", stroke=SOLID["teal"], sw=1.1)
# a side opening in the left wall at depth 2 (a door frame)
w0, h0 = depths[2]; w1, h1 = depths[3]
c.path(f"M{cx-w0},{cy+h0} L{cx-w0},{cy-h0*0.55} L{cx-w1},{cy-h1*0.55} L{cx-w1},{cy+h1} Z", fill="#8A7B5A", stroke=PAL["ochre"][1], sw=1.2)
# the eye of a beholder on the back wall
ex, ey = cx, cy - 2
c.path(f"M{ex-26},{ey} Q{ex},{ey-22} {ex+26},{ey} Q{ex},{ey+22} {ex-26},{ey} Z", fill="#FFFDF8", stroke=PAL["plum"][1], sw=1.6)
c.circle(ex, ey, 9, fill=SOLID["plum"])
c.circle(ex, ey, 3.6, fill="#2B2A28")
c.circle(ex + 2.5, ey - 3, 1.6, fill="#FFFFFF")
for k in range(-2, 3):
    c.line(ex + k * 11, ey - 20 + abs(k) * 2, ex + k * 15, ey - 36 + abs(k) * 3, stroke=SOLID["plum"], sw=1.4)
    c.circle(ex + k * 15, ey - 37 + abs(k) * 3, 3, fill=PAL["plum"][2], stroke=SOLID["plum"], sw=1.2)
# bytes streaming on both sides, as the recovered source
def stream(x0, x1, ys, align):
    for y in ys:
        s = " ".join("%02X" % random.randrange(256) for _ in range(random.choice((6, 7, 8))))
        c.text(x0 if align == "start" else x1, y, s, size=10.5, mono=True, color="#B9B1A0", anchor=align)
stream(0, 0, [40, 70, 100, 130, 160, 190, 220, 250, 280, 310, 340, 370], "start")
stream(0, 760, [40, 70, 100, 130, 160, 190, 220, 250, 280, 310, 340, 370], "end")
# the strip beneath: three programs
y = 428
for i, (n, k) in enumerate((("START1.EXE", "ochre"), ("INTRO.EXE", "navy"), ("EOB.EXE", "teal"))):
    x = 190 + i * 140
    tc, sc, fc = PAL[k]
    c.rect(x, y, 128, 24, fill=fc, stroke=sc, sw=1.2, rx=12)
    c.text(x + 64, y + 16, n, size=11.5, weight=700, color=tc, anchor="middle", mono=True)
    if i < 2:
        c.arrow([(x + 130, y + 12), (x + 138, y + 12)], color=GREY, sw=1.2)
c.save("figs/cover.svg")

import sys; sys.path.insert(0, '.')
from figlib import *

c = Canvas(760, 318)
c.section(0, 11, "What make match does for one unit")
def doc(x, y, w, h, kind, title, body):
    tc, sc, fc = PAL[kind]
    f = 10
    c.path(f"M{x},{y} L{x+w-f},{y} L{x+w},{y+f} L{x+w},{y+h} L{x},{y+h} Z", fill=fc, stroke=sc, sw=1.2)
    c.path(f"M{x+w-f},{y} L{x+w-f},{y+f} L{x+w},{y+f}", fill="none", stroke=sc, sw=1.2)
    c.text(x + w / 2, y + 22, title, size=12.5, weight=700, color=tc, anchor="middle", mono=True)
    c.lines(x + w / 2, y + 40, body, size=10.5, lh=13, color=INK2, anchor="middle")
doc(0, 26, 130, 66, "teal", "src/E1B82.C", ["our unit"])
c.box(170, 26, 150, 28, "navy", "link at base A", [], tsize=11.5)
c.box(170, 64, 150, 28, "navy", "link at base B", [], tsize=11.5)
c.text(245, 106, "B: data +111h, code +113h", size=10.5, color=MUTED, anchor="middle")
c.arrow([(132, 50), (168, 40)], color=GREY); c.arrow([(132, 68), (168, 78)], color=GREY)
doc(0, 124, 130, 66, "grey", "EOB.EXE", ["the original", "(unit's bytes)"])
c.arrow([(132, 157), (350, 157), (350, 130)], color=GREY)
c.panel(350, 26, 410, 164, fill=PANEL)
c.section(362, 46, "The comparison, byte by byte", color=PAL["ochre"][0], size=10)
rules = [
    ("bytes that did not move between A and B", "must equal the original's", "teal"),
    ("fields that moved", "must name the same thing as the original's field:", "navy"),
    ("   a global, a string, a function, a segment", "(by annotation, or by the runtime library's module)", "navy"),
    ("jump and call distances", "must equal the original's, or the same offset in the function", "plum"),
]
yy = 66
for a, b, k in rules:
    c.rect(362, yy - 11, 5, 26, fill=SOLID[k], stroke="none", rx=1)
    c.text(374, yy, a, size=11, weight=600, color=INK2)
    c.text(374, yy + 14, b, size=10.5, color=MUTED)
    yy += 34
c.arrow([(322, 40), (348, 40)], color=GREY); c.arrow([(322, 78), (348, 78)], color=GREY)
# verdict row
c.rect(0, 212, 760, 92, fill=PAL["teal"][2], stroke=PAL["teal"][1], rx=6)
c.text(14, 234, "L2 matched", size=13, weight=700, color=PAL["teal"][0])
y = c.para(14, 252, "recorded in re/symbols.toml only by make match WRITE=1; make match-gate proves every recorded function again and rejects 11 negative fixtures", 690, size=11, color=INK2)
y = c.para(14, y + 3, "each negative fixture is a unit changed to be wrong (a bound 52 to 53, the wrong table, an extra store) and must fail, so a pass means something", 690, size=11, color=INK2)
c.save("figs/prog_matcher.svg")

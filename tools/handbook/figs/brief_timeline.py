import sys; sys.path.insert(0, '.')
from figlib import *

c = Canvas(760, 330)
c.section(0, 11, "31 October to 5 November 2026: phases and commits a day")
days = ["31 Oct", "1 Nov", "2 Nov", "3 Nov", "4 Nov", "5 Nov"]
X0, DW = 150, 101
for i, d in enumerate(days):
    x = X0 + i * DW
    c.line(x, 24, x, 238, stroke=RULE)
    c.text(x + DW / 2, 38, d, size=11, color=MUTED, anchor="middle", weight=600)
c.line(X0 + 6 * DW, 24, X0 + 6 * DW, 238, stroke=RULE)
phases = [  # name, start day float, end day float, kind
    ("P0 bootstrap", 0.0, 0.9, "grey"),
    ("P1 map, toolchain", 0.2, 1.3, "grey"),
    ("P2 oracle, data", 0.6, 1.6, "ochre"),
    ("P3 match harness", 0.8, 1.6, "ochre"),
    ("P4 match every function", 1.0, 4.0, "teal"),
    ("P5 rebuild", 3.0, 5.0, "navy"),
    ("P6 consistency", 3.0, 6.0, "plum"),
    ("P7 portable build", 4.0, 6.0, "red"),
]
for i, (n, a, b, k) in enumerate(phases):
    y = 50 + i * 23
    c.text(X0 - 8, y + 12, n, size=11, color=INK2, anchor="end")
    c.rect(X0 + a * DW, y, (b - a) * DW, 17, fill=SOLID[k], stroke="none", rx=4, opacity=0.9)
# commits per day histogram
counts = [5, 1314, 1054, 27, 227, 0]
c.section(0, 270, "commits a day on main", color=PAL["grey"][0], size=10)
mx = 1314
for i, v in enumerate(counts):
    x = X0 + i * DW + 18
    h = 44 * v / mx
    c.rect(x, 324 - h, DW - 36, max(h, 1), fill=SOLID["teal"], stroke="none", rx=2)
    c.text(x + (DW - 36) / 2, 320 - h - 0, str(v) if v else "", size=10.5, color=MUTED, anchor="middle")
c.text(X0 - 8, 318, "(5 Nov: day in progress)", size=10, color=MUTED, anchor="end")
c.save("figs/brief_timeline.svg")

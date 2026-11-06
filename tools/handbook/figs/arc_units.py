import sys; sys.path.insert(0, '.')
from figlib import *

groups = [
    ("overlays (VROOMM)", 50641, 7, 136, "eov0 to eov6", "navy"),
    ("world and monsters", 30836, 4, 199, "e08e6 e122e e1414 e0ef3", "red"),
    ("sound drivers", 21789, 3, 53, "e_text e1948 e177d", "ochre"),
    ("party and items", 18045, 5, 91, "e0a72 e0bd2 e0d1c e0de8 e1547", "red"),
    ("memory, files, loading", 12399, 10, 95, "e19bc e19ec e1990 e1c0e e1b82 e1d9a e1ba8 e19fc e1c00 e1e8b", "teal"),
    ("start-up and C library", 11390, 2, 130, "e_c0 e_lib", "grey"),
    ("graphics and text", 10675, 9, 28, "e17a6 e1a1f e1a29 e1a47 e1de7 e19f3 e1aa6 e1ba4 e19ea", "plum"),
    ("UI toolkit", 9672, 5, 57, "e082c e14ef e1658 e16a5 e0cf7", "navy"),
    ("main loop", 6279, 3, 42, "e07a9 e07c8 e0acd", "navy"),
    ("input and pointer", 5510, 5, 40, "e1aad e1a72 e1bb1 e1bbe e1bf3", "teal"),
    ("clocks, video mode, sync", 591, 5, 10, "e1d8f e1a65 e1a16 e1de4 e1ddd", "teal"),
    ("small helpers", 249, 4, 6, "e1a63 e1a19 e1d8c e1aaa", "grey"),
]
RH = 27
c = Canvas(760, 30 + len(groups) * RH + 28)
c.section(0, 11, "EOB.EXE code by role: 62 units, 178,076 bytes, 885 functions")
LX, BX0, BW = 0, 190, 390
mx = 50641
for i, (name, b, n, f, ids, k) in enumerate(groups):
    y = 26 + i * RH
    c.text(LX, y + 17, name, size=12, weight=700, color=PAL[k][0])
    w = max(3, BW * b / mx)
    c.rect(BX0, y + 5, w, 17, fill=SOLID[k], rx=3)
    c.text(BX0 + w + 6, y + 18, "%s bytes" % format(b, ","), size=11, color=INK2)
    c.text(758, y + 18, "%d units, %d fn" % (n, f), size=10.5, color=MUTED, anchor="end")
c.text(0, 26 + len(groups) * RH + 10, "Bytes are code only (re/units.toml). Overlay code is not in the load image (section 2.3).", size=10.5, color=MUTED)
c.save("figs/arc_units.svg")

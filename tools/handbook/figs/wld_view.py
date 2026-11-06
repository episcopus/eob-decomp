import sys; sys.path.insert(0, '.')
from figlib import *

c = Canvas(760, 340)
c.section(0, 11, "THE 18 CELLS OF THE VIEW, FACING NORTH (ovl4:0000, offsets from ds:199D)")
S = 46
G = 5
cx = 215
rows = [  # (y index from top, slots, offsets)
    (list(range(0, 7)), [-99, -98, -97, -96, -95, -94, -93]),
    (list(range(7, 12)), [-66, -65, -64, -63, -62]),
    (list(range(12, 15)), [-33, -32, -31]),
    (list(range(15, 18)), [-1, 0, 1]),
]
TC, SC, FC = PAL["teal"]
NC, NS, NF = PAL["navy"]
for r, (slots, offs) in enumerate(rows):
    n = len(slots)
    w = n * S + (n - 1) * G
    x0 = cx - w / 2
    y = 30 + r * (S + 12)
    for i, (sl, o) in enumerate(zip(slots, offs)):
        x = x0 + i * (S + G)
        party = (sl == 16)
        own = sl >= 15
        c.rect(x, y, S, S, fill=(FC if party else (PANEL if own else NF)), stroke=(SC if party else (GREY if own else NS)), sw=1.2, rx=3,
               dash=("3 2" if own and not party else None))
        c.text(x + S / 2, y + 21, str(sl), size=14, weight=700, color=(TC if party else (MUTED if own else NC)), anchor="middle")
        c.text(x + S / 2, y + 38, ("%+d" % o) if o else "0", size=10.5, color=MUTED, anchor="middle", mono=True)
# party arrow
px = cx
c.arrow([(px, 30 + 3 * (S + 12) + S + 14), (px, 30 + 3 * (S + 12) + S + 2)], color=SOLID["teal"], sw=2)
c.text(px, 30 + 3 * (S + 12) + S + 28, "the party, facing north (slot 16)", size=11, color=TC, anchor="middle", weight=600)
# depth labels
for r, lab in enumerate(["3 ahead", "2 ahead", "1 ahead", "beside"]):
    n = len(rows[r][0])
    c.text(cx + (n * S + (n - 1) * G) / 2 + 10, 30 + r * (S + 12) + S / 2 + 4, lab, size=10.5, color=MUTED)

# right text
RX = 470
c.section(RX, 22, "HOW TO READ IT")
paras = [
    "Slot k holds the cell number gViewCellIds[k] = (party cell + offset) & 0x3FF and a pointer to that cell's nine bytes (gViewCells[k]).",
    "ds:199D has one row of 18 offsets per facing. For east the row is -93, -61, -29, +3, +35, +67, +99, then -62, -30, +2, +34, +66, then -31, +1, +33, then -32, 0, +32: the picture turned a quarter turn.",
    "IsCellInView (0D1C:0006) is how monsters, doors and items ask 'is this cell on screen?': it searches the first 15 slots of ds:A9C8 for the cell and, on a hit, sets gViewDirty so the next pass redraws. The three dashed slots are not searched (it loops 15 times, the table has 18 words).",
]
yy = 44
for p in paras:
    for l in wrap(p, 280, 11):
        c.text(RX, yy, l, size=11, color=INK2)
        yy += 14.2
    yy += 6
c.save("figs/wld_view.svg")

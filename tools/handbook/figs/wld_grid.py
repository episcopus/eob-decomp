import sys; sys.path.insert(0, '.')
from figlib import *
import math

c = Canvas(760, 420)
c.section(0, 11, "THE GRID: CELL NUMBERS, DIRECTIONS, SIDES AND SUB-POSITIONS")

# ---- 3 x 3 block of cells ----
S = 100
GX, GY = 54, 40
TC, SC, FC = PAL["teal"]
names = [["c-33", "c-32", "c-31"], ["c-1", "c", "c+1"], ["c+31", "c+32", "c+33"]]
for r in range(3):
    for k in range(3):
        x, y = GX + k * S, GY + r * S
        center = (r == 1 and k == 1)
        c.rect(x, y, S, S, fill=(FC if center else CARD), stroke=(SC if center else GREY), sw=(1.6 if center else 1), rx=0)
        if not center:
            c.text(x + 7, y + 17, names[r][k], size=11.5, color=MUTED, mono=True)
cx, cy = GX + S, GY + S
q = S / 2
for i, (qx, qy) in enumerate([(0, 0), (1, 0), (0, 1), (1, 1)]):
    c.rect(cx + qx * q + 6, cy + qy * q + 6, q - 12, q - 12, fill=CARD, stroke=SOLID["teal"], sw=1, rx=3)
    c.text(cx + qx * q + q / 2, cy + qy * q + q / 2 + 5, str(i), size=14, weight=700, color=TC, anchor="middle")
c.circle(cx + q, cy + q, 9, fill=PAL["ochre"][2], stroke=PAL["ochre"][1], sw=1.2)
c.text(cx + q, cy + q + 4, "4", size=11.5, weight=700, color=PAL["ochre"][0], anchor="middle")
c.text(cx + S / 2, GY + S - 6, "side 0: north", size=10.5, weight=700, color=TC, anchor="middle")
c.text(cx + S / 2, cy + S + 40, "side 2: south", size=10.5, weight=700, color=TC, anchor="middle")
c.text(cx + S + 6, cy + S / 2 + 4, "side 1: east", size=10.5, weight=700, color=TC)
c.text(cx - 6, cy + S / 2 + 4, "side 3: west", size=10.5, weight=700, color=TC, anchor="end")
c.text(GX + 1.5 * S, GY + 3 * S + 30, "cell c = y * 32 + x   (x = c & 31, y = c >> 5)", size=11.5, color=INK2, anchor="middle", mono=True)
c.text(GX + 1.5 * S, GY + 3 * S + 48, "positions 0..3 are the quadrants NW, NE, SW, SE; 4 is the centre", size=10.5, color=MUTED, anchor="middle")

# ---- right: direction table ----
RX = 392
c.section(RX, 22, "STEPPING (StepCell, StepIfOpen)")
hdr = ["dir", "facing", "offset", "wall the step checks"]
colx = [RX, RX + 38, RX + 100, RX + 160]
yy = 44
for h, x in zip(hdr, colx):
    c.text(x, yy, h, size=10.5, color=MUTED, weight=600)
c.line(RX, yy + 6, 760, yy + 6, stroke=RULE)
rows = [("0", "north", "-32", "side 2 of the target cell"),
        ("1", "east", "+1", "side 3 of the target cell"),
        ("2", "south", "+32", "side 0 of the target cell"),
        ("3", "west", "-1", "side 1 of the target cell")]
yy += 24
for r in rows:
    for j, (t, x) in enumerate(zip(r, colx)):
        c.text(x, yy, t, size=12, color=INK2, mono=(j in (0, 2)))
    yy += 21
yy += 4
for l in wrap("The step lands on (c + offset) & 0x3FF, so north and south wrap between row 0 and row 31 and a step east from x = 31 enters x = 0 of the next row. The target's wall on the side facing back needs flag bit 0, and the cell must hold no monster.", 360, 11):
    c.text(RX, yy, l, size=11, color=INK2)
    yy += 14.5
yy += 6
c.line(RX, yy, 760, yy, stroke=RULE)
yy += 20
c.section(RX, yy, "EIGHT-WAY CODE OF DirectionFromPoints")
ccx, ccy = RX + 62, yy + 72
for d, lab in enumerate(["0 N", "1 NE", "2 E", "3 SE", "4 S", "5 SW", "6 W", "7 NW"]):
    a = math.radians(d * 45 - 90)
    x = ccx + 52 * math.cos(a); y = ccy + 52 * math.sin(a)
    c.circle(x, y, 17, fill=(PAL["navy"][2] if d % 2 == 0 else CARD), stroke=PAL["navy"][1], sw=1.1)
    c.text(x, y + 4, lab, size=10.5, weight=600, color=PAL["navy"][0], anchor="middle")
c.circle(ccx, ccy, 4, fill=SOLID["navy"])
tx = RX + 150
y2 = yy + 22
for l in wrap("The helper (17A6:0314) compares twice one coordinate difference with the other. A cardinal code owns a 2:1 slope band, a diagonal lies between. The same cell gives 255. A monster facing orient (0 to 3) looks along code orient * 2.", 190, 11):
    c.text(tx, y2, l, size=11, color=INK2)
    y2 += 14.5
c.save("figs/wld_grid.svg")

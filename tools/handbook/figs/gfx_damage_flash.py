import sys; sys.path.insert(0, '.')
from figlib import *

X2 = [80, 112, 48, 104, 32, 96, 56, 120, 128, 72, 88, 40, 96, 64, 112, 72]
Y2 = [49, 43, 72, 23, 22, 72, 53, 27, 67, 46, 36, 40, 56, 28, 22, 68]
SH = [30, 28, 26, 24, 22, 20, 18, 16, 14, 12, 10, 8, 6, 4, 2, 0]
PASS = [0x40000000, 0x95000000, 0xea550000, 0xbfaa5400, 0x6affa954, 0x15aafea9, 0x55abfe, 0x56ab, 0x156, 0x1, 0x0]
FR = [0x3 << s for s in SH]

c = Canvas(760, 330)
c.section(0, 11, "FlashViewDamage (0EF3:06E7): ELEVEN PASSES OVER SIXTEEN 16 x 16 CELLS OF THE VIEW")

# left: positions, scale 1.35 of the 176 x 120 view
S = 1.5
ox, oy = 4, 36
c.rect(ox, oy, 176 * S, 120 * S, fill="#E9E4D6", stroke=GREY, sw=1, rx=0)
for i in range(16):
    x = ox + X2[i] * S; y = oy + Y2[i] * S
    c.rect(x, y, 16 * S, 16 * S, fill=PAL["red"][2], stroke=PAL["red"][1], sw=1, rx=0, opacity=0.8)
    c.text(x + 8 * S, y + 8 * S + 4, str(i), size=10, weight=700, color=PAL["red"][0], anchor="middle")
yl = oy + 120 * S + 16
for l in wrap("The view, 176 x 120. Each cell is 16 x 16 pixels (2 columns by 16 lines) at an (x, y) from two byte tables, ds:56C7 and ds:56D7.", 262, 10.5):
    c.text(ox, yl, l, size=10.5, color=MUTED); yl += 13

# right: matrix
mx0, my0 = 340, 52
cw, ch = 25.5, 17
c.text(mx0, my0 - 22, "frame shown by cell (columns) in each pass (rows)", size=11.5, weight=700, color=INK)
cols = {0: "#ECE7DD", 1: PAL["ochre"][2], 2: MID["red"], 3: SOLID["red"]}
for p in range(11):
    c.text(mx0 - 6, my0 + p * ch + 12, "pass %d" % p, size=9.5, color=MUTED, anchor="end")
    for i in range(16):
        v = (PASS[p] & FR[i]) >> SH[i]
        c.rect(mx0 + i * cw * 0.98, my0 + p * ch, cw * 0.98, ch - 1, fill=cols[v], stroke="#FFFDF8", sw=0.5, rx=0)
        if v:
            c.text(mx0 + i * cw * 0.98 + cw * 0.49, my0 + p * ch + 12, str(v), size=9.5, weight=700, color="#FFFFFF" if v == 3 else INK2, anchor="middle")
for i in range(16):
    c.text(mx0 + i * cw * 0.98 + cw * 0.49, my0 - 4, str(i), size=9, color=MUTED, anchor="middle")
yy = my0 + 11 * ch + 20
for t in [
    "At the start the view on screen is copied to page 2 and the 16 cells are saved from page 0. Each pass: restore the cells on page 2, draw those whose 2-bit code is not 0 with shape gFlashFrames[code - 1] on page 2, copy the view to the screen, wait 2 ticks. At the end the cells are restored on the screen.",
    "The codes are one 32-bit word a pass (ds:564B), 2 bits a cell: the three explosion frames sweep from cell 0 to cell 15 and die out. Pass 10 draws nothing; a last restore leaves the view as it was.",
]:
    for l in wrap(t, 410, 10.5):
        c.text(mx0 - 4, yy, l, size=10.5, color=INK2); yy += 13.2
    yy += 4
c.h = max(oy + 120 * S + 60, yy + 4)
c.save("figs/gfx_damage_flash.svg")

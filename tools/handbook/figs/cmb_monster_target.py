import sys; sys.path.insert(0, '.')
from figlib import *

c = Canvas(760, 330)
c.section(0, 11, "WHOM A MONSTER HITS: ds:325B, FOUR SIDES, TWO HALVES EACH (party facing up the page)")

# gAttackTargetOrder (ds:325B): group -> (half 0, half 1)
orders = {
    0: ([1, 0, 3, 2, 5, 4], [0, 1, 2, 3, 4, 5]),
    1: ([5, 3, 1, 4, 2, 0], [1, 3, 5, 0, 2, 4]),
    2: ([4, 5, 2, 3, 0, 1], [5, 4, 3, 2, 1, 0]),
    3: ([0, 2, 4, 1, 3, 5], [4, 2, 0, 5, 3, 1]),
}
titles = {0: "from the front", 1: "from the right", 2: "from behind", 3: "from the left"}
which = {  # what picks half 0 / half 1 (gRightSidePosByDir, ds:328B)
    0: ("monster in a right quarter", "monster in a left quarter"),
    1: ("monster in a rear quarter", "monster in a front quarter"),
    2: ("monster in a left quarter", "monster in a right quarter"),
    3: ("monster in a front quarter", "monster in a rear quarter"),
}
arrow_dir = {0: (0, 1), 1: (-1, 0), 2: (0, -1), 3: (1, 0)}  # direction the blow travels
S = 24
def formation(x, y, order):
    rank_of = {m: m // 2 for m in range(6)}
    for k, m in enumerate(order):
        col, row = m % 2, m // 2
        n = k + 1
        shade = [MID["red"], "#EFD3CB", "#F5E0D9", "#FAF0EC", "#FAF0EC", "#FAF0EC"][k]
        c.rect(x + col * (S + 3), y + row * (S + 3), S, S, fill=shade, stroke=SOLID["red"], sw=1, rx=3)
        c.text(x + col * (S + 3) + S / 2, y + row * (S + 3) + S / 2 + 4.5, str(n), size=13, weight=700, color=PAL["red"][0], anchor="middle")
        c.text(x + col * (S + 3) + S - 3, y + row * (S + 3) + 9, str(m), size=8.5, color=MUTED, anchor="end")

PW = 188
for g in range(4):
    px = g * (PW + 2)
    c.rect(px, 24, PW - 4, 188, fill=PANEL, stroke="none", rx=8)
    c.text(px + 10, 44, titles[g], size=12.5, weight=700, color=PAL["red"][0])
    for h in range(2):
        fx = px + 14 + h * 88
        formation(fx, 78, orders[g][h])
        for i, ln in enumerate(wrap(which[g][h], 80, 10)):
            c.text(fx, 78 + 3 * (S + 3) + 14 + i * 12, ln, size=10, color=INK2)
    # direction of the blow (arrow outside the formation)
    ax, ay = px + PW / 2 - 2, 58
    dx, dy = arrow_dir[g]
    c.text(px + 10, 60, {0: "blow comes down", 1: "blow comes from the right", 2: "blow comes up", 3: "blow comes from the left"}[g], size=10, color=MUTED)
yy = c.para(0, 232, "Small numbers: member index (0 and 1 front rank, 2 and 3 middle, 4 and 5 rear). Large numbers: the order searched. The first member that is present, above -10 hp and, if invisible, passes a 1d20 < 5 test is hit; if none qualifies the monster's attack does nothing.", 756, size=10.5, color=MUTED)
c.h = yy + 4
c.save("figs/cmb_monster_target.svg")

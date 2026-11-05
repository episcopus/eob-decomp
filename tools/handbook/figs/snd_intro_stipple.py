import sys; sys.path.insert(0, '.')
from figlib import *

c = Canvas(760, 352)
c.section(0, 11, "BlitImage: three stipple masks of the pixels kept (dark), 12 by 6 pixels shown")
cell = 11
def grid(x, y, keep):
    for r in range(6):
        for k in range(12):
            on = keep(r, k)
            c.rect(x + k * cell, y + r * cell, cell, cell, fill=SOLID["teal"] if on else "#FFFFFF", stroke=RULE, sw=0.7, rx=0)
modes = [
    ("mode 1", "a checkerboard: 1 pixel in 2", lambda r, k: (k % 2) == (r % 2)),
    ("mode 2", "even columns of every other row: 1 in 4", lambda r, k: r % 2 == 0 and k % 2 == 0),
    ("mode 3", "every third pixel of every third row: 1 in 9", lambda r, k: r % 3 == 0 and k % 3 == 0),
]
for i, (t, d, f) in enumerate(modes):
    x = i * 176
    c.text(x, 34, t, size=12, weight=700, color=PAL["teal"][0])
    grid(x, 42, f)
    for k, l in enumerate(wrap(d, 160, 10.5)):
        c.text(x, 122 + 13 * k, l, size=10.5, color=INK2)
c.para(540, 40, "In PlayWtrdpScene the full picture is cut first (sprites[3]); modes 1, 2, 3 then cut sprites[2], [1], [0] from the same buffer. "
       "The scene draws sprites[0] to [3] in turn, 3 ticks apart: a picture that fades in from dots with no palette change.", 200, size=10.5, color=MUTED)

c.section(0, 174, "DrawGrid: 16 lines paint every row and column (colour 12), 8 pixels apart, in two passes", color=PAL["navy"][0])
cw = 44
for s in range(16):
    p, l = divmod(s, 8)
    vertical = l % 2 == 1
    if vertical:
        res = (l + p) % 8
        lab = "x" + str(res)
        kind = "navy"
    else:
        res = (l + p) % 8
        lab = "y" + str(res)
        kind = "ochre"
    tc, sc, fc = PAL[kind]
    x = s * cw
    c.rect(x + 1, 188, cw - 2, 44, fill=fc, stroke=sc, sw=1, rx=3)
    c.text(x + cw / 2, 205, "%d" % s, size=10.5, color=MUTED, anchor="middle")
    c.text(x + cw / 2, 224, lab, size=12, weight=700, color=tc, anchor="middle", mono=True)
c.para(0, 252, "Steps 0 to 15. xN: vertical lines at x = N, N+8, ... (odd line numbers); yN: horizontal lines at y = N, N+8, ... (even line numbers). Each full-width horizontal line alone covers its rows, so the eight y steps already fill the screen.", 700, size=10.5, color=MUTED)
c.para(0, 292, "After step 15 every pixel is colour 12, the colour the scenes also use to clear areas (gFillRect with 0Ch), so the routine is a wipe to a flat colour that is drawn as a growing grid. "
       "Dissolve is the other wipe: two shuffled permutations (320 columns, 200 rows) and a copy of every pixel from one page to another through GetPixel and PutPixel.",
       700, size=10.5, color=MUTED)
c.save("figs/snd_intro_stipple.svg")

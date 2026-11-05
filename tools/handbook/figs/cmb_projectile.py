import sys; sys.path.insert(0, '.')
from figlib import *

c = Canvas(760, 470)
c.section(0, 11, "A FLYING OBJECT: THE 18-BYTE RECORD (gProjectiles, ds:DD2C, ten of them) AND ITS STEP")

# --- record to scale: 18 bytes ---
BX, BW = 0, 42
fields = [("active", 1, "navy"), ("", 1, "grey"), ("m02", 2, "grey"), ("cell", 2, "teal"), ("m06", 1, "teal"),
          ("dir", 1, "teal"), ("item", 2, "ochre"), ("m0A", 1, "ochre"), ("m0B", 1, "plum"), ("m0C", 1, "red"),
          ("m0D", 1, "red"), ("fn", 4, "navy")]
x, off = BX, 0
offs = []
for name, n, kind in fields:
    tc, sc, fc = PAL[kind]
    c.rect(x, 24, n * BW - 2, 30, fill=fc, stroke=sc, sw=1.2, rx=3)
    if name:
        c.text(x + (n * BW - 2) / 2, 44, name, size=11.5, weight=700, color=tc, anchor="middle", mono=(name != "active"))
    c.text(x + 2, 66, "%02X" % off, size=9.5, color=MUTED, mono=True)
    offs.append(off)
    x += n * BW
    off += n
notes = [
    ("active", "0 free; 1 a flying item; 2 a spell missile", "navy"),
    ("m02", "1 until the object leaves its first cell: no hit test meanwhile", "grey"),
    ("cell, m06, dir", "map cell, quarter in it (low 2 bits), facing", "teal"),
    ("item", "gItems index of the object (spell: the effect mode)", "ochre"),
    ("m0A", "item type of the weapon or launcher", "ochre"),
    ("m0B", "who threw or cast it; -1 for monsters and scripts", "plum"),
    ("m0C, m0D", "flags of the mode; steps left (255: unlimited)", "red"),
    ("fn", "hit callback of the spell, used in state 2", "navy"),
]
ny = 90
for i, (k, t, kind) in enumerate(notes):
    col, row = i % 2, i // 2
    xx = col * 380
    yy = ny + row * 17
    c.rect(xx, yy - 9, 10, 10, fill=PAL[kind][2], stroke=PAL[kind][1], sw=1, rx=2)
    c.text(xx + 16, yy, k, size=11, weight=700, color=PAL[kind][0], mono=True)
    c.text(xx + 16 + text_width(k, 11, True) + 8, yy, t, size=11, color=INK2)

# --- the path: cells and half steps ---
py = 178
c.text(0, py, "One step, every 3 ticks: half a cell", size=12, weight=700, color=INK)
cx0, cs = 20, 110
for i in range(4):
    x = cx0 + i * (cs + 14) * (1 if i < 4 else 1)
    c.rect(x, py + 14, cs, cs * 0.62, fill="#F5E0D9" if i else "#E1E5F2", stroke=GREY, sw=1, rx=0)
    c.line(x + cs / 2, py + 14, x + cs / 2, py + 14 + cs * 0.62, stroke=RULE, dash="2 3")
    c.line(x, py + 14 + cs * 0.31, x + cs, py + 14 + cs * 0.31, stroke=RULE, dash="2 3")
    c.text(x + cs / 2, py + 14 + cs * 0.62 + 13, "party cell" if i == 0 else "cell %d ahead" % i, size=10.5, color=MUTED, anchor="middle")
# a path moving left to right in the lower quarter row
pts = [(cx0 + cs * 0.75, py + 14 + cs * 0.47)]
for i in range(1, 4):
    x = cx0 + i * (cs + 14)
    for fx in (0.25, 0.75):
        pts.append((x + cs * fx, py + 14 + cs * 0.47))
for k, (px, pyy) in enumerate(pts):
    c.circle(px, pyy, 4, fill=SOLID["red"] if k else SOLID["navy"], stroke="none")
    c.text(px, pyy - 8, "start" if k == 0 else str(k), size=9.5, color=PAL["red"][0] if k else PAL["navy"][0], anchor="middle", weight=700)
c.arrow([(pts[0][0] + 6, pts[0][1] + 11), (pts[-1][0] - 6, pts[-1][1] + 11)], color=SOLID["red"], sw=1.2, dash="3 3")
c.para(520, py + 40, "The first step crosses into the next cell; each cell then takes two steps (near half, far half). Flying items get 13 steps (about 6 cells); Lightning Bolt and Ice Storm 7 (about 3); other missiles never run out and end at a wall or a hit.", 240, size=10.5, color=MUTED)

# --- the per-step flow ---
fy = 330
c.text(0, fy, "What MoveDDEffects does to each live record (1414:0285)", size=12, weight=700, color=INK)
steps = [
    ("navy", "next quarter", "ds:5CAB gNextPosByDir; bit 7 set: into the next cell"),
    ("ochre", "StepDDEffect", "wall in the way? item: stops, wall handler runs; missile: gone"),
    ("red", "TestDDEffect", "item: monsters in that quarter or the centre get TryRecordAttack; party cell: RunRecordAttack. Missile: its fn"),
    ("teal", "ClearDDEffect", "on a hit, a stop or the last step: the item stays on that cell's floor"),
]
bw = 176
for i, (kind, t, d) in enumerate(steps):
    x = i * (bw + 18)
    tc, sc, fc = PAL[kind]
    lines = wrap(d, bw - 16, 10.5)
    h = 32 + len(lines) * 13.5 + 6
    c.box(x, fy + 10, bw, 112, kind, t, lines, size=10.5, tsize=12, align="left")
    if i:
        c.arrow([(x - 18, fy + 66), (x, fy + 66)])
c.h = fy + 136
c.save("figs/cmb_projectile.svg")

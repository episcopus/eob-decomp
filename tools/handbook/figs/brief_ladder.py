import sys; sys.path.insert(0, '.')
from figlib import *

c = Canvas(760, 300)
c.section(0, 11, "The proof ladder: each rung is a tool's verdict, not a person's")
steps = [
    ("grey", "L0  Drafted", ["C or assembly in src/ that compiles with the pinned toolchain"], "make match", ""),
    ("teal", "L2  Matched", ["our compiler output equals the original's instructions; every address field names the same thing"], "make match WRITE=1", "1,011 functions"),
    ("navy", "L3  Rebuilt", ["make rebuild links all units into a program whose image, relocations and overlays equal the original's"], "make rebuild", "3 programs, 4 drivers"),
    ("plum", "Played", ["the oracle runs original and rebuild on recorded sessions: same instructions, screens, sound writes, state digests"], "make golden REBUILT=1", "7 sessions"),
]
n = len(steps); gap = 18; w = (760 - gap * (n - 1)) / n
for i, (k, t, body, cmd, res) in enumerate(steps):
    x = i * (w + gap)
    tc, sc, fc = PAL[k]
    c.rect(x, 28, w, 150, fill=fc, stroke=sc, sw=1.2, rx=6)
    c.rect(x, 28, w, 26, fill=SOLID[k], stroke=SOLID[k], sw=1.2, rx=6)
    c.rect(x, 44, w, 10, fill=SOLID[k], stroke="none", rx=0)
    c.text(x + 10, 46, t, size=13.5, weight=700, color="#FFFFFF")
    c.para(x + 10, 76, body[0], w - 20, size=11, lh=15, color=INK2)
    c.text(x + 10, 150, cmd, size=9.5, mono=True, color=tc)
    if res:
        c.pill(x + 10, 168, res, k, size=10.5, solid=True)
    if i < n - 1:
        c.arrow([(x + w + 1, 103), (x + w + gap - 1, 103)], color=GREY)
# negative fixtures
c.panel(0, 200, 760, 86, fill=PANEL)
c.section(14, 220, "What keeps the verdicts honest", color=PAL["ochre"][0])
c.text(14, 242, "Negative fixtures: 11 near-miss units the matcher must reject (a bound 52 changed to 53, the wrong table, an extra store).", size=11.5, color=INK2)
c.text(14, 262, "Pinned inputs: re/inputs.toml holds the SHA-256 of every original file; nothing under eob/ is ever changed.", size=11.5, color=INK2)
c.text(14, 280, "Only tools write levels: an agent cannot mark a function matched; make match WRITE=1 does, and make match-gate re-proves them all.", size=11.5, color=INK2)
c.save("figs/brief_ladder.svg")

import sys; sys.path.insert(0, '.')
from figlib import *

c = Canvas(760, 158)
c.section(0, 11, "The hand-over: the command tail INTRO.EXE builds for EOB.EXE (section 2.6 has the mechanism)")
cells = [
    ("3", "graphics", "argv[1]", "teal"), ("4", "sound", "argv[2]", "teal"), ("0", "mouse", "argv[3]", "teal"), ("homie", "marker", "argv[4]", "teal"),
    ("3", "video mode to restore", "gRestoreModeArg", "ochre"), ("0", "memory top", "gMemTopArg", "ochre"),
]
x = 0
ws = [60, 70, 62, 84, 150, 120]
for (v, name, src, kind), w in zip(cells, ws):
    tc, sc, fc = PAL[kind]
    c.rect(x, 34, w, 38, fill=fc, stroke=sc, sw=1.2, rx=4)
    c.text(x + w / 2, 59, v, size=15, weight=700, color=tc, anchor="middle", mono=True)
    c.text(x + w / 2, 90, name, size=10.5, color=INK2, anchor="middle")
    c.text(x + w / 2, 104, src, size=10, color=MUTED, anchor="middle", mono=True)
    x += w + 10
c.text(0, 134, "teal: what START1.EXE gave INTRO.EXE and INTRO passes on (I0892.C:263); ochre: added by INTRO.EXE. EOB.EXE's Main reads argv[1] to argv[5].", size=10.5, color=MUTED)
c.save("figs/snd_handover.svg")

import sys; sys.path.insert(0, '.')
from figlib import *

c = Canvas(760, 468)
c.box(200, 0, 360, 54, "navy", "Main loop and action handlers", ["RunMapLoop, MoveForward, ClickFloorItem, ...", "(units e0acd, e07a9, e07c8)"], size=10.3, tsize=12)
R1 = 98
c.box(0, R1, 232, 62, "teal", "UI toolkit", ["node lists, dispatcher, option lists,", "field editor (e16a5 e1658 e0cf7 e14ef e082c)"], size=10.3)
c.box(264, R1, 232, 62, "red", "World", ["scripts and movement, monsters,", "timed effects, spells (e122e e08e6 e1414 e0ef3)"], size=10.3)
c.box(528, R1, 232, 62, "red", "Party and items", ["characters, inventory, combat rolls", "(e0a72 e0bd2 e0d1c e0de8 e1547)"], size=10.3)
c.arrow([(280, 55), (150, 96)], color=GREY, label="DispatchKeyWait", ldx=-44, ldy=-2)
c.arrow([(380, 55), (380, 96)], color=GREY, label="RunEffects", ldx=34, ldy=2)
c.arrow([(480, 55), (610, 96)], color=GREY, label="TurnUndead, UseItem", ldx=52, ldy=-4)
c.arrow([(498, 130), (526, 130)], color=GREY)
# overlays
R2 = 222
c.rect(0, R2, 760, 74, fill="#F6EEF4", stroke=PAL["plum"][1], sw=1.2, rx=8)
c.text(10, R2 + 17, "Overlays: seven code blocks, one pool, entered through stubs", size=11.5, weight=700, color=PAL["plum"][0])
ov = [("ovl0", "party creation"), ("ovl2", "data, title menu"), ("ovl1", "camp, spells"), ("ovl6", "resting"),
      ("ovl3", "levels, save, load"), ("ovl4", "first-person view"), ("ovl5", "NPCs, events")]
for i, (n, r) in enumerate(ov):
    x = 10 + i * 106.5
    c.box(x, R2 + 28, 98, 36, "plum", None, None, fill=CARD)
    c.text(x + 49, R2 + 43, n, size=11.5, weight=700, color=PAL["plum"][0], anchor="middle", mono=True)
    c.text(x + 49, R2 + 57, r, size=9.5, color=INK2, anchor="middle")
# thick-ish arrows between layers
c.arrow([(130, 162), (130, 220)], color=SOLID["plum"], sw=2)
c.text(140, 196, "title, panels, camp node", size=10, color=MUTED)
c.arrow([(380, 162), (380, 220)], color=SOLID["plum"], sw=2)
c.text(390, 196, "LoadLevel, CallOverlayOp", size=10, color=MUTED)
c.arrow([(640, 162), (640, 220)], color=SOLID["plum"], sw=2)
c.text(650, 196, "DrawViewFrame", size=10, color=MUTED)
# services
R3 = 346
sv = [("Files, memory", "e1c0e e1b82 e19bc"), ("Graphics, text", "e17a6 e1de7, driver *.OVL"),
      ("Sound", "e177d e_text e1948"), ("Input, clocks", "e1aad e1a72 e1d8f")]
sw = 175
for i, (t, b) in enumerate(sv):
    c.box(i * (sw + 20), R3, sw, 46, "ochre", t, [b], size=10.3, tsize=12)
c.arrow([(380, 297), (380, 344)], color=SOLID["ochre"], sw=2)
c.text(390, 326, "every group above calls these", size=10, color=MUTED)
c.arrow([(380, 393), (380, 418)], color=SOLID["grey"], sw=2)
c.box(0, 420, 760, 44, "grey", "DOS, BIOS and the hardware", ["INT 9, 1Ch, 33h, 10h, 21h; video RAM and DAC; OPL2, speaker, SN76489; the PIT"], size=10.3, tsize=12)
c.save("figs/arc_callgraph.svg")

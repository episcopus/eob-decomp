import sys; sys.path.insert(0, '.')
from figlib import *

c = Canvas(760, 498)
c.section(0, 11, "WALL KINDS: FOUR TABLES OF 70 ENTRIES, INDEXED BY THE BYTE IN THE MAZE")
tabs = [
    ("gWallVmpSet", "ds:1879", "which picture block of the .VMP draws it (0 = none)", "teal"),
    ("gWallDecorHead", "ds:18BF", "first decoration record of the kind (1-based, 0 = none)", "ochre"),
    ("gWallHandlerIndex", "ds:1905", "what a click or a hit does: 0 nothing, 1 to 10 a handler", "navy"),
    ("gWallTypeFlags", "ds:194B", "the flag bits below", "plum"),
]
bw = 178
for i, (n, a, d, k) in enumerate(tabs):
    x = i * (bw + 15)
    c.rect(x, 24, bw, 66, fill=PAL[k][2], stroke=PAL[k][1], sw=1.1, rx=6)
    c.text(x + 8, 42, n, size=11.5, weight=700, color=PAL[k][0], mono=True)
    c.text(x + 8, 56, a, size=10.5, color=MUTED, mono=True)
    yy = 71
    for l in wrap(d, bw - 16, 10.5):
        c.text(x + 8, yy, l, size=10.5, color=INK2)
        yy += 12.5
c.text(0, 108, "Kinds 0 to 22 are built in. DefineWallType (ovl3:01C8) sets the kinds a level file names, from a 5-byte record:", size=10.5, color=MUTED)
c.text(0, 121, "kind, picture set, decoration, handler, flags. The file's flag byte f is stored as f ^ 4: bit 2 means 'blocks monsters' in the file.", size=10.5, color=MUTED)

# bit legend
c.line(0, 134, 760, 134, stroke=RULE)
c.section(0, 150, "THE FLAG BITS (gWallTypeFlags), AND WHERE THE CODE TESTS THEM")
bits = [
    ("0x01", "the party can walk through; a thrown item passes", "StepIfOpen, StepDDEffect", "teal"),
    ("0x02", "small things pass or see through (bars): items of icon size 15 or more", "StepDDEffect, TryMonsterRangedAttack", "teal"),
    ("0x04", "a monster may step in", "TryMoveMonster, StepMonsterAlongWall", "teal"),
    ("0x08", "a door (set on side 0 when the door lies north to south)", "OpenDoor, StepDoor", "ochre"),
    ("0x10", "a door in its last 'open' frame", "StepDoor, ResetLevel", "ochre"),
    ("0x20", "a door in its first 'closed' frame", "StepDoor, UnpryableDoorAction", "ochre"),
    ("0x40", "(only kind 2 has it; no code tests it)", "-", "grey"),
    ("0x80", "not a plain wall face: floor items and flying objects are drawn in the cell", "DrawSlotItems, DrawSlotProjectiles", "grey"),
]
yy = 170
for b, d, w, k in bits:
    c.rect(0, yy - 12, 44, 17, fill=PAL[k][2], stroke=PAL[k][1], sw=1, rx=3)
    c.text(22, yy + 1, b, size=11, weight=700, color=PAL[k][0], anchor="middle", mono=True)
    c.text(54, yy + 1, d, size=11, color=INK2)
    c.text(760, yy + 1, w, size=10.5, color=MUTED, anchor="end")
    yy += 21

# built-in kinds strip
yy += 8
c.line(0, yy - 8, 760, yy - 8, stroke=RULE)
c.section(0, yy + 8, "THE BUILT-IN KINDS 0 TO 24: FLAGS (HEX) AS THE PROGRAM CARRIES THEM")
flags = [0x07, 0x00, 0x40, 0xA8, 0x88, 0x88, 0x88, 0x9F, 0xA8, 0x88, 0x88, 0x88, 0x9F, 0xAA, 0x8A, 0x8A, 0x8A, 0x9F, 0xAA, 0x8A, 0x8A, 0x8A, 0x9F, 0x03, 0x03]
handlers = {3: 1, 4: 1, 5: 1, 6: 1, 7: 1, 8: 6, 13: 1, 14: 1, 15: 1, 16: 1, 17: 1, 18: 6}
cw = 29.2
sy = yy + 22
for i, f in enumerate(flags):
    x = i * cw
    if i == 0: k = "teal"
    elif i in (1, 2): k = "grey"
    elif 3 <= i <= 22:
        k = "ochre" if (i - 3) // 5 % 2 == 0 else "red"
    else: k = "teal"
    c.rect(x, sy, cw - 2, 44, fill=PAL[k][2], stroke=PAL[k][1], sw=1, rx=2)
    c.text(x + cw / 2 - 1, sy + 16, str(i), size=11.5, weight=700, color=PAL[k][0], anchor="middle")
    c.text(x + cw / 2 - 1, sy + 33, "%02X" % f, size=10.5, color=INK2, anchor="middle", mono=True)
lab = [(0, 0, "open"), (1, 2, "walls"), (3, 7, "door A"), (8, 12, "door B"), (13, 17, "door C"), (18, 22, "door D"), (23, 24, "kinds 23, 24")]
for a, b, t in lab:
    x0 = a * cw; x1 = (b + 1) * cw - 2
    c.path(f"M{x0},{sy+50} L{x0},{sy+55} L{x1},{sy+55} L{x1},{sy+50}", stroke=GREY, sw=1)
    c.text((x0 + x1) / 2, sy + 68, t, size=10, color=MUTED, anchor="middle")
yy = sy + 86
for l in wrap("A door is five kinds in a row: closed (A8), three opening frames (88), open (9F). C and D carry bit 1 as well (AA, 8A): small things pass the bars even when closed. Handler 1 (door button) sits on every frame of A and C, handler 6 (cannot be pried) on the closed frame of B and D. The level files redefine 23 and 24 and add kinds up to 69.", 700, 10.5):
    c.text(0, yy, l, size=10.5, color=MUTED)
    yy += 13.2
c.save("figs/wld_walltype.svg")

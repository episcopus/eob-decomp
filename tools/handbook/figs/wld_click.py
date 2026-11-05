import sys; sys.path.insert(0, '.')
from figlib import *

c = Canvas(760, 472)
c.section(0, 11, "FROM A CLICK (OR A HIT) ON A WALL TO A SCRIPT: DispatchWallHandler (122E:0896)")
# left: events
ev = [
    ("teal", "mouse click on the view", "node ds:4836 (24, 8, 128 x 96) calls ClickWallAhead", "0x40"),
    ("teal", "key 0x34", "HandleMapKey calls ActivateWallAhead", "0xFF"),
    ("ochre", "thrown item hits a wall", "StepDDEffect, cell and side of the flight", "0x10"),
    ("ochre", "spell hits a wall", "StepDDEffect, same call", "1"),
]
EX, EW, EH = 0, 214, 62
for i, (k, t, b, m) in enumerate(ev):
    y = 30 + i * (EH + 10)
    c.rect(EX, y, EW, EH, fill=PAL[k][2], stroke=PAL[k][1], sw=1.1, rx=6)
    c.text(EX + 8, y + 18, t, size=11.5, weight=700, color=PAL[k][0])
    yy = y + 33
    for l in wrap(b, EW - 80, 10.5):
        c.text(EX + 8, yy, l, size=10.5, color=INK2)
        yy += 12.5
    c.pill(EX + EW - 62, y + 18, "mode " + m, k, size=10, mono=False)
# centre chain
CX, CW = 256, 262
steps = [
    ("navy", "gWallTrigger = mode", "the event, a global (ds:EFD2)"),
    ("navy", "kind = wall of the cell, side ^ 2", "the face that looks at the party or flight"),
    ("navy", "selector = gWallHandlerIndex[kind]", "0: stop"),
    ("navy", "mode & mask, else stop", "mask = (cell +4 & 0xF8) >> 3 | 0xE0"),
    ("navy", "handlers[selector - 1](cell, side)", "a table of ten far pointers"),
    ("grey", "gWallTrigger = 0; gViewDirty = 1", "after any handler"),
]
SH = 46
for i, (k, t, b) in enumerate(steps):
    y = 30 + i * (SH + 14)
    c.rect(CX, y, CW, SH, fill=PAL[k][2], stroke=PAL[k][1], sw=1.1, rx=6)
    lines = wrap(t, CW - 14, 11, mono=False, weight=700)
    c.text(CX + 8, y + 17, lines[0], size=11, weight=700, color=PAL[k][0])
    if len(lines) > 1:
        c.text(CX + 8, y + 30, lines[1], size=11, weight=700, color=PAL[k][0])
        c.text(CX + 8, y + 42, b, size=10, color=MUTED)
    else:
        c.text(CX + 8, y + 33, b, size=10.5, color=MUTED)
    if i < len(steps) - 1:
        c.arrow([(CX + CW / 2, y + SH + 1), (CX + CW / 2, y + SH + 13)], color=GREY)
for i in range(4):
    y = 30 + i * (EH + 10) + EH / 2
    c.arrow([(EX + EW + 2, y), (CX - 3, 30 + 23 if False else 30 + 23)] if False else [(EX + EW + 2, y), (EX + EW + 22, y), (EX + EW + 22, 53), (CX - 3, 53)], color=GREY, sw=1.1)
# right: handlers
HX, HW = 548, 212
c.text(HX, 40, "the handlers", size=10.5, color=MUTED, weight=600)
hs = [
    ("1", "DoorButtonAction", "toggle the door, sound 6"),
    ("2", "WallScriptAction", "run the cell's script"),
    ("3", "WallTypeUpAction", "kind + 1 (a lever flips), script"),
    ("4", "WallTypeDownAction", "kind - 1, script"),
    ("5", "ForceDoorAction", "strongest member forces it"),
    ("6", "UnpryableDoorAction", "text: cannot be pried open"),
    ("10", "WallNicheAction", "take or put an item at position 8"),
]
yy = 56
for n, f, d in hs:
    c.badge(HX + 10, yy + 9, n, "navy", r=10 if len(n) == 1 else 11, size=10)
    c.text(HX + 28, yy + 8, f, size=10.5, weight=700, color=PAL["navy"][0], mono=True)
    c.text(HX + 28, yy + 21, d, size=10.5, color=INK2)
    yy += 36
c.arrow([(CX + CW + 2, 30 + 4 * (SH + 14) + SH / 2), (HX - 4, 30 + 4 * (SH + 14) + SH / 2)], color=SOLID["navy"])
# bottom notes
ny = 30 + 6 * (SH + 14) + 6
c.line(0, ny - 6, 760, ny - 6, stroke=RULE)
notes = [
    "Handlers 2, 3, 4 and 10 first ask IsWallDecorClicked: in mode 0x40 the click must fall in the decoration's rectangle (entry 1 of its ten, from the wall set); in any other mode the answer is yes.",
    "Selector 255 is never dispatched: a weapon hit on such a wall (ApplyItemAttack, 0DE8:0359) adds 1 to the wall byte of both faces instead. Selectors 7 to 9 are empty in the table and no level uses them.",
]
for t in notes:
    for l in wrap(t, 740, 10.5):
        c.text(0, ny + 10, l, size=10.5, color=MUTED)
        ny += 13
    ny += 3
c.save("figs/wld_click.svg")

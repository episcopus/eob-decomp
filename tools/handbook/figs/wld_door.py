import sys; sys.path.insert(0, '.')
from figlib import *

c = Canvas(760, 486)
c.section(0, 11, "A DOOR IS A CELL WHOSE TWO OPPOSITE SIDES CARRY THE SAME DOOR KIND; ANIMATING IT ADDS 1 TO BOTH")
# five states
sx0, sw_, sg = 28, 112, 33
sy, sh = 38, 70
states = [("closed", "kind k", "A8", "0x20 closed", "red"), ("frame 1", "k + 1", "88", "", "ochre"), ("frame 2", "k + 2", "88", "", "ochre"),
          ("frame 3", "k + 3", "88", "", "ochre"), ("open", "k + 4", "9F", "0x10 open", "teal")]
for i, (n, kd, f, note, k) in enumerate(states):
    x = sx0 + i * (sw_ + sg - 6)
    c.rect(x, sy, sw_ - 14, sh, fill=PAL[k][2], stroke=PAL[k][1], sw=1.3, rx=6)
    c.text(x + (sw_ - 14) / 2, sy + 20, n, size=13, weight=700, color=PAL[k][0], anchor="middle")
    c.text(x + (sw_ - 14) / 2, sy + 37, kd + ", flags " + f, size=10.5, color=INK2, anchor="middle")
    if note:
        c.text(x + (sw_ - 14) / 2, sy + 54, note, size=10.5, color=MUTED, anchor="middle")
    if i < 4:
        xa = x + sw_ - 14 + 2
        xb = x + sw_ + sg - 6 - 2
        c.arrow([(xa, sy + 24), (xb, sy + 24)], color=SOLID["teal"], sw=1.6)
        c.arrow([(xb, sy + 50), (xa, sy + 50)], color=SOLID["red"], sw=1.6)
c.text(sx0, sy + sh + 20, "opening: +1 on side s and side s + 2 every 5 ticks, sound 3 each step", size=11, color=PAL["teal"][0], weight=600)
c.text(sx0, sy + sh + 36, "closing: -1 the same way, sound 4 each step, sound 5 on reaching 'closed'", size=11, color=PAL["red"][0], weight=600)
c.text(760, sy + sh + 20, "kinds k = 3, 8, 13, 18", size=10.5, color=MUTED, anchor="end")

# who opens it
y2 = 188
c.line(0, y2 - 14, 760, y2 - 14, stroke=RULE)
c.section(0, y2, "WHO ASKS, AND THE ONE ANIMATION SLOT")
asks = [
    ("navy", "door button", "DoorButtonAction: a click on the button picture, ToggleDoor, sound 6"),
    ("navy", "cell script", "OpenDoorOp, CloseDoorOp, ToggleWallTypeOp (sub EA)"),
    ("ochre", "stuck door", "kinds 30, 31: ForceDoorAction rolls 1d20 against the strongest member, then the kind becomes 8 or 0x12 and OpenDoor runs"),
]
ax, aw = 0, 232
for i, (k, t, b) in enumerate(asks):
    x = i * (aw + 12)
    c.rect(x, y2 + 12, aw, 86, fill=PAL[k][2], stroke=PAL[k][1], sw=1.1, rx=6)
    c.text(x + 9, y2 + 30, t, size=12, weight=700, color=PAL[k][0])
    yy = y2 + 46
    for l in wrap(b, aw - 18, 10.5):
        c.text(x + 9, yy, l, size=10.5, color=INK2)
        yy += 13
# the effect
ey = y2 + 118
c.rect(0, ey, 760, 78, fill=PAL["plum"][2], stroke=PAL["plum"][1], sw=1.2, rx=6)
c.text(10, ey + 20, "gEffects[12]  (period 5 ticks; arg = the door's cell, sub = +1 or -1, active)", size=12, weight=700, color=PAL["plum"][0])
for i, l in enumerate(wrap("OpenDoor / CloseDoor / ToggleDoor arm it twice around a first StepDoor call, so the first frame shows at once. Each later tick runs StepDoor(arg). It stops itself (ArmEffect(12, 0)) when the kind has bit 0x10 while opening or 0x20 while closing. There is only one slot: a call for the same cell sets or reverses the direction, a call for another cell is ignored while a door moves.", 740, 10.5)):
    c.text(10, ey + 36 + i * 13, l, size=10.5, color=INK2)
# rules
ry = ey + 100
c.line(0, ry - 12, 760, ry - 12, stroke=RULE)
c.section(0, ry + 2, "QUIRKS AND RULES READ FROM THE CODE")
rules = [
    "CloseDoor does nothing while the cell holds any monster (count & 7); ToggleDoor tests only bit 0 of the count, so it still closes on a door with 2 or 4 monsters in it.",
    "ResetLevel (before a save or a level change) adds the step to both sides in a loop until the kind has bit 0x30, finishing a door that is still moving.",
]
yy = ry + 20
for r in rules:
    for j, l in enumerate(wrap(r, 730, 10.5)):
        c.text(0, yy, l, size=10.5, color=INK2)
        yy += 13
    yy += 3
c.save("figs/wld_door.svg")

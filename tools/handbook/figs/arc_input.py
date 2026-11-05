import sys; sys.path.insert(0, '.')
from figlib import *

c = Canvas(760, 360)
BW, GAP = 170, 20
def lane(y, title, kind, boxes):
    c.text(0, y - 8, title, size=11, weight=700, color=PAL[kind][0])
    for i, (t, b) in enumerate(boxes):
        x = i * (BW + GAP)
        c.box(x, y, BW, 84, kind, t, [b], size=10, tsize=11.2, fill=CARD)
        if i < len(boxes) - 1:
            c.arrow([(x + BW + 1, y + 42), (x + BW + GAP - 1, y + 42)], color=GREY)
lane(22, "Keyboard: the Up arrow", "teal", [
    ("key down", "scancodes E0h 48h on port 60h; the E0 prefix goes to the BIOS handler"),
    ("INT 9, 1AAD:07C0", "extended table, then folded to the keypad: key code 60h (keypad 8)"),
    ("ring word", "AL = 60h, AH = modifier bits: 1 shift, 2 ctrl, 4 alt, 8 release"),
    ("DispatchKeyWait", "IsKeyPending, WaitKeyEvent; a letter's shift bit is dropped"),
])
lane(142, "Mouse: a click on the forward arrow at (30, 135)", "plum", [
    ("INT 33h callback", "1A72:017A: clamps x, publishes gCursorX, gCursorY"),
    ("1AAD:0B0E", "button change becomes key 41h (left) or 42h (right); queues code, x, y"),
    ("WaitKeyEvent", "1AAD:048D copies x, y to gClickX, gClickY"),
    ("DispatchNodeEvent", "press = left mode 1; the hit point is the click, not the pointer"),
])
# merge
c.rect(0, 250, 760, 100, fill=PAL["navy"][2], stroke=PAL["navy"][1], sw=1.2, rx=6)
c.text(10, 270, "DispatchNodeEvent walks the node list (table 1b on the map screen), node by node", size=12.5, weight=700, color=PAL["navy"][0])
lines = [
    "key: a node matches when the event word equals its key06 (left) or key08 (right) word: Up arrow = keypad 8 = 0060h, node 30",
    "mouse: a node matches when the hit point is inside its box and its flags allow the button mode (press, held, release, idle)",
    "a match toggles the node's on bit, redraws it and calls its callback: MoveForward (0ACD:04F8) here, which ends in MoveParty",
    "a callback that returns nonzero ends the walk; one that returns 0 lets nodes further down the list see the same event",
]
for i, l in enumerate(lines):
    c.text(10, 290 + i * 16, l, size=10.5, color=INK2)
c.arrow([(741, 64), (753, 64), (753, 248)], color=SOLID["teal"], sw=1.4)
c.arrow([(3 * (BW + GAP) + 85, 227), (3 * (BW + GAP) + 85, 248)], color=SOLID["plum"], sw=1.6)
c.save("figs/arc_input.svg")

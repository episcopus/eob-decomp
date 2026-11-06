import sys; sys.path.insert(0, '.')
from figlib import *

c = Canvas(760, 400)
c.section(0, 11, "A. The 60-byte screen node (struct Node, ds:3A62 and the records after it)")
fields = [("next", 4, "grey"), ("key", 2, "navy"), ("k06", 2, "navy"), ("k08", 2, "navy"), ("draw", 3, "plum"),
          ("", 1, "grey"), ("flags", 2, "red"), ("src10, src14, src18", 12, "plum"), ("reg", 2, "teal"), ("x", 2, "teal"), ("y", 2, "teal"),
          ("w", 2, "teal"), ("h", 2, "teal"), ("colours", 8, "ochre"), ("state", 2, "red"), ("callback", 4, "navy"), ("data", 4, "grey"), ("df", 2, "grey"), ("", 2, "grey")]
u = 744.0 / 60
x = 4
off = 0
for n, b, k in fields:
    w = b * u
    c.rect(x, 24, w, 38, fill=MID[k], stroke=SOLID[k], sw=1, rx=0)
    if n and w > 11:
        c.text(x + w / 2, 41, n, size=9.2 if w < 40 else 10.5, weight=700, color=PAL[k][0], anchor="middle")
    if n:
        c.text(x + w / 2, 55, "%02X" % off, size=8.6, color=INK2, anchor="middle", mono=True)
    x += w
    off += b
c.para(4, 80, "key: the node's own id (the number a callback returns). k06, k08: key words that act as a left and a right click. draw: kind and the two modes. flags: which buttons and phases fire it. reg, x, y, w, h: its box in a text region's pixels. state: on, previous-on and pressed bits. callback: a far function. data: tile ids or a button definition. df: copy of the definition flags.", 750, size=10.2, color=INK2)

c.section(0, 130, "B. Button modes and the flag bits that react to them")
rows = [
    ("1", "press", "button went down (event 41h or 42h, or a key matching key06 or key08)", "0100h", "1000h"),
    ("2", "held", "GetKeyState says the button is down and no event arrived", "0200h", "2000h"),
    ("3", "release", "an event with AH bit 8: the button came up", "0400h", "4000h"),
    ("4", "idle", "button up and no event: the pointer only moves", "0800h", "8000h"),
]
y = 142
c.rect(0, y, 760, 20, fill=PANEL, rx=3)
for xx, t in ((8, "mode"), (60, "phase"), (140, "set by"), (570, "left flag"), (660, "right flag")):
    c.text(xx, y + 14, t, size=10.5, weight=700, color=INK2)
y += 22
for m, ph, how, lf, rf in rows:
    c.text(8, y + 14, m, size=11, weight=700, color=INK)
    c.text(60, y + 14, ph, size=11, color=INK)
    c.text(140, y + 14, how, size=10.4, color=INK2)
    c.text(570, y + 14, lf, size=10.5, color=INK2, mono=True)
    c.text(660, y + 14, rf, size=10.5, color=INK2, mono=True)
    c.line(0, y + 20, 760, y + 20, stroke=RULE, sw=0.6)
    y += 22
c.text(0, y + 12, "The map screen nodes use flags 1100h: react to a press of either button. Other bits: 0008h inactive, 0001h on-flag, 0004h pressed look,", size=10.2, color=MUTED)
c.text(0, y + 25, "0002h clear on leave, 0020h end the walk here, 0040h capture the pointer, 0080h freeze the list while captured (the last five read from the code [I]).", size=10.2, color=MUTED)

c.section(0, y + 54, "C. One pass of DispatchNodeEvent(head, key)")
steps = [
    ("set the mode", "left and right mode from GetKeyState (2 or 4); a click event overrides with 1 or 3 and moves the hit point to the click"),
    ("redraw", "if the list head changed or gNodeListChanged: DrawNode every node, once"),
    ("walk", "from the head (or the captured node): skip inactive nodes; a key equal to key06 or key08, or a hit point in the box with the right flag, marks the node"),
    ("fire", "the node's state is updated, DrawNode runs if it changed, then callback(node); nonzero ends the walk"),
]
y2 = y + 66
bw = 178
for i, (t, b) in enumerate(steps):
    xx = i * (bw + 16)
    c.box(xx, y2, bw, 96, "navy", t, [b], size=10, tsize=11.5, fill=CARD)
    if i < 3:
        c.arrow([(xx + bw + 1, y2 + 48), (xx + bw + 15, y2 + 48)], color=GREY)
c.h = y2 + 106
c.save("figs/arc_nodes.svg")

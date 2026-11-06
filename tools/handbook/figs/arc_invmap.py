import sys; sys.path.insert(0, '.')
from figlib import *

S = 2.3
X0, Y0 = 172, 0
W, H = 148, 200
c = Canvas(760, H * S + 6)
def P(x, y):
    return ((x - X0) * S, (y - Y0) * S)
c.rect(0, 0, W * S, H * S, fill="#FFFDF8", stroke=GREY, sw=1.4, rx=0)
def R(x, y, w, h, kind, label=None, lsize=9, fill=None, dash=None, op=0.6):
    px, py = P(x, y)
    c.rect(px, py, w * S, h * S, fill=fill or MID[kind], stroke=SOLID[kind], sw=1.1, rx=0, dash=dash, opacity=op)
    if label:
        c.text(px + w * S / 2, py + h * S / 2 + 3.2, label, size=lsize, weight=700, color=PAL[kind][0], anchor="middle")
slots = {0: (230, 116, 16, 16), 1: (278, 116, 16, 16)}
for i in range(14):
    slots[2 + i] = (181 + 18 * (i % 2), 40 + 18 * (i // 2), 16, 16)
slots.update({16: (225, 55, 16, 16), 17: (224, 76, 16, 16), 18: (225, 96, 16, 16), 19: (298, 55, 16, 16), 20: (287, 75, 16, 16),
              21: (277, 137, 16, 16), 22: (300, 94, 16, 16), 23: (300, 112, 16, 16), 24: (300, 130, 16, 16),
              25: (227, 135, 10, 10), 26: (239, 135, 10, 10)})
for n, (x, y, w, h) in slots.items():
    k = "teal" if n < 2 else ("navy" if n <= 15 else ("plum" if n <= 22 else ("ochre" if n <= 24 else "grey")))
    R(x, y, w, h, k, str(n), lsize=9)
R(181, 3, 32, 32, "red", "face", 9)
R(274, 35, 20, 15, "grey", "prev", 8.5)
R(297, 35, 20, 15, "grey", "next", 8.5)
R(236, 37, 31, 16, "ochre", "food", 9)
R(291, 149, 25, 17, "red", "P", 10)
LX = W * S + 24
c.text(LX, 16, "Inventory screen, node list 2b", size=12, weight=700, color=INK)
items = [
    ("teal", "slots 0, 1", "the two hands: item mask 08h; the same items the map screen's hand boxes show"),
    ("navy", "slots 2 to 15", "the pack: any item (mask FFFFh); 2 columns of 7 on an 18-pixel grid"),
    ("plum", "slots 16 to 22", "one box per item class, by gSlotItemMask: 01h, 02h, 04h, 20h, 40h, 10h, and slot 22 takes anything; slot 16 is also a list head"),
    ("ochre", "slots 23, 24", "mask 80h: two boxes for one class [H: the rings]"),
    ("grey", "slots 25, 26", "mask 100h, drawn 10 x 10 (gInvSlotKind 2) [H: ammunition]"),
    ("ochre", "food box", "FeedInventoryMember: eat the item on the pointer"),
    ("red", "face", "RestorePortraitPanel: back to the map; Esc or I"),
    ("grey", "prev, next", "click: the previous or next member; F1 to F6 jump to members 1 to 6 through the same callbacks (F5, F6 have no box)"),
    ("red", "P", "ToggleCharacterInfo: switch to the character sheet (node list 3) and back"),
]
y = 36
for k, t, b in items:
    c.rect(LX, y - 8, 12, 10, fill=MID[k], stroke=SOLID[k], sw=1, rx=1)
    c.text(LX + 18, y + 1, t, size=10.8, weight=700, color=INK)
    ls = wrap(b, 340, 10)
    for j, l in enumerate(ls):
        c.text(LX + 18, y + 14 + 11.5 * j, l, size=10, color=INK2)
    y += 14 + 11.5 * len(ls) + 10
c.text(LX, y + 6, "The rest of list 2b is the map screen's: view click, floor,", size=10, color=MUTED)
c.text(LX, y + 18, "camp, movement and throw nodes stay active.", size=10, color=MUTED)
c.save("figs/arc_invmap.svg")

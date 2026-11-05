import sys; sys.path.insert(0, '.')
from figlib import *

# gInvSlotX (ds:17CE), gInvSlotY (ds:1804), gInvSlotKind (ds:3875), gSlotItemMask (ds:1C8D): screen positions in the
# 320 x 200 inventory screen, drawn at 3x.
X = [230, 278, 181, 199, 181, 199, 181, 199, 181, 199, 181, 199, 181, 199, 181, 199, 225, 224, 225, 298, 287, 277, 300, 300, 300, 228, 240]
Y = [116, 116, 40, 40, 58, 58, 76, 76, 94, 94, 112, 112, 130, 130, 148, 148, 56, 76, 96, 55, 75, 137, 94, 112, 130, 136, 136]
KIND = [0] * 16 + [1] + [0] * 8 + [2, 2]
MASK = [0x8, 0x8] + [0xFFFF] * 14 + [0x1, 0x2, 0x4, 0x20, 0x40, 0x10, 0xFFFF, 0x80, 0x80, 0x100, 0x100]
S = 3.0
OX, OY = 14 - 176 * S + 176 * S - 176 * S + 176 * S, 0  # placeholder, replaced below
OX = 6 - 172 * S
OY = 28 - 34 * S

def sx(x):
    return OX + x * S

def sy(y):
    return OY + y * S

def group(i):
    if i < 2:
        return "navy"
    if i < 16 or i == 22:
        return "paper"
    if i == 16:
        return "red"
    if i in (17, 18, 19, 21):
        return "teal"
    if i == 20:
        return "teal"
    if i in (23, 24):
        return "plum"
    return "ochre"

c = Canvas(760, 440)
c.section(0, 11, "The 27 item slots of a character (items[0..26]) on the inventory screen, 3 times life size")
# screen region
c.rect(sx(176), sy(35), (318 - 176) * S, (166 - 35) * S, fill="none", stroke=RULE, sw=1, rx=4, dash="3 3")
names = {0: "right hand", 1: "left hand", 16: "quiver", 17: "armour", 18: "bracers", 19: "helmet", 20: "necklace", 21: "boots", 22: "any", 23: "small", 24: "small", 25: "ring", 26: "ring"}
for i in range(27):
    k = KIND[i]
    size = 8 if k == 2 else 16
    x, y = sx(X[i] - 1), sy(Y[i] - 1)
    w = (size + 1) * S
    tc, sc, fc = PAL[group(i)]
    c.rect(x, y, w, w, fill=fc, stroke=sc, sw=1.3, rx=3)
    c.text(x + w / 2, y + w / 2 + 4, str(i), size=12 if k != 2 else 10.5, weight=700, color=tc, anchor="middle")
    if i in names:
        nm = names[i]
        if i == 0:
            c.text(x - 4, y + w / 2 + 4, "hand 0", size=10, color=tc, anchor="end")
        elif i == 1:
            c.text(x + w / 2, y - 5, "hand 1", size=10, color=tc, anchor="middle")
        elif i in (23, 24):
            c.text(x + w + 5, y + w / 2 + 4, nm, size=10, color=tc)
        elif i in (19, 20, 22):
            c.text(x - 4, y + w / 2 + 4, nm, size=10, color=tc, anchor="end")
        elif i in (25, 26):
            c.text(x + w / 2, y + w + 12, nm, size=10, color=tc, anchor="middle")
        elif i == 21:
            c.text(x - 4, y + w / 2 + 4, nm, size=10, color=tc, anchor="end")
        else:
            c.text(x + w + 5, y + w / 2 + 4, nm, size=10, color=tc)
c.text(sx(181), sy(166) + 14, "slots 2 to 15: the backpack", size=10, color=MUTED)

# ---- right: legend of the rules ----
RX = 480
c.text(RX, 40, "What each group accepts", size=11, weight=700, color=PAL["teal"][0])
rules = [
    ("navy", "0, 1 hands", "mask 08h; hands are also where weapons act; two-handed types block the other hand"),
    ("paper", "2 to 15, 22", "mask FFFFh: any item (slot 22 sits in the right column)"),
    ("red", "16 quiver", "mask 01h: arrows only. items[16] is the head of a list; the row prints its count"),
    ("teal", "17 to 21", "armour 02h, bracers 04h, boots 10h, helmet 20h, necklace 40h"),
    ("plum", "23, 24", "mask 80h: small items (daggers, darts, potions, gems, scrolls, keys)"),
    ("ochre", "25, 26", "mask 100h: rings; drawn in a half-size box, the icon centred on it"),
]
y = 52
for k, t, d in rules:
    tc, sc, fc = PAL[k]
    c.rect(RX, y, 14, 14, fill=fc, stroke=sc, sw=1.2, rx=3)
    c.text(RX + 22, y + 11, t, size=10.5, weight=700, color=tc)
    for l in wrap(d, 250, 10):
        c.text(RX + 22, y + 25, l, size=10, color=INK2)
        y += 12
    y += 32
c.text(RX, y + 6, "A drop or swap is allowed when", size=10.5, weight=700, color=MUTED)
c.text(RX, y + 20, "gSlotItemMask[slot] AND the type's slotMask", size=10, color=INK2, mono=True)
c.text(RX, y + 33, "is not zero (CanPlaceItemInSlot).", size=10.5, color=INK2)
c.save("figs/chr_slots.svg")

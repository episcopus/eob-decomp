import sys; sys.path.insert(0, '.')
from figlib import *

c = Canvas(760, 500)
c.section(0, 11, "ITEM.DAT and ITEMTYPE.DAT: the two record layouts")

# ---- files ----
c.text(0, 32, "ITEM.DAT, 9,482 bytes", size=10.5, weight=700, color=MUTED)
fx, fw = 140, 600
tot = 9482.0
parts = [(2, "2", "paper"), (6048, "432 records of 14 bytes (gItems[0..431])", "teal"), (2, "", "paper"), (3430, "98 names of 35 bytes (1F28:0000)", "ochre")]
x = fx
for n, t, k in parts:
    w = max(2.0, n / tot * fw)
    tc, sc, fc = PAL[k]
    c.rect(x, 20, w, 18, fill=fc, stroke=sc, sw=1, rx=0)
    if t and w > 40:
        c.text(x + w / 2, 33, t, size=10, color=tc, anchor="middle", weight=600)
    x += w
c.text(0, 56, "ITEMTYPE.DAT, 914 bytes", size=10.5, weight=700, color=MUTED)
c.rect(fx, 44, 2 / 914.0 * fw, 18, fill=PAL["paper"][2], stroke=PAL["paper"][1], rx=0)
c.rect(fx + 2 / 914.0 * fw, 44, 912 / 914.0 * fw, 18, fill=PAL["navy"][2], stroke=PAL["navy"][1], rx=0)
c.text(fx + 300, 57, "57 records of 16 bytes (gItemTypes[0..56])", size=10, color=PAL["navy"][0], anchor="middle", weight=600)

U = 40.0
def record(y, title, n, fields, hint):
    c.text(0, y - 8, title, size=11.5, weight=700, color=PAL["teal"][0])
    ox = 20
    for o in range(n + 1):
        c.line(ox + o * U, y - 2, ox + o * U, y, stroke=GREY, sw=0.8)
    for o in range(0, n, 1):
        c.text(ox + o * U + U / 2, y + 44 + 14 + 2, "", size=9)
    for (s, sz, name, kind, note) in fields:
        tc, sc, fc = PAL[kind]
        c.rect(ox + s * U, y, sz * U, 34, fill=fc, stroke=sc, sw=1.1, rx=0)
        c.text(ox + s * U + sz * U / 2, y + 15, name, size=10.5, weight=700, color=tc, anchor="middle")
        c.text(ox + s * U + sz * U / 2, y + 28, "+%X" % s, size=9.5, color=MUTED, anchor="middle", mono=True)
    ly = y + 52
    for (s, sz, name, kind, note) in fields:
        pass
    return ox

# ItemRecord
Y1 = 96
rec1 = [
    (0, 1, "name", "ochre", ""), (1, 1, "name2", "ochre", ""), (2, 1, "flags", "red", ""), (3, 1, "icon", "grey", ""),
    (4, 1, "type", "navy", ""), (5, 1, "cellPos", "teal", ""), (6, 2, "key", "teal", ""), (8, 2, "prevLink", "teal", ""),
    (10, 2, "nextLink", "teal", ""), (12, 1, "level", "teal", ""), (13, 1, "value", "plum", ""),
]
record(Y1, "ItemRecord, 14 bytes, 500 of them at ds:8DD0", 14, rec1, "")
notes1 = [
    "name, name2: indexes into the 35-byte names; name2 is the identified name (1 = none)",
    "flags: see the table below.  icon: row of the icon sheets.  type: index into gItemTypes",
    "cellPos: place in the cell (0 to 3, 8 niche) or a tag.  key: owner (map cell), -1 free, -2 spell item free",
    "prevLink, nextLink: the circular list the item sits in.  level: dungeon level.  value: enchantment, spell, food, effect",
]
for i, t in enumerate(notes1):
    c.text(20, Y1 + 52 + i * 13, t, size=10, color=INK2)

# ItemType
Y2 = 232
rec2 = [
    (0, 2, "slotMask", "navy", ""), (2, 2, "word", "grey", ""), (4, 1, "AC", "red", ""), (5, 1, "class", "teal", ""),
    (6, 1, "hands", "plum", ""), (7, 1, "n", "ochre", ""), (8, 1, "d", "ochre", ""), (9, 1, "+", "ochre", ""),
    (10, 1, "n", "ochre", ""), (11, 1, "d", "ochre", ""), (12, 1, "+", "grey", ""), (13, 1, "pad", "grey", ""), (14, 2, "kind", "red", ""),
]
record(Y2, "ItemType, 16 bytes, 57 of them at ds:A9F0", 16, rec2, "")
notes2 = [
    "slotMask: which slot kinds take it (bits below).  AC: signed change (-2 leather to -7 plate); 0E2h marks a free spell type",
    "class: ANDed with gClassBaseBits[class].  hands: 2 = two-handed (CanUseHand).  word: not read in src/ [?]",
    "damage dice: n d  + bonus against ordinary monsters (+7, +8, +9) and n d against large ones (+A, +B, same bonus +9); +C copies +9, unread",
    "kind: low 7 bits choose the use (see text); bit 7 (80h) marks an item a monster can steal",
]
for i, t in enumerate(notes2):
    c.text(20, Y2 + 52 + i * 13, t, size=10, color=INK2)

# ---- bit tables ----
TY = 354
c.section(0, TY - 4, "ItemRecord.flags")
fl = [("80h", "magical: Detect Magic draws it through the colour remap"), ("40h", "identified: the real name is shown"),
      ("3Fh", "wands, staffs (type 48): charges left, one per use"), ("20h", "spell-made items: cannot be moved out of the slot"),
      ("10h", "spell-made items: used up by one hit"), ("08h", "Vampiric Touch: damage dealt also heals the user")]
for i, (b, t) in enumerate(fl):
    y = TY + 8 + i * 17
    c.pill(0, y + 6, b, "red", size=10, solid=True, mono=True)
    c.text(46, y + 10, t, size=10.5, color=INK2)
c.section(380, TY - 4, "slotMask bits")
mk = [("01h", "quiver, slot 16 (arrows)"), ("02h", "body armour, slot 17"), ("04h", "bracers, slot 18"), ("08h", "hands, slots 0 and 1: nearly every item"),
      ("10h", "boots, slot 21"), ("20h", "helmet, slot 19"), ("40h", "necklace, slot 20"), ("80h", "small items, slots 23 and 24"), ("100h", "rings, slots 25 and 26")]
for i, (b, t) in enumerate(mk):
    y = TY + 8 + i * 15
    c.pill(380, y + 6, b, "navy", size=10, solid=True, mono=True)
    c.text(430, y + 10, t, size=10.5, color=INK2)
c.save("figs/chr_itemrec.svg")

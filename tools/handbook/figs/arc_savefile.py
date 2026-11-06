import sys; sys.path.insert(0, '.')
from figlib import *

c = Canvas(760, 400)
c.section(0, 11, "EOBDATA.SAV, 33,101 bytes, to scale (1 pixel = 44.5 bytes)")
blocks = [("characters", 1458, "teal", "6 x 243"), ("hdr", 64, "red", ""), ("items", 7000, "navy", "500 x 14"),
          ("level states", 24480, "plum", "12 x 2,040 = LEVELS.TMP"), ("", 3, "ochre", ""), ("", 96, "ochre", "")]
total = 33101.0
W = 744.0
x = 8
offs = []
off = 0
for n, b, k, sub in blocks:
    w = max(1.6, W * b / total)
    c.rect(x, 26, w, 40, fill=MID[k], stroke=SOLID[k], sw=1.1, rx=0)
    if w > 80:
        c.text(x + w / 2, 43, n, size=12, weight=700, color=PAL[k][0], anchor="middle")
        c.text(x + w / 2, 58, sub, size=10, color=INK2, anchor="middle")
    offs.append((off, x, w))
    x += w
    off += b
# callouts
c.text(8 + 33 / 2, 82, "characters 1,458", size=10, color=INK2, anchor="start")
c.line(8 + 16, 66, 8 + 16, 74, stroke=GREY)
hx = 8 + W * 1458 / total
c.line(hx + 1, 66, hx + 1, 92, stroke=GREY, sw=0.9)
c.text(hx + 5, 100, "header 64 (at offset 1,458)", size=10, color=INK2)
c.line(8 + W - 2, 66, 8 + W - 2, 84, stroke=GREY, sw=0.9)
c.text(8 + W - 6, 92, "3 option bytes, then 96 bytes of item types", size=10, color=INK2, anchor="end")
for o in (0, 1458, 1522, 8522, 33002, 33101):
    pass
c.text(8, 118, "offsets: 0000 characters | 05B2 header | 05F2 items | 214A level states | 80EA options | 80ED item types | 814D end", size=10, color=MUTED, mono=False)

# header
c.section(0, 150, "The 64-byte header: a copy of ds:FBC8 to FC07")
hf = [("gCurLevel", 2, "navy", "level 1 to 12"), ("gPartyCell", 2, "navy", "y * 32 + x"), ("gPartyDir", 2, "navy", "0 to 3"),
      ("gMouseItem", 2, "navy", "item index"), ("gSavedLevels", 2, "teal", "bit n: level n+1 is in LEVELS.TMP"),
      ("spell", 2, "ochre", ""), ("level 0", 4, "grey", ""), ("gLevelFlags[1] to [11]", 44, "plum", "44 bytes: 32-bit flags of levels 1 to 11"), ("gGameFlags", 4, "red", "= flags of 'level 12'")]
u = 744.0 / 64
x = 8
off = 0
for n, b, k, sub in hf:
    w = b * u
    c.rect(x, 164, w - 1, 38, fill=MID[k], stroke=SOLID[k], sw=1, rx=0)
    c.text(x + (w - 1) / 2, 181, "%02X" % off, size=9, color=INK2, anchor="middle", mono=True)
    off += b
    x += w
leg = [
    ("00", "FBC8", "gCurLevel", "the dungeon level, 1 to 12"),
    ("02", "FBCA", "gPartyCell", "the party's cell, y x 32 + x [I]"),
    ("04", "FBCC", "gPartyDir", "0 north, 1 east, 2 south, 3 west"),
    ("06", "FBCE", "gMouseItem", "the item on the pointer"),
    ("08", "FBD0", "gSavedLevels", "bit n set: level n + 1 is in LEVELS.TMP"),
    ("0A", "FBD2", "gPartySpellFlags", "bless, prayer, detect magic"),
    ("0C", "FBD4", "gLevelFlags[0]", "never used"),
    ("10", "FBD8", "gLevelFlags[1..11]", "32-bit script flags, one word per level"),
    ("3C", "FC04", "gGameFlags", "32-bit script flags of the whole game"),
]
for i, (o, a_, n, t) in enumerate(leg):
    colx = 0 if i < 5 else 372
    row = i if i < 5 else i - 5
    y = 218 + row * 14
    c.text(colx + 8, y, "+" + o, size=9.8, color=INK2, mono=True)
    c.text(colx + 40, y, a_, size=9.8, color=MUTED, mono=True)
    c.text(colx + 76, y, n, size=10, weight=700, color=INK)
    c.text(colx + 76 + text_width(n, 10, False, 700) + 8, y, t, size=9.8, color=INK2)
c.text(8, 296, "Words are 16-bit. gLevelFlags is indexed by gCurLevel (1 to 12), so index 0 is never used and level 12 writes into gGameFlags: the two share the last 4 bytes.", size=10, color=MUTED)

# level state
c.section(0, 322, "One level state, 2,040 bytes (7F8h), the unit of LEVELS.TMP and of the 12 slots")
ls = [("LCW stream of the 4,096 wall bytes (1,024 cells x 4); at most 1,200 bytes (4B0h), the rest of the area unused", 1200, "navy"),
      ("30 monster records, 28 bytes each: 840 bytes (348h)", 840, "red")]
tot2 = 2040.0
x = 8
for t, b, k in ls:
    w = W * b / tot2
    c.rect(x, 336, w - 1, 36, fill=MID[k], stroke=SOLID[k], sw=1, rx=0)
    for j, l in enumerate(wrap(t, w - 12, 10)[:3]):
        c.text(x + 6, 351 + 11 * j, l, size=10, color=PAL[k][0], weight=600 if j == 0 else 400)
    x += w
c.text(8, 394, "The walls of a cell can change (doors, switches), and the monsters move, so a level is saved as its changed walls and its monsters. The pristine walls", size=10, color=MUTED)
c.text(8, 407, "are in LEVELn.MAZ; items are not here (they live in the 500-item block, with a level number each).", size=10, color=MUTED)
c.h = 418
c.save("figs/arc_savefile.svg")

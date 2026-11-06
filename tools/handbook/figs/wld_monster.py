import sys; sys.path.insert(0, '.')
from figlib import *

c = Canvas(760, 470)

def bar(y, title, total, cw, fields, x0=16, h=26, note=None):
    """fields: (offset, size, name, kind, label_row) label_row 0 = inside if fits, 1, 2 = below."""
    c.text(0, y - 17, title, size=11, weight=700, color=INK2)
    for off, n, name, kind, row in fields:
        x = x0 + off * cw
        c.rect(x, y, n * cw, h, fill=PAL[kind][2], stroke=PAL[kind][1], sw=1, rx=0)
        inside = (row == 0)
        if inside:
            c.text(x + n * cw / 2, y + 17, name, size=10.5, weight=700, color=PAL[kind][0], anchor="middle")
    for i in range(total):
        if i % 2 == 0 or cw > 40:
            c.text(x0 + i * cw + cw / 2, y - 1, "%X" % i if total > 14 else "%d" % i, size=8.5, color=MUTED, anchor="middle", mono=True)
    for off, n, name, kind, row in fields:
        if row:
            x = x0 + off * cw + n * cw / 2
            yy = y + h + 8 + (row - 1) * 14
            c.line(x, y + h, x, yy - 8, stroke=PAL[kind][1], sw=0.7)
            c.text(x, yy + 3, name, size=10, weight=600, color=PAL[kind][0], anchor="middle")

# INF record
bar(52, "A MONSTER RECORD IN THE LEVEL FILE: 14 BYTES, 30 OF THEM (a record with cell 0 is an empty slot)", 14, 52, [
    (0, 1, "slot", "grey", 0), (1, 1, "spd", "navy", 0), (2, 2, "cell", "teal", 0), (4, 1, "pos", "teal", 0), (5, 1, "dir", "teal", 0),
    (6, 1, "type", "red", 0), (7, 1, "alt", "plum", 0), (8, 1, "mode", "ochre", 0), (9, 1, "tmr", "ochre", 0),
    (10, 2, "drop 1 (50%)", "grey", 0), (12, 2, "drop 2", "grey", 0)])
c.text(16, 106, "spd: speed class 0 or 1 (which pair of update effects); pos 0..3 quadrant, 4 centre; type: row of the type table; alt: second sprite sheet", size=10, color=MUTED)

# Monster record
cw = 26
bar(150, "A LIVE MONSTER, struct Monster, 28 BYTES (gMonsters, ds:8A82, 30 RECORDS = 840 BYTES)", 28, cw, [
    (0, 1, "type", "red", 0), (1, 1, "grp", "navy", 0), (2, 2, "cell", "teal", 0), (4, 1, "pos", "teal", 0), (5, 1, "dir", "teal", 0),
    (6, 1, "an", "plum", 0), (7, 1, "alt", "plum", 0), (8, 1, "md", "ochre", 0), (9, 1, "st", "ochre", 0), (10, 1, "af", "plum", 0), (11, 1, "tm", "ochre", 0),
    (12, 2, "maxHp", "red", 0), (14, 2, "hp", "red", 0), (16, 2, "target", "teal", 0), (18, 2, "drop1", "grey", 0), (20, 2, "drop2", "grey", 0),
    (22, 1, "fl", "navy", 0), (23, 1, "dr", "plum", 0), (24, 1, "dl", "navy", 0), (25, 1, "ac", "navy", 0), (26, 2, "unused", "grey", 0)], x0=16)
notes = [
    ("+0 typeIndex: row of the type table; +1 updateGroup 0..3; +2 cell; +4 cellPos (quadrant 0..3, centre 4); +5 orient 0..3", None),
    ("+6 animPhase (walk frame, flips each update); +7 altShapes; +8 mode (0..10); +9 state (wall-follow memory); +A attackFrame (0xFE, 0xFF while swinging); +B timer", None),
    ("+C maxHp [H], +E hp (dead at 0 or less); +10 targetCell (where it heads); +12, +14 drops; +16 flags: 1 attacked, 2 hit flash, 4 swing, 8 fleeing, 0x10 turn tried", None),
    ("+17 drawOffset (x high nibble, y low; re-rolled by effect 15); +18 actionDelay and +19 actionsLeft (ranged attacks); +1A, +1B read by no code", None),
]
yy = 202
for t, _ in notes:
    for l in wrap(t, 740, 10):
        c.text(0, yy, l, size=10, color=INK2)
        yy += 12
    yy += 2

# MonsterType
cw2 = 26
y3 = yy + 40
bar(y3, "A MONSTER TYPE, struct MonsterType, 27 BYTES (gMonsterTypes, ds:1A3B, 22 ROWS = 594 BYTES)", 27, cw2, [
    (0, 1, "AC", "teal", 0), (1, 1, "T0", "teal", 0), (2, 1, "HD", "red", 0), (3, 1, "n", "ochre", 0),
    (4, 3, "attack 1", "ochre", 0), (7, 3, "attack 2", "ochre", 0), (10, 3, "attack 3", "ochre", 0), (13, 1, "-", "grey", 0),
    (14, 1, "fl", "navy", 0), (15, 4, "hold, undead", "plum", 0), (19, 2, "xp", "red", 0), (21, 1, "sz", "teal", 0), (22, 1, "hit", "grey", 0),
    (23, 1, "mv", "grey", 0), (24, 1, "ra", "ochre", 0), (25, 1, "tu", "plum", 0), (26, 1, "mr", "navy", 0)], x0=16)
notes2 = [
    "+0 armour class (the party's to-hit needs d20 >= 20 - level steps - AC - bonus); +1 the monster's to-hit number (needs d20 >= T0 - the target's armour class); +2 hit dice (hit points = HD d8, or 1d4 when 255)",
    "+3 number of attacks, then three (count, sides, bonus) dice; +E flags: 1 large-weapon damage, 2 stepping frames, 4 half damage from plain weapons, 8 attacks after moving, 0x10 poison, 0x20 paralysis, 0x40 ranged, 0x80 steals an item",
    "+F bit 0 held by Hold Person, bit 1 by Hold Monster, bit 2 undead; +13 experience; +15 size (nonzero: alone in a cell); +16 hit sound; +17 move sound; +18 ranged uses (255 = unlimited); +19 turn-undead row (-1 none); +1A magic resistance percent",
]
yy = y3 + 38
for t in notes2:
    for l in wrap(t, 740, 10):
        c.text(0, yy, l, size=10, color=INK2)
        yy += 12
    yy += 2
c.h = yy + 4
c.save("figs/wld_monster.svg")

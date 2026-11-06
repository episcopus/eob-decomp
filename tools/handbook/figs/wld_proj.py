import sys; sys.path.insert(0, '.')
from figlib import *

c = Canvas(760, 540)
c.section(0, 11, "A FLYING ITEM OR SPELL: HALF A CELL EVERY 3 TICKS (effect 1, MoveDDEffects)")

# --- left: two cells, quadrants, northbound ---
S = 112
X0, Y0 = 20, 52
TC, SC, FC = PAL["teal"]
for r, lab in enumerate(["next cell ahead", "the party's cell"]):
    y = Y0 + r * S
    c.rect(X0, y, S, S, fill=(FC if r == 1 else CARD), stroke=(SC if r == 1 else GREY), sw=1.4, rx=0)
    q = S / 2
    for i, (qx, qy) in enumerate([(0, 0), (1, 0), (0, 1), (1, 1)]):
        c.rect(X0 + qx * q + 6, y + qy * q + 6, q - 12, q - 12, fill=CARD, stroke=SOLID["teal"], sw=1, rx=3)
        c.text(X0 + qx * q + q / 2, y + qy * q + q / 2 + 5, str(i), size=13, weight=700, color=TC, anchor="middle")
    c.text(X0 + S + 10, y + S / 2 + 4, lab, size=10.5, color=MUTED)
# arrows in column 0 and column 1 flying north
q = S / 2
ax = X0 + q / 2
# from party cell pos 2 -> pos 0 (inside)
c.arrow([(ax, Y0 + S + q + q / 2 - 12), (ax, Y0 + S + q / 2 + 14)], color=SOLID["red"], sw=1.8)
# pos 0 of party cell -> pos 2 of next cell
c.arrow([(ax, Y0 + S + q / 2 - 14), (ax, Y0 + q + q / 2 + 14)], color=SOLID["red"], sw=1.8)
c.text(X0 + 4, Y0 + 2 * S + 18, "thrown north from position 2:", size=10.5, color=PAL["red"][0], weight=600)
c.text(X0 + 4, Y0 + 2 * S + 32, "2 -> 0 (same cell), 0 -> 2 (next cell), ...", size=10.5, color=PAL["red"][0])
c.text(X0 + 4, Y0 + 2 * S + 48, "gNextPosByDir[dir * 4 + pos]: bit 7 set = cross", size=10.5, color=MUTED)
c.text(X0 + 4, Y0 + 2 * S + 62, "into StepCell(cell, dir); the rest is the new pos", size=10.5, color=MUTED)

# --- right: one step ---
RX = 300
c.section(RX, 38, "ONE STEP OF ONE LIVE RECORD")
steps = [
    ("navy", "pick the next position", "sel = gNextPosByDir[dir * 4 + (pos & 3)]; bit 7 means the next cell, target = StepCell(cell, dir)"),
    ("navy", "StepDDEffect: may it enter?", "wall of the target on the side facing back: an item passes when flag bit 0, or while still in its launch cell, or flag bit 1 and a small item; a spell when flag bit 0 or the same cell"),
    ("red", "no: it hits the wall", "gWallTrigger = 0x10 (item) or 1 (spell), DispatchWallHandler on that wall, then the record is cleared"),
    ("teal", "yes: move", "the item leaves the old cell's list and joins the target's with tag pos | 4; count down m0D; a new cell runs its script in mode 0x10"),
    ("plum", "TestDDEffect: anyone there?", "monsters in the quadrant or centre: TryRecordAttack (to-hit roll, damage, DamageMonster); no monsters and the party's cell: RunRecordAttack hits a member"),
    ("grey", "end of flight", "after a hit or when m0D runs out (12 steps, about 6 cells): ClearDDEffect: tag & 3, script mode 4 (the item lands), sound 0x12"),
]
y = 52
for k, t, b in steps:
    lines = wrap(b, 420, 10.2)
    h = 24 + 12 * len(lines)
    c.rect(RX, y, 460, h, fill=PAL[k][2], stroke=PAL[k][1], sw=1.1, rx=6)
    c.text(RX + 9, y + 16, t, size=11, weight=700, color=PAL[k][0])
    yy = y + 29
    for l in lines:
        c.text(RX + 9, yy, l, size=10.2, color=INK2)
        yy += 12
    y += h + 9

# --- bottom: record ---
BY = int(y) + 24
c.line(0, BY - 14, 760, BY - 14, stroke=RULE)
c.section(0, BY, "THE RECORD, struct Projectile, 18 BYTES (gProjectiles, ds:DD2C, 10 RECORDS)")
cw = 40
fields = [(0, 1, "act", "navy"), (1, 1, "", "grey"), (2, 2, "m02", "plum"), (4, 2, "cell", "teal"), (6, 1, "pos", "teal"), (7, 1, "dir", "teal"), (8, 2, "item", "ochre"),
          (10, 1, "typ", "ochre"), (11, 1, "who", "navy"), (12, 1, "fl", "plum"), (13, 1, "rng", "red"), (14, 4, "fn (spells)", "plum")]
x0 = 0
# 18 bytes at 40 px = 720
for off, n, name, k in fields:
    x = x0 + off * cw
    c.rect(x, BY + 14, n * cw, 26, fill=PAL[k][2], stroke=PAL[k][1], sw=1, rx=0)
    c.text(x + n * cw / 2, BY + 31, name, size=10.5, weight=700, color=PAL[k][0], anchor="middle")
for i in range(18):
    c.text(i * cw + cw / 2, BY + 52, "%X" % i if i < 16 else "%X" % i, size=8.5, color=MUTED, anchor="middle", mono=True)
notes = [
    "act: 0 free, 1 a thrown item, 2 a spell effect. m02: nonzero on launch, so nothing is hit in the launching cell; cleared when the record crosses into a new cell. item: an index into gItems (a spell: the effect mode).",
    "typ: the item type. who: the thrower (a party member, or -1 for a script or a monster; monsters' missiles hit the party). fl: bit 0x20 makes a blocked flight call AnimateEffect. rng: steps left, 12 at launch.",
    "A script can launch one (opcode E9) or turn every flying object in its cell (E8 F5). ResetLevel clears all ten before a save or a level change, putting thrown items down where they flew.",
]
yy = BY + 70
for t in notes:
    for l in wrap(t, 730, 10):
        c.text(0, yy, l, size=10, color=INK2)
        yy += 12
    yy += 2
c.h = yy + 4
c.save("figs/wld_proj.svg")

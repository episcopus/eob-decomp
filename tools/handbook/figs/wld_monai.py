import sys; sys.path.insert(0, '.')
from figlib import *

c = Canvas(760, 590)
c.section(0, 11, "ONE MONSTER TICK: FOUR TIMED EFFECTS, EACH UPDATING ONE QUARTER OF THE MONSTERS")
# effects
ex = [("effect 2", "group 0", "class 0, even slots", "navy"), ("effect 3", "group 1", "class 0, odd slots", "navy"),
      ("effect 4", "group 2", "class 1, even slots", "teal"), ("effect 5", "group 3", "class 1, odd slots", "teal")]
bw = 118
for i, (a, g, d, k) in enumerate(ex):
    x = i * (bw + 8)
    c.rect(x, 24, bw, 62, fill=PAL[k][2], stroke=PAL[k][1], sw=1.1, rx=6)
    c.text(x + 8, 41, a + ": " + g, size=11, weight=700, color=PAL[k][0])
    c.text(x + 8, 55, d, size=10, color=INK2)
    c.text(x + 8, 69, "period = INF pair " + str(i // 2), size=10, color=MUTED)
    c.text(x + 8, 81, "first expiry +1/2 if odd" if i % 2 else "first expiry +1 period", size=9.5, color=MUTED)
    c.arrow([(x + bw / 2, 87), (x + bw / 2, 104)], color=GREY)
c.rect(0, 105, 4 * bw + 24, 30, fill=PAL["plum"][2], stroke=PAL["plum"][1], sw=1.2, rx=6)
c.text(10, 124, "UpdateMonsters(kind): for each of the 30 monsters with updateGroup == kind and hp > 0", size=11, weight=700, color=PAL["plum"][0])

# per-monster steps
steps = [
    ("1", "AlertMonsterToParty", "skips modes 7 to 10. Distance (octagonal) under 4, and the party is not in the monster's rear arc unless under 2: mode = 0, target = party cell. No wall test."),
    ("2", "ChasePartyIfAttacked", "when mode > 0 and the 'attacked' flag is set (and mode is not 10): mode 0 and target = party; a sleeping group (mode 8) wakes together."),
    ("3", "switch (mode)", "0 hunt; 1 to 3 wander; 5, 6 follow a wall; 7, 10 count the timer down; 4, 8, 9 do nothing."),
    ("4", "animPhase ^= 1", "the walk frame flips, except in modes 4 and 7."),
    ("5", "UpdateDDEffects()", "after the loop: flying objects are tested once more for hits."),
]
y = 150
for n, t, b in steps:
    lines_ = wrap(b, 346, 10.2)
    h = 30 + 12 * len(lines_) + 4
    c.rect(0, y, 372, h, fill=PAL["navy"][2], stroke=PAL["navy"][1], sw=1.1, rx=6)
    c.badge(14, y + 15, n, "navy", r=9)
    c.text(32, y + 19, t, size=11.5, weight=700, color=PAL["navy"][0], mono=True)
    yy = y + 33
    for l in lines_:
        c.text(10, yy, l, size=10.2, color=INK2)
        yy += 12
    y += h + 7
# hunting detail
HX = 396
c.rect(HX, 150, 364, 286, fill=PAL["red"][2], stroke=PAL["red"][1], sw=1.2, rx=6)
c.text(HX + 10, 168, "mode 0, hunting (UpdateHuntingMonster)", size=11.5, weight=700, color=PAL["red"][0])
hs = [
    ("a", "TryMonsterRangedAttack", "type bit 0x40, actions left, delay 0; Distance <= 3, the party exactly ahead (code = 2 x facing), every cell between open (flags & 3) and empty: cast, throw or blast"),
    ("b", "TryMonsterMelee", "only when the cell ahead is the party's. In the front half of its cell, else it takes a new quadrant. Swings every other update: ResolveMonsterMelee"),
    ("c", "StepMonsterToward(targetCell)", "turn toward the party one quarter turn at a time when next to it; else step along the direction, trying the eight headings in a fixed order; fleeing (flag 8) reverses it"),
    ("d", "TryMonsterMelee(-1)", "after moving, only for types with flag 8"),
]
y = 178
for n, t, b in hs:
    lines = wrap(b, 316, 10)
    h = 22 + 12 * len(lines)
    c.badge(HX + 16, y + 8, n, "red", r=8)
    c.text(HX + 30, y + 12, t, size=10.5, weight=700, color=PAL["red"][0], mono=True)
    yy = y + 25
    for l in lines:
        c.text(HX + 30, yy, l, size=10, color=INK2)
        yy += 12
    y += h + 6
c.text(HX + 10, 428, "a, b: the first that acts ends the update", size=10, color=PAL["red"][0], weight=600)

# modes table
MY = 462
c.line(0, MY - 12, 760, MY - 12, stroke=RULE)
c.section(0, MY + 2, "THE MODES OF A MONSTER (Monster +8; the level files use 1, 3, 4, 5, 6, 7 and 8)")
modes = [
    ("0", "hunt: head for targetCell"), ("1", "wander: step ahead, else turn around (+2)"), ("2", "wander: step ahead, else turn left (-1)"),
    ("3", "wander: step ahead, else turn right (+1)"), ("4", "stand still"), ("5", "follow the wall on the left"),
    ("6", "follow the wall on the right"), ("7", "wait; timer counts down, then mode 0"), ("8", "asleep with its group: wakes when one is hit or dies"),
    ("9", "ignores the party's approach, fights when hit"), ("10", "held by a hold spell for 15 updates, then mode 0"),
]
col = [0, 380]
yy = MY + 20
for i, (m, d) in enumerate(modes):
    x = col[i // 6]
    y = yy + (i % 6) * 17
    c.badge(x + 10, y - 3, m, "navy", r=9 if len(m) == 1 else 10, size=9.5)
    c.text(x + 26, y, d, size=10.5, color=INK2)
c.save("figs/wld_monai.svg")

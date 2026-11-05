import sys; sys.path.insert(0, '.')
from figlib import *

c = Canvas(760, 470)
c.section(0, 11, "TO-HIT ARITHMETIC")

def card(x, y, w, kind, title, lines, size=11):
    out = []
    for l in lines:
        out += wrap(l, w - 34, size)
    h = 32 + len(out) * size * 1.35 + 6
    c.box(x, y, w, h, kind, title, out, size=size, tsize=12.5, align="left")
    return y + h

yl = card(0, 22, 372, "teal", "A party member attacks (RollToHit, 0DE8:0007)", [
    "need = 20 - progress(level) - AC of the monster - bonus",
    "progress = (level - 1) / levels-per-step * gain, from the member's first class slot (table below)",
    "bonus = the weapon's +N, only for item types 1 to 3, plus the strength bonus (melee) or the dexterity bonus (shot, thrown, spell)",
    "roll = d20, +1 if Bless or Prayer is up, +1 if Aid is on the member; kept in 1 to 20",
    "hit when roll >= need; a need above 20 never hits, a need of 1 or less always does",
])
yl = card(0, yl + 10, 372, "teal", "Damage (RollAttackDamage, 0DE8:011A)", [
    "strength damage bonus (melee only) + weapon dice + weapon's +N; never below 0",
    "dice: the small column of ITEMTYPE.DAT, or the large column when the monster's flag 1 is set",
    "halved for edged weapons against monsters with flag 4 (skeleton kind)",
])
yr = card(388, 22, 372, "red", "A monster attacks (RollMonsterToHit, 08E6:106B)", [
    "need = THAC0 of the monster type - AC of the member",
    "roll = d20; unless it is 20: -2 if the member has Protection from Evil, -1 if Prayer is up",
    "hit when roll >= need, or the roll was 20",
    "each of the type's 1 to 3 attacks rolls on its own; the dice (NdS+B) of the hits are added",
])
yr = card(388, yr + 10, 372, "red", "What follows (ResolveMonsterMelee, 08E6:10F3)", [
    "Stoneskin: for every hit that lands while the counter is above 0, the counter drops by 1 and the total so far is thrown away",
    "one DamageCharacter with the total; then poison, paralysis or an item lost, by the type's flags",
])

# progression grid
gy = max(yl, yr) + 18
c.text(0, gy, "progress by combat group: the number a level-n member starts from (20 - progress), before AC and bonus", size=11.5, weight=700, color=INK)
groups = [
    ("group 0", "fighter, ranger, paladin, F/x", [20, 20, 20, 18, 18, 18, 16, 16, 16, 14, 14]),
    ("group 1", "mage", [20, 20, 19, 19, 18, 18, 17, 17, 16, 16, 15]),
    ("group 2", "cleric, C/x", [20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10]),
    ("group 3", "thief, T/M", [20, 20, 20, 19, 19, 19, 18, 18, 18, 17, 17]),
]
gx0, cw = 250, 41
hy = gy + 12
for i in range(11):
    c.text(gx0 + i * cw + cw / 2, hy + 10, str(i + 1), size=11, weight=700, color=MUTED, anchor="middle")
c.text(gx0 - 6, hy + 10, "level", size=10.5, color=MUTED, anchor="end")
for r, (g, cls, vals) in enumerate(groups):
    y = hy + 18 + r * 24
    c.text(0, y + 15, g, size=11.5, weight=700, color=INK)
    c.text(52, y + 15, cls, size=10.5, color=MUTED)
    for i, v in enumerate(vals):
        shade = {20: "#F4F0E6", 19: "#E3C98F", 18: "#E3C98F", 17: "#E3B2A5"}.get(v, "#DCEBE7" if v <= 16 else "#F4F0E6")
        t = (20 - v)
        fill = "#FFFDF8" if t == 0 else MID["teal"] if t <= 3 else "#7DBBB0" if t <= 6 else SOLID["teal"]
        c.rect(gx0 + i * cw, y, cw - 2, 21, fill=fill, stroke="none", rx=2)
        c.text(gx0 + i * cw + (cw - 2) / 2, y + 15, str(v), size=11.5, weight=600, color="#FFFFFF" if t > 6 else INK, anchor="middle")
yy = hy + 18 + 4 * 24 + 8
yy = c.para(0, yy + 8, "The four rates (2 per 3 levels, 1 per 2, 1 per 1, 1 per 3) are AD&D's priest, rogue, warrior and wizard rates [H]. The game hands them out in that order, so a fighter improves like a priest and a cleric like a warrior [I]. Class to group: ds:116E; gain: ds:3610; levels per step: ds:3614.", 756, size=10.5, color=MUTED)
c.h = yy + 6
c.save("figs/cmb_tohit.svg")

import sys; sys.path.insert(0, '.')
from figlib import *

c = Canvas(760, 520)
c.section(0, 11, "THE 49 SPELLS BY CLASS AND LEVEL")

CAT = {"dmg": "red", "heal": "teal", "buff": "navy", "ctl": "plum", "util": "ochre", "none": "grey"}
mage = {
    1: [(1, "Armor", "buff"), (2, "Burning Hands", "dmg"), (3, "Detect Magic", "util"), (4, "Magic Missile", "dmg"),
        (5, "Read Magic", "none"), (6, "Shield", "buff"), (7, "Shocking Grasp", "dmg")],
    2: [(8, "Invisibility", "buff"), (9, "Knock", "none"), (10, "Melf's Acid Arrow", "dmg"), (11, "Stinking Cloud", "none")],
    3: [(12, "Dispel Magic", "util"), (13, "Fireball", "dmg"), (14, "Flame Arrow", "dmg"), (15, "Haste", "buff"),
        (16, "Hold Person", "ctl"), (17, "Invisibility 10'", "buff"), (18, "Lightning Bolt", "dmg"), (19, "Vampiric Touch", "dmg")],
    4: [(20, "Fear", "ctl"), (21, "Ice Storm", "dmg"), (22, "Stoneskin", "buff")],
    5: [(23, "Cloudkill", "none"), (24, "Cone of Cold", "dmg"), (25, "Hold Monster", "ctl")],
}
cleric = {
    1: [(1, "Bless", "buff"), (2, "Cure Light Wounds", "heal"), (3, "Cause Light Wounds", "dmg"), (4, "Detect Magic", "util"),
        (5, "Protect. from Evil", "buff"), (24, "Lay on Hands", "heal")],
    2: [(6, "Aid", "buff"), (7, "Flame Blade", "dmg"), (8, "Hold Person", "ctl"), (9, "Slow Poison", "heal")],
    3: [(10, "Create Food", "util"), (11, "Dispel Magic", "util"), (12, "Magical Vestment", "buff"), (13, "Prayer", "buff"),
        (14, "Remove Paralysis", "heal")],
    4: [(15, "Cure Serious Wounds", "heal"), (16, "Cause Serious Wounds", "dmg"), (17, "Neutralize Poison", "heal"),
        (18, "Prot. from Evil 10'", "buff"), (19, "Prot. from Lightning", "none")],
    5: [(20, "Cure Critical Wounds", "heal"), (21, "Cause Critical Wounds", "dmg"), (22, "Flame Strike", "dmg"), (23, "Raise Dead", "heal")],
}
paladin = {2, 5, 6, 7, 8, 9, 10, 11, 12, 24}
CW, GAP = 146, 7
PH = 22

def block(y, title, data, pal=False):
    c.text(0, y, title, size=12, weight=700, color=INK)
    for lv in range(1, 6):
        x = (lv - 1) * (CW + GAP)
        c.text(x, y + 18, f"level {lv}", size=10.5, weight=700, color=MUTED)
        for i, (n, name, cat) in enumerate(data[lv]):
            yy = y + 24 + i * (PH + 4)
            tc, sc, fc = PAL[CAT[cat]]
            dash = "3 2" if cat == "none" else None
            c.rect(x, yy, CW, PH, fill=fc, stroke=sc, sw=1, rx=4, dash=dash)
            c.text(x + 6, yy + 15, str(n), size=10, weight=700, color=tc, mono=True)
            c.text(x + 24, yy + 15, name, size=10.5, color=INK, mono=False)
            if pal and n in paladin:
                c.circle(x + CW - 9, yy + PH / 2, 6.5, fill=SOLID["ochre"])
                c.text(x + CW - 9, yy + PH / 2 + 3.6, "P", size=9, weight=700, color="#FFFFFF", anchor="middle")
    return y + 24 + max(len(v) for v in data.values()) * (PH + 4)

y = block(34, "Mage: 25 spells (slots 1 to 5 per level in the spell book)", mage)
y = block(y + 12, "Cleric: 24 spells; P marks the spells a paladin may pray for (from level 9)", cleric, pal=True)
c.legend(0, y + 14, [("red", "damage"), ("teal", "heal, cure"), ("navy", "buff"), ("plum", "hold, fear"), ("ochre", "utility"), ("grey", "no effect", "3 2")])
yy = c.para(0, y + 34, "Dashed: spells with no effect. Read Magic and Knock have an all-zero gSpells record (a null callback); Stinking Cloud, Cloudkill and Protection from Lightning have an empty one. Protection from Lightning is not in the cleric's prayer list (mask 7BFFFFh); Lay on Hands only in the paladin's.", 756, size=10.5, color=MUTED)
c.h = yy + 4
c.save("figs/cmb_spells.svg")

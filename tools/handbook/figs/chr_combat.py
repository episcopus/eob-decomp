import sys; sys.path.insert(0, '.')
from figlib import *

c = Canvas(760, 520)
c.section(0, 11, "How armour class and the to-hit numbers meet (both are 'roll a d20, reach a number')")

def panel(x, y, w, h, kind, title):
    tc, sc, fc = PAL[kind]
    c.rect(x, y, w, h, fill=fc, stroke=sc, sw=1.2, rx=6)
    c.text(x + 10, y + 18, title, size=12.5, weight=700, color=tc)

def line(x, y, s, mono=False, color=INK2, size=10.5, weight=400):
    c.text(x, y, s, size=size, color=color, mono=mono, weight=weight)
    return y + size * 1.4

# ---- left: character attacks ----
LX, LW, TY, TH = 0, 372, 28, 228
panel(LX, TY, LW, TH, "navy", "Character hits monster: RollToHit (0DE8:0007)")
y = TY + 38
y = line(LX + 10, y, "need = GetHitTarget - bonus", mono=True, weight=600, size=11)
y = line(LX + 18, y + 2, "GetHitTarget = 20", mono=True, size=10.5)
y = line(LX + 18, y, "  - (levels[0] - 1) / levelsPerStep * gainPerStep", mono=True, size=10)
y = line(LX + 18, y, "  - the monster's armour byte (type +0)", mono=True, size=10)
y = line(LX + 18, y + 2, "bonus = item enchantment (item types 1 to 3)", mono=True, size=10)
y = line(LX + 18, y, "      + STR to-hit (melee) or DEX missile (thrown, shot)", mono=True, size=10)
y = line(LX + 10, y + 8, "roll = d20, +1 if Bless or Prayer is active (once), +1 if Aid,", size=10.5)
y = line(LX + 10, y, "clamped to 1..20; the hit lands when roll >= need.", size=10.5)
y = line(LX + 10, y + 8, "Fighter group improves 2 points per 3 levels (levelsPerStep 3,", size=10, color=MUTED)
y = line(LX + 10, y, "gainPerStep 2); mage 1 per 2, cleric 1 per level, thief 1 per 3.", size=10, color=MUTED)
y = line(LX + 10, y, "Only the level of class slot 0 counts, also for multi-classes.", size=10, color=MUTED)

# ---- right: monster attacks ----
RX, RW = 388, 372
panel(RX, TY, RW, TH, "red", "Monster hits character: RollMonsterToHit (08E6:106B)")
y = TY + 38
y = line(RX + 10, y, "need = monster to-hit (type +1) - armorClass", mono=True, weight=600, size=11)
y = line(RX + 10, y + 6, "roll = d20. Unless the roll is 20:", size=10.5)
y = line(RX + 18, y, "-2 if the target has Protection from Evil (effect flag 200h)", size=10.5)
y = line(RX + 18, y, "-1 if Prayer is active (party flag 20h)", size=10.5)
y = line(RX + 10, y + 2, "hit when roll >= need, and a 20 always hits.", size=10.5)
y = line(RX + 10, y + 8, "damage per hit = RollDice of one of the type's up to three", size=10.5)
y = line(RX + 10, y, "attack routines {count, sides, bonus} (type +4).", size=10.5)
y = line(RX + 10, y + 8, "Because need falls as armorClass rises, a low armorClass is", size=10, color=MUTED)
y = line(RX + 10, y, "good: armorClass 10 is unarmoured, and each armour point", size=10, color=MUTED)
y = line(RX + 10, y, "lowers it by one.", size=10, color=MUTED)

# ---- bottom: how armorClass is built ----
BY = 276
c.section(0, BY - 6, "ComputeArmourClass (0DE8:0D5E) builds armorClass, then three spells cap it")
steps = [
    ("teal", "start", "gDexArmourAdjust[DEX] + 10"),
    ("teal", "slot 17", "body armour: type m04 (-2 to -7) minus enchantment"),
    ("teal", "slots 0, 1", "only a shield (type 27): m04 -1, minus enchantment"),
    ("teal", "slot 18", "bracers: m04 0, minus enchantment"),
    ("ochre", "rings 25, 26", "if slot 17's enchantment is 0: minus the better ring of protection"),
]
w = 138
for i, (k, t, b) in enumerate(steps):
    x = i * (w + 17)
    c.box(x, BY + 6, w, 76, k, t, [b], size=10, tsize=11.5)
    if i:
        c.arrow([(x - 17, BY + 44), (x, BY + 44)], color=GREY)
c.text(0, BY + 104, "then, in this order:", size=10.5, weight=700, color="#14504A")
caps = [
    ("Armor (mDF > 0)", "armorClass cut to base + 6 if higher; base = gDexArmourAdjust[DEX]"),
    ("Shield (flag 2)", "armorClass cut to 4 if higher"),
    ("Magical Vestment (flag 100h)", "cut to 5, or 5 + (level - 5) / 3 for a cleric above level 5, if higher"),
]
for i, (t, b) in enumerate(caps):
    x = i * 252
    c.box(x, BY + 112, 240, 52, "red", t, [b], size=10, tsize=11.5)
c.text(0, BY + 186, "A helmet (slot 19) is not counted: the loop walks four slots, 17, 0, 1 and 18, although the table after them also holds 19.", size=10.5, color="#80311F")
c.save("figs/chr_combat.svg")

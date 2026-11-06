import sys; sys.path.insert(0, '.')
from figlib import *

c = Canvas(760, 470)
c.section(0, 11, "RollCharacterStats (ovl0:1BF2)")

# ---- left: the per-score pipeline ----
LX, LW = 0, 420
steps = [
    ("navy", "1  RollAbilityScore", "four d6, drop the lowest: 3 to 18 (ovl0:1177)"),
    ("navy", "2  race adjustment", "add gRaceAbilityBonus[race / 2][i]: elf DEX +1 CON -1, dwarf CON +1 CHA -1, gnome INT +1 WIS -1, halfling STR -1 DEX +1"),
    ("navy", "3  raise to the floor", "gStatMin[i] = larger of gClassMinAbilities[class][i] and gRaceMinAbilities[race / 2][i]"),
    ("ochre", "4  strength only", "a score of exactly 18 gets a percentile: d100 in the high byte (18/01 to 18/00)"),
    ("navy", "5  lower to the ceiling", "gStatMax[i] = gRaceMaxAbilities[race / 2][i]; strength is held at 18 when gClassCombatGroup[class] is not 0"),
    ("teal", "6  store", "current and base bytes, two per ability (offsets 0D to 1A)"),
]
y = 30
hs = [44, 62, 56, 48, 62, 40]
ys = []
for (k, t, b), h in zip(steps, hs):
    tc, sc, fc = PAL[k]
    c.rect(LX, y, LW, h, fill=fc, stroke=sc, sw=1.2, rx=6)
    c.text(LX + 10, y + 17, t, size=12.5, weight=700, color=tc)
    yy = y + 32
    for l in wrap(b, LW - 20, 10.5):
        c.text(LX + 10, yy, l, size=10.5, color=INK2)
        yy += 13
    ys.append((y, h))
    y += h + 14
for i in range(len(ys) - 1):
    y0 = ys[i][0] + ys[i][1]
    c.arrow([(LX + LW / 2, y0), (LX + LW / 2, y0 + 14)], color=GREY)
c.text(LX, y + 8, "Steps 1 to 5 run for the six scores in the order STR, INT, WIS, DEX, CON, CHA.", size=10.5, color=MUTED)

# ---- right: derived values ----
RX, RW = 450, 310
c.section(RX, 11, "Derived from the stored scores")
c.box(RX, 30, RW, 84, "red", "armour class", ["armorClass = gDexArmourAdjust[DEX] + 10.", "Lower is better; DEX 15 or more subtracts, DEX 6 or less adds (ds:349E)."], size=10.5, tsize=12.5)
c.box(RX, 128, RW, 128, "red", "hit points: RollHitPoints (0A72:01AE)", [
    "per class slot: levels[slot] dice of that slot's die, plus a CON bonus per die;",
    "the bonus is gConHitPointBonus[CON - 1], capped at +2 unless slot 0 is a d10;",
    "the sum is divided by 3 for three classes, by 2 for two."], size=10.5, tsize=12.5)
c.box(RX, 268, RW, 100, "ochre", "levels and experience", [
    "levels[i] = gClassStartLevels[class][i].",
    "experience[i] = 5000 / number of classes, for all three slots (even the unused ones)."], size=10.5, tsize=12.5)
c.box(RX, 380, RW, 56, "paper", "editing afterwards", ["EditStatField moves each score inside gStatMin and gStatMax, so the roll is only a start."], size=10.5, tsize=12)
c.save("figs/chr_abilities.svg")

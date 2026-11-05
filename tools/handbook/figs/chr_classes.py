import sys; sys.path.insert(0, '.')
from figlib import *

# The 15 classes as the class-slot tables see them (gClassSlotBaseClass ds:35D4, gHitDieSlot0/1/2 ds:174B/175A/1769,
# gClassSlotCount ds:115F, gClassStartLevels ds:5EF9, gClassCombatGroup ds:116E, gClassBaseBits ds:1231).
NAMES = ["FIGHTER", "RANGER", "PALADIN", "MAGE", "CLERIC", "THIEF", "FIGHTER/CLERIC", "FIGHTER/THIEF", "FIGHTER/MAGE",
         "FIGHTER/MAGE/THIEF", "THIEF/MAGE", "CLERIC/THIEF", "FIGHTER/CLERIC/MAGE", "RANGER/CLERIC", "CLERIC/MAGE"]
BASE = [[0], [5], [4], [1], [2], [3], [0, 2], [0, 3], [0, 1], [0, 1, 3], [3, 1], [2, 3], [0, 2, 1], [5, 2], [2, 1]]
HD0 = [10, 10, 10, 4, 8, 6, 10, 10, 10, 10, 4, 8, 10, 10, 8]
HD1 = [0, 0, 0, 0, 0, 0, 8, 6, 4, 4, 6, 6, 8, 8, 4]
HD2 = [0, 0, 0, 0, 0, 0, 0, 0, 0, 6, 0, 0, 4, 0, 0]
SPLIT = [1, 1, 1, 1, 1, 1, 2, 2, 2, 3, 2, 2, 3, 2, 2]
START = [[3, 0, 0], [3, 0, 0], [3, 0, 0], [3, 0, 0], [3, 0, 0], [4, 0, 0], [2, 2, 0], [2, 3, 0], [2, 2, 0], [1, 1, 2],
         [3, 2, 0], [2, 3, 0], [1, 2, 1], [2, 2, 0], [2, 2, 0]]
GROUP = [0, 0, 0, 1, 2, 3, 0, 0, 0, 0, 3, 2, 0, 0, 2]
BITS = [1, 1, 5, 2, 4, 8, 5, 9, 3, 11, 10, 12, 7, 5, 6]
LET = "FMCTPR"
KIND = ["teal", "navy", "ochre", "plum", "red", "grey"]
RH = 25
Y0 = 56
H = Y0 + 15 * RH + 60
c = Canvas(760, H)
c.section(0, 11, "The 15 classes in the class-slot tables")

cols = {"idx": 6, "name": 28, "slots": 186, "dice": 302, "split": 404, "start": 468, "group": 572, "bits": 664}
hdr = [("class", cols["name"], "start"), ("slots (base class)", cols["slots"], "start"), ("hit die per slot", cols["dice"], "start"),
       ("XP split", cols["split"], "start"), ("levels at creation", cols["start"], "start"),
       ("combat group", cols["group"], "start"), ("class bits", cols["bits"], "start")]
c.rect(0, 22, 760, 26, fill=PANEL, stroke="none", rx=4)
c.text(cols["idx"], 39, "#", size=10.5, weight=700, color=MUTED)
for t, x, a in hdr:
    c.text(x, 39, t, size=10.5, weight=700, color=MUTED)

for i in range(15):
    y = Y0 + i * RH
    if i % 2 == 1:
        c.rect(0, y - 4, 760, RH, fill="#F8F4EA", stroke="none", rx=0)
    c.text(cols["idx"], y + 12, str(i), size=10.5, color=MUTED, mono=True)
    c.text(cols["name"], y + 12, NAMES[i], size=11.5, weight=600, color=INK)
    for s in range(3):
        x = cols["slots"] + s * 30
        if s < len(BASE[i]):
            b = BASE[i][s]
            tc, sc, fc = PAL[KIND[b]]
            c.rect(x, y - 1, 24, 18, fill=fc, stroke=sc, sw=1, rx=3)
            c.text(x + 12, y + 12, LET[b], size=11, weight=700, color=tc, anchor="middle")
        else:
            c.rect(x, y - 1, 24, 18, fill="none", stroke=RULE, sw=1, rx=3, dash="2 2")
    dice = [HD0[i], HD1[i], HD2[i]]
    for s in range(3):
        x = cols["dice"] + s * 40
        if s < len(BASE[i]):
            swapped = (i == 10)
            c.text(x, y + 12, "d%d" % dice[s], size=11.5, color=("#A8452F" if swapped else INK2), weight=(700 if swapped else 400), mono=True)
        else:
            c.text(x, y + 12, "-", size=11.5, color=MUTED, mono=True)
    c.text(cols["split"] + 8, y + 12, str(SPLIT[i]), size=11.5, color=INK2, mono=True)
    c.text(cols["start"], y + 12, "  ".join(str(v) if s < len(BASE[i]) else "-" for s, v in enumerate(START[i])), size=11.5, color=INK2, mono=True)
    c.text(cols["group"] + 28, y + 12, str(GROUP[i]), size=11.5, color=INK2, mono=True)
    for k, (bit, lab, kd) in enumerate([(1, "F", "teal"), (2, "M", "navy"), (4, "C", "ochre"), (8, "T", "plum")]):
        x = cols["bits"] + k * 22
        tc, sc, fc = PAL[kd]
        if BITS[i] & bit:
            c.rect(x, y - 1, 18, 18, fill=fc, stroke=sc, sw=1, rx=3)
            c.text(x + 9, y + 12, lab, size=10.5, weight=700, color=tc, anchor="middle")
        else:
            c.rect(x, y - 1, 18, 18, fill="none", stroke=RULE, sw=1, rx=3)

yy = Y0 + 15 * RH + 4
c.legend(0, yy + 6, [("teal", "F fighter"), ("navy", "M mage"), ("ochre", "C cleric"), ("plum", "T thief"), ("red", "P paladin"), ("grey", "R ranger")], size=10.5)
c.text(0, yy + 24, "THIEF/MAGE rolls d4 for slot 0 (thief levels) and d6 for slot 1 (mage levels): the dice are in the other order than the slots (red).", size=10.5, color="#A8452F")
c.save("figs/chr_classes.svg")

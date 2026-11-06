import sys; sys.path.insert(0, '.')
from figlib import *

c = Canvas(760, 440)
c.section(0, 11, "Saving throws: GetSavingThrowTarget (0A72:0006)")

# --- formula ---
c.rect(0, 26, 410, 118, fill=PAL["navy"][2], stroke=PAL["navy"][1], sw=1.2, rx=6)
c.text(10, 44, "index  = min(level, max) / step + rowLength * kind", size=11, weight=700, color=PAL["navy"][0], mono=True)
c.text(10, 60, "target = gSaveTables[base][index]", size=11, weight=700, color=PAL["navy"][0], mono=True)
c.text(10, 78, "base: the base class of class slot 0 (paladin and ranger count as fighter)", size=10.5, color=INK2)
c.text(10, 92, "level: levels[0], the first class slot only", size=10.5, color=INK2)
c.text(10, 106, "kind: the row, 0 to 4 (poison and death 0, paralysis 2, spell damage 4)", size=10.5, color=INK2)
c.text(10, 120, "success: a d20 roll of at least target", size=10.5, color=INK2)
c.text(10, 134, "dwarf, gnome, halfling: a constitution bonus is subtracted (see text)", size=10.5, color=INK2)

# --- parameter table ---
c.text(430, 40, "per base class (ds:341E, ds:3424, ds:342A)", size=10.5, weight=700, color=MUTED)
hdr = ["base", "max", "step", "row length", "table"]
hx = [430, 490, 530, 575, 650]
for t, x in zip(hdr, hx):
    c.text(x, 58, t, size=10.5, weight=700, color=PAL["teal"][0])
rows = [("fighter", 17, 2, 10, "0x33BB"), ("mage", 21, 5, 5, "0x33ED"), ("cleric", 19, 3, 7, "0x337A"), ("thief", 21, 4, 6, "0x339D"),
        ("paladin", 17, 2, 10, "= fighter"), ("ranger", 17, 2, 10, "= fighter")]
for i, r in enumerate(rows):
    y = 76 + i * 15
    for v, x in zip(r, hx):
        c.text(x, y, str(v), size=10.5, color=INK2, mono=(x > 480))

# --- matrices ---
MY = 190
c.section(0, MY - 14, "the four tables, kind by column (values read from the image)")
T = {
    "fighter": ([[16, 14, 13, 11, 10, 8, 7, 5, 4, 3], [18, 16, 15, 13, 12, 10, 9, 7, 6, 5], [17, 15, 14, 12, 11, 9, 8, 6, 5, 4],
                 [20, 17, 16, 13, 12, 9, 8, 5, 4, 4], [19, 17, 16, 14, 13, 11, 10, 8, 7, 6]], "teal", "level / 2"),
    "mage": ([[14, 13, 11, 10, 8], [11, 9, 7, 5, 3], [13, 11, 9, 7, 5], [15, 13, 11, 9, 7], [12, 10, 8, 6, 4]], "navy", "level / 5"),
    "cleric": ([[10, 9, 7, 6, 5, 4, 2], [14, 13, 11, 10, 9, 8, 6], [13, 12, 10, 9, 8, 7, 5], [16, 15, 13, 12, 11, 10, 8], [15, 14, 12, 11, 10, 9, 7]], "ochre", "level / 3"),
    "thief": ([[13, 12, 11, 10, 9, 8], [14, 12, 10, 8, 6, 7], [12, 11, 10, 9, 8, 4], [16, 15, 14, 13, 12, 11], [15, 13, 11, 9, 7, 5]], "plum", "level / 4"),
}
kinds = ["0 poison, death", "1 rod, staff, wand", "2 petrify, polymorph", "3 breath", "4 spells"]
CW, CH = 19, 20
x = 118
top = MY + 34
for i, k in enumerate(kinds):
    c.text(0, top + i * CH + 14, k, size=10.5, color=INK2)
for name in ["fighter", "mage", "cleric", "thief"]:
    rowsv, kd, note = T[name]
    tc, sc, fc = PAL[kd]
    ncol = len(rowsv[0])
    c.text(x, top - 8, name, size=11, weight=700, color=tc)
    for r in range(5):
        for j in range(ncol):
            hl = (name == "fighter" and r == 4 and j == 2)
            c.rect(x + j * CW, top + r * CH, CW, CH, fill=(SOLID["red"] if hl else fc), stroke=sc, sw=0.8, rx=0)
            c.text(x + j * CW + CW / 2, top + r * CH + 14, str(rowsv[r][j]), size=10, color=("#FFFFFF" if hl else tc), anchor="middle", weight=(700 if hl else 400), mono=True)
    c.text(x, top + 5 * CH + 14, "column = " + note, size=10, color=MUTED)
    x += ncol * CW + 16
c.text(0, top + 5 * CH + 46, "Example: a fighter of level 5 saves against spell damage (kind 4) with index 5 / 2 + 10 * 4 = 42, the value 16 (red): a d20 roll of 16 or more.", size=10.5, color="#80311F")
c.text(0, top + 5 * CH + 62, "The values agree with the AD&D first edition saving throw matrices for the same classes [H: recalled, not from the files].", size=10.5, color=MUTED)
c.save("figs/chr_save.svg")

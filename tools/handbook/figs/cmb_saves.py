import sys; sys.path.insert(0, '.')
from figlib import *

c = Canvas(760, 520)
c.section(0, 11, "SAVING THROWS: ds:3406 TABLES, success on a d20 at or above the number")

tables = {
    "fighter (also paladin, ranger, monsters)": (2, 17, [
        [16, 14, 13, 11, 10, 8, 7, 5, 4, 3], [18, 16, 15, 13, 12, 10, 9, 7, 6, 5],
        [17, 15, 14, 12, 11, 9, 8, 6, 5, 4], [20, 17, 16, 13, 12, 9, 8, 5, 4, 4],
        [19, 17, 16, 14, 13, 11, 10, 8, 7, 6]]),
    "mage": (5, 21, [[14, 13, 11, 10, 8], [11, 9, 7, 5, 3], [13, 11, 9, 7, 5], [15, 13, 11, 9, 7], [12, 10, 8, 6, 4]]),
    "cleric": (3, 19, [[10, 9, 7, 6, 5, 4, 2], [14, 13, 11, 10, 9, 8, 6], [13, 12, 10, 9, 8, 7, 5],
                       [16, 15, 13, 12, 11, 10, 8], [15, 14, 12, 11, 10, 9, 7]]),
    "thief": (4, 21, [[13, 12, 11, 10, 9, 8], [14, 12, 10, 8, 6, 7], [12, 11, 10, 9, 8, 4], [16, 15, 14, 13, 12, 11], [15, 13, 11, 9, 7, 5]]),
}
CW = 25

def colour(v):
    if v >= 17: return "#F5E0D9", INK
    if v >= 14: return "#F4E8CC", INK
    if v >= 11: return "#E3C98F", INK
    if v >= 8: return "#A9D0C8", INK
    if v >= 5: return "#5FA89B", "#FFFFFF"
    return "#1F6F66", "#FFFFFF"

def draw(x, y, title, step, mx, data):
    c.text(x, y, title, size=11.5, weight=700, color=INK)
    n = len(data[0])
    # column headers: level range
    for j in range(n):
        lo = j * step
        hi = min(mx, lo + step - 1)
        lab = f"{lo}-{hi}" if hi > lo else f"{lo}"
        dead = (mx // step) < j
        if dead:
            lab = "never"
        c.text(x + 18 + j * CW + CW / 2 - 1, y + 14, lab, size=8.5, color=MUTED, anchor="middle")
    for k in range(5):
        c.text(x + 8, y + 34 + k * 19, str(k), size=10.5, weight=700, color=MUTED, anchor="middle")
        for j in range(n):
            f, tcol = colour(data[k][j])
            if (mx // step) < j:
                f, tcol = "#ECE7DD", MUTED
            c.rect(x + 18 + j * CW, y + 21 + k * 19, CW - 2, 17, fill=f, stroke="none", rx=2)
            c.text(x + 18 + j * CW + CW / 2 - 1, y + 34 + k * 19, str(data[k][j]), size=10.5, weight=600, color=tcol, anchor="middle")
    return y + 21 + 5 * 19

y1 = 26
draw(0, y1, "fighter (also paladin, ranger, monsters)", *tables["fighter (also paladin, ranger, monsters)"])
draw(300, y1, "mage", *tables["mage"])
draw(470, y1, "cleric", *tables["cleric"])
y2 = 160
draw(0, y2, "thief", *tables["thief"])

# the lookup formula, to the right of the thief table
fx = 300
c.text(fx, y2, "The lookup (GetSavingThrowTarget, 0A72:0006)", size=11.5, weight=700, color=INK)
for i, l in enumerate([
    "column = min(level, cap) / levels-per-column",
    "number = table[kind * columns + column]",
    "member: table of its first class, level of its first slot",
    "monster: fighter table, hit dice as the level, kind 4",
    "paladin and ranger use the fighter table (RollMemberSave)",
]):
    c.text(fx, y2 + 18 + i * 15, l, size=11, color=INK2, mono=(i < 2))

# kinds
ky = 290
c.text(0, ky, "Kind (row), as the game calls it:", size=11.5, weight=700, color=INK)
kinds = [("0", "poison (type flag 16, poison potion)"), ("1", "psychic mind blast"), ("2", "paralysing touch (type flag 32)"),
         ("3", "not used by any call found"), ("4", "save-for-half spell damage, Fear")]
for i, (k, t) in enumerate(kinds):
    c.text(0, ky + 18 + i * 15, k, size=11, weight=700, color=PAL["navy"][0])
    c.text(14, ky + 18 + i * 15, t, size=11, color=INK2)
c.para(380, ky + 2, "AD&D 2nd edition names the five rows paralysis/poison/death, rod/staff/wand, petrification/polymorph, breath weapon, spell [H]. The fighter table has a tenth column that no level reaches (its cap is 17, 17 / 2 = 8).", 370, size=10.5, color=MUTED)

# quirks
qy = 392
c.rect(0, qy, 760, 112, fill=PANEL, stroke="none", rx=8)
c.text(12, qy + 20, "Three things the code does that the tables do not suggest", size=12, weight=700, color=PAL["red"][0])
qs = [
    "HalveSaveDamageA (0DE8:0F98) asks RollMemberSave for 0 or 1 and then tests d20 >= that, which always holds: a party member always takes half from a save-for-half effect.",
    "The same test makes DamageOneMember's save (1 >> 1 = 0) fail every time: the first eligible member is hit for 100.",
    "RollMemberSave looks written to give a Constitution bonus to dwarves, halflings and gnomes (AD&D's rule) [I]; it tests race * 2 against 3, 5 and 4, so the bonus goes only to male elves (race index 2).",
]
yy = qy + 38
for q in qs:
    yy = c.para(14, yy, q, 730, size=10.8, color=INK2) + 3
c.h = 508
c.save("figs/cmb_saves.svg")

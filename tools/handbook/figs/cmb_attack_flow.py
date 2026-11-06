import sys; sys.path.insert(0, '.')
from figlib import *

c = Canvas(760, 560)

def tbox(x, y, w, kind, title, lines, size=11, align="left", minh=0):
    out = []
    for l in lines:
        out += wrap(l, w - 36, size)
    h = max(minh, 10 + 17 + len(out) * size * 1.3 + 8)
    c.box(x, y, w, h, kind, title, out, size=size, tsize=12, align=align)
    return y + h

c.section(0, 11, "A PARTY ATTACK, FROM THE CLICK TO THE DAMAGE")

# column A
xa, wa = 0, 232
y = 24
y1 = tbox(xa, y, wa, "navy", "ClickHandSlot", ["0ACD:0415: one mouse button swaps the item with the one on the pointer, the other uses it."])
y2 = tbox(xa, y1 + 18, wa, "navy", "UseItemSlot", ["0BD2:1001. Needs MemberIs(who, 13): present, hp above 0, not paralysed. The hand must not be busy; the class must be allowed to use the item."])
c.arrow([(xa + wa / 2, y1), (xa + wa / 2, y1 + 18)])
y3 = tbox(xa, y2 + 18, wa, "ochre", "Use kind: ItemType +0E & 7F", [
    "1  melee weapon",
    "2  thrown weapon (dagger, spear, dart, rock)",
    "3  launcher (bow, sling)",
    "5, 6  spell book, holy symbol",
    "9, 10  scroll;  18  wand;  14  potion;  7  food"])
c.arrow([(xa + wa / 2, y2), (xa + wa / 2, y2 + 18)])

# column B: ApplyItemAttack
xb, wb = 262, 250
yb = tbox(xb, 24, wb, "teal", "1: ApplyItemAttack  0DE8:0359", [
    "who above 1 (back rank)?  result -3, CAN'T REACH",
    "cell ahead = StepCell(party cell, facing)",
    "FindMeleeTarget (see the cell figure)",
    "no target: a hackable wall gets its wall byte raised, -2 HACK; else -1",
    "RollToHit (0DE8:0007); a miss is -1, MISS",
    "spell-made weapon and MonsterResistsMagic: -1",
    "RollAttackDamage (0DE8:011A); half when IsWeaponDamageHalved",
    "result = damage"], size=10.5)
yb2 = tbox(xb, yb + 14, wb, "teal", "2, 3: thrown, shot  0DE8:0537, 05BB", [
    "thrown: the weapon itself leaves the hand",
    "launcher: first arrow (quiver, slot 16) or rock found",
    "none found: result -4, NO AMMO",
    "AddDDEffect puts the item into one of the 10 projectile records and onto the cell's item list; result 0"], size=10.5)

# column C: UseItem
xc, wc = 542, 218
yc = tbox(xc, 24, wc, "plum", "UseItem  0DE8:0763", [
    "ends invisibility if set",
    "calls the handler above",
    "melee result 0 becomes -1",
    "result above 0: item flag 8 heals the wielder by the damage (cap 127); flag 16 uses the item up; DamageMonster(target, result)",
    "lastAction[slot] = result (the label: damage, MISS, HACK, CAN'T REACH, NO AMMO)",
    "busy bit of the hand flipped"], size=10.5)
yc2 = tbox(xc, yc + 14, wc, "red", "Cooldown", [
    "ApplyCharEffect(who, 18 ticks, slot + 2): type 2 or 3, then 36 ticks (18 hasted) to type 4 or 5",
    "result below -2: type slot, 18 ticks only"], size=10.5)
c.arrow([(xc + wc / 2, yc), (xc + wc / 2, yc + 14)])

# connectors
yk = y2 + 18
c.arrow([(xa + wa, yk + 24), (xb - 16, yk + 24), (xb - 16, 150), (xb, 150)], label="1", lpos=0.12, ldy=-4, ldx=6)
c.arrow([(xa + wa, yk + 70), (xb - 8, yk + 70), (xb - 8, yb + 50), (xb, yb + 50)], label="2, 3", lpos=0.12, ldy=-4, ldx=6)
c.arrow([(xb + wb, 90), (xc, 90)])
c.arrow([(xb + wb, yb + 60), (xb + wb + 14, yb + 60), (xb + wb + 14, 130), (xc, 130)])
c.h = max(yc2, yb2, y3) + 14
c.save("figs/cmb_attack_flow.svg")

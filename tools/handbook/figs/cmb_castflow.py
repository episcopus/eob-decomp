import sys; sys.path.insert(0, '.')
from figlib import *

c = Canvas(760, 470)
c.section(0, 11, "CASTING: FROM THE ITEM IN THE HAND TO THE SPELL'S HANDLER")

def tbox(x, y, w, kind, title, lines, size=10.5, h=None):
    out = []
    for l in lines:
        out += wrap(l, w - 32, size)
    hh = h or (10 + 17 + len(out) * size * 1.3 + 8)
    c.box(x, y, w, hh, kind, title, out, size=size, tsize=12, align="left")
    return y + hh

# sources
xa, wa = 0, 214
a1 = tbox(xa, 24, wa, "navy", "Spell book or holy symbol", ["use kind 5 or 6: UseItemSlot opens OpenSpellPanel(who, cleric) 0EF3:21E0; five level tabs, six slots, ABORT", "click a slot: HandleSpellList 0EF3:2929 calls CastSpell(slot value, +25 for clerics)"])
a2 = tbox(xa, a1 + 12, wa, "navy", "Scroll", ["use kind 9 (mage) or 10 (cleric): StartItemCast(who, value, hand) 0EF3:28ED; the caster level counts as 9"])
a3 = tbox(xa, a2 + 12, wa, "navy", "Wand", ["use kind 18: UseWandItem 0BD2:0EE8; value 1 to 6 maps to a spell through gWandSpells, then StartItemCast; the Silvias wand (5) moves monsters instead"])
# CastSpell
xb, wb = 244, 232
b1 = tbox(xb, 24, wb, "teal", "CastSpell  0EF3:2676", [
    "attack spells end invisibility",
    "flag 2 (item spells): a hand must be free, and no spell item already held",
    "panel cast: the slot is removed and its negated value goes to the end of the level's row (spent)",
    "item cast: the scroll is used up, or a wand charge",
    "\"%s casts %s.\""])
b2 = tbox(xb, b1 + 12, wb, "ochre", "Flag 1: needs a target", [
    "PromptSpellTarget 0EF3:059D asks \"Cast spell on which character?\" and the party panel waits; a click on a portrait runs CastSpellOnTarget 0EF3:08CF for that member"])
b3 = tbox(xb, b2 + 12, wb, "teal", "Common tail", [
    "spell sound; flash the caster (flag 6) or the party (flag 8); \"ok\"",
    "gCurrentSpellApply(): the handler",
    "item casts: EndItemCast arms the hand's cooldown; panel casts: RedrawPartyPanel"])
# handlers
xc, wc = 506, 254
c1 = tbox(xc, 24, wc, "red", "Instant handlers", ["Cure and Cause Wounds, Fear, Burning Hands, Cone of Cold, Create Food, Raise Dead, Dispel Magic, Remove Paralysis...", "change hp, flags or monsters at once"])
c2 = tbox(xc, c1 + 12, wc, "plum", "Timed handlers", ["set a flag, then ApplyTimedCharEffect: slot type = minus the spell number; when it runs out ExpireSpellEffect calls gSpells[n].remove: Bless, Shield, Haste, Aid, Prayer, Vestment, Slow Poison..."])
c3 = tbox(xc, c2 + 12, wc, "ochre", "Missile handlers", ["StartCellEffect(member, mode, party cell, quarter, facing): a projectile record in state 2; MoveDDEffects moves it, the Hit callback calls DamageTargetCell: Magic Missile, Acid Arrow, Fireball, Flame Arrow, Lightning, Ice Storm, Hold, Flame Strike"])
c4 = tbox(xc, c3 + 12, wc, "grey", "Item handlers", ["CreateItemType and CreateItemRecord make a temporary weapon in the free hand: Shocking Grasp, Vampiric Touch, Flame Blade; the timed effect discards it"])
# arrows
bus = xb - 14
c.line(bus, 60, bus, a3 - 40, stroke=GREY, sw=1.3)
c.arrow([(bus, 60), (xb, 60)])
for yy in (a1 - 40, a2 - 30, a3 - 40):
    c.line(xa + wa, yy, bus, yy, stroke=GREY, sw=1.3)
c.arrow([(xb + wb / 2, b1), (xb + wb / 2, b1 + 12)], label="flag 1", lpos=0.5, ldx=24, ldy=3)
c.arrow([(xb + wb / 2, b2), (xb + wb / 2, b2 + 12)])
bus2 = xb + wb + 14
taily = b2 + 12 + 50
c.line(xb + wb, taily, bus2, taily, stroke=GREY, sw=1.3)
ys = (c1 - 36, c2 - 36, c3 - 50, c4 - 28)
c.line(bus2, min(ys), bus2, max(max(ys), taily), stroke=GREY, sw=1.3)
for yy in ys:
    c.arrow([(bus2, yy), (xc, yy)])
c.h = max(a3, b3, c4) + 16
c.save("figs/cmb_castflow.svg")

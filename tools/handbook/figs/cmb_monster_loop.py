import sys; sys.path.insert(0, '.')
from figlib import *

c = Canvas(760, 520)
c.section(0, 11, "ONE MONSTER UPDATE, AND WHAT A MELEE ATTACK DOES")

def tbox(x, y, w, kind, title, lines, size=10.5):
    out = []
    for l in lines:
        out += wrap(l, w - 36, size)
    h = 10 + 17 + len(out) * size * 1.3 + 8
    c.box(x, y, w, h, kind, title, out, size=size, tsize=12, align="left")
    return y + h

xa, wa = 0, 244
y = tbox(xa, 24, wa, "red", "UpdateMonsters(group)  08E6:0CC8", [
    "run by the timer of group 0 to 3 (effects 2 to 5), every 15 to 35 ticks; for each of the 30 records of that group with hp above 0:"])
ya = y
y = tbox(xa, y + 12, wa, "grey", "1  AlertMonsterToParty", [
    "not in mode 7 to 10; party within 3 cells (Distance < 4), and not behind the monster unless within 1 cell: mode 0, target = party cell"])
y = tbox(xa, y + 12, wa, "grey", "2  ChasePartyIfAttacked", ["flag 1 (hit by the party): mode 0; a sleeper (mode 8) wakes its whole group"])
y = tbox(xa, y + 12, wa, "grey", "3  switch (mode)", [
    "0 hunt;  1, 2, 3 walk ahead, turn when blocked;  5, 6 follow a wall;  4 stand still;  7, 10 count the timer down, then mode 0;  8 asleep;  9 never alerted"])
y = tbox(xa, y + 12, wa, "grey", "4  flip the walk frame; UpdateDDEffects", ["modes 4 and 7 keep their frame; projectiles are tested again against the new positions"])
for yy in (ya + 0,):
    pass

xb, wb = 270, 236
yb = tbox(xb, 24, wb, "red", "Mode 0 hunting  08E6:0AB8", ["each update, in this order, the first that acts ends it:"])
yb2 = tbox(xb, yb + 10, wb, "ochre", "a  TryMonsterRangedAttack", [
    "type flag 64, actions left, and the 5-update delay over; within 3 cells straight ahead with nothing between; then a spell or throw chosen by type (spell table, figure on spells); sets actionDelay 5"])
yb3 = tbox(xb, yb2 + 10, wb, "ochre", "b  TryMonsterMelee", [
    "the cell ahead is the party's; the monster stands in the near half or the centre (else it picks a new quarter); it swings on every second update (flag 4 flips); swing frames 0FEh and 0FFh drawn, the view clock paused"])
yb4 = tbox(xb, yb3 + 10, wb, "ochre", "c  StepMonsterToward(target cell)", [
    "else it takes one step along the direction to the target, turning round obstacles; with type flag 8 it may swing after the step"])

xc, wc = 532, 228
yc = tbox(xc, 24, wc, "plum", "ResolveMonsterMelee  08E6:10F3", [
    "target: first member of the list for this side and quarter (figure on targets)",
    "for each of the type's 1 to 3 attacks: RollMonsterToHit; a hit adds RollDice(N, S, B)",
    "Stoneskin counter above 0: it drops by 1 and the running total becomes 0",
    "total above 0: DamageCharacter once"])
yc2 = tbox(xc, yc + 10, wc, "plum", "Then, by the type's flags", [
    "128: a metal item of the target is lost (random slot from a random start)",
    "16: poisoned (save kind 0, no skip if already poisoned); 32: paralysed (save kind 2)"])
c.arrow([(xa + wa, 90), (xb, 60)], color=GREY)
c.arrow([(xb + wb, 120), (xc, 120)])
c.arrow([(xb + wb, 330), (xc - 12, 330), (xc - 12, yc + 40), (xc, yc + 40)])
yt = c.para(xb, yb4 + 20, "Timing: a monster swings once per two updates, so every 30 to 70 ticks (1.6 to 3.8 s) by its group.", 480, size=10.5, color=MUTED)
c.h = max(y, yt, yc2) + 20
c.save("figs/cmb_monster_loop.svg")

import sys; sys.path.insert(0, '.')
from figlib import *

c = Canvas(760, 470)
c.section(0, 11, "HIT POINTS, STATUS BITS AND THE PARTY-WIPE CHECK")

def tbox(x, y, w, kind, title, lines, size=10.5, h=None):
    out = []
    for l in lines:
        out += wrap(l, w - 34, size)
    hh = h or (10 + 17 + len(out) * size * 1.3 + 8)
    c.box(x, y, w, hh, kind, title, out, size=size, tsize=12, align="left")
    return y + hh

# hp axis
y = 24
widths = [(0, 360, "teal", "hp above 0: awake", ["acts, casts, is a target, shares experience; MemberIs bits 1, 2, 4 pass"]),
          (370, 220, "ochre", "0 to -9: unconscious", ["cannot act (bit 4 fails); still a target and still heals; nothing bleeds"]),
          (600, 160, "red", "-10 or less: dead", ["hp is set to -10; status keeps only bit 0; HealCharacter ignores it"])]
for x, w, kind, t, l in widths:
    tbox(x, y, w, kind, t, l, h=88)
c.text(0, y + 106, "hp is a signed byte: Aid and healing items are capped at 127; a death is when hp reaches -10 (DamageCharacter, 0DE8:09B9).", size=10.5, color=MUTED)

# masks
my = 150
c.text(0, my, "MemberIs(who, mask)  0D1C:0670: every bit in the mask must hold", size=12, weight=700, color=INK)
rows = [("1", "slot holds a character (status bit 0)"), ("2", "hp above -10 (not dead)"), ("4", "hp above 0 (not unconscious)"),
        ("8", "not paralysed (status bit 2)"), ("16", "not poisoned (status bit 1)")]
for i, (m, t) in enumerate(rows):
    yy = my + 18 + i * 16
    c.text(0, yy, m, size=11, weight=700, color=PAL["navy"][0], mono=True)
    c.text(34, yy, t, size=11, color=INK2)
uses = [("1", "any member present"), ("3", "alive: targets of healing, spells, shared XP"), ("5", "awake: counted by the party-wipe check"),
        ("13", "able to act: use an item, turn undead")]
c.text(330, my + 18, "common masks", size=11, weight=700, color=MUTED)
for i, (m, t) in enumerate(uses):
    yy = my + 34 + i * 16
    c.text(330, yy, m, size=11, weight=700, color=PAL["navy"][0], mono=True)
    c.text(362, yy, t, size=11, color=INK2)

# conditions
cy = 270
c.text(0, cy, "Conditions and what ends them (timed effect types 8 and 9)", size=12, weight=700, color=INK)
x0 = 0
bw = 244
t1 = tbox(0, cy + 12, bw, "plum", "Poisoned: status bit 1", [
    "a monster with flag 16 hits, or a poison potion is drunk, and the member fails a kind-0 save",
    "every 546 ticks (30 s): 5 hp of damage and a message, until cured",
    "Slow Poison (flag 128): the ticks do nothing for a while"])
t2 = tbox(258, cy + 12, bw, "plum", "Paralysed: status bit 2", [
    "monster flag 32 (save kind 2) or Mind Blast (kind 1, whole party)",
    "ends by itself after 5 * 546 = 2730 ticks (150 s), with a message",
    "Remove Paralysis clears up to four members at once"])
t3 = tbox(516, cy + 12, 244, "red", "Party wiped", [
    "checked only when a member dies: if no member is present with hp above 0, a message is shown, a key is awaited and the game exits. There is no game-over menu."])
c.h = max(t1, t2, t3) + 10
c.save("figs/cmb_states.svg")

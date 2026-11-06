import sys; sys.path.insert(0, '.')
from figlib import *

c = Canvas(760, 410)
c.section(0, 11, "The party: six record slots, four made by the player, nine ready-made NPCs")

# ---- left: slots ----
c.text(0, 34, "gChars[0..5] as the party panel draws them", size=10.5, weight=700, color=MUTED)
ranks = [("front", ["0", "1"]), ("middle", ["2", "3"]), ("back", ["4", "5"])]
for r, (nm, slots) in enumerate(ranks):
    y = 44 + r * 70
    for j, s in enumerate(slots):
        x = j * 100
        kind = "teal" if int(s) < 4 else "ochre"
        c.box(x, y, 92, 58, kind, "slot " + s, ["made in the panel" if int(s) < 4 else "NPC or reserve"], size=10, tsize=12.5)
    c.text(204, y + 34, nm + " rank", size=10.5, color=MUTED)
c.para(0, 266, "A monster coming from the front scans the slots front rank first (gAttackTargetOrder, ds:325B). Cell positions: slots 0 to 3 take the four quarters of the cell; 4 and 5 use the quarters of 2 and 3. Screen: x = slot & 1, y = slot >> 1.", 300, size=10, color=INK2)

# ---- middle: operations ----
OX, OW = 330, 205
ops = [
    ("teal", "create", "RunCharacterPanel fills slots 0 to 3 and sets status to 1, food to 100."),
    ("navy", "join", "AddPartyMember copies a pool record into the first slot whose status bit 0 is clear; with six present ChooseChar asks whom to remove."),
    ("red", "drop", "DropCharacter, only with 5 or more present: items go to the floor, and for slots 0 to 3 slot 5 (else 4) moves in."),
    ("grey", "reorder", "PickOrSwapMember swaps two whole 243-byte records."),
]
y = 26
hs = [62, 92, 80, 50]
for (k, t, b), h in zip(ops, hs):
    c.box(OX, y, OW, h, k, t, [b], size=10, tsize=12)
    y += h + 8

# ---- right: pool ----
PX = 548
c.text(PX, 34, "gCharPool, ds:7298: 9 records", size=10.5, weight=700, color=MUTED)
pool = [("Anya", "human, fighter", 4, 45), ("Beohram", "human, paladin", 8, 95), ("Kirath", "elf, mage", 11, 32), ("Ileria", "human, cleric", 6, 52),
        ("Tyrra", "elf, ranger", 10, 98), ("Tod Uphill", "halfling, thief", 4, 30), ("Taghor", "dwarf, fighter", 5, 58), ("Dohrum", "dwarf, fighter", 3, 28),
        ("Keirgar", "dwarf, fighter", 5, 72)]
c.text(PX, 52, "name", size=10, weight=700, color=PAL["teal"][0])
c.text(PX + 66, 52, "race, class", size=10, weight=700, color=PAL["teal"][0])
c.text(PX + 150, 52, "L", size=10, weight=700, color=PAL["teal"][0])
c.text(PX + 168, 52, "maxHp", size=10, weight=700, color=PAL["teal"][0])
for i, (n, rc, lv, hp) in enumerate(pool):
    yy = 70 + i * 16
    c.text(PX, yy, n, size=10.5, color=INK2)
    c.text(PX + 66, yy, rc, size=10, color=INK2)
    c.text(PX + 152, yy, str(lv), size=10, color=INK2, mono=True)
    c.text(PX + 170, yy, str(hp), size=10, color=INK2, mono=True)
c.text(PX, 70 + 9 * 16 + 6, "Taghor and Keirgar are stored", size=10, color=MUTED)
c.text(PX, 70 + 9 * 16 + 19, "wounded (hp 3 and 10).", size=10, color=MUTED)

c.para(0, 372, "A pool character joins through a panel of overlay 5 (AddCharPanel); the matching 'bones' item (type 21h, value = pool index + 1) is first removed from the party.", 740, size=10.5, color=INK2)
c.save("figs/chr_party.svg")

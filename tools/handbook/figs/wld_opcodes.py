import sys; sys.path.insert(0, '.')
from figlib import *

ops = [
    ("FF", "SET WALL", "sub F7 cell u16, kind: all four sides; E9 cell, side, kind; ED dir: the party's facing", 491, "teal", "SetWallTypeOp"),
    ("FE", "TOGGLE WALL", "sub F7 cell, a, b: swap the four sides between kinds a and b; EA cell: toggle a door", 112, "teal", "ToggleWallTypeOp"),
    ("FD", "OPEN DOOR", "cell u16", 93, "teal", "OpenDoorOp"),
    ("FC", "CLOSE DOOR", "cell u16", 47, "teal", "CloseDoorOp"),
    ("FB", "SPAWN", "the 14-byte monster record (first byte unused)", 42, "red", "SpawnMonsterOp"),
    ("FA", "MOVE", "sub E8 from, to: the party; F3 from, to: monsters; F5 from, to: items and thrown things; F1 to", 653, "red", "TeleportOp"),
    ("F9", "STEAL ITEM", "member (FF random), cell u16, position: a carried item goes to the cell", 9, "ochre", "MoveCarriedItemOp"),
    ("F8", "TEXT", "string, NUL, two colour bytes", 668, "plum", "ShowTextOp"),
    ("F7", "SET FLAG", "sub EF bit (this level); F0 bit (game); F3 monster, bit (and mode 0); E4 (unread flag)", 208, "navy", "SetFlagOp"),
    ("F6", "SOUND", "sound, cell u16 (0 = heard everywhere)", 770, "plum", "PlaySoundOp"),
    ("F5", "CLEAR FLAG", "sub EF bit; F0 bit", 18, "navy", "ClearFlagOp"),
    ("F4", "HEAL", "member (FF all), amount (no level uses it)", 0, "ochre", "HealPartyMember"),
    ("F3", "DAMAGE", "member (FF all), dice count, sides, bonus", 88, "ochre", "DamagePartyMember"),
    ("F2", "JUMP", "offset u16 from the start of the file", 544, "grey", "JumpToScriptOffset"),
    ("F1", "END", "(none)", 871, "grey", "ScriptStop"),
    ("F0", "RETURN", "(none): pop the call stack", 35, "grey", "PopScriptPointer"),
    ("EF", "CALL", "offset u16: push the return address (ten deep), jump", 136, "grey", "PushScriptPointer"),
    ("EE", "IF", "expression tokens, EE, else-offset u16 (section 5.5.4)", 1572, "grey", "EvaluateScriptExpr"),
    ("ED", "DRAIN", "FF: the mouse item; FE cell: all items of the cell; type, cell: items of that type", 238, "ochre", "DrainItemOp"),
    ("EC", "LOAD LEVEL", "unused, level, cell u16, dir (FF keeps the facing); stops the script", 125, "red", "LoadLevelOp"),
    ("EB", "GIVE XP", "E2, amount u16: party experience", 143, "ochre", "AddExperienceOp"),
    ("EA", "NEW ITEM", "item u16, cell u16 (FFFF: onto the mouse), position", 61, "ochre", "CreateItemOp"),
    ("E9", "LAUNCH", "DF spell or other item, id, unused, cell u16, dir, position: a flying object", 68, "red", "LaunchProjectileOp"),
    ("E8", "TURN", "sub F1 n: the party; F5 n: flying objects in this cell (quarter turns)", 33, "teal", "TurnOp"),
    ("E7", "IDENTIFY", "cell u16: identify the items there (and the party's own if the party is there)", 2, "ochre", "IdentifyItemsOp"),
    ("E6", "OVERLAY", "arg: CharOverlay panel 0 to 10 (a picture and a dialogue screen)", 27, "plum", "CallOverlayOp"),
]
RH = 50
H = 40 + 13 * RH + 30
c = Canvas(760, H)
c.section(0, 11, "THE 26 OPCODES, WITH THEIR USE COUNT IN THE 12 LEVEL FILES (7,054 INSTRUCTIONS)")
CW = 372
for i, (b, m, o, n, k, fn) in enumerate(ops):
    col, row = divmod(i, 13)
    x = col * (CW + 16)
    y = 28 + row * RH
    c.rect(x, y, CW, RH - 5, fill=PAL[k][2], stroke=PAL[k][1], sw=1, rx=5)
    c.rect(x + 6, y + 7, 28, 18, fill=SOLID[k], stroke="none", rx=3)
    c.text(x + 20, y + 20, b, size=11, weight=700, color="#FFFFFF", anchor="middle", mono=True)
    c.text(x + 42, y + 17, m, size=11.5, weight=700, color=PAL[k][0])
    c.text(x + CW - 8, y + 17, "%d" % n if n else "0", size=10.5, color=MUTED, anchor="end", mono=True)
    c.text(x + 42 + text_width(m, 11.5, False, 700) + 8, y + 17, fn, size=9.5, color=MUTED, mono=True)
    ls = wrap(o, CW - 56, 10)
    for j, l in enumerate(ls[:2]):
        c.text(x + 42, y + 30 + j * 12, l, size=10, color=INK2)
yl = 28 + 13 * RH + 8
c.legend(0, yl + 6, [("teal", "walls, doors, facing"), ("red", "monsters, moves, level change"), ("ochre", "items, damage, experience"), ("plum", "text, sound, panels"), ("navy", "flags"), ("grey", "control flow")])
c.save("figs/wld_opcodes.svg")

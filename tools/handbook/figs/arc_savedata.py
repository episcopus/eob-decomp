import sys; sys.path.insert(0, '.')
from figlib import *

c = Canvas(760, 470)
def lane(y, title, kind, steps, h=92):
    tc, sc, fc = PAL[kind]
    c.text(0, y - 8, title, size=11.5, weight=700, color=tc)
    n = len(steps)
    bw = (760 - 14 * (n - 1)) / n
    for i, (t, b) in enumerate(steps):
        x = i * (bw + 14)
        c.box(x, y, bw, h, kind, t, [b], size=9.8, tsize=11, fill=CARD)
        if i < n - 1:
            c.arrow([(x + bw + 1, y + h / 2), (x + bw + 13, y + h / 2)], color=GREY)
lane(26, "Leaving a level (a stairs or teleport script: LoadLevelOp, 122E:1AAB)", "navy", [
    ("ResetLevel", "finish a moving door, clear projectiles and attack frames"),
    ("SaveLevelState", "pack: copy the 4 wall bytes of the 1,024 cells, LCW-compress to a stream of at most 1,200 bytes, append the 30 monsters"),
    ("write a slot", "LEVELS.TMP at (level - 1) x 2,040, 2,040 bytes; set bit level - 1 of gSavedLevels"),
    ("LoadLevel(new)", "next section"),
], 82)
lane(146, "Entering a level (LoadLevel, ovl3:0A7C)", "teal", [
    ("INF and maze", "LEVELn.INF is unpacked: wall sets, timers, monster list. LEVELn.MAZ gives the pristine wall bytes"),
    ("visited?", "bit level - 1 of gSavedLevels: the slot is read from LEVELS.TMP, unpacked over the walls, monsters copied back"),
    ("first visit", "monsters are created from the INF list (InitMonster) and the walls stay as in the MAZ file"),
    ("items", "RebuildItemList: every item whose level byte is this level goes into its cell's list"),
], 92)
lane(272, "Save game (SaveGame, ovl3:1368; the camp menu asks first)", "plum", [
    ("ResetLevel", "settle doors and projectiles"),
    ("merge", "read all of LEVELS.TMP (24,480) into gArtPage, pack the current level over its own slot there"),
    ("write", "recreate EOBDATA.SAV: characters (timers aged), 64-byte header, items, the level image, 3 option bytes, 96 bytes"),
    ("check", "each write must return its full length; the result picks the message \"Game saved.\" or \"Save game failure!\""),
], 92)
lane(398, "Load game (LoadGame, ovl3:15B9; from the title menu or the camp menu)", "ochre", [
    ("read", "six blocks, sizes from ds:6A8C, back to back into gArtPage; any short read = failure"),
    ("restore", "characters (keeping their portrait pointers), header, items; options; the 96 bytes"),
    ("LEVELS.TMP", "recreated from block 4: the file is the save's level image"),
    ("portraits", "re-cut from CHARGENA or OUTTAKE sheets; then LoadLevel and redraw"),
], 62)
c.h = 470
c.save("figs/arc_savedata.svg")

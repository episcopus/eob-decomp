import sys; sys.path.insert(0, '.')
from figlib import *

steps = [
    ("teal", "LEVELn.INF", "read the u16 size and the packed body, Unpack it", "gLevelInf  (7,500 B)", "ochre"),
    ("teal", "LEVELn.MAZ", "LoadMaze: 6-byte header, then 4,096 wall bytes, into the 9-byte cells", "gMaze  (9,216 B)", "ochre"),
    ("plum", "LEVELS.TMP", "if the level was visited (gSavedLevels bit), LoadLevelState replaces the walls and the monsters", "gMaze, gMonsters", "ochre"),
    ("teal", "name.VMP / .EMP, .PAL", "wall block maps; the palette in VGA mode", "gWallBlockMaps, gPaletteBuf", "ochre"),
    ("navy", "INF bytes 38 to 72", "door picture set, level timer (mode, period, steps), two monster sprite sheets", "gDoorShapes, timers, gLoadedMonsterShapes", "ochre"),
    ("navy", "INF timer pairs", "the periods of the four monster update effects, their first expiry", "gEffects[2..5]", "ochre"),
    ("navy", "INF: 30 monster records", "InitMonster each (skipped for a visited level, whose monsters are in the state)", "gMonsters  (30 x 28 B)", "ochre"),
    ("teal", "INF wall records, name.DAT", "0xEC: LoadWallSet (decorations, rectangles); 0xFB: DefineWallType (one wall kind)", "the four per-kind tables, gWallRecs, gDecorShapes", "ochre"),
    ("teal", "ITEM.DAT (already in gItems)", "clear the item heads, RebuildItemList: every item of this level goes into its cell's list", "cell +5 (item list head)", "ochre"),
    ("navy", "INF trigger table", "for each entry: cell +4 |= flag byte, cell +7 = script offset", "cell +4 and +7", "ochre"),
    ("teal", "name.VCN / .ECN, INVENT", "LoadWallTextures, the inventory screen; gLevelPalettePending = 1", "gWallBlocks, screen", "ochre"),
]
RH, GAP = 40, 7
top = 30
H = top + len(steps) * (RH + GAP) + 24
c = Canvas(760, H)
c.section(0, 11, "LoadLevel (ovl3:0A7C), IN ORDER: WHAT IT READS, WHAT IT DOES, WHAT IT FILLS")
LX, LW = 0, 168
MX, MW = 188, 390
RX, RW = 598, 162
for i, (sk, src, what, dst, dk) in enumerate(steps):
    y = top + i * (RH + GAP)
    c.box(LX, y, LW, RH, sk, None, [src], size=11)
    c.rect(MX, y, MW, RH, fill=PAL["navy"][2], stroke=PAL["navy"][1], sw=1.1, rx=5)
    c.badge(MX + 14, y + RH / 2, str(i + 1), "navy", r=9)
    lines = wrap(what, MW - 44, 11)
    yy = y + RH / 2 - (len(lines) - 1) * 7 + 4
    for l in lines:
        c.text(MX + 30, yy, l, size=11, color=INK2)
        yy += 14
    c.box(RX, y, RW, RH, dk, None, [dst], size=10.5)
    c.arrow([(LX + LW + 1, y + RH / 2), (MX - 2, y + RH / 2)], color=SOLID[sk])
    c.arrow([(MX + MW + 1, y + RH / 2), (RX - 2, y + RH / 2)], color=SOLID["ochre"])
c.legend(0, H - 6, [("teal", "read from a data file"), ("plum", "read from the saved state"), ("navy", "from the unpacked level file"), ("ochre", "where it lands")])
c.save("figs/wld_load.svg")

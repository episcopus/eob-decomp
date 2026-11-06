import sys; sys.path.insert(0, '.')
from figlib import *

c = Canvas(760, 330)
c.section(0, 11, "A PAK archive: a directory, then the files back to back")
c.rect(0, 30, 250, 100, fill=PAL["navy"][2], stroke=PAL["navy"][1], rx=0)
c.text(125, 24, "directory (600 bytes in EOBDATA3)", size=10.5, color=MUTED, anchor="middle")
rows = [("00000258", "BRICK1.CPS"), ("00002F6F", "BRICK2.CPS"), ("00004C18", "BRICK3.CPS"), ("...", ""), ("????????", "(nameless end)")]
for i, (o, n) in enumerate(rows):
    y = 48 + i * 17
    c.text(10, y, o, size=10.5, mono=True, color=PAL["navy"][0])
    c.text(100, y, n, size=10.5, mono=True, color=INK2)
c.text(125, 150, "u32 offset + NUL-terminated name", size=10.5, color=INK2, anchor="middle")
c.text(125, 164, "ends where the first file starts", size=10.5, color=MUTED, anchor="middle")
x = 250
names = [("BRICK1.CPS", 11543, "teal"), ("BRICK2.CPS", 7337, "teal"), ("BRICK3.CPS", 3303, "teal"), ("BRICK1.EGA", 9215, "plum"), ("...", 4000, "grey")]
tot = sum(n[1] for n in names)
for (n, b, k) in names:
    w = 510 * b / tot
    tc, sc, fc = PAL[k]
    c.rect(x, 30, w, 100, fill=fc, stroke=sc, rx=0)
    c.text(x + w / 2, 76, n.replace(".CPS", "") if w < 90 else n, size=10 if w < 100 else 11, color=tc, anchor="middle", weight=600, mono=True)
    c.text(x + w / 2, 92, f"{b:,}" if n != "..." else "", size=10, color=MUTED, anchor="middle")
    x += w
c.text(255, 150, "size of a file = distance to the next offset (the last one runs to the end of the file)", size=10.5, color=INK2)
c.section(0, 200, "Six archives, 193 files")
arch = [
    ("EOBDATA1", "17 files, 237,691 B", "sound drivers' data and the intro pictures (.CMP)", "ochre"),
    ("EOBDATA2", "2 files, 14,499 B", "DOOR.EGA and the game palette EOBPAL.COL", "grey"),
    ("EOBDATA3", "levels 1 to 3", "BRICK wall set, kobolds, skeletons, intro and character generation", "teal"),
    ("EOBDATA4", "levels 4 to 6", "BLUE wall set, dwarves, mages, palettes, SOUND.DAT", "teal"),
    ("EOBDATA5", "levels 10 to 12", "GREEN and XANATHA wall sets, mind flayers, the golem", "teal"),
    ("EOBDATA6", "levels 7 to 9", "DROW wall set, the item and item type tables, portals", "teal"),
]
w = (760 - 5 * 8) / 6
for i, (n, a, b, k) in enumerate(arch):
    x = i * (w + 8)
    tc, sc, fc = PAL[k]
    c.rect(x, 214, w, 100, fill=fc, stroke=sc, rx=6)
    c.text(x + 8, 232, n, size=11.5, weight=700, color=tc, mono=True)
    c.text(x + 8, 248, a, size=10, color=INK2, weight=600)
    c.para(x + 8, 264, b, w - 14, size=10, color=INK2, lh=12.5)
c.save("figs/data_pak.svg")

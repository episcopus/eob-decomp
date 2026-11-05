import sys; sys.path.insert(0, '.')
from figlib import *

c = Canvas(760, 430)
# A: the record
c.section(0, 11, "A. One record of the file table (ds:01FF), 19 bytes")
fields = [("name", 4, "navy", "far ptr"), ("size", 4, "teal", "long"), ("data", 4, "ochre", "far ptr"),
          ("link", 2, "plum", "int"), ("pos", 4, "grey", "long"), ("fl", 1, "red", "char")]
u = 740.0 / 19
x = 0
for n, b, k, t in fields:
    w = b * u
    c.rect(x, 22, w - 2, 34, fill=MID[k], stroke=SOLID[k], sw=1.1, rx=3)
    c.text(x + (w - 2) / 2, 38, n, size=11.5, weight=700, color=PAL[k][0], anchor="middle")
    c.text(x + (w - 2) / 2, 51, "+%02X" % (sum(f[1] for f in fields[:fields.index((n, b, k, t))])) + ("" if b == 1 else " " + t), size=9.8, color=INK2, anchor="middle")
    x += w
c.text(0, 72, "link < 0: the record is a member of another record's PAK (low 15 bits = that record); link >= 0: a plain file, on disk number link + 1.", size=10.8, color=INK2)
c.text(0, 87, "data is set only while a copy of the file is held in memory; flags would mark files to preload or load on demand. Every flags byte is 0 here.", size=10.8, color=INK2)

# B: the walk
c.section(0, 118, "B. FileOpen(\"BRICK.VCN\", 1): from a name to a stretch of a PAK")
steps = [
    ("FindFile", "strcmp down the table: record 32, link = alias of record 24 (eobdata3.pak), size 34,847"),
    ("CheckFile", "LocateOverlayFile opens eobdata3.pak on the current drive; if absent: the dialog \"Insert disk 3 into any drive\""),
    ("open + LookupFileEntry", "DOS open of the PAK; walk its directory (u32 offset, name) to BRICK.VCN: offset and length, seek there"),
    ("a handle slot", "first free of 10 slots in gFileHandles: DOS handle | 4000h; slot offset and length are kept"),
    ("FileRead", "reads at most what is left of the member (the length clamp), in chunks of 0FA00h bytes"),
]
bw = 140
for i, (t, b) in enumerate(steps):
    x = i * (bw + 10)
    c.box(x, 130, bw, 122, "navy" if i != 1 else "red", t, [b], size=10.3, tsize=11.5, fill=CARD)
    c.badge(x + 12, 142, str(i + 1), "navy" if i != 1 else "red", r=8)
    if i < 4:
        c.arrow([(x + bw + 1, 191), (x + bw + 9, 191)], color=GREY)

# C: census
c.section(0, 280, "C. What the 203 records are")
rows = [
    ("10 plain files", "INTRO.EXE and eobdata1.pak (disk 1), EOB.EXE, LEVELS.TMP, eobdata2.pak (disk 2), eobdata3.pak, 4, 5 (disks 3 to 5), EOBDATA.SAV and eobdata6.pak (disk 6)", "teal"),
    ("193 aliases", "one per PAK member: 17 in eobdata1, 2 in eobdata2, 174 in eobdata3 to 6; sizes are the members' unpacked file sizes", "navy"),
    ("0 preloaded", "no flags byte has FLAG_KEEP or FLAG_LOAD, so BuildFileCache loads nothing and no record ever has a copy in memory", "grey"),
]
y = 292
for t, b, k in rows:
    c.rect(0, y, 760, 34, fill=PAL[k][2], stroke=PAL[k][1], sw=1, rx=5)
    c.text(10, y + 21, t, size=12, weight=700, color=PAL[k][0])
    ls = wrap(b, 590, 10.6)
    for j, l in enumerate(ls[:2]):
        c.text(150, y + 15 + 12 * j, l, size=10.6, color=INK2)
    y += 40
c.text(0, y + 10, "The disk numbers look like the floppy release's: a file found on no drive makes the game ask for its disk by number.", size=10.5, color=MUTED)
c.h = y + 22
c.save("figs/arc_files.svg")

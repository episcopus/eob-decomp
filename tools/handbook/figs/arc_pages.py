import sys; sys.path.insert(0, '.')
from figlib import *

c = Canvas(760, 330)
c.section(0, 11, "The page table gPageSegs (ds:FC0C): 8 segment words per graphics choice")
LX = 150
cw = (760 - LX) / 4.0   # 4 pairs
# headers: pairs
pairs = [("slots 0, 1", "screen", "0"), ("slots 2, 3", "work page; slot 3 = gArtPage", "2"),
         ("slots 4, 5", "slot 5 = gStagingPage", "4"), ("slots 6, 7", "slot 7 = gWallBlocks", "6")]
for i, (a, b, k) in enumerate(pairs):
    x = LX + i * cw
    c.text(x + cw / 2, 40, "page kind " + k, size=11.5, weight=700, color=PAL["navy"][0], anchor="middle")
    c.text(x + cw / 2, 54, b, size=10, color=MUTED, anchor="middle")
    c.text(x + cw / 2, 68, a + ": video, memory", size=10, color=MUTED, anchor="middle")
rows = [
    ("0  CGA", [("B800", "v"), ("B800", "v"), ("64,000", "r2"), ("32,000", "r2"), ("52,000", "r2")]),
    ("1  Tandy", [("B800", "v"), ("B800", "v"), ("memtop", "r2"), ("32,576", "r2"), ("52,000", "r2")]),
    ("2  EGA", [("A000", "v"), ("(unset)", "n"), ("A200", "v"), ("64,000", "r"), ("A200", "v"), ("32,000", "r"), ("A200", "v"), ("52,000", "r")]),
    ("3  VGA (MCGA.OVL)", [("A000", "v"), ("A000", "v"), ("65,000", "r2"), ("32,000", "r2"), ("52,000", "r2")]),
    ("4  vga.ovl", [("A000", "v"), ("64,000", "r"), ("A400", "v"), ("64,000", "r"), ("A800", "v"), ("64,000", "r"), ("AC00", "v"), ("64,512", "r")]),
]
y = 80
RH = 40
for name, cells in rows:
    c.text(0, y + 24, name, size=12, weight=700, color=INK)
    # expand cells to 8 slots
    slots = []
    for t, kind in cells:
        if kind == "r2":
            slots.append((t, "r", 2))
        else:
            slots.append((t, kind, 1))
    # layout by slot index
    s = 0
    # pairs structure: first pair is slots 0,1
    idx = 0
    for t, kind, span in slots:
        x = LX + idx * cw / 2
        w = span * cw / 2 - 3
        if kind == "v":
            c.rect(x, y + 4, w, 30, fill=MID["ochre"], stroke=SOLID["ochre"], sw=1.1, rx=3)
            c.text(x + w / 2, y + 24, t, size=11, weight=700, color=PAL["ochre"][0], anchor="middle", mono=True)
        elif kind == "n":
            c.rect(x, y + 4, w, 30, fill=PANEL, stroke=GREY, sw=1, rx=3, dash="3 2")
            c.text(x + w / 2, y + 24, t, size=10.5, color=MUTED, anchor="middle")
        else:
            c.rect(x, y + 4, w, 30, fill=MID["navy"], stroke=SOLID["navy"], sw=1.1, rx=3)
            c.text(x + w / 2, y + 24, (t + " B") if t[0].isdigit() else t, size=11, weight=700, color=PAL["navy"][0], anchor="middle")
        idx += span
    y += RH
c.text(0, y + 12, "ochre: a video segment (the screen, or off-screen video memory); blue: a MemAlloc block of that many bytes;", size=10.5, color=MUTED)
c.text(0, y + 26, "a wide cell is one block that both slots of the pair point at. memtop is the 64 KB the Tandy path reserves under video memory.", size=10.5, color=MUTED)
c.text(0, y + 40, "GetPageBuffer(kind) returns the odd slot (always memory), GetPageVideo(kind) the even one (the segment the driver draws to).", size=10.5, color=MUTED)
c.h = y + 50
c.save("figs/arc_pages.svg")

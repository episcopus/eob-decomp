import sys; sys.path.insert(0, '.')
from figlib import *

c = Canvas(760, 470)
c.section(0, 11, "The 500 item records and the lists they live in")

# ---- id space ----
c.text(0, 34, "item ids 0 to 499 (gItems, ds:8DD0), to scale", size=10.5, weight=700, color=MUTED)
X0, W = 0, 740.0
def px(i):
    return X0 + i / 500.0 * W
segs = [
    (0, 1, "paper", "0"), (1, 11, "teal", "1 to 10"), (11, 18, "plum", ""), (18, 432, "navy", "18 to 431: from ITEM.DAT"),
    (432, 500, "grey", "432 to 499: free"),
]
for a, b, k, t in segs:
    tc, sc, fc = PAL[k]
    c.rect(px(a), 44, px(b) - px(a), 24, fill=fc, stroke=sc, sw=1.1, rx=0)
    if t and b - a > 60:
        c.text((px(a) + px(b)) / 2, 60, t, size=10.5, weight=600, color=tc, anchor="middle")
c.legend(0, 86, [("paper", "0 none"), ("teal", "1 to 10 starting-kit templates"), ("plum", "11 to 17 spell-made items"), ("navy", "18 to 431 from ITEM.DAT"), ("grey", "432 to 499 free")], size=10.5)
c.text(0, 112, "ITEM.DAT fills 0 to 431: 350 are on the maps (key = map cell, level = 1 to 12), 62 are in no map list (key 0), 20 are free (key -1).", size=10.5, color=INK2)
c.text(0, 126, "CopyItemToFreeSlot clones a template into the first record with key -1, from id 1 on; 500 records are all the game ever has.", size=10.5, color=INK2)

# ---- cell record ----
CY = 156
c.text(0, CY, "a map cell: nine bytes at gMaze + cell * 9", size=10.5, weight=700, color=MUTED)
cells = [(4, "walls +0..3", "grey"), (1, "+4", "grey"), (2, "item head", "red"), (2, "+7, +8", "grey")]
x = 0
for n, t, k in cells:
    w = n * 36
    tc, sc, fc = PAL[k]
    c.rect(x, CY + 8, w, 26, fill=fc, stroke=sc, sw=1.1, rx=0)
    c.text(x + w / 2, CY + 25, t, size=10.5, color=tc, anchor="middle", weight=(700 if k == "red" else 400))
    x += w
c.text(x + 10, CY + 25, "+5: word, 0 = no items; else the id of the newest item", size=10.5, color=INK2)

# ---- ring before / after ----
def ring(y, label, nodes, head_name):
    c.text(0, y, label, size=10.5, weight=700, color=PAL["teal"][0])
    bw, gap = 112, 40
    xs = []
    for i, (nm, sub) in enumerate(nodes):
        x = 90 + i * (bw + gap)
        xs.append(x)
        hot = (i == 0)
        c.rect(x, y + 10, bw, 44, fill=PAL["red"][2] if hot else CARD, stroke=PAL["red"][1] if hot else GREY, sw=1.3 if hot else 1, rx=5)
        c.text(x + bw / 2, y + 28, nm, size=11.5, weight=700, color=PAL["red"][0] if hot else INK, anchor="middle")
        c.text(x + bw / 2, y + 43, sub, size=9.5, color=MUTED, anchor="middle")
    for i in range(len(nodes) - 1):
        c.arrow([(xs[i] + bw, y + 26), (xs[i + 1], y + 26)], color=GREY)
        c.arrow([(xs[i + 1], y + 40), (xs[i] + bw, y + 40)], color=RULE)
    # wrap: last next -> head
    xl = xs[-1] + bw / 2
    c.path("M%.1f,%.1f L%.1f,%.1f L%.1f,%.1f L%.1f,%.1f" % (xl, y + 54, xl, y + 68, xs[0] + bw / 2, y + 68, xs[0] + bw / 2, y + 56), stroke=GREY, sw=1.3)
    c.arrow([(xs[0] + bw / 2, y + 62), (xs[0] + bw / 2, y + 55)], color=GREY)
    c.text(xl - 6, y + 80, "nextLink of the last item points back to the head", size=9.5, color=MUTED, anchor="end")
    c.text(0, y + 36, "head word", size=10, color=PAL["red"][0])
    c.arrow([(56, y + 32), (88, y + 32)], color=SOLID["red"])

ring(236, "before", [("item A", "newest"), ("item B", ""), ("item C", "oldest")], "A")
ring(346, "after InsertListItem(head, cell, N, tag)", [("item N", "key = cell, cellPos = tag"), ("item A", ""), ("item B", ""), ("item C", "")], "N")
c.save("figs/chr_itemlist.svg")

import sys, json, struct; sys.path.insert(0, '.')
from figlib import *

diff = json.load(open("data/paldiff.json"))   # the five .PAL files against EOBPAL.COL, from tools/re/wwfmt.py
# the colour tables of the VCN files (decoded by tools/re/wwfmt.py)
sys.path.insert(0, "../re")
import wwfmt
used = {}
for n, pk in [("BRICK", "3"), ("BLUE", "4"), ("DROW", "6"), ("GREEN", "5"), ("XANATHA", "5")]:
    d, _ = wwfmt.unpack(wwfmt.find("EOBDATA%s.PAK:%s.VCN" % (pk, n), "../../eob"))
    used[n] = sorted(set(d[2:34]) - {0})

c = Canvas(760, 500)
c.section(0, 11, "THE VGA PALETTE: WHERE IT COMES FROM AND WHEN IT CHANGES")
# flow
bx = [
    ("EOBPAL.COL", "768 bytes in EOBDATA2.PAK: 6-bit red, green, blue", "grey"),
    ("start-up", "main: LoadFile to gPaletteBuf, gSetPalette (INT 10h AX=1012h, 256 DAC registers)", "teal"),
    ("LoadLevel", "reads <set>.PAL over gPaletteBuf, sets gLevelPalettePending = 1", "ochre"),
    ("DrawViewFrame", "after the sprites, before the copy to the screen: gSetPalette, pending = 0", "navy"),
]
x = 0
w = 178
for i, (t, b, k) in enumerate(bx):
    c.box(x, 30, w, 86, k, t, [b], size=10.2, tsize=12)
    if i:
        c.arrow([(x - 12, 73), (x, 73)], color=GREY)
    x += w + 12
yy = 134
for t in ["Tandy and EGA call gSetPalette with a built-in 16-entry table (ds:3057); CGA has no palette (the CGA driver's SetPalette is a stub).",
          "The five .PAL files differ little from EOBPAL.COL: 6 entries (BRICK) to 41 (DROW) differ in their low six bits. A .PAL byte may also carry 11b in its top two bits (the DAC uses six)."]:
    for l in wrap(t, 750, 10.5):
        c.text(0, yy, l, size=10.5, color=MUTED); yy += 13.5
# strips
y0 = 208
c.section(0, y0 - 14, "THE 256 ENTRIES OF EACH WALL SET'S PALETTE")
sc = 2.7
x0 = 62
for i, n in enumerate(["BRICK", "BLUE", "DROW", "GREEN", "XANATHA"]):
    y = y0 + 6 + i * 44
    c.text(0, y + 12, n, size=10.5, weight=700, color=INK)
    c.rect(x0, y, 256 * sc, 14, fill="#ECE7DD", stroke="none", rx=0)
    for e in diff[n]:
        c.rect(x0 + e * sc, y, sc, 14, fill=SOLID["red"], stroke="none", rx=0)
    for e in used[n]:
        c.rect(x0 + e * sc, y + 16, sc, 6, fill=SOLID["teal"], stroke="none", rx=0)
    c.text(x0 + 256 * sc + 4, y + 11, "%d differ" % len(diff[n]), size=9.5, color=MUTED) if False else None
    c.text(x0, y + 33, "differ: %d    used by the block tables: %s" % (len(diff[n]), " ".join("%02X" % u for u in used[n][:3]) + " .. " + "%02X" % used[n][-1] if used[n] else ""), size=9, color=MUTED)
for e in (0, 0x40, 0x80, 0xC0, 0xFF):
    c.text(x0 + e * sc, y0 + 6 + 5 * 44 + 4, "%02X" % e, size=9.5, color=MUTED, anchor="middle", mono=True)
c.legend(x0, y0 + 6 + 5 * 44 + 24, [("red", "entry differs from EOBPAL.COL"), ("teal", "index named by the set's colour tables A and B")])
c.h = y0 + 6 + 5 * 44 + 34
c.save("figs/gfx_palette_flow.svg")

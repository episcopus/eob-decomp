import sys; sys.path.insert(0, '.')
from figlib import *

c = Canvas(760, 362)
c.section(0, 11, "adlib.dat, 7,489 bytes, to scale")
TOTAL = 7489
X0, W = 0, 758
def px(off): return X0 + W * off / TOTAL
by, bh = 36, 44
segs = [
    (0, 120, "grey", "map"),
    (120, 720, "ochre", "table"),
    (720, 6092, "teal", "sound scripts: 149 sounds, 5,372 bytes"),
    (6092, 7489, "plum", "patches"),
]
for a, b, kind, lab in segs:
    tc, sc, fc = PAL[kind]
    c.rect(px(a), by, px(b) - px(a), bh, fill=fc, stroke=sc, sw=1.2, rx=0)
c.text((px(720) + px(6092)) / 2, by + 26, "sound scripts: 149 sounds, 5,372 bytes", size=12.5, weight=700, color=PAL["teal"][0], anchor="middle")
c.text((px(6092) + px(7489)) / 2, by + 20, "patches", size=12, weight=700, color=PAL["plum"][0], anchor="middle")
c.text((px(6092) + px(7489)) / 2, by + 34, "127 x 11", size=10.5, color=INK2, anchor="middle")
c.text(px(60), by + 24, "", size=10)
for off, lab, anc in [(0, "0", "start"), (720, "720", "middle"), (6092, "6092", "middle"), (7489, "7489", "end")]:
    c.text(px(off) if anc != "end" else 759, by - 5, lab, size=10.5, color=MUTED, anchor=anc, mono=True)

# panels
py, ph = 118, 188
def trap(a, b, x0, w):
    t = by + bh
    c.path(f"M{px(a):.1f},{t} L{x0:.1f},{py - 2} M{px(b):.1f},{t} L{x0 + w:.1f},{py - 2}", stroke=GREY, sw=0.8, dash="3 2")
PW = 236
xs = [0, 262, 524]
trap(0, 120, xs[0], PW); trap(120, 720, xs[1], PW); trap(720, 6092, xs[2], PW)
for kind, x, head in (("grey", xs[0], "map[120]: u8, file offsets 0 to 119"),
                      ("ochre", xs[1], "300 words, at block offset 0"),
                      ("teal", xs[2], "sound at block offset 3012 (133)")):
    c.card(x, py, PW, ph, kind, head, [])
# map panel
rows = [("1", "5"), ("20", "66  music conductor"), ("22", "75  death conductor"), ("75", "133  a monster's move"),
        ("79", "36  refused action"), ("70, 84", "FFh  ignored")]
yy = py + 38
c.text(xs[0] + 9, yy, "game number", size=10.5, color=MUTED); c.text(xs[0] + 92, yy, "driver sound", size=10.5, color=MUTED)
for g, d in rows:
    yy += 17
    c.text(xs[0] + 9, yy, g, size=11.5, mono=True, color=INK)
    c.text(xs[0] + 92, yy, d, size=11.5, color=INK2)
c.para(xs[0] + 9, yy + 22, "PlaySound ignores a driver number of 96h or more.", PW - 18, size=10.5, color=MUTED)
# table panel
c.rect(xs[1] + 9, py + 36, PW - 18, 34, fill=CARD, stroke=PAL["ochre"][1], sw=1, rx=3)
c.text(xs[1] + 17, py + 51, "words 0 to 149: sound i", size=11.5, weight=600, color=PAL["ochre"][0])
c.text(xs[1] + 17, py + 65, "offset of its script header", size=10.5, color=INK2)
c.rect(xs[1] + 9, py + 74, PW - 18, 34, fill=CARD, stroke=PAL["ochre"][1], sw=1, rx=3)
c.text(xs[1] + 17, py + 89, "words 150 to 299: patch p", size=11.5, weight=600, color=PAL["ochre"][0])
c.text(xs[1] + 17, py + 103, "offset of its 11 patch bytes", size=10.5, color=INK2)
c.para(xs[1] + 9, py + 128, "The first word is 600 (0258h): the table's own size, so every offset is from the start of the table. "
       "RebaseDriverBlock adds 0416h to all 300 words, so each becomes a near pointer in segment 0.", PW - 18, size=10.5, color=MUTED)
# sound panel
c.text(xs[2] + 9, py + 40, "2 header bytes, then the script", size=10.5, color=MUTED)
byts = ["02", "00", "AB", "0E", "83", "01", "90", "70", "87", "40", "91", "FF", "FF", "82", "01", "07", "88"]
cw = (PW - 18) / 9.0
for i, b in enumerate(byts):
    r, k = divmod(i, 9)
    x = xs[2] + 9 + k * cw; y = py + 48 + r * 24
    kind = "navy" if i < 2 else "paper"
    c.rect(x, y, cw - 2, 20, fill=PAL[kind][2], stroke=PAL[kind][1], sw=1, rx=2)
    c.text(x + (cw - 2) / 2, y + 14.5, b, size=11, mono=True, anchor="middle")
c.para(xs[2] + 9, py + 112, "voice 2, priority 0, then LEVEL2 0Eh, KEYOFF2 01, PATCH 70h, TRANSPOSE 40h, SLIDE, "
       "one note (octave 0, semitone 1) for 7 units, END. Section 5.14.2 explains each command.", PW - 18, size=10.5, color=MUTED)
c.para(0, 330, "pcgamesn.dat has the same map and table, but its sounds are song streams (5.14.4). INTRO.EXE reads sound.dat (42 sounds) or pcsound.dat in the same layout.", 758, size=10.5, color=MUTED)
c.save("figs/snd_files.svg")

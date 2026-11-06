import sys; sys.path.insert(0, '.')
from figlib import *

c = Canvas(760, 330)

def strip(y, total, parts, title, h=34):
    c.section(0, y - 8, title)
    x = 0.0
    W = 760.0
    for name, b, k, lab in parts:
        w = W * b / total
        c.rect(x, y, w, h, fill=MID[k] if k != "paper" else CARD, stroke=SOLID[k], sw=1)
        if w > 52:
            c.text(x + w / 2, y + 15, lab[0], size=10.5, weight=700, color=PAL[k][0], anchor="middle")
            c.text(x + w / 2, y + 28, lab[1], size=9.8, color=INK2, anchor="middle")
        x += w
    return y + h

# A: the load image
img = [("code", 127440, "navy", ("resident code", "127,440")),
       ("data", 8464, "ochre", ("", "")),
       ("DGROUP", 65508, "teal", ("DGROUP 212E", "65,508")),
       ("stack", 140, "grey", ("", ""))]
yb = strip(28, 201552, img, "A. The load image, 201,552 bytes (loaded at paragraph 1000h)")
# small labels for the thin parts
xdata = 760 * 127440 / 201552
c.line(xdata + 11, yb, xdata + 11, yb + 8, stroke=GREY)
c.text(xdata + 11, yb + 19, "other data segments and the 7 overlay stubs, 8,464", size=10, color=MUTED, anchor="middle")
c.text(758, yb + 19, "stack 128 (312D:0080)", size=10, color=MUTED, anchor="end")

# B: the far heap, fixed allocations (VGA)
heap = [("pool", 11456, "plum", ("overlay pool", "11,456")),
        ("drv", 11320, "ochre", ("MCGA.OVL", "11,320")),
        ("p65", 65000, "navy", ("page 2/3", "65,000")),
        ("p32", 32000, "navy", ("page 4/5", "32,000")),
        ("p52", 52000, "navy", ("page 6/7", "52,000")),
        ("lvl", 19316, "teal", ("level buffers", "19,316")),
        ("fnt", 3080, "grey", ("", "")),
        ("por", 8100, "red", ("", ""))]
tot = sum(h[1] for h in heap)
yh = strip(120, tot, heap, "B. The far heap after start-up, VGA, fixed blocks only: %s bytes" % format(tot, ","))
def callout(x, ytop, ylab, txt, anchor):
    c.line(x, ytop, x, ylab - 10, stroke=GREY, sw=0.9)
    c.text(x + (5 if anchor == 'start' else -5), ylab, txt, size=10, color=MUTED, anchor=anchor)

cum = 0
pos = []
for nm, b, k, lab in heap:
    pos.append((nm, 760 * (cum + b / 2) / tot))
    cum += b
P = dict(pos)
callout(P["pool"], yh, yh + 20, "overlay pool 11,456", "start")
callout(P["drv"], yh, yh + 34, "MCGA.OVL 11,320 (64,000 reserved, then shrunk)", "start")
callout(P["fnt"], yh, yh + 20, "fonts 2,312, palette 768", "end")
callout(P["por"], yh, yh + 34, "6 portrait buffers 7,800, icon 300", "end")
c.text(0, yh + 56, "Not counted: the sprite cells cut from the art sheets (icons, item sheets, compass, per-level monster and wall shapes) and", size=10.5, color=MUTED)
c.text(0, yh + 70, "the block headers. The heap runs from the end of the image to the top of conventional memory.", size=10.5, color=MUTED)

# C: header of a block
c.section(0, 256, "C. MemAlloc block headers (19BC:0007)")
y0 = 270
c.text(0, y0 + 16, "flags 0", size=11, weight=700, color=INK)
c.rect(70, y0, 36, 24, fill=MID["ochre"], stroke=SOLID["ochre"])
c.text(88, y0 + 16, "flags", size=10.5, anchor="middle", color=INK2)
c.rect(106, y0, 230, 24, fill=CARD, stroke=SOLID["teal"])
c.text(221, y0 + 16, "the block: size bytes", size=10.5, anchor="middle", color=INK2)
c.para(356, y0 + 10, "farmalloc(size + 1); the byte before the block holds the flags", 400, size=10.5, color=MUTED)
y1 = y0 + 44
c.text(0, y1 + 16, "flags 20h", size=11, weight=700, color=INK)
c.rect(70, y1, 36, 24, fill=PANEL, stroke=GREY, dash="3 2")
c.text(88, y1 + 16, "pad", size=10.5, anchor="middle", color=MUTED)
c.rect(106, y1, 20, 24, fill=MID["ochre"], stroke=SOLID["ochre"])
c.text(116, y1 + 16, "h", size=10.5, anchor="middle", color=INK2, weight=700)
c.rect(126, y1, 210, 24, fill=CARD, stroke=SOLID["teal"])
c.text(231, y1 + 16, "block at offset 0: size bytes", size=10.5, anchor="middle", color=INK2)
c.para(356, y1 + 10, "farmalloc(size + 16); the header h, (pad - 1) or flags, sits at offset 15 of the paragraph before the block", 400, size=10.5, color=MUTED)
c.h = 350
c.save("figs/arc_memory.svg")

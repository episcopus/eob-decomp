import sys; sys.path.insert(0, '.')
from figlib import *

c = Canvas(760, 470)
c.section(0, 11, "A call that crosses into an overlay (example: StartupAlloc calls LoadItemData)")

def step(x, y, w, h, kind, n, title, lines, mono_first=False):
    tc, sc, fc = PAL[kind]
    c.rect(x, y, w, h, fill=fc, stroke=sc, sw=1.2, rx=6)
    c.badge(x + 14, y + 16, str(n), kind, r=8)
    c.text(x + 29, y + 20, title, size=12.5, weight=700, color=tc)
    yy = y + 38
    for i, l in enumerate(lines):
        mono = l.startswith("~")
        if mono:
            l = l[1:]
        for t in wrap(l, w - 18, 11, mono):
            c.text(x + 9, yy, t, size=11 if not mono else 10.5, color=INK2, mono=mono)
            yy += 14

W = 176
step(0, 26, W, 128, "teal", 1, "caller", [
    "a far call to the stub entry:", "~9A 2A 00 06 21", "= call 2106:002A", "the return address is the caller's"])
step(194, 26, W, 128, "ochre", 2, "stub, segment 2106", [
    "overlay 2's entry table; each entry is 5 bytes:", "~CD 3F CC 09 00", "INT 3Fh, then the code offset 09CC"])
step(388, 26, W, 128, "navy", 3, "INT 3Fh", [
    "the game's hook 1A72:0006 checks EOB.EXE can be opened, then chains to the manager, 1E8B:04D0, which reads the word after the INT and finds the record"])
step(582, 26, W, 128, "plum", 4, "overlay code", [
    "runs at pool:09CC (LoadItemData, ovl2:09CC); its RETF goes back to the caller"])
for x in (178, 372, 566):
    c.arrow([(x + 1, 90), (x + 15, 90)], color=GREY)

# load path
c.section(0, 182, "When overlay 2 is not in memory (1E8B:0580, LoadOverlayRecord)")
steps = [
    ("age the pool", "1E8B:0536 drops or moves other overlays to make room"),
    ("read the image", "from EOB.EXE: FBOV data base + the record's offset (section 2.3)"),
    ("fix it up", "1E8B:03FD adds the pool segment to every fixup; segment-table indexes become paragraphs"),
    ("patch the stack", "1E8B:073B: return addresses into a moved overlay are rewritten"),
]
bw = 176
for i, (t, b) in enumerate(steps):
    x = i * (bw + 18)
    c.box(x, 192, bw, 78, "grey", t, [b], size=10.8, tsize=12, fill=CARD)
    if i < 3:
        c.arrow([(x + bw + 1, 231), (x + bw + 17, 231)], color=GREY)

# pool to scale
c.section(0, 296, "The overlay pool and the seven overlays, to scale (bytes of code)")
SX0, SW = 150, 560
scale = SW / 12000.0
c.text(0, 318, "pool: 716 paragraphs", size=11, weight=700, color=INK)
c.rect(SX0, 306, 11456 * scale, 16, fill=PANEL, stroke=GREY, sw=1.2, rx=2, dash="4 3")
c.text(SX0 + 11456 * scale + 6, 318, "11,456", size=10.5, color=MUTED)
ov = [("ovl0 character panel", 10873, "navy"), ("ovl1 camp, spells", 9864, "ochre"), ("ovl4 first-person view", 8259, "plum"),
      ("ovl5 NPCs, ending", 8094, "red"), ("ovl3 level load, save", 6961, "teal"), ("ovl2 data, title menu", 3332, "navy"),
      ("ovl6 rest", 3258, "ochre")]
y = 334
for name, b, k in ov:
    c.text(0, y + 11, name, size=11, color=INK2)
    c.rect(SX0, y + 2, b * scale, 12, fill=SOLID[k], rx=2)
    c.text(SX0 + b * scale + 6, y + 12, format(b, ","), size=10.5, color=MUTED)
    y += 18
c.text(0, y + 12, "Pool = DGROUP:3088 (715 paragraphs) + 1, doubled instead only if the biggest record (code + fixups, plus 2) reaches 715.", size=10.5, color=MUTED)
c.text(0, y + 26, "Overlay 0 is the biggest record: 705 paragraphs + 2. It nearly fills the pool by itself.", size=10.5, color=MUTED)
c.h = y + 36
c.save("figs/arc_overlay_call.svg")

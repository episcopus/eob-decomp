import sys; sys.path.insert(0, '.')
from figlib import *

c = Canvas(760, 372)
c.section(0, 11, "How the programs were built (every box proved by matching the bytes)")
def doc(x, y, w, h, kind, title, body):
    tc, sc, fc = PAL[kind]
    f = 10
    c.path(f"M{x},{y} L{x+w-f},{y} L{x+w},{y+f} L{x+w},{y+h} L{x},{y+h} Z", fill=fc, stroke=sc, sw=1.2)
    c.path(f"M{x+w-f},{y} L{x+w-f},{y+f} L{x+w},{y+f}", fill="none", stroke=sc, sw=1.2)
    c.text(x + w / 2, y + 22, title, size=12.5, weight=700, color=tc, anchor="middle", mono=True)
    c.lines(x + w / 2, y + 40, body, size=10.5, lh=13, color=INK2, anchor="middle")
doc(0, 30, 150, 70, "teal", "*.C", ["Westwood's C", "30 resident + 7 overlay units"])
doc(0, 120, 150, 70, "plum", "*.ASM", ["25 assembly units, 4 drivers", "sound, input, video, files"])
c.box(190, 30, 170, 70, "navy", "TCC 1.00", ["-ml -O -Z", "-v for the game's modules", "-G for 5 library units"], size=10.5, tsize=13, title_mono=True)
c.box(190, 120, 170, 70, "navy", "TASM 2.0", ["/mx, SMART off", "jmp short written out"], size=10.5, tsize=13, title_mono=True)
c.arrow([(152, 65), (188, 65)], color=SOLID["navy"])
c.arrow([(152, 155), (188, 155)], color=SOLID["navy"])
c.text(275, 207, "OBJ files, in the original's order", size=10.5, color=MUTED, anchor="middle")
c.arrow([(360, 65), (388, 65), (388, 226), (418, 226)], color=GREY)
c.arrow([(360, 155), (388, 155)], color=GREY)
c.rect(190, 232, 170, 36, fill=PAL["grey"][2], stroke=PAL["grey"][1], rx=6)
c.text(275, 255, "C0L.OBJ + CL.LIB + OVERLAY.LIB", size=10, weight=600, color=INK2, anchor="middle")
c.arrow([(360, 262), (418, 262)], color=GREY)
c.box(420, 200, 150, 100, "teal", "TLINK 3.0", ["segment table", "overlay stubs", "relocations", "FBOV records"], size=10.5, tsize=13, title_mono=True)
c.arrow([(572, 250), (610, 250)], color=SOLID["teal"])
doc(612, 200, 148, 100, "teal", "EOB.EXE", ["INTRO.EXE", "START1.EXE (small)", "= the originals"])
# right column: what is the same
c.panel(420, 30, 340, 150, fill=PANEL)
c.section(432, 50, "How each fact was found", color=PAL["ochre"][0], size=10)
rows = [
    "Release: 66 runtime modules found as 1.00's, 59 as 1.01's (make rtl-compare)",
    "Switches: eliminated by matching whole modules (section 2.1.1)",
    "Debug info: -v keeps code tails apart, as the original does",
    "Assembler: TASM 2.0 SMART off, one pass for the drivers",
    "Linker: TLINK 3.0, signature FB 30 at 1Eh in the header",
]
yy = 68
for r in rows:
    for l in wrap(r, 316, 10.5):
        c.text(432, yy, l, size=10.5, color=INK2); yy += 13.5
    yy += 3
y = c.para(0, 322, "Not in the picture: the Borland runtime library is linked, never decompiled (CL.LIB, found module by module in the originals).", 700, size=11, color=MUTED)
y = c.para(0, y + 3, "Differences left in the rebuilds: the link date written into the overlay data, and the order of the overlay stub entries.", 700, size=11, color=MUTED)
y = c.para(0, y + 3, "Only TCC -c (internal assembler) merges identical tails; the originals were built with debug information, which stops that.", 700, size=11, color=MUTED)
c.h = y + 6
c.save("figs/prog_toolchain.svg")

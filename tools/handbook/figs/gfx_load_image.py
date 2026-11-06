import sys; sys.path.insert(0, '.')
from figlib import *

c = Canvas(760, 400)
c.section(0, 11, "LOADING A PICTURE: LoadScreenImage (082C:051B) AND LoadPacked (1B82:0041)")

# pipeline
y = 36
boxes = [
    ("name + suffix", "sprintf \"%s.%s\": CPS if the choice is 3 (VGA), else EGA", "grey", 150),
    ("FileOpen, FileRead", "u16 length, then the packed block into page buffer srcKind (5, staging)", "teal", 190),
    ("Unpack", "by the format byte (4 LCW: all 47 .CPS, 42 of 48 .EGA; 1 LZW: 5 .EGA; 0 stored: 1 .EGA), into page buffer dstKind (3, art)", "teal", 210),
    ("CopyPage(dst, mode, map)", "unless mode is -1; map is the 32-byte CGA colour map, read by CGA.OVL only", "ochre", 180),
]
x = 0
for i, (t, b, k, w) in enumerate(boxes):
    c.box(x, y, w, 96, k, t, [b], size=10, tsize=11.5)
    if i:
        c.arrow([(x - 8, y + 48), (x, y + 48)], color=GREY)
    x += w + 8

# three ways the game calls it
y2 = 160
c.section(0, y2 - 10, "THE CALLS THE GAME MAKES")
rows = [
    ("LoadScreenImage(\"PLAYFLD\", map, 5, 3, 0)", "picture to the art page, then page 3 to page 0: the play screen is on the display (EnterMapScreen)", "teal"),
    ("LoadScreenImage(\"INVENT\", map, 5, 3, 2)", "picture to the art page, copied to page 2 (VGA: one segment, so no copy): the inventory panel waits there", "teal"),
    ("LoadOverlayScreen(name, 5, 3)", "mode -1, so nothing is copied; gDrawPage = 2. The art page now holds a sheet GetSpriteCell cuts shapes from", "ochre"),
    ("LoadPacked(file, 7, 3) + 3 x Unpack0", "wall textures: the .VCN goes to the art page, then the two colour tables and the 0xCB20 bytes of blocks are copied out of it into slot 7", "navy"),
]
yy = y2 + 6
for t, b, k in rows:
    c.rect(0, yy, 756, 40, fill=PAL[k][2], stroke=PAL[k][1], sw=1, rx=4)
    c.text(8, yy + 16, t, size=11, weight=700, color=PAL[k][0], mono=True)
    c.text(8, yy + 31, b, size=10.5, color=INK2)
    yy += 46

# special cases
c.text(0, yy + 12, "Two special cases inside LoadScreenImage: CGA and Tandy with mode 2 copy the art to page 4 and then page 4 to page 2 (CopyRegion 40 x 200);", size=10.5, color=MUTED)
c.text(0, yy + 26, "EGA with mode 0 copies the art to page 2 (video RAM, A200) and then page 2 to page 0 with CopyRegion, inside the video memory.", size=10.5, color=MUTED)
c.text(0, yy + 44, "The pictures are 320 x 200 with one byte a pixel; their palettes come from the .PAL and .COL files: the palette field of all 137 packed files read here is 0.", size=10.5, color=MUTED)
c.h = yy + 54
c.save("figs/gfx_load_image.svg")

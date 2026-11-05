import sys; sys.path.insert(0, '.')
from figlib import *

c = Canvas(760, 470)
c.section(0, 11, "ONE VIEW FRAME: DrawViewFrame (ovl4:1F51)")

LX, LW = 0, 330
steps = [
    ("1", "teal", "BuildViewBlockMap(gPartyCell, gPartyDir)", ["18 cells, the two-layer 22 x 15 block map at ds:ADB0"]),
    ("2", "teal", "DrawTile(gWallBlocks)", ["each block word to 8 x 8 pixels, written to page 2 at (0, 0): 176 x 120"]),
    ("3", "ochre", "DrawViewSlots(0)", ["18 slots, far to near; sprites go onto page 2 on top of the walls"]),
    ("4", "navy", "palette, if one is pending", ["VGA only: gSetPalette(gPaletteBuf); with full set, the view is blanked first"]),
    ("5", "grey", "wait for gFrameWaitTicks == 0", ["a countdown the INT 1Ch tick handler runs down"]),
    ("6", "red", "CopyRegion(0, 0, 0, 0, 22, 120, 2, 0)", ["page 2 to page 0 (the screen), between HidePointerZone and ShowPointerZone"]),
    ("7", "grey", "sound 0, DrawCompass, gViewDirty = 0", ["the compass shapes are redrawn only when the facing changed"]),
]
y = 26
H = 48
pos = []
for n, k, t, b in steps:
    c.box(LX + 22, y, LW - 22, H, k, t, b, size=10.5, tsize=11.5, title_mono=False)
    c.badge(LX + 10, y + H / 2, n, k, r=9)
    pos.append(y)
    if y > 26:
        c.arrow([(LX + 22 + (LW - 22) / 2, y - 8), (LX + 22 + (LW - 22) / 2, y)], color=GREY)
    y += H + 8
bottom = y

# detail of step 3
DX = 372
c.rect(DX, pos[2] - 24, 388, 262, fill=PANEL, stroke="none", rx=8)
c.text(DX + 10, pos[2] - 8, "step 3, for each slot s in gViewDrawOrder", size=12, weight=700, color="#7A5412")
c.arrow([(LX + LW, pos[2] + H / 2), (DX, pos[2] + H / 2)], color=PAL["ochre"][1])
items = [
    ("ComputeViewSpan(s)", "clip region 5 to the columns not hidden by nearer walls; stop if empty", "teal"),
    ("DrawSlotDecorations(s)", "wall decorations of the slot's two faces", "teal"),
    ("DrawSlotItems(s)", "objects lying in the cell (item list at record +5)", "ochre"),
    ("DrawSlotDoor(s)", "slots 0 to 14, when the wall type has flag 8 (a door)", "ochre"),
    ("DrawSlotMonsters(s)", "slots 0 to 14, when record +4 has bits 0 to 2 set", "plum"),
    ("DrawSlotProjectiles(s)", "thrown items and spell missiles in the cell", "plum"),
    ("DrawSlotTeleporter(s)", "only when the wall type is 34h", "grey"),
]
yy = pos[2]
for t, b, k in items:
    c.box(DX + 10, yy, 368, 28, k, None, None)
    c.text(DX + 18, yy + 12, t, size=11, weight=700, color=PAL[k][0], mono=False)
    c.text(DX + 18, yy + 24, b, size=9.8, color=INK2)
    yy += 31
c.text(DX + 10, yy + 12, "every sprite goes through DrawShapeClipped(2, shape, x, y, 5): page 2,", size=10.5, color=MUTED)
c.text(DX + 10, yy + 26, "clipped to region 5, the rectangle ComputeViewSpan just set", size=10.5, color=MUTED)
c.h = max(bottom, pos[2] - 24 + 262) + 8
c.save("figs/gfx_frame_flow.svg")

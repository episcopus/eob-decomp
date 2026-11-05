import sys; sys.path.insert(0, '.')
from figlib import *

c = Canvas(760, 430)
c.section(0, 11, "THE SOFTWARE MOUSE POINTER: SAVE UNDER, DRAW, RESTORE")

# left lane: mouse event
LX, LW = 0, 360
c.text(LX, 34, "the mouse moves (INT 33h handler, 1A72:017A)", size=12, weight=700, color=PAL["navy"][0])
left = [
    ("clamp x to 319, publish gCursorX, gCursorY", "only when gPointerBusy is 0; otherwise the event draws nothing", "grey"),
    ("moved, and gHideDepth is 0?", "else nothing to redraw", "grey"),
    ("SaveDrawState(), HidePointer()", "HidePointer puts the saved rectangle back through gRestoreRect (ds:804E), gHideDepth = 1", "navy"),
    ("ShowPointer()", "gHideDepth = 0, gDrawnX/Y = cursor; gSaveRect (ds:80AE) copies the screen under the new place into the save-under buffer; gDrawShape (ds:80C2) draws the pointer shape on page 0", "navy"),
    ("RestoreDrawState()", "the driver's region (and, on EGA, its registers) are put back", "grey"),
]
y = 44
for i, (t, b, k) in enumerate(left):
    h = 44 if len(b) < 90 else 72
    c.box(LX, y, LW, h, k, t, [b], size=10, tsize=11.5)
    if i:
        c.arrow([(LX + LW / 2, y - 8), (LX + LW / 2, y)], color=GREY)
    y += h + 8
leftbottom = y

# right lane: the game draws
RX, RW = 400, 360
c.text(RX, 34, "the game draws on the screen (page 0)", size=12, weight=700, color=PAL["plum"][0])
right = [
    ("HidePointerZone(l, t, r, b)", "or LockHidePointer() for no zone. The zone is stored; if the pointer's rectangle overlaps it, the pointer is hidden as above and 4000h (redraw owed) is set", "plum"),
    ("the drawing", "DrawText, gFillRect, CopyRegion, DrawShape: the driver writes the screen. While the zone hide is on, the mouse handler does not redraw inside the zone (its left edge is not tested, QUIRK 1A72:0248)", "grey"),
    ("ShowPointerZone()", "or LockShowPointer(): gHideDepth goes down; at 0, with a redraw owed, ShowPointer saves the new background and draws the pointer again", "plum"),
]
y = 44
for i, (t, b, k) in enumerate(right):
    h = 72
    c.box(RX, y, RW, h, k, t, [b], size=10, tsize=11.5)
    if i:
        c.arrow([(RX + RW / 2, y - 8), (RX + RW / 2, y)], color=GREY)
    y += h + 8
# state box
sy = y + 10
c.rect(RX, sy, RW, 148, fill=PANEL, stroke="none", rx=8)
c.text(RX + 10, sy + 16, "state the two lanes share", size=11.5, weight=700, color=INK)
lines = [
    ("gHideDepth ds:8146", "0 drawn; counts nested hides, saturates at FFFFh"),
    ("gPointerState ds:8160", "8000h zone hide on; 4000h redraw owed; low byte a repeat count"),
    ("gPointerBusy ds:812E", "taken (test, then increment, not a lock) by every wrapper"),
    ("save-under buffer ds:816A", "cols x rows x 8 bytes (SaveRectSize); the pointer's rectangle is (shape cols + 1) columns wide, rounded to 8 pixels"),
]
yy = sy + 31
for a, b in lines:
    c.text(RX + 10, yy, a, size=10, weight=700, color=INK2, mono=False)
    ls = wrap(b, 180, 10)
    for l in ls:
        c.text(RX + 170, yy, l, size=10, color=INK2); yy += 12.5
    yy += 3
c.h = max(leftbottom, sy + 156) + 4
c.save("figs/gfx_pointer.svg")

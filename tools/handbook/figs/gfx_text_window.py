import sys; sys.path.insert(0, '.')
from figlib import *

c = Canvas(760, 360)
c.section(0, 11, "THE TEXT WINDOW: WHAT DrawText DOES WITH A STRING")

# left: window geometry
wx, wy, ww, wh = 4, 44, 270, 100
c.rect(wx, wy, ww, wh, fill="#FFFDF8", stroke=PAL["teal"][1], sw=1.4, rx=0)
fh = 12
for r in range(8):
    y = wy + r * fh
    if r < 7:
        c.line(wx, y + fh, wx + ww, y + fh, stroke="#EFE9DA", sw=0.5)
c.text(wx + 4, wy + 10, "A text window is one row of", size=9.5, color=INK2)
c.text(wx + 4, wy + 22, "the region table: left in 8-pixel", size=9.5, color=INK2)
c.text(wx + 4, wy + 34, "columns, top in pixel lines,", size=9.5, color=INK2)
c.text(wx + 4, wy + 46, "width in columns, height in lines.", size=9.5, color=INK2)
c.rect(wx + 4 + 6 * 11, wy + 2 * fh + 3 * 12 - 8, 6, 12, fill=SOLID["red"], stroke="none", rx=0, opacity=0.8)
c.text(wx + 4, wy + wh - 6, "the cursor is (col, row) in font cells", size=9.5, color=PAL["red"][0])
c.text(wx, wy - 6, "left edge gTextLeft x 8, right edge (gTextLeft + gTextWidth) x 8", size=9.5, color=MUTED)
yy = wy + wh + 20
for t in [
    "pixel x = gTextLeft * 8 + gFontWidth * col; y = gTextTop + gFontHeight * row; the window spans gTextTop to gTextTop + gTextHeight",
    "colours: gTextAttr1 (text) and gTextAttr2 (fill); the window saves them, with the cursor, in its region record when another window is selected (SetTextWindow)",
    "when the next line would pass the bottom: CopyRegion scrolls the window up one font height, the last line is filled with gTextAttr2 and drawn there",
    "the pointer is hidden over the window for every line (HidePointerZone / ShowPointerZone)",
]:
    for l in wrap(t, 270, 10.5):
        c.text(0, yy, l, size=10.5, color=INK2); yy += 13
    yy += 4

# right: control codes
RX = 300
c.text(RX, 34, "bytes DrawText (1DE7:060A) treats specially", size=12, weight=700, color=INK)
rows = [
    ("0Dh", "end the line, start the next (a word that no longer fits is moved to the next line)"),
    ("20h", "word break: a space is kept only if there is room, otherwise the line is flushed"),
    ("02h n", "set the fill colour gTextAttr2 to the next byte; the line so far is drawn first"),
    ("06h n", "set the text colour gTextAttr1 to the next byte"),
    ("09h n", "move the cursor to column n (column 0 if n is past the right edge)"),
    ("13h n", "pad with spaces up to column n"),
    ("bit 7", "the byte is used with bit 7 cleared and ends the string"),
    ("other", "added to the current word; a word as wide as the window is flushed on its own line"),
]
y = 46
for code, t in rows:
    ls = wrap(t, 300, 10.5)
    h = len(ls) * 12.5 + 8
    c.rect(RX, y, 456, h, fill=CARD, stroke=RULE, sw=1, rx=3)
    c.text(RX + 8, y + 15, code, size=11, weight=700, color=PAL["teal"][0], mono=True)
    for j, l in enumerate(ls):
        c.text(RX + 100, y + 15 + j * 12.5, l, size=10.5, color=INK2)
    y += h + 3
c.text(RX, y + 12, "The driver's own gDrawText draws one string at pixel (x, y): CR starts a new line at x,", size=10.5, color=MUTED)
c.text(RX, y + 25, "a character past the right edge wraps, background colour 0 is transparent.", size=10.5, color=MUTED)
c.h = max(yy, y + 34) + 4
c.save("figs/gfx_text_window.svg")

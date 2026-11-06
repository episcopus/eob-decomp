import sys; sys.path.insert(0, '.')
from figlib import *

c = Canvas(760, 340)
def card(x, y, w, title, sub, k, rows):
    tc, sc, fc = PAL[k]
    h = (44 if sub else 32) + 24 * len(rows) + 6
    c.rect(x, y, w, h, fill=fc, stroke=sc, sw=1.2, rx=6)
    c.text(x + 10, y + 19, title, size=12.5, weight=700, color=tc)
    if sub:
        c.text(x + 10, y + 33, sub, size=9.6, color=INK2)
    yy = y + (42 if sub else 30)
    pos = []
    for lab, key in rows:
        c.rect(x + 8, yy, w - 16, 21, fill=CARD, stroke=sc, sw=0.8, rx=3)
        c.text(x + 14, yy + 14.5, lab, size=10.6, weight=600, color=INK)
        c.text(x + w - 14, yy + 14.5, key, size=10.6, weight=700, color=tc, anchor="end", mono=True)
        pos.append((x, yy + 10.5, x + w))
        yy += 24
    return pos, h

p0, h0 = card(0, 0, 250, "Camp (menu 0)", "C or the camp icon: RunCampMenu, ovl1:1ACD", "ochre", [
    ("Rest Party", "R"), ("Memorize Spells", "M"), ("Pray for Spells", "P"), ("Scribe Scrolls", "S"),
    ("Preferences", "-"), ("Game Options", "G"), ("Exit", "E")])
p2, h2 = card(300, 0, 210, "Preferences (menu 2)", "", "teal", [
    ("Tunes are ON", "T"), ("Sounds are ON", "S"), ("Bar Graphs are ON", "B"), ("Mouse is ON", "M"), ("Exit", "E")])
p1, h1 = card(300, h2 + 14, 210, "Game Options (menu 1)", "", "navy", [
    ("Load Game", "L"), ("Save Game", "S"), ("Drop Character", "D"), ("Quit Game", "Q"), ("Exit", "E")])
c.arrow([(251, p0[4][1]), (270, p0[4][1]), (270, p2[0][1] - 20), (298, p2[0][1] - 20)], color=GREY)
c.arrow([(251, p0[5][1]), (284, p0[5][1]), (284, p1[0][1] - 18), (298, p1[0][1] - 18)], color=GREY)
# helpers
p3, h3 = card(560, 0, 200, "Boxes it opens", "", "plum", [("Member picker, F1 to F6", ""), ("Spell slot screen", ""), ("Yes / No: Y N", ""), ("Message box", "")])
c.text(560, h3 + 18, "Memorize, Pray, Scribe and Drop go", size=10, color=INK2)
c.text(560, h3 + 31, "through the member picker; Load, Save", size=10, color=INK2)
c.text(560, h3 + 44, "and Quit through Yes / No; results,", size=10, color=INK2)
c.text(560, h3 + 57, "failures and hints through the message", size=10, color=INK2)
c.text(560, h3 + 70, "box (a key or about 120 frames).", size=10, color=INK2)
yb = max(h0, h2 + 14 + h1) + 22
c.section(0, yb, "The menu machinery")
lines = [
    "Each menu is a node list built by BuildMenuButtons (ovl1:1153) from 5-byte menu records (ds:628C) and 9-byte button definitions (208E:0000).",
    "RunCampMenu loops on DispatchKeyWait over that list: the letter on a button is its key06 word, Home and End (keypad 7, 1) light the first and last",
    "button, Up and Down move the lit one, Enter, Space or keypad 5 press it, a click presses the button under the pointer. Esc is remapped to Exit.",
    "Game time is stopped while it runs (gTimerEnableFlags bit 1 cleared); on the way out the view frame and the INVENT sheet are redrawn.",
]
for i, l in enumerate(lines):
    c.text(0, yb + 20 + 14 * i, l, size=10.4, color=INK2)
c.h = yb + 20 + 14 * len(lines) + 4
c.save("figs/arc_menus.svg")

import sys; sys.path.insert(0, '.')
from figlib import *

c = Canvas(760, 520)
# title
c.box(215, 0, 330, 54, "teal", "Title menu", ["LoadGamePanel, ovl2:0846"], size=11.5)
c.box(600, 0, 160, 54, "red", "Exit to DOS", ["ShutdownGame, exit(0)"], size=11)
c.arrow([(547, 27), (598, 27)], color=SOLID["red"])
c.text(572, 20, "entry 2", size=10.5, color=MUTED, anchor="middle")
# new party / load
c.box(0, 108, 290, 56, "plum", "New party", ["RunCharacterPanel, ovl0:28F9", "race, class, alignment, stats, name x4"], size=11)
c.box(470, 108, 290, 56, "plum", "Load a game", ["LoadGame, ovl3:15B9", "EOBDATA.SAV in, LEVELS.TMP out"], size=11)
c.arrow([(300, 55), (150, 106)], color=GREY, label="entry 1", ldx=-34, ldy=-8)
c.arrow([(460, 55), (615, 106)], color=GREY, label="entry 0", ldx=34, ldy=-8)
c.arrow([(615, 166), (615, 226)], color=GREY, label="ok", ldx=14, ldy=3)
c.arrow([(150, 166), (150, 226)], color=GREY)
# map
mx, my, mw, mh = 0, 226, 760, 140
tc, sc, fc = PAL["navy"]
c.rect(mx, my, mw, mh, fill="#F1F3FA", stroke=sc, sw=1.4, rx=8)
c.text(10, my + 19, "Map screen: RunMapLoop, 0ACD:0E8A, never returns", size=13, weight=700, color=tc)
subs = [
    ("Map view", "list 1, mode 0"),
    ("Inventory", "list 2, mode 1"),
    ("Character sheet", "list 3, mode 2"),
    ("Spell panel", "tables a, gFlag 1"),
    ("Swap members", "list 4"),
    ("Cast target", "list 5"),
]
sw_, sh_ = 115, 46
for i, (t, b) in enumerate(subs):
    x = 10 + i * 125
    c.box(x, my + 36, sw_, sh_, "navy", t, [b], size=10.5, tsize=12, fill=CARD)
c.text(10, my + 106, "The key and mouse regions are the node list that gPanelMode and gFlag select (section 5.10);", size=11.5, color=INK2)
c.text(10, my + 122, "every pass also runs the timed effects, the redraw and the sound hooks (section 3.1).", size=11.5, color=INK2)
# lower row
c.box(0, 420, 240, 60, "ochre", "Camp menu", ["RunCampMenu, ovl1:1ACD", "rest, spells, options, load, save, quit"], size=11)
c.box(260, 420, 240, 60, "ochre", "Event panels", ["CharOverlay, ovl5:1A82", "temple, NPCs, ending, riddle"], size=11)
c.box(520, 420, 240, 60, "grey", "Level change", ["LoadLevelOp: SaveLevelState,", "then LoadLevel, ovl3:0A7C"], size=11)
c.arrow([(110, 368), (110, 418)], color=GREY, both=True)
c.text(120, 396, "C key or the camp icon", size=10.5, color=MUTED)
c.arrow([(380, 368), (380, 418)], color=GREY, both=True)
c.text(390, 396, "a cell script asks for it", size=10.5, color=MUTED)
c.arrow([(640, 368), (640, 418)], color=GREY, both=True)
c.text(650, 396, "stairs and scripts", size=10.5, color=MUTED)
c.text(0, 505, "Ways out to DOS (red): the title menu's third entry, Quit Game in the camp menu, a party with nobody left, the ending, any fatal error.", size=11, color=MUTED)
c.h = 515
c.save("figs/arc_states.svg")

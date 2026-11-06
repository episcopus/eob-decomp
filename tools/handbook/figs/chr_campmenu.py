import sys; sys.path.insert(0, '.')
from figlib import *

c = Canvas(760, 480)
c.section(0, 11, "The camp menu: RunCampMenu (ovl1:1ACD) and what each button does")

# ---- menu switching ----
c.box(260, 24, 120, 34, "teal", "Camp", [], tsize=13)
c.box(60, 24, 130, 34, "navy", "Game Options", [], tsize=12.5)
c.box(450, 24, 130, 34, "navy", "Preferences", [], tsize=12.5)
c.arrow([(260, 36), (190, 36)], color=GREY, both=True)
c.arrow([(380, 36), (450, 36)], color=GREY, both=True)
c.box(260, 70, 120, 30, "grey", "leave the camp", [], tsize=11.5)
c.arrow([(320, 58), (320, 70)], color=GREY)
c.text(328, 66, "Exit", size=9.5, color=MUTED)
c.text(0, 120, "Exit in the two sub-menus returns to Camp. The game clock is stopped while the menu is open (gTimerEnableFlags bit 1 cleared).", size=10.5, color=INK2)

def table(y, title, kind, rows):
    tc, sc, fc = PAL[kind]
    c.rect(0, y, 760, 20 + 20 * len(rows), fill=fc, stroke=sc, sw=1.2, rx=6)
    c.text(10, y + 15, title, size=12, weight=700, color=tc)
    for i, (b, d) in enumerate(rows):
        yy = y + 36 + i * 20
        if i % 2:
            c.rect(2, yy - 13, 756, 20, fill="#FFFFFF", stroke="none", rx=0, opacity=0.35)
        c.text(14, yy, b, size=10.5, weight=700, color=INK)
        c.text(190, yy, d, size=10.5, color=INK2)
    return y + 20 + 20 * len(rows)

y = table(134, "Camp (menu 0)", "teal", [
    ("Rest Party", "CampLoop (overlay 6). A nonzero result ends the camp; otherwise gNeedsRest is cleared"),
    ("Memorize Spells", "SelectPartyMember(17h), then SpellSlotScreen(who, 0): the mage's spell book"),
    ("Pray for Spells", "SelectPartyMember(1Ah), then SpellSlotScreen(who, 1): a cleric's or paladin's list"),
    ("Scribe Scrolls", "ScribeScrolls (ovl1:1691): copy carried mage scrolls into a spell book, one at a time"),
    ("Preferences, Game Options", "open the sub-menu"),
    ("Exit", "leave; first the message 'Your party needs to rest to gain spells.' if gNeedsRest is set"),
])
y = table(y + 12, "Game Options (menu 1)", "navy", [
    ("Load Game, Save Game", "AskYesNo, then LoadGame (210C:0039) or SaveGame (210C:0089); Save writes EOBDATA.SAV"),
    ("Drop Character", "refused with fewer than 5 members present ('less than four characters'); else DropCharacter"),
    ("Quit Game", "AskYesNo(2Eh), then ShutdownGame"),
])
y = table(y + 12, "Preferences (menu 2)", "navy", [
    ("Tunes, Sounds", "flip gTunesOn and gSoundOn and rewrite the button's ON/OFF label"),
    ("Bar Graphs", "flip gBarGraphsOn (HP bar or the 'n of m' text) and redraw the party"),
    ("Mouse", "flip gMouseDisabled, only if a mouse driver was found"),
])
c.text(0, y + 18, "Keys 5Bh/65h light the first button, 60h/62h move up and down the selectable ones, 5Dh/67h the last; 2Bh/3Dh/61h press the lit one.", size=10, color=MUTED)
c.save("figs/chr_campmenu.svg")

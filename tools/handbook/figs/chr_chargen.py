import sys; sys.path.insert(0, '.')
from figlib import *

c = Canvas(760, 500)
c.section(0, 11, "Character creation: RunCharacterPanel and CreateCharacter (overlay 0, src/EOV0.C)")

# ---------- row 1: the panel loop ----------
c.box(0, 26, 214, 84, "navy", "RunCharacterPanel", ["ovl0:28F9. Four name slots; gChargenMember is the current one."], size=11, tsize=13, title_mono=True)
c.box(260, 22, 214, 42, "teal", "slot empty: CreateCharacter", [], size=11, tsize=12.5)
c.box(260, 72, 214, 42, "grey", "slot named: ViewCharacter", [], size=11, tsize=12.5)
c.text(267, 54, "", size=10)
c.arrow([(214, 50), (260, 43)], color=SOLID["navy"])
c.arrow([(214, 82), (260, 93)], color=SOLID["navy"])
c.box(520, 26, 220, 84, "paper", "the loop ends", ["when PLAY is chosen, on the panel or in ViewCharacter, and all four slots hold a name"], size=11, tsize=12.5)
c.arrow([(474, 93), (520, 82)], color=GREY)

# ---------- row 2: the creation sequence ----------
Y = 176
c.section(0, Y - 18, "CreateCharacter (ovl0:23EC), one member")
bx = [0, 150, 300, 450, 600]
bw = 136
steps = [
    ("navy", "PickRace", ["12 entries: 6 races, each male and female"]),
    ("navy", "PickClass", ["classes the race allows; no paladin if an evil member exists"]),
    ("navy", "PickAlignment", ["alignments the class allows; no evil if a paladin exists"]),
    ("teal", "RollCharacterStats", ["levels, XP, six scores, AC, hit points"]),
    ("ochre", "ReviewRolledStats", ["roll again, edit, portrait; then the name"]),
]
for (k, t, b), x in zip(steps, bx):
    c.box(x, Y, bw, 104, k, t, b, size=10.5, tsize=11.5)
for i in range(4):
    c.arrow([(bx[i] + bw, Y + 52), (bx[i + 1], Y + 52)], color=GREY)
c.arrow([(bx[1] + 50, Y + 104), (bx[1] + 50, Y + 122), (bx[0] + 68, Y + 122), (bx[0] + 68, Y + 104)], color=SOLID["red"], dash="4 3")
c.arrow([(bx[2] + 50, Y + 104), (bx[2] + 50, Y + 122), (bx[1] + 90, Y + 122), (bx[1] + 90, Y + 104)], color=SOLID["red"], dash="4 3")
c.text(bx[0] + 5, Y + 138, "back: icon 5 (key 6Eh) in the class and alignment lists returns one step", size=10.5, color="#80311F")

# ---------- row 3: what review offers ----------
R = 350
c.section(0, R - 12, "ReviewRolledStats (ovl0:0DDC): four buttons, repeated until PLAY")
btn = [("icon 4", "roll again", "RollCharacterStats runs again, any number of times"),
       ("icon 7", "modify", "EditStatField: change each value inside its limits"),
       ("icon 8", "portrait", "PickPortrait: 44 faces, male and female ranges"),
       ("icon 6", "play", "leave the review and ask for the name")]
for i, (ic, t, d) in enumerate(btn):
    x = i * 187
    c.box(x, R, 174, 56, "paper", t, [d], size=10, tsize=12)
    c.pill(x + 6, R + 70, ic, "ochre", size=10)

# ---------- row 4: leaving the panel ----------
L = 458
c.text(0, L - 12, "Leaving the panel (all four named, PLAY):", size=11.5, weight=700, color="#14504A")
tail = ["GiveStartingEquipment", "GiveStartingSpellBooks", "SetBaseStats", "FreePanelBuffers"]
tx = 0
for i, t in enumerate(tail):
    w = text_width(t, 11, True) + 20
    c.rect(tx, L, w, 24, fill="#DCEBE7", stroke="#1F6F66", sw=1.2, rx=5)
    c.text(tx + w / 2, L + 16, t, size=11, color="#14504A", anchor="middle", mono=True, weight=600)
    if i < 3:
        c.arrow([(tx + w, L + 12), (tx + w + 14, L + 12)], color=GREY)
    tx += w + 16
c.save("figs/chr_chargen.svg")

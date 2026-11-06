import sys, json; sys.path.insert(0, '.')
from figlib import *

T = json.load(open('data/snd_timeline.json'))
c = Canvas(760, 452)
c.section(0, 11, "The intro on the clock of INT 1Ch: 2,723 ticks (149 s in the oracle with AdLib)")
X0, W, TOT = 70, 688, 2723
def px(t): return X0 + W * t / TOT

# ruler
y = 40
c.line(X0, y + 14, X0 + W, y + 14, stroke=GREY, sw=1)
for t in range(0, 2724, 500):
    c.line(px(t), y + 10, px(t), y + 18, stroke=GREY, sw=1)
    c.text(px(t), y + 4, str(t), size=10.5, color=MUTED, anchor="middle")
c.text(X0 + W, y + 4, "ticks", size=10.5, color=MUTED, anchor="end") if False else None
for s in (30, 60, 90, 120):
    t = s * 18.2065
    c.line(px(t), y + 18, px(t), y + 24, stroke=RULE, sw=1)
    c.text(px(t), y + 36, "%d s" % s, size=10.5, color=MUTED, anchor="middle")

# lane 1: scenes
scenes = [
    (18, 164, "teal"), (164, 250, "navy"), (250, 350, "teal"), (350, 437, "navy"), (437, 577, "teal"),
    (577, 735, "ochre"), (735, 901, "plum"), (901, 1036, "red"), (1036, 1373, "teal"), (1373, 1544, "navy"),
    (1544, 1899, "ochre"), (1899, 2385, "plum"), (2385, 2723, "red"),
]
ly = 100
c.text(0, ly + 20, "scenes", size=11.5, weight=700, color=INK2)
for i, (a, b, kind) in enumerate(scenes):
    tc, sc, fc = PAL[kind]
    c.rect(px(a), ly, px(b) - px(a) - 1, 32, fill=fc, stroke=sc, sw=1.1, rx=3)
    c.text((px(a) + px(b)) / 2, ly + 21, str(i + 1), size=11.5, weight=700, color=tc, anchor="middle")
# lane 2: music
my = 150
c.text(0, my + 18, "music", size=11.5, weight=700, color=INK2)
pieces = [(24, 24 + 27 * 18.2065, "piece 1"), (583, 2160, "piece 2"), (2191, 2723, "piece 3")]
for a, b, lab in pieces:
    tc, sc, fc = PAL["plum"]
    c.rect(px(a), my, px(b) - px(a), 26, fill=PAL["plum"][2], stroke=sc, sw=1.1, rx=3)
    c.text((px(a) + px(b)) / 2, my + 17, lab, size=11, weight=600, color=tc, anchor="middle")
# lane 3: sound effects
ey = 196
c.text(0, ey + 16, "effects", size=11.5, weight=700, color=INK2)
music_ids = {1, 11, 21}
for t, sid in T["queue"]:
    if sid in music_ids: continue
    c.line(px(t), ey, px(t), ey + 24, stroke=PAL["ochre"][1], sw=1.6)
# lane 4: register writes per second
wy = 232
c.text(0, wy + 16, "OPL2", size=11.5, weight=700, color=INK2)
c.text(0, wy + 30, "writes/s", size=10.5, color=MUTED)
for s_, v in T["bins"].items():
    s = int(s_)
    t = s * 18.2065
    h = 52 * v / 210.0
    c.rect(px(t), wy + 52 - h, W / 150.0 * 0.92, h, fill=SOLID["teal"], stroke="none", rx=0)
c.line(X0, wy + 52, X0 + W, wy + 52, stroke=GREY, sw=1)
c.text(X0 + W, wy + 70, "top of the scale: 210 writes in a second", size=10.5, color=MUTED, anchor="end")

# legend of scenes
names = [
    "1 WESTWOOD card, 140 ticks", "2 AND card", "3 SSI card", "4 PRESENT card", "5 DAND card",
    "6 title card (TITLE-V, TEXT)", "7 tower mage", "8 orb", "9 corridor (WTRDP1 to 3)", "10 kings (KING, ADVENTUR)",
    "11 hands (HANDS)", "12 tunnel (WTRDP3, ORB, TUNNEL)", "13 avalanche (AVALANCH, ZOOMTUNL)",
]
ly2 = 330
cols = [0, 260, 510]
for i, n in enumerate(names):
    col, row = divmod(i, 5)
    c.text(cols[col], ly2 + row * 15, n, size=10.5, color=INK2)
c.para(0, ly2 + 5 * 15 + 12, "Sound events are the QueueSound calls of an oracle run (INTRO.EXE '3 1 0 homie'). Piece 2's end is where its register writes stop in that run; "
       "the oracle charges for every status-port read, so its music runs about 14 percent slower than the script timing says (76 s, 5.14.3).", 758, size=10.5, color=MUTED)
c.save("figs/snd_intro_timeline.svg")

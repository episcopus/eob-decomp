import sys; sys.path.insert(0, '.')
from figlib import *

c = Canvas(760, 428)
c.section(0, 11, "INTRO.EXE: what runs, and where the code lives")

# control flow row
y = 30
steps = [
    ("Main", "0892:0000", "args, hooks, finds EOB.EXE"),
    ("StartupGame", "0892:0241", "mode, MCGA.OVL, pages, font, sound"),
    ("RunIntro", "ovl0:0000", "stub entry 16 of overlay 0"),
    ("PlayLogoSequence", "ovl0:2A99", "WESTWOOD, AND, SSI, PRESENT, DAND"),
]
w, gap = 172, 24
for i, (n, a, d) in enumerate(steps):
    x = i * (w + gap) - (0 if i < 4 else 0)
    c.box(x, y, w - 14, 64, "teal" if i < 2 else "navy", n, [a, d], size=10, tsize=12, title_mono=True)
    if i:
        c.arrow([((i - 1) * (w + gap) + (w - 14) + 1, y + 32), (x - 1, y + 32)], color=GREY)
y2 = 112
c.box(0, y2, 186, 52, "navy", "PlayIntroSequence", ["ovl0:003A: title card, TEXT, then 7 scenes"], size=10, tsize=12, title_mono=True)
c.arrow([(3 * (w + gap) + (w - 14) / 2, y + 64), (3 * (w + gap) + (w - 14) / 2, y2 - 8), (93, y2 - 8), (93, y2 - 1)], color=GREY)
scs = ["TowerMage", "Orb", "Wtrdp", "King", "Hands", "Tunnel", "Avalanche"]
sx = 206
for i, n in enumerate(scs):
    c.box(sx + i * 79, y2, 74, 52, "plum", n, ["scene"], size=10, tsize=11)
    if i == 0:
        c.arrow([(186, y2 + 26), (sx - 1, y2 + 26)], color=GREY)
    else:
        c.arrow([(sx + (i - 1) * 79 + 74, y2 + 26), (sx + i * 79 - 1, y2 + 26)], color=GREY)
c.arrow([(sx + 6 * 79 + 37, y2 + 52), (sx + 6 * 79 + 37, 208), (331, 208)], color=GREY)
c.box(0, 184, 330, 48, "teal", "ShutdownGame, then execl(EOB.EXE ...)", ["0892:0835; argv[1] to argv[4] and two numbers; section 2.6"], size=10, tsize=12)

# three columns: where the code is
cy = 252
cols = [
    ("navy", "overlay 0: IOV0.C, 11,741 bytes, 22 functions", [
        "3 entry points: RunIntro, PlayLogoSequence, PlayIntroSequence",
        "7 scenes: Wtrdp, Orb, TowerMage, King, Hands, Tunnel, Avalanche",
        "12 helpers: WaitFrameDelay, DelayFrames, LoadPackedImage, LoadPalette, Dissolve, DrawGrid, BlitImage, GrowFrame, SignOf, ClearBottomBar, PlaySound, RevealInStrips (no caller)"]),
    ("teal", "INTRO.EXE's own code in the root", [
        "i0892: Main, StartupGame, ShutdownGame and two hooks (2,288 bytes)",
        "i_text: the AdLib driver, 20,980 bytes at 0000:020C, the EOB driver in another version",
        "i0923 (memory chain), i0aed (loader), i0b10 (far copy), i0cda (RandomByte)"]),
    ("grey", "shared with EOB.EXE (section 2.6)", [
        "23 units are twins of EOB.EXE's: sound (084A), memory (094F), page table (097F), CopyRegion (0986), shapes (0998), font (09B5), mouse (09E1), keyboard (0A19), files (0B5D), tick (0CDD), LCW (0CE9), text (0D35) and more"]),
]
cw = 242
for i, (kind, head, rows) in enumerate(cols):
    x = i * (cw + 16)
    tc, sc, fc = PAL[kind]
    c.rect(x, cy, cw, 168, fill=fc, stroke=sc, sw=1.2, rx=6)
    ls = wrap(head, cw - 18, 11.5, False, 700)
    yy = cy + 18
    for l in ls:
        c.text(x + 9, yy, l, size=11.5, weight=700, color=tc)
        yy += 14
    yy += 2
    for r in rows:
        for k, l in enumerate(wrap(r, cw - 26, 10.5)):
            c.text(x + 9 + (0 if k else 0), yy, ("- " if k == 0 else "  ") + l, size=10.5, color=INK2)
            yy += 13
        yy += 3
c.save("figs/snd_intro_prog.svg")

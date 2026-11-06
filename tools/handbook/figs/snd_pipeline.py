import sys; sys.path.insert(0, '.')
from figlib import *

c = Canvas(760, 500)
c.section(0, 11, "From a game event to the speaker")

# ---- game side ----
GY, GH = 24, 58
ents = [
    ("PlaySound(n)", ["plays now; about 59 call sites"]),
    ("PlaySoundAtCell(n, cell)", ["PlaySoundDefault(n) = party cell", "plays the previous call's sound"]),
    ("SayAndPlaySound(text)", ["prints the text, then", "PlaySound(4Fh)"]),
]
ex = [0, 262, 524]
for (t, b), x in zip(ents, ex):
    c.box(x, GY, 236, GH, "navy", t, b, size=11, tsize=12.5, title_mono=True)
c.box(0, 104, 760, 50, "navy", "gSoundOn gate, then gSoundMap[n]",
      ["the 120-byte map from adlib.dat or pcgamesn.dat gives the driver sound number; 96h and up are ignored"],
      size=11.5, tsize=12.5)
for x in (118, 380, 642):
    c.arrow([(x, GY + GH), (x, 103)], color=SOLID["navy"])
c.box(0, 176, 760, 30, "grey", None, ["switch (gSoundCard): the digit START1 passed on, minus '1'"], size=12)
c.arrow([(380, 154), (380, 175)], color=SOLID["navy"])

LX = 0
lanes = [
    ("teal", "card 0", "AdLib or Sound Blaster", [
        ("QueueSound(id)", ["16-byte ring, 0000:400C"]),
        ("INT 08h at 874 Hz", ["every 12th pass runs the tick (72.8 Hz)"]),
        ("SoundDriverTick", ["queue, ten voice scripts, tempo; 0000:3FF4"]),
        ("AdlibWrite", ["ports 388h and 389h, the OPL2"]),
    ]),
    ("ochre", "card 1", "PC speaker", [
        ("StartSound(song)", ["GetSoundData(id AND 7Fh), 1948:0418"]),
        ("INT 08h at 291 Hz", ["SoundTick on every pass"]),
        ("SoundTick: one voice", ["note table, duration count"]),
        ("PIT channel 2", ["port 42h divisor, port 61h gate"]),
    ]),
    ("plum", "card 2", "Tandy", [
        ("StartSound(song)", ["same call, after SoundDriverResume"]),
        ("INT 08h at 291 Hz", ["SoundTick on every pass"]),
        ("SoundTick: three voices", ["one song, echoed 35 and 70 passes late"]),
        ("SN76489", ["port C0h, tone and volume bytes"]),
    ]),
]
sx = [108, 275, 442, 609]
sw = 151
y = 226
for kind, cardn, name, stages in lanes:
    tc, sc, fc = PAL[kind]
    c.text(0, y + 22, cardn, size=12.5, weight=700, color=tc)
    for k, ln in enumerate(wrap(name, 96, 10.5)):
        c.text(0, y + 38 + 12 * k, ln, size=10.5, color=MUTED)
    for i, (t, b) in enumerate(stages):
        c.box(sx[i], y, sw, 62, kind, t, b, size=10.5, tsize=11.5)
        if i:
            c.arrow([(sx[i - 1] + sw, y + 31), (sx[i] - 2, y + 31)], color=SOLID[kind])
    c.arrow([(380 if kind == "teal" else 380, 206), (sx[0] + sw / 2, 206), (sx[0] + sw / 2, y - 2)], color=SOLID[kind]) if False else None
    y += 76
# branch arrows from the switch to the first stage of each lane
c.line(98, 207, 98, 226 + 76 * 2 + 31, stroke=GREY, sw=1.3)
for i, (kind, *_r) in enumerate(lanes):
    yy = 226 + 76 * i
    c.arrow([(98, yy + 31), (sx[0] - 2, yy + 31)], color=SOLID[kind])
c.para(0, 484, "Cards 1 and 2 share the driver in segment 1948 (E1948.ASM); gTandyOn picks the note table and the chip. "
       "The AdLib driver is the 20 KB at 0000:020C (E_TEXT.ASM).", 760, size=10.5, color=MUTED)
c.save("figs/snd_pipeline.svg")

import sys; sys.path.insert(0, '.')
from figlib import *

c = Canvas(760, 470)
c.section(0, 11, "One voice record: 43h bytes, ten of them at 0000:3B93, to scale")
# byte -> group
G = {}
groups = [
    ("script", "navy", "Script state", "+03 script pointer (2), +05 note delay counter, +06 loop counter, +09 block depth, +0A return stack (4 words)",
     [(0x03, 2), (0x05, 1), (0x06, 1), (0x09, 1), (0x0A, 8)]),
    ("voice", "teal", "Voice settings", "+00 level adjustment, +07 transpose, +08 priority, +12 detune, +24 fine tune, +25 speed, +26 speed accumulator, +40 follow-tempo flag",
     [(0x00, 1), (0x07, 1), (0x08, 1), (0x12, 1), (0x24, 1), (0x25, 1), (0x26, 1), (0x40, 1)]),
    ("pitch", "ochre", "Pitch state", "+27 register A0h image, +28 register B0h image, +41 note byte, +42 bend amount",
     [(0x27, 1), (0x28, 1), (0x41, 1), (0x42, 1)]),
    ("level", "plum", "Levels", "+20 level offset, +30 second level offset, +2E modulator and +2F carrier level bytes, +31 connection bit, +32 and +33 shifts, +34 decay level",
     [(0x20, 1), (0x30, 1), (0x2E, 1), (0x2F, 1), (0x31, 1), (0x32, 1), (0x33, 1), (0x34, 1)]),
    ("timing", "red", "Key-off and timing", "+21 key-off count, +2D key-off multiplier, +35 second key-off count, +36 random-delay mask",
     [(0x21, 1), (0x2D, 1), (0x35, 1), (0x36, 1)]),
    ("slide", "grey", "Pitch slide (91h)", "+13 rate, +14 phase, +15 amount (2)", [(0x13, 1), (0x14, 1), (0x15, 2)]),
    ("vib", "grey", "Vibrato (95h)", "+17 delta (2), +19 shift, +1A count, +1B reload, +1C wait, +1D rate, +1E phase, +1F delay counter",
     [(0x17, 2), (0x19, 1), (0x1A, 1), (0x1B, 1), (0x1C, 1), (0x1D, 1), (0x1E, 1), (0x1F, 1)]),
    ("sweep", "grey", "Register sweep (8Dh)", "+39 rate, +3A phase, +3B reload, +3C count, +3D register offset, +3E table pointer (2)",
     [(0x39, 1), (0x3A, 1), (0x3B, 1), (0x3C, 1), (0x3D, 1), (0x3E, 2)]),
    ("hand", "navy", "Handlers", "+29 per-tick handler (2), +2B per-voice handler (2)", [(0x29, 2), (0x2B, 2)]),
]
for key, kind, name, txt, spans in groups:
    for a, n in spans:
        for k in range(n):
            G[a + k] = (key, kind)
X0, W = 0, 758
cw = W / 0x43
by, bh = 44, 40
fills = {"grey": PAL["grey"], "navy": PAL["navy"]}
shade = {"script": "navy", "voice": "teal", "pitch": "ochre", "level": "plum", "timing": "red", "slide": "grey", "vib": "grey", "sweep": "grey", "hand": "navy"}
for i in range(0x43):
    x = X0 + i * cw
    if i in G:
        key, kind = G[i]
        tc, sc, fc = PAL[kind]
        solid = key in ("slide", "vib", "sweep")
        f = fc if key not in ("hand",) else MID["navy"]
        if key == "slide": f = MID["grey"]
        if key == "vib": f = "#CFC8B8"
        if key == "sweep": f = "#BDB5A2"
        if key == "hand": f = MID["navy"]
        c.rect(x, by, cw, bh, fill=f, stroke=sc, sw=0.8, rx=0)
    else:
        c.rect(x, by, cw, bh, fill="#FFFFFF", stroke=RULE, sw=0.8, rx=0, dash="2 2")
for off in range(0, 0x43, 8):
    c.text(X0 + off * cw + 1, by - 6, "%02X" % off, size=10.5, color=MUTED, mono=True)
c.text(X0 + 0x42 * cw + cw, by - 6, "43", size=10.5, color=MUTED, mono=True, anchor="end")
c.text(0, by + bh + 14, "dashed white: not used (+01, +02, +16 high byte of the amount aside: +22, +23, +37, +38 and the high bytes of the words)", size=10.5, color=MUTED) if False else None
c.text(0, by + bh + 16, "white with a dashed edge: bytes the code never touches (+01, +02, +22, +23, +37, +38); the second byte of each word field is drawn with its field",
       size=10.5, color=MUTED)

# legend: two columns
y = 124
colx = [0, 382]
col = 0
rows_left = 0
ys = [y, y]
for key, kind, name, txt, spans in groups:
    tc, sc, fc = PAL[kind]
    f = fc
    if key == "slide": f = MID["grey"]
    if key == "vib": f = "#CFC8B8"
    if key == "sweep": f = "#BDB5A2"
    if key == "hand": f = MID["navy"]
    ci = 0 if ys[0] <= ys[1] else 1
    x = colx[ci]; yy = ys[ci]
    c.rect(x, yy - 10, 14, 14, fill=f, stroke=sc, sw=1, rx=2)
    c.text(x + 22, yy + 1, name, size=12, weight=700, color=tc)
    ls = wrap(txt, 350, 10.5)
    for k, l in enumerate(ls):
        c.text(x + 22, yy + 16 + 13 * k, l, size=10.5, color=INK2)
    ys[ci] = yy + 22 + 13 * len(ls) + 12
H = max(ys) + 6
c.para(0, H + 4, "Every offset is read in E_TEXT.ASM (the cites are in its comment at 0000:3B93). ClearVoice (4D95) zeroes +03 to +42 and then sets speed FFh, handlers to a bare ret and the second key-off count to 1. "
       "Voice 9 has a record but no OPL2 channel: it conducts.", 758, size=10.5, color=MUTED)
c.h = H + 50
c.save("figs/snd_voice.svg")

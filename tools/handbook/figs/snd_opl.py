import sys; sys.path.insert(0, '.')
from figlib import *

c = Canvas(760, 400)
c.section(0, 11, "Ten voices on the OPL2: operator slots (left) and one patch as register writes (right)")

# ---- left: slot map ----
c.text(0, 34, "slot offset = low bits of the register; carrier = modulator + 3", size=10.5, color=MUTED)
c.text(3 * 46 / 2, 56, "modulators", size=10.5, weight=600, color=INK2, anchor="middle")
c.text(3 * 46 * 1.5, 56, "carriers", size=10.5, weight=600, color=INK2, anchor="middle")
kinds = ["teal", "navy", "ochre"]
voices = [(0, 0x00), (1, 0x01), (2, 0x02), (3, 0x08), (4, 0x09), (5, 0x0A), (6, 0x10), (7, 0x11), (8, 0x12)]
cw, chh = 46, 46
y = 64
for g in range(3):
    gy = y + g * (chh + 14)
    for j in range(3):
        v, off = voices[g * 3 + j]
        tc, sc, fc = PAL[kinds[g]]
        for part, o in (("mod", off), ("car", off + 3)):
            x = j * cw if part == "mod" else (j + 3) * cw
            c.rect(x + 1, gy, cw - 2, chh, fill=fc if part == "mod" else MID[kinds[g]], stroke=sc, sw=1.1, rx=3)
            c.text(x + cw / 2, gy + 17, "%02X" % o, size=12, weight=700, color=tc, anchor="middle", mono=True)
            c.text(x + cw / 2, gy + 33, "voice %d" % v, size=10, color=INK2, anchor="middle")
c.para(0, y + 3 * (chh + 14) + 12, "A0h+n, B0h+n and C0h+n belong to voice n. Voice 9 has no channel: its record only runs scripts. "
       "Rhythm mode (BDh bit 5) would turn voices 6 to 8 into drums; no shipped sound uses it.", 280, size=10.5, color=MUTED)

# ---- right: patch table ----
RX = 316
c.text(RX, 34, "patch 6 of sound.dat, loaded into voice 8 (slots 12h, 15h), as written to the card", size=10.5, color=MUTED)
hd = [("byte", 0), ("value", 40), ("register", 86), ("meaning [H]", 216)]
y = 52
c.rect(RX, y - 14, 444, 20, fill=PANEL, rx=3)
for t, dx in hd:
    c.text(RX + dx + 6, y, t, size=10.5, weight=700, color=PAL["navy"][0])
rows = [
    ("0", "18", "20h + mod = 32h", "AM, vibrato, sustain mode, KSR, multiplier"),
    ("1", "25", "23h + mod = 35h", "the same for the carrier"),
    ("2", "08", "C0h + voice = C8h", "feedback; bit 0 = connection (FM / additive)"),
    ("3", "00", "E0h + mod = F2h", "modulator waveform"),
    ("4", "00", "E3h + mod = F5h", "carrier waveform"),
    ("5", "88", "40h + mod = 52h", "modulator key scale and total level"),
    ("6", "80", "43h + mod = 55h", "carrier key scale and total level (+ offsets)"),
    ("7", "FB", "60h + mod = 72h", "modulator attack and decay"),
    ("8", "F4", "63h + mod = 75h", "carrier attack and decay"),
    ("9", "22", "80h + mod = 92h", "modulator sustain level and release"),
    ("10", "12", "83h + mod = 95h", "carrier sustain level and release"),
]
for i, (b, v, reg, mean) in enumerate(rows):
    y += 22
    if i % 2:
        c.rect(RX, y - 15, 444, 22, fill="#FAF6EC", rx=0)
    c.text(RX + 6, y, b, size=11, color=MUTED, mono=True)
    c.text(RX + 46, y, v, size=11.5, weight=700, color=PAL["plum"][0], mono=True)
    c.text(RX + 92, y, reg, size=11, color=INK, mono=True)
    c.text(RX + 222, y, mean, size=10.5, color=INK2)
y += 26
c.rect(RX, y - 14, 444, 22, fill=PANEL, rx=3)
c.text(RX + 6, y + 1, "next, BEND 01 sets the pitch: A8h := 35h, B8h := 01h (F-number 135h, block 0)", size=10.5, color=PAL["ochre"][0], weight=600)
y += 28
c.para(RX, y, "The 13 writes took 540 to 840 us each in the oracle, which charges 20 us per status-port poll; "
       "AdlibWrite waits with 4 reads after the register number and 23 after the value.", 444, size=10.5, color=MUTED)
c.save("figs/snd_opl.svg")

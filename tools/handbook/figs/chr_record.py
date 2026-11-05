import sys; sys.path.insert(0, '.')
from figlib import *

# The 243-byte character record (struct CharRec, src/EOB.H) drawn to scale in four strips of 64 bytes.
W = 720.0
BPS = 64
UNIT = W / BPS                # 11.25 units per byte
X0 = 20
STRIP_H = 30

F = [
    (0x00, 1, "index", "grey"), (0x01, 1, "status", "red"), (0x02, 11, "name[11]", "teal"),
    (0x0D, 2, "STR", "navy"), (0x0F, 2, "STR%", "navy"), (0x11, 2, "INT", "navy"), (0x13, 2, "WIS", "navy"),
    (0x15, 2, "DEX", "navy"), (0x17, 2, "CON", "navy"), (0x19, 2, "CHA", "navy"),
    (0x1B, 1, "hp", "red"), (0x1C, 1, "maxHp", "red"), (0x1D, 1, "AC", "red"), (0x1E, 1, "m1E", "red"),
    (0x1F, 1, "race", "teal"), (0x20, 1, "class", "teal"), (0x21, 1, "align", "teal"), (0x22, 1, "portr#", "teal"),
    (0x23, 1, "food", "red"),
    (0x24, 3, "levels[3]", "ochre"), (0x27, 12, "experience[3]", "ochre"), (0x33, 4, "portrait*", "grey"),
    (0x37, 30, "mageSpells[30]", "plum"), (0x55, 30, "clericSpells[30]", "plum"), (0x73, 4, "spellBook", "plum"),
    (0x77, 54, "items[27]", "paper"),
    (0xAD, 40, "timers[10]", "teal"), (0xD5, 10, "mD5[10]", "teal"),
    (0xDF, 1, "mDF", "red"), (0xE0, 1, "mE0", "red"), (0xE1, 1, "pad", "grey"), (0xE2, 1, "mE2", "red"),
    (0xE3, 4, "effectFlags", "red"), (0xE7, 1, "lastDmg", "grey"), (0xE8, 11, "lastAction[11]", "grey"),
]
# bracket labels for runs of narrow fields: (first byte, last byte, text, colour)
GROUPS = [
    (0x00, 0x01, "index, status", "grey"),
    (0x0D, 0x1A, "STR, STR%, INT, WIS, DEX, CON, CHA: (current, base)", "navy"),
    (0x1B, 0x1E, "hp maxHp AC m1E", "red"),
    (0x1F, 0x23, "race class align portr# food", "teal"),
    (0x24, 0x26, "levels[3]", "ochre"),
    (0x33, 0x36, "portrait*", "grey"),
    (0x73, 0x76, "spellBook", "plum"),
    (0xDF, 0xE2, "mDF mE0 pad mE2", "red"),
    (0xE7, 0xE7, "lastDmg", "grey"),
]
total = 0xF3
nstrips = (total + BPS - 1) // BPS
ROWH = 13


GAPS = [22 + ROWH * 3, 22 + ROWH, 28, 22 + ROWH]


def strip_y(i):
    return 34 + sum(STRIP_H + g for g in GAPS[:i])

H = strip_y(nstrips - 1) + STRIP_H + GAPS[-1] + 28
c = Canvas(760, H)
c.section(0, 11, "struct CharRec: 243 bytes (0xF3), six of them at ds:F580, to scale")

for i in range(nstrips):
    y = strip_y(i)
    lo, hi = i * BPS, min((i + 1) * BPS, total)
    for o in range(lo, hi + 1, 8):
        x = X0 + (o - lo) * UNIT
        c.line(x, y - 4, x, y, stroke=GREY, sw=0.8)
        c.text(x, y - 7, "%02X" % o, size=9.5, color=MUTED, anchor="middle", mono=True)
    c.rect(X0, y, (hi - lo) * UNIT, STRIP_H, fill=CARD, stroke=RULE, sw=1, rx=0)
    grouped = set()
    for (a, b, t, k) in GROUPS:
        if a >= lo and a < hi:
            for q in range(a, b + 1):
                grouped.add(q)
    for (s, n, name, kind) in F:
        e = s + n
        if e <= lo or s >= hi:
            continue
        a, b = max(s, lo), min(e, hi)
        x, w = X0 + (a - lo) * UNIT, (b - a) * UNIT
        tc, sc, fc = PAL[kind]
        c.rect(x, y, w, STRIP_H, fill=fc, stroke=sc, sw=1, rx=0)
        cont = (s < lo)
        nm = name + (" (cont.)" if cont else "")
        if text_width(nm, 10.5, weight=600) + 6 <= w:
            c.text(x + w / 2, y + STRIP_H / 2 + 4, nm, size=10.5, weight=600, color=tc, anchor="middle")
        elif text_width(name, 10.5, weight=600) + 6 <= w:
            c.text(x + w / 2, y + STRIP_H / 2 + 4, name, size=10.5, weight=600, color=tc, anchor="middle")
    # brackets under the strip for runs of narrow fields, alternating two rows when they would overlap
    last_end = [-100, -100, -100]
    for (a, b, t, k) in GROUPS:
        if not (a >= lo and a < hi):
            continue
        x1 = X0 + (a - lo) * UNIT + 1
        x2 = X0 + (b + 1 - lo) * UNIT - 1
        tw = text_width(t, 10.5)
        cx = (x1 + x2) / 2
        # keep the label inside the strip width
        lx = min(max(cx, X0 + tw / 2), X0 + W - tw / 2)
        row = 0
        while row < 2 and lx - tw / 2 < last_end[row] + 6:
            row += 1
        tc, sc, fc = PAL[k]
        by = y + STRIP_H + 3
        c.path(f"M{x1:.1f},{by} L{x1:.1f},{by+4} L{x2:.1f},{by+4} L{x2:.1f},{by}", stroke=sc, sw=1)
        ty = by + 17 + row * ROWH
        if row:
            c.line(cx, by + 4, cx, ty - 9, stroke=sc, sw=0.7)
        else:
            c.line(cx, by + 4, cx, ty - 9, stroke=sc, sw=0.7)
        c.text(lx, ty, t, size=10.5, color=tc, anchor="middle")
        last_end[row] = lx + tw / 2

ly = strip_y(nstrips - 1) + STRIP_H + GAPS[-1] + 8
c.legend(X0, ly, [("teal", "identity and timers"), ("navy", "abilities"), ("red", "condition"),
                 ("ochre", "class slots"), ("plum", "spells"), ("paper", "items (27 words)"), ("grey", "bookkeeping")], size=10.5)
c.save("figs/chr_record.svg")

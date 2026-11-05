import sys; sys.path.insert(0, '.')
from figlib import *

c = Canvas(760, 440)
c.section(0, 11, "LEVELn.INF AFTER UNPACKING: LEVEL 2, 7,444 BYTES, TO SCALE")
# bar
BX0, BX1 = 0, 760
TOT = 7444
def mx(o): return BX0 + (BX1 - BX0) * o / TOT
segs = [
    (0, 78, "grey", "header"),
    (78, 498, "red", "monsters"),
    (498, 724, "ochre", "wall records"),
    (724, 7022, "navy", "scripts (6,298 bytes)"),
    (7022, 7444, "teal", "trigger table"),
]
by, bh = 30, 34
for lo, hi, k, lab in segs:
    tc, sc, fc = PAL[k]
    c.rect(mx(lo), by, mx(hi) - mx(lo), bh, fill=fc, stroke=sc, sw=1.1, rx=0)
c.text(mx(724 + (7022 - 724) / 2), by + 22, "scripts: 6,298 bytes", size=12.5, weight=700, color=PAL["navy"][0], anchor="middle")
# labels under small segments with leader lines
lab = [(0, 78, "header 0..77", "grey"), (78, 498, "30 monster records", "red"), (498, 724, "wall records", "ochre"), (7022, 7444, "trigger table 7022..7443", "teal")]
ly = by + bh + 14
c.text(mx(0), ly, "header", size=10.5, color=PAL["grey"][0], weight=700)
c.text(mx(0), ly + 13, "0..77", size=10.5, color=MUTED)
c.text(mx(78) + 4, ly + 28, "30 monster records, 78..497", size=10.5, color=PAL["red"][0], weight=700)
c.line(mx(288), by + bh, mx(288), ly + 18, stroke=PAL["red"][1], sw=0.8)
c.text(mx(498) + 14, ly + 44, "count and wall records, 498..723", size=10.5, color=PAL["ochre"][0], weight=700)
c.line(mx(611), by + bh, mx(611), ly + 34, stroke=PAL["ochre"][1], sw=0.8)
c.text(760, ly + 12, "trigger table 7022..7443", size=10.5, color=PAL["teal"][0], weight=700, anchor="end")

# detail table
TY = 142
c.line(0, TY - 16, 760, TY - 16, stroke=RULE)
c.section(0, TY - 2, "THE FIELDS, IN FILE ORDER (OFFSETS ARE FROM THE START; read by LoadLevel, ovl3:0A7C)")
cols = [0, 56, 150]
hdr = ["offset", "bytes", "field"]
yy = TY + 18
for h, x in zip(hdr, cols):
    c.text(x, yy, h, size=10.5, color=MUTED, weight=600)
c.line(0, yy + 5, 760, yy + 5, stroke=RULE)
rows = [
    ("0", "2", "offset T of the trigger table; the scripts run up to it"),
    ("2", "12", "maze file name, NUL-terminated, e.g. level2.maz"),
    ("14", "12", "wall block map name (the .VMP or .EMP, and the .PAL), e.g. brick"),
    ("26", "12", "wall texture name (the .VCN or .ECN); passed as gLevelInf + 0x1A"),
    ("38", "4", "door pictures: two (set, first slot) pairs for LoadDoorShapes, 255 = none"),
    ("42", "1 + 2 + 2", "level timer: mode (0 off, 1 ticks, 2 steps, 3 both), period in ticks, steps"),
    ("47", "13 + 13", "two monster sprite sheets: loader 0, 1 or 2 (255 none), then the sheet name"),
    ("73", "2n + 1", "n pairs (monster speed class, period in ticks), ended by FF"),
    ("next", "420", "30 monster records of 14 bytes (a record with cell 0 is an empty slot)"),
    ("next", "2 + ...", "count, then wall records: EC + 24 bytes (screen name, .DAT name) or FB + 5 bytes (a wall kind)"),
    ("next", "...", "scripts: shared subroutines first, then the script of each trigger entry, back to back"),
    ("T", "2 + 5k", "trigger table: count k, then k entries (cell u16, flag byte, script offset u16), ending at the end of the file"),
]
yy += 24
for off, n, f in rows:
    c.text(cols[0], yy, off, size=11.5, color=INK2, mono=True)
    c.text(cols[1], yy, n, size=11.5, color=INK2, mono=True)
    lines = wrap(f, 600, 11.5)
    for j, l in enumerate(lines):
        c.text(cols[2], yy + j * 14, l, size=11.5, color=INK2)
    yy += 22 + (len(lines) - 1) * 14
c.save("figs/wld_inf.svg")

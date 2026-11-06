import sys; sys.path.insert(0, '.')
from figlib import *

c = Canvas(760, 400)
c.section(0, 11, "A SCRIPT IS A BYTE STREAM: AN OPCODE BYTE (E6 TO FF), THEN ITS OPERANDS. REAL BYTES, LEVEL 1")
BX0 = 40

def row(y, off, groups, caption, bw=27):
    c.text(0, y + 15, off, size=10.5, color=MUTED, mono=True)
    x = BX0
    for kind, hx in groups:
        for h in hx:
            c.rect(x, y, bw, 22, fill=PAL[kind][2], stroke=PAL[kind][1], sw=1, rx=2)
            c.text(x + bw / 2, y + 15, h, size=10.5, weight=700, color=PAL[kind][0], anchor="middle", mono=True)
            x += bw
        x += 8
    c.text(BX0, y + 38, caption, size=10.5, color=INK2)

row(34, "719", [("navy", ["EE"]), ("teal", ["E0", "01", "FF"]), ("navy", ["EE"]), ("ochre", ["5A", "03"])],
    "IF  (token E0 = the mode, literal 1, FF = equal)  end of expression  else jump to 0x035A: 'if mode == 1'")
row(88, "746", [("navy", ["F8"]), ("plum", ["67", "6F", "69", "6E", "67", "20", "64", "6F", "77", "6E", "2E", "2E", "2E", "00"]), ("ochre", ["05", "00"])],
    "TEXT  the string 'going down...' with its NUL  two colour bytes")
row(142, "767", [("navy", ["EC"]), ("grey", ["E5"]), ("teal", ["02"]), ("teal", ["F3", "02"]), ("teal", ["01"])],
    "LOAD LEVEL  an unused byte  level 2  cell 0x02F3 = 755  facing 1 (east)")
row(196, "773", [("navy", ["F2"]), ("ochre", ["5A", "03"])],
    "JUMP  to 0x035A = 858, counted from the start of the file")

# the driver
Y = 262
c.line(0, Y - 14, 760, Y - 14, stroke=RULE)
c.section(0, Y, "THE DRIVER, RunCellScript (122E:1DA8)")
flow = [
    ("navy", "op = *script++", "the next byte"),
    ("navy", "E6 <= op <= FF ?", "any other byte is skipped"),
    ("navy", "table[FF - op]", "26 far pointers"),
    ("navy", "script += h(script)", "bytes used, or a jump"),
    ("plum", "until gScriptDone", "END, LOAD LEVEL set it"),
]
fw, fg = 138, 17.5
for i, (k, t, b) in enumerate(flow):
    x = i * (fw + fg)
    c.rect(x, Y + 12, fw, 52, fill=PAL[k][2], stroke=PAL[k][1], sw=1.1, rx=6)
    c.text(x + 8, Y + 31, t, size=11.5, weight=700, color=PAL[k][0])
    c.text(x + 8, Y + 48, b, size=10.5, color=MUTED)
    if i < 4:
        c.arrow([(x + fw + 1, Y + 38), (x + fw + fg - 2, Y + 38)], color=GREY)
yy = Y + 86
for l in wrap("State the driver and handlers share: gScriptCell (the cell), gScriptTrigger (the mode; a script reads it as expression token E0), gScriptDone, a call stack of ten far pointers (gScriptStack) and the expression stack gEvalStack. Before it runs anything the driver reads the cell's script offset (+7) and tests the mode against the cell's mask: no offset or no matching event, no script.", 720, 10.5):
    c.text(0, yy, l, size=10.5, color=MUTED)
    yy += 13
c.save("figs/wld_script.svg")

import sys, json; sys.path.insert(0, '.')
from figlib import *

cnt = json.load(open('data/snd_opcounts.json'))
c = Canvas(760, 380)
c.section(0, 11, "The 74-entry command table at 0000:4F94: script byte 80h + n")
# opcode -> (label, category)
K = {"flow": "navy", "time": "red", "pitch": "ochre", "level": "plum", "voice": "teal", "fx": "grey"}
D = {
 0x80: ("LOOP#", "flow"), 0x81: ("LOOP", "flow"), 0x82: ("START", "flow"), 0x83: ("OFF2", "time"), 0x84: ("JUMP", "flow"),
 0x85: ("CALL", "flow"), 0x86: ("RET", "flow"), 0x87: ("TRANSP", "pitch"), 0x88: ("END", "flow"), 0x89: ("REST", "time"),
 0x8A: ("OPL", "fx"), 0x8B: ("PITCH", "pitch"), 0x8C: ("DETUNE", "pitch"), 0x8D: ("SWEEP", "fx"), 0x8E: ("STOP", "voice"),
 0x90: ("PATCH", "level"), 0x91: ("SLIDE", "pitch"), 0x92: ("SLIDE0", "pitch"), 0x93: ("FINE", "pitch"), 0x95: ("VIBRAT", "pitch"),
 0x9A: ("PRIO", "voice"), 0x9C: ("TEMPO", "time"), 0x9D: ("BEAT", "flow"), 0x9E: ("LEVEL", "level"),
 0xA0: ("HOLD", "time"), 0xA1: ("RETRIG", "time"), 0xA4: ("GATE", "time"), 0xA6: ("SPEED*", "time"), 0xA7: ("NOHAND", "voice"),
 0xA9: ("SPEED", "time"), 0xAB: ("LEVEL2", "level"), 0xAC: ("SETADJ", "level"), 0xAD: ("ADDADJ", "level"), 0xAE: ("AMDEP", "level"),
 0xAF: ("VIBDEP", "level"), 0xB0: ("ADDLVL", "level"), 0xB3: ("KILL", "voice"), 0xB5: ("JITTER", "pitch"), 0xB6: ("NOTICK", "voice"),
 0xB9: ("BEND", "pitch"), 0xBA: ("FOLLOW", "time"), 0xBB: ("NOP", "time"), 0xBC: ("RNDDLY", "time"), 0xBD: ("SPEED+", "time"),
 0xBF: ("FX", "fx"), 0xC0: ("FXOFF", "fx"), 0xC1: ("RHYSET", "fx"), 0xC2: ("RHYKEY", "fx"), 0xC3: ("RHYCLR", "fx"),
 0xC4: ("RLVLC", "fx"), 0xC5: ("RLVLB+", "fx"), 0xC6: ("RLVLB", "fx"), 0xC7: ("FLAG", "voice"), 0xC8: ("FOLLOW", "time"),
 0xC9: ("SHIFT", "level"),
}
D[0xC8] = ("TRACK", "time")
for _k, _v in {128: 'SETLP', 135: 'TRANS', 140: 'DETUN', 146: 'SLID0', 149: 'VIBRA', 161: 'RETRG', 166: 'RATE', 167: 'NOHND', 171: 'LVL2', 172: 'SETAJ', 173: 'ADDAJ', 175: 'VIBDP', 176: 'ADDLV', 181: 'JITTR', 182: 'NOTIK', 188: 'RNDLY', 189: 'SPD+', 193: 'RSET', 194: 'RKEY', 195: 'RCLR', 196: 'RLVC', 197: 'RLV+', 198: 'RLVB'}.items():
    D[_k] = (_v, D[_k][1])
D[0xBA] = ("ONCE", "time")
cw, ch = 43.5, 50
gx, gy = 44, 46
for k in range(16):
    c.text(gx + k * cw + cw / 2, gy - 8, "%X" % k, size=11, weight=700, color=MUTED, anchor="middle", mono=True)
for r, hi in enumerate(range(8, 13)):
    y = gy + r * ch
    c.text(gx - 8, y + ch / 2 + 4, "%Xx" % hi, size=12, weight=700, color=MUTED, anchor="end", mono=True)
    for k in range(16):
        op = hi * 16 + k
        x = gx + k * cw
        if op > 0xC9:
            c.rect(x + 1, y + 1, cw - 2, ch - 2, fill="#FFFFFF", stroke=RULE, sw=0.8, rx=3, dash="2 2")
            continue
        if op in D:
            lab, cat = D[op]
            tc, sc, fc = PAL[K[cat]]
            used = cnt.get("%02X" % op, 0)
            c.rect(x + 1, y + 1, cw - 2, ch - 2, fill=fc if used or op == 0x88 else "#FFFDF8", stroke=sc, sw=1.2 if used else 0.8, rx=3,
                   dash=None if used else "3 2")
            c.text(x + 5, y + 14, "%02X" % op, size=10, color=MUTED, mono=True)
            c.text(x + cw / 2, y + 29, lab, size=10, weight=700, color=tc, anchor="middle")
            if used:
                c.text(x + cw / 2, y + 43, "{:,}".format(used), size=10, color=INK2, anchor="middle")
        else:
            c.rect(x + 1, y + 1, cw - 2, ch - 2, fill="#ECE7DD", stroke=RULE, sw=0.8, rx=3)
            c.text(x + 5, y + 14, "%02X" % op, size=10, color=MUTED, mono=True)
            c.text(x + cw / 2, y + 33, "end", size=10, color=MUTED, anchor="middle")
ly = gy + 5 * ch + 18
c.legend(0, ly, [("navy", "flow"), ("red", "timing and tempo"), ("ochre", "pitch"), ("plum", "levels, patch, depth"),
                 ("teal", "voice control"), ("grey", "raw register, rhythm, effects")])
c.para(0, ly + 20, "Labels are short names of this chapter (the source names the handlers by address only). Number: how often the byte starts a command in the shipped "
       "adlib.dat and sound.dat; a dashed cell is a handler no shipped sound uses. Grey 'end' cells share the end-of-script handler (4311); "
       "so do the bytes F1h to FFh, while CAh to F0h jump through whatever follows the table. A byte below 80h is a note, not a command.",
       690, size=10.5, color=MUTED)
c.save("figs/snd_commands.svg")

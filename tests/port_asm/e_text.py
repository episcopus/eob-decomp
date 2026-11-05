"""E_TEXT.c (the AdLib driver in C) against src/E_TEXT.ASM on the bench (playbook 80, M3): the original
then the C, compared (hybrid.Direct), with the driver block of ADLIB.DAT, the game's sounds and
generated scripts that use every command of the interpreter, the timer handlers on every pass, the
far routines no C calls (through ctypes); coverage of the original's instructions from a Unicorn code
hook. Exit status 1 if any call differs.

  e_text.py [LIB] [SEED] [TICKS]
"""
import sys, random, collections, ctypes
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools" / "port")); sys.path.insert(0, str(ROOT / "tools" / "re"))
import hybrid, pak
import capstone
from unicorn import UC_HOOK_CODE

LIB = Path(sys.argv[1]) if len(sys.argv) > 1 else hybrid.LIB
SEED = int(sys.argv[2]) if len(sys.argv) > 2 else 1
TICKS = int(sys.argv[3]) if len(sys.argv) > 3 else 1000
rng = random.Random(SEED)
CS = hybrid.LOAD_SEG
dat = [e for e in pak.read_pak(open(ROOT / "eob" / "EOBDATA1.PAK", "rb").read())["entries"] if e["name"].upper() == "ADLIB.DAT"][0]["data"]
smap, block = dat[:120], dat[120:120 + 0x32C8]

d = hybrid.Direct(lib=LIB, sound="adlib")
m = d.m
stats = collections.Counter()
bad = []
hit = set()
targets = collections.Counter()

def on_code(uc, addr, size, ud):
    hit.add(addr - CS * 16)
    if addr - CS * 16 == 0x425D:
        targets[uc.reg_read(__import__("unicorn").x86_const.UC_X86_REG_BX)] += 1

m.uc.hook_add(UC_HOOK_CODE, on_code, None, CS * 16 + 0x03A1, CS * 16 + 0x4F93)

def note(r):
    stats[(r["function"], r["status"])] += 1
    if r["status"] != "same":
        print("NOT SAME", r, flush=True)
        for a, v, dd in [g for g in gens if g[0] <= 0x3344 < g[0] + len(g[2])]:
            print("  gen %04X v%d %s" % (a, v, dd.hex(" ")))
        for v in range(10):
            print("  rec%d %s" % (v, d.read(CS, 0x3B93 + 0x43 * v, 0x43).hex()))
        if len(bad) < 12:
            bad.append(r)
    return r

def call(name, *a, **k):
    return note(d.call(name, *a, **k))

# -- the far routines no C calls (no thunk): the original, then the exported C function, by hand
SENT = hybrid.SENTINEL
def call_far(cname, off, args, ret, retbytes):
    bench = d.bench
    block_ = b"".join((v & 0xFFFF).to_bytes(2, "little") if w == 2 else (v & 0xFFFFFFFF).to_bytes(4, "little") for v, w in args)
    ss, top = d.STACK
    sp = (top - len(block_) - 4) & 0xFFFF
    m.write(ss, sp + 4, block_)
    m.ww(ss, sp, SENT[1]); m.ww(ss, sp + 2, SENT[0])
    for r_, v in (("ss", ss), ("sp", sp), ("ds", hybrid.DGROUP), ("es", hybrid.DGROUP), ("cs", CS), ("ip", off), ("ax", 0)):
        m.set(r_, v)
    before = bench.memory(); before_state = bench.save()
    why = bench.until(SENT[0], SENT[1], (sp + 4) & 0xFFFF)
    r = {"function": cname}
    if why:
        r["status"] = "not tested: " + why
        return note(r)
    o_mem, o_val = bench.memory(), (m.get("dx") << 16) | m.get("ax")
    o_out = bench.outside(); after_state = bench.save()
    ctypes.memmove(bench.base, before, hybrid.RAM); m.uc.ctl_remove_cache(0, hybrid.RAM); bench.restore(before_state)
    fn = getattr(d.t.lib, cname)
    fn.restype = ret
    fn.argtypes = [ctypes.c_uint16 if w == 2 else ctypes.c_uint32 for _, w in args]
    val = fn(*[v for v, _ in args])
    val = 0 if val is None else val
    c_mem, c_out = bench.memory(), bench.outside()
    ctypes.memmove(bench.base, o_mem, hybrid.RAM); m.uc.ctl_remove_cache(0, hybrid.RAM); bench.restore(after_state)
    spans = hybrid.diff(o_mem, c_mem, [(ss * 16, ss * 16 + top)])
    mask = (1 << (8 * retbytes)) - 1
    regs = [] if (o_val & mask) == (val & mask) else ["returns %X, the original %X" % (val & mask, o_val & mask)]
    outs = [k for k in o_out if o_out[k] != c_out[k]]
    r["status"] = "same" if not spans and not regs and not outs else "differs"
    if r["status"] == "differs":
        r.update(registers=regs, outside=outs, memory=[d.t.describe(a, b, o_mem, c_mem) for a, b in spans[:6]])
    return note(r)

# -- the driver as LoadSoundDriver and StartAdlibDriver leave it
d.write(CS, 0x416, block)
call("RebaseDriverBlock")
call("StartAdlibDriver")
call("GetDriverBlock")
sounds = sorted(set(b for b in smap if b < 0x96))
rd16 = lambda off: int.from_bytes(d.read(CS, off, 2), "little")
sound_voice = {s: d.read(CS, rd16(0x416 + 2 * s), 1)[0] for s in sounds}

# -- generated scripts
SCRIPT_AREA = 0x2200
SIMPLE = {0x83: 1, 0x87: 1, 0x8C: 1, 0x93: 1, 0x9A: 1, 0x9C: 1, 0x9E: 1, 0xA4: 1, 0xA6: 1, 0xA9: 1, 0xAB: 1,
          0xAE: 1, 0xAF: 1, 0xB0: 1, 0xB9: 1, 0xBC: 1, 0xBD: 1, 0xC7: 1, 0xC8: 1, 0x9D: 1, 0xA0: 1, 0xA1: 1, 0x89: 1}
NOOP1 = [0x92, 0xA7, 0xB6, 0xBA, 0xBB, 0xC0, 0xC3]   # `dec si`: one byte

def gen(voice, level):
    """a sound for `voice`: header, main, subroutine; labels resolved after"""
    items = []      # bytes or ("lbl", name) or ("ref", name)
    def b(*xs): items.extend(xs)
    def notebytes():
        b(rng.randrange(0, 0x80), rng.choice([0, 0, 1, 2, 3, 6, 15]))
        if level:
            b(rng.randrange(1, 256))
    b(voice, rng.randrange(0, 0x100))
    if level:
        b(0xC9, rng.randrange(1, 4), rng.randrange(0, 4))
    loop_open = False
    for k in range(rng.randrange(4, 30)):
        x = rng.random()
        if x < 0.25:
            notebytes()
            continue
        op = rng.choice(list(SIMPLE) * 2 + NOOP1 + [0x80, 0x84, 0x85, 0x82, 0x8A, 0x8B, 0x8D, 0x8E, 0x90, 0x91, 0x95,
                                                  0xAC, 0xAD, 0xB3, 0xB5, 0xBF, 0xC1, 0xC2, 0xC4, 0xC5, 0xC6, 0xC9] * 1)
        if op in SIMPLE:
            v = rng.randrange(256)
            if op in (0x9D,):
                v = rng.choice([1, 2, 4, 8, 0])
            if op in (0xA0, 0xA1, 0x89):
                v = rng.choice([0, 1, 2, 5])
            b(op, v)
        elif op in NOOP1:
            b(op)
        elif op == 0x80:
            if not loop_open:
                b(0x80, rng.choice([1, 2, 3, 0x7]), ("lbl", "loop%d" % k))
                loop_open = k
        elif op == 0x84:
            b(0x84, ("ref", "j%d" % k), ("lbl", "j%d" % k))
        elif op == 0x85:
            b(0x85, ("ref", "sub"))
        elif op == 0x82:
            cand = [s for s in sounds if sound_voice[s] != voice] + [0xFF]
            b(0x82, rng.choice(cand))
        elif op == 0x8A:
            b(0x8A, rng.randrange(0x20, 0xF6), rng.randrange(256))
        elif op == 0x8B:
            b(0x8B, rng.randrange(0, 0x80), rng.choice([0, 1, 3]))
        elif op == 0x8D:
            t = rng.randrange(0x416, 0x2000)
            b(0x8D, rng.randrange(256), rng.randrange(0, 0x10), rng.randrange(0x40, 0x56), t & 0xFF, t >> 8)
        elif op in (0x8E, 0xB3):
            b(op, rng.choice([v for v in range(10) if v != voice]))
        elif op == 0x90:
            b(0x90, rng.randrange(0, 0x30))
        elif op == 0x91:
            amt = rng.choice([rng.randrange(-0x40, 0x40), rng.randrange(-0x400, 0x400)]) & 0xFFFF
            b(0x91, rng.randrange(256), amt >> 8, amt & 0xFF)
        elif op == 0x95:
            b(0x95, rng.randrange(256), rng.randrange(0, 12), rng.randrange(256), rng.randrange(256))
        elif op in (0xAC, 0xAD):
            b(op, rng.randrange(0, 10), rng.randrange(256))
        elif op == 0xB5:
            b(0xB5, rng.randrange(256), rng.randrange(256))
        elif op == 0xBF:
            b(0xBF, rng.choice([0, 1, 2, 2, 3]), rng.choice([0, 1, 2, 3, 4, rng.randrange(256)]))
        elif op == 0xC1:
            b(0xC1, rng.randrange(0, 0x30), rng.randrange(0, 0x30), rng.randrange(0, 0x30), *[rng.randrange(256) for _ in range(6)])
        elif op == 0xC2:
            b(0xC2, rng.randrange(256))
        elif op in (0xC4, 0xC5, 0xC6):
            b(op, rng.randrange(0, 0x20), rng.randrange(256))
        elif op == 0xC9 and level:
            b(0xC9, rng.randrange(1, 256), rng.randrange(256))
        if loop_open is not False and rng.random() < 0.2:
            b(0x81, ("ref", "loop%d" % loop_open))
            loop_open = False
    if loop_open is not False:
        b(0x81, ("ref", "loop%d" % loop_open))
    b(rng.choice([0x88, 0x8F, 0xF5, 0xFF]), 0)              # the end (the table's L_4311 entries, the range check)
    b(("lbl", "sub"))
    if level:
        b(0xC9, rng.randrange(1, 4), rng.randrange(0, 4))
    for k in range(rng.randrange(0, 4)):
        notebytes()
    b(0x86)
    return items

def assemble(items, base):
    lbl, pos = {}, base
    for it in items:
        if isinstance(it, tuple):
            if it[0] == "lbl":
                lbl[it[1]] = pos
            else:
                pos += 2
        else:
            pos += 1
    out = bytearray()
    for it in items:
        if isinstance(it, tuple):
            if it[0] == "ref":
                out += lbl[it[1]].to_bytes(2, "little")
        else:
            out.append(it & 0xFF)
    return bytes(out)

next_slot = [240]
gens = []
area = [SCRIPT_AREA]
def new_sound():
    voice = rng.randrange(0, 10)
    data = assemble(gen(voice, rng.random() < 0.4), area[0])
    if area[0] + len(data) > 0x36D0:
        return rng.choice(sounds)              # no room: the area is not reused (a voice may still read it)
    d.write(CS, area[0], data)
    gens.append((area[0], voice, data))
    slot = next_slot[0]
    d.write(CS, 0x416 + 2 * slot, area[0].to_bytes(2, "little"))
    area[0] += len(data)
    next_slot[0] = 240 + (slot - 239) % 10
    return slot

# -- the far routines with no C caller
for n in range(0, 7):
    call_far("GetDriverTable", 0x4102, [(n, 2)], ctypes.c_uint16, 2)
for n in (2, 1, 0):      # 2 reads past the two entries (gScriptFlag and code): the table is put back by 0
    call_far("SelectDriverTable", 0x40E5, [(n, 2)], ctypes.c_uint16, 2)
call_far("GetScriptFlag", 0x03A1, [], ctypes.c_uint32, 4)
call_far("GetSampleOffset", 0x4CAC, [], ctypes.c_uint16, 2)
for v in range(10):
    call_far("GetVoiceDelay", 0x4CB1, [(v | (rng.randrange(256) << 8), 2)], ctypes.c_uint16, 2)
call_far("SetSamplePlaying", 0x40CE, [(rng.randrange(0x10000), 2)], None, 0)

import time
t0 = time.time()
for t in range(TICKS):
    if t % 50 == 0:
        print("tick", t, "%.0fs" % (time.time() - t0), dict(stats), "area %04X" % area[0], flush=True)
    x = rng.random()
    if x < 0.06:
        call("QueueSound", rng.choice(sounds))
    elif x < 0.16:
        call("QueueSound", new_sound())
    if rng.random() < 0.02:
        call("SetSoundByte", rng.choice(sounds), rng.choice((1, 3)), rng.randrange(0, 0x40))
        call("GetSoundByte", rng.choice(sounds), rng.choice((1, 3)))
    if rng.random() < 0.03:
        call_far("SetNote", 0x409E, [(rng.randrange(0, 0x40) | rng.choice([0, 0x8000]), 2)], None, 0)
    if rng.random() < 0.02:
        d.write(CS, 0x3B36, bytes([rng.randrange(0, 3)]))       # gBusyCountdown
    if rng.random() < 0.03:
        d.write(CS, 0x3B6F, b"\x01")                            # gTimerPasses48: chain on this pass
    if rng.random() < 0.02:
        d.write(CS, 0x3B4A, bytes([rng.randrange(256)]))        # gRegBDDepth
    if rng.random() < 0.75:
        d.write(CS, 0x3B6E, b"\x01")                            # gTimerPasses12: tick on this pass
    for v in range(10):
        call_far("GetVoiceDelay", 0x4CB1, [(v, 2)], ctypes.c_uint16, 2) if rng.random() < 0.01 else None
    call("AdlibTimerIsr", ds=rng.randrange(0, 0x10000), es=rng.randrange(0, 0x10000))

# -- the sample player: random samples, nibbles, lengths
for t in range(200):
    seg, off = rng.choice([(hybrid.DGROUP, rng.randrange(0x10000)), (0x5000, rng.randrange(0, 0xFFFF))])
    d.write(CS, 0x3B74, bytes([rng.randrange(256), rng.randrange(256)]))
    ln = rng.choice([1, 2, 3, 100, 0x8000, 0])
    start = (off - rng.randrange(0, 4)) & 0xFFFF
    d.write(CS, 0x3B76, ln.to_bytes(2, "little") + off.to_bytes(2, "little") + seg.to_bytes(2, "little") + start.to_bytes(2, "little"))
    d.write(CS, 0x3B7E, bytes([rng.choice([0, 1, 1, 1])]))
    d.write(CS, 0x3B6A, rng.randrange(0x10000).to_bytes(2, "little") + rng.randrange(0x10000).to_bytes(2, "little"))
    vec = d.read(0, 0x20, 4)
    call("SampleTimerIsr", ds=rng.randrange(0, 0x10000))
    d.write(0, 0x20, vec)
call("StopAdlibDriver")
call("StartAdlibDriver")
call("StopAdlibDriver")

for k, v in sorted(stats.items()):
    print(k, v)
print("adlib writes in the log:", len(getattr(m, "adlib_log", [])))
for b_ in bad:
    print(b_)
# coverage
code = d.read(CS, 0, 0x5100)
md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_16)
insns = [i.address for i in md.disasm(code[0x3E66:0x4F94], 0x3E66)] + [i.address for i in md.disasm(code[0x3A1:0x416], 0x3A1)]
missed = sorted(a for a in insns if a not in hit)
print("instructions: %d, executed: %d, never: %d" % (len(insns), len(insns) - len(missed), len(missed)))
print("never:", " ".join("%04X" % a for a in missed))
tbl = [int.from_bytes(code[0x4F94 + 2 * i:0x4F96 + 2 * i], "little") for i in range(0x4A)]
print("command handlers never dispatched:", sorted("%02X->%04X" % (0x80 + i, t) for i, t in enumerate(tbl) if t not in targets))
sys.exit(1 if bad else 0)

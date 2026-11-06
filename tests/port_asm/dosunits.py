"""The DOS memory chain, the console string, the drives and the driver loader (port/asm/E1990.c,
E19EA.c, E1A19.c, E1C00.c) against the original's (playbook 80, M3), each routine called by
hybrid.Direct, the original then the C on the same state, comparing memory (the MCBs, the PSP's words,
the unit's code-segment words, DGROUP), the registers returned and the devices and files. The bench
first runs the start-up to main (07C8:0001), so DOS memory and the far heap are as the game starts
with them.

  - PrintString: strings with their NUL, empty, long, with every byte value but 0 (0FFh among them,
    function 06h's input request), one that wraps at the end of its segment, one whose terminator is
    whatever follows in memory, in DGROUP and in another segment;
  - FindLastDrive and IsLastLogicalDrive: on the machine's DOS (every drive mapped), then through an
    INT 21h handler of the test that answers 19h, 0Eh and 440Eh from tables: default drives and drive
    counts at and past the last (a count of 0 among them), IOCTL answers with the carry clear (AL 0,
    the drive's own letter, another) and set (AL 0Fh, 1, others), for every drive number 0 to 0FFh;
  - CheckMemoryChain, ReserveMemoryBelowVideo, TrimMemoryToVideo (and through them CheckMCB): MCB
    chains written from the one-paragraph block DOS gives, intact and damaged (a type that is neither
    'M' nor 'Z', a chain whose last block is not reached), with free neighbours to merge, the
    environment block in the chain (moved) and not, its copy's allocation failing, DOS out of memory
    for the one-paragraph block; the video probe with marker bytes planted in the probed segments;
    reserve counts 1 to 8; the moved block or none; memory tops above and below the chain's end;
  - RunOverlayRequest (and through it LoadOverlayImage): each graphics driver the game ships (cga, tga,
    ega, mcga; the names in DGROUP the game uses), vga.ovl (not shipped) and a name that is no file,
    with the mouse handler installed and not, and MemAlloc failing (the heap's top lowered, the failure
    hook a stub that returns 0).

The driver's entry returns BX and its link routine CF, which the C reads back from the registers the
bench leaves after the original's code ran for it (tc_hybrid_far_regs). Exit status 1 if a call
differs or does not run.

  dosunits.py [LIB]
"""
import copy
import random
import struct
import sys
from collections import Counter
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools" / "port"))
sys.path.insert(0, str(ROOT / "tools"))
import hybrid  # noqa: E402
from oracle.machine import Stop  # noqa: E402

LIB = Path(sys.argv[1]) if len(sys.argv) > 1 else hybrid.LIB

DG = hybrid.DGROUP
CSEG = hybrid.LOAD_SEG + 0x1990          # the unit's code segment: its words
PSP = 0x0FF0
rng = random.Random(23)
bad, seen, values = [], Counter(), Counter()
d = hybrid.Direct(lib=LIB)

started = []


def at_main(mach):
    if not started:
        started.append(True)
        raise Stop("main")


d.m.add_breakpoint(hybrid.LOAD_SEG + 0x07C8, 0x0001, at_main)
assert d.m.run(5_000_000) == "main"
ARENA0 = copy.deepcopy(d.m.dos.arena)
PSP2C = d.read(PSP, 0x2C, 2)


def call(name, *args, case=None):
    r = d.call(name, *args)
    seen[(name, r["status"][:40])] += 1
    if name in ("CheckMemoryChain", "IsLastLogicalDrive", "RunOverlayRequest", "ReserveMemoryBelowVideo"):
        values[(name, "%04X" % (r.get("value", 0) & 0xFFFF))] += 1
    if r["status"] != "same":
        bad.append(dict(r, args=[hex(x) if isinstance(x, int) else (hex(x[0]), x[1]) for x in args], case=case))
    return r


def ww(seg, off, v):
    d.write(seg, off, struct.pack("<H", v & 0xFFFF))


def far(seg, off):
    return ((seg << 16) | off, 4)


# ---- PrintString
def string(seg, off, data):
    d.write(seg, off, data)
    call("PrintString", far(seg, off), case=data[:16])


string(DG, 0x9000, b"Hello, world\r\n\0")
string(DG, 0x9000, b"\0")
string(DG, 0x9000, bytes(rng.randrange(0x20, 0x7F) for _ in range(300)) + b"\0")
string(DG, 0x9000, bytes(range(1, 256)) + b"\0")
string(DG, 0x9000, b"\xff\xffA\0")
string(0x8000, 0x0123, b"another segment\r\n\0")
string(DG, 0xFFF8, b"wrapping")                   # goes on at DG:0000 to the first NUL there
d.write(0x8000, 0x0000, bytes(rng.randrange(1, 256) for _ in range(100)))
call("PrintString", far(0x8000, 0x0000), case="no NUL of its own")


# ---- the drives
for _ in range(3):
    call("FindLastDrive")
for drive in list(range(0, 30)) + [0x7F, 0x80, 0xFF, 0x100, 0x1FF, -1]:
    call("IsLastLogicalDrive", drive)

# an INT 21h handler at 8000:0400 answering 19h (AL = cs:[0]), 0Eh (AL = cs:[1]) and 440Eh (AL =
# cs:[100h + BL], the carry when cs:[300h + BL] is not 0), each with `retf 2`
HANDLER = bytes.fromhex(
    "80FC19" "7508" "2EA00000" "F8" "CA0200"       # 0400 cmp ah,19h; jne 040D; mov al,cs:[0]; clc; retf 2
    "80FC0E" "7508" "2EA00100" "F8" "CA0200"       # 040D cmp ah,0Eh; jne 041A; mov al,cs:[1]; clc; retf 2
    "53" "32FF" "2E8A870001" "2E80BF000300" "5B"   # 041A push bx; xor bh,bh; mov al,cs:[bx+100h];
                                                   #      cmp byte cs:[bx+300h],0; pop bx
    "7404" "F9" "CA0200" "F8" "CA0200")            # 0429 je 042F; stc; retf 2; clc; retf 2
HSEG = 0x8000
d.write(HSEG, 0x0400, HANDLER)
old21 = d.read(0, 0x84, 4)


def drive_tables(default, count, last, kind):
    """IOCTL answers: drives above `last` fail with 0Fh (invalid drive); `last` and below answer
    by `kind`"""
    al, cf = bytearray(256), bytearray(256)
    for bl in range(256):
        drv = (bl - 1) & 0xFF
        if drv > last:
            al[bl], cf[bl] = 0x0F, 1
        elif kind == "zero":
            al[bl], cf[bl] = 0, 0
        elif kind == "own":
            al[bl], cf[bl] = bl, 0
        else:
            k = rng.random()
            if k < 0.25:
                al[bl], cf[bl] = 0x0F, 1
            elif k < 0.4:
                al[bl], cf[bl] = 1, 1
            elif k < 0.5:
                al[bl], cf[bl] = rng.choice((2, 5, 0x0E, 0x10, 0xFF)), 1
            elif k < 0.65:
                al[bl], cf[bl] = 0, 0
            elif k < 0.85:
                al[bl], cf[bl] = bl, 0
            else:
                al[bl], cf[bl] = rng.randrange(256), 0
    # the scan ends: drive 0 answers anything but 0Fh with the carry
    if al[1] == 0x0F and cf[1]:
        al[1], cf[1] = 0, 0
    d.write(HSEG, 0, bytes([default, count]))
    d.write(HSEG, 0x100, bytes(al))
    d.write(HSEG, 0x300, bytes(cf))


d.write(0, 0x84, struct.pack("<HH", 0x0400, HSEG))
for k in range(60):
    kind = ("zero", "own", "mixed")[k % 3]
    count = rng.choice((0, 1, 2, 3, 5, 26, 27, 32, 0x80, 0xFF, rng.randrange(256)))
    last = rng.choice((0, 1, 2, 3, 4, 25, 26, 0xFE, rng.randrange(256)))
    drive_tables(rng.randrange(26), count, last, kind)
    call("FindLastDrive", case=(count, last, kind))
    for drive in rng.sample(range(256), 12) + [last, (last + 1) & 0xFF, count & 0xFF]:
        call("IsLastLogicalDrive", drive, case=(drive, last, kind))
d.write(0, 0x84, old21)


# ---- the DOS memory chain
def tiny_seg():
    """the segment the oracle's DOS gives a one-paragraph block now (Dos.f48's first gap)"""
    blocks = sorted(d.m.dos.arena, key=lambda b: b[0])
    prev = 0x1000
    for b in blocks:
        if b[0] - prev > 0:
            return prev
        prev = max(prev, b[0] + b[1])
    return prev


def mcb(seg, typ, owner, size):
    d.write(seg, 0, struct.pack("<BHH", typ, owner, size))


def chain(start, blocks):
    """writes MCBs from `start`: (type, owner, size) each; returns their segments"""
    segs, s = [], start
    for typ, owner, size in blocks:
        mcb(s, typ, owner, size)
        segs.append(s)
        s = (s + size + 1) & 0xFFFF
    return segs


def owner():
    return rng.choice((0, 0, PSP, PSP, 0x1234, 0x0008))


def reset_dos():
    d.m.dos.arena = copy.deepcopy(ARENA0)
    d.write(PSP, 0x2C, PSP2C)


def cs_words(mem_top, video_top, moved_seg, moved_paras, new_top, mcb_seg):
    d.write(CSEG, 2, struct.pack("<6H", mem_top, video_top, moved_seg, moved_paras, 0, new_top))
    ww(CSEG, 0x0E, mcb_seg)


for k in range(160):
    reset_dos()
    t = tiny_seg() - 1
    room = rng.choice((0x400, 0x600, 0x800))         # the tiny MCB's size: the chain starts past the copy
    n = rng.randrange(1, 9)
    blocks = [(0x4D, PSP, room)] + [(0x4D, owner(), rng.randrange(0, 0x200)) for _ in range(n)]
    blocks.append((0x5A, owner(), rng.randrange(0, 0x400)))
    env = None
    kind = rng.random()
    if kind < 0.45:                                  # the environment block in the chain
        i = rng.randrange(1, len(blocks))
        size = rng.randrange(1, room - 2)          # the copy stays below the next block
        if rng.random() < 0.15:
            size = 0x6000                            # its copy's allocation fails
        blocks[i] = (blocks[i][0], PSP, size)
        env = i
    if 0.45 <= kind < 0.6:                           # damaged: a type that is neither 'M' nor 'Z'
        i = rng.randrange(1, len(blocks))
        blocks[i] = (rng.choice((0, 0x4C, 0x4E, 0x5B, 0xFF)),) + blocks[i][1:]
    segs = chain(t, blocks)
    if env is not None:
        ww(PSP, 0x2C, segs[env] + 1)
    ww(CSEG, 0x06, 0)
    ww(CSEG, 0x08, 0)
    call("CheckMemoryChain", case=("chain", k, kind, env))
    if struct.unpack("<H", d.read(CSEG, 0x06, 2))[0]:
        values[("CheckMemoryChain", "moved the environment")] += 1

# DOS out of memory for the one-paragraph block
reset_dos()
d.m.dos.arena.append([tiny_seg(), 0xA000 - tiny_seg(), PSP])
call("CheckMemoryChain", case="no memory")
reset_dos()

for k in range(120):
    reset_dos()
    t = tiny_seg() - 1
    blocks = [(0x4D, PSP, rng.choice((0, 1, 0x10)))]
    blocks += [(0x4D, owner(), rng.randrange(0, 0x300)) for _ in range(rng.randrange(0, 6))]
    if len(blocks) > 1 and rng.random() < 0.2:      # damaged before the end: Reserve walks past it
        blocks[-1] = (0x4E,) + blocks[-1][1:]
    blocks.append((0x5A, owner(), rng.randrange(0, 0x800)))
    segs = chain(t, blocks)
    moved, paras = 0, 0
    if rng.random() < 0.6:                           # a block CheckMemoryChain moved
        paras = rng.randrange(1, 0x80)
        moved = rng.randrange(0x7000, 0x7800)       # above the chain, below the probe's markers
        mcb(moved - 1, 0x4D, PSP, paras - 1)
        d.write(moved, 0, bytes(rng.randrange(256) for _ in range(min(paras * 16, 64))))
        if rng.random() < 0.2:
            paras = rng.randrange(0, 0x100)          # gMovedParas not the copy's own size
    mcb_seg = rng.choice((t, segs[-1], segs[0]))
    cs_words(rng.randrange(0x10000), 0, moved, paras, 0, mcb_seg)
    for seg in rng.sample(range(0x7C00, 0xA000, 0x400), rng.randrange(0, 4)):
        d.write(seg, 0, b"\xc3")                     # a marker byte where the probe looks
    # one step leaves the video top at A000h, so a moved block's copy would end just below it, over the
    # bench's own stack (Direct.STACK, 9000:FFF0): one step only without one
    steps = rng.choice((2, 3, 3, 4, 5, 8) if moved else (1, 2, 3, 4, 8))
    call("ReserveMemoryBelowVideo", steps, case=("reserve", k, steps, moved, paras))

for k in range(80):
    reset_dos()
    t = tiny_seg() - 1
    blocks = [(0x4D, PSP, rng.choice((0, 1, 0x10)))]
    blocks += [(0x4D, owner(), rng.randrange(0, 0x300)) for _ in range(rng.randrange(0, 6))]
    blocks.append((0x5A, owner(), rng.randrange(0, 0x800)))
    segs = chain(t, blocks)
    top = rng.choice((0, segs[-1], segs[-1] + 1, segs[-1] + 0x20, 0x9000, 0xA000, rng.randrange(0x10000)))
    ww(CSEG, 0x02, top)
    call("TrimMemoryToVideo", case=("trim", k, top))
reset_dos()


# ---- the driver loader
REQ = 0x8121
TABLE = d.read(DG, 0x802A, 47 * 4)                   # what the request's +4 and +6 point at: the 47 slots
NAMES = {"cga.ovl": 0x30C1, "tga.ovl": 0x30C9, "ega.ovl": 0x30D1, "mcga.ovl": 0x30D9, "vga.ovl": 0x30E2}
d.write(DG, 0x9100, b"NOPE.OVL\0")
NAMES["NOPE.OVL"] = 0x9100
STUB = (0x8000, 0x0500)                              # xor ax,ax; retf: a failure hook that gives up
d.write(*STUB, bytes.fromhex("31C0CB"))


def request(name, mouse, table=TABLE):
    d.write(DG, 0x802A, table)
    ww(DG, REQ, NAMES[name])
    ww(DG, REQ + 2, DG)
    ww(DG, 0x8148, mouse)
    ww(DG, 0x814A, rng.choice((0, 1)))
    r = call("RunOverlayRequest", far(DG, REQ), case=(name, mouse, table != TABLE))
    print(name, mouse, "table changed" if table != TABLE else "", r["status"], hex(r.get("value", 0)))
    # give the block back (MemFree, on the bench as well), so the heap holds every case: the driver's
    # block once MemRealloc has resized it (its segment at 8190h), the 0FA00h block when the load failed
    # (the load segment at 818Ah, one past the block's header paragraph)
    word = 0x818A if NAMES[name] in (0x30E2, 0x9100) else 0x8190
    call("MemFree", far(struct.unpack("<H", d.read(DG, word, 2))[0], 0))
    return r


# the link routine failing (the carry set): a zero pair in the table sends it to the driver's exports at
# that point, and a driver slot it then reads may be 0. Not ega.ovl's: its link routine then calls an
# import slot the table did not fill (`lcall cs:[1Bh]` at its 0F3Fh), 0000:0000 on the bench
for name in ("cga.ovl", "mcga.ovl", "tga.ovl"):
    request(name, 1, bytes(4) + TABLE[4:])
    for _ in range(2):
        t = bytearray(TABLE)
        i = rng.randrange(1, 12) * 4
        t[i:i + 4] = bytes(4)
        request(name, 1, bytes(t))
for name in ("cga.ovl", "tga.ovl", "ega.ovl", "mcga.ovl", "vga.ovl", "NOPE.OVL"):
    for mouse in (0, 1):
        request(name, mouse)
# MemAlloc failing: the heap's top just past the break level, the failure hook the stub
top = d.read(DG, 0x008B, 4)
hook = d.read(DG, 0x810A, 4)
brk = struct.unpack("<HH", d.read(DG, 0x0087, 4))
d.write(DG, 0x008B, struct.pack("<HH", 0, brk[1] + 0x100))
d.write(DG, 0x810A, struct.pack("<HH", STUB[1], STUB[0]))
d.write(DG, 0x802A, TABLE)
ww(DG, REQ, NAMES["mcga.ovl"])
call("RunOverlayRequest", far(DG, REQ), case="no memory")
d.write(DG, 0x008B, top)
d.write(DG, 0x810A, hook)

for k, n in sorted(seen.items()):
    print(k, n)
for k, n in sorted(values.items()):
    print("value", k, n)

# QUIRK 1990:01CE and 1990:0296: when the one-paragraph block is the last ('Z'), the original returns
# through the wrong stack words (the C stops there with the quirk's words); last, as the bench is left
# where the original went
for name, args in (("ReserveMemoryBelowVideo", (3,)), ("TrimMemoryToVideo", ())):
    reset_dos()
    t = tiny_seg() - 1
    chain(t, [(0x5A, PSP, 0x10)])
    r = d.call(name, *args)
    print("QUIRK", name, r["status"])
    if not r["status"].startswith("not tested"):
        bad.append(dict(r, case="the quirk's exit"))
for b in bad[:10]:
    print(b)
sys.exit(1 if bad else 0)

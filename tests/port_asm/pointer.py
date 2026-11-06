"""The mouse and the game's own pointer in C (port/asm/E1A72.c, E1BB1.c, E1BBE.c, E1BF3.c) against the
original, by Direct calls on the bench (playbook 80, M3). The bench runs the program with the INT 33h
driver until its first SetMousePointerShape (the graphics driver loaded, the heap and the mouse set up,
the item icons read), then calls every routine, the original then the C, over random states: the mouse
disabled or not, the hide count at 0, 1, 2 and its saturation, the zone state (8000h, 4000h, the
repeat byte at 0, 1 and FFh), zones overlapping the pointer or not, the drawn position equal to the
cursor or not, moves to and past the screen's edges and the clamp limits, the event handler with
every button word (the middle-button mask on and off), x in both widths, the busy flag set, each record
mode; the pointer's shape with real and resized icons and odd hot spots, the buffer kept, resized and
refused; the far pointer at ds:8172; the driver's install with a driver, a null vector, an IRET
vector, a reset that fails and one in the 320 wide mode; its removal installed or not. The mouse
driver's own state (its position, its handler, its visibility) is compared too.

Exit status 1 if any call differs or does not run.

  pointer.py [LIB] [STEPS]
"""
import collections
import ctypes
import random
import struct
import sys
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools" / "port"))
sys.path.insert(0, str(ROOT / "tools"))
import hybrid
from oracle.machine import Stop
from oracle.input import ROM_MOUSE, no_mouse

LIB = Path(sys.argv[1]) if len(sys.argv) > 1 else hybrid.LIB
STEPS = int(sys.argv[2]) if len(sys.argv) > 2 else 1500
DG = hybrid.DGROUP
L = hybrid.LOAD_SEG
rng = random.Random(1991)
d = hybrid.Direct(lib=LIB, mouse=True)
m = d.m
results = collections.defaultdict(collections.Counter)
cov = collections.Counter()
bad = []

# the mouse driver's state is compared with the rest of the outside world
_outside = d.bench.outside


def outside():
    o = _outside()
    mo = m.mouse
    o["mouse"] = (mo.x, mo.y, mo.buttons, mo.visible, mo.handler, mo.range_x, mo.range_y)
    return o


d.bench.outside = outside

# the start-up, to the first SetMousePointerShape
first = []


def at_shape(mach):
    if not first:
        ss, sp = mach.get("ss"), mach.get("sp")
        first.append(tuple(mach.rw(ss, sp + k) for k in (4, 6, 8, 10)))
        raise Stop("shape")


m.add_breakpoint(L + 0x1BB1, 0x0002, at_shape)
assert m.run(50_000_000) == "shape", "the program never set the pointer's shape"
HOT_X, HOT_Y, ICON_OFF, ICON_SEG = first[0]
icons = [(ICON_SEG << 16) | ICON_OFF]
for i in range(1, 40):                                   # gItemIcons (ds:DDE4)
    p = struct.unpack("<I", d.read(DG, 0xDDE4 + 4 * i, 4))[0]
    if p >> 16:
        icons.append(p)


def w16(seg, off, v): d.write(seg, off, struct.pack("<H", v & 0xFFFF))
def r16(seg, off): return struct.unpack("<H", d.read(seg, off, 2))[0]
def w8(seg, off, v): d.write(seg, off, bytes([v & 0xFF]))


# the calls' stack and the scratch (resized copies of the first icon, a byte for an IRET vector) in
# blocks of the game's own heap, so that no save-under buffer the heap gives later lands on them
def heap_block(size):
    r = d.call("MemAlloc", (size, 4), 0)
    assert r["status"] == "same" and r["value"] >> 16, r
    return r["value"]


stack_block = heap_block(0x1000)
d.STACK = (stack_block >> 16, (stack_block & 0xFFFF) + 0xFF0)
scratch_block = heap_block(0x4400)
SCRATCH, SCRATCH_OFF = scratch_block >> 16, scratch_block & 0xFFFF
IRET_OFF = SCRATCH_OFF + 0x4000
base_icon = d.read(ICON_SEG, ICON_OFF, 0x400)


def synthetic_icon(k):
    off = SCRATCH_OFF + 0x400 * k
    rec = bytearray(base_icon)
    rec[2] = rng.choice((0, 1, 2, 3, 5, rng.randrange(0, 8), 0x7F, 0xFF))
    rec[3] = rng.choice((0, 1, 8, 16, 33, rng.randrange(0, 64), 0xC8, 0xFF))
    d.write(SCRATCH, off, bytes(rec))
    return (SCRATCH << 16) | off


def call(name, *args, **kw):
    r = d.call(name, *args, **kw)
    results[name][r["status"]] += 1
    if r["status"] != "same":
        bad.append(dict(r, args=[hex(a) if isinstance(a, int) else (hex(a[0]), a[1]) for a in args],
                        state=snapshot()))
    return r


def snapshot():
    return {"%04X" % o: "%04X" % r16(DG, o) for o in range(0x812E, 0x818A, 2)}


def s16(v):
    return v & 0xFFFF


def coord(lo, hi):
    """a coordinate: mostly on screen, sometimes at or past an edge, sometimes far off"""
    k = rng.random()
    if k < 0.6:
        return rng.randrange(lo, hi + 1)
    if k < 0.85:
        return rng.choice((lo - 1, lo, hi, hi + 1, lo - 8, hi + 8, lo - 40, hi + 40))
    return rng.randrange(-0x8000, 0x8000)


def drawn_rect():
    """the pointer's rectangle as HidePointerZone computes it, in pixels"""
    x = (r16(DG, 0x815C) - r16(DG, 0x8158)) & 0xFFF8
    y = (r16(DG, 0x815E) - r16(DG, 0x815A)) & 0xFFFF
    return x, y, (x + 8 * r16(DG, 0x8156)) & 0xFFFF, (y + r16(DG, 0x8154)) & 0xFFFF


def zone():
    """a zone overlapping the pointer's rectangle, touching it, or apart"""
    x0, y0, x1, y1 = drawn_rect()
    k = rng.random()
    if k < 0.45:
        l, t = x0 - rng.randrange(0, 40), y0 - rng.randrange(0, 40)
        return l, t, x0 + rng.randrange(0, 60), y0 + rng.randrange(0, 60)
    if k < 0.6:
        return x1, y1, x1 + rng.randrange(0, 30), y1 + rng.randrange(0, 30)      # touching the corner
    l, t = coord(0, 319), coord(0, 199)
    return l, t, l + rng.randrange(-10, 120), t + rng.randrange(-10, 80)


def shake(handler=False):
    """a random pointer state; the busy flag stays free unless the caller is the handler"""
    w16(DG, 0x814A, 1 if rng.random() < 0.12 else 0)                  # gMouseDisabled
    w16(DG, 0x8146, rng.choice((0, 0, 0, 0, 1, 1, 2, 3, 0xFFFF, 0xFFFE)))   # gHideDepth
    st = rng.choice((0, 0, 0, 0x8000, 0xC000, 0x4000))
    if st & 0x8000:
        st |= rng.choice((0, 0, 1, 2, 0xFE, 0xFF, rng.randrange(256)))
    w16(DG, 0x8160, st)                                                # gPointerState
    if rng.random() < 0.5:
        z = zone()
        for o, v in zip((0x8162, 0x8164, 0x8166, 0x8168), z):
            w16(DG, o, v)
    x, y = coord(0, 319), coord(0, 199)
    w16(DG, 0x8138, x)
    w16(DG, 0x813A, y)
    if rng.random() < 0.4:
        w16(DG, 0x815C, x)                                             # gDrawnX equal to the cursor
        w16(DG, 0x815E, y if rng.random() < 0.7 else coord(0, 199))
    else:
        w16(DG, 0x815C, coord(0, 319))
        w16(DG, 0x815E, coord(0, 199))
    if rng.random() < 0.25:                                            # the hot spot
        w16(DG, 0x8158, rng.choice((0, 8, 3, 15, -4, 40)))
        w16(DG, 0x815A, rng.choice((0, 8, 7, 15, -4, 40)))
    w16(DG, 0x8132, rng.choice((2, 2, 0, 1, 3, 6, rng.randrange(0x10000))))   # gMiddleButtonBits
    w16(DG, 0x8130, rng.choice((0, 0, 1, 2, 3, rng.randrange(8))))     # gLastMouseButtons
    w16(DG, 0x8144, 1 if rng.random() < 0.8 else 0)                   # gMouseXDoubled
    w8(DG, 0x80EC, rng.choice((0, 0, 0, 1, 2)))                        # gRecordMode
    if rng.random() < 0.15:                                            # the clamp limits
        for o, v in ((0x814C, coord(-10, 20)), (0x814E, coord(200, 330)), (0x8150, coord(-10, 20)),
                     (0x8152, coord(100, 210))):
            w16(DG, o, v)
    else:
        for o, v in ((0x814C, 0), (0x814E, 0x13F), (0x8150, 0), (0x8152, 0xC7)):
            w16(DG, o, v)
    w16(DG, 0x812E, 1 if handler and rng.random() < 0.15 else 0)      # gPointerBusy


def event_handler():
    shake(handler=True)
    cond = rng.choice((1, 2, 4, 8, 0x10, 3))
    bx = rng.choice((0, 0, 1, 2, 3, 4, 5, 6, 7, rng.randrange(0x10000)))
    cx = rng.choice((rng.randrange(0, 640), rng.randrange(0, 320), 0, 639, 0x13F, 0x140, 0x27E, 0x27F,
                     rng.randrange(0x10000)))
    dx = rng.choice((rng.randrange(0, 200), 0, 199, 200, rng.randrange(0x10000)))
    if rng.random() < 0.3:
        dx = r16(DG, 0x813A)                                           # y unchanged
    if rng.random() < 0.3:
        cx = (r16(DG, 0x8138) << (1 if r16(DG, 0x8144) == 1 else 0)) & 0xFFFF   # x unchanged
    record = struct.pack("<6H", cond, bx, cx, dx, 0, 0)
    m.uc.mem_write(m.linear(*ROM_MOUSE) + 0x80, record)
    # the C's register record: the trampoline's data in the arena (the bench maps only the RAM below
    # A0000h onto the library's arena, so its ROM is apart)
    ctypes.memmove(d.bench.base + m.linear(*ROM_MOUSE) + 0x80, record, len(record))
    for reg, v in (("ax", cond), ("bx", bx), ("cx", cx), ("dx", dx), ("si", 0), ("di", 0)):
        m.set(reg, v)
    ds = DG if rng.random() < 0.7 else rng.choice((SCRATCH, L, 0))
    r = call("MouseEventHandler", ds=ds)
    cov["handler drawn" if r16(DG, 0x815C) == r16(DG, 0x8138) else "handler not drawn"] += 1


def set_shape():
    shake()
    w16(DG, 0x8146, rng.choice((0, 0, 0, 1, 2)))
    k = rng.random()
    if k < 0.5:
        icon = rng.choice(icons)
    else:
        icon = synthetic_icon(rng.randrange(16))
    hx = rng.choice((0, 8, 0, 8, 1, 3, 7, 15, 16, -1, -8, 100, rng.randrange(0x10000)))
    hy = rng.choice((0, 8, 0, 8, 1, 5, 9, 31, -1, -8, 100, rng.randrange(0x10000)))
    if rng.random() < 0.1 and r16(DG, 0x816C):
        # no buffer yet: the old one goes back to the heap first (the heap does not grow)
        call("MemFree", ((r16(DG, 0x816C) << 16) | r16(DG, 0x816A), 4))
        w16(DG, 0x816C, 0)
    seg = r16(DG, 0x816C)
    r = call("SetMousePointerShape", hx, hy, (icon, 4))
    v = r.get("value", 0) & 0xFFFF
    cov["shape returns %d%s" % (v, ", had a buffer" if seg else "")] += 1


def set_far_pointer():
    if rng.random() < 0.2:
        w16(DG, 0x8174, 0)
    p = rng.choice((0, (SCRATCH << 16) | rng.randrange(SCRATCH_OFF, SCRATCH_OFF + 0x3FF0), (DG << 16) | rng.randrange(0xF000, 0xFFF0),
                    (DG << 16) | 0xFFF8, rng.randrange(0x10000)))
    if p and rng.random() < 0.5:
        d.write(p >> 16, p & 0xFFFF, bytes(rng.randrange(256) for _ in range(16)))
    call("SetFarPointer", (p, 4))
    call("GetFarPointer")


ivt33 = d.read(0, 0x33 * 4, 4)
mouse_int33 = m.mouse.int33


def init_mouse():
    k = rng.random()
    d.write(0, 0x33 * 4, ivt33)
    m.register_service(0x33, mouse_int33)
    if k < 0.1:
        d.write(0, 0x33 * 4, bytes(4))                                 # a null vector
        what = "null vector"
    elif k < 0.2:
        d.write(0, 0x33 * 4, struct.pack("<HH", IRET_OFF, SCRATCH))    # a vector at an IRET
        d.write(SCRATCH, IRET_OFF, b"\xCF")
        what = "IRET vector"
    elif k < 0.35:
        no_mouse(m)                                                    # a reset that answers 0
        what = "no driver"
    elif k < 0.5:
        def narrow(mach):                                              # the 320 wide mode
            mouse_int33(mach)
            if mach.get("ax") == 0xFFFF and m.mouse.x == 320:
                m.mouse.x = 0xA0
        m.register_service(0x33, narrow)
        what = "320 wide"
    else:
        what = "driver"
    shake()
    call("InitMouse")
    cov["InitMouse " + what] += 1
    d.write(0, 0x33 * 4, ivt33)
    m.register_service(0x33, mouse_int33)
    w16(DG, 0x814A, 0)
    if r16(DG, 0x8148) == 0:
        w16(DG, 0x8148, 1)


def step():
    k = rng.random()
    if k < 0.10:
        shake()
        depth = r16(DG, 0x8146)
        call("HidePointer")
        cov["hide at depth %s" % ("0" if depth == 0 else "FFFF" if depth == 0xFFFF else "n")] += 1
    elif k < 0.20:
        shake(); call("ShowPointer")
    elif k < 0.27:
        shake(); call("LockHidePointer")
    elif k < 0.34:
        shake(); call("LockShowPointer")
    elif k < 0.43:
        shake()
        st = r16(DG, 0x8160)
        call("HidePointerZone", *zone())
        cov["zone hide, %s" % ("repeated" if st & 0x8000 else
                               "overlapping" if r16(DG, 0x8160) & 0x4000 else "disjoint")] += 1
    elif k < 0.50:
        shake(); call("ShowPointerZone")
    elif k < 0.56:
        shake(); call("MovePointerTo", coord(0, 319), coord(0, 199))
    elif k < 0.63:
        shake(); call("MovePointerBy", rng.choice((coord(-20, 20), coord(-400, 400))),
                      rng.choice((coord(-20, 20), coord(-250, 250))))
    elif k < 0.68:
        shake(); call("IsMouseInRect", *zone())
    elif k < 0.85:
        event_handler()
    elif k < 0.92:
        set_shape()
    elif k < 0.95:
        set_far_pointer()
    elif k < 0.97:
        call(rng.choice(("EnableInterrupts", "DisableInterrupts")))
    elif k < 0.985:
        init_mouse()
    else:
        w16(DG, 0x8148, rng.choice((0, 1, 1)))
        call("RemoveMouseHandler")
        w16(DG, 0x8148, 1)
        m.mouse.handler = (L + 0x1A72, 0x017A, 0x1F)


# the game's own first call, then the sequence
call("SetMousePointerShape", HOT_X, HOT_Y, (icons[0], 4))
call("LockShowPointer")
for i in range(STEPS):
    step()
    if i % 100 == 0:
        d.clear_logs()
# a nested hide and show pair, as the game makes them
for _ in range(20):
    shake()
    w16(DG, 0x8160, 0)
    z = zone()
    call("HidePointerZone", *z)
    call("HidePointerZone", *z)
    call("ShowPointerZone")
    call("ShowPointerZone")

# the buffer refused: the heap is taken by MemAlloc until its largest block is small, then shapes of
# other sizes are set, each from that same state: the first-time failure (no buffer yet), a failure
# that frees the old buffer first, then the freed buffer kept by a call of the same size (QUIRK
# 1BB1:0058). A different size then would free it twice, which leaves the library's heap undefined
# (the C library's and the original's may then part), so the bench goes back to its state before.
def bench_save():
    return d.bench.memory(), d.bench.save()


def bench_load(saved):
    ctypes.memmove(d.bench.base, saved[0], hybrid.RAM)
    m.uc.ctl_remove_cache(0, hybrid.RAM)
    d.bench.restore(saved[1])


def largest():
    v = call("LargestFreeBlock").get("value", 0)
    return v - (1 << 32) if v & 0x80000000 else v


before_taking = bench_save()
for _ in range(64):
    n = largest()
    if n < 0x100:
        break
    p = call("MemAlloc", ((n - 0x40) if n > 0x40 else n, 4), 0).get("value", 0)
    if not p >> 16:
        break
cov["heap taken, largest %X" % largest()] += 1
taken = bench_save()
for first_time in (True, False, False, False):
    for _ in range(6):
        bench_load(taken)
        shake()
        if first_time:
            w16(DG, 0x816C, 0)
        icon = synthetic_icon(rng.randrange(16))
        had = r16(DG, 0x816C)
        r = call("SetMousePointerShape", rng.choice((0, 8, 3)), rng.choice((0, 8, 5)), (icon, 4))
        cov["shape returns %d%s (heap taken)" % (r.get("value", 0) & 0xFFFF, ", had a buffer" if had else "")] += 1
        if (r.get("value", 0) & 0xFFFF) == 0:
            shake()
            r = call("SetMousePointerShape", 0, 0, (icon, 4))          # the same size again
            cov["shape after a failure returns %d" % (r.get("value", 0) & 0xFFFF)] += 1
bench_load(before_taking)
for _ in range(10):
    set_shape()

for name in sorted(results):
    print("%-22s %s" % (name, dict(results[name])))
for k in sorted(cov):
    print("  %-40s %d" % (k, cov[k]))
for b in bad[:10]:
    print(b)
total = sum(sum(c.values()) for c in results.values())
print("%d calls, %d not the same" % (total, len(bad)))
sys.exit(1 if bad else 0)

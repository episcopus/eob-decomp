"""E1AAD.c (the keyboard driver in C) against the original, by Direct calls on the bench (playbook 80, M3):
the INT 9 handler on make, break, E0 and E1 scancodes with the ring full or not, the readers in every
record mode (mode 1 writes the record file, mode 2 replays it, mouse records and its end included), the
key states, the mouse buttons and the event queue with nearly full rings, install over random BIOS shift
bytes, removal. Exit status 1 if any call differs.

  e1aad.py [LIB] [CALLS] [ROUTINE,...]
"""
import sys, random, struct, collections
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools" / "port"))
import hybrid

LIB = Path(sys.argv[1] if len(sys.argv) > 1 else hybrid.LIB)
N = int(sys.argv[2]) if len(sys.argv) > 2 else 200
ONLY = sys.argv[3].split(",") if len(sys.argv) > 3 else None
d = hybrid.Direct(lib=LIB)
DG = hybrid.DGROUP
CS = hybrid.LOAD_SEG + 0x1AAD
rng = random.Random(1234)
results = collections.defaultdict(collections.Counter)
bad = []


def w8(seg, off, v): d.write(seg, off, bytes([v & 0xFF]))
def w16(seg, off, v): d.write(seg, off, struct.pack("<H", v & 0xFFFF))
def r16(seg, off): return struct.unpack("<H", d.read(seg, off, 2))[0]


cov = collections.Counter()


def call(name, *args, **kw):
    r = d.call(name, *args, **kw)
    if "value" in r:
        cov[name + " value=%X" % (r["value"] & 0xFFFF) if name != "KeyboardIsr" else "x"] += 0
        cov[name + (" zero" if (r["value"] & 0xFFFF) == 0 else " nonzero")] += 1
    results[name][r["status"]] += 1
    if r["status"] != "same":
        bad.append(r)
    return r


ivt9 = d.read(0, 9 * 4, 4)
ivt23 = d.read(0, 0x23 * 4, 4)


def install(b17, b18, b96, keep=False):
    d.write(0, 9 * 4, ivt9)
    d.write(0, 0x23 * 4, ivt23)
    w8(0x40, 0x17, b17); w8(0x40, 0x18, b18); w8(0x40, 0x96, b96)
    call("InstallKeyboard")


# files for the recording: slot 0 a write file, slot 1 a read file with random content
def make_file(name, data):
    f = d.m.vfs.open(name, True, create=True)
    f.data[:] = data
    return d.m.dos._alloc_handle(f)


HANDLES = []


def prepare_files():
    if not HANDLES:
        HANDLES.append(make_file("RECW.DAT", b""))
        HANDLES.append(make_file("RECR.DAT", b""))
    hw, hr = HANDLES
    f = d.m.dos.handles[hr]
    if rng.random() < 0.4:
        f.data[:] = struct.pack("<HHHH", rng.choice([0x41, 0x42, 0x43, 0x44, 0x0105, 0x10]), rng.randrange(20),
                                rng.randrange(0x10000), rng.randrange(0x10000))[:rng.choice([4, 6, 8, 8, 8])]
    else:
        f.data[:] = bytes(rng.randrange(256) for _ in range(rng.choice([0, 2, 4, 6, 8, 40])))
    f.pos = 0
    w16(DG, 0xFEEA, hw)
    w16(DG, 0xFEEC, hr)
    for h in HANDLES:                           # the runtime's _openfd (ds:8758): open, O_RDWR | O_BINARY
        w16(DG, 0x8758 + 2 * h, 0x8004)


def mouse_word():
    return rng.choice([0x41, 0x42, 0x43, 0x44, 0x0841, 0x0142, 0x0105, 0x0444])


def rand_event():
    c = rng.random()
    if c < 0.3:
        return rng.randrange(0x10000)
    if c < 0.5:
        return mouse_word()
    if c < 0.6:
        return rng.choice([0x1E, 0x2C, 0x39, 0x3A, 0x3C, 0x3E, 0x40, 0x5A, 0x80])
    if c < 0.7:
        return 0x0800 | rng.randrange(0x7A)
    if c < 0.8:
        return rng.randrange(0x7A, 0x100) | (rng.randrange(8) << 8)
    return rng.randrange(1, 0x7A) | (rng.randrange(8) << 8)


def rand_ring(nonempty=True, good_last=False):
    buf = bytearray()
    for _ in range(128):
        buf += struct.pack("<H", rand_event())
    rd = rng.randrange(256) & (0xFE if rng.random() < 0.8 else 0xFF)
    n = rng.randrange(1 if nonempty else 0, 40) * 2
    wr = (rd + n) & 0xFF
    if good_last:
        # a key every reader takes, just before the write index
        k = (wr - 2) & 0xFF
        v = rng.choice([0x10, 0x20, 0x0120, 0x0430, 0x0225, 0x4B, 0x55, 0x6E, 0x72, 0x0275, 0x0479, 0x45, 0x3F])
        buf[k:k + 2] = struct.pack("<H", v)
        if k == 0xFF:
            buf[0xFF] = v & 0xFF
    d.write(CS, 0x7C, bytes(buf))
    w16(CS, 0x17C, rd)
    w16(CS, 0x17E, wr)
    if wr == 0xFF or rd == 0xFF:
        pass


def rand_globals(mode=None, safe_key=False):
    mode = mode if mode is not None else rng.choice([0, 0, 0, 1, 2, 2, 3])
    w8(DG, 0x80EC, mode)
    w16(DG, 0x80ED, rng.choice([0, 1]) if mode == 1 else 1)
    if mode == 1:
        w16(DG, 0x80ED, 0)
    key = rng.choice([0x10, 0x20, 0x41, 0x42, 0x43, 0x44, 0x0105]) if safe_key else rand_event()
    w16(DG, 0x80EF, key)
    w16(DG, 0x80F1, rng.randrange(20))
    w16(DG, 0x80F3, rng.randrange(0x10000)); w16(DG, 0x80F5, rng.randrange(0x10000))
    w16(DG, 0x80F7, 1 if rng.random() < 0.2 else 0)
    w16(DG, 0x8488, rng.randrange(20))
    w16(CS, 0x180, rng.randrange(0x10000))
    w16(CS, 0x182, rng.randrange(0x10000))
    w16(CS, 0x184, rng.randrange(0x10000))
    w16(DG, 0x80EA, rng.randrange(0x10000))
    return mode


# ---- install, with every BIOS shift bit pattern of interest
for _ in range(12):
    install(rng.randrange(256), rng.randrange(256), rng.randrange(256))
install(0, 0, 0)
call("CtrlBreakIret")
prepare_files()
print("installed; vector 9 now", d.read(0, 36, 4).hex())

tests = {}


def t_handler():
    sc = rng.random()
    if sc < 0.1:
        s = 0xE0
    elif sc < 0.15:
        s = 0xE1
    elif sc < 0.5:
        s = rng.randrange(0x59)
    elif sc < 0.6:
        s = rng.choice([0x1F, 0x32, 0x1E, 0x2C, 0x39, 0x3A, 0x3C, 0x48, 0x4B, 0x4D, 0x50, 0x1C, 0x35, 0x01])
    else:
        s = rng.randrange(256)
    rand_ring(nonempty=False)
    rand_globals(mode=0)
    w8(CS, 0x210, rng.choice([0, 0, 0, 0, 1, 2, 3]))
    w8(CS, 0x20F, 1 if rng.random() < 0.3 else 0)
    w8(CS, 0x9F2, 1 if rng.random() < 0.1 else 0)
    w16(CS, 0x9F3, rng.randrange(0x10000))
    d.write(CS, 0x1FF, bytes(rng.randrange(256) for _ in range(5)))
    w8(CS, 0x204, rng.randrange(256)); w8(CS, 0x206, rng.randrange(256)); w8(CS, 0x207, rng.randrange(256))
    w8(0x40, 0x17, rng.randrange(256)); w8(0x40, 0x96, rng.randrange(256))
    w16(DG, 0x80E6, rng.randrange(0x10000)); w16(DG, 0x80E8, rng.randrange(0x10000))
    if rng.random() < 0.1:                       # Alt+S, Alt+M: the sound and music toggles
        s = rng.choice([0x1F, 0x32])
        w8(CS, 0x204, 0); w8(CS, 0x206, 0x40); w8(CS, 0x207, 0); w8(0x40, 0x17, 0)
        w8(CS, 0x210, 0); w8(CS, 0x20F, 0); w8(CS, 0x9F2, 0)
        d.write(CS, 0x1FF, bytes(5))
    # a key code of 90h or more (scancodes 59h..7Fh read past kScanToKey) has its held bit in the
    # saved INT 9 vector (1AAD:0211): the original then chains to garbage; keep those out
    while not (s & 0x7F) in (0x60, 0x61) and d.read(CS, 0x210, 1)[0] == 0 and d.read(CS, 0x20F, 1)[0] == 0 \
            and s not in (0xE0, 0xE1) and d.read(CS, 0x186 + (s & 0x7F), 1)[0] >= 0x90:
        s = rng.randrange(0x59)
    d.m.set("cx", rng.randrange(256))           # CH = 0: see the QUIRK at 1AAD:08FA
    d.m.kbd_data = s
    d.m.irq1_in_service = True
    w8(0x40, 0x96, d.read(0x40, 0x96, 1)[0] | 2)
    wr0 = r16(CS, 0x17E); snd = r16(DG, 0x80E6) ^ r16(DG, 0x80E8)
    from unicorn import UC_HOOK_CODE
    trace = []
    m = d.m
    h = m.uc.hook_add(UC_HOOK_CODE, lambda uc, a, z, u: trace.append((m.get("cs"), m.get("ip"), m.get("sp"))))
    r = call("KeyboardIsr")
    m.uc.hook_del(h)
    if r["status"].startswith("not") and not cov["dumped"]:
        cov["dumped"] += 1
        print("scancode %02X" % s, ["%04X:%04X sp=%04X" % t for t in trace if t[0] != 0xF000 or t[1] < 0x2100][-40:])
    cov["isr queued"] += r16(CS, 0x17E) != wr0
    cov["isr acked (40:96 bit 1 cleared)"] += not (d.read(0x40, 0x96, 1)[0] & 2)
    cov["isr scancode " + ("E0" if s == 0xE0 else "E1" if s == 0xE1 else "break" if s & 0x80 else "make")] += 1
    cov["isr toggled"] += (r16(DG, 0x80E6) ^ r16(DG, 0x80E8)) != snd
    return r


def t_reader(name):
    def f():
        mode = rand_globals(safe_key=name in ("WaitGameKey", "WaitGameKey2", "WaitKeyEvent"))
        blocking = name.startswith("Wait")
        rand_ring(nonempty=blocking, good_last=blocking)
        if name == "WaitGameKey2":
            w8(CS, 0x80EC, rng.choice([0, 0, 2]))
            if d.read(CS, 0x80EC, 1)[0] == 2 and mode != 2:
                w16(DG, 0x80EF, 0x10)   # the DS = CS state byte makes it take a key from 80EF's slot
        rf = d.m.dos.handles[HANDLES[1]]
        wf = d.m.dos.handles[HANDLES[0]]
        p0, n0 = rf.pos, len(wf.data)
        r = call(name)
        cov[name + " mode %d" % mode] += 1
        cov[name + " replay read"] += d.m.dos.handles[HANDLES[1]].pos != p0
        cov[name + " record write"] += len(d.m.dos.handles[HANDLES[0]].data) != n0
        return r
    return f


def t_getkeystate():
    d.write(CS, 0x1FF, bytes(rng.randrange(256) for _ in range(5)))
    w8(CS, 0x66E, rng.randrange(256))
    w16(DG, 0x80EA, rng.randrange(0x10000))
    k = rng.choice([rng.randrange(0x10000), rng.choice([0x3C, 0x3A, 0x63, 0x68, 0x5C, 0x2B, 0x37]) | (rng.randrange(4) << 8),
                    rng.randrange(0x28)])
    return call("GetKeyState", k)


def t_flush():
    rand_ring(nonempty=False)
    return call("FlushKeyBuffer")


def rand_indices():
    rd = rng.randrange(256) & (0xFE if rng.random() < 0.8 else 0xFF)
    gap = rng.choice([0, 2, 4, 6, 8, rng.randrange(256)])      # bytes free before the read index, roughly
    wr = (rd - gap) & 0xFF
    w16(CS, 0x17C, rd); w16(CS, 0x17E, wr)


def t_mouse():
    d.write(CS, 0x7C, bytes(rng.randrange(256) for _ in range(256)))
    rand_indices()
    w8(CS, 0xC16, rng.randrange(256) if rng.random() < 0.2 else rng.randrange(4))
    w8(CS, 0x222, rng.randrange(256))
    w16(CS, 0x182, rng.randrange(0x10000))
    d.write(CS, 0x1FF, bytes(rng.randrange(256) for _ in range(5)))
    w16(DG, 0x813C, rng.randrange(0x10000)); w16(DG, 0x813E, rng.randrange(0x10000))
    b = rng.randrange(4) if rng.random() < 0.8 else rng.randrange(0x10000)
    r = call("HandleMouseButtons", b)
    return r


def t_queue():
    d.write(CS, 0x7C, bytes(rng.randrange(256) for _ in range(256)))
    rand_indices()
    w16(DG, 0x8138, rng.randrange(0x10000)); w16(DG, 0x813A, rng.randrange(0x10000))
    lo = rng.choice([0, 0x41, 0x42, 0x43, 0x44, rng.randrange(256), rng.randrange(0x41)])
    return call("QueueInputEvent", (rng.randrange(0x100) << 8) | lo)


tests = {"KeyboardIsr": t_handler, "ReadKey": t_reader("ReadKey"), "ReadKeySpecial": t_reader("ReadKeySpecial"),
         "IsKeyPending": t_reader("IsKeyPending"), "WaitGameKey": t_reader("WaitGameKey"),
         "WaitGameKey2": t_reader("WaitGameKey2"), "WaitKeyEvent": t_reader("WaitKeyEvent"),
         "GetKeyState": t_getkeystate, "FlushKeyBuffer": t_flush}
if "HandleMouseButtons" in d.index:
    tests["HandleMouseButtons"] = t_mouse
    tests["QueueInputEvent"] = t_queue
for name, fn in tests.items():
    if ONLY and name not in ONLY:
        continue
    for i in range(N if name not in ("FlushKeyBuffer",) else 20):
        prepare_files()
        fn()

call("RemoveKeyboard")
for k, v in results.items():
    print(k, dict(v))
print({k: v for k, v in cov.items() if v})
for b in bad[:10]:
    print(b)
sys.exit(1 if bad else 0)

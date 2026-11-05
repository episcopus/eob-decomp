"""The library functions on the machine (port/runtime/tcdos.c) against the original's (playbook 80,
"The library on the machine"): each called by hybrid.Direct at the library routine's address, the
original then the C on the same state, comparing memory (the library's data in DGROUP among it: the
file table, _fmode, errno, _doserrno, rand's state, the time zone), the registers returned and the
devices and files.

  - rand and srand over a sequence, reseeded; getdisk and setdisk, valid drives and not;
  - time, with and without a pointer, at two times of day;
  - int86 for every interrupt number but 25h and 26h (and 6, which Unicorn takes for its invalid-opcode
    exception), to a handler that returns with the carry clear
    and to one that sets it, with each DOS error code 0 to 5Bh and others, negative ones among them
    (the library's errno table and its limits), and through the machine's own INT 10h and 21h;
  - harderr;
  - the files: open in each access mode, with and without O_CREAT, O_TRUNC, O_EXCL, O_APPEND, _fmode's
    default, on files that are there and not; read, write, lseek (each whence, a bad one), filelength,
    close (twice, handles past the 20) and remove; the end-of-file flag and the append path.

Exit status 1 if a call differs or does not run.

  library.py [LIB]
"""
import random
import struct
import sys
from collections import Counter
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools" / "port"))
import hybrid
from unicorn.x86_const import UC_X86_REG_EFLAGS

LIB = Path(sys.argv[1]) if len(sys.argv) > 1 else hybrid.LIB
DG = hybrid.DGROUP
rng = random.Random(11)
bad, seen = [], Counter()
d = hybrid.Direct(lib=LIB)


def call(name, *args):
    # every call from a caller with interrupts on, as the game's are (a stub below returns with IF clear,
    # and the library keeps the caller's IF in the flags int86 reports)
    d.m.uc.reg_write(UC_X86_REG_EFLAGS, d.m.get("flags") | 0x0200)
    r = d.call(name, *args)
    seen[(name, r["status"][:40])] += 1
    if r["status"] != "same":
        bad.append(dict(r, args=[hex(x) if isinstance(x, int) else (hex(x[0]), x[1]) for x in args]))
    return r


def far(off):
    return ((DG << 16) | off, 4)


# ---- the random generator and the disk
for k in range(300):
    if k % 37 == 0:
        call("tcl_srand", rng.randrange(0x10000))
    call("tcl_rand")
for drive in (0, 1, 2, 3, 25, 26, 0x7F, 0xFF, 2):
    call("tcl_getdisk")
    call("tcl_setdisk", drive)

# ---- time
call("tcl_time", (0, 4))
call("tcl_time", far(0x9000))
d.m.insns += 3_000_000_000                      # later in the day
call("tcl_time", far(0x9004))

# ---- int86
STC = (0x8000, 0x0000)                           # stc; retf 2: the carry set
CLC = (0x8000, 0x0010)                           # clc; retf 2: the carry clear
d.write(0x8000, 0x0000, bytes([0xF9, 0xCA, 0x02, 0x00]))
d.write(0x8000, 0x0010, bytes([0xF8, 0xCA, 0x02, 0x00]))
IN, OUT = 0x9100, 0x9120


def int86(n, ax, handler=None):
    old = d.read(0, n * 4, 4)
    if handler is not None:
        d.write(0, n * 4, struct.pack("<HH", handler[1], handler[0]))
    d.write(DG, IN, struct.pack("<8H", ax, rng.randrange(0x10000), rng.randrange(0x10000), rng.randrange(0x10000),
                                rng.randrange(0x10000), rng.randrange(0x10000), 0, 0))
    r = call("tcl_int86", n, far(IN), far(OUT))
    if r["status"] != "same":
        bad[-1]["int"] = (hex(n), hex(ax), handler)
    if handler is not None:
        d.write(0, n * 4, old)


for n in range(256):
    if n in (0x25, 0x26, 0x06):     # absread, abswrite; INT 6 is Unicorn's invalid-opcode exception
        continue
    int86(n, rng.randrange(0x10000), STC if n % 2 else CLC)
for ax in list(range(0x5C)) + [0x100, 0x5003, 0x7FFF, 0x8000, 0x8001, 0xFF00, 0xFFDB, 0xFFDC, 0xFFDD, 0xFFDE,
                                 0xFFFE, 0xFFFF]:
    int86(0x60, ax, STC)
for ax in (0x0F00, 0x1A00, 0x1000, 0x0013):
    int86(0x10, ax)
for ax in (0x1900, 0x3000, 0x2A00, 0x2C00, 0x3D00):
    int86(0x21, ax)

# ---- harderr
call("tcl_harderr", ((0x2A63 << 16) | 0x0004, 4))
call("tcl_harderr", ((0x1234 << 16) | 0x5678, 4))

# ---- the files
PATH, BUF = 0x9200, 0x9300


def path(p):
    d.write(DG, PATH, p.encode() + b"\0")
    return far(PATH)


def opened(p, access, *mode):
    r = call("tcl_open", path(p), access, *mode)
    return r.get("value", 0xFFFF) & 0xFFFF


def flags(fd, v):
    d.write(DG, 0x8758 + 2 * fd, struct.pack("<H", v))


d.write(DG, BUF, bytes(rng.randrange(256) for _ in range(64)))
fd = opened("EOBDATA1.PAK", 0x8001)
call("tcl_filelength", fd)
for off, whence in ((10, 0), (0xFFFFFFFC, 1), (0, 2), (0xFFFFFFFD, 2), (0, 0), (0xFFFFFFFF, 0), (0, 7), (1234, 1)):
    call("tcl_lseek", fd, (off, 4), whence)
    call("tcl_read", fd, far(BUF), rng.choice((0, 1, 16, 33)))
call("tcl_lseek", fd, (0, 4), 2)
call("tcl_read", fd, far(BUF), 16)                # at the end
call("tcl_write", fd, far(BUF), 4)                # read-only
for bit in range(16):                             # the end-of-file bit, and lseek clearing it
    flags(fd, 0x8001 | (1 << bit))
    call("tcl_read", fd, far(BUF), 4)
    flags(fd, 0x8001 | (1 << bit))
    call("tcl_lseek", fd, (0, 4), rng.choice((0, 9)))
flags(fd, 0x8A01)
call("tcl_write", fd, far(BUF), 0)
call("tcl_write", fd, far(BUF), 2)                # append on a read-only handle
flags(fd, 0x8001)
call("tcl_close", fd)
call("tcl_close", fd)
call("tcl_read", fd, far(BUF), 4)
call("tcl_filelength", fd)
for h in (19, 20, 21, 0x7FFF, 0xFFFF):
    call("tcl_close", h)
for h in (19, 0x7FFF, 0xFFFF):
    call("tcl_lseek", h, (0, 4), 0)
    call("tcl_filelength", h)
for access, mode in ((0x8001, ()), (0x8002, ()), (0x8004, ()), (0x8201, ()), (0x8202, ()), (0x8402, ()),
                     (0x8802, ()), (0x0001, ()), (0x8101, (0x180,)), (0x8302, (0x180,)), (0x8502, (0x180,)),
                     (0x8702, (0x180,)), (0x8102, (0x100,))):
    for name in ("EOBDATA1.PAK", "NEW.DAT"):
        h = opened(name, access, *mode)
        if h != 0xFFFF:
            if access & 6:
                call("tcl_write", h, far(BUF), rng.randrange(1, 40))
            call("tcl_filelength", h)
            call("tcl_close", h)
        if name == "NEW.DAT":
            call("tcl_remove", path(name))
call("tcl_remove", path("MISSING.DAT"))
opened("MISSING.DAT", 0x8001)
h = opened("NEW.DAT", 0x8102, 0x180)
for k in range(10):
    call("tcl_write", h, far(BUF), rng.randrange(0, 64))
    call("tcl_lseek", h, (rng.randrange(0, 200), 4), 0)
call("tcl_filelength", h)
call("tcl_close", h)
h = opened("NEW.DAT", 0x8804)
call("tcl_write", h, far(BUF), 7)
call("tcl_lseek", h, (0, 4), 0)
call("tcl_read", h, far(BUF + 0x40), 64)
call("tcl_close", h)
call("tcl_remove", path("NEW.DAT"))

for k, n in sorted(seen.items()):
    print(k, n)
for b in bad[:10]:
    print(b)
sys.exit(1 if bad else 0)

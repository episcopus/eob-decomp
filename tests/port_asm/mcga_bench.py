"""mcga_bench.py: the bench of the driver's direct tests (tests/port_asm/mcga*.py, playbook 80, M4).

The program runs on the bench until the original has loaded and linked MCGA.OVL, then on until it has set
its pages and drawn its title (30 million instructions), and the driver's C is registered at the
driver's segment. Each call is hybrid.Direct's: the original routine, then the C from the same state,
compared (memory, the screen, the registers the routine answers in); the bench goes on from the
original's state.

  from mcga_bench import Driver
  t = Driver(lib)                       # lib: the hybrid library, or None for make port-hybrid's
  t.call("DrawShape", page, (far, 4), x, y)
  sys.exit(t.report())
"""
import random
import struct
import sys
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools" / "port"))
sys.path.insert(0, str(ROOT / "tools"))
import hybrid  # noqa: E402

DG = hybrid.DGROUP
PAGE_SEGS = 0xFC0C          # ds: the game's eight page segments (gPageSegs)
DRAW_PAGE = 0x8115          # ds: the page the driver draws to (gDrawPage)
CUR_FONT = 0xFE4E           # ds: the far pointer to the current font (gCurFont)
SCRATCH = 0x9000            # the tests' shapes, buffers and tables: 9000:0000 to 9000:7FFF (the calls'
                            # stack is hybrid.Direct's, 9000:FFF0 down)


class Driver:
    def __init__(self, lib=None, insns=30_000_000, seed=1):
        self.d = hybrid.Direct(lib=Path(lib) if lib else hybrid.LIB)
        self.seg = self.d.link_driver()
        why = self.d.m.run(insns)
        assert why == "max-insns", why
        self.rng = random.Random(seed)
        self.bad = []
        self.seen = Counter()

    # -- memory
    def read(self, seg, off, n):
        return self.d.read(seg, off, n)

    def write(self, seg, off, data):
        self.d.write(seg, off, bytes(data))

    def word(self, seg, off):
        return struct.unpack("<H", self.d.read(seg, off, 2))[0]

    def set_word(self, seg, off, v):
        self.d.write(seg, off, struct.pack("<H", v & 0xFFFF))

    def page_seg(self, page):
        return self.word(DG, PAGE_SEGS + 2 * (page & 0x1F))

    def set_draw_page(self, page):
        self.set_word(DG, DRAW_PAGE, page)

    def fill_page(self, page, data=None):
        """Fills a page (64000 bytes) with data, or random bytes."""
        if data is None:
            data = bytes(self.rng.randrange(256) for _ in range(64000))
        self.write(self.page_seg(page), 0, data)

    def far(self, seg, off=0):
        """a far pointer argument"""
        return ((seg << 16) | off, 4)

    # -- calls
    def call(self, name, *args, **kw):
        """Calls MCGA.<name>, the original then the C; counts the result, keeps the differences."""
        r = self.d.call("MCGA." + name, *args, **kw)
        self.seen[(name, r["status"][:40])] += 1
        if r["status"] != "same":
            self.bad.append(dict(r, args=[hex(a) if isinstance(a, int) else (hex(a[0]), a[1]) for a in args]))
        return r

    def report(self):
        for k, n in sorted(self.seen.items()):
            print(k, n)
        for b in self.bad[:10]:
            print(b)
        return 1 if self.bad else 0

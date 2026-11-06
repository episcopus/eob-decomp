"""The far heap (port/runtime/tcheap.c) against the original's (playbook 80, "The library on the
machine"): the program runs its start-up on the bench to main, so the heap is the one the game starts
with; then farmalloc, farfree and realloc in a long random sequence (sizes from 0 to past the heap's
top, blocks freed in random order so that the free ring, the rover, the merges on both sides and the
release of the last block all come up, realloc shrinking, keeping and growing), with farcoreleft and a
whole farheapwalk now and then, each call by hybrid.Direct, the original then the C on the same state,
comparing memory (the blocks' headers, the heap's words in the library's code segment, the break level
and DOS's memory block), the registers returned and the devices.

Exit status 1 if a call differs or does not run.

  heap.py [LIB]
"""
import random
import sys
from collections import Counter
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools" / "port"))
sys.path.insert(0, str(ROOT / "tools"))
import hybrid
from oracle.machine import Stop

LIB = Path(sys.argv[1]) if len(sys.argv) > 1 else hybrid.LIB
DG = hybrid.DGROUP
rng = random.Random(5)
bad, seen = [], Counter()
d = hybrid.Direct(lib=LIB)

# the start-up, to main (07C8:0001)
started = []


def at_main(mach):
    if not started:
        started.append(True)
        raise Stop("main")


d.m.add_breakpoint(hybrid.LOAD_SEG + 0x07C8, 0x0001, at_main)
assert d.m.run(5_000_000) == "main"


def call(name, *args):
    r = d.call(name, *args)
    seen[(name, r["status"][:40])] += 1
    if r["status"] != "same":
        bad.append(dict(r, args=[hex(x) if isinstance(x, int) else (hex(x[0]), x[1]) for x in args]))
    return r


def size():
    k = rng.random()
    if k < 0.05:
        return 0
    if k < 0.6:
        return rng.randrange(1, 0x80)
    if k < 0.9:
        return rng.randrange(0x80, 0x2000)
    if k < 0.98:
        return rng.randrange(0x2000, 0x20000)
    return rng.randrange(0x20000, 0x70000)          # past the heap's top, often


HI = 0x9400


def walk():
    d.write(DG, HI, bytes(10))
    for _ in range(400):
        r = call("tcl_farheapwalk", ((DG << 16) | HI, 4))
        if (r.get("value", 0) & 0xFFFF) != 2:
            break


live = []
walk()
call("tcl_farcoreleft")
for k in range(1500):
    op = rng.random()
    if op < 0.45 or not live:
        r = call("tcl_farmalloc", (size(), 4))
        p = r.get("value", 0)
        if p:
            live.append(p)
            d.write(p >> 16, 4, bytes(rng.randrange(256) for _ in range(4)))
    elif op < 0.8:
        p = live.pop(rng.randrange(len(live)))
        call("tcl_farfree", (p, 4))
    else:
        i = rng.randrange(len(live) + 1)
        p = live[i] if i < len(live) else 0
        s = rng.choice((rng.randrange(1, 0x100), rng.randrange(0x100, 0x4000), rng.randrange(1, 0xFFFF)))
        r = call("tcl_realloc", (p, 4), s)
        q = r.get("value", 0)
        if i < len(live):
            live.pop(i)
        if q:
            live.append(q)
    if k % 50 == 0:
        call("tcl_farcoreleft")
    if k % 150 == 0:
        walk()
call("tcl_farfree", (0, 4))
for p in live:
    call("tcl_farfree", (p, 4))
walk()
call("tcl_farcoreleft")

for k, n in sorted(seen.items()):
    print(k, n)
for b in bad[:10]:
    print(b)
sys.exit(1 if bad else 0)

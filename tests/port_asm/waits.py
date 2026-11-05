"""The machine's wait (port/runtime/irq.c) against the original, by Direct calls with interrupts
(playbook 80, "The interrupts and the C clock"): the original runs as in a session, the oracle
delivering the timer's interrupts and IRQ 1 for the keys given; the C is given what the original was
delivered, at the same accesses, and the oracle's clock where it changed. The blocking key readers of
E1AAD.c (with keys waiting, special and released keys among them, so a reader takes several turns and
waits between them), and StartSample of E_TEXT.c, which spins on the timer chip's count through
several timer interrupts. Then two inputs altered, each of which must be noticed: the clock marks
dropped (StartSample spins on), an interrupt dropped (a reader waits for a key that does not come).
Exit status 1 if a call differs or an alteration goes unnoticed.

  waits.py [LIB]
"""
import random
import sys
from collections import Counter
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools" / "port"))
import hybrid

LIB = Path(sys.argv[1]) if len(sys.argv) > 1 else hybrid.LIB
DG = hybrid.DGROUP
rng = random.Random(7)
bad, seen = [], Counter()

kb = hybrid.Direct(lib=LIB)
for name in ("InstallKeyboard", "InstallTickHandler"):
    assert kb.call(name)["status"] == "same"
KEYS = [0x1E, 0x30, 0x2E, 0x39, 0x1C, 0x48, 0x50, 0x01, 0x3B, 0x2A, 0x1D, 0x38]
BUDGET = 3_000_000
for k in range(25):
    for name in ("WaitGameKey", "WaitGameKey2", "WaitKeyEvent"):
        kb.call("FlushKeyBuffer")
        keys = []
        for _ in range(rng.randrange(0, 4)):
            sc = rng.choice(KEYS)
            keys += [sc, sc | 0x80] if rng.random() < 0.7 else [sc | 0x80]
        keys += [0x1E, 0x9E]                                  # an A at the end: every reader takes it
        r = kb.call(name, interrupts=True, keys=keys, budget=BUDGET)
        seen[(name, r["status"][:30], r.get("interrupts", 0) > 1)] += 1
        if r["status"] not in ("same",) and not r["status"].startswith("not tested"):
            bad.append(r)

snd = hybrid.Direct(lib=LIB, sound="adlib")
for k in range(8):
    r = snd.call("StartSample", ((DG << 16) | rng.randrange(0x10000), 4), rng.randrange(1, 4000), interrupts=True, budget=BUDGET)
    seen[("StartSample", r["status"][:30], r.get("interrupts", 0))] += 1
    if r["status"] != "same":
        bad.append(r)
    snd.m.insns += rng.randrange(100000)

# alterations the comparison must notice
r = snd.call("StartSample", ((DG << 16) | 0x1234, 4), 100, interrupts=True, budget=BUDGET,
             alter=lambda e, t: (e, []))
if r["status"] == "same":
    bad.append({"unnoticed": "StartSample without the clock marks", **r})
kb.call("FlushKeyBuffer")
r = kb.call("WaitGameKey", interrupts=True, keys=[0x9E, 0x1E, 0x9E], budget=BUDGET,
            alter=lambda e, t: (e[:-2], t))
if r["status"] == "same":
    bad.append({"unnoticed": "WaitGameKey without its key", **r})

for k, n in sorted(seen.items()):
    print(k, n)
for b in bad[:10]:
    print(b)
sys.exit(1 if bad else 0)

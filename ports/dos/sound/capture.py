#!/usr/bin/env python3
"""capture.py: what the original sound drivers send to the chips, for the Tandy FM study (README.md).

  capture.py sfx   --card adlib|tandy     every game sound EOB.EXE maps, one after another, from the title screen
  capture.py intro --card adlib|tandy     INTRO.EXE from start to hand-over
  capture.py introparts                   each of INTRO.EXE's 14 sounds alone on the AdLib (sndat.py's intro)

The program is the unpatched rebuild (ports/dos/build/ref, `make -C ports/dos ref`) on the 8088 model of
tools/tandybox.py, so time is the EX's own clock. A sound is started by calling PlaySound (177D:0070) the next time
the game polls the keyboard, with the stack it polls with: the call returns to a two-instruction stub that drops the
argument and jumps back into the poll. Writes `build/sound/<what>-<card>.json`: per window the game number, the
times, the chip's registers at the start (AdLib) and the writes in it.
"""
from __future__ import annotations

import argparse
import json
import struct
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
PORT = HERE.parent
ROOT = PORT.parent.parent
sys.path.insert(0, str(PORT / "tools"))
import tandybox as T  # noqa: E402
from oracle.adlib import AdLib  # noqa: E402
from oracle.machine import LOAD_SEG  # noqa: E402

OUT = PORT / "build/sound"
STUB = (0xF000, 0x3F10)
CARD_DIGIT = {"adlib": 1, "tandy": 3}


class QuietAdLib(AdLib):
    """The card without the oracle's charge for a status read: on the cycle clock the reads cost their instructions."""

    def read(self, port, size):
        if port == self.base:
            self._timers()
            return self.flags | 0x06
        return 0xFF


def sound_map(name: str) -> bytes:
    sys.path.insert(0, str(ROOT / "tools/re"))
    import pak
    data = pak.read_pak((ROOT / "eob/EOBDATA1.PAK").read_bytes())
    return next(e["data"] for e in data["entries"] if e["name"] == name)[:120]


class Driver:
    """Starts the sounds of a schedule at keyboard polls and cuts the chip log into windows."""

    def __init__(self, m: T.TandyMachine, card: str, numbers: list[int], start: float, max_len: dict, min_len: dict):
        self.m, self.card = m, card
        self.todo = list(numbers)
        self.max_len, self.min_len = max_len, min_len
        self.next_at = int(start * T.CLOCK)
        self.pending = None            # the game number to start at the next poll
        self.cur = None                # the window being recorded
        self.windows = []
        self.done = False
        b = m.build
        for name in T.POLLS:
            if name in b.pubs:
                seg, off = b.addr(name)
                lin = m.linear(seg, off)
                m.uc.hook_add(T.UC_HOOK_CODE, lambda uc, a, s, ud, seg=seg, off=off: self.on_poll(seg, off), begin=lin, end=lin)

    def log(self):
        return self.m.adlib_log if self.card == "adlib" else self.m.tandy_log

    def on_poll(self, seg, off):
        n = self.pending
        if n is None:
            return
        self.pending = None
        m = self.m
        # add sp,2 ; jmp far seg:off
        m.uc.mem_write(m.linear(*STUB), bytes([0x83, 0xC4, 0x02, 0xEA]) + struct.pack("<HH", off, seg))
        m.push(n)
        m.push(STUB[0])
        m.push(STUB[1])
        seg, off = m.build.addr("_PlaySound")       # 177D:0070 in the reference; tandy-sound.patch moves it
        m.set("cs", seg & 0xFFFF)
        m.set("ip", off)
        m.uc.emu_stop()
        if n != 0:
            regs = list(m.adlib.regs) if self.card == "adlib" else None
            self.cur = {"game": n, "t0": m.insns / T.CLOCK, "first": len(self.log()), "regs": regs}

    def service(self, m):
        now = m.insns
        if self.pending is not None or now < self.next_at:
            return
        if self.cur is not None:
            log = self.log()
            new = log[self.cur["first"]:]
            age = now / T.CLOCK - self.cur["t0"]
            last = (new[-1][0] / T.CLOCK) if new else self.cur["t0"]
            limit = self.max_len.get(self.cur["game"], 8.0)
            least = self.min_len.get(self.cur["game"], 0.4)
            if (age < least or now / T.CLOCK - last < 0.6) and age < limit:
                self.next_at = now + T.CLOCK // 20
                return
            self.cur["t1"] = now / T.CLOCK
            self.cur["writes"] = [(t / T.CLOCK, *rest) for t, *rest in new]
            del self.cur["first"]
            self.windows.append(self.cur)
            print("  game %3d: %5.2fs, %4d writes" % (self.cur["game"], self.cur["t1"] - self.cur["t0"], len(new)), flush=True)
            self.cur = None
            self.pending = 0                        # the silence script between two sounds
            self.next_at = now + int(0.4 * T.CLOCK)
            return
        if not self.todo:
            self.done = True
            m.request_stop()
            return
        self.pending = self.todo.pop(0)
        self.next_at = now + T.CLOCK // 20

    def progress(self, *a, **k):
        return False


def machine(card: str, program: str, build: str = "ref") -> T.TandyMachine:
    b = T.get_build(build, program)
    digit = CARD_DIGIT[card]
    args = ("1 %d 0 homie 3 40960" if program == "EOB.EXE" else "1 %d 0 homie") % digit
    m = T.TandyMachine(b, args=args)
    if card == "adlib":
        m.adlib = QuietAdLib(m)
    return m


def sfx(card: str, build: str = "ref") -> dict:
    m = machine(card, "EOB.EXE", build)
    amap = sound_map("ADLIB.DAT")
    numbers = [n for n in range(120) if 0 < amap[n] < 0x96]
    d = Driver(m, card, numbers, start=20.0, max_len={20: 130.0}, min_len={20: 110.0})
    m.keys = d
    reason = m.run_clocked(int(1500 * T.CLOCK))
    print("stopped:", reason, "windows", len(d.windows))
    tmap = sound_map("PCGAMESN.DAT")
    return {"card": card, "map": list(amap if card == "adlib" else tmap), "windows": d.windows}


def intro(card: str) -> dict:
    m = machine(card, "INTRO.EXE")
    regs = list(m.adlib.regs) if card == "adlib" else None

    class Idle:
        def service(self, mach):
            pass

        def progress(self, *a, **k):
            return False
    m.keys = Idle()
    reason = m.run_clocked(int(170 * T.CLOCK))
    log = m.adlib_log if card == "adlib" else m.tandy_log
    print("stopped:", reason, "at %.1fs" % (m.insns / T.CLOCK), "writes", len(log))
    w = {"game": "intro", "t0": 0.0, "t1": m.insns / T.CLOCK, "regs": regs, "writes": [(t / T.CLOCK, *r) for t, *r in log]}
    return {"card": card, "windows": [w]}


# INTRO.EXE's sounds (its SOUND.DAT map): game sound -> the AdLib driver's. 1 to 3 are music (the Westwood card, the title
# and the scenes after it, the last scene), 4 to 14 effects that the scenes start over it
INTRO_SOUNDS = {1: 0x01, 2: 0x0B, 3: 0x15, 4: 0x1E, 5: 0x1F, 6: 0x20, 7: 0x21, 8: 0x22, 9: 0x23, 10: 0x25, 11: 0x26,
                12: 0x27, 13: 0x28, 14: 0x29}
INTRO_END = 2.0             # s of silence after which a sound has ended; what follows is the intro's own closing writes


def intro_part(game: int) -> dict:
    """INTRO.EXE on the AdLib with every request of a sound (QueueSound, 0000:43E7) skipped but the first of this one:
    its writes alone, from its request to its last write before a silence of INTRO_END (the intro runs for about 250 s,
    then waits for a key)."""
    want = INTRO_SOUNDS[game]
    m = machine("adlib", "INTRO.EXE")
    seg, off = m.build.addr("_QueueSound")
    st = {}

    def on_queue(uc, a, size, ud):
        ss, sp = m.get("ss"), m.get("sp")
        ip, cs, arg = struct.unpack("<HHH", bytes(uc.mem_read(m.linear(ss, sp), 6)))
        if arg == want and not st:
            st.update(t0=m.insns, regs=list(m.adlib.regs), first=len(m.adlib_log))
            return
        m.set("sp", sp + 4)                     # retf: the request is not made
        m.set("cs", cs)
        m.set("ip", ip)
        uc.emu_stop()
    m.uc.hook_add(T.UC_HOOK_CODE, on_queue, begin=m.linear(seg, off), end=m.linear(seg, off))

    class Idle:
        def service(self, mach):
            pass

        def progress(self, *a, **k):
            return False
    m.keys = Idle()
    m.run_clocked(int(260 * T.CLOCK))
    if not st:
        sys.exit("capture: INTRO.EXE did not ask for its sound %d" % game)
    log = m.adlib_log[st["first"]:]
    ts = [t for t, *_r in log]
    gaps = [i + 1 for i in range(len(ts) - 1) if ts[i + 1] - ts[i] > INTRO_END * T.CLOCK]
    log = log[:gaps[-1]] if gaps else log      # up to the last silence: after it come the intro's closing writes
    t0 = st["t0"] / T.CLOCK
    print("  intro sound %2d (driver %02X): at %.1f s, %.2f s, %d writes" % (
        game, want, t0, (log[-1][0] / T.CLOCK - t0) if log else 0, len(log)), flush=True)
    return {"game": game, "driver": want, "t0": t0, "t1": (log[-1][0] / T.CLOCK if log else t0) + INTRO_END,
            "regs": st["regs"], "writes": [(t / T.CLOCK, *r) for t, *r in log]}


def intro_parts() -> dict:
    from concurrent.futures import ProcessPoolExecutor
    with ProcessPoolExecutor(max_workers=7) as ex:
        windows = list(ex.map(intro_part, sorted(INTRO_SOUNDS)))
    return {"card": "adlib", "windows": windows}


def main(argv=None):
    ap = argparse.ArgumentParser()
    ap.add_argument("what", choices=["sfx", "intro", "introparts"])
    ap.add_argument("--card", choices=list(CARD_DIGIT), default="adlib")
    ap.add_argument("--build", default="ref", help="ports/dos/build/<name>: ref (the original sound) or game (sndat.py check)")
    a = ap.parse_args(argv)
    res = sfx(a.card, a.build) if a.what == "sfx" else intro_parts() if a.what == "introparts" else intro(a.card)
    OUT.mkdir(parents=True, exist_ok=True)
    path = OUT / ("%s-%s%s.json" % (a.what, a.card, "" if a.build == "ref" else "-" + a.build))
    path.write_text(json.dumps(res))
    print("wrote", path.relative_to(ROOT))


if __name__ == "__main__":
    main()

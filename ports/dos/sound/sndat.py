#!/usr/bin/env python3
"""sndat.py: SN.DAT and SNSOUND.DAT, the Tandy sounds of EOB.EXE and INTRO.EXE (README.md, "The game's sound").

  sndat.py build        build/sound/SN.DAT from sound/picks.json and the clips' chip logs (make sound first), and
                        build/sound/SNSOUND.DAT from the intro's sounds mapped one by one (tandymap.py intro)
  sndat.py show         the size of each sound in them
  sndat.py check        the patched game's own writes (build/sound/sfx-tandy-game.json, `capture.py sfx --card tandy
                        --build game`) against what the player should write, sound for sound, tick for tick
  sndat.py check-intro  the patched INTRO.EXE through its scenes: every call of the player against the model

Each sound is a chip log (EOB.EXE's: the version the listening passes picked, designed, now, plain or rich; INTRO.EXE's:
each of its 14 sounds alone, rich) read as one state of the SN76489's eight registers per driver tick (72.8 Hz), written
as the changes from tick to tick in a stream of nibbles that the player SNPLAY.ASM reads. `Player` here is that player,
in Python, call for call: the round trip through it must give the picked states back, which `build` checks for every
sound, and the checks hold the programs' own writes against it.

The files have the layout the loaders already read (LoadSoundDriver, src/E177D.C, and StartupGame, src/I0892.C, which
tandy-sound.patch has open SN.DAT and SNSOUND.DAT on a Tandy): a 120-byte map from the program's sound number to the
driver's, then the driver block, which the loader reads to cs:0416 of the AdLib driver's segment (dead on a Tandy) and
whose first 300 words RebaseDriverBlock (0000:03FA) turns into offsets in that segment. The block is the table of 128
words (word n is where driver sound n starts; bit 15 set: it is music, which plays in the music slot), the player
(SNPLAY.ASM, from cs:0516) and the streams. EOB.EXE's may run to 0000:5028, the end of its AdLib driver (19,474
bytes); INTRO.EXE reads 36B0h bytes (14,000). The block's words from 128 to 299 (the player and the first streams)
are stored less 0416h, so that the rebase gives them back.

The stream, per tick (a byte holds two nibbles, the low one first):
  A, B      A: bits 0 to 2 the tones that change, bit 3 the noise; B: bits 0 to 3 the volumes that change
  A = B = 0 a pause: one nibble W, 1 to 15 ticks with no change, this one included; W = 0 is followed by two
            nibbles X (low first), 1 to 255 ticks, or X = 0: the end of the sound, and one more nibble F: 1 sets the
            AdLib driver's script flag (0000:03A0), which the character generation screens wait for to play their
            music again (WaitKeyOrPause, src/EOV0.C:232)
  per tone  one nibble D: 1 to 7 add D to the period, 9 to 15 add D - 16; D = 8 is followed by a signed byte to add
            (two nibbles, low first); D = 0 by the period itself in three nibbles (low first)
  noise     one nibble, the noise register (bits 0 and 1 the rate, bit 2 white)
  per volume one nibble, the attenuation (0 loudest, 15 off); the player adds the sound's distance attenuation
A tick with changes is one frame. A sound starts from silence; the first frame sets what sounds, and a tone's first
period in a sound is always the period itself. The player writes a tone as two bytes (latch with the low four
bits, then the high six), a volume and the noise as one.

What is dropped on the way, none of it audible: the period of a tone channel while its volume is off (unless tone 2
clocks the noise and the noise sounds), and the noise register while the noise is off; each is written when it
sounds again. Within one tick only the last state counts.
"""
from __future__ import annotations

import argparse
import json
import shutil
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from arrange import PERIODS, VIB, VIB_DELAY  # noqa: E402

HERE = Path(__file__).resolve().parent
PORT = HERE.parent
ROOT = PORT.parent.parent
OUT = PORT / "build/sound"
CLIPS = OUT / "clips"
PICKS = HERE / "picks.json"
TARGET = OUT / "SN.DAT"
PLAYER = HERE / "SNPLAY.ASM"
PLAYER_AT = 0x0516                    # where SNPLAY.ASM is assembled to run (its ORG): the block's byte 256

TICK = 1193182 / 0x555 / 12           # the AdLib driver's tick, at which the clips were made; the Tandy driver's (291 Hz
                                      # divided by four, 72.82 Hz) is 0.03% slower
BLOCK_MAX = 0x5028 - 0x0416           # from cs:0416 to the end of the AdLib driver
REBASED = 300                         # words RebaseDriverBlock rebases (cx = 12Ch, 0000:03FD)
TABLE = 128                           # the table's words: a driver sound is 7 bits (177D:00D2). The block's words from 128
                                      # to 299 are stream bytes; they are stored less 0416h, so that the rebase restores them
BASE = 0x0416
REGS = ("t0", "t1", "t2", "n", "v0", "v1", "v2", "v3")
SILENT = {"t0": 0, "t1": 0, "t2": 0, "n": 0, "v0": 15, "v1": 15, "v2": 15, "v3": 15}
START = {"t0": None, "t1": None, "t2": None, "n": None, "v0": 15, "v1": 15, "v2": 15, "v3": 15}   # the chip after a start
MUSIC = 20                            # character generation: the only sound that sets the script flag
MUSIC_LOOP = 102.097                  # s after its start: when the AdLib sets the flag (the C7h 01 at the end of driver
                                      # sound 67, voice 0's track; measured on the reference, 122.098 s for a start at 20.001)


def chip_log(name: str, pick: str) -> list[tuple[int, int]]:
    """(tick, byte) of a clip's picked version, from its render log."""
    out = []
    for line in (CLIPS / name / ("tandy-%s.txt" % pick)).read_text().splitlines():
        if line.startswith("w "):
            _w, t, b = line.split()
            out.append((round(float(t) * TICK), int(b)))
    return out


def states(log) -> list[tuple[int, dict]]:
    """The eight registers after each tick that writes, as the chip latches them."""
    reg = dict(SILENT)
    latch = None
    out = []
    for t, b in log:
        if b & 0x80:
            c = (b >> 5) & 3
            latch = c
            if b & 0x10:
                reg["v%d" % c] = b & 15
            elif c == 3:
                reg["n"] = b & 7
            else:
                reg["t%d" % c] = (reg["t%d" % c] & ~15) | (b & 15)
        elif latch is not None and latch < 3:
            reg["t%d" % latch] = (reg["t%d" % latch] & 15) | ((b & 63) << 4)
        if out and out[-1][0] == t:
            out[-1] = (t, dict(reg))
        else:
            out.append((t, dict(reg)))
    return out


def audible(r: dict) -> dict:
    """What can be heard of a state: a silent channel's period and a silent noise's register do not matter."""
    a = dict(r)
    for c in range(3):
        clocks_noise = c == 2 and r["v3"] != 15 and r["n"] & 3 == 3
        if r["v%d" % c] == 15 and not clocks_noise:
            a["t%d" % c] = None
    if r["v3"] == 15:
        a["n"] = None
    return a


def frames(st) -> list[tuple[int, dict]]:
    """The register values to write at each tick: what changed since the start's silence and is heard."""
    out = []
    chip = dict(START)
    for t, r in st:
        a = audible(r)
        ch = {k: r[k] for k in REGS if a[k] is not None and r[k] != chip[k]}
        if ch:
            chip.update(ch)
            out.append((t, ch))
    return out


class Nibbles:
    def __init__(self):
        self.n = []

    def put(self, *v):
        for x in v:
            assert 0 <= x < 16
            self.n.append(x)

    def bytes(self) -> bytes:
        n = self.n + [0] * (len(self.n) & 1)
        return bytes(n[i] | n[i + 1] << 4 for i in range(0, len(n), 2))


def encode(fr, end_tick: int, flag: int = 0) -> bytes:
    s = Nibbles()
    period = {}
    t_next = 0

    def pause(n):
        while n > 0:
            w = min(n, 255)
            if w < 16:
                s.put(0, 0, w)
            else:
                s.put(0, 0, 0, w & 15, w >> 4)
            n -= w
    for t, ch in fr:
        pause(t - t_next)
        a = sum(1 << i for i, k in enumerate(("t0", "t1", "t2", "n")) if k in ch)
        b = sum(1 << i for i, k in enumerate(("v0", "v1", "v2", "v3")) if k in ch)
        s.put(a, b)
        for c in range(3):
            k = "t%d" % c
            if k in ch:
                p = ch[k] & 0x3FF
                d = p - period[k] if k in period else 1 << 12
                if -7 <= d <= 7 and d != 0:
                    s.put(d & 15)
                elif -128 <= d <= 127:
                    s.put(8, d & 15, (d >> 4) & 15)
                else:
                    s.put(0, p & 15, (p >> 4) & 15, p >> 8)
                period[k] = p
        if "n" in ch:
            s.put(ch["n"])
        for c in range(4):
            if "v%d" % c in ch:
                s.put(ch["v%d" % c])
        t_next = t + 1
    pause(end_tick - t_next)
    s.put(0, 0, 0, 0, 0, flag)
    return s.bytes()


def header(fr, stop: bool = False) -> int:
    """An effect's first byte: the channels its stream writes (bits 0 to 2 the tones, 3 the noise), 4 when its noise is
    clocked by tone 2 (the player then keeps its tone 2 on the chip's), 5 when it stops every sound (driver sound 0)."""
    h = H_STOP if stop else 0
    for _t, ch in fr:
        for k in ch:
            h |= {"t0": 1, "t1": 2, "t2": 4, "n": 8, "v0": 1, "v1": 2, "v2": 4, "v3": 8}[k]
        if ch.get("n") is not None and ch["n"] & 3 == 3:
            h |= H_CLOCK
    return h


def effect(fr, end_tick: int, flag: int = 0, stop: bool = False) -> bytes:
    return bytes([header(fr, stop)]) + encode(fr, end_tick, flag)


def music(name: str, flag: int) -> tuple[bytes, list, int]:
    """A piece's stream for the music slot (arrange.py's notes): its instruments, then its events. Bytes:
      count, then per instrument: n, n attack volumes, fall, floor, release, vib
      00 F        the end; F 1 sets the script flag
      01 to 7F    wait that many ticks
      8c N I D    channel c's note: N in arrange.PERIODS, I instrument << 4 | gain, D ticks held (one byte under 80h,
                  else two: 80h | high, low)
    Also the arrangement's own render (its reference) and its length."""
    import arrange
    chans = arrange.parts(name)
    end = arrange.length(name, chans)
    used = sorted({n["ins"] for ch in chans for n in ch}, key=list(arrange.INSTRUMENTS).index)
    out = bytearray([len(used)])
    for k in used:
        i = arrange.INSTRUMENTS[k]
        out += bytes([len(i["attack"])] + i["attack"] + [i["fall"], i["floor"], i["release"], i["vib"]])
    events = sorted((n["on"], c, n) for c, ch in enumerate(chans) for n in ch)
    t = 0
    for on, c, n in events:
        while on > t:
            w = min(127, on - t)
            out.append(w)
            t += w
        d = n["dur"]
        out += bytes([0x80 | c, n["note"], used.index(n["ins"]) << 4 | n["gain"]])
        out += bytes([d]) if d < 0x80 else bytes([0x80 | d >> 8, d & 0xFF])
    while end > t:
        w = min(127, end - t)
        out.append(w)
        t += w
    out += bytes([0, flag])
    st, _end = arrange.render(name)
    return bytes(out), st + [(end, dict(SILENT))], end


def remap(st, cmap):
    """Logical channels' states as the chip's channels, by an effect slot's map."""
    out = []
    for t, r in st:
        x = dict(SILENT)
        x["n"], x["v3"] = r["n"], r["v3"]
        for lc in range(3):
            if cmap[lc] is not None:
                x["t%d" % cmap[lc]], x["v%d" % cmap[lc]] = r["t%d" % lc], r["v%d" % lc]
        out.append((t, x))
    return out


def rebase(block: bytes) -> bytes:
    """What RebaseDriverBlock makes of the block once it is loaded at cs:0416."""
    b = bytearray(block)
    for i in range(REBASED):
        w = (b[2 * i] | b[2 * i + 1] << 8) + BASE
        b[2 * i:2 * i + 2] = (w & 0xFFFF).to_bytes(2, "little")
    return bytes(b)


def steps(distance: int) -> int | None:
    """Volume steps an effect at this distance (map cells, src/E17A6.ASM Distance) loses; None: not played. The AdLib
    moves a sound's level (d - 1) / 16 of the way to silence (177D:01AF): for a full level about 2.95 dB a cell, which
    here is 2.95 dB a cell too (1.5 of the chip's 2 dB steps), so that from 11 cells a sound is silent and not played.
    7FFFh is no distance given (INTRO.EXE, and the start PlaySound makes): 0."""
    if distance == NO_DISTANCE or distance <= 1:
        return 0
    n = (distance - 1) * 3 >> 1
    return None if n >= 15 else n


def priority(distance: int) -> int:
    """An effect's claim on a channel, as the AdLib's (177D:01DA, the sound's priority byte 20h - distance)."""
    return 0x20 - (0 if distance == NO_DISTANCE else distance)


MUSIC_BIT = 0x8000                    # a table word with it set is music: it plays in the music slot (SNPLAY.ASM)
NO_DISTANCE = 0x7FFF                  # the distance word when nobody has written one since the last start
EFFECTS = 3                           # effect slots
H_CLOCK, H_STOP = 0x10, 0x20          # an effect's header: bits 0 to 3 its channels, 4 noise clocked by tone 2, 5 stop all


class Slot:
    def __init__(self, n: int):
        self.n = n                    # 0 the music, 1 to 3 the effects
        self.live = False
        self.prio = 0
        self.steps = 0
        self.seq = 0                  # when it started: the newer of two at one priority wins
        self.map = [0, 1, 2, 3]       # logical channel -> the chip's
        self.uses = 0
        self.period = [0, 0, 0]
        self.vol = [15, 15, 15, 15]   # as the stream has them (the music's: as written, its gains in them)
        self.noise = 0
        self.pos = self.hold = 0
        self.half = False


class Note:
    """A music channel's note (arrange.Voice, from the stream)."""

    def __init__(self, period: int, ins: dict, gain: int, dur: int):
        self.period, self.ins, self.gain, self.dur = period, ins, gain, dur
        self.age = self.count = 0
        self.vib = 0                  # the vibrato's tick, from VIB_DELAY on
        self.phase = 0                # 0 attack, 1 held, 2 released
        self.vol = 15


class Player:
    """SNPLAY.ASM, call for call: `call()` is one call of Tick and returns the bytes it writes to the chip. `data` is
    the block as the rebase leaves it (data[0] is cs:0416)."""

    def __init__(self, data: bytes):
        self.data = data
        self.slots = [Slot(n) for n in range(EFFECTS + 1)]
        self.music = self.slots[0]
        self.owner = [None, None, None, None]
        self.notes = [None, None, None]
        self.ins = 0                  # the music's instruments
        self.seq = 0
        self.flag = False
        self.div = 1                  # calls to the next tick: the player works on every fourth call
        self.taken = 0                # starts that began a sound
        self.taken_music = 0

    # -- starts and ends
    def call(self, start=None, stop=False) -> list[int]:
        """`start`: (gSongPtr, distance) when gStartRequest is set; `stop`: gStopRequest. Three calls in four do
        nothing: a start or a stop waits for the next tick."""
        out = []
        self.div -= 1
        if self.div:
            return out
        self.div = 4
        if start is not None:
            ptr, distance = start
            seq = self.seq
            if ptr & MUSIC_BIT:
                self.start_music(ptr & ~MUSIC_BIT, out)
            else:
                self.start_effect(ptr, distance, out)
            self.taken += self.seq != seq
            self.taken_music += self.seq != seq and bool(ptr & MUSIC_BIT)
        elif stop:
            self.stop_all(out)
        self.run_music(out)
        for s in self.slots[1:]:
            self.run_effect(s, out)
        return out

    @property
    def playing(self) -> bool:
        return any(s.live for s in self.slots)

    def stop_all(self, out):
        for s in self.slots:
            s.live = False
        self.notes = [None, None, None]
        self.music.vol = [15, 15, 15, 15]
        for c in range(4):
            self.owner[c] = None
            out.append(0x9F | c << 5)

    def start_effect(self, ptr: int, distance: int, out):
        st = steps(distance)
        if st is None:
            return
        at = ptr - BASE
        head = self.data[at]
        if head & H_STOP:
            self.stop_all(out)
        pr = priority(distance)
        free = [s for s in self.slots[1:] if not s.live]
        if free:
            s = free[0]
        else:
            s = min(self.slots[1:], key=lambda x: (x.prio, x.seq))
            if s.prio > pr:
                return
            s.live = False
            self.release(s, out)
        self.seq += 1
        s.live, s.prio, s.steps, s.seq, s.uses = True, pr, st, self.seq, head
        s.pos, s.half, s.hold = at + 1, False, 1
        s.period, s.vol, s.noise = [0, 0, 0], [15, 15, 15, 15], 0
        taken = set()
        s.map = [None, None, None, 3]
        if head & H_CLOCK and head & 4:
            s.map[2] = 2
            taken.add(2)
        for lc in range(3):
            if head & (1 << lc) and s.map[lc] is None:
                s.map[lc] = min((c for c in range(3) if c not in taken), key=lambda c: self.claim_cost(c))
                taken.add(s.map[lc])

    def claim_cost(self, c: int):
        """Which chip tone an effect's tone goes to: a free one first, then the music's, then the effect's that
        matters least; the lowest of equals."""
        o = self.owner[c]
        return (0 if o is None else 1 if o.n == 0 else 2 + o.prio, c)

    def release(self, s: Slot, out):
        """The channels a slot held go to the slot that wants each most (it sounds there), or are silenced."""
        for c in range(4):
            if self.owner[c] is not s:
                continue
            best = None
            for o in self.slots:
                if o is s or not o.live:
                    continue
                lc = self.logical(o, c)
                if lc is None or min(15, o.vol[lc] + o.steps) >= 15:
                    continue
                if best is None or (o.prio, o.seq) > (best.prio, best.seq):
                    best = o
            self.owner[c] = best
            if best is None:
                out.append(0x9F | c << 5)
            else:
                self.restore(best, c, out)

    def logical(self, s: Slot, c: int):
        if s.n == 0:
            return c
        for lc in range(4):
            if s.map[lc] == c and (lc == 3 and s.uses & 8 or lc < 3 and s.uses & (1 << lc)):
                return lc
        return None

    def restore(self, s: Slot, c: int, out):
        lc = self.logical(s, c)
        if c < 3:
            p = s.period[lc]
            out += [0x80 | c << 5 | p & 15, (p >> 4) & 0x3F]
        else:
            out.append(0xE0 | s.noise)
        out.append(0x90 | c << 5 | min(15, s.vol[lc] + s.steps))

    # -- writes
    def owns(self, s: Slot, c: int, out) -> bool:
        """May slot s write chip channel c now? Taking it from a lower claim writes all of s's channel."""
        o = self.owner[c]
        if o is s:
            return True
        if o is None or (s.prio, s.seq) >= (o.prio, o.seq):
            self.owner[c] = s
            self.restore(s, c, out)
        return False

    def tone(self, s: Slot, lc: int, out):
        c = s.map[lc]
        if self.owns(s, c, out):
            p = s.period[lc]
            out += [0x80 | c << 5 | p & 15, (p >> 4) & 0x3F]

    def volume(self, s: Slot, lc: int, out):
        c = s.map[lc]
        if self.owns(s, c, out):
            out.append(0x90 | c << 5 | min(15, s.vol[lc] + s.steps))

    def noise_reg(self, s: Slot, out):
        if self.owns(s, 3, out):
            out.append(0xE0 | s.noise)

    # -- the effects' streams
    def nib(self, s: Slot) -> int:
        b = self.data[s.pos]
        if s.half:
            s.pos += 1
            v = b >> 4
        else:
            v = b & 15
        s.half = not s.half
        return v

    def run_effect(self, s: Slot, out):
        if not s.live:
            return
        s.hold -= 1
        if s.hold:
            return
        nib = lambda: self.nib(s)    # noqa: E731
        a, b = nib(), nib()
        if a == 0 and b == 0:
            w = nib()
            if w == 0:
                w = nib() | nib() << 4
            if w:
                s.hold = w
                return
            if nib():
                self.flag = True
            s.live = False
            self.release(s, out)
            return
        s.hold = 1
        for lc in range(3):
            if a & (1 << lc):
                d = nib()
                if d == 0:
                    p = nib() | nib() << 4 | nib() << 8
                elif d == 8:
                    x = nib() | nib() << 4
                    p = (s.period[lc] + (x if x < 128 else x - 256)) & 0x3FF
                else:
                    p = (s.period[lc] + (d if d < 8 else d - 16)) & 0x3FF
                s.period[lc] = p
                self.tone(s, lc, out)
        if a & 8:
            s.noise = nib()
            self.noise_reg(s, out)
        for c in range(4):
            if b & (1 << c):
                s.vol[c] = nib()
                self.volume(s, c, out)

    # -- the music's notes
    def start_music(self, ptr: int, out):
        m = self.music
        if m.live:
            m.live = False
            self.release(m, out)
        self.notes = [None, None, None]
        at = ptr - BASE
        self.ins = at + 1                     # the instruments, after their count
        k = self.ins
        for _ in range(self.data[at]):
            k += self.data[k] + 5
        self.seq += 1
        m.live, m.prio, m.steps, m.seq, m.pos, m.hold = True, 0, 0, self.seq, k, 0
        m.period, m.vol = [0, 0, 0], [15, 15, 15, 15]

    def instrument(self, i: int) -> dict:
        k = self.ins
        for _ in range(i):
            k += self.data[k] + 5
        n = self.data[k]
        d = self.data
        return {"attack": list(d[k + 1:k + 1 + n]), "fall": d[k + 1 + n], "floor": d[k + 2 + n],
                "release": d[k + 3 + n], "vib": d[k + 4 + n]}

    def run_music(self, out):
        m, d = self.music, self.data
        if not m.live:
            return
        while m.hold == 0:
            e = d[m.pos]
            if e == 0:
                if d[m.pos + 1]:
                    self.flag = True
                m.live = False
                self.notes = [None, None, None]
                m.vol = [15, 15, 15, 15]
                self.release(m, out)
                return
            if e < 0x80:
                m.hold = e
                m.pos += 1
                continue
            c = e & 3
            note, ig, du = d[m.pos + 1], d[m.pos + 2], d[m.pos + 3]
            m.pos += 4
            if du & 0x80:
                du = (du & 0x7F) << 8 | d[m.pos]
                m.pos += 1
            self.notes[c] = Note(PERIODS[note], self.instrument(ig >> 4), ig & 15, du)
            m.period[c] = 0xFFFF              # written on its first tick
            m.vol[c] = 0xFF
        m.hold -= 1
        for c in range(3):
            n = self.notes[c]
            if n is None:
                continue
            p, v = self.note_tick(n)
            if p != m.period[c]:
                m.period[c] = p
                self.tone(m, c, out)
            if v != m.vol[c]:
                m.vol[c] = v
                self.volume(m, c, out)
            if n.phase == 3:
                self.notes[c] = None

    @staticmethod
    def note_tick(n: Note):
        ins = n.ins
        if n.phase < 2 and n.age >= n.dur:
            n.phase, n.count = 2, 0
        if n.phase == 0:
            n.vol = ins["attack"][n.age]
            if n.age + 1 == len(ins["attack"]):
                n.phase, n.count = 1, 0
        elif n.phase == 1:
            if ins["fall"]:
                n.count += 1
                if n.count == ins["fall"]:
                    n.count = 0
                    if n.vol < ins["floor"]:
                        n.vol += 1
        else:
            n.count += 1
            if n.count == ins["release"]:
                n.count = 0
                n.vol += 1
                if n.vol >= 15:
                    n.vol = 15
                    n.phase = 3
        p = n.period
        if ins["vib"] and n.age >= VIB_DELAY:
            p = min(0x3FF, p + VIB[n.vib] * (n.period >> 6))
            n.vib = (n.vib + 1) % len(VIB)
        n.age += 1
        return p, min(15, n.vol + n.gain)


def play(data: bytes, word: int, distance: int = NO_DISTANCE, ticks: int = 1 << 22):
    """One sound from a player at rest: (tick, byte) chip writes, the flag, the tick of its end and its slot's channel
    map, from its table word (GetSoundData's cs offset, bit 15 for music). Its ticks are every fourth call from the
    one that starts it."""
    p = Player(data)
    out = []
    for k in range(4 * ticks):
        out += [(k // 4, b) for b in p.call((word, distance) if k == 0 else None)]
        if k == 0:
            used = next((s for s in p.slots if s.live), p.music)
            cmap = list(used.map)
        if not p.playing:
            return out, int(p.flag), k // 4, cmap
    raise ValueError("no end")


def replay(data: bytes, calls, player: Player | None = None) -> list[list[int]]:
    """Every call of a session, (start or None, stop), through the model: the bytes each one writes."""
    p = player or Player(data)
    return [p.call(start, stop) for start, stop in calls]


CALL_RETURN = bytes([0x26, 0xFF, 0x1E, 0x16, 0x05])  # call dword ptr es:[516h], in _SoundTick (tandy-sound.patch),
                                                     # after mov ax, offset gSongPtr
DRIVER = {"start": 0x19E - 0x16, "stop": 0x19F - 0x16}  # the driver's variables from its gSongPtr (SNPLAY.ASM)


def hook_player(m, data: bytes, load_seg: int) -> dict:
    """Every call of the player in a run of the 8088 model (tools/tandybox.py), as the model needs it: the start it
    takes up ((gSongPtr, distance) when gStartRequest is set, else None), the stop request, and the length of the
    chip log when it is entered and when it returns to _SoundTick, so that its own writes are known."""
    from unicorn import UC_HOOK_CODE
    at = PLAYER_AT - BASE
    entry = data[at] | data[at + 1] << 8
    seen = {"calls": []}

    def on_entry(uc, a, size, ud):
        ds = m.get("ds")
        start = None
        if uc.mem_read(m.linear(ds, song + DRIVER["start"]), 1)[0]:
            ptr = struct.unpack("<H", bytes(uc.mem_read(m.linear(ds, song), 2)))[0]
            dist = struct.unpack("<h", bytes(uc.mem_read(m.linear(load_seg, 0x51C), 2)))[0]
            start = (ptr, dist)
        stop = bool(uc.mem_read(m.linear(ds, song + DRIVER["stop"]), 1)[0])
        seen["calls"].append([start, stop, len(m.tandy_log), None])

    def on_return(uc, a, size, ud):
        if seen["calls"]:
            seen["calls"][-1][3] = len(m.tandy_log)
    seg, off = m.build.addr("SoundTick")
    code = bytes(m.uc.mem_read(m.linear(seg, off), 64))
    k = code.find(CALL_RETURN)
    if k < 3 or code[k - 3] != 0xB8:
        raise RuntimeError("sndat: _SoundTick does not call the player (tandy-sound.patch)")
    song = code[k - 2] | code[k - 1] << 8                           # mov ax, offset gSongPtr
    m.uc.hook_add(UC_HOOK_CODE, on_entry, begin=m.linear(load_seg, entry), end=m.linear(load_seg, entry))
    ret = m.linear(seg, off + k + len(CALL_RETURN))
    m.uc.hook_add(UC_HOOK_CODE, on_return, begin=ret, end=ret)
    return seen


def player_diff(seen: dict, log, data: bytes) -> str | None:
    """Did every call of the player write what the model's does, given the same starts and stops? `log` is the chip
    log of the run, `data` the rebased block."""
    calls = [c for c in seen["calls"] if c[3] is not None]          # the run may stop inside the last call
    model = Player(data)
    want = replay(data, [(c[0], c[1]) for c in calls], model)
    for k, (c, w) in enumerate(zip(calls, want)):
        got = [b for _t, b in log[c[2]:c[3]]]
        if got != w:
            n = sum(1 for x in calls[:k] if x[0] is not None)
            return "call %d of %d (after %d starts): writes %s / the model's %s" % (
                k, len(calls), n, " ".join("%02X" % b for b in got[:12]), " ".join("%02X" % b for b in w[:12]))
    seen["played"], seen["music"] = model.taken, model.taken_music
    return None


def selftest(program: str = "EOB.EXE", calls: int = 60000, seed: int = 1) -> int:
    """The assembled player alone under the CPU emulator (unicorn) against Player: random starts (any sound, any
    distance, often several at once, music among them) and stops, every call's writes compared."""
    import random
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_INSN
    from unicorn.x86_const import (UC_X86_INS_OUT, UC_X86_REG_AX, UC_X86_REG_CS, UC_X86_REG_DS, UC_X86_REG_ES,
                                   UC_X86_REG_IP, UC_X86_REG_SP, UC_X86_REG_SS, UC_X86_REG_AL)
    r = build(program)
    block = rebase(r["block"])
    SEG, DRV, STK = 0x1000, 0x3000, 0x4000          # the block's segment (cs:0416 its byte 0), the driver's, a stack
    song = 0x0016
    uc = Uc(UC_ARCH_X86, UC_MODE_16)
    uc.mem_map(0, 0x100000)
    uc.mem_write(SEG * 16 + BASE, block)
    stub = 0x2000 * 16                               # call far [es:516h]; hlt
    uc.mem_write(stub, bytes([0x26, 0xFF, 0x1E, 0x16, 0x05, 0xF4]))
    uc.mem_write(SEG * 16 + 0x518, struct.pack("<H", SEG))
    outs = []

    def on_out(uc_, port, size, value, ud):
        if port == 0xC0:
            outs.append(value & 0xFF)
    uc.hook_add(UC_HOOK_INSN, on_out, None, 1, 0, UC_X86_INS_OUT)
    words = [block[2 * i] | block[2 * i + 1] << 8 for i in range(TABLE)]
    sounds = sorted(set(words[i["driver"]] for i in r["info"].values()) | {words[0]})
    model = Player(block)
    rnd = random.Random(seed)
    for k in range(calls):
        if rnd.random() < 0.02:                     # StartSound, and PlaySoundAtCell's distance
            w = rnd.choice(sounds)
            d = rnd.choice([NO_DISTANCE, 0, 0, 1, 2, 3, 5, 8, 10, 11, 14])
            uc.mem_write(DRV * 16 + song, struct.pack("<H", w))
            uc.mem_write(DRV * 16 + song + DRIVER["start"], b"\1")
            uc.mem_write(SEG * 16 + 0x51C, struct.pack("<H", d & 0xFFFF))
        uc.mem_write(DRV * 16 + song + DRIVER["stop"], b"\1" if rnd.random() < 0.0005 else b"\0")
        # what the player sees, as hook_player reads it
        start = None
        if uc.mem_read(DRV * 16 + song + DRIVER["start"], 1)[0]:
            start = (struct.unpack("<H", bytes(uc.mem_read(DRV * 16 + song, 2)))[0],
                     struct.unpack("<h", bytes(uc.mem_read(SEG * 16 + 0x51C, 2)))[0])
        stop = bool(uc.mem_read(DRV * 16 + song + DRIVER["stop"], 1)[0])
        for reg, v in ((UC_X86_REG_DS, DRV), (UC_X86_REG_ES, SEG), (UC_X86_REG_SS, STK), (UC_X86_REG_SP, 0xFFF0),
                       (UC_X86_REG_AX, song), (UC_X86_REG_CS, 0x2000), (UC_X86_REG_IP, 0)):
            uc.reg_write(reg, v)
        outs.clear()
        uc.emu_start(stub, stub + 5, count=200000)
        want = model.call(start, stop)
        if outs != want:
            print("sndat selftest %s: call %d (start %s, stop %s): writes %s / the model's %s" % (
                program, k, start, stop, " ".join("%02X" % b for b in outs[:16]), " ".join("%02X" % b for b in want[:16])))
            return 1
    print("sndat selftest %s: %d calls, every one's writes as the model's" % (program, calls))
    return 0


def check_intro() -> int:
    """The patched INTRO.EXE from its start to its last scene on the 8088 model: every call of the player against the
    model, and which of its sounds it asked for (GetSoundData's caller's argument: PlaySound's, ovl0:2D4E)."""
    sys.path.insert(0, str(HERE))
    import capture as C
    T = C.T
    m = C.machine("tandy", "INTRO.EXE", "game")
    data = rebase((T.builds_dir() / "game" / "SNSOUND.DAT").read_bytes()[120:])
    seen = hook_player(m, data, C.LOAD_SEG)
    asked = []
    seg, off = m.build.addr("_GetSoundData")

    def on_get(uc, a, size, ud):
        asked.append(struct.unpack("<H", bytes(uc.mem_read(m.linear(m.get("ss"), m.get("bp") + 6), 2)))[0])
    from unicorn import UC_HOOK_CODE
    m.uc.hook_add(UC_HOOK_CODE, on_get, begin=m.linear(seg, off), end=m.linear(seg, off))

    class Idle:
        def service(self, mach):
            pass

        def progress(self, *a, **k):
            return False
    m.keys = Idle()
    m.run_clocked(int(260 * T.CLOCK))
    bad = player_diff(seen, m.tandy_log, data)
    music = seen.get("music", 0)
    print("sndat check-intro: %d sounds asked for (%s), %d player calls, %d starts taken up (%d music): %s" % (
        len(asked), " ".join(str(g) for g in asked), len(seen["calls"]), seen.get("played", 0), music,
        bad or "every call as the model's"))
    return 1 if bad or music != 3 else 0


def same_sound(want_states, got_log) -> str | None:
    """Do the player's writes give the picked states, tick for tick, in what can be heard?"""
    got = states(got_log)
    gi = {t: r for t, r in got}
    w = {t: audible(r) for t, r in want_states}
    ticks = sorted(set(w) | set(gi))
    cur_w = cur_g = None
    for t in ticks:
        if t in w:
            cur_w = w[t]
        if t in gi:
            cur_g = audible(gi[t])
        if cur_w is not None and cur_g is not None and cur_w != cur_g:
            return "tick %d: %s / %s" % (t, cur_w, cur_g)
    return None


def assemble_player() -> bytes:
    """SNPLAY.ASM through the pinned TASM and TLINK: the image from PLAYER_AT on."""
    sys.path.insert(0, str(ROOT / "tools/re"))
    import tc_toolchain as tc
    w = OUT / "snplay"
    shutil.rmtree(w, ignore_errors=True)
    w.mkdir(parents=True)
    shutil.copy(PLAYER, w / "SNPLAY.ASM")
    (w / "PERIODS.INC").write_text(                      # arrange.py's tables, which the music plays from
        "; written by sndat.py from arrange.py: the period of each note from A2, and the vibrato\r\n"
        + "".join("%s %s\r\n" % ("Periods dw" if i == 0 else "        dw", ", ".join("%d" % x for x in PERIODS[i:i + 8]))
                  for i in range(0, len(PERIODS), 8))
        + "Vib     db %s\r\n" % ", ".join("%d" % (x & 0xFF) for x in VIB))
    r = tc.run(tc.manifest()["pinned_asm"], "TASM", ["/m3", "SNPLAY.ASM"], cwd=w)
    if not (w / "SNPLAY.OBJ").exists():
        sys.exit("sndat: SNPLAY.ASM does not assemble\n" + r.stdout.decode("latin-1"))
    r = tc.run("tc100", "TLINK", ["SNPLAY.OBJ"], cwd=w)
    exe = (w / "SNPLAY.EXE").read_bytes() if (w / "SNPLAY.EXE").exists() else b""
    if exe[:2] != b"MZ":
        sys.exit("sndat: SNPLAY.OBJ does not link\n" + r.stdout.decode("latin-1"))
    last, pages, nrel, hdr = struct.unpack("<HHHH", exe[2:10])
    if nrel:
        sys.exit("sndat: the player must not need relocations (%d)" % nrel)
    size = pages * 512 - (512 - last if last else 0)
    image = exe[hdr * 16:size]
    code = image[PLAYER_AT:]
    if image[:PLAYER_AT].strip(b"\0") or code[4:6] != b"SN":
        sys.exit("sndat: SNPLAY.EXE's image is not the player at %04X" % PLAYER_AT)
    return code


def game_sounds(picks_path: Path = PICKS):
    """EOB.EXE's: per game sound the picked clip's states and its end (tick, flag)."""
    picks = json.loads(picks_path.read_text())
    amap = json.loads((OUT / "sfx-adlib.json").read_text())["map"]
    out = {}
    for g in range(120):
        name = "game%03d" % g
        if not (0 < amap[g] < 0x96) or name not in picks:
            out[g] = 0xFF if amap[g] >= 0x96 else 0
            continue
        if g == MUSIC:                  # arranged for the chip (arrange.py), in the music slot, its end the loop point
            data, st, end = music(name, 1)
            out[g] = {"pick": "arranged", "states": st, "end": end, "flag": 1, "music": True, "data": data}
            continue
        pick = picks[name]["pick"]
        st = states(chip_log(name, pick))
        out[g] = {"pick": pick, "states": st, "end": max(t for t, _r in st) + 1, "flag": 0, "music": False}
    return out


INTRO_MUSIC = (1, 2, 3)               # INTRO.EXE's sounds that are music (capture.py INTRO_SOUNDS): the music slot


def intro_sounds():
    """INTRO.EXE's: each of its 14 sounds alone, rich (tandymap.py intro), the music in the music slot."""
    out = {0: 0}
    for g in range(1, 15):
        if g in INTRO_MUSIC:
            data, st, end = music("intro%02d" % g, 0)
            out[g] = {"pick": "arranged", "states": st, "end": end, "flag": 0, "music": True, "data": data}
            continue
        st = states(chip_log("intro%02d" % g, "rich"))
        out[g] = {"pick": "rich", "states": st, "end": max(t for t, _r in st) + 1, "flag": 0, "music": False}
    for g in range(15, 120):
        out[g] = 0
    return out


PROGRAMS = {
    # the file, what its block may take: from cs:0416 to the end of the AdLib driver (EOB.EXE: 0000:5028), or the
    # block INTRO.EXE's loader reads (DRIVER_BLOCK_SIZE, src/I0892.C: 36B0h; the driver runs to 0000:5400)
    "EOB.EXE": {"target": OUT / "SN.DAT", "max": BLOCK_MAX, "sounds": game_sounds},
    "INTRO.EXE": {"target": OUT / "SNSOUND.DAT", "max": 0x36B0, "sounds": intro_sounds},
}


def build(program: str = "EOB.EXE") -> dict:
    sounds = PROGRAMS[program]["sounds"]()
    streams = [(effect([], 1, stop=True), False)]       # driver sound 0: silence (PlaySound(0), and the map's 0): stops all
    index = {streams[0]: 0}
    gmap = bytearray(120)
    info = {}
    for g in range(120):
        x = sounds[g]
        if not isinstance(x, dict):
            gmap[g] = x
            continue
        key = (x["data"], True) if x["music"] else (effect(frames(x["states"]), x["end"], x["flag"]), False)
        if key not in index:
            index[key] = len(streams)
            streams.append(key)
        gmap[g] = index[key]
        info[g] = dict(x, driver=index[key], bytes=len(key[0]), ticks=x["end"])
    if len(streams) > 128:
        sys.exit("sndat: %d sounds, the map's values are 7 bits" % len(streams))
    player = assemble_player()
    head = TABLE * 2 + len(player)
    offs, body = [], bytearray()
    for data, music in streams:
        offs.append(head + len(body))
        body += data
    words = [o | (MUSIC_BIT if music else 0) for o, (_d, music) in zip(offs, streams)]
    table = words + [words[0]] * (TABLE - len(words))   # driver sounds past the last (a 96h or more map byte masked
    block = bytearray(b"".join(o.to_bytes(2, "little") for o in table)) + player + body   # to 7 bits) play the silence
    assert BASE + TABLE * 2 == PLAYER_AT
    block += bytes(max(0, 2 * REBASED - len(block)))
    for i in range(TABLE, REBASED):                     # stream bytes the rebase will add 0416h to
        w = (block[2 * i] | block[2 * i + 1] << 8) - BASE
        block[2 * i:2 * i + 2] = (w & 0xFFFF).to_bytes(2, "little")
    if len(block) > PROGRAMS[program]["max"]:
        sys.exit("sndat: %s's block is %d bytes, %d fit" % (program, len(block), PROGRAMS[program]["max"]))
    # the round trip: the player's writes give back what was picked
    loaded = rebase(bytes(block))
    if loaded[PLAYER_AT - BASE:PLAYER_AT - BASE + len(player)] != player:
        sys.exit("sndat: the rebase does not give the player back")
    for g, i in info.items():
        word = loaded[2 * i["driver"]] | loaded[2 * i["driver"] + 1] << 8      # GetSoundData's cs offset
        if bool(word & MUSIC_BIT) != i["music"]:
            sys.exit("sndat: %s sound %d is in the wrong slot" % (program, g))
        log, flag, t_end, cmap = play(loaded, word)
        bad = same_sound(i["states"] if i["music"] else remap(i["states"], cmap), log)
        if bad or t_end != i["ticks"] or flag != i["flag"]:
            sys.exit("sndat: %s sound %d does not play back: %s (end %d / %d, flag %d)" % (
                program, g, bad, t_end, i["ticks"], flag))
    return {"map": bytes(gmap), "block": bytes(block), "info": info, "streams": len(streams), "player": len(player),
            "offsets": offs, "program": program}


PASS = 4 * 4096 / 1193182             # s between two player ticks: four passes of the Tandy driver's timer


def check(r: dict) -> int:
    """Each game sound the capture started on the patched game: its chip writes, tick for tick, against play()."""
    cap = json.loads((OUT / "sfx-tandy-game.json").read_text())
    loaded = rebase(r["block"])
    bad = 0
    seen = set()
    for w in cap["windows"]:
        g = w["game"]
        if g not in r["info"]:
            continue
        seen.add(g)
        word = loaded[2 * r["info"][g]["driver"]] | loaded[2 * r["info"][g]["driver"] + 1] << 8
        want, _flag, _end, _map = play(loaded, word)
        got = w["writes"]
        t0 = got[0][0] if got else 0.0
        got = [(round((t - t0) / PASS), b) for t, b in got]
        w0 = want[0][0] if want else 0                  # both from their first write: a start writes nothing itself
        want = [(t - w0, b) for t, b in want]
        if got != want:
            i = next((i for i, (x, y) in enumerate(zip(got, want)) if x != y), min(len(got), len(want)))
            print("game %3d: DIFFERENT at write %d of %d / %d: %s / %s" % (
                g, i, len(got), len(want), got[i:i + 3], want[i:i + 3]))
            bad += 1
    missing = sorted(set(r["info"]) - seen)
    print("sndat check: %d game sounds, %d the same as the player's model, %d different, %d not captured %s" % (
        len(seen), len(seen) - bad, bad, len(missing), missing or ""))
    return 1 if bad or missing else 0


def main(argv=None) -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("what", choices=["build", "show", "check", "check-intro", "selftest"])
    ap.add_argument("--program", choices=list(PROGRAMS), help="build and show: one program's file (default: both)")
    a = ap.parse_args(argv)
    if a.what == "check":
        return check(build("EOB.EXE"))
    if a.what == "check-intro":
        return check_intro()
    if a.what == "selftest":
        return max(selftest(p) for p in ([a.program] if a.program else list(PROGRAMS)))
    for program in [a.program] if a.program else list(PROGRAMS):
        r = build(program)
        target = PROGRAMS[program]["target"]
        if a.what == "build":
            target.write_bytes(r["map"] + r["block"])
            print("sndat: %s, %d bytes: %d sounds in %d streams, the player %d bytes, block %d of %d bytes" % (
                target.relative_to(ROOT), target.stat().st_size, len(r["info"]), r["streams"], r["player"],
                len(r["block"]), PROGRAMS[program]["max"]))
        else:
            for g, i in sorted(r["info"].items()):
                print("%-9s %3d  %-8s driver %3d  %5d bytes  %5d ticks%s" % (
                    program, g, i["pick"], i["driver"], i["bytes"], i["ticks"], "  music" if i["music"] else ""))
    return 0


if __name__ == "__main__":
    sys.exit(main())

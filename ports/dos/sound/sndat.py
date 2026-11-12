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


def rebase(block: bytes) -> bytes:
    """What RebaseDriverBlock makes of the block once it is loaded at cs:0416."""
    b = bytearray(block)
    for i in range(REBASED):
        w = (b[2 * i] | b[2 * i + 1] << 8) + BASE
        b[2 * i:2 * i + 2] = (w & 0xFFFF).to_bytes(2, "little")
    return bytes(b)


def steps(distance: int) -> int | None:
    """Volume steps a sound at this distance loses (SNPLAY.ASM); None: not played."""
    if distance >= 16:
        return None
    return max(0, distance - 1) * 7 >> 3


MUSIC_BIT = 0x8000                    # a table word with it set is music: it plays in the music slot (SNPLAY.ASM)


class Slot:
    def __init__(self):
        self.pos = 0                  # in nibbles from the block's start
        self.hold = self.div = 0
        self.live = False
        self.steps = 0
        self.period = [0, 0, 0]
        self.vol = [15, 15, 15, 15]  # off: what an effect gives back with no music
        self.noise = 0


class Player:
    """SNPLAY.ASM, call for call: `call()` is one call of Tick and returns the bytes it writes to the chip. `data` is
    the block as the rebase leaves it (data[0] is cs:0416)."""

    def __init__(self, data: bytes):
        self.data = data
        self.music, self.effect = Slot(), Slot()
        self.own = 0
        self.flag = False             # the script flag set (cs:03A0)

    def call(self, start=None, stop=False) -> list[int]:
        """`start`: (gSongPtr, distance) when gStartRequest is set; `stop`: gStopRequest."""
        out = []
        if start is not None:
            ptr, distance = start
            if ptr & MUSIC_BIT:
                self.hush(out)
                self.begin(self.music, ptr & ~MUSIC_BIT)
            elif steps(distance) is not None:
                if self.effect.live:
                    self.give_back(out)
                self.effect.steps = steps(distance)
                self.begin(self.effect, ptr)
        elif stop:
            self.music.live = self.effect.live = False
            self.own = 0
            self.hush(out)
        self.run(self.music, out)
        self.run(self.effect, out)
        return out

    @property
    def playing(self) -> bool:
        return self.music.live or self.effect.live

    def begin(self, s: Slot, ptr: int):
        s.pos = (ptr - BASE) * 2
        s.hold = s.div = 1
        s.live = True

    def hush(self, out):
        self.music.vol = [15, 15, 15, 15]
        for c in range(4):
            if not self.own & (1 << c):
                out.append(0x9F | c << 5)

    def give_back(self, out):
        m = self.music
        for c in range(4):
            if self.own & (1 << c):
                if c < 3:
                    out += [0x80 | c << 5 | m.period[c] & 15, (m.period[c] >> 4) & 0xFF]
                else:
                    out.append(0xE0 | m.noise)
                out.append(0x90 | c << 5 | m.vol[c])
        self.own = 0

    def put(self, s: Slot, c: int, b: int, out):
        bit = 1 << c
        if s is self.effect:
            if not self.own & bit:
                self.own |= bit
                out.append(0x9F | c << 5)
        elif self.own & bit:
            return
        out.append(b)

    def nib(self, s: Slot) -> int:
        b = self.data[s.pos >> 1]
        v = (b >> 4) if s.pos & 1 else (b & 15)
        s.pos += 1
        return v

    def run(self, s: Slot, out):
        if not s.live:
            return
        s.div -= 1
        if s.div:
            return
        s.div = 4
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
            if s is self.music:
                self.hush(out)
            else:
                self.give_back(out)
            return
        s.hold = 1
        for c in range(3):
            if a & (1 << c):
                d = nib()
                if d == 0:
                    p = nib() | nib() << 4 | nib() << 8
                elif d == 8:
                    x = nib() | nib() << 4
                    p = (s.period[c] + (x if x < 128 else x - 256)) & 0x3FF
                else:
                    p = (s.period[c] + (d if d < 8 else d - 16)) & 0x3FF
                s.period[c] = p
                self.put(s, c, 0x80 | c << 5 | p & 15, out)
                self.put(s, c, p >> 4, out)
        if a & 8:
            s.noise = nib()
            self.put(s, 3, 0xE0 | s.noise, out)
        for c in range(4):
            if b & (1 << c):
                v = nib()
                s.vol[c] = v
                self.put(s, c, 0x90 | c << 5 | min(15, v + s.steps), out)


def play(data: bytes, start: int, distance: int = 0, ticks: int = 1 << 20):
    """One effect from a player at rest: (tick, byte) chip writes, the flag and the tick of its end, from the stream at
    `start` (an offset in the rebased block `data`). Its ticks are every fourth call from the one that starts it."""
    p = Player(data)
    out = []
    for k in range(4 * ticks):
        out += [(k // 4, b) for b in p.call((start + BASE, distance) if k == 0 else None)]
        if not p.playing:
            return out, int(p.flag), k // 4
    raise ValueError("no end")


def replay(data: bytes, calls) -> list[list[int]]:
    """Every call of a session, (start or None, stop), through the model: the bytes each one writes."""
    p = Player(data)
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
    want = replay(data, [(c[0], c[1]) for c in calls])
    for k, (c, w) in enumerate(zip(calls, want)):
        got = [b for _t, b in log[c[2]:c[3]]]
        if got != w:
            n = sum(1 for x in calls[:k] if x[0] is not None)
            return "call %d of %d (after %d starts): writes %s / the model's %s" % (
                k, len(calls), n, " ".join("%02X" % b for b in got[:12]), " ".join("%02X" % b for b in w[:12]))
    seen["played"] = sum(1 for c in calls if c[0] is not None and (c[0][0] & MUSIC_BIT or steps(c[0][1]) is not None))
    return None


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
    music = sum(1 for c in seen["calls"] if c[0] is not None and c[0][0] & MUSIC_BIT)
    print("sndat check-intro: %d sounds asked for (%s), %d player calls, %d starts (%d music): %s" % (
        len(asked), " ".join(str(g) for g in asked), len(seen["calls"]),
        sum(1 for c in seen["calls"] if c[0] is not None), music, bad or "every call as the model's"))
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
        pick = picks[name]["pick"]
        st = states(chip_log(name, pick))
        last = max(t for t, _r in st)
        end = round(MUSIC_LOOP * TICK) if g == MUSIC else last + 1
        if g == MUSIC:
            assert end > last, "the music's last write is after the AdLib's loop point"
        out[g] = {"pick": pick, "states": st, "end": end, "flag": int(g == MUSIC), "music": False}
    return out


INTRO_MUSIC = (1, 2, 3)               # INTRO.EXE's sounds that are music (capture.py INTRO_SOUNDS): the music slot


def intro_sounds():
    """INTRO.EXE's: each of its 14 sounds alone, rich (tandymap.py intro), the music in the music slot."""
    out = {0: 0}
    for g in range(1, 15):
        st = states(chip_log("intro%02d" % g, "rich"))
        out[g] = {"pick": "rich", "states": st, "end": max(t for t, _r in st) + 1, "flag": 0, "music": g in INTRO_MUSIC}
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
    streams = [(encode([], 1), False)]                  # driver sound 0: silence (PlaySound(0), and the map's 0)
    index = {streams[0]: 0}
    gmap = bytearray(120)
    info = {}
    for g in range(120):
        x = sounds[g]
        if not isinstance(x, dict):
            gmap[g] = x
            continue
        key = (encode(frames(x["states"]), x["end"], x["flag"]), x["music"])
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
        log, flag, t_end = play(loaded, (word & ~MUSIC_BIT) - BASE) if not i["music"] else play_music(loaded, word)
        bad = same_sound(i["states"], log)
        if bad or t_end != i["ticks"] or flag != i["flag"]:
            sys.exit("sndat: %s sound %d does not play back: %s (end %d / %d, flag %d)" % (
                program, g, bad, t_end, i["ticks"], flag))
    return {"map": bytes(gmap), "block": bytes(block), "info": info, "streams": len(streams), "player": len(player),
            "offsets": offs, "program": program}


def play_music(data: bytes, word: int):
    """A piece of music from a player at rest, as play() does an effect."""
    p = Player(data)
    out = []
    for k in range(1 << 22):
        out += [(k // 4, b) for b in p.call((word, 0) if k == 0 else None)]
        if not p.playing:
            return out, int(p.flag), k // 4
    raise ValueError("no end")


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
        want, _flag, _end = play(loaded, r["offsets"][r["info"][g]["driver"]])   # EOB.EXE's are all effects
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
    ap.add_argument("what", choices=["build", "show", "check", "check-intro"])
    ap.add_argument("--program", choices=list(PROGRAMS), help="build and show: one program's file (default: both)")
    a = ap.parse_args(argv)
    if a.what == "check":
        return check(build("EOB.EXE"))
    if a.what == "check-intro":
        return check_intro()
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

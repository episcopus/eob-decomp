#!/usr/bin/env python3
"""tandymap.py: a model of the proposed Tandy voice mapper, and the clips that compare it (README.md).

  tandymap.py clips [--only NAME ...]       every clip: chip logs for render, WAVs, metrics, index.html
  tandymap.py intro                         the intro's 14 sounds, each alone (capture.py introparts): clips/intro01
                                            to intro14, rich, which sndat.py makes SNSOUND.DAT of

The mapper is what the 8086 code would do inside the AdLib driver's tick (72.8 Hz): it sees only the OPL2
registers the driver writes, keeps a software envelope per FM voice, and gives the SN76489's three tones and
its noise channel to the voices that matter most at that moment. It is written to be portable to integer
8086 code (per-tick steps, small tables), not for speed in Python.

Modes:
  plain   envelopes, voice choice, noisy patches (as measured on ymfm) on the noise channel; a note's pitch is
          settled when it starts; bass below the chip's floor an octave up in the music, and in game sounds a
          buzz at its own pitch on periodic noise
  rich    plain, and: the free tones play the loudest further lines of the voices' FM spectra (J_k of the index
          the modulator's level gives), each line kept on its channel and its volume following the index; or a
          slight detune of the loudest voice when no line is loud enough; a low noisy voice clocks its noise
          from tone 2 when tone 2 is free

An effect of the intro (intro04 to intro14) plays over its music, which keeps the channels the effect leaves it
(SNPLAY.ASM): it is mapped rich without the further lines, so it takes no more tone channels than it has voices that
sound, and from tone 2 down, where the music has its further lines; the music keeps tone 0 longest.

Styles (per game sound, STYLE): "noise" puts the whole sound on the noise channel, its brightness from the FM
spectrum (plain: the nearest fixed rate; rich: white noise clocked by tone 2, following it tick by tick).
"""
from __future__ import annotations

import argparse
import collections
import json
import math
import struct
import subprocess
import time
import sys
from array import array
from pathlib import Path

import designs

HERE = Path(__file__).resolve().parent
PORT = HERE.parent
OUT = PORT / "build/sound"
RENDER = OUT / "render"

TICK = 1193182 / 0x555 / 12          # the AdLib driver's tick: PIT divisor 555h, every 12th pass (72.8 Hz)
SN_CLOCK = 3579545
SN_TONE = SN_CLOCK / 32              # tone Hz = SN_TONE / period
SN_FLOOR = SN_TONE / 1023            # 109.3 Hz: the lowest tone
OPS = [0, 1, 2, 8, 9, 10, 16, 17, 18]
MULT2 = [1, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 20, 24, 24, 30, 30]   # the OPL2 multiplier, times two
NOISE_FOLLOW = 24                    # rich: white noise shifts at this many times the voice's pitch (tone 2 drives it)
DB_STEP = 3.0                        # dB of OPL2 attenuation to one SN76489 volume step (2 dB): the AdLib's 45 dB
                                     # in the SN's 30, so that quiet and distant sounds still sound
NOISE_FLAT = 0.4                     # measured spectral flatness from which a voice is noise (white noise is 0.56)
SPLIT_FLAT = 0.2                     # from which it is a tone with noise in it: both, the noise SPLIT_DB quieter
SPLIT_DB = 6.0
BASS_NOISE = False                   # periodic-noise bass in the music: off, it sounded like a low rumble (first
                                     # listening, 2026-11-09), and the EX's chip may make periodic noise differently
AUDIBLE = 45.0                       # dB below full: quieter than this a voice is not worth a channel
TOP_PARTIAL = 16500                  # rich: no line above this on a tone channel (periods 7 and 8 are 16 and 14 kHz)
HIGH_LINE = 4000                     # a rich line above this is "high detail" on the page
LINE_HOLD = 9.0                      # rich: dB a line playing on a free tone must lose by before another takes it
                                     # (a swap while it sounds is a leap: game sound 103)
BUZZ_STEPS = 3                       # periodic noise (a 1-in-15 pulse) is about 6 dB weaker than a square wave
PERIODIC = 15                        # the Tandy's noise register: periodic noise repeats every 15 shifts (SMS Power)

# Per game sound, from listening (choices-2026-11-09.json): these are heard as noise and get the noise channel only
STYLE = {g: "noise" for g in (24, 25, 27, 32, 35, 40, 42, 44, 50, 57, 61, 63, 67, 74, 75, 81, 86, 116)}
MUSIC = ("intro", 20)                # the music: no periodic-noise buzz (first listening: a rumble under it)

# Per game sound, from the second listening (choices-2026-11-10.json):
#   vib      cents of vibrato on its tone channels, 6 Hz, after the attack (83, 110, 114, 115: "more vibrato"; 64,
#            a wail that sounded weak)
#   bed      dB under the sound's own level: soft white noise beneath it on the free noise channel (114)
#   release  ticks per volume step at most as a channel fades (111: "a more graceful fade out")
#   low      a voice below the chip's floor plays its loudest measured line above the floor, at that line's own
#            level, instead of its fundamental folded up at the voice's full level (the intro: the low drone of
#            its first part, a 16 Hz voice whose 128 Hz harmonic is 20 dB down, played as a full-volume square wave)
#            and no buzz (64: a 17 Hz voice whose sound is its harmonics around 1.3 kHz, a wail, not a 17 Hz buzz)
TWEAK = {83: {"vib": 35}, 110: {"vib": 30}, 114: {"vib": 30, "bed": 12}, 115: {"vib": 30}, 64: {"vib": 25, "low": True},
         111: {"release": 3}, "intro": {"low": True}}
VIB_HZ = 6.0
VIB_DELAY = 5                        # ticks before the vibrato starts, then as many again to reach its depth


def rate_time(r: int, ksr_on: bool, key: int, full: float) -> float:
    """Seconds for an envelope phase at rate r (0 to 15) to cover 96 dB; 0 is 'never'."""
    if r == 0:
        return math.inf
    eff = min(63, 4 * r + (key if ksr_on else key >> 2))
    if full < 3 and eff >= 60:
        return 0.0                   # attack at the top rates is instant
    return full * 2 ** (-(eff - 4) / 4)


class Op:
    """One operator's envelope, in dB of attenuation (0 loud, 96 silent)."""

    def __init__(self):
        self.db = 96.0
        self.stage = "off"

    def key_on(self):
        self.stage = "attack"

    def key_off(self):
        if self.stage != "off":
            self.stage = "release"

    def step(self, regs, slot, key, dt):
        r20, r60, r80 = regs[0x20 + slot], regs[0x60 + slot], regs[0x80 + slot]
        ksr = bool(r20 & 0x10)
        ar, dr, sl, rr = r60 >> 4, r60 & 15, r80 >> 4, r80 & 15
        sl_db = 93.0 if sl == 15 else 3.0 * sl
        if self.stage == "attack":
            # the OPL2's attack is exponential: attenuation falls in proportion to itself, so most of the way
            # to full level is covered early; ln(192) puts it within half a dB at the attack time
            t = rate_time(ar, ksr, key, 2.82624)
            self.db = 0.0 if t == 0 else self.db * math.exp(-5.26 * dt / t)
            if self.db <= 0.5:
                self.db = 0.0
                self.stage = "decay"
        elif self.stage == "decay":
            self.db = min(sl_db, self.db + 96 * dt / rate_time(dr, ksr, key, 39.28064))
            if self.db >= sl_db:
                self.stage = "sustain"
        elif self.stage == "sustain":
            if not r20 & 0x20:       # a percussive patch: no hold, it goes on at the release rate
                self.db = min(96.0, self.db + 96 * dt / rate_time(rr, ksr, key, 39.28064))
        elif self.stage == "release":
            self.db = min(96.0, self.db + 96 * dt / rate_time(rr, ksr, key, 39.28064))
            if self.db >= 96:
                self.stage = "off"


class Voice:
    def __init__(self, c):
        self.c = c
        self.mod, self.car = Op(), Op()
        self.keyed = False
        self.edge = False
        self.assigned = None
        self.decided = False          # what the note is, settled at its first audible tick
        self.kind = "tone"            # tone, noise, or split (an additive patch whose modulator is noise)
        self.mult2 = None             # the note's pitch as a multiple of half the base frequency, settled per note
        self.lines = None             # the note's measured line table (LINES)


def measured_lines(table, base, car_db, mod_db):
    """A voice's spectral lines from its measured table (render --lines): the table holds, at modulator levels of
    0, 3, ... 45 dB, each line as (frequency over the base frequency, dB against a lone sine at full level); the
    level between two steps is interpolated. Returns (Hz, dB, line id), loudest first."""
    a = min(45.0, max(0.0, mod_db))
    lo = int(a // 3) * 3
    hi = min(45, lo + 3)
    frac = (a - lo) / 3
    l0, l1 = table[str(lo)], table[str(hi)]
    out = {}
    for r, db in l0:
        out[round(2 * r, 1)] = [r, (1 - frac) * db + frac * -70.0]
    for r, db in l1:
        k = round(2 * r, 1)
        if k in out:
            out[k][1] += frac * (db + 70.0)
        else:
            out[k] = [r, (1 - frac) * -70.0 + frac * db]
    # a line's attenuation: the carrier's, plus how far the line is under a full sine
    lines = [(r * base, car_db - db, k) for k, (r, db) in out.items() if db > -60]
    return sorted(lines, key=lambda x: x[1])


def spectrum_centroid(partials):
    w = s = 0.0
    for f, db, _ in partials:
        p = 10 ** (-db / 10)
        w += p
        s += p * min(f, 22050)
    return s / w if w else 0.0


WAVE_DOUBLES = (False, False, True, True)   # waveforms 2 (abs sine) and 3 (pulse sine) repeat every half cycle
KSL_TOP = (0, 24, 32, 37, 40, 43, 45, 47, 48, 50, 51, 52, 53, 54, 55, 56)   # ymfm_opl.cpp: 0.75 dB units at block 7
KSL_SHARE = (0.0, 0.5, 0.25, 1.0)    # register value 1: 3 dB an octave, 2: 1.5, 3: 6


def ksl_db(reg40, fnum, block):
    """The key scale level's attenuation of an operator, in dB: more the higher the note (opl_key_scale_atten)."""
    return max(0, KSL_TOP[fnum >> 6] - 8 * (7 - block)) * 0.75 * KSL_SHARE[reg40 >> 6]


def voice_info(regs, v: Voice, key_scale):
    c = v.c
    m, k = OPS[c], OPS[c] + 3
    fnum = regs[0xA0 + c] | (regs[0xB0 + c] & 3) << 8
    block = (regs[0xB0 + c] >> 2) & 7
    base = fnum * 49716 / 2 ** (20 - block)
    c0 = regs[0xC0 + c]
    additive = bool(c0 & 1)
    mtl, ctl = regs[0x40 + m] & 63, regs[0x40 + k] & 63
    mm, cm = MULT2[regs[0x20 + m] & 15], MULT2[regs[0x20 + k] & 15]
    if WAVE_DOUBLES[regs[0xE0 + m] & 3]:
        mm *= 2
    if WAVE_DOUBLES[regs[0xE0 + k] & 3]:
        cm *= 2
    mod_db = mtl * 0.75 + ksl_db(regs[0x40 + m], fnum, block) + v.mod.db
    car_db = ctl * 0.75 + ksl_db(regs[0x40 + k], fnum, block) + v.car.db
    fc, fm = base * cm / 2, base * mm / 2
    if v.lines is None:
        v.lines = LINES.get(lines_key(regs, c), {})
    if additive:
        level = min(car_db, mod_db)
        mult2 = math.gcd(mm, cm)
        partials = sorted([(fc, car_db, 0), (fm, mod_db, 99)], key=lambda x: x[1])
        old_mult2 = mult2
    else:
        level = car_db
        # the pitch: the carrier's, unless the modulator is strong (by its level setting) and slow enough against
        # the carrier that the lines fill a harmonic series below it (as the intro's arpeggio: modulator half the
        # carrier); then the series' fundamental. Settled once per note: switching mid-note was heard as a jump of
        # an octave in game sounds 103 and 115, where the modulator's envelope crosses a threshold
        g = math.gcd(mm, cm)
        if v.mult2 is None:
            v.mult2 = g if (mtl * 0.75 < 24 and mm // g <= cm // g + 1) else cm
        mult2 = v.mult2
        # what the first pass's rule (decided every tick, on the modulator's level with its envelope) would play:
        # counted for the page's tags only
        old_mult2 = g if mod_db < 24 else cm
        partials = measured_lines(v.lines, base, car_db, mod_db) if v.lines else [(fc, car_db, 0)]
    ksl = max(ksl_db(regs[0x40 + m], fnum, block), ksl_db(regs[0x40 + k], fnum, block))
    return {"f": base * mult2 / 2, "level": level, "bright": not additive and mod_db < 24, "fnum": fnum,
            "block": block, "partials": partials, "pitch_changed": old_mult2 != mult2, "ksl": ksl,
            "wave": bool((regs[0xE0 + m] | regs[0xE0 + k]) & 3)}


PATCH_REGS = (0x20, 0x23, 0x40, 0x43, 0x60, 0x63, 0x80, 0x83, 0xE0, 0xE3, 0xC0)


def patch_key(regs, c):
    """A voice's sound as the chip has it at a key-on: its 11 patch registers (the carrier's level left out: it
    changes with distance and fades, not the timbre) and the block (feedback noise depends on the pitch)."""
    m = OPS[c]
    vals = [regs[(r + m) if r != 0xC0 else (r + c)] for r in PATCH_REGS]
    vals[3] = 0
    return "%s/%d" % ("".join("%02X" % v for v in vals), (regs[0xB0 + c] >> 2) & 7)


def lines_key(regs, c):
    """The key of a voice's line table: the patch without either level (the table is over the modulator's)."""
    k = patch_key(regs, c)
    return k[:4] + "00" + k[6:]


def keyons(windows):
    """Every voice sound the captures key on: (patch key, lines key, the values render --patches and --lines take)."""
    for w in windows:
        regs = bytearray(w.get("regs") or bytes(256))
        keyed = [bool(regs[0xB0 + c] & 0x20) for c in range(9)]
        for _, r, v in w["writes"]:
            regs[r] = v
            if 0xB0 <= r <= 0xB8:
                c = r - 0xB0
                k = bool(v & 0x20)
                if k and not keyed[c]:
                    m = OPS[c]
                    vals = [regs[(x + m) if x != 0xC0 else (x + c)] for x in PATCH_REGS]
                    fnum = regs[0xA0 + c] | (v & 3) << 8
                    yield patch_key(regs, c), lines_key(regs, c), vals + [fnum, (v >> 2) & 7]
                keyed[c] = k


def measure(windows, what, cache_name, parse):
    """Run render --patches or --lines on the voice sounds not yet in the cache (build/sound/<cache_name>)."""
    cache = OUT / cache_name
    known = json.loads(cache.read_text()) if cache.exists() else {}
    todo = {}
    for pkey, lkey, vals in keyons(windows):
        key = pkey if what == "patches" else lkey
        if key not in known and key not in todo:
            todo[key] = vals
    if todo:
        lst = OUT / ("%s.txt" % what)
        lst.write_text("".join("%s %s\n" % (k, " ".join(str(x) for x in vals)) for k, vals in todo.items()))
        res = subprocess.run([str(RENDER), "--" + what, str(lst)], check=True, capture_output=True, text=True).stdout
        parse(known, res)
        cache.write_text(json.dumps(known, indent=0, sort_keys=True))
    return known


def measure_patches(windows):
    """The spectral flatness of every voice sound the captures key on, measured on ymfm (render --patches)."""
    def parse(known, res):
        for line in res.splitlines():
            k, fl = line.split()
            known[k] = float(fl)
    return measure(windows, "patches", "patchnoise.json", parse)


def measure_line_tables(windows):
    """The spectral lines of every voice sound at each modulator level, measured on ymfm (render --lines)."""
    def parse(known, res):
        for line in res.splitlines():
            k, a, *pairs = line.split()
            known.setdefault(k, {})[a] = [[float(x) for x in pr.split(",")] for pr in pairs]
    return measure(windows, "lines", "lines.json", parse)


PATCH_NOISE = {}                     # patch_key -> spectral flatness, filled by clips()
LINES = {}                           # lines_key -> {modulator dB: [(frequency / base, dB)]}, filled by clips()


def period(f):
    return max(1, min(1023, round(SN_TONE / f)))


def vol(db, steps=0):
    """An SN76489 volume (0 loud, 15 off) for an attenuation in dB of the OPL2, `steps` louder (minus) or quieter."""
    return max(0, min(15, round(db / DB_STEP) + steps))


def noise_centroid(rate: float) -> float:
    """The spectral centroid of the chip's white noise shifted at `rate` Hz, through the renderer's output filters
    (12 kHz low-pass, 20 Hz high-pass), over 0 to 22,050 Hz: a sinc-squared spectrum."""
    s = w = 0.0
    for i in range(1, 400):
        f = 22050 * i / 400
        x = math.pi * f / rate
        p = (math.sin(x) / x) ** 2 / (1 + (f / 12000) ** 2) * (f * f / (f * f + 400))
        s += p * f
        w += p
    return s / w


NOISE_TABLE = [(p, noise_centroid(SN_TONE / p)) for p in range(1, 1024)]   # tone 2's period -> the noise's centroid


NOISE_LOWEST = 250                   # Hz of centroid: slower noise is a sparse crackle, not a rumble (game sound 40)


def noise_period(centroid: float) -> int:
    """Tone 2's period that clocks white noise to the brightness asked for (the 8086 version: a table of 16)."""
    for p, c in NOISE_TABLE:
        if c <= max(NOISE_LOWEST, centroid):
            return p
    return 1023


class Mapper:
    def __init__(self, mode: str, regs0, style: str = "", music: bool = False, tweak=None, effect: bool = False):
        self.mode = mode
        self.effect = effect          # an effect over music (the module's docstring): no further lines, tones from 2 down
        self.tweak = tweak or {}      # TWEAK
        self.sounding = 0             # ticks since the sound became audible (the vibrato's delay)
        self.style = style            # "noise": every voice on the noise channel only (STYLE)
        self.music = music
        self.regs = bytearray(regs0 or bytes(256))
        self.voices = [Voice(c) for c in range(9)]
        for v in self.voices:
            v.keyed = bool(self.regs[0xB0 + v.c] & 0x20)
        self.state = {"p": [None] * 3, "v": [15] * 4, "n": None}
        self.out = []                 # (t, byte)
        self.slot_of = {}             # OPL voice -> SN tone channel
        self.extra_of = {}            # rich: (OPL voice, line k) -> the free SN tone channel playing that FM line
        # the last four count the ticks where a rule new in the second pass changed what plays (the page's tags)
        self.stats = {"ticks": 0, "busy3": 0, "dropped": 0, "candidates": 0,
                      "pitch_kept": 0, "ksl": 0, "buzz": 0, "high_lines": 0, "wave": 0}

    def write(self, r, val):
        if 0xB0 <= r <= 0xB8:
            v = self.voices[r - 0xB0]
            k = bool(val & 0x20)
            if k and not v.keyed:
                v.edge = True
                v.decided = False
                v.mult2 = None
                v.lines = None
                v.mod.key_on()
                v.car.key_on()
            elif v.keyed and not k:
                v.mod.key_off()
                v.car.key_off()
            v.keyed = k
        self.regs[r] = val

    def emit(self, t, want):
        s = self.state
        for ch in range(3):
            p = want["p"][ch]
            if p is not None and p != s["p"][ch]:
                self.out.append((t, 0x80 | ch << 5 | (p & 15)))
                self.out.append((t, (p >> 4) & 0x3F))
                s["p"][ch] = p
        if want["n"] is not None and want["n"] != s["n"]:
            self.out.append((t, 0xE0 | want["n"]))
            s["n"] = want["n"]
        for ch in range(4):
            v = want["v"][ch]
            if v != s["v"][ch]:
                self.out.append((t, 0x90 | ch << 5 | v))
                s["v"][ch] = v

    def tick(self, t):
        dt = 1 / TICK
        regs = self.regs
        cand = []
        for v in self.voices:
            key = ((regs[0xB0 + v.c] >> 2) & 7) << 1 | (regs[0xB0 + v.c] >> 1) & 1
            silenced = regs[0x60 + OPS[v.c] + 3] == 0xFF and regs[0x80 + OPS[v.c] + 3] == 0xFF
            if silenced:              # VoiceSilence: both rates at the top, the voice is cut
                v.car.db = v.mod.db = 96.0
                v.car.stage = v.mod.stage = "off"
            v.mod.step(regs, OPS[v.c], key, dt)
            v.car.step(regs, OPS[v.c] + 3, key, dt)
            info = voice_info(regs, v, key)
            if info["level"] < AUDIBLE and info["f"] > 0:
                if not v.decided:
                    # what the note is, from how its sound measured on ymfm (measure_patches): the registers alone
                    # do not tell (feedback 6 with a strong modulator is as often a warm tone as a noise)
                    v.decided = True
                    flat = PATCH_NOISE.get(patch_key(regs, v.c), 0.0)
                    v.kind = "noise" if flat >= NOISE_FLAT else "split" if flat >= SPLIT_FLAT else "tone"
                info["v"] = v
                cand.append(info)
        self.stats["ticks"] += 1
        self.stats["candidates"] += len(cand)
        self.stats["pitch_kept"] += any(i["pitch_changed"] for i in cand)
        self.stats["ksl"] += any(i["ksl"] >= 3 for i in cand)
        self.stats["wave"] += any(i["wave"] for i in cand)
        rich = self.mode == "rich"
        want = {"p": [None] * 3, "v": [15] * 4, "n": None}
        if self.style == "noise":
            self.noise_only(cand, want, rich)
            for v in self.voices:
                v.edge = False
            self.emit(t, want)
            return
        tones = [i for i in cand if i["v"].kind != "noise" and i["level"] < AUDIBLE]
        noisy = []
        for i in cand:
            if i["v"].kind == "noise":
                noisy.append(dict(i, nlevel=i["level"]))
            elif i["v"].kind == "split" and i["level"] + SPLIT_DB < AUDIBLE:
                noisy.append(dict(i, nlevel=i["level"] + SPLIT_DB))
        if tones:
            lo = min(tones, key=lambda i: i["f"])
            hi = max(tones, key=lambda i: i["f"])
        for i in tones:
            s = -i["level"]
            if i["v"].edge:
                s += 6
            if i["v"].assigned is not None:
                s += 4
            if len(tones) > 3 and i is lo:
                s += 5
            if len(tones) > 3 and i is hi:
                s += 3
            i["score"] = s
        tones.sort(key=lambda i: -i["score"])
        noise_user = min(noisy, key=lambda i: i["nlevel"]) if noisy else None
        tone_slots = 3
        # a game sound's voice below the chip's floor: a buzz at its own pitch on periodic noise, clocked by tone 2
        # (the noise channel then repeats every 15 of tone 2's cycles), when it is louder than any noise; not in
        # the music, where it was heard as a rumble (BASS_NOISE)
        buzz = None
        if (BASS_NOISE or not self.music) and tones and not self.tweak.get("low"):
            low = [i for i in tones[:3] if i["f"] < SN_FLOOR]
            if low:
                b = min(low, key=lambda i: i["level"])
                if noise_user is None or b["level"] < noise_user["nlevel"]:
                    buzz, noise_user = b, None
                    tones = [i for i in tones if i is not buzz]
                    tone_slots = 2
        # rich: a low noisy voice (a rumble, below what the fixed rates reach) gets tone 2 to set its noise rate
        tone_follow = rich and noise_user is not None and len(tones) <= 2 and noise_user["v"].kind == "noise" \
            and noise_user["f"] < 150
        if tone_follow:
            tone_slots = 2
        chosen = tones[:tone_slots]
        self.stats["dropped"] += max(0, len(tones) - tone_slots)
        if len(cand) >= 3:
            self.stats["busy3"] += 1
        # keep a voice on the SN channel it had
        free = [ch for ch in range(tone_slots)]
        if self.effect:
            free.reverse()
        new_slot = {}
        for i in chosen:
            ch = self.slot_of.get(i["v"].c)
            if ch is not None and ch in free:
                new_slot[i["v"].c] = ch
                free.remove(ch)
        for i in chosen:
            if i["v"].c not in new_slot:
                new_slot[i["v"].c] = free.pop(0)
        for v in self.voices:
            v.assigned = new_slot.get(v.c)
        self.slot_of = new_slot
        for i in chosen:
            ch = new_slot[i["v"].c]
            f, db = i["f"], i["level"]
            if f < SN_FLOOR and self.tweak.get("low"):
                up = [(pf, pdb) for pf, pdb, _ in i["partials"] if SN_FLOOR <= pf <= 2000]
                if up:
                    f, db = min(up, key=lambda x: x[1])
                    i["shown"] = f
            while f < SN_FLOOR:
                f *= 2
                db += 6 if self.tweak.get("low") else 0
            want["p"][ch] = period(f)
            want["v"][ch] = vol(db)
        if rich:
            self.extra_lines(chosen, buzz, new_slot, [] if self.effect else free, want)
        else:
            self.extra_of = {}
        if noise_user is not None:
            f = noise_user["f"]
            if tone_follow:
                # white noise clocked by tone 2: the shift rate follows the voice's pitch
                want["n"] = 0x7
                want["p"][2] = max(2, min(1023, round(SN_TONE / max(30.0, f * NOISE_FOLLOW))))
                want["v"][2] = 15
            else:
                # feedback noise is broad: the fastest fixed rate, unless the voice is a low rumble
                want["n"] = 0x4 | (2 if f < 45 else 1 if f < 150 else 0)
            want["v"][3] = vol(noise_user["nlevel"])
        elif buzz is not None:
            self.stats["buzz"] += 1
            f = buzz["f"]
            while f * PERIODIC < SN_FLOOR:     # tone 2 cannot go below the floor: 7.3 Hz is the lowest buzz
                f *= 2
            want["n"] = 0x3
            want["p"][2] = period(f * PERIODIC)
            want["v"][2] = 15
            want["v"][3] = vol(buzz["level"], -BUZZ_STEPS)
        bed = self.tweak.get("bed")
        if bed is not None and want["n"] is None and cand:
            lines = [x for i in cand for x in i["partials"]]
            level = -10 * math.log10(sum(10 ** (-db / 10) for _, db, _ in lines))
            c = spectrum_centroid(lines)
            want["n"] = 0x4 | (0 if c > 1800 else 1 if c > 1000 else 2)
            want["v"][3] = vol(level + bed)
        self.shape(cand, want)
        for v in self.voices:
            v.edge = False
        self.emit(t, want)

    def shape(self, cand, want):
        """TWEAK's vibrato and release, on what the tick wants before it is written."""
        self.sounding = self.sounding + 1 if cand else 0
        cents = self.tweak.get("vib")
        if cents and self.sounding > VIB_DELAY:
            depth = cents * min(1.0, (self.sounding - VIB_DELAY) / VIB_DELAY)
            x = 2 ** (depth / 1200 * math.sin(2 * math.pi * VIB_HZ * self.sounding / TICK))
            for ch in range(3):
                p = want["p"][ch]
                if p is not None and want["v"][ch] < 15:
                    want["p"][ch] = max(1, min(1023, round(p / x)))
        rel = self.tweak.get("release")
        if rel:
            self.fade = getattr(self, "fade", 0) + 1
            for ch in range(4):
                was = self.state["v"][ch]
                if want["v"][ch] > was:
                    if self.fade % rel:
                        want["v"][ch] = was
                    else:
                        want["v"][ch] = was + 1
                    if ch < 3 and want["p"][ch] is None:
                        want["p"][ch] = self.state["p"][ch]

    def extra_lines(self, chosen, buzz, slot, free, want):
        """rich: the free tone channels play the loudest further lines of the measured spectra of the voices that
        sound (not the line a voice's own channel plays). A line keeps its channel while it lasts, and its volume
        follows the modulator's level, so the brightness comes and goes smoothly with its envelope. Each voice's
        played lines are then balanced as its spectrum has them and together carry its whole power. With no line
        to play, a slight detune of the loudest voice."""
        pool = chosen + ([buzz] if buzz else [])
        lines = []
        for i in pool:
            v = i["v"]
            own = i.get("shown") or i["f"]       # the line its own channel plays (TWEAK "low": not the fundamental)
            while own < SN_FLOOR:
                own *= 2
            for f, db, k in i["partials"]:
                if f < SN_FLOOR or f > TOP_PARTIAL or db >= AUDIBLE or abs(f / own - 1) < 0.03:
                    continue
                key = (v.c, k)
                lines.append([-db + (LINE_HOLD if key in self.extra_of else 0), key, f, db])
        lines.sort(key=lambda x: -x[0])
        new = {}
        left = list(free)
        used = {want["p"][ch] for ch in range(3) if want["p"][ch] is not None}
        for _, key, f, db in lines:
            if not left:
                break
            if key in new or period(f) in used:      # two lines on one period: the chip plays it once
                continue
            used.add(period(f))
            ch = self.extra_of.get(key)
            if ch not in left:
                ch = left[0]
            left.remove(ch)
            new[key] = (ch, f, db)
            want["p"][ch] = period(f)
            want["v"][ch] = vol(db)
        self.extra_of = {key: x[0] for key, x in new.items()}
        self.stats["high_lines"] += any(f > HIGH_LINE for _, f, _ in new.values())
        for i in chosen:
            c, ls = i["v"].c, i["partials"]
            if len(ls) < 2:
                continue
            own = i.get("shown") or i["f"]       # the line its own channel plays (TWEAK "low": not the fundamental)
            while own < SN_FLOOR:
                own *= 2
            near = [db for f, db, _ in ls if abs(f / own - 1) < 0.03]
            own_db = min(near) if near else ls[0][1]
            mine = [(ch, db) for (vc, _), (ch, f, db) in new.items() if vc == c]
            # (TWEAK "low": the power of the lines the chip can play, none below its floor)
            total = sum(10 ** (-db / 10) for f, db, _ in ls if f >= SN_FLOOR or not self.tweak.get("low"))
            if not total:
                continue
            got = 10 ** (-own_db / 10) + sum(10 ** (-db / 10) for _, db in mine)
            gain = 10 * math.log10(total / got)
            # the voice's loudness maps as every level does (DB_STEP), but the lines within it keep their dB apart
            # (2 dB a step), or a quiet sideband comes out nearly as loud as the carrier
            top = own_db - gain
            want["v"][slot[c]] = vol(top)
            for ch, db in mine:
                want["v"][ch] = min(15, vol(top) + max(0, round((db - own_db) / 2)))
        if buzz is not None:
            for (vc, _), (ch, f, db) in new.items():
                if vc == buzz["v"].c:
                    want["v"][ch] = min(15, vol(buzz["level"]) + max(0, round((db - buzz["level"]) / 2)))
        if left and not new and chosen:
            ch = left[0]
            top = min(chosen, key=lambda i: i["level"])
            f = top["f"]
            while f < SN_FLOOR:
                f *= 2
            want["p"][ch] = min(1023, period(f) + 1)
            want["v"][ch] = vol(top["level"], 2)

    def noise_only(self, cand, want, rich):
        """STYLE "noise": the sound on the noise channel alone, as loud as all its voices together. plain: the
        fixed rate nearest its brightness; rich: white noise clocked by tone 2, its brightness following the FM
        spectrum's centroid tick by tick (so a slide or a fading modulator is heard)."""
        if not cand:
            return
        lines = [x for i in cand for x in i["partials"]]
        level = -10 * math.log10(sum(10 ** (-db / 10) for _, db, _ in lines))
        c = spectrum_centroid(lines)
        if rich:
            want["n"] = 0x7
            want["p"][2] = noise_period(c)
            want["v"][2] = 15
        else:
            want["n"] = 0x4 | (0 if c > 1800 else 1 if c > 1000 else 2)
        want["v"][3] = vol(level)


INTRO_MUSIC = ("intro01", "intro02", "intro03")   # the intro's sounds 1 to 3: its music, mapped as the whole intro was


def map_window(w, mode):
    g = w["game"]
    effect = isinstance(g, str) and g.startswith("intro") and g != "intro" and g not in INTRO_MUSIC
    m = Mapper(mode, w.get("regs"), style=STYLE.get(g, ""), music=g in MUSIC or g in INTRO_MUSIC,
               tweak=TWEAK.get(g, TWEAK["intro"] if g in INTRO_MUSIC else {}), effect=effect)
    writes = w["writes"]
    t0, t1 = w["t0"], w["t1"]
    k = 0
    # the mapper runs at the end of the driver's own tick: align the grid to the first write, after its burst (writes take up to 7 ms)
    start = (writes[0][0] + 0.009) if writes else t0
    while start - 1 / TICK > t0:
        start -= 1 / TICK
    n = int((t1 - start) * TICK) + 1
    for i in range(n):
        t = start + i / TICK
        while k < len(writes) and writes[k][0] <= t:
            _, r, val = writes[k]
            m.write(r, val)
            k += 1
        m.tick(t)
    return m


def write_opl(path, w):
    with open(path, "w") as f:
        f.write("chip opl\nlen %.3f\n" % (w["t1"] - w["t0"] + 0.5))
        regs = w.get("regs") or [0] * 256
        # 01 (waveform select: the driver turns it on, and most of the game's voices use the other waveforms), 08
        # and BD with the voices' registers
        for r in [0x01, 0x08] + list(range(0x20, 0x100)):
            if regs[r] and not 0xB0 <= r <= 0xB8:
                f.write("init %d %d\n" % (r, regs[r]))
        for r in range(0xB0, 0xB9):           # without the key bits: the state, not a note left sounding
            if regs[r] & 0x1F:
                f.write("init %d %d\n" % (r, regs[r] & 0x1F))
        for t, r, v in w["writes"]:
            f.write("w %.6f %d %d\n" % (t - w["t0"], r, v))


def write_sn(path, t0, length, writes):
    with open(path, "w") as f:
        f.write("chip sn\nlen %.3f\n" % length)
        for b in (0x9F, 0xBF, 0xDF, 0xFF):           # all four channels silent at the start
            f.write("w 0 %d\n" % b)
        for t, b in writes:
            f.write("w %.6f %d\n" % (max(0.0, t - t0), b))


def wav(path):
    d = path.read_bytes()
    a = array("h")
    a.frombytes(d[44:])
    return a


def envelope(a, block=441):
    e = []
    for i in range(0, len(a) - block, block):
        s = 0
        for x in a[i:i + block:3]:
            s += x * x
        e.append(10 * math.log10(s / (block / 3) + 1))
    return e



def centroid(path):
    return float(subprocess.run([str(RENDER), "--centroid", str(path)], check=True, capture_output=True, text=True).stdout)


def corr(x, y):
    n = min(len(x), len(y))
    if n < 3:
        return float("nan")
    x, y = x[:n], y[:n]
    mx, my = sum(x) / n, sum(y) / n
    sxy = sum((a - mx) * (b - my) for a, b in zip(x, y))
    sx = math.sqrt(sum((a - mx) ** 2 for a in x))
    sy = math.sqrt(sum((b - my) ** 2 for b in y))
    return sxy / (sx * sy) if sx and sy else float("nan")


NAMES = {20: "character generation music", 21: "a hit on a character", 22: "a character's death (jingle)",
         30: "ambient noise, levels 1 to 3", 79: "refused"}


def clips(only):
    adl = json.loads((OUT / "sfx-adlib.json").read_text())
    tdy = json.loads((OUT / "sfx-tandy.json").read_text())
    tw = {w["game"]: w for w in tdy["windows"]}
    tmap = tdy["map"]
    items = []
    for w in adl["windows"]:
        g = w["game"]
        name = "game%03d" % g
        items.append((name, NAMES.get(g, ""), w, tw.get(g) if 0 < tmap[g] < 0x96 else None))
    ia = json.loads((OUT / "intro-adlib.json").read_text())["windows"][0]
    PATCH_NOISE.update(measure_patches([w for w in adl["windows"]] + [ia]))
    LINES.update(measure_line_tables([w for w in adl["windows"]] + [ia]))
    it = json.loads((OUT / "intro-tandy.json").read_text())["windows"][0]
    items.insert(0, ("intro", "the intro, to its hand-over", ia, it))
    if only:
        items = [x for x in items if x[0] in only]
    rows = []
    for name, desc, w, tw_ in items:
        d = OUT / "clips" / name
        d.mkdir(parents=True, exist_ok=True)
        length = w["t1"] - w["t0"] + 0.5
        write_opl(d / "adlib.txt", w)
        files = {"adlib": d / "adlib.wav"}
        subprocess.run([str(RENDER), str(d / "adlib.txt"), str(files["adlib"])], check=True)
        stats = {}
        for mode in ("plain", "rich"):
            m = map_window(w, mode)
            write_sn(d / ("tandy-%s.txt" % mode), w["t0"], length, m.out)
            files[mode] = d / ("tandy-%s.wav" % mode)
            subprocess.run([str(RENDER), str(d / ("tandy-%s.txt" % mode)), str(files[mode])], check=True)
            stats[mode] = dict(m.stats, writes=len(m.out), writes_per_s=len(m.out) / max(0.1, length))
        made = designs.render(w["game"]) if isinstance(w["game"], int) else None
        if made is not None:
            write_sn(d / "tandy-designed.txt", 0.0, made[1], made[0])
            files["designed"] = d / "tandy-designed.wav"
            subprocess.run([str(RENDER), str(d / "tandy-designed.txt"), str(files["designed"])], check=True)
        if tw_ is not None and tw_["writes"]:
            write_sn(d / "tandy-now.txt", tw_["t0"], tw_["t1"] - tw_["t0"] + 0.5, [(t, b) for t, b in tw_["writes"]])
            files["now"] = d / "tandy-now.wav"
            subprocess.run([str(RENDER), str(d / "tandy-now.txt"), str(files["now"])], check=True)
        ea = envelope(wav(files["adlib"]))
        met = {}
        for k in ("now", "plain", "rich", "designed"):
            if k in files:
                a = wav(files[k])
                met[k] = {"env_corr": corr(ea, envelope(a)), "centroid": centroid(files[k])}
        met["adlib"] = {"centroid": centroid(files["adlib"])}
        rows.append({"name": name, "desc": desc, "length": length, "files": {k: str(v.relative_to(OUT)) for k, v in files.items()},
                     "metrics": met, "stats": stats})
        print("%-8s %6.1fs  env corr now %5s plain %.2f rich %.2f | centroid Hz adlib %5.0f now %5s plain %5.0f rich %5.0f | SN writes/s %.0f" % (
            name, length, ("%.2f" % met["now"]["env_corr"]) if "now" in met else "  -  ", met["plain"]["env_corr"],
            met["rich"]["env_corr"], met["adlib"]["centroid"], ("%.0f" % met["now"]["centroid"]) if "now" in met else "-",
            met["plain"]["centroid"], met["rich"]["centroid"], stats["rich"]["writes_per_s"]), flush=True)
    old = {}
    p = OUT / "clips.json"
    if p.exists() and only:
        old = {r["name"]: r for r in json.loads(p.read_text())}
    for r in rows:
        old[r["name"]] = r
    allrows = sorted(old.values(), key=lambda r: (r["name"] != "intro", r["name"] != "game020", r["name"]))
    p.write_text(json.dumps(allrows, indent=1))
    index(allrows)


PASSES = [HERE / "choices-2026-11-09.json", HERE / "choices-2026-11-10.json",
          HERE / "choices-2026-11-11.json"]   # the listening passes, in order (the third: README.md, "Fourth pass")
PREVIOUS = OUT / "pass3"             # the renders the last pass listened to (copied before this run)
NOTES = {"intro": "the background chord of the first part: too loud, grating", 15: "nothing like the AdLib",
         19: "grating", 33: "too loud", 40: "should be a low whump", 57: "no sound", 64: "a wail; rich closest but weak",
         69: "low rumble, not good", 71: "low rumble, not good", 83: "more vibrato", 97: "not good",
         107: "horrible; maybe a different sound for its use", 108: "horrible; maybe a different sound for its use",
         109: "horrible; maybe a different sound for its use", 110: "not enough vibrato", 111: "a more graceful fade out",
         114: "more vibrato, a bit of noise in the background", 115: "more vibrato"}   # the second pass's notes
PICK_CLASS = {"now": "p-now", "plain": "p-plain", "rich": "p-rich", "none good": "p-none", "designed": "p-designed"}


def changed_files(name):
    """Which Tandy versions differ from what the last pass listened to."""
    out = []
    for k in ("plain", "rich"):
        a, b = OUT / "clips" / name / ("tandy-%s.txt" % k), PREVIOUS / name / ("tandy-%s.txt" % k)
        if a.exists() and (not b.exists() or a.read_bytes() != b.read_bytes()):
            out.append(k)
    return out


def index(rows):
    stamp = int(time.time())
    passes = [json.loads(p.read_text()) if p.exists() else {} for p in PASSES]

    def player(r, k):
        f = r["files"].get(k)
        return '<audio controls preload="none" src="%s?v=%d"></audio>' % (f, stamp) if f else '<span class="none">-</span>'

    def num(x):
        return "-" if x is None or (isinstance(x, float) and math.isnan(x)) else "%.2f" % x
    body = []
    counts = collections.Counter()
    kinds = collections.Counter()
    for r in rows:
        met = r["metrics"]
        name = r["name"]
        g = int(name[4:]) if name.startswith("game") else name
        old = [p.get(name, {}).get("pick") for p in passes]
        last = old[-1] or ""
        opts = ((["designed"] if "designed" in r["files"] else []) + (["now"] if "now" in r["files"] else [])
                + ["plain", "rich", "none good"])
        pick = "".join('<label class="%s"><input type="radio" name="%s" value="%s">%s%s</label><br>' % (
            "was" if v == last else "", name, v, v, ' <small>(last pass)</small>' if v == last else "") for v in opts)
        flags = []
        design = designs.DESIGNS.get(g)
        if design:
            if design.batch == 1:
                flags.append("accepted")
            else:
                flags.append("new")
                flags.append("kind-" + design.kind)
                kinds[design.kind] += 1
        ch = changed_files(name)
        if ch:
            flags.append("changed")
        for k in flags:
            counts[k] += 1
        chips = "".join('<span class="chip %s">pass %d: %s</span>' % (PICK_CLASS.get(o, ""), n + 1, o)
                        for n, o in enumerate(old) if o)
        if design and design.batch == 1:
            chips += '<span class="chip p-designed">designed, pass 3: kept</span>'
        elif design:
            chips += '<span class="chip p-new">new design</span>'
        lines = [("%s %.1f s" % (r["desc"], r["length"])).strip()]
        if design:
            lines.append('<span class=use>used for: %s</span>' % design.use)
            lines.append('<span class=done>designed: %s</span>' % design.what)
        if g in NOTES:
            lines.append('<span class=note-old>your pass 2 note: %s</span>' % NOTES[g])
        if ch:
            lines.append('<span class=done>changed since the last pass: %s</span>' % ", ".join(ch))
        envs = "%snow %s<br>plain %s<br>rich %s" % (
            ("designed %s<br>" % num(met["designed"]["env_corr"])) if "designed" in met else "",
            num(met.get("now", {}).get("env_corr")), num(met["plain"]["env_corr"]), num(met["rich"]["env_corr"]))
        body.append('<tr data-flags="%s" data-last="%s"><td><span class=name>%s</span><div>%s</div><small>%s</small></td>'
                    "<td>%s</td><td class=dcol>%s</td><td>%s</td><td>%s</td><td>%s</td><td><small>%s</small></td>"
                    "<td class=pick>%s<input class=note name=\"%s\" placeholder=\"note\"></td></tr>" % (
                        " ".join(flags), last, name, chips, "<br>".join(lines),
                        player(r, "adlib"), player(r, "designed"), player(r, "now"), player(r, "plain"), player(r, "rich"),
                        envs, pick, name))
    kind_names = designs.KINDS
    filters = ('<button data-f="new" class=on><span class="chip p-new">new designs %d</span></button>' % counts["new"]
               + '<button data-f="todo">new designs not picked yet <span id=todo></span></button>'
               + '<button data-f="accepted"><span class="chip p-designed">designed in pass 3, kept %d</span></button>' % counts["accepted"]
               + '<button data-f="">all %d</button>' % len(rows)
               + ('<button data-f="changed">changed since the last pass %d</button>' % counts["changed"] if counts["changed"] else "")
               + '<button data-f="mine">picked this pass</button>'
               + '<br>New designs by use: '
               + "".join('<button data-f="kind-%s">%s %d</button>' % (k, kind_names[k], kinds[k]) for k in kind_names if kinds[k])
               + '<br>Your last pick: '
               + "".join('<button data-f="last:%s"><span class="chip %s">%s</span></button>' % (k, PICK_CLASS[k], k)
                         for k in ("designed", "now", "plain", "rich")))
    html = r"""<!doctype html><html><head><meta charset="utf-8"><title>Tandy FM study</title>
<style>body{font:14px system-ui,sans-serif;margin:16px;background:#fff;color:#222}table{border-collapse:collapse}
td,th{border-bottom:1px solid #ddd;padding:6px 8px;vertical-align:top;text-align:left}audio{width:190px;height:32px}
.none{color:#bbb}small{color:#666}.name{font-family:ui-monospace,monospace;font-size:15px}.pick{white-space:nowrap}
.note{width:140px;margin-top:4px}.chip{display:inline-block;border-radius:9px;padding:1px 7px;margin:3px 3px 0 0;
font-size:12px;color:#fff;white-space:nowrap}.p-now{background:#6b6b6b}.p-plain{background:#2563a8}.p-rich{background:#0f7d6b}
.p-none{background:#b4235a}.p-designed{background:#8a5a12}.p-new{background:#6b3fa0}.note-old{color:#b4235a}
.done{color:#0f6a3a}.use{color:#222}label.was{background:#fff3c4;border-radius:4px;padding:0 4px}
tr[data-flags~="new"] td:first-child{border-left:5px solid #6b3fa0}
tr[data-flags~="accepted"] td:first-child{border-left:5px solid #8a5a12}
tr[data-flags~="new"] td.dcol{background:#f4effa}th.dcol{background:#f4effa}
td:first-child{min-width:240px;max-width:320px}
#filters{position:sticky;top:0;background:#fff;padding:8px 0;border-bottom:1px solid #ddd;z-index:1;line-height:2}
#filters button{margin:2px;padding:3px 8px;border:1px solid #bbb;background:#f6f6f6;border-radius:4px;cursor:pointer}
#filters button.on{outline:2px solid #222}#filters .chip{margin:0}</style></head><body>
<h1>Tandy FM study: designed sounds (fourth pass)</h1>
<p>You kept every designed sound of the third pass, so this pass designs the rest: every game sound except the two
pieces of music (the intro and the character generation music) now has a Designed version, a new sound written for the
Tandy's chip from scratch (designs.py), made for what the sound is used for in the game rather than copied from the
AdLib. The Designed column sits next to the AdLib one. The other columns are as before: Tandy now is the game's own
Tandy driver; plain and rich are the AdLib driver's writes through the mapper (tandymap.py). Loudness is matched as
heard (K-weighted). Method and results: ports/dos/sound/README.md.</p>
<p>A purple bar on the left marks a new design, a brown one a design you kept in pass 3 (already picked). Under each
name: your picks so far, what the sound is used for in the game, and what the design does. Your last pick is
highlighted in yellow and selected; picking another replaces it. The filters start on the new designs; "by use" groups
them (all the footsteps together, say), and "not picked yet" shows the new designs you have not decided on.</p>
<p>When done, <button id=dl>Download choices</button> and give the file back (it holds every clip: your picks, and
your last pass's pick for the rest). <span id=count></span></p>
<div id=filters>Show: @FILTERS@ <span id=shown></span></div>
<table><tr><th>Clip</th><th>AdLib</th><th class=dcol>Designed</th><th>Tandy now</th><th>Tandy plain</th><th>Tandy rich</th>
<th>Envelope match</th><th>Best</th></tr>
@ROWS@</table>
<script>
const KEY = "tandy-fm-choices-4";
let state = {};
try { state = JSON.parse(localStorage.getItem(KEY) || "{}"); } catch (e) { state = {}; }
const rows = [...document.querySelectorAll("tr[data-flags]")];
const nameOf = tr => tr.querySelector("input.note").name;
function save() { try { localStorage.setItem(KEY, JSON.stringify(state)); } catch (e) {} count(); }
function count() {
  const mine = Object.values(state).filter(x => x.pick).length;
  const todo = rows.filter(tr => tr.dataset.flags.split(" ").includes("new") && !(state[nameOf(tr)] || {}).pick).length;
  document.getElementById("count").textContent = mine + " picked this pass";
  document.getElementById("todo").textContent = todo;
}
rows.forEach(tr => {
  const s = state[nameOf(tr)] || {};
  tr.querySelectorAll("input[type=radio]").forEach(el => {
    el.checked = s.pick ? s.pick === el.value : el.value === tr.dataset.last;
    el.addEventListener("change", () => { (state[el.name] = state[el.name] || {}).pick = el.value; save(); });
  });
});
document.querySelectorAll("input.note").forEach(el => {
  const s = state[el.name]; if (s && s.note) el.value = s.note;
  el.addEventListener("input", () => { (state[el.name] = state[el.name] || {}).note = el.value; save(); });
});
document.getElementById("dl").addEventListener("click", () => {
  const out = {};
  rows.forEach(tr => {
    const n = nameOf(tr), s = state[n] || {};
    const pick = s.pick || tr.dataset.last;
    if (pick || s.note) out[n] = Object.assign({pick: pick}, s.pick ? {} : {kept: true}, s.note ? {note: s.note} : {});
  });
  const a = document.createElement("a");
  a.href = URL.createObjectURL(new Blob([JSON.stringify(out, null, 1)], {type: "application/json"}));
  a.download = "choices.json"; a.click();
});
count();
function show(f) {
  let n = 0;
  rows.forEach(tr => {
    const flags = tr.dataset.flags.split(" ").filter(x => x);
    const s = state[nameOf(tr)] || {};
    const ok = !f ? true
      : f === "todo" ? flags.includes("new") && !s.pick
      : f === "mine" ? !!s.pick
      : f.startsWith("last:") ? tr.dataset.last === f.slice(5)
      : flags.includes(f);
    tr.style.display = ok ? "" : "none"; n += ok;
  });
  document.getElementById("shown").textContent = n + " shown";
}
document.querySelectorAll("#filters button").forEach(b => b.addEventListener("click", () => {
  document.querySelectorAll("#filters button").forEach(x => x.classList.remove("on"));
  b.classList.add("on"); show(b.dataset.f);
}));
show("new");
</script></body></html>""".replace("@FILTERS@", filters).replace("@ROWS@", "\n".join(body))
    (OUT / "index.html").write_text(html)


INTRO_TAIL = 10.0                    # s mapped after a sound's last write: its voices' releases die away in it


def intro_parts():
    """The intro's sounds, each alone: clips/introNN/ with the AdLib's render and the rich mapping (sndat.py intro)."""
    parts = json.loads((OUT / "introparts-adlib.json").read_text())["windows"]
    for w in parts:
        w["game"] = "intro%02d" % w["game"]
        w["t1"] = (w["writes"][-1][0] if w["writes"] else w["t0"]) + INTRO_TAIL
    PATCH_NOISE.update(measure_patches(parts))
    LINES.update(measure_line_tables(parts))
    rows = []
    for w in parts:
        d = OUT / "clips" / w["game"]
        d.mkdir(parents=True, exist_ok=True)
        m = map_window(w, "rich")
        if m.state["v"] != [15, 15, 15, 15]:
            sys.exit("tandymap: %s still sounds %.0f s after its last write" % (w["game"], INTRO_TAIL))
        # both renders as long as the longer sound: the AdLib's to 2 s after its last write, the mapping to its end
        w["t1"] = max(w["t1"] - INTRO_TAIL + 2.0, max(t for t, _b in m.out))
        length = w["t1"] - w["t0"] + 0.5
        write_opl(d / "adlib.txt", w)
        subprocess.run([str(RENDER), str(d / "adlib.txt"), str(d / "adlib.wav")], check=True)
        write_sn(d / "tandy-rich.txt", w["t0"], length, m.out)
        subprocess.run([str(RENDER), str(d / "tandy-rich.txt"), str(d / "tandy-rich.wav")], check=True)
        r = corr(envelope(wav(d / "adlib.wav")), envelope(wav(d / "tandy-rich.wav")))
        rows.append({"name": w["game"], "driver": w["driver"], "length": length, "writes": len(m.out), "env_corr": r})
        print("%-8s driver %02X %6.1fs  %4d SN writes, env corr %.2f" % (w["game"], w["driver"], length, len(m.out), r),
              flush=True)
    (OUT / "clips-intro.json").write_text(json.dumps(rows, indent=1))


def main(argv=None):
    ap = argparse.ArgumentParser()
    ap.add_argument("cmd", choices=["clips", "intro"])
    ap.add_argument("--only", nargs="*")
    a = ap.parse_args(argv)
    if a.cmd == "intro":
        intro_parts()
    else:
        clips(a.only)


if __name__ == "__main__":
    main()

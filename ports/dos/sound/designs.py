#!/usr/bin/env python3
"""designs.py: game sounds made for the Tandy's chip from scratch, for the Tandy FM study (README.md).

Every game sound but the two pieces of music (the intro, 20) has one: first the sounds heard as "none good" twice
(choices-2026-11-09.json, choices-2026-11-10.json; batch 1, kept in the third pass), then all the rest (batch 2, the
fourth pass). Each is a short script of the chip's own registers, one step a driver tick (72.8 Hz), made from what the
sound is used for in the game and what the AdLib version does (its length, its pitch and where its energy lies).
Such a script is what the game's own Tandy driver plays (PCGAMESN.DAT), so it needs no mapper on the EX.

The techniques are the usual ones for this chip (README.md, "Designed sounds"):
- a volume envelope in software, 2 dB a step, one step a tick at most where a smooth fade is wanted;
- a pitch sweep on a tone channel, and white noise clocked by tone 2 swept with it (explosions, impacts, whooshes);
- a single tick of white noise at the start of a low note for its attack (a kick drum's click);
- periodic noise clocked by tone 2 for a pitch under the chip's 109 Hz floor (a buzz or growl at its own pitch);
- vibrato on the tone register, after the attack; a slight detune on a second channel, or a delayed quieter copy
  (an echo), to thicken a single tone;
- a fast arpeggio over two or three notes to suggest a chord on one channel.
"""
from __future__ import annotations

import math

TICK = 1193182 / 0x555 / 12          # the AdLib driver's tick (tandymap.TICK)
SN_TONE = 3579545 / 32
FLOOR = SN_TONE / 1023


def period(f: float) -> int:
    return max(1, min(1023, round(SN_TONE / f)))


def att(db: float) -> int:
    """The chip's volume for an attenuation in dB (2 dB a step, 15 off)."""
    return max(0, min(15, round(db / 2)))


class Script:
    """A sound as one state of the chip a tick: three tone periods, four volumes, the noise control."""

    def __init__(self, seconds: float):
        self.n = int(seconds * TICK) + 1
        self.p = [[None] * 3 for _ in range(self.n)]
        self.v = [[15] * 4 for _ in range(self.n)]
        self.nz = [None] * self.n

    def ticks(self, t0: float, t1: float):
        return range(max(0, int(t0 * TICK)), min(self.n, int(t1 * TICK) + 1))

    def tone(self, ch, t0, t1, hz, db, vib=0.0, vib_hz=6.0, vib_after=0.08):
        """Tone channel ch from t0 to t1 seconds; hz(x) and db(x) of x, the time into the note (seconds)."""
        for i in self.ticks(t0, t1):
            x = max(0.0, i / TICK - t0)
            f = hz(x)
            if vib and x > vib_after:
                depth = vib * min(1.0, (x - vib_after) / vib_after)
                f *= 2 ** (depth / 1200 * math.sin(2 * math.pi * vib_hz * (x - vib_after)))
            self.p[i][ch] = period(f)
            self.v[i][ch] = att(db(x))

    def noise(self, t0, t1, db, white=True, rate=None, clock_hz=None):
        """The noise channel from t0 to t1: white or periodic, at a fixed rate (0 fast to 2 slow) or clocked by tone 2
        at clock_hz(x) (then tone 2 is silent: it only drives the noise)."""
        for i in self.ticks(t0, t1):
            x = max(0.0, i / TICK - t0)
            if clock_hz is not None:
                self.nz[i] = (4 if white else 0) | 3
                self.p[i][2] = period(clock_hz(x))
                self.v[i][2] = 15
            else:
                self.nz[i] = (4 if white else 0) | rate
            self.v[i][3] = att(db(x))

    def bytes(self):
        """The writes, as tandymap writes them: only what changed."""
        out = []
        sp, sv, sn = [None] * 3, [15] * 4, None
        for i in range(self.n):
            t = i / TICK
            for ch in range(3):
                p = self.p[i][ch]
                if p is not None and p != sp[ch]:
                    out += [(t, 0x80 | ch << 5 | (p & 15)), (t, (p >> 4) & 0x3F)]
                    sp[ch] = p
            if self.nz[i] is not None and self.nz[i] != sn:
                out.append((t, 0xE0 | self.nz[i]))
                sn = self.nz[i]
            for ch in range(4):
                if self.v[i][ch] != sv[ch]:
                    out.append((t, 0x90 | ch << 5 | self.v[i][ch]))
                    sv[ch] = self.v[i][ch]
        for ch in range(4):
            if sv[ch] != 15:
                out.append((self.n / TICK, 0x90 | ch << 5 | 15))
        return out


# envelopes, in dB of attenuation over x seconds into the note
def decay(start, per_s, attack=0.0):
    """Up from silence in `attack` seconds, then down `per_s` dB a second."""
    def f(x):
        if attack and x < attack:
            return start + 30 * (1 - x / attack)
        return start + per_s * (x - attack)
    return f


def hold_fade(start, hold, per_s, attack=0.0):
    def f(x):
        if attack and x < attack:
            return start + 30 * (1 - x / attack)
        return start if x < hold else start + per_s * (x - hold)
    return f


def sweep(f0, f1, secs, curve=1.0):
    """f0 to f1 over secs, exponential in pitch (curve > 1: fast first), then held."""
    def f(x):
        u = min(1.0, x / secs) ** (1 / curve)
        return f0 * (f1 / f0) ** u
    return f


def const(f0):
    return lambda x: f0


def arp(notes, ticks=1):
    """A fast arpeggio: the notes in turn, `ticks` driver ticks each (a chord on one channel)."""
    return lambda x: notes[int(x * TICK / ticks) % len(notes)]


def seq(*steps):
    """Pitch steps: (seconds, hz) pairs, each held until the next."""
    def f(x):
        hz = steps[0][1]
        for t, h in steps:
            if x >= t:
                hz = h
        return hz
    return f


def wobble(f, depth_cents, rate_hz, r=None, jitter=0.0):
    """f with a slow pitch wobble (a waver, a growl's roughness), and random jitter in cents if r is given."""
    return lambda x: f(x) * 2 ** ((depth_cents * math.sin(2 * math.pi * rate_hz * x)
                                   + (jitter * (r() - 0.5) if r else 0.0)) / 1200)


def pulses(times, env):
    """An envelope that restarts `env` at each of `times` (seconds): footsteps, knocks, a rattle."""
    def f(x):
        best = 99.0
        for t in times:
            if x >= t:
                best = env(x - t)
        return best
    return f


class Design:
    def __init__(self, fn, what, use, kind, batch):
        self.fn, self.what, self.use, self.kind, self.batch = fn, what, use, kind, batch


DESIGNS = {}
KINDS = {"steps": "monster footsteps", "attack": "attacks", "hit": "hits and impacts", "death": "deaths",
         "spell": "spells", "magic": "magic places", "door": "doors and mechanisms", "item": "items",
         "party": "the party", "ambient": "ambience"}


def design(*numbers, what="", use="", kind="", batch=2):
    """Register a design for game sounds `numbers`: `what` it sounds like, the `use` it is made for (from the source),
    its `kind` (KINDS) and its `batch` (1: the third pass, kept in its picks; 2: the fourth pass)."""
    assert kind in KINDS, kind

    def reg(fn):
        for g in numbers:
            assert g not in DESIGNS, g
            DESIGNS[g] = Design(fn, what, use, kind, batch)
        return fn
    return reg


class Rand:
    """A small LCG, so a design is the same every run (and an 8086 version can do the same)."""

    def __init__(self, seed):
        self.s = seed

    def __call__(self):
        self.s = (self.s * 1103515245 + 12345) & 0x7FFFFFFF
        return self.s / 0x7FFFFFFF


# Uses from the source (src/D07C8.C monster and spell tables, src/E08E6.C, src/EOV5.C, the level scripts).

@design(15, what="teleporter: a shimmering rise and fall, noise swept with it",
        use="the teleporter (34 level scripts)", kind="magic", batch=1)
def teleport(s):
    s.tone(0, 0.0, 2.2, sweep(220, 880, 1.0), hold_fade(2, 1.0, 20, attack=0.12), vib=35)
    s.tone(1, 0.07, 2.27, lambda x: sweep(220, 880, 1.0)(x) * 1.007, hold_fade(10, 1.0, 20, attack=0.12), vib=35)
    s.noise(0.0, 2.2, hold_fade(14, 0.9, 22, attack=0.3), clock_hz=lambda x: 900 * 2 ** (2 * math.sin(math.pi * min(1, x / 1.6))))


@design(19, what="portal opening: a swelling fifth, an airy noise rising under it",
        use="a portal opening (src/EOV5.C)", kind="magic", batch=1)
def portal_open(s):
    s.tone(0, 0.0, 1.3, sweep(147, 294, 0.9), hold_fade(2, 0.9, 40, attack=0.18), vib=15)
    s.tone(1, 0.0, 1.3, lambda x: sweep(147, 294, 0.9)(x) * 1.5, hold_fade(8, 0.9, 40, attack=0.25), vib=20)
    s.noise(0.0, 1.3, hold_fade(18, 0.8, 40, attack=0.6), white=True, rate=0)


@design(40, what="displacer beast step: a low whump, a click and a falling thud",
        use="displacer beast movement", kind="steps", batch=1)
def whump(s):
    s.noise(0.0, 0.02, const(0), white=True, rate=1)
    s.noise(0.02, 0.5, decay(0, 70), white=False, clock_hz=lambda x: 15 * sweep(90, 32, 0.25, 2)(x))
    s.tone(0, 0.0, 0.3, sweep(170, 110, 0.12, 2), decay(4, 110))


@design(44, what="drow footsteps: two light, quick taps",
        use="drow movement", kind="steps", batch=1)
def drow_steps(s):
    for t0, db in ((0.0, 0), (0.2, 6)):
        s.noise(t0, t0 + 0.08, decay(db, 220), white=True, rate=0)


@design(57, what="leech moving: a soft, wet slither",
        use="leech movement", kind="steps", batch=1)
def leech(s):
    r = Rand(57)
    s.noise(0.0, 0.8, hold_fade(6, 0.4, 50, attack=0.15),
            clock_hz=lambda x: 2200 * 2 ** (0.6 * math.sin(2 * math.pi * 5 * x) + 0.2 * (r() - 0.5)))


@design(69, what="golem step: a heavy stomp",
        use="golem movement", kind="steps", batch=1)
def golem(s):
    s.noise(0.0, 0.03, const(0), white=True, rate=2)
    s.noise(0.03, 0.65, decay(0, 45), white=False, clock_hz=lambda x: 15 * sweep(60, 28, 0.3, 2)(x))
    s.tone(0, 0.0, 0.4, sweep(140, 110, 0.1), decay(2, 70))


@design(71, what="xorn moving through stone: a grinding scrape",
        use="xorn movement", kind="steps", batch=1)
def xorn(s):
    r = Rand(71)
    s.noise(0.0, 0.75, hold_fade(2, 0.45, 60, attack=0.08), clock_hz=lambda x: 350 + 400 * r())
    s.tone(0, 0.0, 0.75, const(110), lambda x: hold_fade(10, 0.45, 60, attack=0.08)(x) + 6 * r())


@design(74, what="zombie shuffle: a dragging scuff, then a smaller one",
        use="zombie movement", kind="steps", batch=1)
def zombie(s):
    s.noise(0.0, 0.45, decay(2, 40, attack=0.1), clock_hz=sweep(1300, 600, 0.45))
    s.noise(0.5, 0.8, decay(10, 60, attack=0.06), clock_hz=sweep(1100, 600, 0.3))


@design(75, what="kobold footsteps: a quick, small patter",
        use="kobold movement", kind="steps", batch=1)
def kobold(s):
    for t0, db in ((0.0, 0), (0.12, 4), (0.24, 2)):
        s.noise(t0, t0 + 0.07, decay(db, 260), white=True, rate=1)


@design(83, what="mind blast (and Xanathar's disintegrate): a high, wavering psychic tone that beats against itself",
        use="the mind flayer's mind blast, Xanathar's disintegrate", kind="attack", batch=1)
def mind_blast(s):
    s.tone(0, 0.0, 0.95, const(2494), hold_fade(2, 0.35, 45, attack=0.05), vib=80, vib_hz=9, vib_after=0.04)
    s.tone(1, 0.0, 0.95, const(2494 * 1.015), hold_fade(6, 0.35, 45, attack=0.05), vib=80, vib_hz=7, vib_after=0.04)
    s.tone(2, 0.0, 0.95, const(1247), hold_fade(12, 0.35, 45, attack=0.1), vib=60, vib_hz=9)
    s.noise(0.0, 0.5, decay(16, 40), white=True, rate=0)


@design(86, what="portal sting: a quick falling chime with an echo",
        use="a portal animation step (src/EOV5.C)", kind="magic", batch=1)
def portal_sting(s):
    s.tone(0, 0.0, 0.7, sweep(1568, 784, 0.3), decay(0, 45))
    s.tone(1, 0.07, 0.77, sweep(1568, 784, 0.3), decay(8, 45))
    s.noise(0.0, 0.15, decay(14, 100), white=True, rate=0)


@design(87, what="Burning Hands: a flaring whoosh of flame that crackles out",
        use="the spell Burning Hands", kind="spell", batch=1)
def burning_hands(s):
    r = Rand(87)
    clock = lambda x: (sweep(600, 3500, 0.2)(x) if x < 0.2 else sweep(3500, 1200, 0.5)(x - 0.2))
    s.noise(0.0, 0.8, lambda x: decay(0, 30, attack=0.06)(x) + (4 * r() if x > 0.25 else 0), clock_hz=clock)


@design(90, what="portal, later: a long swirling hum, two tones beating and noise swirling",
        use="a portal animation, later stage (src/EOV5.C)", kind="magic", batch=1)
def portal_swirl(s):
    s.tone(0, 0.0, 2.6, const(220), hold_fade(4, 1.8, 30, attack=0.4), vib=20, vib_hz=3)
    s.tone(1, 0.0, 2.6, const(222), hold_fade(8, 1.8, 30, attack=0.6), vib=20, vib_hz=4)
    s.noise(0.0, 2.6, hold_fade(14, 1.8, 30, attack=0.5), clock_hz=lambda x: 900 * 2 ** (0.8 * math.sin(2 * math.pi * x / 0.8)))


@design(97, what="Dispel Magic: a chord's shimmer falling away, with an echo",
        use="the spell Dispel Magic", kind="spell", batch=1)
def dispel(s):
    notes = (880, 1109, 1319)
    hz = lambda x: notes[int(x * TICK / 2) % 3] * 0.5 ** min(1.0, x / 1.2)
    s.tone(0, 0.0, 1.6, hz, hold_fade(2, 0.3, 18, attack=0.04))
    s.tone(1, 0.1, 1.7, hz, hold_fade(10, 0.3, 18, attack=0.04))
    s.noise(0.0, 1.0, decay(16, 20), white=True, rate=0)


@design(107, what="Cause Light Wounds: a short dark stab, a falling tone against a semitone",
        use="the spell Cause Light Wounds", kind="spell", batch=1)
def cause_light(s):
    s.noise(0.0, 0.08, decay(4, 150), white=True, rate=1)
    s.tone(0, 0.0, 0.55, sweep(330, 165, 0.4), decay(2, 50), vib=30)
    s.tone(1, 0.0, 0.55, lambda x: sweep(330, 165, 0.4)(x) * 1.06, decay(8, 50), vib=30)


@design(108, 109, what="Cause Serious and Critical Wounds (109 is never played): a heavier stab, a falling tritone over a low growl",
        use="the spells Cause Serious and Critical Wounds, Xanathar's attack (109 is never played)", kind="spell", batch=1)
def cause_serious(s):
    s.noise(0.0, 0.12, decay(2, 100), white=True, rate=2)
    s.tone(0, 0.0, 0.75, sweep(262, 110, 0.55), decay(2, 35), vib=30)
    s.tone(1, 0.0, 0.75, lambda x: sweep(262, 110, 0.55)(x) * 1.414, decay(8, 35), vib=30)
    s.noise(0.12, 0.75, decay(6, 35), white=False, clock_hz=lambda x: 15 * sweep(55, 35, 0.6)(x))


@design(114, what="Remove Paralysis: a bright rising chime that rings with vibrato over soft noise",
        use="the spell Remove Paralysis", kind="spell", batch=1)
def remove_paralysis(s):
    notes = (1105, 1475, 1760)
    hz = lambda x: notes[min(2, int(x / 0.08))]
    s.tone(0, 0.0, 1.0, hz, hold_fade(0, 0.3, 30, attack=0.02), vib=30, vib_after=0.3)
    s.tone(1, 0.08, 1.08, hz, hold_fade(8, 0.3, 30, attack=0.02), vib=30, vib_after=0.3)
    s.noise(0.0, 1.0, hold_fade(16, 0.3, 30, attack=0.1), white=True, rate=0)


# The fourth pass: every other game sound but the two pieces of music (the intro, 20 the character generation music).

@design(21, what="a blow landing: a click, a punch of low noise and a falling thump",
        use="a party member takes damage (src/E0DE8.C:499)", kind="hit")
def hit_party(s):
    s.noise(0.0, 0.015, const(0), white=True, rate=0)
    s.noise(0.015, 0.25, decay(2, 110), white=True, clock_hz=sweep(1800, 500, 0.15))
    s.tone(0, 0.0, 0.22, sweep(220, 110, 0.08, 2), decay(2, 120))


@design(22, what="a dark tolling bell: a low note and its inharmonic partial over a deep growl, fading slowly",
        use="a party member dies (src/E0DE8.C:497)", kind="party")
def death_toll(s):
    s.tone(0, 0.0, 1.6, const(165), decay(0, 16, attack=0.01), vib=8, vib_hz=4, vib_after=0.3)
    s.tone(1, 0.0, 1.0, const(165 * 2.76), decay(6, 28, attack=0.01))
    s.noise(0.0, 1.4, decay(4, 20, attack=0.02), white=False, clock_hz=lambda x: 15 * 41)


@design(30, what="a water drip: a quick rising plink and a faint echo",
        use="ambient noise on levels 1 to 3 (the sewers), at a distance (src/E0ACD.C:968)", kind="ambient")
def drip(s):
    s.tone(0, 0.0, 0.09, sweep(520, 1400, 0.05), decay(2, 160))
    s.tone(1, 0.16, 0.24, sweep(520, 1400, 0.05), decay(14, 160))


@design(79, what="a refusal: two short falling buzzes",
        use="refused: a spell fails or ends, a target is wanted; also the stairs before a level change and some traps (src/E082C.C:615, 71 script ops)", kind="party")
def refused(s):
    for t0, hz in ((0.0, 392), (0.13, 294)):
        s.tone(0, t0, t0 + 0.1, const(hz), hold_fade(2, 0.06, 150))
        s.tone(1, t0, t0 + 0.1, const(hz * 1.01), hold_fade(10, 0.06, 150))


@design(64, what="a ghostly wail: a voice rising and sinking with a wide, slow vibrato, a second voice against it, breath under it",
        use="the skeleton warrior's attack, Xanathar's attack on the whole party", kind="attack")
def wail(s):
    path = lambda x: (sweep(420, 900, 0.5)(x) if x < 0.5 else sweep(900, 300, 1.2)(x - 0.5))
    s.tone(0, 0.0, 1.8, path, hold_fade(0, 0.7, 22, attack=0.15), vib=70, vib_hz=5, vib_after=0.12)
    s.tone(1, 0.05, 1.85, lambda x: path(x) * 1.5, hold_fade(10, 0.7, 22, attack=0.2), vib=70, vib_hz=4, vib_after=0.12)
    s.noise(0.0, 1.6, hold_fade(16, 0.7, 25, attack=0.3), white=True, rate=0)


@design(110, what="a holy ward: a bright major chord swelling in, shimmering with vibrato",
        use="the spell Protection from Evil", kind="spell")
def protection(s):
    for ch, (hz, db, at) in enumerate(((587, 2, 0.25), (740, 6, 0.32), (880, 8, 0.4))):
        s.tone(ch, 0.0, 1.2, const(hz), hold_fade(db, 0.7, 30, attack=at), vib=30, vib_hz=6 + ch, vib_after=0.3)


@design(111, what="a calm healing tone that sinks a little and fades slowly, with a soft echo",
        use="the spell Slow Poison", kind="spell")
def slow_poison(s):
    s.tone(0, 0.0, 1.5, sweep(940, 700, 1.2), decay(2, 14, attack=0.12), vib=20, vib_hz=4, vib_after=0.3)
    s.tone(1, 0.15, 1.65, sweep(940, 700, 1.2), decay(12, 14, attack=0.12), vib=20, vib_hz=4, vib_after=0.3)


@design(115, what="cleansing: a rising sweep with sparkling noise that settles into a pure, wavering tone",
        use="the spell Neutralize Poison", kind="spell")
def neutralize(s):
    s.tone(0, 0.0, 1.2, sweep(440, 880, 0.35), hold_fade(2, 0.6, 30, attack=0.06), vib=35, vib_hz=6, vib_after=0.4)
    s.tone(1, 0.35, 1.2, const(1320), hold_fade(10, 0.25, 30, attack=0.05), vib=35, vib_hz=7, vib_after=0.1)
    s.noise(0.0, 0.5, decay(12, 40, attack=0.1), white=True, clock_hz=sweep(1500, 4000, 0.35))


# building blocks for the fourth pass

def click(s, t0, db=0, rate=0, dur=0.025):
    """One tick of white noise: a click, a tap (rate 0 bright, 2 dull)."""
    s.noise(t0, t0 + dur, const(db), white=True, rate=rate)


def thud(s, t0, db=0, low=55, dur=0.4, ch=0, top=170):
    """A low impact: a dull click, a falling growl under the floor (periodic noise) and a falling tone."""
    click(s, t0, db, rate=2, dur=0.02)
    s.noise(t0 + 0.02, t0 + dur, decay(db, 36 / dur), white=False, clock_hz=lambda x: 15 * sweep(low * 1.6, low, dur / 2, 2)(x))
    s.tone(ch, t0, t0 + dur * 0.7, sweep(top, 110, 0.1, 2), decay(db + 4, 40 / dur))


def swish(s, t0, dur, f0, fpk, f1, db=0):
    """A whoosh: white noise clocked by tone 2, its rate up to fpk and down again, swelling in and out."""
    up = dur * 0.35
    s.noise(t0, t0 + dur, lambda x: db + (24 * (1 - x / up) if x < up else 30 * (x - up) / (dur - up)),
            white=True, clock_hz=lambda x: sweep(f0, fpk, up)(x) if x < up else sweep(fpk, f1, dur - up)(x - up))


def ping(s, ch, t0, hz, db=0, per_s=60, dur=None, vib=0.0):
    s.tone(ch, t0, t0 + (dur or 30 / per_s), const(hz), decay(db, per_s), vib=vib)


def chime(s, notes, gap, db=0, per_s=18, dur=1.2, vib=20, echo=None):
    """Notes struck one after another, each ringing on its own channel (at most three)."""
    for ch, hz in enumerate(notes):
        s.tone(ch, gap * ch, gap * ch + dur, const(hz), decay(db + 2 * ch, per_s, attack=0.01), vib=vib, vib_after=0.2)


def growl(s, t0, t1, hz, db, r, per_s=0.0, attack=0.04):
    """A growl under the floor: periodic noise at hz, its pitch roughened by r."""
    s.noise(t0, t1, decay(db, per_s, attack=attack), white=False, clock_hz=lambda x: 15 * hz * 2 ** ((r() - 0.5) * 0.4))


# the doors and mechanisms (level scripts: the switch, plate and lock ops; src/E1414.C, src/E122E.C)

@design(1, what="a switch clicking home: a bright click and a short wooden knock",
        use="a wall switch is clicked, an item set in a socket (49 script ops)", kind="door")
def switch_click(s):
    click(s, 0.0, 0, rate=0)
    s.tone(0, 0.02, 0.12, sweep(260, 180, 0.05), decay(2, 160))
    click(s, 0.07, 6, rate=1)


@design(3, what="a door grinding open: a rough scrape that rises a little",
        use="a door opening, each step of its animation (src/E1414.C:320)", kind="door")
def door_open(s):
    r = Rand(3)
    s.noise(0.0, 0.3, lambda x: hold_fade(2, 0.18, 80, attack=0.04)(x) + 4 * r(), clock_hz=lambda x: sweep(300, 480, 0.3)(x) * (0.8 + 0.4 * r()))
    s.tone(0, 0.0, 0.3, const(110), lambda x: hold_fade(12, 0.18, 80, attack=0.04)(x) + 6 * r())


@design(4, what="a door grinding shut: the scrape falling",
        use="a door closing, each step of its animation (src/E1414.C:318)", kind="door")
def door_close(s):
    r = Rand(4)
    s.noise(0.0, 0.3, lambda x: hold_fade(2, 0.18, 80, attack=0.04)(x) + 4 * r(), clock_hz=lambda x: sweep(480, 300, 0.3)(x) * (0.8 + 0.4 * r()))
    s.tone(0, 0.0, 0.3, const(110), lambda x: hold_fade(12, 0.18, 80, attack=0.04)(x) + 6 * r())


@design(5, what="a heavy door slamming home: a deep thud and a short rattle",
        use="a door reaching closed (src/E1414.C:316); the last beat of the ending", kind="door")
def door_shut(s):
    thud(s, 0.0, 0, low=45, dur=0.8, top=150)
    s.tone(1, 0.04, 0.3, const(392), lambda x: 14 + 60 * x + 6 * (int(x * TICK) % 2))


@design(6, what="a button pressed: a firm click and a small clack",
        use="the button beside a door (src/E122E.C:386)", kind="door")
def door_button(s):
    click(s, 0.0, 0, rate=0)
    ping(s, 0, 0.0, 880, 4, per_s=300)
    ping(s, 1, 0.05, 440, 6, per_s=250)


@design(7, what="a lever thrown: a quick ratchet of clicks and a clunk",
        use="a wall lever or switch is pulled, or hit by a thrown item (63 script ops)", kind="door")
def lever(s):
    for i, t0 in enumerate((0.0, 0.04, 0.08)):
        click(s, t0, 4 + 2 * i, rate=1, dur=0.015)
    click(s, 0.13, 0, rate=2)
    s.tone(0, 0.13, 0.35, sweep(200, 130, 0.06), decay(0, 90))


@design(8, what="a puzzle solved: a rising four-note chime with an echo and a sparkle",
        use="a puzzle solved: an item set in a niche, the spheres (29 script ops, mostly level 6)", kind="magic")
def solved(s):
    hz = seq((0, 523), (0.09, 659), (0.18, 784), (0.27, 1047))
    s.tone(0, 0.0, 1.1, hz, hold_fade(2, 0.35, 30, attack=0.01), vib=20, vib_after=0.4)
    s.tone(1, 0.12, 1.2, hz, hold_fade(10, 0.35, 30, attack=0.01), vib=20, vib_after=0.4)
    s.noise(0.25, 0.9, decay(16, 30, attack=0.1), white=True, rate=0)


@design(12, what="a floor plate sinking under a foot: a dull click and a short stone knock",
        use="stepping on or off a pressure plate (86 script ops)", kind="door")
def plate(s):
    click(s, 0.0, 2, rate=2, dur=0.03)
    s.tone(0, 0.0, 0.15, sweep(165, 120, 0.05), decay(2, 150))
    click(s, 0.09, 10, rate=2, dur=0.02)


@design(13, what="a stone wall sliding away: a long, juddering grind that slows and stops",
        use="a wall changes: secret walls, walls that vanish when a puzzle is solved (84 script ops)", kind="door")
def wall_slide(s):
    r = Rand(13)
    jud = lambda x: 5 * (0.5 + 0.5 * math.sin(2 * math.pi * sweep(9, 5, 1.0)(x) * x))
    s.noise(0.0, 1.1, lambda x: hold_fade(2, 0.75, 40, attack=0.1)(x) + jud(x) + 2 * r(), clock_hz=lambda x: 380 * (0.85 + 0.3 * r()))
    s.tone(0, 0.0, 1.1, const(112), lambda x: hold_fade(8, 0.75, 40, attack=0.1)(x) + jud(x))
    s.tone(1, 0.95, 1.25, const(110), decay(6, 50))


@design(14, what="a key turning: two metal clicks and the bolt springing back",
        use="a key or lock pick works, an item fits its slot (61 script ops)", kind="door")
def key_turn(s):
    click(s, 0.0, 6, rate=0, dur=0.015)
    ping(s, 0, 0.0, 2400, 6, per_s=400)
    click(s, 0.12, 4, rate=0, dur=0.015)
    ping(s, 1, 0.12, 1800, 4, per_s=350)
    click(s, 0.24, 0, rate=1, dur=0.03)
    s.tone(0, 0.24, 0.45, sweep(330, 220, 0.05), decay(0, 100))


@design(17, what="a trapdoor dropping open: a chain rattle, a creak and a hollow thud",
        use="a pit or trapdoor opens (67 script ops)", kind="door")
def trapdoor(s):
    for i in range(5):
        click(s, 0.03 * i, 6 + 2 * i, rate=0, dur=0.015)
    s.tone(1, 0.0, 0.25, wobble(sweep(330, 220, 0.25), 40, 18), hold_fade(8, 0.12, 60))
    thud(s, 0.25, 0, low=50, dur=0.55, top=160)


@design(25, what="a trap springing: a sharp snap and a spring's twang",
        use="a dart trap fires after a pressure plate (levels 6, 8, 9)", kind="door")
def trap_snap(s):
    click(s, 0.0, 0, rate=0, dur=0.03)
    s.tone(0, 0.0, 0.35, sweep(700, 330, 0.15, 2), decay(2, 60), vib=60, vib_hz=18, vib_after=0.02)
    s.tone(1, 0.0, 0.25, sweep(1400, 660, 0.15, 2), decay(10, 80))


@design(27, what="a heavy lever: a slow ratchet and a deep clunk",
        use="two wall switches on level 4 (a quest switch)", kind="door")
def heavy_lever(s):
    for i, t0 in enumerate((0.0, 0.07, 0.14)):
        click(s, t0, 2 + 2 * i, rate=2, dur=0.02)
    thud(s, 0.2, 2, low=60, dur=0.35, top=180)


@design(34, what="a gem locking into place: a crystal clink and a solid knock under it",
        use="a gem set into a socket (level 3); each frame of Xanathar's death", kind="door")
def gem(s):
    ping(s, 0, 0.0, 1760, 0, per_s=50, vib=15)
    ping(s, 1, 0.0, 2637, 6, per_s=70)
    s.noise(0.0, 0.02, const(6), white=True, rate=0)
    s.tone(2, 0.03, 0.2, sweep(196, 130, 0.05), decay(4, 120))


@design(81, what="a key that will not turn: a nervous rattle of small metal clicks",
        use="a lock fails: \"it doesn't fit\", a failed pick (81 script ops)", kind="door")
def lock_fail(s):
    for i, t0 in enumerate((0.0, 0.05, 0.1, 0.17, 0.22)):
        click(s, t0, 4 + i, rate=0, dur=0.015)
        ping(s, i % 2, t0, 2000 + 300 * (i % 2), 10 + i, per_s=500)


@design(82, what="a lock pick snapping: a sharp crack and a falling ping",
        use="the lock picks break (39 script ops)", kind="door")
def pick_snap(s):
    click(s, 0.0, 0, rate=0, dur=0.03)
    s.tone(0, 0.0, 0.3, sweep(3000, 1500, 0.08), decay(2, 90))
    s.tone(1, 0.15, 0.3, const(1200), decay(10, 150))


# items and the party (src/E0ACD.C, src/E0BD2.C, src/E0DE8.C, src/E1414.C)

@design(9, what="eating: two crunchy bites",
        use="eating food (src/E0ACD.C:636)", kind="item")
def munch(s):
    r = Rand(9)
    for t0 in (0.0, 0.2):
        s.noise(t0, t0 + 0.13, lambda x: decay(0, 140)(x) + 4 * r(), white=True, clock_hz=lambda x: 1400 * (0.6 + 0.8 * r()))


@design(10, what="drinking: two gulps, each a bubble rising",
        use="drinking a potion (src/E0BD2.C:720)", kind="item")
def gulp(s):
    for t0, db in ((0.0, 0), (0.18, 2)):
        s.tone(0, t0, t0 + 0.11, sweep(260, 520, 0.08), lambda x, d=db: d + (12 * (1 - x / 0.02) if x < 0.02 else 80 * (x - 0.02)))
    s.noise(0.0, 0.3, const(20), white=True, rate=2)


@design(11, what="a throw: a fast whoosh, rising and falling away",
        use="throwing a dagger, spear, dart or rock, a sling, a dart trap's dart (src/E0BD2.C:522)", kind="item")
def throw(s):
    swish(s, 0.0, 0.32, 800, 4000, 1200, 0)


@design(18, 78, what="something landing: a clatter of three small knocks and a tick of metal (78 is never played)",
        use="a thrown item or missile comes to rest (src/E1414.C:220)", kind="item")
def clatter(s):
    for i, t0 in enumerate((0.0, 0.06, 0.1)):
        click(s, t0, 2 + 4 * i, rate=1, dur=0.02)
    ping(s, 0, 0.0, 1250, 6, per_s=200)


@design(26, what="a bowstring: a snap, a low twang and the arrow's hiss",
        use="firing a bow (src/E0DE8.C:378)", kind="item")
def bow(s):
    click(s, 0.0, 2, rate=0, dur=0.015)
    s.tone(0, 0.0, 0.35, sweep(260, 220, 0.1), decay(0, 70), vib=40, vib_hz=14, vib_after=0.02)
    s.tone(1, 0.0, 0.2, const(520), decay(6, 120))
    s.noise(0.03, 0.3, decay(10, 80, attack=0.03), white=True, rate=0)


@design(32, what="a weapon swing: a short, heavy swish",
        use="every melee swing, hit or miss (src/E0DE8.C:412)", kind="item")
def weapon_swing(s):
    swish(s, 0.0, 0.2, 700, 2600, 900, 0)


@design(23, what="a level gained: a quick rising fanfare that lands on a ringing octave",
        use="a party member gains a level (src/E0DE8.C:604)", kind="party")
def level_up(s):
    hz = seq((0, 392), (0.08, 523), (0.16, 659), (0.24, 784))
    s.tone(0, 0.0, 0.32, hz, const(2))
    s.tone(0, 0.32, 1.1, const(1047), decay(0, 20), vib=25, vib_after=0.12)
    s.tone(1, 0.32, 1.1, const(784), decay(6, 20), vib=25, vib_after=0.12)
    s.tone(2, 0.32, 1.1, const(523), decay(8, 20))


@design(28, what="a quest fulfilled: a short fanfare of three chords, the last held",
        use="the level's special quest is completed (12 script ops)", kind="party")
def quest(s):
    chords = ((0.0, (523, 659, 784)), (0.16, (587, 698, 880)), (0.32, (659, 784, 1047)))
    for t0, notes in chords:
        last = t0 == 0.32
        for ch, hz in enumerate(notes):
            s.tone(ch, t0, t0 + (1.2 if last else 0.13), const(hz), (decay(2 * ch, 14) if last else const(2 + 2 * ch)),
                   vib=(20 if last else 0), vib_after=0.2)


@design(29, what="a bump: a soft, dull knock",
        use="walking into a wall, stairs that cannot be used (src/E0ACD.C:183)", kind="party")
def bump(s):
    click(s, 0.0, 6, rate=2, dur=0.02)
    s.tone(0, 0.0, 0.15, sweep(150, 110, 0.05), decay(2, 160))


@design(76, what="a panel button: a tiny click",
        use="a button pressed in the character panel and in character creation (src/EOV0.C:280)", kind="party")
def ui_click(s):
    click(s, 0.0, 8, rate=0, dur=0.015)
    ping(s, 0, 0.0, 1050, 4, per_s=500)


# monster footsteps: the movement sound of each type (src/D07C8.C:525, field +17h), played at its cell

@design(36, what="a mantis skittering: two quick runs of light, dry ticks",
        use="mantis movement", kind="steps")
def mantis_steps(s):
    for i, t0 in enumerate((0.0, 0.035, 0.07, 0.25, 0.285, 0.32)):
        click(s, t0, 2 + 2 * (i % 3), rate=0, dur=0.012)


@design(38, what="a spider scuttling: an uneven patter of tiny ticks",
        use="spider movement", kind="steps")
def spider_steps(s):
    r = Rand(38)
    t = 0.0
    for i in range(9):
        click(s, t, 4 + 4 * r(), rate=0, dur=0.012)
        t += 0.025 + 0.035 * r()


@design(42, what="a drider: eight legs clattering on stone, heavier than a spider",
        use="drider movement; the last frames of the ending", kind="steps")
def drider_steps(s):
    r = Rand(42)
    for i in range(8):
        t0 = 0.045 * i + 0.01 * r()
        click(s, t0, 2 + 3 * r(), rate=1, dur=0.02)
        ping(s, i % 2, t0, 600 + 150 * (i % 2), 12, per_s=400)


@design(46, what="a dwarf's heavy boots: two firm steps",
        use="dwarf movement", kind="steps")
def dwarf_steps(s):
    for t0, db in ((0.0, 0), (0.28, 4)):
        click(s, t0, db, rate=2, dur=0.025)
        s.tone(0, t0, t0 + 0.14, sweep(170, 115, 0.05), decay(db + 2, 150))


@design(48, what="a flind padding: two soft, heavy paws",
        use="flind movement", kind="steps")
def flind_steps(s):
    for t0, db in ((0.0, 2), (0.3, 6)):
        s.noise(t0, t0 + 0.12, decay(db, 130, attack=0.02), white=True, clock_hz=const(500))
        s.tone(0, t0, t0 + 0.1, const(120), decay(db + 6, 160))


@design(50, what="a hellhound: two quick paw falls and a hot, panting breath",
        use="hellhound movement", kind="steps")
def hound_steps(s):
    for t0 in (0.0, 0.12):
        click(s, t0, 4, rate=2, dur=0.02)
    for t0 in (0.22, 0.4):
        s.noise(t0, t0 + 0.14, decay(8, 90, attack=0.04), white=True, clock_hz=sweep(1500, 900, 0.14))


@design(53, what="a kenku: a short flutter of wings",
        use="kenku movement", kind="steps")
def kenku_steps(s):
    s.noise(0.0, 0.42, lambda x: hold_fade(4, 0.25, 70)(x) + 9 * (0.5 + 0.5 * math.cos(2 * math.pi * 14 * x)),
            white=True, clock_hz=lambda x: 900 + 300 * math.sin(2 * math.pi * 14 * x))


@design(55, what="a kuo-toa: two wet slaps",
        use="kuo-toa movement", kind="steps")
def kuotoa_steps(s):
    for t0, db in ((0.0, 0), (0.26, 4)):
        s.noise(t0, t0 + 0.09, decay(db, 160), white=True, clock_hz=sweep(2600, 800, 0.08))
        s.tone(0, t0, t0 + 0.07, sweep(320, 200, 0.06), decay(db + 6, 200))


@design(59, what="a mind flayer gliding: the soft swish of a robe",
        use="mind flayer movement", kind="steps")
def flayer_steps(s):
    s.noise(0.0, 0.5, lambda x: (6 + 24 * (1 - x / 0.2)) if x < 0.2 else 6 + 60 * (x - 0.2), white=True,
            clock_hz=lambda x: 1300 * 2 ** (0.4 * math.sin(2 * math.pi * 2 * x)))


@design(61, what="a rust monster: a quick scuttle of hard little feet and a twitch of feelers",
        use="rust monster movement", kind="steps")
def rust_steps(s):
    for i, t0 in enumerate((0.0, 0.05, 0.1, 0.15)):
        click(s, t0, 4 + 2 * (i % 2), rate=1, dur=0.015)
    ping(s, 0, 0.2, 2600, 12, per_s=250, vib=80)


@design(63, what="a shindia: two light, soft cat steps",
        use="shindia movement", kind="steps")
def shindia_steps(s):
    for t0, db in ((0.0, 8), (0.22, 10)):
        s.noise(t0, t0 + 0.08, decay(db, 200, attack=0.015), white=True, clock_hz=const(700))


@design(65, what="a skeleton warrior: bones and armour rattling, a ring of metal",
        use="skeleton warrior movement", kind="steps")
def skel_warrior_steps(s):
    r = Rand(65)
    for i in range(5):
        click(s, 0.04 * i + 0.01 * r(), 2 + 3 * r(), rate=0, dur=0.015)
    ping(s, 0, 0.0, 1480, 6, per_s=40)
    ping(s, 1, 0.08, 1480 * 1.012, 12, per_s=40)


@design(67, what="a skeleton: a dry clatter of bones",
        use="skeleton movement", kind="steps")
def skel_steps(s):
    r = Rand(67)
    for i in range(5):
        t0 = 0.035 * i + 0.01 * r()
        click(s, t0, 2 + 3 * r(), rate=0, dur=0.012)
        ping(s, i % 2, t0, (1200, 900)[i % 2], 6, per_s=500)


# monster attacks: field +16h of the monster type table, played at the party's cell

@design(2, what="a kobold's stab: a little swish and a sharp yip",
        use="kobold attack", kind="attack")
def kobold_attack(s):
    swish(s, 0.0, 0.12, 900, 3000, 1500, 4)
    s.tone(0, 0.1, 0.25, lambda x: 900 * 2 ** (0.6 * math.sin(math.pi * min(1, x / 0.12))), decay(2, 80))


@design(31, what="a lightning bolt: a crackling, flickering zap that jumps about in pitch",
        use="the spell Lightning Bolt; the mage monster's bolt and melee; the kuo-toa's spell; the portal's crackle", kind="spell")
def lightning(s):
    r = Rand(31)
    s.noise(0.0, 0.6, lambda x: decay(0, 30)(x) + 8 * r(), white=True, clock_hz=lambda x: 800 + 4200 * r())
    s.tone(0, 0.0, 0.45, lambda x: 1500 + 2500 * r(), lambda x: decay(6, 40)(x) + 10 * r())


@design(37, what="a mantis strike: a shrill, trilling insect screech",
        use="mantis attack; the level 10 ambush", kind="attack")
def mantis_attack(s):
    s.tone(0, 0.0, 0.5, sweep(2400, 2900, 0.15), hold_fade(2, 0.25, 60, attack=0.02), vib=150, vib_hz=24, vib_after=0.01)
    s.tone(1, 0.0, 0.5, sweep(2400, 2900, 0.15), hold_fade(10, 0.25, 60, attack=0.02), vib=150, vib_hz=19, vib_after=0.01)
    s.noise(0.0, 0.3, decay(12, 60), white=True, rate=0)


@design(39, what="a tentacle lash: a whip crack and a falling hiss",
        use="displacer beast attack", kind="attack")
def tentacle(s):
    s.noise(0.0, 0.35, lambda x: (24 * (1 - x / 0.04)) if x < 0.04 else 60 * (x - 0.04), white=True,
            clock_hz=lambda x: sweep(600, 5000, 0.04)(x) if x < 0.04 else sweep(5000, 1500, 0.3)(x - 0.04))
    s.tone(0, 0.04, 0.3, sweep(1300, 300, 0.2), decay(4, 90))


@design(41, what="a drider's attack: a slash and an angry chitter",
        use="drider attack", kind="attack")
def drider_attack(s):
    swish(s, 0.0, 0.16, 900, 3200, 1200, 0)
    s.tone(0, 0.12, 0.42, const(1500), lambda x: 4 + 40 * x + (10 if int(x * TICK) % 2 else 0), vib=60, vib_hz=12)


@design(43, what="a drow blade: a quick metal swish and a fine ring",
        use="drow attack", kind="attack")
def drow_attack(s):
    swish(s, 0.0, 0.14, 1500, 4500, 2500, 2)
    ping(s, 0, 0.1, 2640, 2, per_s=45)
    ping(s, 1, 0.16, 2640 * 1.01, 10, per_s=45)


@design(45, what="a dwarf's axe: a heavy swish and a chopping impact",
        use="dwarf attack", kind="attack")
def dwarf_attack(s):
    swish(s, 0.0, 0.16, 500, 1800, 700, 2)
    thud(s, 0.15, 0, low=70, dur=0.3, top=220)


@design(47, what="a flind's snarl and strike: a rough growl, then a blow",
        use="flind attack; a level 3 script that wakes the flinds", kind="attack")
def flind_attack(s):
    r = Rand(47)
    growl(s, 0.0, 0.3, 75, 2, r, per_s=10)
    s.tone(0, 0.0, 0.3, wobble(const(150), 60, 9), hold_fade(8, 0.2, 60))
    thud(s, 0.3, 2, low=60, dur=0.25, ch=1, top=200)


@design(49, what="a spider's bite: two clicking fangs and a venomous hiss",
        use="spider attack", kind="attack")
def spider_attack(s):
    click(s, 0.0, 2, rate=0, dur=0.015)
    click(s, 0.05, 2, rate=0, dur=0.015)
    s.noise(0.1, 0.45, decay(2, 60, attack=0.03), white=True, clock_hz=sweep(5000, 3500, 0.3))


@design(51, what="a hellhound: a snarling growl with a breath of fire",
        use="hellhound attack", kind="attack")
def hound_attack(s):
    r = Rand(51)
    s.tone(0, 0.0, 0.7, wobble(sweep(180, 130, 0.6), 80, 11), hold_fade(2, 0.35, 40, attack=0.05))
    s.tone(1, 0.0, 0.7, wobble(sweep(270, 195, 0.6), 80, 13), hold_fade(10, 0.35, 40, attack=0.05))
    s.noise(0.15, 0.85, lambda x: decay(4, 30, attack=0.1)(x) + 4 * r(), white=True, clock_hz=sweep(800, 2400, 0.3))


@design(52, what="a kenku: two harsh squawks",
        use="kenku attack", kind="attack")
def kenku_attack(s):
    for t0, k in ((0.0, 1.0), (0.18, 0.9)):
        s.tone(0, t0, t0 + 0.14, lambda x, k=k: k * 1100 * 2 ** (0.5 * math.sin(math.pi * min(1, x / 0.14))),
               decay(0, 60), vib=90, vib_hz=30, vib_after=0.01)
        s.noise(t0, t0 + 0.1, decay(8, 100), white=True, rate=0)


@design(54, what="a kuo-toa: a croaking, gulping strike",
        use="kuo-toa attack; level 3 scripts that wake them; two frames of the ending", kind="attack")
def kuotoa_attack(s):
    s.noise(0.0, 0.28, lambda x: 2 + 12 * (int(x * 25) % 2), white=False, clock_hz=const(15 * 90))
    s.tone(0, 0.0, 0.28, sweep(260, 180, 0.28), lambda x: 8 + 12 * (int(x * 25) % 2))
    swish(s, 0.28, 0.16, 800, 2500, 1000, 2)


@design(56, what="a leech's bite: a wet, sucking slurp falling away",
        use="leech attack", kind="attack")
def leech_attack(s):
    s.noise(0.0, 0.45, decay(2, 40, attack=0.05), white=True, clock_hz=lambda x: sweep(3000, 600, 0.4)(x) * 2 ** (0.3 * math.sin(2 * math.pi * 12 * x)))
    s.tone(0, 0.0, 0.4, sweep(800, 260, 0.35), decay(6, 40, attack=0.05), vib=80, vib_hz=12)


@design(58, what="a mind flayer's grasp: a slithering squelch over a low, wavering moan",
        use="mind flayer attack", kind="attack")
def flayer_attack(s):
    s.tone(0, 0.0, 0.6, const(196), hold_fade(4, 0.3, 40, attack=0.06), vib=120, vib_hz=7, vib_after=0.02)
    s.tone(1, 0.0, 0.6, const(207), hold_fade(10, 0.3, 40, attack=0.06), vib=120, vib_hz=5, vib_after=0.02)
    s.noise(0.0, 0.5, decay(4, 40, attack=0.04), white=True, clock_hz=lambda x: 1500 * 2 ** (0.8 * math.sin(2 * math.pi * 9 * x)))


@design(60, what="a rust monster's touch: metal fizzing and corroding, a ringing tone sinking",
        use="rust monster attack", kind="attack")
def rust_attack(s):
    r = Rand(60)
    s.noise(0.0, 0.55, lambda x: decay(2, 30)(x) + 6 * r(), white=True, rate=0)
    s.tone(0, 0.0, 0.55, wobble(sweep(1900, 1300, 0.5), 60, 13), decay(4, 35))


@design(62, what="a shindia's claws: a yowl and a fast double scratch",
        use="shindia attack", kind="attack")
def shindia_attack(s):
    s.tone(0, 0.0, 0.3, lambda x: 700 * 2 ** (0.7 * math.sin(math.pi * min(1, x / 0.3))), decay(2, 40), vib=70, vib_hz=9, vib_after=0.04)
    for t0 in (0.22, 0.32):
        s.noise(t0, t0 + 0.08, decay(0, 150), white=True, clock_hz=sweep(2500, 4500, 0.08))


@design(66, what="a skeleton's chop: a dry swish and a bony clack",
        use="skeleton attack", kind="attack")
def skel_attack(s):
    swish(s, 0.0, 0.14, 800, 2800, 1200, 4)
    click(s, 0.14, 0, rate=0, dur=0.02)
    ping(s, 0, 0.14, 1300, 2, per_s=250)
    ping(s, 1, 0.14, 950, 6, per_s=250)


@design(68, what="a golem's fist: a huge, crushing blow that shakes the floor",
        use="golem attack", kind="attack")
def golem_attack(s):
    swish(s, 0.0, 0.14, 300, 1200, 400, 8)
    thud(s, 0.13, 0, low=35, dur=0.75, top=150)
    s.tone(1, 0.13, 0.6, const(110), decay(2, 40))


@design(72, what="a xorn's jaws: three grinding crunches",
        use="xorn attack; a frame of the ending", kind="attack")
def xorn_attack(s):
    r = Rand(72)
    for t0 in (0.0, 0.15, 0.3):
        s.noise(t0, t0 + 0.12, lambda x: decay(0, 120)(x) + 5 * r(), white=True, clock_hz=lambda x: 500 * (0.6 + 0.8 * r()))
        s.tone(0, t0, t0 + 0.1, const(110), lambda x: decay(6, 150)(x) + 4 * r())


@design(73, what="a zombie's claw: a low groan and a slow, heavy blow",
        use="zombie attack", kind="attack")
def zombie_attack(s):
    s.tone(0, 0.0, 0.4, wobble(sweep(190, 150, 0.4), 50, 5), hold_fade(4, 0.2, 40, attack=0.08))
    s.tone(1, 0.0, 0.4, wobble(sweep(200, 158, 0.4), 50, 4), hold_fade(10, 0.2, 40, attack=0.08))
    thud(s, 0.38, 2, low=55, dur=0.35, ch=0, top=170)


# spells: the sound byte of the spell table (src/D07C8.C:394-450), played by CastSpell

@design(85, what="a magic missile: a zippy rising streak with an echo, wavering as it flies",
        use="the spell Magic Missile; the mage monster's and the kenku's missiles", kind="spell")
def magic_missile(s):
    hz = lambda x: sweep(400, 2000, 0.12)(x) if x < 0.12 else sweep(2000, 1500, 0.4)(x - 0.12)
    s.tone(0, 0.0, 0.55, hz, decay(0, 30), vib=60, vib_hz=12, vib_after=0.12)
    s.tone(1, 0.08, 0.63, hz, decay(10, 30), vib=60, vib_hz=12, vib_after=0.12)
    s.noise(0.0, 0.15, decay(12, 80), white=True, rate=0)


@design(88, what="a shocking grasp: an electric buzz in the hand, crackling",
        use="the spell Shocking Grasp", kind="spell")
def shocking(s):
    r = Rand(88)
    s.noise(0.0, 0.6, lambda x: decay(2, 30, attack=0.02)(x) + (10 if r() < 0.3 else 0), white=False, clock_hz=lambda x: 15 * 60)
    s.tone(0, 0.0, 0.6, lambda x: 2000 + 2500 * r(), lambda x: decay(8, 30)(x) + (0 if r() < 0.4 else 14))


@design(89, what="an ice storm: a hail of sharp ticks and glassy tinkles over a cold wind",
        use="the spell Ice Storm", kind="spell")
def ice_storm(s):
    r = Rand(89)
    hits = [0.0]
    while hits[-1] < 0.9:
        hits.append(hits[-1] + 0.03 + 0.07 * r())
    s.noise(0.0, 1.1, lambda x: (0 if any(0 <= x - t < 0.014 for t in hits) else 16) + 30 * max(0, x - 0.8), white=True, rate=0)
    for i, t in enumerate(hits[:12]):
        ping(s, i % 2, t, 2500 + 1600 * r(), 6 + 4 * (i % 3), per_s=250)


@design(91, what="a blessing: three high bell notes struck in turn, ringing together with vibrato",
        use="the spells Bless, Aid and Prayer", kind="spell")
def bless(s):
    chime(s, (784, 988, 1175), 0.1, db=0, per_s=16, dur=1.3, vib=20)


@design(92, what="a ward forming: two tones sliding up into a fifth and ending on a metallic ping",
        use="the spells Armor and Shield", kind="spell")
def ward(s):
    s.tone(0, 0.0, 0.8, sweep(220, 440, 0.3), hold_fade(2, 0.35, 40, attack=0.05), vib=25, vib_after=0.3)
    s.tone(1, 0.0, 0.8, sweep(330, 660, 0.3), hold_fade(8, 0.35, 40, attack=0.05), vib=25, vib_after=0.3)
    ping(s, 2, 0.3, 1760, 4, per_s=40)


@design(93, what="stoneskin: a low grind that hardens and stops with a solid knock",
        use="the spell Stoneskin", kind="spell")
def stoneskin(s):
    r = Rand(93)
    s.noise(0.0, 0.7, lambda x: hold_fade(4, 0.4, 40, attack=0.15)(x) + 3 * r(), clock_hz=lambda x: sweep(600, 250, 0.7)(x) * (0.85 + 0.3 * r()))
    s.tone(0, 0.0, 0.7, sweep(330, 165, 0.7), hold_fade(8, 0.4, 40, attack=0.15))
    s.tone(1, 0.7, 0.95, sweep(180, 120, 0.05), decay(0, 100))


@design(94, what="invisibility: a high, shimmering glide downward that thins away to nothing",
        use="the spells Invisibility and Invisibility 10'", kind="spell")
def invisibility(s):
    s.tone(0, 0.0, 1.3, sweep(1760, 880, 1.2), lambda x: decay(2, 22, attack=0.1)(x), vib=40, vib_hz=7, vib_after=0.1)
    s.tone(1, 0.0, 1.3, lambda x: sweep(1760, 880, 1.2)(x) * 1.012, decay(6, 22, attack=0.1), vib=40, vib_hz=5, vib_after=0.1)
    s.noise(0.0, 0.9, decay(14, 25, attack=0.2), white=True, rate=0)


@design(95, what="a revealing shimmer: a fast high arpeggio with an echo and a sparkle",
        use="the spells Detect Magic; Turn Undead succeeding (src/E0D1C.C:669)", kind="spell")
def detect(s):
    hz = arp((1319, 1568, 1976), 2)
    s.tone(0, 0.0, 1.0, hz, hold_fade(2, 0.4, 30, attack=0.05))
    s.tone(1, 0.09, 1.09, hz, hold_fade(10, 0.4, 30, attack=0.05))
    s.noise(0.0, 0.8, decay(14, 25, attack=0.1), white=True, rate=0)


@design(96, what="an acid arrow: a launching zip and a long, hissing fizz",
        use="the spell Melf's Acid Arrow", kind="spell")
def acid_arrow(s):
    r = Rand(96)
    s.tone(0, 0.0, 0.15, sweep(600, 1600, 0.12), decay(0, 60))
    s.noise(0.1, 0.9, lambda x: decay(2, 30, attack=0.05)(x) + 5 * r(), white=True, clock_hz=lambda x: 3800 * (0.8 + 0.4 * r()))


@design(98, what="a fireball: a rising whoosh, then a roaring blast that rumbles away",
        use="the spells Fireball and Flame Strike; a wand; Xanathar's and the mage monster's fireballs; the level 7 fire traps", kind="spell")
def fireball(s):
    r = Rand(98)
    clock = lambda x: sweep(400, 2400, 0.22)(x) if x < 0.22 else sweep(2400, 300, 1.0)(x - 0.22) * (0.85 + 0.3 * r())
    s.noise(0.0, 1.4, lambda x: (10 * (1 - x / 0.22)) if x < 0.22 else 25 * (x - 0.22), white=True, clock_hz=clock)
    s.tone(0, 0.22, 1.0, sweep(160, 110, 0.3), decay(2, 40))
    s.tone(1, 0.22, 0.8, wobble(const(116), 80, 17), decay(6, 40))


@design(99, what="a flame flaring up: a bright ignition and a short crackling burn",
        use="the spells Flame Arrow and Flame Blade", kind="spell")
def flame(s):
    r = Rand(99)
    clock = lambda x: sweep(1000, 4500, 0.08)(x) if x < 0.08 else 2200 * (0.6 + 0.8 * r())
    s.noise(0.0, 0.7, lambda x: decay(0, 35)(x) + (6 * r() if x > 0.1 else 0), white=True, clock_hz=clock)
    s.tone(0, 0.0, 0.3, sweep(440, 1320, 0.08), decay(6, 60))


@design(100, what="haste: blips that rise and speed up",
        use="the spell Haste", kind="spell")
def haste(s):
    t, hz, i = 0.0, 440.0, 0
    while t < 0.8:
        ping(s, i % 2, t, hz, 2 + 4 * (i % 2), per_s=300, dur=0.05)
        t += max(0.03, 0.12 * 0.85 ** i)
        hz *= 1.07
        i += 1
    s.tone(0, t, t + 0.35, const(hz), decay(2, 50), vib=30, vib_after=0.05)


@design(101, what="a hold: two tones swelling into a tense, beating chord that freezes and cuts off",
        use="the spells Hold Person and Hold Monster", kind="spell")
def hold(s):
    for ch, (hz, db) in enumerate(((392, 2), (392 * 1.5, 6), (392 * 2.02, 8))):
        s.tone(ch, 0.0, 1.0, const(hz), lambda x, db=db: db + 24 * max(0.0, 1 - x / 0.7))
    click(s, 1.0, 4, rate=1, dur=0.03)


@design(102, what="a vampiric touch: a draining glide downward, sucked away with a falling hiss",
        use="the spell Vampiric Touch", kind="spell")
def vampiric(s):
    s.tone(0, 0.0, 1.2, sweep(1500, 180, 1.0), hold_fade(2, 0.5, 30, attack=0.05), vib=80, vib_hz=6, vib_after=0.1)
    s.tone(1, 0.06, 1.26, sweep(1500, 180, 1.0), hold_fade(10, 0.5, 30, attack=0.05), vib=80, vib_hz=6, vib_after=0.1)
    s.noise(0.0, 1.0, decay(10, 25, attack=0.1), white=True, clock_hz=sweep(3500, 500, 1.0))


@design(103, 16, what="fear: a low, dissonant moan that swells over a deep growl and sinks (16 is never played)",
        use="the spell Fear", kind="spell")
def fear(s):
    r = Rand(103)
    s.tone(0, 0.0, 1.5, sweep(233, 196, 1.4), hold_fade(2, 0.9, 30, attack=0.4), vib=40, vib_hz=4, vib_after=0.3)
    s.tone(1, 0.0, 1.5, sweep(247, 207, 1.4), hold_fade(6, 0.9, 30, attack=0.5), vib=40, vib_hz=3, vib_after=0.3)
    growl(s, 0.0, 1.5, 50, 6, r, per_s=8, attack=0.5)


@design(104, what="a gentle cure: two soft rising notes that ring with a little vibrato",
        use="the spells Cure Light Wounds and Lay on Hands", kind="spell")
def cure_light(s):
    chime(s, (659, 880), 0.14, db=2, per_s=18, dur=1.2, vib=20)


@design(105, what="a stronger cure: three rising notes ringing together, with a soft echo of the first",
        use="the spell Cure Serious Wounds", kind="spell")
def cure_serious(s):
    chime(s, (659, 880, 1109), 0.12, db=0, per_s=16, dur=1.3, vib=20)


@design(106, what="the strongest cure: a quick rising arpeggio that blooms into a full major chord with a sparkle",
        use="the spell Cure Critical Wounds", kind="spell")
def cure_critical(s):
    s.tone(0, 0.0, 0.24, seq((0, 523), (0.06, 659), (0.12, 784), (0.18, 1047)), const(2))
    for ch, hz in enumerate((1047, 1319, 1568)):
        s.tone(ch, 0.24, 1.6, const(hz), decay(2 + 2 * ch, 14, attack=0.03), vib=25, vib_after=0.25)
    s.noise(0.24, 1.0, decay(14, 25, attack=0.1), white=True, rate=0)


@design(112, what="food appearing: a falling sparkle of notes and a soft plop",
        use="the spell Create Food", kind="spell")
def create_food(s):
    hz = seq((0, 1568), (0.07, 1319), (0.14, 1047), (0.21, 784))
    s.tone(0, 0.0, 0.3, hz, const(4))
    s.tone(1, 0.07, 0.37, hz, const(12))
    s.noise(0.0, 0.3, decay(14, 30), white=True, rate=0)
    s.tone(0, 0.36, 0.5, sweep(420, 200, 0.08), decay(0, 120))


@design(113, what="a magical vestment: a warm chord swelling on and a bright metal ping as it settles",
        use="the spell Magical Vestment", kind="spell")
def vestment(s):
    for ch, (hz, db) in enumerate(((523, 2), (659, 6))):
        s.tone(ch, 0.0, 1.1, const(hz), hold_fade(db, 0.6, 30, attack=0.25), vib=25, vib_after=0.3)
    ping(s, 2, 0.35, 2093, 4, per_s=35)


@design(116, what="a ward against lightning: a cool chord swelling, crossed by small crackles",
        use="the spell Protection from Lightning", kind="spell")
def prot_lightning(s):
    r = Rand(116)
    for ch, (hz, db, at) in enumerate(((440, 2, 0.2), (523, 6, 0.28), (659, 8, 0.35))):
        s.tone(ch, 0.0, 1.1, const(hz), hold_fade(db, 0.6, 30, attack=at), vib=25, vib_hz=6 + ch, vib_after=0.3)
    s.noise(0.0, 1.0, lambda x: 4 if r() < 0.12 else 30, white=True, rate=0)


@design(117, what="life returning: two faint heartbeats, then a slow rise into a bright chord",
        use="the spell Raise Dead", kind="spell")
def raise_dead(s):
    for t0, db in ((0.0, 6), (0.18, 8), (0.55, 4), (0.73, 6)):
        s.noise(t0, t0 + 0.1, decay(db, 150), white=False, clock_hz=const(15 * 55))
    s.tone(0, 0.6, 1.9, lambda x: sweep(220, 523, 0.6)(x), hold_fade(4, 0.8, 30, attack=0.4), vib=20, vib_after=0.6)
    s.tone(1, 0.6, 1.9, lambda x: sweep(330, 784, 0.6)(x), hold_fade(8, 0.8, 30, attack=0.5), vib=20, vib_after=0.6)


@design(118, what="a cone of cold: a blast of icy air with a thin, whistling tone",
        use="the spell Cone of Cold", kind="spell")
def cone_of_cold(s):
    s.noise(0.0, 1.1, decay(0, 25, attack=0.05), white=True, clock_hz=sweep(5000, 2500, 1.0))
    s.tone(0, 0.0, 1.1, sweep(3000, 2600, 1.0), decay(6, 25, attack=0.1), vib=60, vib_hz=8, vib_after=0.1)
    s.tone(1, 0.0, 1.1, sweep(3000 * 1.02, 2600 * 1.02, 1.0), decay(12, 25, attack=0.1), vib=60, vib_hz=6, vib_after=0.1)


# magic places, explosions, Xanathar

@design(24, what="a portal surge: a hissing swell with a high, wobbling tone in it",
        use="the portal animation, with 86 (src/EOV5.C:1065)", kind="magic")
def portal_surge(s):
    s.noise(0.0, 0.85, lambda x: 4 + 20 * abs(1 - x / 0.35) if x < 0.7 else 12 + 40 * (x - 0.7), white=True, rate=0)
    s.tone(0, 0.0, 0.85, wobble(const(1760), 150, 5), lambda x: 8 + 16 * abs(1 - x / 0.35))


@design(33, what="a portal powering up: a slow, soft hum rising over seven seconds, two tones beating, wind under it",
        use="the portal animation starts (src/EOV5.C:1046)", kind="magic")
def portal_power(s):
    s.tone(0, 0.0, 8.0, sweep(110, 330, 7.0), lambda x: 6 + 18 * max(0.0, 1 - x / 6.0) + max(0.0, x - 7.2) * 30, vib=15, vib_hz=3)
    s.tone(1, 0.0, 8.0, lambda x: sweep(110, 330, 7.0)(x) * 1.008, lambda x: 12 + 18 * max(0.0, 1 - x / 6.0) + max(0.0, x - 7.2) * 30)
    s.noise(0.0, 8.0, lambda x: 16 + 10 * max(0.0, 1 - x / 7.0) + max(0.0, x - 7.2) * 30, white=True,
            clock_hz=lambda x: 600 * 2 ** (x / 4 + 0.3 * math.sin(2 * math.pi * x / 1.3)))


@design(80, what="an oracle's insight: a rising whole-tone arpeggio, mysterious, with vibrato and a soft whoosh",
        use="an oracle identifies items (levels 4 and 9)", kind="magic")
def oracle(s):
    hz = seq((0, 523), (0.1, 587), (0.2, 659), (0.3, 740), (0.4, 831), (0.5, 932))
    s.tone(0, 0.0, 1.3, hz, hold_fade(2, 0.6, 30, attack=0.02), vib=40, vib_hz=5, vib_after=0.5)
    s.tone(1, 0.15, 1.45, hz, hold_fade(10, 0.6, 30, attack=0.02), vib=40, vib_hz=5, vib_after=0.5)
    s.noise(0.0, 1.0, decay(14, 25, attack=0.3), white=True, clock_hz=sweep(800, 2400, 1.0))


@design(35, what="an explosion: a crack, a burst of noise falling into a deep rumble",
        use="an area spell's burst in the view (src/E0EF3.C:478)", kind="hit")
def explosion(s):
    r = Rand(35)
    s.noise(0.0, 0.02, const(0), white=True, rate=0)
    s.noise(0.02, 1.0, lambda x: decay(0, 30)(x) + 3 * r(), white=True, clock_hz=lambda x: sweep(2000, 250, 0.8, 2)(x) * (0.8 + 0.4 * r()))
    s.tone(0, 0.0, 0.8, sweep(180, 110, 0.15, 2), decay(2, 35))
    s.tone(1, 0.0, 0.6, wobble(const(113), 90, 15), decay(6, 40))


@design(119, what="Xanathar coming apart: a short, crunching blast, made to repeat",
        use="each of the 19 explosion steps of Xanathar's death (src/EOV5.C:1200)", kind="death")
def xanathar_blast(s):
    r = Rand(119)
    s.noise(0.0, 0.4, lambda x: decay(0, 60)(x) + 4 * r(), white=True, clock_hz=lambda x: sweep(1500, 350, 0.3)(x) * (0.7 + 0.6 * r()))
    s.tone(0, 0.0, 0.35, sweep(200, 110, 0.1, 2), decay(2, 70))


def render(g):
    """The design of game sound g as tandymap's (t, byte) writes and its length in seconds, or None."""
    if g not in DESIGNS:
        return None
    s = Script(9.0)
    DESIGNS[g].fn(s)
    last = max((i for i in range(s.n) if any(v < 15 for v in s.v[i])), default=0)
    s.n = last + 2
    return s.bytes(), s.n / TICK + 0.3

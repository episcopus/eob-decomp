#!/usr/bin/env python3
"""arrange.py: the four pieces of music arranged for the Tandy's chip, from the AdLib's notes (README.md, "The music").

  arrange.py clips          build/sound/clips/<piece>/tandy-arranged.txt and .wav, and music.html to listen to them
                            beside the AdLib and the earlier mapping (rich)

The mapper played whatever voices were loudest each tick, so a tone channel went from one AdLib voice to another in
the middle of a note: the character generation music had 205 pitch jumps on a sounding channel against 21 tone runs.
Here each of the three tone channels is a part with a list of the AdLib voices it plays (score.py reads their notes),
in order of preference: a channel changes note only when a note starts, takes the note of a voice it prefers over
the one it plays, and goes back to a less preferred voice only at that voice's next note. Doubled voices (the AdLib
plays most lines twice, in unison or an octave apart) are left out. A note keeps the level the AdLib plays it at (its
carrier's level at its start, 3 dB of it to one step of the chip, as the mapper has it), so the swells and fades
stay. Notes below the chip's lowest tone (A2, 110 Hz) go up by octaves. Each part has an instrument made for the chip: a table of volumes from the note's start, then a
slow fall while the note is held, a release after it, and a vibrato for the leads (INSTRUMENTS).

`render()` here is the arrangement's reference: the chip's eight registers at every tick, made from the notes and the
instruments directly. sndat.py encodes the same notes as the player's music stream, and its build checks that the
player (sndat.Player, SNPLAY.ASM) gives back exactly these registers.
"""
from __future__ import annotations

import argparse
import json
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import score  # noqa: E402
import tandymap as M  # noqa: E402

OUT = M.OUT
CLIPS = OUT / "clips"
SN_TONE = 3579545 / 32
LOW = 45                                # MIDI A2, 110 Hz: note 0 of the period table
PERIODS = [round(SN_TONE / (110 * 2 ** (n / 12))) for n in range(64)]   # A2 to C8
VIB_DELAY = 8                           # ticks from a note's start to its vibrato
VIB = [0, 1, 1, 1, 1, 0, 0, -1, -1, -1, -1, 0]   # one cycle in 12 ticks (6 Hz), times period >> 6

# volumes are the chip's attenuation, 0 loudest, 15 off (2 dB a step); a part's gain is added to all of them
INSTRUMENTS = {
    # attack: the volumes of the first ticks; then held: one step quieter every `fall` ticks (0: never) down to
    # `floor`; released: one step quieter every `release` ticks to silence; vib: the lead's vibrato
    "pad":   {"attack": [13, 11, 9, 8, 7, 6, 6, 5], "fall": 48, "floor": 8, "release": 4, "vib": 0},
    "lead":  {"attack": [3, 1, 1, 2, 2, 3, 3, 4], "fall": 24, "floor": 7, "release": 2, "vib": 1},
    "bell":  {"attack": [2, 1, 2, 3, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13, 14, 14, 15],
              "fall": 0, "floor": 15, "release": 1, "vib": 0},
    "pluck": {"attack": [1, 2, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15], "fall": 0, "floor": 15, "release": 1, "vib": 0},
    "bass":  {"attack": [1, 1, 2, 3, 3, 4, 4, 5], "fall": 32, "floor": 7, "release": 2, "vib": 0},
}

# per piece, per tone channel: the voices it plays, preferred first: (AdLib voice, semitones, instrument, gain)
ARRANGEMENTS = {
    # character generation: four-voice pads (voices 0, 1, 3, 4, 5, 6 in pairs) under a bass (2, two octaves below the
    # chip) and, from 32 s, a counter melody (6), a harmony (4) and a quiet high line (7)
    "game020": [[(7, 0, "bell", 3), (0, 0, "pad", 1)],
                [(6, 0, "lead", 1), (5, 0, "pad", 1)],
                [(2, 24, "bass", 0)]],
    # the Westwood card: four arpeggios a step apart (0 to 3; two kept), a bell (5), a bass (8), and the drone (7,
    # 16 Hz on the AdLib, the loudest voice there; first listening: "too loud, grating" as the mapper played it) as a
    # quiet pad three octaves up; the quiet pad (4, 6) is left out
    "intro01": [[(5, 0, "bell", 2), (0, 0, "pluck", 1)],
                [(1, 0, "pluck", 2)],
                [(8, 12, "bass", 1), (7, 36, "pad", 3)]],
    # the title: the melody in four voices (5 to 8), a line in octaves (2, 3), the bass in octaves (0, 1)
    "intro02": [[(5, 0, "lead", 0), (7, 0, "lead", 0)],
                [(2, 0, "pad", 2), (3, -12, "pad", 2)],
                [(0, 12, "bass", 1), (7, 24, "bass", 1)]],
    # the last scene: a chord held in three doubled voices (0 to 5), a bass (6), a melody (7)
    "intro03": [[(7, 0, "lead", 0), (4, 0, "pad", 1)],
                [(2, 0, "pad", 1)],
                [(6, 24, "bass", 0)]],
}
END = {"game020": round(102.097 * M.TICK)}   # where the AdLib sets its loop flag (sndat.MUSIC_LOOP)


def note_number(midi: float, up: int) -> int:
    n = round(midi) + up - LOW
    while n < 0:
        n += 12
    while n >= len(PERIODS):
        n -= 12
    return n


def parts(name: str) -> list[list[dict]]:
    """Each channel's notes, in order: on (tick), dur (ticks held), note (PERIODS index), ins, gain."""
    notes = [n for n in score.transcribe(score.window(name)) if n["f"] >= 8]   # not the keyed silences (fnum 0)
    by = {}
    for n in notes:
        by.setdefault(n["voice"], []).append(n)
    out = []
    chosen = {v for sources in ARRANGEMENTS[name] for v, *_ in sources}
    ref = min(n["level"] for n in notes if n["voice"] in chosen)   # the loudest note the arrangement plays
    for sources in ARRANGEMENTS[name]:
        events = []
        for rank, (voice, up, ins, gain) in enumerate(sources):
            for n in by.get(voice, []):
                events.append((n["on"], rank, n, up, ins, gain))
        events.sort(key=lambda e: (e[0], e[1]))
        chan, cur = [], None
        for on, rank, n, up, ins, gain in events:
            if cur is not None and on < cur["on"] + cur["dur"] and cur["rank"] < rank:
                continue                    # a voice it prefers holds a note
            if cur is not None and cur["on"] == on:
                continue                    # one note a tick: the preferred voice's, sorted first
            dur = max(1, n["off"] - n["on"])
            if cur is not None and cur["on"] + cur["dur"] > on:
                cur["dur"] = on - cur["on"]     # cut by the note that takes its place
            # the note's own level on the AdLib, as the mapper has it (3 dB of OPL2 attenuation to a 2 dB step):
            # the swells, the fades and the quiet lines
            g = min(15, gain + max(0, round((n["level"] - ref) / M.DB_STEP)))
            cur = {"on": on, "dur": dur, "note": note_number(n["midi"], up), "ins": ins, "gain": g, "rank": rank}
            chan.append(cur)
        out.append(chan)
    return out


class Voice:
    """One tone channel's note: the envelope and the vibrato, tick by tick (SNPLAY.ASM's music, sndat.Player)."""

    def __init__(self, note: int, ins: dict, gain: int, dur: int):
        self.period = PERIODS[note]
        self.ins, self.gain, self.dur = ins, gain, dur
        self.age = 0
        self.phase = "attack"
        self.count = 0
        self.vol = 15
        self.live = True

    def tick(self) -> tuple[int, int]:
        """This tick's period and volume; the note ends (live False) when its release reaches silence."""
        ins = self.ins
        if self.phase != "release" and self.age >= self.dur:
            self.phase, self.count = "release", 0
        if self.phase == "attack":
            self.vol = ins["attack"][self.age]
            if self.age + 1 == len(ins["attack"]):
                self.phase, self.count = "held", 0
        elif self.phase == "held":
            if ins["fall"]:
                self.count += 1
                if self.count == ins["fall"]:
                    self.count = 0
                    if self.vol < ins["floor"]:
                        self.vol += 1
        else:
            self.count += 1
            if self.count == ins["release"]:
                self.count = 0
                self.vol += 1
                if self.vol >= 15:
                    self.vol = 15
                    self.live = False
        p = self.period
        if ins["vib"] and self.age >= VIB_DELAY:
            p = min(0x3FF, p + VIB[(self.age - VIB_DELAY) % len(VIB)] * (self.period >> 6))
        self.age += 1
        return p, min(15, self.vol + self.gain)


def length(name: str, chans) -> int:
    """The piece's length in ticks: its last release, or the AdLib's loop point."""
    last = 0
    for chan in chans:
        for n in chan:
            ins = INSTRUMENTS[n["ins"]]
            last = max(last, n["on"] + max(n["dur"], len(ins["attack"])) + 16 * ins["release"] + 1)
    return END.get(name, last)


def render(name: str):
    """The arrangement as the chip's registers: (tick, state) at every tick something changes, sndat's states."""
    chans = parts(name)
    end = length(name, chans)
    reg = {"t0": 0, "t1": 0, "t2": 0, "n": 0, "v0": 15, "v1": 15, "v2": 15, "v3": 15}
    out = []
    voices = [None, None, None]
    nxt = [0, 0, 0]
    for t in range(end):
        for c, chan in enumerate(chans):
            while nxt[c] < len(chan) and chan[nxt[c]]["on"] == t:
                n = chan[nxt[c]]
                voices[c] = Voice(n["note"], INSTRUMENTS[n["ins"]], n["gain"], n["dur"])
                nxt[c] += 1
            v = voices[c]
            if v is not None:
                p, vol = v.tick()
                reg["t%d" % c], reg["v%d" % c] = p, vol
                if not v.live:
                    voices[c] = None
        if not out or out[-1][1] != reg:
            out.append((t, dict(reg)))
    return out, end


def write_log(path: Path, states, end: int):
    """A chip log for render.cpp from the states."""
    prev = {"t0": None, "t1": None, "t2": None, "n": None, "v0": 15, "v1": 15, "v2": 15, "v3": 15}
    with open(path, "w") as f:
        f.write("chip sn\nlen %.3f\n" % (end / M.TICK + 0.5))
        for b in (0x9F, 0xBF, 0xDF, 0xFF):
            f.write("w 0 %d\n" % b)
        for t, r in states:
            for c in range(3):
                if r["t%d" % c] != prev["t%d" % c]:
                    p = r["t%d" % c]
                    f.write("w %.6f %d\nw %.6f %d\n" % (t / M.TICK, 0x80 | c << 5 | p & 15, t / M.TICK, p >> 4))
            for c in range(4):
                if r["v%d" % c] != prev["v%d" % c]:
                    f.write("w %.6f %d\n" % (t / M.TICK, 0x90 | c << 5 | r["v%d" % c]))
            prev = dict(r)


PAGE = """<!doctype html><html><head><meta charset="utf-8"><title>Tandy music</title>
<style>body{font:15px/1.5 system-ui,sans-serif;max-width:56rem;margin:2rem auto;padding:0 1rem;background:#fff;color:#111}
@media (prefers-color-scheme:dark){body{background:#151515;color:#eee}}
table{border-collapse:collapse;width:100%%}td,th{padding:.4rem;text-align:left;vertical-align:top;border-bottom:1px solid #8884}
audio{width:15rem}</style></head><body><h1>The music, arranged for the Tandy</h1>
<p>Each piece three ways: the AdLib, the mapping you heard on the EX (rich), and the new arrangement for the chip.</p>
<table><tr><th>Piece</th><th>AdLib</th><th>Mapped (rich)</th><th>Arranged</th></tr>%s</table></body></html>"""
TITLES = {"game020": "Character generation", "intro01": "Intro: the Westwood card", "intro02": "Intro: the title",
          "intro03": "Intro: the last scene"}


def clips() -> None:
    rows = []
    for name in ARRANGEMENTS:
        d = CLIPS / name
        d.mkdir(parents=True, exist_ok=True)
        states, end = render(name)
        write_log(d / "tandy-arranged.txt", states, end)
        subprocess.run([str(M.RENDER), str(d / "tandy-arranged.txt"), str(d / "tandy-arranged.wav")], check=True)
        if not (d / "adlib.wav").exists():          # the intro's pieces: tandymap.py intro made them
            raise SystemExit("arrange: %s has no AdLib render (make sound, tandymap.py intro)" % name)
        cells = "".join('<td><audio controls preload="none" src="clips/%s/%s.wav"></audio></td>' % (name, f)
                        for f in ("adlib", "tandy-rich", "tandy-arranged"))
        rows.append("<tr><td>%s</td>%s</tr>" % (TITLES[name], cells))
        print("%-8s %6.1f s, %d channel notes" % (name, end / M.TICK, sum(len(c) for c in parts(name))), flush=True)
    (OUT / "music.html").write_text(PAGE % "".join(rows))
    print("arrange: open %s" % (OUT / "music.html"))


def main(argv=None) -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("what", choices=["clips"])
    ap.parse_args(argv)
    clips()
    return 0


if __name__ == "__main__":
    sys.exit(main())

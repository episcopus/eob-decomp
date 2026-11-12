#!/usr/bin/env python3
"""score.py: the notes the AdLib plays in a piece of music, read from its OPL2 capture (README.md, "The music").

  score.py show NAME        each voice of a piece: its notes, range, level, and the voices it doubles
                            (NAME: game020, intro01, intro02, intro03)

A note is a key-on edge of an OPL2 voice (register B0h + voice, bit 5) to the next key-off or key-on, with the pitch
the mapper hears at its start (tandymap.voice_info: the carrier's frequency, or the harmonic series' fundamental), the
level of its carrier as set (dB under full, from TL and KSL) and the tick, at the AdLib driver's 72.8 Hz, where its
envelope falls 45 dB under full (tandymap.AUDIBLE): how long it really sounds, past its key-off on a slow release, or
before it on a decaying patch. The arrangements (arrange.py) are made from these notes.
"""
from __future__ import annotations

import argparse
import json
import math
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import tandymap as M  # noqa: E402

OUT = M.OUT
PIECES = {"game020": ("sfx-adlib.json", 20), "intro01": ("introparts-adlib.json", 1),
          "intro02": ("introparts-adlib.json", 2), "intro03": ("introparts-adlib.json", 3)}


def window(name: str) -> dict:
    f, g = PIECES[name]
    return next(w for w in json.loads((OUT / f).read_text())["windows"] if w["game"] == g)


def midi(f: float) -> float:
    return 69 + 12 * math.log2(f / 440) if f > 0 else 0.0


def transcribe(w: dict) -> list[dict]:
    """The notes of a capture window, in order of their start."""
    M.PATCH_NOISE.update(M.measure_patches([w]))
    M.LINES.update(M.measure_line_tables([w]))
    seen = {}
    real = M.voice_info

    def spy(regs, v, key):
        i = real(regs, v, key)
        seen[v.c] = i
        return i
    M.voice_info = spy
    try:
        m = M.Mapper("plain", w.get("regs"), music=True)
        m.emit = lambda t, want: None
        writes, t0 = w["writes"], w["t0"]
        start = (writes[0][0] + 0.009) if writes else t0
        while start - 1 / M.TICK > t0:
            start -= 1 / M.TICK
        n = int((w["t1"] - start) * M.TICK) + 1
        k = 0
        open_ = {}
        notes = []
        for tick in range(n):
            t = start + tick / M.TICK
            edges = []
            while k < len(writes) and writes[k][0] <= t:
                _, r, val = writes[k]
                if 0xB0 <= r <= 0xB8:
                    c = r - 0xB0
                    was = m.voices[c].keyed
                    if val & 0x20 and not was:
                        edges.append(c)
                    elif was and not val & 0x20 and c in open_:
                        open_[c]["off"] = tick
                m.write(r, val)
                k += 1
            seen.clear()
            m.tick(t)
            for c in edges:
                if c in open_:
                    open_[c].setdefault("off", tick)
                regs = m.regs
                k_op = M.OPS[c] + 3
                i = seen.get(c) or real(regs, m.voices[c], 0)
                note = {"voice": c, "on": tick, "f": i["f"], "midi": midi(i["f"]),
                        "level": (regs[0x40 + k_op] & 63) * 0.75 + M.ksl_db(regs[0x40 + k_op], i["fnum"], i["block"]),
                        "kind": m.voices[c].kind if m.voices[c].decided else
                        ("noise" if M.PATCH_NOISE.get(M.patch_key(regs, c), 0.0) >= M.NOISE_FLAT else "tone"),
                        "sustain": bool(regs[0x20 + k_op] & 0x20), "patch": list(M.patch_key(regs, c)),
                        "heard": tick}
                notes.append(note)
                open_[c] = note
            for c, i in seen.items():
                if c in open_ and i["level"] < M.AUDIBLE:
                    open_[c]["heard"] = tick
        for note in notes:
            note.setdefault("off", n)
            note["heard"] = max(note["heard"], note["on"]) + 1
    finally:
        M.voice_info = real
    return notes


def name_note(x: float) -> str:
    n = round(x)
    return "CcDdEFfGgAaB"[n % 12] + str(n // 12 - 1)


def show(name: str) -> None:
    notes = transcribe(window(name))
    by = {}
    for n in notes:
        by.setdefault(n["voice"], []).append(n)
    seqs = {c: [(n["on"], round(n["midi"])) for n in v] for c, v in by.items()}
    for c, v in sorted(by.items()):
        ms = [n["midi"] for n in v]
        lens = sorted(n["heard"] - n["on"] for n in v)
        twins = [d for d in by if d != c and [t for t, _ in seqs[d]] == [t for t, _ in seqs[c]]]
        octs = {d: sorted({b - a for (_, a), (_, b) in zip(seqs[c], seqs[d])}) for d in twins}
        print("voice %d: %3d notes %-5s %s to %s, level %4.1f dB, heard %d ticks (median), sustain %s, twins %s" % (
            c, len(v), v[0]["kind"], name_note(min(ms)), name_note(max(ms)),
            sum(n["level"] for n in v) / len(v), lens[len(lens) // 2], v[0]["sustain"], octs))


def main(argv=None) -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("what", choices=["show"])
    ap.add_argument("name", choices=list(PIECES))
    a = ap.parse_args(argv)
    show(a.name)
    return 0


if __name__ == "__main__":
    sys.exit(main())

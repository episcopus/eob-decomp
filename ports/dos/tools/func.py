#!/usr/bin/env python3
"""func.py: do two builds do the same thing? The gate for a speed patch: same screens, game state, sound, files.

  func.py [--ref ref] [--game game] [--session NAME ...]

Plays each session on both builds with the program clock (tandybox: virtual time moves only at the game's own waits,
so a build that draws faster sees the same times). At every key the game reads and every `shot` it compares: the
video memory (32 KB at 9800:0), the 64 KB of pages below it (8800:0), DGROUP, and the times; at the end all the Tandy
sound writes, and the files read. Exit status 0 when every session agrees. Prints the first difference otherwise.
"""
from __future__ import annotations

import argparse
import hashlib
import struct
import sys
import time
from collections import Counter
from concurrent.futures import ProcessPoolExecutor
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import tandybox as T  # noqa: E402
import prof  # noqa: E402


def run(build: str, session: str, pad_to: int = 0) -> dict:
    text = (prof.SESSIONS / (session + ".keys")).read_text()
    options = T.Session(text, None).options
    b = T.get_build(build, options.get("program", "EOB.EXE"))
    if pad_to and b.drv.stat().st_size < pad_to:
        # the reference's driver with the game's size, so both builds lay out their memory the same way
        padded = T.builds_dir() / build / ("TGA-%d.OVL" % pad_to)
        d = bytearray(b.drv.read_bytes().ljust(pad_to, b"\0"))
        d[2:6] = struct.pack("<HH", pad_to % 512, (pad_to + 511) // 512)     # the MZ header's image size
        padded.write_bytes(d)
        b.drv = padded
    save = options.get("save")
    m = T.TandyMachine(b, clock="program", save=(T.ROOT / "tests/sessions" / save) if save else None)
    points = []

    def snapshot(label):
        ds = m.dgroup()
        data = {"label": label, "t": m.insns,
                "vram": hashlib.sha1(m.screen()).hexdigest(),
                "pages": hashlib.sha1(bytes(m.uc.mem_read(T.PAGE, 0x10000))).hexdigest(),
                "dgroup": bytes(m.uc.mem_read(ds * 16, 0x10000)),
                "work": m.work}
        points.append(data)

    def on_action(label, t0, t1):
        snapshot("action " + str(label))

    s = T.Session(text, m, on_action=on_action)
    s.attach()
    orig = s.checkpoint
    s.checkpoint = lambda name: (orig(name), snapshot("shot " + name))
    reason = m.run_clocked(int(3000 * T.CLOCK))
    # a read inside a PAK's directory is how a file is found, not what is loaded (pak-lookup.patch reads the directory at once)
    reads = [(r[1], r[2], r[3]) for r in m.reads_log if not (r[1].endswith(".PAK") and r[2] < pak_dir_end(r[1]))]
    return {"reason": reason, "done": s.done(), "points": points, "tandy": m.tandy_log, "reads": reads,
            "work": m.work, "program": b.program}


DRIVER_TABLE = {"EOB.EXE": (0x802A, 0x80E6), "INTRO.EXE": (0x1EB6, 0x1F72)}      # the driver's exported entry points: where its routines are, not what they do


BURST_WORDS = {0xFC62, 0xFC64, 0xFC6C, 0xFC6E, 0xFC72}   # AnimateParticleBurst's globals for the current particle: read only inside it (grep), and the driver routine does not store them


HEAP_WORDS = {"EOB.EXE": BURST_WORDS | {0x0088, 0x008A, 0x818E, 0x84B2, 0x85C6, 0xF5B4, 0xF5B6, 0xF79A, 0xF980}, "INTRO.EXE": {0x0070, 0x0072, 0x0088, 0x008A, 0x008C, 0x201A}}   # sizes and addresses of heap blocks, as words: they follow
# the driver's size (listed by what differs when only the driver's size differs; values not pointers into the heap)


def moved(ref: str, game: str, program: str) -> set:
    """(old offset, new offset) of every routine a patch moved: a shorter function moves what follows it in its unit, so a table in
    DGROUP that points at those routines holds new offsets (spell records, node handlers). Such a word is the same pointer."""
    a, b = T.get_build(ref, program).pubs, T.get_build(game, program).pubs
    return {(a[n][1], b[n][1]) for n in a if n in b and a[n][1] != b[n][1]}


def dgroup_diff(x: bytes, y: bytes, program: str = "EOB.EXE", moves=frozenset()) -> str | None:
    """A larger driver moves every heap block after it by the same number of paragraphs, S: DGROUP's far pointers into
    the heap differ by S, a few words that record heap sizes (HEAP_WORDS) by something else. Every other word must
    be equal, including the driver table only for what is not the driver's own offsets."""
    if x == y:
        return None
    x, y = bytearray(x), bytearray(y)
    lo, hi = DRIVER_TABLE[program]
    x[lo:hi] = y[lo:hi] = bytes(hi - lo)
    for k in [k for k in range(0, 0xFFFE) if x[k] != y[k]]:      # a moved routine's offset, at any byte (spell records are 19 bytes)
        for s in (k - 1, k):
            if s >= 0 and ((x[s] | x[s + 1] << 8), (y[s] | y[s + 1] << 8)) in moves:
                x[s:s + 2] = y[s:s + 2]
                break
    delta = [(k, ((y[k] | y[k + 1] << 8) - (x[k] | x[k + 1] << 8)) & 0xFFFF) for k in range(0, 0xFFFE, 2) if x[k:k + 2] != y[k:k + 2]]
    delta = [(k, d) for k, d in delta if k not in HEAP_WORDS[program]]
    if not delta:
        return None
    s = Counter(d for _k, d in delta).most_common(1)[0][0]
    for k in [k for k in range(0, 0xFFFE) if x[k] != y[k]]:     # a heap segment at any byte (packed records)
        for o in (k - 1, k):
            if o >= 0 and ((y[o] | y[o + 1] << 8) - (x[o] | x[o + 1] << 8)) & 0xFFFF == s:
                x[o:o + 2] = y[o:o + 2]
                break
    for k in range(0, 0xFFFE, 2):
        if x[k:k + 2] != y[k:k + 2] and k not in HEAP_WORDS[program]:
            return "ds:%04X differs by %04X (heap shift %04X)" % (k, ((y[k] | y[k + 1] << 8) - (x[k] | x[k + 1] << 8)) & 0xFFFF, s)
    return None


def first_diff(a, b, moves=frozenset()):
    for i, (x, y) in enumerate(zip(a["points"], b["points"])):
        for k in ("label", "t", "vram", "pages"):
            if x[k] != y[k]:
                return "checkpoint %d (%s): %s differs (%s / %s)" % (i, x["label"], k, x[k], y[k])
        # INTRO.EXE's DGROUP is mostly far pointers to heap blocks whose places follow the driver and overlay sizes; what it does is
        # on the screen, in the pages, in the sound writes and in the file reads, which are all compared
        d = dgroup_diff(x["dgroup"], y["dgroup"], a["program"], moves) if a["program"] == "EOB.EXE" else None
        if d:
            return "checkpoint %d (%s): DGROUP %s" % (i, x["label"], d)
    if len(a["points"]) != len(b["points"]):
        return "checkpoints: %d / %d" % (len(a["points"]), len(b["points"]))
    if a["tandy"] != b["tandy"]:
        return "Tandy sound writes differ (%d / %d)" % (len(a["tandy"]), len(b["tandy"]))
    exes = {"EOB.EXE", "INTRO.EXE"}                             # the overlays' offsets in the program follow the size of each overlay
    ra = [r for r in a["reads"] if r[0] not in exes]
    rb = [r for r in b["reads"] if r[0] not in exes]
    if ra != rb:
        return "file reads differ"
    if a["reason"] != b["reason"]:
        return "the runs ended differently: %s / %s" % (a["reason"], b["reason"])
    return None


PAK_DIR = {}


def pak_dir_end(name: str) -> int:
    """The end of a PAK's directory: the offset of its first file."""
    if name not in PAK_DIR:
        PAK_DIR[name] = struct.unpack("<I", (T.GAME / name).read_bytes()[:4])[0]
    return PAK_DIR[name]


def job(a):
    t0 = time.time()
    return a[:2], run(*a), time.time() - t0


def main(argv=None) -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--ref", default="ref")
    ap.add_argument("--game", default="game")
    ap.add_argument("--session", action="append")
    a = ap.parse_args(argv)
    sessions = a.session or prof.SUITE
    pad = T.get_build(a.game).drv.stat().st_size
    jobs = [(a.ref, s, pad) for s in sessions] + [(a.game, s, 0) for s in sessions]
    results = {}
    with ProcessPoolExecutor(max_workers=6) as ex:
        for (b, s), r, host in ex.map(job, jobs):
            results[(b, s)] = r
    bad = 0
    table = {}
    for s in sessions:
        ra, rb = results[(a.ref, s)], results[(a.game, s)]
        d = first_diff(ra, rb, moved(a.ref, a.game, ra["program"]))
        n = len(ra["points"])
        if d:
            bad += 1
            print("%-10s DIFFERENT: %s" % (s, d))
        else:
            print("%-10s same: %d checkpoints, %d sound writes, %d file reads, %s; work %.2f s -> %.2f s (%.2fx)" % (
                s, n, len(ra["tandy"]), len(ra["reads"]), ra["reason"], ra["work"] / T.CLOCK, rb["work"] / T.CLOCK,
                ra["work"] / max(1, rb["work"])))
            pa, pb = ra["points"], rb["points"]
            for x0, x1, y0, y1 in zip(pa, pa[1:], pb, pb[1:]):
                table.setdefault(s + ":" + x1["label"].split()[1] if x1["label"].startswith("action") else "", []).append(
                    (x1["work"] - x0["work"], y1["work"] - y0["work"]))
    if not bad:
        print("\n%-20s %5s %9s %9s %8s %10s" % ("action (same game)", "n", "ref (ms)", "game (ms)", "median", "total"))
        for k in sorted(table):
            if not k:
                continue
            xs = sorted(x / T.CLOCK * 1000 for x, _y in table[k])
            ys = sorted(y / T.CLOCK * 1000 for _x, y in table[k])
            tx, ty = sum(xs), sum(ys)
            print("%-20s %5d %9.0f %9.0f %7.2fx %9.2fx" % (k, len(xs), xs[len(xs) // 2], ys[len(ys) // 2],
                                                            xs[len(xs) // 2] / max(0.001, ys[len(ys) // 2]), tx / max(0.001, ty)))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())

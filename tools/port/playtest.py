#!/usr/bin/env python3
"""playtest.py: play the port the way a player does, fast (playbook 80, "Playtesting"; `make port-playtest`).

The port's headless program runs live play on a clock that only its own sleeps move, so an hour of game
time costs seconds and every run is the same. This drives it from short scripts of what a player does,
and asks only what a player asks: did it stop with an error, did it hang, did the screen do something.
No instruction counts and no comparison with a recording.

  - Scripts (tests/playtest/*.play), one per thing a player does. Failure: an error stop ("not ported",
    a DOS call the machine lacks, a crash), no end within the time limit (a hang), a screen marked
    `changed` that did not change, or an unexpected Exit to DOS.
  - The monkey: random clicks and keys from the saved game and from a new party (--monkey N runs).
    Nothing is scripted about what it reaches; a failure names the seed, and --monkey-seed S replays it.
  - Coverage (--coverage): which of the game's functions no run reached (build/port/playtest/never-run.txt).

Every screenshot is written as a PNG, and each script's as one contact sheet
(build/port/playtest/<script>.png), to be looked at.

Script lines, times in milliseconds of the game's clock:
  start title | intro            where the run starts (default title: EOB.EXE directly; intro: INTRO.EXE)
  save FILE | save none          the saved game the run starts with (default none; FILE under tests/sessions)
  wait MS                        let the game run
  key NAME                       press and release a key, then 400 ms (ENTER ESC UP DOWN LEFT RIGHT A..Z 0..9 F1..F10 SPACE ...)
  click X Y | rclick X Y         move the pointer to the screen pixel, press and release, then 400 ms
  shot NAME [changed]            a screenshot; `changed`: it must differ from the one before
  restart                        end this run and start the program again, with the saves it wrote
  expect exit                    the run ends by the game's own Exit to DOS
  expect sound N                 from here to the run's end, at least N notes start on the AdLib
  realtime                       this run on the wall clock, as in a window (slow): its timer ticks are checked
  silent                         this run with no sound card (every other run has the AdLib, as a player does)
"""
from __future__ import annotations

import argparse
import concurrent.futures as cf
import os
import random
import re
import shutil
import subprocess
import sys
from dataclasses import dataclass, field
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))
from port.replay import game_dir  # noqa: E402

SCRIPTS = ROOT / "tests" / "playtest"
OUT = ROOT / "build" / "port" / "playtest"
LIMIT_S = 120                                    # the wall clock a run may take: more is a hang

SCAN = {"ESC": 0x01, "ENTER": 0x1C, "SPACE": 0x39, "TAB": 0x0F, "BACKSPACE": 0x0E,
        "UP": 0x48, "DOWN": 0x50, "LEFT": 0x4B, "RIGHT": 0x4D, "HOME": 0x47, "END": 0x4F, "PGUP": 0x49, "PGDN": 0x51,
        "KP4": 0x4B, "KP5": 0x4C, "KP6": 0x4D, "KP7": 0x47, "KP8": 0x48, "KP9": 0x49, "KP1": 0x4F, "KP2": 0x50, "KP3": 0x51,
        **{c: s for s, c in zip([0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19], "QWERTYUIOP")},
        **{c: s for s, c in zip([0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26], "ASDFGHJKL")},
        **{c: s for s, c in zip([0x2C, 0x2D, 0x2E, 0x2F, 0x30, 0x31, 0x32], "ZXCVBNM")},
        **{str((i + 1) % 10): 0x02 + i for i in range(10)},
        **{"F%d" % (i + 1): 0x3B + i for i in range(10)}}


@dataclass
class Run:
    start: str = "title"
    save: str | None = None
    events: list[str] = field(default_factory=list)
    shots: list[tuple[int, str, bool]] = field(default_factory=list)
    t: int = 0
    expect_exit: bool = False
    realtime: bool = False
    silent: bool = False
    sound: list[tuple[int, int]] = field(default_factory=list)


@dataclass
class Result:
    name: str
    problems: list[str]
    shots: list[Path]
    log: str
    reached_exit: bool = False


def parse(text: str) -> list[Run]:
    runs = [Run()]
    r = runs[0]
    for raw in text.splitlines():
        line = raw.split("#", 1)[0].strip()
        if not line:
            continue
        w = line.split()
        if w[0] == "start":
            r.start = w[1]
        elif w[0] == "save":
            r.save = None if w[1] == "none" else w[1]
        elif w[0] == "wait":
            r.t += int(w[1])
        elif w[0] == "key":
            code = SCAN[w[1]]
            r.events += ["%d:%02X" % (r.t, code), "%d:%02X" % (r.t + 100, code | 0x80)]
            r.t += 400
        elif w[0] in ("click", "rclick"):
            x, y, b = int(w[1]), int(w[2]), 1 if w[0] == "click" else 2
            r.events += ["%d:M%d.%d.0" % (r.t, x, y), "%d:M%d.%d.%d" % (r.t + 100, x, y, b), "%d:M%d.%d.0" % (r.t + 200, x, y)]
            r.t += 400
        elif w[0] == "shot":
            r.shots.append((r.t, w[1], "changed" in w[2:]))
        elif w[:2] == ["expect", "exit"]:
            r.expect_exit = True
        elif w[:2] == ["expect", "sound"]:
            r.sound.append((r.t, int(w[2])))
        elif w[0] == "realtime":
            r.realtime = True
        elif w[0] == "silent":
            r.silent = True
        elif w[0] == "restart":
            r = Run(start=r.start)
            runs.append(r)
        else:
            raise SystemExit("playtest: bad line: %s" % raw)
    return runs


def png(raw: bytes, path: Path) -> None:
    from PIL import Image
    im = Image.frombytes("P", (320, 200), raw[:64000])
    im.putpalette([(c << 2) | (c >> 4) for c in raw[64000:64768]])
    im.convert("RGB").save(path)


def sheet(paths: list[Path], out: Path) -> None:
    from PIL import Image, ImageDraw
    if not paths:
        return
    cols = min(4, len(paths))
    rows = (len(paths) + cols - 1) // cols
    im = Image.new("RGB", (cols * 324, rows * 216), (40, 40, 40))
    d = ImageDraw.Draw(im)
    for i, p in enumerate(paths):
        x, y = (i % cols) * 324 + 2, (i // cols) * 216 + 14
        im.paste(Image.open(p), (x, y))
        d.text((x, y - 12), p.stem, fill=(255, 255, 160))
    im.save(out)


def run_program(exe: Path, gd: Path, saves: Path, r: Run, where: Path, stem: str, env_extra: dict) -> tuple[str, int | None, list[tuple[Path, bool]]]:
    env = dict(os.environ, TC_NULL_INPUT=",".join(r.events), TC_PLAY_END=str(r.t + 500), TC_PLAY_STATS="1", **env_extra)
    if r.realtime:
        env["TC_NULL_REALTIME"] = "1"
    made = []
    plan = []
    for t, name, ch in r.shots:
        p = where / ("%s%s.bin" % (stem, name))
        plan.append("%d:%s" % (t, p))
        made.append((p, ch))
    if plan:
        env["TC_PLAY_SNAPSHOT"] = ";".join(plan)
    if r.sound:
        env["TC_NULL_ADLIB"] = str(where / (stem + "adlib.txt"))
    args = [str(exe), "--play", str(gd), "--saves", str(saves)]
    if r.start == "intro":
        args.append("--intro")
    if r.silent:
        args.append("--no-sound")
    try:
        c = subprocess.run(args, env=env, capture_output=True, text=True, timeout=LIMIT_S)
        return c.stderr, c.returncode, made
    except subprocess.TimeoutExpired as e:
        err = e.stderr.decode(errors="replace") if isinstance(e.stderr, bytes) else (e.stderr or "")
        return err, None, made


def play(name: str, runs: list[Run], exe: Path, gamedirs: dict, env_extra: dict) -> Result:
    where = OUT / name
    if where.exists():
        shutil.rmtree(where)
    where.mkdir(parents=True)
    saves = where / "saves"
    saves.mkdir()
    problems: list[str] = []
    pngs: list[Path] = []
    logs = []
    reached_exit = False
    prev = None
    for i, r in enumerate(runs):
        gd = gamedirs[r.save]
        stem = "%d-" % i if len(runs) > 1 else ""
        err, rc, shots = run_program(exe, gd, saves, r, where, stem, env_extra)
        logs.append(err)
        ended = "the game ended" in err
        reached_exit = reached_exit or ended
        last_line = err.strip().splitlines()[-1] if err.strip() else ""
        if rc is None:
            problems.append("run %d: no end within %d s: a hang" % (i, LIMIT_S))
        elif rc < 0:
            problems.append("run %d: killed by signal %d (a crash)" % (i, -rc))
        elif ended and not r.expect_exit and not name.startswith("monkey"):
            problems.append("run %d: the game ended early: %s" % (i, last_line))
        elif not ended and rc != 0:
            problems.append("run %d: stopped: %s" % (i, last_line))
        elif r.expect_exit and not ended:
            problems.append("run %d: expected the game's exit, got: %s" % (i, last_line))
        if r.sound:
            log = where / (stem + "adlib.txt")
            notes = [int(t) for t, reg, val in (ln.split() for ln in (log.read_text().splitlines() if log.exists() else []))
                     if 0xB0 <= int(reg, 16) <= 0xB8 and int(val, 16) & 0x20]
            for t0, want in r.sound:
                got = sum(1 for t in notes if t >= t0)
                if got < want:
                    problems.append("run %d: from %d ms %d notes started on the AdLib, expected at least %d" % (i, t0, got, want))
        m = re.search(r"ticks due (\d+), delivered (\d+)", err)
        if m and int(m.group(1)) - int(m.group(2)) > max(3, int(m.group(1)) // 50):
            problems.append("run %d: the game got %s of %s timer ticks" % (i, m.group(2), m.group(1)))
        for k, (b, ch) in enumerate(shots):
            if not b.exists():
                if (rc == 0 or rc is None) and not ended:
                    problems.append("run %d: no screenshot %s" % (i, b.stem))
                continue
            raw = b.read_bytes()
            p = where / ("%02d-%s.png" % (len(pngs), b.stem))
            png(raw, p)
            pngs.append(p)
            if ch and prev == raw[:64000]:
                problems.append("run %d: %s is the same screen as the shot before" % (i, b.stem))
            prev = raw[:64000]
            b.unlink()
    if len(pngs) and not name.startswith("monkey"):
        sheet(pngs, OUT / (name + ".png"))
    return Result(name, problems, pngs, "\n".join(logs), reached_exit)


# where a player clicks: the movement arrows, the view, the hands and portraits, Camp, the menu rows, the
# inventory's pages and the Yes and No of a question
SPOTS = [(11, 134), (35, 134), (57, 134), (11, 152), (35, 152), (57, 152)] * 3 + \
        [(x, y) for x in (225, 290) for y in (22, 36, 70, 84)] + \
        [(x, y) for x in (195, 262) for y in (30, 80)] + \
        [(300, 185)] * 2 + [(60, y) for y in (27, 44, 61, 78, 95, 112)] + [(148, 129)] * 3 + [(30, 63), (140, 63)] + \
        [(88, 60), (40, 40), (140, 40), (40, 100), (140, 100), (88, 100)] * 2


def monkey_script(seed: int, start_save: bool, length_ms: int) -> tuple[str, list[str]]:
    """Random play: walking, the buttons a player clicks, anywhere on the screen now and then, and the keys."""
    rng = random.Random(seed)
    keys = ["UP"] * 8 + ["LEFT", "RIGHT"] * 3 + ["DOWN", "ENTER", "ESC", "SPACE", "PGUP", "PGDN", "HOME", "END"] + list("ABCDEFIRSZ123456")
    lines = ["save fixtures/EOBDATA.SAV" if start_save else "save none", "wait 40000"]
    if start_save:
        lines += ["key ENTER", "wait 6000"]
    else:
        lines += ["key DOWN", "wait 300", "key ENTER", "wait 4000"]
    t = 0
    while t < length_ms:
        roll = rng.random()
        if roll < 0.35:
            lines.append("click %d %d" % rng.choice(SPOTS))
        elif roll < 0.55:
            lines.append("rclick %d %d" % rng.choice(SPOTS))
        elif roll < 0.62:
            lines.append("click %d %d" % (rng.randrange(320), rng.randrange(200)))
        else:
            lines.append("key %s" % rng.choice(keys))
        w = rng.choice([50, 150, 400, 800])
        lines.append("wait %d" % w)
        t += 400 + w
    lines.append("shot end")
    return "\n".join(lines), lines


def gamedirs_for(names: set) -> dict:
    out = {}
    base = OUT / "game"
    for s in names:
        d = base / ("none" if s is None else re.sub(r"\W", "_", s))
        game_dir(d, None if s is None else ROOT / "tests" / "sessions" / s)
        out[s] = d
    return out


def unported() -> list[str]:
    """What the build says it could not port: each one is a stop in the middle of the game."""
    f = ROOT / "build" / "port" / "gen" / "unported.c"
    return re.findall(r'tc_unported\("(\w+)"', f.read_text()) if f.exists() else []


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("names", nargs="*", help="scripts of tests/playtest (default: all)")
    ap.add_argument("--monkey", type=int, default=0, help="how many random runs")
    ap.add_argument("--monkey-seed", type=int, default=None, help="replay one random run")
    ap.add_argument("--monkey-ms", type=int, default=240000, help="the game time of each random run")
    ap.add_argument("--exe", type=Path, default=ROOT / "build" / "port" / "eob-port-headless")
    ap.add_argument("--jobs", type=int, default=os.cpu_count() or 4)
    ap.add_argument("--coverage", action="store_true", help="the instrumented build: which functions no run reached")
    a = ap.parse_args(argv)
    OUT.mkdir(parents=True, exist_ok=True)
    if unported():
        print("playtest: FAIL: the build left these unported, and the game stops if it reaches one: %s" % ", ".join(unported()))
        return 1
    env_extra = {}
    exe = a.exe
    prof = OUT / "profiles"
    if a.coverage:
        exe = ROOT / "build" / "port-cov" / "eob-port-headless"
        if prof.exists():
            shutil.rmtree(prof)
        prof.mkdir()
        env_extra["LLVM_PROFILE_FILE"] = str(prof / "%p-%m.profraw")
    jobs = []
    if a.monkey_seed is not None:
        for sv in (True, False):
            jobs.append(("monkey-%d-%s" % (a.monkey_seed, "save" if sv else "new"), parse(monkey_script(a.monkey_seed, sv, a.monkey_ms)[0])))
    else:
        names = a.names or sorted(p.stem for p in SCRIPTS.glob("*.play"))
        for n in names:
            jobs.append((n, parse((SCRIPTS / (n + ".play")).read_text())))
        for s in range(a.monkey):
            sv = s % 2 == 0
            jobs.append(("monkey-%d-%s" % (s, "save" if sv else "new"), parse(monkey_script(s, sv, a.monkey_ms)[0])))
    gamedirs = gamedirs_for({r.save for _, rs in jobs for r in rs})
    failed = []
    with cf.ThreadPoolExecutor(a.jobs) as pool:
        futs = {pool.submit(play, n, rs, exe, gamedirs, env_extra): n for n, rs in jobs}
        for f in cf.as_completed(futs):
            r = f.result()
            print("%-28s %s" % (r.name, "ok" if not r.problems else "FAIL"), flush=True)
            for p in r.problems:
                print("    " + p, flush=True)
            if r.problems:
                failed.append(r.name)
                (OUT / (r.name + ".log")).write_text(r.log)
    print("playtest: %d runs, %d failed; screenshots in %s" % (len(jobs), len(failed), OUT.relative_to(ROOT)))
    if a.coverage:
        report(exe, prof)
    return 1 if failed else 0


def report(exe: Path, prof: Path) -> None:
    data = OUT / "merged.profdata"
    raws = [str(p) for p in prof.glob("*.profraw")]
    subprocess.run(["xcrun", "llvm-profdata", "merge", "-o", str(data)] + raws, check=True)
    out = subprocess.run(["xcrun", "llvm-cov", "report", str(exe), "-instr-profile=" + str(data), "--show-functions",
                          *[str(p) for p in sorted((ROOT / "build/port-cov/gen").glob("*.c"))]],
                         capture_output=True, text=True).stdout
    never, ran = [], 0
    for line in out.splitlines():
        m = re.match(r"(\S+)\s+\d+\s+\d+\s+[\d.]+%\s+(\d+)\s+(\d+)\s+([\d.]+)%", line)
        if not m or m.group(1) in ("Name", "TOTAL"):
            continue
        if int(m.group(2)) == int(m.group(3)):
            never.append(m.group(1))
        else:
            ran += 1
    (OUT / "never-run.txt").write_text("\n".join(sorted(never)) + "\n")
    print("coverage: %d of %d functions ran; the others in %s" % (ran, ran + len(never), (OUT / "never-run.txt").relative_to(ROOT)))


if __name__ == "__main__":
    sys.exit(main())

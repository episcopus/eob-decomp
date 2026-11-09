#!/usr/bin/env python3
"""dbx.py: the game on an emulated Tandy (DOSBox-X, machine=tandy), headless: does a real BIOS and DOS run it?

  dbx.py [--seconds 100] [--keep]

The reference build and the game, each started the way START1.EXE starts them (Tandy video, Tandy sound, no mouse),
typing the same keys (AUTOTYPE: load the saved game, turn, a character sheet). Nothing in those keys rolls dice or depends
on the time, so every screen the reference holds for 2 s must appear in the game's video, pixel for pixel. Also checks
that the screens are 320x200 (the Tandy mode). The emulated CPU runs flat out (cycles=max): this is not a timing test.
Exit status 0 on success, 77 when DOSBox-X or ffmpeg is missing. Frames: build/dbx/<run>/frames.
"""
from __future__ import annotations

import argparse
import os
import shutil
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

HERE = Path(__file__).resolve().parent
PORT = HERE.parent
ROOT = PORT.parents[1]
sys.path.insert(0, str(ROOT / "tools/re"))
OUT = PORT / "build/dbx"
ARGS = "1 3 0 homie 3 40960"
KEYS = "enter pageup pageup f1 i esc pageup"
WAIT, PACE = 25, 7
STEADY = 8


def game_dir(c: Path, build: Path) -> None:
    eob = c / "EOB"
    if eob.exists():
        shutil.rmtree(eob)
    eob.mkdir(parents=True)
    for p in (ROOT / "eob").iterdir():
        if p.is_file():
            shutil.copy(p, eob / p.name)
    shutil.copy(build / "EOB.EXE", eob / "EOB.EXE")
    shutil.copy(build / "TGA.OVL", eob / "TGA.OVL")
    if (build / "INTRO.EXE").exists():
        shutil.copy(build / "INTRO.EXE", eob / "INTRO.EXE")
    save = ROOT / "tests/sessions/fixtures/EOBDATA.SAV"
    shutil.copy(save, eob / "EOBDATA.SAV")
    import eobfmt
    (eob / "LEVELS.TMP").write_bytes(eobfmt.save(save.read_bytes())["levels"])


def conf(run: Path, c: Path, typed: str, start: str = "eob.exe " + ARGS) -> str:
    return f"""[dosbox]
machine=tandy
memsize=640
captures={run / "cap"}
[cpu]
core=normal
cputype=8086
cycles=max
[render]
aspect=false
scaler=none
[sdl]
output=surface
[speaker]
tandy=on
[autoexec]
mount c "{c}"
c:
cd \\eob
{typed}dx-capture /v {start}
exit
"""


def record(name: str, build: Path, seconds: int, start: str = "eob.exe " + ARGS, keys: str = KEYS) -> Path:
    run = OUT / name
    if run.exists():
        shutil.rmtree(run)
    (run / "cap").mkdir(parents=True)
    game_dir(run / "c", build)
    typed = "autotype -w %d -p %d %s\n" % (WAIT, PACE, keys) if keys else ""
    cf = run / "dosbox.conf"
    cf.write_text(conf(run, run / "c", typed, start))
    env = dict(os.environ, SDL_VIDEODRIVER="dummy", SDL_AUDIODRIVER="dummy")
    subprocess.run(["dosbox-x", "-conf", str(cf), "-nopromptfolder", "-nogui", "-fastlaunch", "-silent",
                    "-time-limit", str(seconds)], env=env, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=False)
    return run


def frames(run: Path) -> list[Path]:
    avis = sorted((run / "cap").glob("*.avi"), key=lambda p: p.stat().st_size)
    if not avis:
        raise RuntimeError("%s: DOSBox-X wrote no video capture" % run.name)
    d = run / "frames"
    if d.exists():
        shutil.rmtree(d)
    d.mkdir()
    subprocess.run(["ffmpeg", "-hide_banner", "-loglevel", "error", "-i", str(avis[-1]), "-vf", "fps=4", str(d / "f%05d.png")],
                   check=True)
    return sorted(d.glob("*.png"))


def pixels(p: Path):
    from PIL import Image
    im = Image.open(p).convert("RGB")
    return im.size, im.tobytes()


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--seconds", type=int, default=100)
    ap.add_argument("--play", action="store_true", help="open a window on the Tandy machine (the game, your keys)")
    a = ap.parse_args(argv)
    if not (shutil.which("dosbox-x") and shutil.which("ffmpeg")):
        print("dbx: dosbox-x and ffmpeg are required (brew install dosbox-x ffmpeg)")
        return 77
    if a.play:
        run = OUT / "play"
        game_dir(run / "c", PORT / "build/game")
        cf = run / "dosbox.conf"
        cf.write_text(conf(run, run / "c", "").replace("dx-capture /v eob.exe", "eob.exe").replace("cycles=max", "cycles=fixed 700")
                      .replace("\nexit\n", "\n").replace("aspect=false", "aspect=true").replace("scaler=none", "scaler=normal3x"))
        return subprocess.run(["dosbox-x", "-conf", str(cf), "-nopromptfolder", "-fastlaunch"], check=False).returncode
    builds = {"ref": PORT / "build/ref", "game": PORT / "build/game"}
    for n, b in builds.items():
        if not (b / "EOB.EXE").exists():
            print("dbx: no build %s (make -C ports/dos game ref)" % n)
            return 1
    print("dbx: four DOSBox-X runs (machine=tandy, headless), %d s each ..." % a.seconds, flush=True)
    modes = {"game": ("eob.exe " + ARGS, KEYS), "intro": ("intro.exe 1 3 0 homie", "")}
    ok = True
    with ThreadPoolExecutor(4) as ex:
        done = {(m, n): ex.submit(record, "run-%s-%s" % (m, n), b, a.seconds, *modes[m]) for n, b in builds.items() for m in modes}
        dirs = {k: f.result() for k, f in done.items()}
    for mode in modes:
        every, steady = {}, {}
        for n in builds:
            fs = frames(dirs[(mode, n)])
            px = [pixels(f) for f in fs]
            every[n] = set(px)
            steady[n] = [(f, p) for i, (f, p) in enumerate(zip(fs, px))
                         if i >= STEADY and all(px[i - k] == p for k in range(1, STEADY + 1))]
            print("  %s/%s: %d frames, %d distinct" % (mode, n, len(fs), len(every[n])))
        distinct, seen = [], set()
        for f, p in steady["ref"]:
            if p not in seen:
                seen.add(p)
                distinct.append((f, p))
        sizes = {p[0] for _f, p in distinct}
        missing = [f for f, p in distinct if p not in every["game"]]
        good = len(distinct) >= 4 and not missing and all(s == (320, 200) for s in sizes)
        ok = ok and good
        print("  %-4s %s: %d steady screens of the reference; sizes %s; missing in the game: %s" % (
            "ok" if good else "FAIL", mode, len(distinct), sizes, [str(f) for f in missing[:5]]))
    print("dbx: %s" % ("PASS" if ok else "FAIL"))
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())

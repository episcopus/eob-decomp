#!/usr/bin/env python3
"""play.py: the game in a window on the port (playbook 80, M7; `make port-play`).

With no option it runs the application `make port` built, as a double click does: the game's files in the
bundle, the saves in the player's own directory. With --save FILE it plays that saved game instead, from
a scratch game directory whose saves are thrown away with it (never eob/ itself, AGENTS.md).

  play.py [--save FILE] [--no-mouse] [--no-sound]
"""
from __future__ import annotations

import argparse
import os
import shutil
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))
from port.replay import game_dir  # noqa: E402


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--save", type=Path)
    ap.add_argument("--no-mouse", action="store_true")
    ap.add_argument("--no-sound", action="store_true")
    a = ap.parse_args(argv)
    exe = ROOT / "build" / "port" / "eob-port"
    if a.save is None and not a.no_mouse and not a.no_sound:
        args = [str(exe)]
    else:
        play = ROOT / "build" / "port" / "play"
        gd = game_dir(play / "game", a.save)
        saves = play / "saves"
        if saves.exists():
            shutil.rmtree(saves)
        saves.mkdir()
        args = [str(exe), "--play", str(gd), "--saves", str(saves)] + (["--no-mouse"] if a.no_mouse else []) + (
            ["--no-sound"] if a.no_sound else [])
    print("port-play: %s %s" % (exe.relative_to(ROOT), " ".join(args[1:])), flush=True)
    os.execv(str(exe), args)


if __name__ == "__main__":
    sys.exit(main())

#!/usr/bin/env python3
"""play.py: the game in a window on the port (playbook 80, M7; `make port-play`).

Makes a game directory with the pinned files of eob/ (never eob/ itself, AGENTS.md), and with
--save FILE a saved game in it (EOBDATA.SAV and its LEVELS.TMP, as the replay serves one), then runs
`build/port/eob-port --play`. The port's DOS keeps the game's writes in memory: nothing is written
back to the directory.

  play.py [--save FILE] [--no-mouse] [--adlib]
"""
from __future__ import annotations

import argparse
import os
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))
from port.replay import game_dir  # noqa: E402


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--save", type=Path)
    ap.add_argument("--no-mouse", action="store_true")
    ap.add_argument("--adlib", action="store_true")
    a = ap.parse_args(argv)
    gd = game_dir(ROOT / "build" / "port" / "play" / "game", a.save)
    exe = ROOT / "build" / "port" / "eob-port"
    args = [str(exe), "--play", str(gd)] + (["--no-mouse"] if a.no_mouse else []) + (["--adlib"] if a.adlib else [])
    os.execv(str(exe), args)


if __name__ == "__main__":
    sys.exit(main())

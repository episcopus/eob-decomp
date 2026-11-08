#!/usr/bin/env python3
"""app.py: the port as an application to double click (playbook 80, M7; `make port`).

Builds `build/port/Eye of the Beholder.app`: the SDL3 program, its SDL3 library, and the game's files
copied from eob/ (with the saved game eob/ has, so "Load game in progress" has something to load on the
first run). The program finds them by its own place (platform_find_install, port/platform/files.c) and
keeps the player's saves in ~/Library/Application Support/Eye of the Beholder. It is built only on
macOS, under build/, which is never committed.
"""
from __future__ import annotations

import plistlib
import re
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))
from port.replay import game_dir  # noqa: E402

NAME = "Eye of the Beholder"
EXE = ROOT / "build" / "port" / "eob-port"
APP = ROOT / "build" / "port" / (NAME + ".app")


def sdl_of(exe: Path) -> tuple[str, Path]:
    """The name the program links SDL3 by, and the file."""
    for line in subprocess.run(["otool", "-L", str(exe)], capture_output=True, text=True, check=True).stdout.splitlines():
        m = re.match(r"\s+(\S*libSDL3\S*\.dylib)", line)
        if m:
            return m.group(1), Path(m.group(1))
    raise SystemExit("app: %s does not link SDL3" % exe)


def main() -> int:
    if sys.platform != "darwin" or not EXE.exists():
        print("app: no application built (needs macOS and build/port/eob-port)")
        return 0
    contents = APP / "Contents"
    if APP.exists():
        shutil.rmtree(APP)
    (contents / "MacOS").mkdir(parents=True)
    (contents / "Frameworks").mkdir()
    (contents / "Resources").mkdir()
    exe = contents / "MacOS" / "eob-port"
    shutil.copy2(EXE, exe)
    linked, lib = sdl_of(EXE)
    bundled = contents / "Frameworks" / lib.name
    shutil.copy2(lib.resolve(), bundled)
    bundled.chmod(0o755)
    subprocess.run(["install_name_tool", "-id", "@rpath/" + lib.name, str(bundled)], check=True, capture_output=True)
    subprocess.run(["install_name_tool", "-change", linked, "@executable_path/../Frameworks/" + lib.name, str(exe)],
                   check=True, capture_output=True)
    save = ROOT / "eob" / "EOBDATA.SAV"
    game_dir(contents / "Resources" / "game", save if save.exists() else None)
    with open(contents / "Info.plist", "wb") as f:
        plistlib.dump({
            "CFBundleName": NAME, "CFBundleDisplayName": NAME, "CFBundleIdentifier": "local.eob-decomp.port",
            "CFBundleExecutable": "eob-port", "CFBundlePackageType": "APPL", "CFBundleVersion": "1",
            "CFBundleShortVersionString": "1.0", "NSHighResolutionCapable": True, "LSMinimumSystemVersion": "11.0",
        }, f)
    for target in (bundled, exe):
        subprocess.run(["codesign", "--force", "--sign", "-", str(target)], check=True, capture_output=True)
    subprocess.run(["codesign", "--force", "--sign", "-", str(APP)], check=True, capture_output=True)
    print("app: %s" % APP.relative_to(ROOT))
    return 0


if __name__ == "__main__":
    sys.exit(main())

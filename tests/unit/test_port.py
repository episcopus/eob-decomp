"""The portable build's skeleton (SPEC O5, playbook 80): tools/port/tc2c.py translates the units of
port/units.txt from src/ without touching them, the C compiler links them with the arena runtime and
the SDL3 host, and the program's self-test runs the translated functions on EOB.EXE's own DGROUP.
A unit with a construct tc2c does not handle is refused by name, never translated by a guess.
Skipped where clang or SDL3 is missing (the playbook says what to install)."""
import re
import shutil
import subprocess
import sys
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parents[2]
TC2C = ROOT / "tools" / "port" / "tc2c.py"

needs_clang = pytest.mark.skipif(shutil.which("clang") is None, reason="clang is not installed")
needs_sdl3 = pytest.mark.skipif(
    shutil.which("pkg-config") is None
    or subprocess.run(["pkg-config", "--exists", "sdl3"]).returncode != 0,
    reason="SDL3 is not installed (pkg-config sdl3)")


@needs_clang
@needs_sdl3
def test_the_skeleton_builds_and_passes_its_self_test():
    before = subprocess.run(["git", "status", "--porcelain", "src"], cwd=ROOT, capture_output=True, text=True).stdout
    r = subprocess.run(["make", "--no-print-directory", "port", "PY=%s" % sys.executable], cwd=ROOT,
                       capture_output=True, text=True)
    assert r.returncode == 0, r.stdout[-3000:] + r.stderr[-3000:]
    assert "8 lists: ok" in r.stdout
    # platform separation: the self-test ran on the headless program, which links no SDL
    assert "eob-port-headless --self-test" in r.stdout
    assert (ROOT / "build" / "port" / "eob-port-headless").exists()
    after = subprocess.run(["git", "status", "--porcelain", "src"], cwd=ROOT, capture_output=True, text=True).stdout
    assert before == after, "the port build changed src/"


def test_only_the_platform_directory_uses_sdl():
    """platform.h is the only boundary (playbook 80, "Platform separation"): the game, the runtime and
    the machine never include or call SDL, so they build for any host."""
    offenders = []
    for f in (ROOT / "port").rglob("*"):
        if f.suffix in (".c", ".h") and "platform" not in f.relative_to(ROOT / "port").parts[:1]:
            if re.search(r"#include *<SDL|SDL_[A-Za-z]", f.read_text()):
                offenders.append(str(f.relative_to(ROOT)))
    assert offenders == []


@needs_clang
def test_an_unhandled_construct_is_refused_by_name(tmp_path):
    """A unit taking a local's address needs the arena stack, which tc2c does not have yet."""
    unit = tmp_path / "X.C"
    unit.write_text("void Use(int far *p); /*@ 1234:0000 */\n"
                    "/*@ 1235:0000 */ void F(void)\n{\n    int x;\n    Use(&x);\n}\n")
    r = subprocess.run([sys.executable, str(TC2C), "--out", str(tmp_path / "gen"), str(unit)],
                       capture_output=True, text=True)
    assert r.returncode == 2
    assert "address of a local" in r.stderr

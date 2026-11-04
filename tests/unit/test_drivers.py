"""The graphics drivers (SPEC O1 since 0.2, whitepaper 2.5): each src/drivers/<NAME>.ASM assembles with TASM 2.0
and links with TLINK 3.0 to eob/<NAME>.OVL, every byte of the file; a source with one wrong constant is
rejected at its line. Each build takes well under a second, so these run in the fast set."""
import os
import re
import subprocess
import sys
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parents[2]
TOOL = ROOT / "tools" / "rebuild" / "driver.py"
SOURCES = sorted((ROOT / "src" / "drivers").glob("*.ASM"))


def run(*args):
    return subprocess.run([sys.executable, str(TOOL), *args], cwd=ROOT, capture_output=True, text=True)


@pytest.mark.parametrize("source", SOURCES, ids=[p.stem for p in SOURCES])
def test_driver_rebuilds_identical(source):
    r = run(source.stem + ".OVL")
    assert r.returncode == 0, r.stdout[-2000:] + r.stderr[-1000:]
    assert "the whole file is identical" in r.stdout


def test_a_wrong_constant_is_rejected_at_its_line(tmp_path):
    """MCGA.OVL's ClearPage with one word fewer: the tool reports the image offset and the source line."""
    src = (ROOT / "src" / "drivers" / "MCGA.ASM").read_text()
    bad, n = re.subn(r"(ClearPage\tPROC\tNEAR(?:\n.*?)*?\tmov\tcx, )PAGE_WORDS", r"\1PAGE_WORDS - 1", src, count=1)
    assert n == 1
    f = tmp_path / "MCGA.ASM"
    f.write_text(bad)
    r = run("MCGA.OVL", "--source", str(f))
    assert r.returncode == 1, r.stdout
    assert re.search(r"image 06A3: .*mov cx, PAGE_WORDS - 1", r.stdout), r.stdout


def test_a_missing_source_is_an_error():
    r = run("VGA.OVL")
    assert r.returncode == 2

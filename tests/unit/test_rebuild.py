"""The rebuild (Phase 5, playbook 50): a program built from src/ is the original, byte for byte."""
import os
import subprocess
import sys

import pytest

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))


@pytest.mark.slow
def test_start1_rebuilds_identical():
    """START1.EXE from src/S_TEXT.C, src/S_TEXT.ASM, the generated data, C0S and CS.LIB: every byte of
    the file, header included (2026-11-03)."""
    r = subprocess.run([sys.executable, os.path.join(ROOT, "tools/rebuild/rebuild.py"), "START1.EXE"],
                       cwd=ROOT, capture_output=True, text=True)
    assert r.returncode == 0, r.stdout[-2000:] + r.stderr[-1000:]
    assert "the whole file is identical" in r.stdout

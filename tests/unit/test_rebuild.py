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


@pytest.mark.slow
def test_intro_rebuilds_but_stub_order_and_date():
    """INTRO.EXE from src/: the relocation table identical (order included), and the whole file differs
    only in the link date and the overlay stub order, which follows the original function names
    (playbook 50, 2026-11-03): at most 42 bytes, all in one far call and the overlay segments."""
    r = subprocess.run([sys.executable, os.path.join(ROOT, "tools/rebuild/rebuild.py"), "INTRO.EXE"],
                       cwd=ROOT, capture_output=True, text=True)
    assert "relocations 806, original 806" in r.stdout, r.stdout[-2000:] + r.stderr[-1000:]
    a = open(os.path.join(ROOT, "eob", "INTRO.EXE"), "rb").read()
    b = open(os.path.join(ROOT, "build", "rebuild", "intro", "INTRO.EXE"), "rb").read()
    assert len(a) == len(b)
    head = int.from_bytes(a[8:10], "little") * 16
    diffs = [k - head for k in range(len(a)) if a[k] != b[k]]
    assert len(diffs) <= 42
    assert all(k == 0x8A82 or 0xE930 <= k < 0xE9F0 for k in diffs), ["%05X" % k for k in diffs[:8]]

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


@pytest.mark.slow
def test_eob_rebuilds_but_stub_order_date_and_hook():
    """EOB.EXE from src/ (2026-11-03): the load image's size, every relocation in the original's order,
    each overlay's code to its length. What differs has three causes (playbook 50, 'Left on EOB.EXE'):
    the link date (3 bytes), the stub entries TLINK numbers from the original function names (the stub
    table, far pointers to it, and far calls in the overlays), and overlay 5's three fixups of a far
    pointer to its own MessagePanel through its stub."""
    r = subprocess.run([sys.executable, os.path.join(ROOT, "tools/rebuild/rebuild.py"), "EOB.EXE"],
                       cwd=ROOT, capture_output=True, text=True)
    out = r.stdout
    assert "image 201552 bytes, original 201552; relocations 2504, original 2504" in out, out[-2000:] + r.stderr[-1000:]
    assert "the relocation table is in the original's order" in out
    assert ("of the image's differing bytes, 26 are stub entries in far pointers, 234 in the stub table, "
            "3 elsewhere (20F34 20F35 20F36;") in out
    lines = [l for l in out.splitlines() if l.startswith("rebuild: overlay ")]
    assert len(lines) == 7, lines
    for l in lines:
        assert "code identical" in l or "code bytes differ, all stub entries" in l, l
        assert "fixups" not in l or l.startswith("rebuild: overlay 5:"), l
    assert "fixups only the original's at 167F 1BE4 1D5F, only ours at -" in lines[5]

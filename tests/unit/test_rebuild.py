"""The rebuild (Phase 5, playbook 50): a program built from src/ is the original, byte for byte."""
import functools
import os
import re
import subprocess
import sys

import pytest

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))


@functools.lru_cache(maxsize=None)
def rebuild(program: str) -> subprocess.CompletedProcess:
    """`make rebuild PROGRAM=program`, once per test run: the tests below and the golden replays share
    the build in build/rebuild/."""
    return subprocess.run([sys.executable, os.path.join(ROOT, "tools/rebuild/rebuild.py"), program],
                          cwd=ROOT, capture_output=True, text=True)


@pytest.mark.slow
def test_start1_rebuilds_identical():
    """START1.EXE from src/S_TEXT.C, src/S_TEXT.ASM, the generated data, C0S and CS.LIB: every byte of
    the file, header included (2026-11-03)."""
    r = rebuild("START1.EXE")
    assert r.returncode == 0, r.stdout[-2000:] + r.stderr[-1000:]
    assert "the whole file is identical" in r.stdout


def stub_classes(out: str, date: str) -> None:
    """What differs is of the kinds playbook 50 leaves: stub entries in far pointers and the stub table,
    whose numbering follows the function names (so a rename may change their counts, never their kind),
    and the three bytes of the link date. Every overlay's code is identical but for stub entries."""
    assert "the relocation table is in the original's order" in out, out[-2000:]
    assert re.search(r"of the image's differing bytes, \d+ are stub entries in far pointers, \d+ in the stub "
                     r"table, 3 elsewhere \(%s;" % date, out), out[-2000:]
    for l in (l for l in out.splitlines() if l.startswith("rebuild: overlay ")):
        assert "code identical" in l or "code bytes differ, all stub entries" in l, l


@pytest.mark.slow
def test_intro_rebuilds_but_stub_order_and_date():
    """INTRO.EXE from src/: the file's size and relocation table identical (order included); what
    differs is the link date and the overlay stub order, which follows the function names (playbook 50,
    2026-11-03)."""
    r = rebuild("INTRO.EXE")
    assert "relocations 806, original 806" in r.stdout, r.stdout[-2000:] + r.stderr[-1000:]
    a = open(os.path.join(ROOT, "eob", "INTRO.EXE"), "rb").read()
    b = open(os.path.join(ROOT, "build", "rebuild", "intro", "INTRO.EXE"), "rb").read()
    assert len(a) == len(b)
    stub_classes(r.stdout, "0E93C 0E93D 0E93E")
    assert len([l for l in r.stdout.splitlines() if l.startswith("rebuild: overlay ")]) == 1


@pytest.mark.slow
def test_eob_rebuilds_but_stub_order_date_and_hook():
    """EOB.EXE from src/ (2026-11-03): the load image's size, every relocation in the original's order,
    each overlay's code to its length. What differs has three causes (playbook 50, 'Left on EOB.EXE'):
    the link date (3 bytes), the stub entries TLINK numbers from the function names (the stub table,
    far pointers to it, and far calls in the overlays), and overlay 5's three fixups of a far pointer
    to its own MessagePanel through its stub."""
    r = rebuild("EOB.EXE")
    out = r.stdout
    assert "image 201552 bytes, original 201552; relocations 2504, original 2504" in out, out[-2000:] + r.stderr[-1000:]
    stub_classes(out, "20F34 20F35 20F36")
    lines = [l for l in out.splitlines() if l.startswith("rebuild: overlay ")]
    assert len(lines) == 7, lines
    for l in lines:
        assert "fixups" not in l or l.startswith("rebuild: overlay 5:"), l
    assert "fixups only the original's at 167F 1BE4 1D5F, only ours at -" in lines[5]


@pytest.mark.slow
def test_goldens_play_on_the_rebuilds():
    """Every golden session, recorded on the originals, replayed on the rebuilt EOB.EXE and INTRO.EXE
    (INTRO.EXE loads EOB.EXE, so both are served): the same instructions, checkpoints, screens, sound
    and state digests, the rebuild's memory read in the original's form (tools/oracle/layout.py: the
    stub entries in the original's order, the link date, the overlay data's offsets and sizes that
    follow from overlay 5's three fixups). Phase 5's exit (SPEC 10, playbook 50; 2026-11-04)."""
    if not os.path.exists(os.path.join(ROOT, "eob", "EOB.EXE")):
        pytest.skip("eob/ not present")
    for program in ("INTRO.EXE", "EOB.EXE"):
        r = rebuild(program)
        assert "relocations" in r.stdout, r.stdout[-2000:] + r.stderr[-1000:]
    sys.path.insert(0, os.path.join(ROOT, "tools"))
    from oracle import golden
    assert golden.main(["--check", "--rebuilt"]) == 0

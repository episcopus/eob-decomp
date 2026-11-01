"""eobfmt.py: the game's own data files, the test save, and LEVELS.TMP as the game writes it."""
import os, struct, sys
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools" / "re"))
import eobfmt  # noqa: E402

SAVE = ROOT / "tests" / "sessions" / "fixtures" / "EOBDATA.SAV"


def test_text_and_records():
    d = struct.pack("<2H", 4, 7) + b"ab\0cd\0"
    assert eobfmt.text(d) == [b"ab", b"cd"]
    with pytest.raises(eobfmt.FormatError):
        eobfmt.item_types(struct.pack("<H", 2) + b"\0" * 16)


def test_every_pinned_file():
    if not (ROOT / "eob").is_dir():
        pytest.skip("eob/ not present")
    assert eobfmt.verify(str(ROOT / "eob")) == 0


def test_fonts():
    if not (ROOT / "eob").is_dir():
        pytest.skip("eob/ not present")
    f = eobfmt.font((ROOT / "eob" / "FONT6.FNT").read_bytes())
    assert (f["height"], f["width"]) == (6, 6)
    assert f["glyphs"][ord("A")] == bytes([0x70, 0x88, 0xF8, 0x88, 0x88, 0x00])


def test_save_round_trip():
    d = SAVE.read_bytes()
    s = eobfmt.save(d)
    assert eobfmt.save_bytes(s) == d
    assert s["names"] == [b"KIMIL", b"IMMAR", b"ZANDI'", b"AZALIS"]


@pytest.mark.slow
def test_the_game_writes_the_level_states_of_the_save():
    """Loading the test save makes the game delete LEVELS.TMP and write it again: the save's 12 level
    states, in order. (The run starts with LEVELS.TMP as the game leaves it after saving, run.use_save;
    the game needs one, and what it writes must be the same.)"""
    if not (ROOT / "eob").is_dir():
        pytest.skip("eob/ not present")
    sys.path.insert(0, str(ROOT / "tools"))
    from oracle.run import build_machine, use_save
    from oracle.input import Script
    m = build_machine()
    use_save(m, SAVE)
    m.vfs.layer["\\EOB\\LEVELS.TMP"][:] = bytes(24480)      # what the game writes must come from the save
    m.keys = Script("wait 600\nkey ENTER\nwait 300\nquit\n")
    m.run(60_000_000)
    assert ("create", "\\EOB\\LEVELS.TMP", None) in m.vfs.log
    written = bytes(m.vfs.layer["\\EOB\\LEVELS.TMP"])
    assert eobfmt.levels(written) == eobfmt.save(SAVE.read_bytes())["level_states"]

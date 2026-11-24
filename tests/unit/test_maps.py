"""tools/maps: the level decoder and the script analysis behind the player maps (docs/maps/)."""
import sys
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools" / "maps"))

pytestmark = pytest.mark.skipif(not (ROOT / "eob").is_dir(), reason="eob/ not present")


def test_every_script_byte_decodes():
    """the twelve level files: every script ends where the next begins and the last at the trigger
    table (Level raises otherwise); the totals are the whitepaper's (5.5.1, 5.5.2, 5.3.7)"""
    import level as lv
    triggers = instructions = level_changes = 0
    for n in range(1, 13):
        L = lv.Level(n)
        triggers += len(L.triggers)
        instructions += len(L.code)
        level_changes += sum(1 for i in L.code.values() if i.op == 0xEC)
    assert (triggers, instructions, level_changes) == (871, 7054, 125)


def test_items_and_names():
    """350 items lie on the maps (whitepaper 5.3.4); names follow DrawItemName"""
    import level as lv
    placed = [r for r in lv.items()["records"] if 1 <= r["level"] <= 12 and r["key"] > 0]
    assert len(placed) == 350
    assert lv.item_name(49) == "Dagger +4 'Guinsoo'"


def test_events_read_as_the_scripts_say():
    import analyze
    import level as lv
    one = {e.cell: e for e in analyze.events(lv.Level(1))}
    assert one[754].kind == "down" and one[754].dest == (2, 755)     # (18,23): stairs to level 2
    assert one[129].kind == "lock" and "Silver Key" in one[129].title
    seven = [e for e in analyze.events(lv.Level(7)) if e.kind == "portal"]
    assert sorted(e.dest[0] for e in seven) == [2, 4, 5, 9, 11]       # the hub (whitepaper 5.3.7)
    five = [e for e in analyze.events(lv.Level(5)) if e.kind == "pit"]
    assert len(five) == 41                                           # the hole of level 5

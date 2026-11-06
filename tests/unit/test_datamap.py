"""tools/rebuild/datamap.py, the anchors of step D0 (playbook 80, "Step D"): the facts the plan cites."""
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools" / "rebuild"))
import datamap  # noqa: E402


def test_initialised_data_has_969_relocations():
    relocs = datamap.relocations(datamap.EXE.read_bytes())
    dg = datamap.DGROUP * 16
    assert sum(1 for x in relocs if 0x01FE <= x - dg < 0x84B2) == 969


def test_pools_bound_the_units_whose_names_are_their_own():
    pools = datamap.pools()
    names = datamap.declared()
    order = sorted((v[0], v[1], u) for u, v in pools.items() if 0x01FE <= v[0] < 0x84B2)
    clean = set()
    for (_, prev_end, _), (start, _, u) in zip(order, order[1:]):
        between = [a for a in names if prev_end <= a < start]
        if between and all(u in names[a] for a in between):
            clean.add(u)
    assert {"e08e6", "e0a72", "e0acd", "e0bd2", "e1547", "eov2", "eov3"} <= clean


def test_main_runs(capsys):
    assert datamap.main([]) == 0
    out = capsys.readouterr().out
    assert "ds:3408-35D4 reloc e0a72" in out

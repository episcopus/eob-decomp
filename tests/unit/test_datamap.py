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
    # (not e1547: E14EF's gHoverSelectsOption, ds:5D1A, lies between E1414's pool and E1547's names)
    assert {"e08e6", "e0a72", "e0acd", "e0bd2", "eov2", "eov3"} <= clean


def test_declared_reads_annotations_with_text_after_the_address():
    # `/*@ ds:5D1A: nonzero lets ...` (E14EF.C) and `; @ ds:80EA: the driver's option bits` (E1AAD.ASM)
    names = datamap.declared()
    assert "e14ef" in names[0x5D1A] and "e1aad" in names[0x80EA]


def test_no_boundary_splits_a_far_pointer():
    # a far pointer is an offset word and its relocated segment word: a cut never falls between them
    import tomllib
    dg = datamap.DGROUP * 16
    rel = {x - dg for x in datamap.relocations(datamap.EXE.read_bytes())}
    for m in tomllib.loads((ROOT / "re/datamap.toml").read_text())["module"]:
        s = int(m["start"][3:], 16)
        assert not {s - 1, s, s + 1} & rel, m["id"]


def test_main_runs(capsys):
    assert datamap.main([]) == 0
    out = capsys.readouterr().out
    assert "ds:3408-35D4 reloc e0a72" in out


def _rows():
    import tomllib
    return tomllib.loads((ROOT / "re/datamap.toml").read_text())["module"]


def test_map_covers_the_program_data_without_gap_or_overlap():
    rows = _rows()
    iv = [(int(r["start"][3:], 16), int(r["end"][3:], 16)) for r in rows]
    assert iv[0][0] == 0x0090 and iv[-1][1] == 0x84B2       # from C0's _DATA (ds:008F, word aligned) to the library's
    assert all(a[1] == b[0] for a, b in zip(iv, iv[1:]))
    assert all(a % 2 == 0 and a < b for a, b in iv)


def test_map_settles_each_boundary_or_says_why_it_is_free():
    rows = _rows()
    for r in rows:
        assert r["start_by"] in ("settled", "settled [H]", "touching", "free")
    rel = [x - datamap.DGROUP * 16 for x in datamap.relocations(datamap.EXE.read_bytes())]
    for r in rows:                                    # a free boundary has no relocation the image could tell by
        if r["start_by"] == "free":
            m = __import__("re").search(r"between ds:([0-9A-F]{4}) and ds:([0-9A-F]{4})", r["start_why"])
            a, b = int(m.group(1), 16), int(m.group(2), 16)
            assert not any(a <= x < b for x in rel), r["id"]


def test_map_is_what_the_tool_writes():
    import pytest
    if not (ROOT / "build/re/eob.json").exists():
        pytest.skip("needs make overlays (the segment table)")
    import tempfile
    with tempfile.TemporaryDirectory() as t:
        p = Path(t) / "datamap.toml"
        datamap.write_toml(datamap.resolve(0x008F, 0x84B2), p)
        assert p.read_text() == (ROOT / "re/datamap.toml").read_text()

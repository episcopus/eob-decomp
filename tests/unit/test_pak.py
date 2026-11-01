"""pak.py: byte-exact round trip over every pinned archive, and the directory rules."""
import glob, os, struct, sys

import pytest

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(ROOT, "tools", "re"))
import pak  # noqa: E402

PAKS = sorted(glob.glob(os.path.join(ROOT, "eob", "*.PAK")))


@pytest.mark.parametrize("path", PAKS, ids=[os.path.basename(p) for p in PAKS])
def test_round_trip(path):
    d = open(path, "rb").read()
    p = pak.read_pak(d)
    assert len(p["terminator"]) == 4
    assert pak.build_pak(p["entries"], p["terminator"]) == d


def test_file_count():
    if not PAKS:
        pytest.skip("eob/ not present")
    assert sum(len(pak.read_pak(open(p, "rb").read())["entries"]) for p in PAKS) == 193


def test_synthetic_and_empty_name_terminator():
    files = [{"name": "A.DAT", "data": b"abc"}, {"name": "B.CPS", "data": b""}]
    d = pak.build_pak(files, struct.pack("<I", 0) + b"\0")
    p = pak.read_pak(d)
    assert [(e["name"], e["data"]) for e in p["entries"]] == [("A.DAT", b"abc"), ("B.CPS", b"")]


def test_rejects_descending_offsets():
    d = bytearray(pak.build_pak([{"name": "A", "data": b"xx"}, {"name": "B", "data": b"y"}], b"\0" * 4))
    struct.pack_into("<I", d, 6, 1)  # B's offset before A's
    with pytest.raises(ValueError):
        pak.read_pak(bytes(d))

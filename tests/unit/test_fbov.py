"""fbov.py over the pinned programs, plus synthetic cases for what it must reject."""
import os, struct, sys

import pytest

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(ROOT, "tools", "re"))
import fbov  # noqa: E402


def load(name):
    p = os.path.join(ROOT, "eob", name)
    if not os.path.exists(p):
        pytest.skip("eob/ not present")
    return open(p, "rb").read()


@pytest.mark.parametrize("name,segments,overlays,entries,code", [
    ("EOB.EXE", 99, 7, 125, 50641),
    ("INTRO.EXE", 54, 1, 22, 11741),
])
def test_pinned_manifest(name, segments, overlays, entries, code):
    m, errors = fbov.manifest(load(name))
    assert errors == []
    assert m["tlink"] == 0x30
    assert (len(m["segments"]), len(m["overlays"])) == (segments, overlays)
    assert (m["overlay_entries"], m["overlay_code_bytes"]) == (entries, code)


def test_eob_dgroup_and_stubs():
    m, _ = fbov.manifest(load("EOB.EXE"))
    stubs = [o["stub_para"] for o in m["overlays"]]
    assert stubs == [0x20F6, 0x2102, 0x2106, 0x210C, 0x2116, 0x211E, 0x212A]
    # the startup code loads DS from the paragraph after the last stub (whitepaper 2.2)
    assert any(s["para"] == 0x212E and s["kind"] == 0 for s in m["segments"])


def test_start1_has_no_overlays():
    m, errors = fbov.manifest(load("START1.EXE"))
    assert errors == [] and m["fbov"] is None


def test_rejects_bad_size_and_padding():
    d = bytearray(load("EOB.EXE"))
    mz_end = len(d) - fbov.manifest(bytes(d))[0]["fbov"]["bytes"] - 16
    bad = bytearray(d)
    struct.pack_into("<I", bad, mz_end + 4, 1)
    assert any("FBOV size" in e for e in fbov.manifest(bytes(bad))[1])
    bad = bytearray(d)
    bad[-1] = 0xAA  # the zero padding after the last overlay
    assert any("after the last overlay" in e for e in fbov.manifest(bytes(bad))[1])

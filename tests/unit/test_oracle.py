"""The oracle runs the original EOB.EXE to its title menu, the same way every time."""
import json, os, sys

import pytest

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(ROOT, "tools"))


@pytest.mark.slow
def test_boot_to_title_is_deterministic(tmp_path):
    if not os.path.exists(os.path.join(ROOT, "eob", "EOB.EXE")):
        pytest.skip("eob/ not present")
    from oracle import run
    digests = []
    for k in range(2):
        out = tmp_path / str(k)
        assert run.main(["--max-insns", "30000000", "--out", str(out)]) == 0
        s = json.loads((out / "summary.json").read_text())
        assert s["video_mode"] == "13h" and s["stop"] == "max-insns"
        assert "\\EOB\\EOBDATA1.PAK" in s["files"] and "\\EOB\\MCGA.OVL" in s["files"]
        assert s["ints"].get("3F", 0) >= 1          # the overlay manager loaded an overlay
        digests.append(s["digest"])
    assert digests[0] == digests[1]

"""The agent process tools (tools/agent/, tools/match/gate.py): their rules, without git side effects.
The whole loop (claim, hand-off, integrate through make check, review, worker) was rehearsed in a
throwaway clone (docs/process/playbooks/90-orchestrate.md)."""
import shutil
import sys
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools" / "agent"))
sys.path.insert(0, str(ROOT / "tools" / "match"))
import common  # noqa: E402
import next as nxt  # noqa: E402
import safety  # noqa: E402
import skeleton  # noqa: E402
import worker  # noqa: E402


def test_identifiers_are_names():
    assert common.identifier("e0cf7") == "e0cf7"
    for bad in ("../x", "E0CF7", "-rf", "", "a b"):
        with pytest.raises(ValueError):
            common.identifier(bad)


def test_safety_rules():
    assert safety.findings(["eob/EOB.EXE"], [], "task/x")                      # never, on any branch
    assert safety.findings(["tools/match/cmatch.py"], [], "unit/e0cf7")         # not from a unit branch
    assert safety.findings(["tests/match/negative/e0cf7-constant.C"], [], "review/e0cf7")
    assert not safety.findings(["tools/match/cmatch.py"], [], "task/matcher")   # a task the human asked for
    assert not safety.findings(["src/E0CF7.C", "re/symbols.toml"], [], "unit/e0cf7")
    assert safety.findings([], [("work/x.md", "a — b")], "main")
    assert not safety.findings([], [("work/x.md", "a, b; c (d)")], "main")


def test_queue_is_game_code_smallest_first():
    c = nxt.candidates()
    ids = [u["id"] for u in c]
    assert "e_lib" not in ids and "e_c0" not in ids and not any(i.startswith("i") for i in ids)
    assert [u["bytes"] for u in c] == sorted(u["bytes"] for u in c)
    assert "e0cf7" in ids and "eov2" in ids


def test_skeleton_has_a_stub_per_function():
    for uid in ("e082c", "e1a63", "eov2"):
        ext, text = skeleton.text(uid)
        fs = common.unit_functions(uid)
        assert ext == (".ASM" if common.unit(uid)["kind"] == "asm" else ".C")
        for f in fs:
            assert ("@ %s stub" % f["addr"]) in text
    assert "/*@ ovl2:0110 stub */ void LoadItemTypes(void) {}" in skeleton.text("eov2")[1]


def test_worker_prompt():
    p = worker.prompt("w1", "reviewer", "review-e0cf7", "review/e0cf7", Path("/tmp/wt"))
    assert "UNIT=e0cf7 STATE=<review-passed|review-failed>" in p and "70-review.md" in p
    p = worker.prompt("w1", "decompiler", "e0cf7", "unit/e0cf7", Path("/tmp/wt"))
    assert "make packet UNIT=e0cf7" in p and "{" not in p


def test_the_gate_fails_on_a_fixture_that_matches(tmp_path, monkeypatch, capsys):
    """A negative fixture that is not rejected tests nothing: the gate must fail on it."""
    if not (ROOT / "build/toolchain").exists():
        pytest.skip("toolchain not installed (make tc-install)")
    import gate
    shutil.copy(ROOT / "src/E19EA.ASM", tmp_path / "e19ea-unchanged.ASM")
    monkeypatch.setattr(gate, "NEGATIVE", tmp_path)
    assert gate.main([]) == 1
    assert "e19ea-unchanged.ASM was not rejected" in capsys.readouterr().out

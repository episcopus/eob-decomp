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


import subprocess  # noqa: E402

import merge_registry  # noqa: E402
import run as run_mod  # noqa: E402

REGISTRY = '''# header

[[function]]
id = "a"
addr = "0001:0000"

[[function]]
id = "b"
addr = "0001:0010"

[[function]]
id = "c"
addr = "0001:0020"
'''


def test_merge_registry_rules():
    ours = REGISTRY.replace('addr = "0001:0000"\n', 'addr = "0001:0000"\nname = "A"\nlevel = "L2"\n')
    theirs = REGISTRY.replace('addr = "0001:0010"\n', 'addr = "0001:0010"\nname = "B"\nlevel = "L2"\n')
    out, conflicts = merge_registry.merge(REGISTRY, ours, theirs)
    assert not conflicts and 'name = "A"' in out and 'name = "B"' in out
    assert merge_registry.merge(REGISTRY, REGISTRY, REGISTRY) == (REGISTRY, [])
    other = REGISTRY.replace('addr = "0001:0000"\n', 'addr = "0001:0000"\nname = "Z"\n')
    out, conflicts = merge_registry.merge(REGISTRY, ours, other)
    assert conflicts == ['"a"'] and "<<<<<<< ours" in out
    n1 = REGISTRY.replace('addr = "0001:0020"\n', 'addr = "0001:0020"\nnotes = "x"\n')
    n2 = REGISTRY.replace('addr = "0001:0020"\n', 'addr = "0001:0020"\nnotes = "y"\n')
    assert 'notes = "x; y"' in merge_registry.merge(REGISTRY, n1, n2)[0]
    added = REGISTRY + '\n[[function]]\nid = "d"\naddr = "0001:0030"\n'
    out, _ = merge_registry.merge(REGISTRY, ours, added)
    assert out.index('id = "c"') < out.index('id = "d"') and 'name = "A"' in out


def test_merge_driver_through_git(tmp_path):
    """One branch matches a function, another notes something about it: git's line merge conflicts
    (both append to the same entry), the driver merges the fields."""
    def g(*a):
        return subprocess.run(["git", *a], cwd=tmp_path, capture_output=True, text=True)
    g("init", "-q", "-b", "main")
    g("config", "user.email", "t@example.com")
    g("config", "user.name", "t")
    (tmp_path / ".gitattributes").write_text("symbols.toml merge=eob-registry\n")
    (tmp_path / "symbols.toml").write_text(REGISTRY)
    g("add", "-A"), g("commit", "-qm", "base")
    g("checkout", "-qb", "one")
    (tmp_path / "symbols.toml").write_text(REGISTRY.replace('addr = "0001:0000"\n', 'addr = "0001:0000"\nname = "A"\nlevel = "L2"\n'))
    g("commit", "-qam", "one")
    g("checkout", "-q", "main")
    (tmp_path / "symbols.toml").write_text(REGISTRY.replace('addr = "0001:0000"\n', 'addr = "0001:0000"\nnotes = "three words pushed"\n'))
    g("commit", "-qam", "two")
    assert g("merge", "-q", "--no-edit", "one").returncode != 0          # without the driver
    g("merge", "--abort")
    g("config", "merge.eob-registry.driver", "%s %s %%O %%A %%B" % (sys.executable, ROOT / "tools/agent/merge_registry.py"))
    r = g("merge", "-q", "--no-edit", "one")
    assert r.returncode == 0, r.stdout + r.stderr
    text = (tmp_path / "symbols.toml").read_text()
    assert 'level = "L2"' in text and 'notes = "three words pushed"' in text


def test_roster_and_commands():
    run, entries = run_mod.roster()
    assert {e["role"] for e in entries} <= set(run_mod.ROLES)
    for e in entries:
        cmd = run_mod.command(run, e)
        assert cmd[1] == ("tools/agent/integrator.py" if e["role"] == "integrator" else "tools/agent/worker.py")
        if e["role"] != "integrator":
            assert cmd[cmd.index("--agent") + 1] == e["name"] and "--session-cmd" in cmd

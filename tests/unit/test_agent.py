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
    bad = {"re/symbols.toml": 'notes = "form \\`and ax,1\\`"\n'}            # the e122e note of 2026-11-02
    assert "not valid TOML" in safety.findings(["re/symbols.toml"], [], "unit/e122e", bad.get)[0]
    good = {"re/symbols.toml": 'notes = "form `and ax,1`, a \\\\ b"\n'}
    assert not safety.findings(["re/symbols.toml"], [], "unit/e122e", good.get)
    assert not safety.findings(["re/old.toml"], [], "unit/e122e", {}.get)      # deleted


def test_queue_is_game_code_smallest_first():
    c = nxt.candidates()
    ids = [u["id"] for u in c]
    assert not {"e_lib", "e_c0", "i_lib", "i_c0"} & set(ids)
    eob = [u for u in c if u["program"] == "EOB.EXE"]
    assert c[:len(eob)] == eob                                  # EOB.EXE first, then INTRO.EXE
    for part in (eob, c[len(eob):]):
        assert [u["bytes"] for u in part] == sorted(u["bytes"] for u in part)
    assert "e0cf7" in ids and "eov2" in ids
    assert not any(u.get("twin") for u in c) and "i0892" in ids  # a twin is proven, not decompiled again


def test_skeleton_has_a_stub_per_function():
    for uid in ("e082c", "e1a63", "eov2"):
        ext, text = skeleton.text(uid)
        fs = common.unit_functions(uid)
        assert ext == (".ASM" if common.unit(uid)["kind"] == "asm" else ".C")
        for f in fs:
            assert ("@ %s stub" % f["addr"]) in text
    assert "/*@ ovl2:0110 stub */ void LoadItemTypes(void) {}" in skeleton.text("eov2")[1]


def test_safety_merge_of_main(tmp_path, monkeypatch):
    """A unit branch that merges main stages main's edits to the matcher: only what the branch itself
    changes is judged (the e_text merge of 2026-11-02 was refused for main's tools/match/rename.py)."""
    import subprocess
    import common as cm

    def g(*a):
        subprocess.run(["git", *a], cwd=tmp_path, check=True, capture_output=True)
    g("init", "-q", "-b", "main")
    g("config", "user.email", "t@t"); g("config", "user.name", "t")
    (tmp_path / "tools/match").mkdir(parents=True)
    (tmp_path / "tools/match/rename.py").write_text("a\n")
    (tmp_path / "src").mkdir()
    (tmp_path / "src/X.C").write_text("x\n")
    g("add", "-A"); g("commit", "-qm", "base")
    g("checkout", "-qb", "unit/x")
    (tmp_path / "src/X.C").write_text("y\n")
    g("commit", "-qam", "unit")
    g("checkout", "-q", "main")
    (tmp_path / "tools/match/rename.py").write_text("b\n")
    g("commit", "-qam", "tool fix")
    g("checkout", "-q", "unit/x")
    g("merge", "-q", "--no-commit", "--no-ff", "main")
    monkeypatch.setattr(cm, "ROOT", tmp_path)
    assert safety.staged_findings() == []
    (tmp_path / "tools/match/rename.py").write_text("c\n")       # the branch's own edit is still refused
    g("add", "-A")
    assert safety.staged_findings()


def test_worker_prompt():
    p = worker.prompt("w1", "reviewer", "review-e0cf7", "review/e0cf7", Path("/tmp/wt"))
    assert "UNIT=e0cf7 STATE=<review-passed|review-failed>" in p and "70-review.md" in p
    import common as cm0
    reviews = cm0.ROOT / "work/reviews"
    later = next((p.stem for p in reviews.glob("e*.md")), None)
    if later:                                   # a unit reviewed before: a later round, not a full read
        assert "later round" in worker.prompt("w1", "reviewer", "review-" + later, "review/" + later, Path("/tmp/wt"))
    p = worker.prompt("w1", "decompiler", "e0cf7", "unit/e0cf7", Path("/tmp/wt"))
    assert "make packet UNIT=e0cf7" in p and "{" not in p
    import common as cm
    real = cm.open_must_fix
    try:
        cm.open_must_fix = lambda uid: ["- [ ] must-fix x"]
        assert "open must-fix findings" in worker.prompt("w1", "decompiler", "e0cf7", "unit/e0cf7", Path("/tmp/wt"))
    finally:
        cm.open_must_fix = real
    p = worker.prompt("w1", "decompiler", "e0cf7", "unit/e0cf7", Path("/tmp/wt"), "LEFT")
    assert "uncommitted changes" in p and "rejected" not in p.split("Then continue")[0]


def test_left_claims(tmp_path, monkeypatch):
    """A claim of this agent with no hand-off and a dirty worktree is its to retake; a clean one is not."""
    import common as cm
    locks, handoffs = tmp_path / "locks", tmp_path / "handoffs"
    locks.mkdir(); handoffs.mkdir()
    monkeypatch.setattr(cm, "state_dir", lambda n: locks if n == "eob-locks" else handoffs)
    for k in ("dirty", "clean", "other"):
        (locks / ("%s.lock" % k)).write_text("key = %s\nagent = %s\nbranch = unit/%s\n" % (k, "w9" if k == "other" else "w1", k))
        (tmp_path / k).mkdir()
    monkeypatch.setattr(cm, "worktree_of", lambda k: tmp_path / k)
    monkeypatch.setattr(cm, "git", lambda *a, cwd=None, check=True: " M src/X.C" if cwd and cwd.name in ("dirty", "other") else "")
    assert worker.left_claims("w1") == [("dirty", "LEFT")]


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
    base = REGISTRY.replace('addr = "0001:0020"\n', 'addr = "0001:0020"\nsize = 9\n')
    grew = REGISTRY.replace('addr = "0001:0020"\n', 'addr = "0001:0020"\nsize = 12\n')
    out, conflicts = merge_registry.merge(base, REGISTRY, grew)                # ours deleted the field
    assert conflicts and "None" not in out and "<<<<<<< ours\n=======\nsize = 12\n>>>>>>> theirs" in out


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


def test_roster_rules():
    """The roster's rules, whatever its entries (the human edits it): ids without #variant, a model
    wherever the session command passes one, tier 2 only for decompilers, one integrator at most."""
    good = {"run": {}, "session": [{"name": "d1", "role": "decompiler", "model": "p/m", "variant": "high"}]}
    bad = [
        {"name": "d1", "role": "decompiler", "model": "p/m#high"},
        {"name": "d1", "role": "decompiler"},
        {"name": "r1", "role": "reviewer", "model": "p/m", "tier": 2},
        {"name": "X", "role": "decompiler", "model": "p/m"},
    ]
    import tomllib
    real = tomllib.loads
    try:
        tomllib.loads = lambda text: good
        run, entries = run_mod.roster()
        cmd = run_mod.command(run, entries[0])
        assert cmd[cmd.index("--model") + 1] == "p/m" and cmd[cmd.index("--tier") + 1] == "1"
        for b in bad:
            tomllib.loads = lambda text, b=b: {"run": {}, "session": [b]}
            with pytest.raises(ValueError):
                run_mod.roster()
    finally:
        tomllib.loads = real
    run_mod.roster()                                   # the committed roster obeys them
    assert run_mod.program("cd {worktree} && opencode run --auto x") == "opencode"


def test_tier_selection(monkeypatch):
    """Tier 2 takes only units tier 1 gave up on; both giving up leaves the unit to the human. A queue
    of its own: the project's queue runs out as units complete (e0a72, 2026-11-02, left none)."""
    att = {}
    monkeypatch.setattr(common, "attempts", lambda: att)
    monkeypatch.setattr(nxt, "busy", lambda key: False)
    monkeypatch.setattr(common, "queue", lambda: ["u1", "u2"])
    monkeypatch.setattr(common, "functions", lambda: [])
    monkeypatch.setattr(common, "unit_complete", lambda uid, fs: False)
    monkeypatch.setattr(common, "unit_blocked", lambda uid, fs: False)
    monkeypatch.setattr(common, "open_must_fix", lambda uid: [])
    first = nxt.next_unit(1)
    assert first and nxt.next_unit(2) is None
    att[first] = {"1": common.TIER1_ATTEMPTS}
    assert nxt.next_unit(1) != first and nxt.next_unit(2) == first
    att[first]["2"] = 2
    assert nxt.next_unit(2, 2) is None and nxt.stuck(2) == [first]


def test_watch_screen_renders():
    import watch
    text = watch.screen(100)
    assert "== progress" in text and "functions " in text and "== hand-offs" in text
    assert "== costs (USD)" in text and "all runs" in text
    assert "functions " in text and "%" in text
    assert watch.span(0.5) == "30m" and watch.span(3.25) == "3h15m" and watch.span(50) == "2d02h"
    assert watch.pct(1, 4) == 25.0
    t = watch.tally([{"l2_before": 1, "l2_after": 4, "cost": 0.5}, {"l2_before": 2, "l2_after": 2, "cost": None}])
    assert t == {"sessions": 2, "gained": 3, "cost": 0.5, "unknown": 1}


sys.path.insert(0, str(ROOT / "tools" / "match"))
import readability  # noqa: E402
import rename  # noqa: E402


def test_rename_skips_literals_and_follows_comments():
    text = 'extern int gFE6C; /*@ ds:FE6C */\n/* gFE6C is the mode */\nchar *s = "gFE6C";\nf(gFE6C);\n'
    out, n = rename.replace_word(text, "gFE6C", "gVideoMode", asm=False)
    assert n == 3 and '"gFE6C"' in out and "/* gVideoMode is the mode */" in out
    assert rename.norm("ds:fe6c") == "ds:FE6C" and rename.norm("ovl2:110") == "ovl2:0110"


def test_readability_rules(tmp_path):
    good = ('/* Unit at EOB.EXE 1A16:000B: frame delays, waits for vertical retraces of the display. */\n\n'
            'extern int gMode; /*@ ds:FE6C */\n\n'
            '/* 1A16:000B: waits n vertical retraces of the display, polling port 3DAh. */\n'
            '/*@ 1A16:000B */ void WaitFrames(int n)\n{\n    int count;\n\n    count = n;\n}\n')
    bad = ('/* x */\n/*@ 1A16:000B */ void f1A16_000B(int a)\n{\n    int tmp;\n\n    tmp = a;\n}\n')
    (tmp_path / "GOOD.C").write_text(good)
    (tmp_path / "BAD.C").write_text(bad)
    assert readability.check_file(tmp_path / "GOOD.C", {}, {}) == []
    rules = {r for _, r, _ in readability.check_file(tmp_path / "BAD.C", {}, {})}
    assert rules == {"R1", "R2", "R3", "R4"}
    named = readability.check_file(tmp_path / "GOOD.C", {"ds:FE6C": "gVideoMode"}, {})
    assert [r for _, r, _ in named] == ["R5"]


def test_placeholders():
    for n in ("f19EC_0016", "Ovl2_0110", "g811B", "gFn80CA", "gRect806A", "gBuf8DCA", "Level07A9", "_f1BA8_0006"):
        assert rename.PLACEHOLDER.match(n), n
    for n in ("gFillRect", "LockShowPointer", "CheckMCB", "gTextAttr2", "WaitFrames", "gFACE", "ObjAlloc"):
        assert not rename.PLACEHOLDER.match(n), n


def test_apply_takes_names_from_definitions(tmp_path, monkeypatch):
    """A unit names its own function in its file; a file that calls it by a placeholder gets that name
    at integration, and two real names for one address are left for a person."""
    src = tmp_path / "src"
    src.mkdir()
    (src / "E1BBE.C").write_text('/*@ 1BBE:0332 */ void LockShowPointer(void)\n{\n}\n')
    (src / "E14EF.C").write_text('extern void f1BBE_0332(void); /*@ 1BBE:0332 */\nvoid g(void) { f1BBE_0332(); }\n')
    (src / "E1AAA.C").write_text('/*@ 1AAA:0005 */ int RandomRange(int lo)\n{\n    return lo;\n}\n')
    (src / "E1DE7.C").write_text('extern char gTextAttr2; /*@ ds:811B */\n')
    (src / "E1A16.C").write_text('extern char g811B; /*@ ds:811B */\nvoid h(void) { g811B = 1; }\n')
    (src / "E0001.C").write_text('extern int PickNumber(int lo); /*@ 1AAA:0005 */\n')
    monkeypatch.setattr(rename, "SRC", src)
    monkeypatch.setattr(rename, "LOG", tmp_path / "renames.toml")
    rename.apply()
    assert "LockShowPointer();" in (src / "E14EF.C").read_text()
    assert "PickNumber" in (src / "E0001.C").read_text()          # a real name: not overwritten
    assert "gTextAttr2 = 1;" in (src / "E1A16.C").read_text()     # a global: one real name wins


def test_blocked_units_leave_the_queue(monkeypatch):
    fs = [{"unit": "e0001", "addr": "0001:0000", "level": "L2"},
          {"unit": "e0001", "addr": "0001:0010", "notes": "blocked: a compiler question"},
          {"unit": "e0002", "addr": "0002:0000", "notes": "draft: not yet"}]
    assert common.unit_blocked("e0001", fs) and not common.unit_blocked("e0002", fs)
    monkeypatch.setattr(common, "functions", lambda: fs)
    monkeypatch.setattr(common, "queue", lambda: ["e0001", "e0002"])
    monkeypatch.setattr(common, "attempts", lambda: {})
    monkeypatch.setattr(common, "open_must_fix", lambda uid: [])
    monkeypatch.setattr(nxt, "busy", lambda key: False)
    assert nxt.next_unit(1) == "e0002" and nxt.blocked_units() == ["e0001"]


def test_apply_renames_stale_uses(tmp_path, monkeypatch):
    """eov1, 2026-11-02: main renamed PointInRect and its EOV1.ASM declares the new name; the branch's
    new call still used the old one, so the merged file did not assemble."""
    src = tmp_path / "src"
    src.mkdir()
    (src / "EOV1.ASM").write_text("        EXTRN   _IsPointInRect:FAR        ; @ 14EF:00E1 (E14EF.C)\n"
                                  "        call    far ptr _IsPointInRect\n        call    far ptr _PointInRect\n")
    (src / "E0002.C").write_text("int PointInRect; /*@ ds:0100 */\n")   # bound to another address: left
    log = tmp_path / "renames.toml"
    log.write_text('[[rename]]\nid = "14EF:00E1=IsPointInRect"\naddr = "14EF:00E1"\nold = "PointInRect"\n'
                   'new = "IsPointInRect"\nkind = "function"\nwhy = "x"\ndate = "2026-11-01"\n')
    monkeypatch.setattr(rename, "SRC", src)
    monkeypatch.setattr(rename, "LOG", log)
    rename.apply()
    assert "_PointInRect" not in (src / "EOV1.ASM").read_text()
    assert "int PointInRect;" in (src / "E0002.C").read_text()


def test_names_are_per_program(tmp_path, monkeypatch):
    """EOB.EXE's ds:8A40 and INTRO.EXE's are different variables: a rename or the integration's name
    pass never carries a name from one program to the other."""
    src = tmp_path / "src"
    src.mkdir()
    (src / "E1948.C").write_text("extern char gPlaying; /*@ ds:8A40 */\n")
    (src / "I0921.C").write_text("extern char g8A40; /*@ ds:8A40 */\nvoid f(void) { g8A40 = 1; }\n")
    monkeypatch.setattr(rename, "SRC", src)
    monkeypatch.setattr(rename, "LOG", tmp_path / "renames.toml")
    assert rename.bindings(src / "I0921.C") == {"g8A40": "intro/ds:8A40"}
    assert rename.bindings(src / "E1948.C") == {"gPlaying": "ds:8A40"}
    rename.apply()
    assert "g8A40 = 1" in (src / "I0921.C").read_text()
    assert rename.norm("INTRO/ovl0:3a") == "intro/ovl0:003A"



def test_rename_cache_sees_a_changed_file(tmp_path, monkeypatch):
    """bindings() parses a file once per version of it: an edit, by the pass or anyone, is seen."""
    src = tmp_path / "src"
    src.mkdir()
    f = src / "E1948.C"
    f.write_text("extern char gPlaying; /*@ ds:8A40 */\n")
    monkeypatch.setattr(rename, "SRC", src)
    assert rename.bindings(f) == {"gPlaying": "ds:8A40"}
    f.write_text("extern char gPlayingNow; /*@ ds:8A40 */\n")
    assert rename.bindings(f) == {"gPlayingNow": "ds:8A40"}
    rename.forget(f)
    assert rename.bindings(f) == {"gPlayingNow": "ds:8A40"}

#!/usr/bin/env python3
"""smoke.py: one real model session on one unit, end to end, in a scratch clone (playbook 90).

  make smoke-agent [NAME=dec-deepseek] [UNIT=e19ec] [TIMEOUT=30] [WORKTREE=1]

Proves that a roster entry's model can do the job before a run spends money on it. In
build/smoke/repo, a fresh clone of this checkout's HEAD (WORKTREE=1 adds the uncommitted changes),
it runs the worker of roster entry NAME for one session on UNIT, exactly as `make start-run` would
(OpenCode on the background service, the entry's model, variant, cost cap), then `make integrate`
in the clone: the hand-off must pass `make check`. Nothing of this checkout changes; the clone
stays for inspection until the next smoke test.

PASS: the session ran (the provider did not refuse it), its claim ended in a hand-off, the merge
passed `make check`, and a decompiler's unit has more functions at L2 than before (a reviewer's
smoke test, NAME=rev-haiku, needs a complete unit: UNIT=e0cf7). Exit code 0 on PASS.
"""
from __future__ import annotations

import argparse
import json
import os
import shutil
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import common  # noqa: E402
import run as runmod  # noqa: E402


def sh(*cmd: str, cwd: Path, check: bool = True) -> subprocess.CompletedProcess:
    r = subprocess.run(cmd, cwd=cwd, capture_output=True, text=True)
    if check and r.returncode:
        raise RuntimeError("%s: %s" % (" ".join(cmd), (r.stdout + r.stderr).strip()[-500:]))
    return r


def main(argv=None) -> int:
    env = os.environ.get
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--name", default=env("NAME") or "dec-deepseek")
    ap.add_argument("--unit", default=env("UNIT") or "e19ec")
    ap.add_argument("--timeout", type=float, default=float(env("TIMEOUT") or 30))
    ap.add_argument("--worktree", action="store_true", default=bool(env("WORKTREE")))
    a = ap.parse_args(argv)
    sys.stdout.reconfigure(line_buffering=True)
    run, entries = runmod.roster()
    e = next((e for e in entries if e["name"] == a.name), None)
    if e is None or e["role"] == "integrator":
        print("smoke: %s is not a worker of work/roster.toml" % a.name, file=sys.stderr)
        return 2
    uid = common.identifier(a.unit, "unit")
    main_dir = common.main_checkout()
    base = main_dir / "build/smoke"
    repo = base / "repo"
    shutil.rmtree(base, ignore_errors=True)
    base.mkdir(parents=True)
    print("smoke: cloning %s into %s" % (main_dir, repo))
    sh("git", "clone", "-q", str(main_dir), str(repo), cwd=base)
    for rel in common.LINKS:
        (repo / rel).parent.mkdir(parents=True, exist_ok=True)
        if (main_dir / rel).exists():
            (repo / rel).symlink_to(main_dir / rel)
    if a.worktree:
        files = sh("git", "ls-files", "-m", "-o", "--exclude-standard", cwd=main_dir).stdout.split()
        for f in files:
            if (main_dir / f).is_file():
                (repo / f).parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(main_dir / f, repo / f)
        sh("git", "add", "-A", cwd=repo)
        sh("git", "-c", "core.hooksPath=/dev/null", "commit", "-qm", "smoke: the working tree", cwd=repo, check=False)
    sh("git", "config", "user.name", sh("git", "config", "user.name", cwd=main_dir).stdout.strip() or "smoke", cwd=repo)
    sh("git", "config", "user.email", sh("git", "config", "user.email", cwd=main_dir).stdout.strip() or "smoke@localhost", cwd=repo)
    cmd = runmod.command(run, e)
    for flag, value in (("--sessions", "1"), ("--timeout", str(a.timeout))):
        cmd[cmd.index(flag) + 1] = value
    agent = "smoke-" + e["name"]
    cmd[cmd.index("--agent") + 1] = agent
    cmd[1] = str(repo / "tools/agent/worker.py")
    cmd += ["--unit", uid]
    print("smoke: %s (%s#%s) on %s, at most %g minutes%s" % (
        e["name"], e.get("model", ""), e.get("variant", ""), uid, a.timeout,
        ", USD %s cap" % e.get("cost_cap", run.get("cost_cap")) if e.get("cost_cap", run.get("cost_cap")) else ""))
    r = subprocess.run(cmd, cwd=repo)
    gitdir = repo / ".git"
    sessions = [json.loads(l) for l in (gitdir / "eob-events.log").read_text().splitlines()
                if '"worker-session"' in l] if (gitdir / "eob-events.log").exists() else []
    s = sessions[-1] if sessions else {}
    review = e["role"] == "reviewer"
    handoff = gitdir / "eob-handoffs" / ("%s%s.md" % ("review-" if review else "", uid))
    integrated = "no hand-off"
    if handoff.exists():
        print("smoke: make integrate in the clone (make check on the merged tree, about a minute)")
        ri = subprocess.run(["make", "-s", "integrate"], cwd=repo, capture_output=True, text=True)
        integrated = (ri.stdout + ri.stderr).strip().splitlines()[-1] if (ri.stdout + ri.stderr).strip() else "?"
    merged = "merged" in integrated and "0 merged" not in integrated
    after = common.l2_count(repo, uid)
    gained = (s.get("l2_after") or after) - (s.get("l2_before") or 0) if s else 0
    ok = bool(s) and not s.get("refused") and s.get("outcome") in ("handed-off", "auto-handed-off") and merged \
        and (review or gained > 0)
    cost = s.get("cost")
    print("\nsmoke: %s" % ("PASS" if ok else "FAIL"))
    print("  model        %s#%s" % (e.get("model", ""), e.get("variant", "")))
    print("  unit         %s: %s of %d functions at L2 after the session (+%d)" % (
        uid, after, len(common.unit_functions(uid)), gained))
    print("  session      %s, %s s, %s; exit %s%s" % (
        s.get("outcome", "none"), s.get("seconds", "?"), "USD %.2f" % cost if cost is not None else "cost ?",
        s.get("exit", "?"), "; PROVIDER REFUSED: %s" % s["refused"] if s.get("refused") else ""))
    print("  integration  %s" % integrated)
    if review and (repo / "work/reviews" / ("%s.md" % uid)).exists():
        print("  review       %s" % (repo / "work/reviews" / ("%s.md" % uid)))
    logs = sorted((repo / "build/worker" / agent).glob("*.log"))
    print("  session log  %s" % (logs[-1] if logs else "none"))
    print("  clone        %s (the unit's branch and worktree: %s)" % (repo, base / "eob-worktrees" / uid))
    return 0 if ok and r.returncode == 0 else 1


if __name__ == "__main__":
    sys.exit(main())

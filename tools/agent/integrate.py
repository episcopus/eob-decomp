#!/usr/bin/env python3
"""integrate.py: merge the waiting hand-offs into main, mechanically (playbook 90).

  make integrate [KEY=<key>] [DRY=1]

For each branch hand-off, oldest first (rejected ones only when named with KEY):
  1. a temporary worktree build/integrate/<key> on branch integrate/<key> from main;
  2. `git merge --no-ff --no-commit` of the commit that was handed off (not whatever the branch
     holds now);
  3. the branch's changes checked (tools/agent/safety.py: nothing under eob/, no dashes, and from a
     unit or review branch nothing in the spec, the pins, the matcher, its fixtures, the agent tools);
  4. `make check` in the temporary worktree: the receipt is for exactly the tree that lands;
  5. one merge commit, `main` fast-forwarded to it, the hand-off archived, the claim released, the
     agent's worktree removed when it is clean and its branch deleted.
A failing step leaves main untouched: the hand-off is marked rejected with the reason and the log
(build/integrate/<key>.log); the owner re-claims the unit (same AGENT), fixes, and hands off again.
DRY=1 merges and checks without touching main. Escalations are never merged: they wait for the human
(`make handoffs`), who settles them with `make handoff-done KEY=<key> DECISION="..."`.
"""
from __future__ import annotations

import argparse
import os
import shutil
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import common  # noqa: E402
import handoff  # noqa: E402
import safety  # noqa: E402


class Rejected(Exception):
    pass


def remove_worktree(path: Path, branch: str) -> None:
    main_dir = common.main_checkout()
    if path.exists():
        common.git("worktree", "remove", "--force", str(path), cwd=main_dir, check=False)
        shutil.rmtree(path, ignore_errors=True)
    common.git("worktree", "prune", cwd=main_dir, check=False)
    if branch:
        common.git("branch", "-D", branch, cwd=main_dir, check=False)


def integrate_one(key: str, dry: bool) -> bool:
    main_dir = common.main_checkout()
    path = common.handoff_path(key)
    f = common.fields(path.read_text())
    branch, commit, state = f["branch"], f["commit"], f["state"]
    tmp, tbranch = main_dir / "build/integrate" / key, "integrate/%s" % key
    log = main_dir / "build/integrate" / ("%s.log" % key)
    log.parent.mkdir(parents=True, exist_ok=True)
    remove_worktree(tmp, tbranch)
    try:
        if common.git("rev-parse", branch, cwd=main_dir, check=False) == "":
            raise Rejected("branch %s is gone" % branch)
        common.git("merge-base", "--is-ancestor", commit, branch, cwd=main_dir)
        problems = safety.branch_findings(branch, cwd=main_dir, rev=commit)
        if problems:
            raise Rejected("forbidden changes: " + "; ".join(problems))
        common.git("worktree", "add", "-b", tbranch, str(tmp), "main", cwd=main_dir)
        common.link_untracked(tmp)
        r = subprocess.run(["git", "merge", "--no-ff", "--no-commit", commit], cwd=tmp, capture_output=True, text=True)
        if r.returncode:
            raise Rejected("merge conflict: %s" % (r.stdout + r.stderr).strip().splitlines()[-1])
        with open(log, "w") as out:
            r = subprocess.run(["make", "check"], cwd=tmp, stdout=out, stderr=subprocess.STDOUT)
        if r.returncode:
            tail = log.read_text().strip().splitlines()[-3:]
            raise Rejected("make check failed: %s" % " | ".join(tail))
        common.git("commit", "-q", "--no-verify", "-m", "Merge %s (%s): %s\n\nAgent: %s\nHanded-off: %s" % (
            branch, state, f.get("note", ""), f.get("agent"), commit), cwd=tmp)
        merged = common.git("rev-parse", "HEAD", cwd=tmp)
        if dry:
            print("integrate: %s merges and checks cleanly (DRY: main untouched, log %s)" % (key, log))
            return True
        common.git("merge", "--ff-only", "-q", merged, cwd=main_dir)
    except (Rejected, RuntimeError) as e:
        common.git("merge", "--abort", cwd=tmp, check=False) if tmp.exists() else None
        remove_worktree(tmp, tbranch)
        text = path.read_text()
        kept = "".join(l + "\n" for l in text.splitlines() if not l.startswith("integrate"))
        common.atomic_write(path, kept + common.record({
            "integrate": "rejected", "integrate_reason": common.single_line(str(e).replace("\n", " ")[:300], "reason"),
            "integrate_log": str(log)}))
        common.event("integrate-rejected", key=key, reason=str(e)[:300])
        print("integrate: %s REJECTED: %s" % (key, e))
        return False
    remove_worktree(tmp, tbranch)
    handoff.archive(key, path.read_text(), "merged %s" % merged)
    path.unlink()
    common.lock_path(key).unlink(missing_ok=True)
    wt = common.worktree_of(key)
    if wt.exists() and not common.git("status", "--porcelain", cwd=wt, check=False):
        remove_worktree(wt, "")
    common.git("branch", "-d", branch, cwd=main_dir, check=False)
    common.event("integrate", key=key, branch=branch, commit=commit, merge=merged)
    print("integrate: %s merged into main as %s" % (key, merged[:10]))
    return True


def done(key: str, decision: str) -> None:
    """The human settles an escalation (or drops a hand-off): archived with the decision."""
    common.require_main("handoff-done")
    path = common.handoff_path(key)
    if not path.exists():
        raise ValueError("no hand-off %s" % key)
    if not decision.strip():
        raise ValueError("DECISION=\"<the human's decision>\" is required")
    common.single_line(decision, "decision")
    handoff.archive(key, path.read_text(), "decision: %s" % decision)
    path.unlink()
    common.event("handoff-done", key=key, decision=decision)


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--key", default=os.environ.get("KEY", ""))
    ap.add_argument("--dry", action="store_true", default=bool(os.environ.get("DRY")))
    ap.add_argument("--done", action="store_true")
    ap.add_argument("--decision", default=os.environ.get("DECISION", ""))
    a = ap.parse_args(argv)
    try:
        if a.done:
            done(common.identifier(a.key, "key"), a.decision)
            print("handoff-done: %s archived" % a.key)
            return 0
        common.require_main("integrate")
        if common.git("status", "--porcelain", "--untracked-files=no"):
            raise ValueError("main has uncommitted changes; integrate only from a clean main")
    except (ValueError, RuntimeError) as e:
        print("integrate: %s" % e, file=sys.stderr)
        return 2
    files = sorted(common.state_dir("eob-handoffs").glob("*.md"), key=lambda p: p.stat().st_mtime)
    keys = []
    for p in files:
        f = common.fields(p.read_text())
        if f.get("state") == "escalate":
            continue
        if a.key and p.stem != a.key:
            continue
        if f.get("integrate") == "rejected" and not a.key:
            continue
        keys.append(p.stem)
    if not keys:
        print("integrate: nothing to merge%s" % (" (no hand-off %s)" % a.key if a.key else ""))
        return 0
    ok = [integrate_one(k, a.dry) for k in keys]
    print("integrate: %d merged, %d rejected" % (sum(ok), len(ok) - sum(ok)))
    return 0 if all(ok) else 1


if __name__ == "__main__":
    sys.exit(main())

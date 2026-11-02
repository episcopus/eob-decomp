#!/usr/bin/env python3
"""integrate.py: merge the waiting hand-offs into main, mechanically (playbook 90).

  make integrate [KEY=<key>] [DRY=1]

For each branch hand-off, oldest first (rejected ones only when named with KEY):
  1. a temporary worktree build/integrate/<key> on branch integrate/<key> from main;
  2. `git merge --no-ff --no-commit` of the commit that was handed off (not whatever the branch
     holds now); a conflict in the unit's own file is merged again with main's names given to the
     ancestor and the branch (resolve_by_names: such conflicts come from renames at earlier merges);
  3. the recorded renames applied to the merged tree (`make rename APPLY=1`), so one address keeps
     one name whatever the branch called it;
  4. the branch's changes checked (tools/agent/safety.py: nothing under eob/, no dashes, and from a
     unit or review branch nothing in the spec, the pins, the matcher, its fixtures, the agent tools);
  5. `make check` in the temporary worktree: the receipt is for exactly the tree that lands;
  6. one merge commit, `main` fast-forwarded to it, the hand-off archived, the claim released, the
     agent's worktree removed when it is clean and its branch deleted.
A failing step leaves main untouched: the hand-off is marked rejected with the reason and the log
(build/integrate/<key>.log); the owner re-claims the unit (same AGENT), fixes, and hands off again.
DRY=1 merges and checks without touching main. Escalations are never merged: they wait for the human
(`make handoffs`), who settles them with `make handoff-done KEY=<key> DECISION="..."`.
"""
from __future__ import annotations

import argparse
import fcntl
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


def resolve_by_names(tmp: Path, main_dir: Path, commit: str, uid: str, log: Path) -> bool:
    """Most conflicts are names: main gave the unit's file new names (make rename APPLY=1 at an earlier
    merge) while the branch edited lines nearby. Give main's names (the recorded renames and the one
    real names of main's src/) to the common ancestor's and the branch's copies of the file too, and
    merge the three again (git merge-file): both sides then differ from the ancestor only in what they
    really changed. True when every conflicted file was the unit's own and merged cleanly."""
    import tempfile
    conflicted = common.git("diff", "--name-only", "--diff-filter=U", cwd=tmp).split()
    own = [f for f in conflicted if f.startswith("src/") and Path(f).stem.lower() == uid]
    if not conflicted or own != conflicted:
        return False
    sys.path.insert(0, str(main_dir / "tools/match"))
    import rename
    try:
        rename.SRC, rename.LOG = main_dir / "src", main_dir / "re/renames.toml"
        names = rename.recorded()
        for addr, new in rename.real_names().items():
            names.setdefault(addr, new)
        base = common.git("merge-base", "HEAD", commit, cwd=tmp)
        for f in own:
            with tempfile.TemporaryDirectory() as d:
                d = Path(d)
                copies = {}
                for side, rev in (("base", base), ("theirs", commit)):
                    (d / side).mkdir()
                    text = common.git("show", "%s:%s" % (rev, f), cwd=tmp, check=False)
                    (d / side / Path(f).name).write_text(text + "\n" if text else "")
                    rename.SRC = d / side
                    for addr, new in names.items():
                        try:
                            rename.rename_address(addr, new, only=uid)
                        except ValueError:
                            pass
                    copies[side] = d / side / Path(f).name
                ours = d / "ours"
                ours.write_text(common.git("show", "HEAD:%s" % f, cwd=tmp) + "\n")
                r = subprocess.run(["git", "merge-file", "-p", str(ours), str(copies["base"]), str(copies["theirs"])],
                                   capture_output=True, text=True)
                if r.returncode:
                    with open(log, "a") as out:
                        out.write("resolve_by_names: %s still conflicts after main's names\n" % f)
                    return False
                (tmp / f).write_text(r.stdout)
                common.git("add", "--", f, cwd=tmp)
        with open(log, "a") as out:
            out.write("resolve_by_names: %s merged once main's names were given to the ancestor and the branch\n"
                      % ", ".join(own))
        return True
    finally:
        rename.SRC, rename.LOG = main_dir / "src", main_dir / "re/renames.toml"


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
        r = subprocess.run(["git", "merge", "--no-ff", "--no-commit", commit], cwd=tmp, capture_output=True, text=True)
        if r.returncode and branch.startswith("unit/") and resolve_by_names(tmp, main_dir, commit, branch[5:], log):
            r = subprocess.CompletedProcess(r.args, 0, "", "")
        if r.returncode:
            lines = [l.strip() for l in (r.stdout + r.stderr).splitlines() if l.strip()]
            raise Rejected("merge conflict: %s" % "; ".join(lines[-4:]))
        # The links to the main checkout's tools (.venv, tools/vendor, ...) are never part of a merge,
        # even when a branch committed them (git add -A in a worktree, before .gitignore matched links)
        common.git("rm", "-r", "-q", "--cached", "--ignore-unmatch", "--", *common.LINKS, cwd=tmp)
        common.link_untracked(tmp)
        # Names: a branch may still use a name another branch renamed, or declare a renamed address
        # under its placeholder; the recorded renames win (tools/match/rename.py), before the check
        if (tmp / "tools/match/rename.py").exists():
            r = subprocess.run([str(tmp / ".venv/bin/python"), "tools/match/rename.py", "--apply"], cwd=tmp,
                               capture_output=True, text=True)
            if r.returncode:
                raise Rejected("make rename APPLY=1 failed: %s" % (r.stdout + r.stderr).strip()[-300:])
            common.git("add", "-A", "--", "src", "re", "docs", cwd=tmp)
        changed = common.git("diff", "--name-only", "%s...%s" % ("main", commit), cwd=main_dir).split()
        with open(log, "w") as out:                   # the readability gate judges what the branch changed
            r = subprocess.run(["make", "check"], cwd=tmp, stdout=out, stderr=subprocess.STDOUT,
                               env=dict(os.environ, READABILITY_FILES=" ".join(changed)))
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
        common.install_git_config()
        if common.git("status", "--porcelain", "--untracked-files=no"):
            raise ValueError("main has uncommitted changes; integrate only from a clean main")
    except (ValueError, RuntimeError) as e:
        print("integrate: %s" % e, file=sys.stderr)
        return 2
    guard = open(common.common_dir() / "eob-integrate.lock", "w")
    try:                                   # one integration at a time (the integrator, a human)
        fcntl.flock(guard, fcntl.LOCK_EX | fcntl.LOCK_NB)
    except BlockingIOError:
        print("integrate: another make integrate is running; try again when it is done")
        return 3
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

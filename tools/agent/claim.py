#!/usr/bin/env python3
"""claim.py: start, list and release work (playbooks 30, 70, 90).

  make claim AGENT=<name> [UNIT=<id>]       a unit to decompile: UNIT, or `make next`
  make claim AGENT=<name> REVIEW=1 [UNIT=]  a review of a complete unit (playbook 70)
  make claim AGENT=<name> TASK=<name>       other work (a whitepaper chapter, a tool): branch task/<name>
  make locks                                the claims, oldest first
  make unlock UNIT=<key> AGENT=<owner>      the owner gives a claim back (its branch has no commits
                                            beyond main, or FORCE=1 REASON="<the human's instruction>")

A claim locks the key atomically in the clone's git directory (tools/agent/common.py), so two agents
never hold the same unit, and creates the worktree ../eob-worktrees/<key> on branch unit/<id>,
review/<id> or task/<name> from main (or reuses them: an agent coming back to its own claim, for
example after `make integrate` rejected its hand-off, continues where it was). The untracked tools
are linked in from the main checkout: .venv, build/toolchain, build/re, tools/vendor.
"""
from __future__ import annotations

import argparse
import os
import socket
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import common  # noqa: E402
import next as nxt  # noqa: E402


def setup(key: str, branch: str) -> Path:
    main_dir = common.main_checkout()
    wt = common.worktree_of(key)
    if wt.exists():
        actual = common.git("branch", "--show-current", cwd=wt)
        if actual != branch:
            raise ValueError("worktree %s is on %s, not %s; leave it alone and ask the human" % (wt, actual, branch))
    else:
        wt.parent.mkdir(parents=True, exist_ok=True)
        if common.git("branch", "--list", branch, cwd=main_dir):
            common.git("worktree", "add", str(wt), branch, cwd=main_dir)
        else:
            common.git("worktree", "add", "-b", branch, str(wt), "main", cwd=main_dir)
    common.link_untracked(wt)
    common.install_git_config()
    return wt


def claim(agent: str, uid: str = "", review: bool = False, task: str = "", tier: int = 1,
          max_attempts: int = 2) -> tuple[str, Path]:
    """Lock and set up; returns (key, worktree). Raises ValueError when there is nothing to take."""
    common.identifier(agent, "agent")
    main_dir = common.main_checkout()
    if common.git("status", "--porcelain", "--untracked-files=no", cwd=main_dir):
        raise ValueError("main has uncommitted changes; a branch starts from committed main. Wait for the "
                         "human; never stash, reset or clean main")
    explicit = bool(uid or task)
    if uid:
        common.identifier(uid, "unit")
        if uid not in common.queue():
            raise ValueError("%s is not in work/queue.toml" % uid)
        if review and not common.unit_complete(uid):
            raise ValueError("%s is not complete on main; review a unit once it is merged complete" % uid)
    for _attempt in range(50):
        if task:
            key, branch = common.identifier(task, "task"), "task/%s" % task
        else:
            if not explicit:
                uid = (nxt.next_review() if review else nxt.next_unit(tier, max_attempts)) or ""
                if not uid:
                    raise ValueError("nothing to %s: the queue is done, locked or waiting for integration"
                                     % ("review" if review else "claim"))
            key = ("review-" if review else "") + uid
            branch = "%s/%s" % ("review" if review else "unit", uid)
        owner = common.lock_owner(key)
        if owner == agent:
            print("claim: %s is already yours; reusing its worktree and branch" % key)
            h = common.handoff_path(key)
            if h.exists() and common.fields(h.read_text()).get("integrate") == "rejected":
                f = common.fields(h.read_text())
                print("claim: the last hand-off was rejected by make integrate: %s (log %s)" % (
                    f.get("integrate_reason"), f.get("integrate_log")))
            break
        h = common.handoff_path(key)
        if h.exists() and not (common.fields(h.read_text()).get("integrate") == "rejected"
                               and common.fields(h.read_text()).get("agent") == agent):
            if explicit:
                raise ValueError("%s has a hand-off waiting for integration; do not duplicate it" % key)
            continue
        if common.try_lock(key, {"key": key, "agent": agent, "branch": branch, "host": socket.gethostname(),
                                 "started": common.now()}):
            common.event("claim", agent=agent, key=key, branch=branch)
            break
        if explicit:
            raise ValueError("%s is claimed by %s" % (key, common.lock_owner(key)))
        # another agent took it between `next` and the lock: take the next one
    else:
        raise ValueError("could not take a unit: every candidate was taken while choosing")
    try:
        wt = setup(key, branch)
    except (RuntimeError, ValueError, OSError):
        if owner != agent:
            common.lock_path(key).unlink(missing_ok=True)
        raise
    return key, wt


def unlock(key: str, agent: str, force: bool, reason: str) -> None:
    p = common.lock_path(key)
    if not p.exists():
        raise ValueError("%s is not claimed" % key)
    f = common.fields(p.read_text())
    if f.get("agent") != agent and not force:
        raise ValueError("%s is claimed by %s, not %s" % (key, f.get("agent"), agent))
    branch = f.get("branch", "")
    ahead = int(common.git("rev-list", "--count", "main..%s" % branch, check=False) or 0) if branch else 0
    if ahead and not force:
        raise ValueError("%s has %d commits beyond main: hand them off (make handoff), or the human releases "
                         "the claim with FORCE=1 REASON=\"...\" (branch and worktree are kept)" % (branch, ahead))
    if force:
        if not reason.strip():
            raise ValueError("FORCE=1 needs REASON=\"<the human's instruction>\"")
        common.single_line(reason, "reason")
    p.unlink()
    common.event("unlock", agent=agent, key=key, force=force, reason=reason)
    wt = common.worktree_of(key)
    if not ahead and wt.exists() and not common.git("status", "--porcelain", cwd=wt):
        common.git("worktree", "remove", str(wt), cwd=common.main_checkout())
        common.git("branch", "-D", branch, cwd=common.main_checkout(), check=False)


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--agent", default=os.environ.get("AGENT", ""))
    ap.add_argument("--unit", default=os.environ.get("UNIT", ""))
    ap.add_argument("--review", action="store_true", default=bool(os.environ.get("REVIEW")))
    ap.add_argument("--task", default=os.environ.get("TASK", ""))
    ap.add_argument("--list", action="store_true")
    ap.add_argument("--unlock", action="store_true")
    ap.add_argument("--force", action="store_true", default=bool(os.environ.get("FORCE")))
    ap.add_argument("--reason", default=os.environ.get("REASON", ""))
    a = ap.parse_args(argv)
    try:
        if a.list:
            locks = sorted(common.state_dir("eob-locks").glob("*.lock"), key=lambda p: p.stat().st_mtime)
            for p in locks:
                f = common.fields(p.read_text())
                print("%-20s %-16s %-24s since %s" % (f.get("key"), f.get("agent"), f.get("branch"), f.get("started")))
            print("locks: %d" % len(locks))
            return 0
        if a.unlock:
            unlock(common.identifier(a.unit, "key"), common.identifier(a.agent, "agent"), a.force, a.reason)
            print("unlock: released %s" % a.unit)
            return 0
        if not a.agent:
            print("claim: need AGENT=<a name unique to this agent session>", file=sys.stderr)
            return 2
        key, wt = claim(a.agent, a.unit, a.review, a.task)
    except (ValueError, RuntimeError, OSError) as e:
        print("%s: %s" % ("unlock" if a.unlock else "claim", e), file=sys.stderr)
        return 1
    print("claim: %s for %s, worktree %s" % (key, a.agent, wt))
    if a.task:
        print("claim: next: cd %s" % wt)
    elif a.review:
        print("claim: next: cd %s && cp work/reviews/TEMPLATE.md work/reviews/%s.md   (playbook 70)" % (wt, key[7:]))
    else:
        print("claim: next: cd %s && make packet UNIT=%s   (playbook 30)" % (wt, key))
    return 0


if __name__ == "__main__":
    sys.exit(main())

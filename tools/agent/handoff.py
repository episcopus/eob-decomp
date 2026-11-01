#!/usr/bin/env python3
"""handoff.py: hand a branch to integration, or a question to the human (playbooks 30, 70, 90).

  make handoff AGENT=<you> UNIT=<id> STATE=<state> NOTE="..."
  make handoffs                     what waits: escalations first, then branches, oldest first

STATE:
  complete, partial, blocked        a unit branch (unit/<id>) is ready to merge: every function of
                                    the unit file is matched, a stub or a draft (`make match` passes)
  review-passed, review-failed      a review branch (review/<id>) with work/reviews/<id>.md
  escalate                          not a branch: a question for the human (a suspected matcher
                                    problem, a spec question, a claim elsewhere that looks wrong);
                                    UNIT may be `general`; nothing needs committing

A hand-off is a small record in the clone's git directory (tools/agent/common.py); nothing is
committed. Run it from your claimed worktree after committing. `make integrate` takes it from there.
"""
from __future__ import annotations

import argparse
import datetime as dt
import os
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import common  # noqa: E402
import safety  # noqa: E402


def unmatched(uid: str) -> str:
    """The matcher's complaint about the unit's file, or ''."""
    src = common.source_of(uid)
    if src is None:
        return "no src file for %s" % uid
    r = subprocess.run([sys.executable, str(common.ROOT / "tools/match/cmatch.py"), str(src)],
                       cwd=common.ROOT, capture_output=True, text=True)
    if r.returncode == 0:
        return ""
    return "\n".join(l for l in (r.stdout + r.stderr).splitlines() if not l.startswith("ok"))[-1500:]


def archive(key: str, text: str, why: str) -> None:
    d = common.state_dir("eob-handoff-archive")
    common.atomic_write(d / ("%s-%s.md" % (key, dt.datetime.now(dt.timezone.utc).strftime("%Y%m%dT%H%M%S%f"))),
                        text + "archived = %s\n" % why)


def make(agent: str, uid: str, state: str, note: str) -> str:
    common.identifier(agent, "agent")
    common.identifier(uid, "unit")
    common.single_line(note, "note")
    if state not in common.STATES:
        raise ValueError("STATE must be one of %s" % ", ".join(common.STATES))
    if not note.strip():
        raise ValueError("a hand-off needs NOTE=\"<what was done, what is left, the evidence>\"")
    if state == "escalate":
        key = "escalate-%s-%s" % (uid, dt.datetime.now(dt.timezone.utc).strftime("%Y%m%d%H%M%S"))
        common.atomic_write(common.handoff_path(key), common.record({
            "key": key, "unit": uid, "agent": agent, "state": state, "time": common.now(), "note": note}))
        common.event("handoff", agent=agent, key=key, state=state)
        return key
    branch = common.git("branch", "--show-current")
    review = state in common.REVIEW_STATES
    expected = "%s/%s" % ("review" if review else "unit", uid)
    if branch.startswith("task/") and state in ("complete", "partial", "blocked"):
        expected = branch                                  # a task branch hands off like a unit
    if branch != expected:
        raise ValueError("STATE %s for %s is handed off from branch %s, not %s" % (state, uid, expected, branch))
    key = ("review-" + uid) if review else (branch[5:] if branch.startswith("task/") else uid)
    if common.lock_owner(key) != agent:
        raise ValueError("%s is not claimed by %s; claim it first (make claim)" % (key, agent))
    if common.git("status", "--porcelain", "--untracked-files=no"):
        raise ValueError("commit your work first (git status shows changes)")
    if not int(common.git("rev-list", "--count", "main..HEAD") or 0):
        raise ValueError("%s has no commits beyond main: nothing to merge" % branch)
    problems = safety.branch_findings(branch)
    if problems:
        raise ValueError("the branch changes what it may not:\n  " + "\n  ".join(problems))
    if branch.startswith("unit/"):
        why = unmatched(uid)
        if why:
            raise ValueError("make match fails on %s; turn every unfinished function back into a `stub` or a "
                             "`draft` (playbook 30), commit, and hand off again:\n%s" % (uid, why))
    if review and not (common.ROOT / "work/reviews" / ("%s.md" % uid)).exists():
        raise ValueError("a review hands off work/reviews/%s.md" % uid)
    path = common.handoff_path(key)
    if path.exists():
        old = path.read_text()
        if common.fields(old).get("agent") != agent:
            raise ValueError("%s already has a hand-off from another agent" % key)
        archive(key, old, "replaced")
    head = common.git("rev-parse", "HEAD")
    common.atomic_write(path, common.record({
        "key": key, "unit": uid, "agent": agent, "branch": branch, "commit": head, "state": state,
        "time": common.now(), "note": note}))
    common.event("handoff", agent=agent, key=key, branch=branch, commit=head, state=state)
    return key


def listing() -> int:
    files = sorted(common.state_dir("eob-handoffs").glob("*.md"),
                   key=lambda p: (not p.stem.startswith("escalate-"), p.stat().st_mtime))
    for p in files:
        f = common.fields(p.read_text())
        tag = "ESCALATION for the human" if f.get("state") == "escalate" else (
            "REJECTED: %s" % f.get("integrate_reason") if f.get("integrate") == "rejected" else "to merge")
        print("%s  [%s]\n  %s %s %s %s\n  %s\n" % (p.stem, tag, f.get("agent"), f.get("branch", ""),
                                                    f.get("state"), f.get("time"), f.get("note")))
    print("handoffs: %d waiting, %d escalations" % (len(files), sum(p.stem.startswith("escalate-") for p in files)))
    return 0


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--agent", default=os.environ.get("AGENT", ""))
    ap.add_argument("--unit", default=os.environ.get("UNIT", ""))
    ap.add_argument("--state", default=os.environ.get("STATE", ""))
    ap.add_argument("--note", default=os.environ.get("NOTE", ""))
    ap.add_argument("--list", action="store_true")
    a = ap.parse_args(argv)
    if a.list:
        return listing()
    try:
        key = make(a.agent, a.unit.lower(), a.state, a.note)
    except (ValueError, RuntimeError) as e:
        print("handoff: %s" % e, file=sys.stderr)
        return 1
    print("handoff: %s (%s) waits for %s" % (key, a.state, "the human" if a.state == "escalate" else "make integrate"))
    return 0


if __name__ == "__main__":
    sys.exit(main())

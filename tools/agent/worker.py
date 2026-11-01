#!/usr/bin/env python3
"""worker.py: a supervisor that runs model sessions on claimed units (docs/process/playbooks/90-orchestrate.md).

  make worker AGENT=<name> [ROLE=decompiler|reviewer] [UNIT=<id>] [SESSIONS=1] [TIMEOUT=60]
              [SESSION_CMD="claude -p --permission-mode acceptEdits"] [DRY=1]

The human starts it; scripts never start one. For each session (at most SESSIONS):
  1. claim a unit (`make claim`: UNIT, or the next one; ROLE=reviewer claims a review);
  2. run SESSION_CMD in the unit's worktree with the prompt (tools/agent/worker_prompt.md, filled
     in) on its standard input, for at most TIMEOUT minutes; the output goes to
     build/worker/<agent>/<n>.log;
  3. afterwards the claim must end in a hand-off: if the session handed off, good; if it committed
     work that matches but did not hand off, the supervisor hands it off as `partial`; if it
     committed nothing, the claim is released; anything else is left for the human (`make locks`).
Its own hand-offs that `make integrate` rejected come first: it re-claims that unit, with the
reason in the prompt. It stops after SESSIONS, when there is nothing to claim, after two sessions in
a row that produced nothing, when build/worker/STOP exists (`make stop-run` creates it), or on
SIGTERM (the session is stopped and the claim settled as above). DRY=1 prints the first prompt.
"""
from __future__ import annotations

import argparse
import os
import shlex
import signal
import subprocess
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import claim as claim_mod  # noqa: E402
import common  # noqa: E402

PROMPT = Path(__file__).resolve().parent / "worker_prompt.md"
ROLES = {
    "decompiler": ("docs/process/playbooks/30-lift-module.md",
                   "Run `make packet UNIT={unit}` and read build/packets/{unit}.md. If src/ has no file for "
                   "the unit, `make skeleton UNIT={unit}`. Decompile the functions that are not at L2.",
                   "complete|partial|blocked"),
    "reviewer": ("docs/process/playbooks/70-review.md",
                 "Copy work/reviews/TEMPLATE.md to work/reviews/{unit}.md and review the unit as the playbook says.",
                 "review-passed|review-failed"),
}


class Stopped(Exception):
    """SIGTERM from `make stop-run`: stop the session, settle the claim, exit."""


def prompt(agent: str, role: str, key: str, branch: str, wt: Path, rejected: str = "") -> str:
    unit = key[7:] if key.startswith("review-") else key
    playbook, first, states = ROLES[role]
    if rejected:
        first = ("Your last hand-off of this branch was rejected by make integrate: %s. Fix that first (a "
                 "merge conflict: `git merge main` in this worktree and keep both sides; a failed check: "
                 "read the log). Then continue: " % rejected) + first
    return PROMPT.read_text().format(agent=agent, role=role, key=key, unit=unit, branch=branch, worktree=wt,
                                     playbook=playbook, first_step=first.format(unit=unit), states=states)


def rejected_handoffs(agent: str) -> list[tuple[str, str]]:
    """(key, reason) of this agent's hand-offs that make integrate rejected: its work to finish first."""
    out = []
    for p in sorted(common.state_dir("eob-handoffs").glob("*.md"), key=lambda p: p.stat().st_mtime):
        f = common.fields(p.read_text())
        if f.get("agent") == agent and f.get("integrate") == "rejected":
            out.append((p.stem, "%s (log %s)" % (f.get("integrate_reason", ""), f.get("integrate_log", ""))))
    return out


def run_session(cmd: str, wt: Path, env: dict, text: str, timeout: float, log: Path) -> str:
    """Run one session in its own process group; returns its exit code, "timeout" or "stopped"."""
    with open(log, "w") as out:
        p = subprocess.Popen(shlex.split(cmd), cwd=wt, env=env, stdin=subprocess.PIPE, stdout=out,
                             stderr=subprocess.STDOUT, text=True, start_new_session=True)
        try:
            p.communicate(text, timeout=timeout)
            return str(p.returncode)
        except subprocess.TimeoutExpired:
            code = "timeout"
        except Stopped:
            code = "stopped"
        for sig in (signal.SIGTERM, signal.SIGKILL):
            try:
                os.killpg(p.pid, sig)
                p.wait(timeout=20)
                break
            except (ProcessLookupError, subprocess.TimeoutExpired):
                continue
        return code


def settle(agent: str, key: str, branch: str, wt: Path, head_before: str = "") -> str:
    """What became of the claim after a session: handed-off, auto-handed-off, released, or left. A
    rejected hand-off is handed off again only when the session committed something new (otherwise
    the integrator would reject the same commit again and again)."""
    h = common.handoff_path(key)
    if h.exists() and common.fields(h.read_text()).get("integrate") != "rejected":
        return "handed-off"
    if h.exists() and common.git("rev-parse", branch, check=False) == head_before:
        return "left"
    ahead = int(common.git("rev-list", "--count", "main..%s" % branch, check=False) or 0)
    dirty = common.git("status", "--porcelain", "--untracked-files=no", cwd=wt, check=False)
    if not ahead and not dirty:
        claim_mod.unlock(key, agent, False, "")
        return "released"
    if ahead and not dirty and not key.startswith("review-"):
        r = subprocess.run(["make", "handoff", "AGENT=%s" % agent, "UNIT=%s" % key, "STATE=partial",
                            "NOTE=worker: the session ended without a hand-off; %d commits" % ahead],
                           cwd=wt, capture_output=True, text=True)
        if r.returncode == 0:
            return "auto-handed-off"
    return "left"


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--agent", default=os.environ.get("AGENT", ""))
    ap.add_argument("--role", default=os.environ.get("ROLE", "decompiler"), choices=sorted(ROLES))
    ap.add_argument("--unit", default=os.environ.get("UNIT", ""))
    ap.add_argument("--sessions", type=int, default=int(os.environ.get("SESSIONS") or 1))
    ap.add_argument("--timeout", type=float, default=float(os.environ.get("TIMEOUT") or 60))
    ap.add_argument("--session-cmd", default=os.environ.get("SESSION_CMD") or "claude -p --permission-mode acceptEdits")
    ap.add_argument("--dry", action="store_true", default=bool(os.environ.get("DRY")))
    a = ap.parse_args(argv)
    try:
        common.identifier(a.agent, "agent")
    except ValueError as e:
        print("worker: %s (AGENT=<name>)" % e, file=sys.stderr)
        return 2
    if a.dry:
        uid = a.unit or "<unit>"
        key = ("review-" + uid) if a.role == "reviewer" else uid
        print(prompt(a.agent, a.role, key, ("review/" if a.role == "reviewer" else "unit/") + uid,
                     common.worktree_of(key)))
        return 0
    logs = common.main_checkout() / "build/worker" / a.agent
    logs.mkdir(parents=True, exist_ok=True)

    def on_term(sig, frame):
        raise Stopped()
    signal.signal(signal.SIGTERM, on_term)
    idle, retried = 0, set()
    try:
        return loop(a, logs, idle, retried)
    except Stopped:
        print("worker: stopped by make stop-run")
        return 0


def loop(a, logs: Path, idle: int, retried: set) -> int:
    for n in range(1, a.sessions + 1):
        if (common.main_checkout() / "build/worker/STOP").exists():
            print("worker: build/worker/STOP exists; stopping")
            break
        rejected = [r for r in rejected_handoffs(a.agent) if r[0] not in retried][:1]
        retried.update(k for k, _ in rejected)
        try:
            if rejected:                        # its own rejected work comes first
                key = rejected[0][0]
                key, wt = claim_mod.claim(a.agent, key[7:] if key.startswith("review-") else key, key.startswith("review-"))
            else:
                key, wt = claim_mod.claim(a.agent, a.unit if n == 1 else "", a.role == "reviewer")
        except (ValueError, RuntimeError) as e:
            print("worker: %s" % e)
            break
        branch = common.fields(common.lock_path(key).read_text())["branch"]
        log = logs / ("%d-%s.log" % (n, key))
        print("worker: session %d on %s in %s (log %s)%s" % (n, key, wt, log, ", after a rejection" if rejected else ""),
              flush=True)
        env = dict(os.environ, AGENT=a.agent)
        start, head_before = time.time(), common.git("rev-parse", branch, check=False)
        try:
            code = run_session(a.session_cmd, wt, env, prompt(a.agent, a.role, key, branch, wt,
                                                              rejected[0][1] if rejected else ""), a.timeout * 60, log)
        except OSError as e:
            print("worker: cannot run %r: %s" % (a.session_cmd, e), file=sys.stderr)
            print("worker: %s: %s" % (key, settle(a.agent, key, branch, wt, head_before)), file=sys.stderr)
            return 2
        outcome = settle(a.agent, key, branch, wt, head_before)
        common.event("worker-session", agent=a.agent, key=key, exit=str(code), outcome=outcome,
                     minutes=round((time.time() - start) / 60, 1))
        print("worker: session %d on %s ended (%s): %s" % (n, key, code, outcome), flush=True)
        if code == "stopped":
            print("worker: stopped by make stop-run")
            break
        idle = idle + 1 if outcome in ("released", "left") else 0
        if idle >= 2:
            print("worker: two sessions in a row produced nothing to merge; stopping")
            break
    return 0


if __name__ == "__main__":
    sys.exit(main())

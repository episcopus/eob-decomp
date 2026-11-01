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
It stops after SESSIONS, when there is nothing to claim, after two sessions in a row that produced
nothing, or when build/worker/STOP exists. DRY=1 prints the first prompt and claims nothing.
"""
from __future__ import annotations

import argparse
import os
import shlex
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


def prompt(agent: str, role: str, key: str, branch: str, wt: Path) -> str:
    unit = key[7:] if key.startswith("review-") else key
    playbook, first, states = ROLES[role]
    return PROMPT.read_text().format(agent=agent, role=role, key=key, unit=unit, branch=branch, worktree=wt,
                                     playbook=playbook, first_step=first.format(unit=unit), states=states)


def settle(agent: str, key: str, branch: str, wt: Path) -> str:
    """What became of the claim after a session: handed-off, auto-handed-off, released, or left."""
    if common.handoff_path(key).exists() and common.fields(common.handoff_path(key).read_text()).get("integrate") != "rejected":
        return "handed-off"
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
    idle = 0
    for n in range(1, a.sessions + 1):
        if (common.main_checkout() / "build/worker/STOP").exists():
            print("worker: build/worker/STOP exists; stopping")
            break
        try:
            key, wt = claim_mod.claim(a.agent, a.unit if n == 1 else "", a.role == "reviewer")
        except (ValueError, RuntimeError) as e:
            print("worker: %s" % e)
            break
        branch = common.fields(common.lock_path(key).read_text())["branch"]
        log = logs / ("%d-%s.log" % (n, key))
        print("worker: session %d on %s in %s (log %s)" % (n, key, wt, log))
        env = dict(os.environ, AGENT=a.agent)
        start = time.time()
        with open(log, "w") as out:
            try:
                r = subprocess.run(shlex.split(a.session_cmd), cwd=wt, env=env, input=prompt(a.agent, a.role, key, branch, wt),
                                   stdout=out, stderr=subprocess.STDOUT, text=True, timeout=a.timeout * 60)
                code = r.returncode
            except subprocess.TimeoutExpired:
                code = "timeout"
            except OSError as e:
                print("worker: cannot run %r: %s" % (a.session_cmd, e), file=sys.stderr)
                claim_mod.unlock(key, a.agent, False, "")
                return 2
        outcome = settle(a.agent, key, branch, wt)
        common.event("worker-session", agent=a.agent, key=key, exit=str(code), outcome=outcome,
                     minutes=round((time.time() - start) / 60, 1))
        print("worker: session %d on %s ended (%s): %s" % (n, key, code, outcome))
        idle = idle + 1 if outcome in ("released", "left") else 0
        if idle >= 2:
            print("worker: two sessions in a row produced nothing to merge; stopping")
            break
    return 0


if __name__ == "__main__":
    sys.exit(main())

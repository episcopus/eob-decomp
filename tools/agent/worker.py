#!/usr/bin/env python3
"""worker.py: a supervisor that runs model sessions on claimed units (playbook 90).

  make worker AGENT=<name> [ROLE=decompiler|reviewer] [MODEL=openrouter/...] [VARIANT=high] [TIER=1|2]
              [UNIT=<id>] [SESSIONS=1] [TIMEOUT=90] [COST_CAP=0] [MAX_ATTEMPTS=2] [SESSION_CMD=...] [DRY=1]

Normally `make start-run` starts one per roster entry; the human may start one by hand. For each
session (at most SESSIONS):

  1. claim: its own hand-off that `make integrate` rejected (once per worker run, with the reason
     in the prompt), else its own claim a session left with uncommitted changes (once per worker run;
     nothing else would take it: `make next` skips claimed units), else UNIT, else the next unit for
     its tier (`make next`), or a review (a retaken claim left uncommitted again has its changes
     stashed, `git stash list`, and is settled from its commits);
  2. run the session command in the unit's worktree, the prompt (tools/agent/worker_prompt.md,
     filled in) in a file. The default runs OpenCode on its background service, which holds the
     provider credentials, with a session id chosen here so the session can be found again:

       cd {worktree} && opencode run --auto --session {session_id} --title {title}{model_flag} "$(cat {prompt_file})"

     ({model_flag} is ` --model <model>#<variant>`). A template without {prompt_file} gets the
     prompt on standard input. The session is killed (and interrupted on the service, where it would
     otherwise go on) after TIMEOUT minutes, when OpenCode reports COST_CAP USD spent, or on SIGTERM;
  3. settle the claim: the session's own hand-off; else a `partial` hand-off of new commits that
     match; else the claim is released (nothing committed) or left for the human;
  4. count the attempt: a decompiler session that matched no new function and fixed no must-fix
     review finding counts one attempt of its tier on the unit; progress clears the count
     (common.record_attempt). Tier 1 gives a unit
     up after TIER1_ATTEMPTS, tier 2 (the stronger model) after MAX_ATTEMPTS.

Between sessions it restarts itself (same arguments) when tools/agent/worker.py, common.py,
claim.py or next.py changed on disk: a fix merged into main applies to the running workers.

When nothing is claimable it waits (IDLE seconds) and looks again: tier 2 waits for units tier 1
gave up on, reviewers for complete units. A session the provider refused (no credits, model
unavailable) counts no attempt; the worker waits 15 minutes. It stops after SESSIONS, when
build/worker/STOP exists (`make stop-run`), or on SIGTERM. DRY=1 prints the prompt and the command.
"""
from __future__ import annotations

import argparse
import json
import os
import re
import shlex
import signal
import subprocess
import sys
import time
import uuid
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import claim as claim_mod  # noqa: E402
import common  # noqa: E402

PROMPT = Path(__file__).resolve().parent / "worker_prompt.md"
DEFAULT_SESSION_CMD = ('cd {worktree} && opencode run --auto --session {session_id} --title {title}{model_flag} '
                       '"$(cat {prompt_file})"')
COST_POLL_SECONDS = 30
PROVIDER_BACKOFF_SECONDS = 900
KILL_GRACE_SECONDS = 10
# OpenCode's `run` exits 0 even when the provider refused (poolrad, 2026-10-14): read the log.
PROVIDER_ERROR = re.compile(r"exceed your available credits|insufficient credits|insufficient_quota|"
                            r"credit balance is too low|payment required|Model unavailable|"
                            r"ProviderModelNotFound|model not found", re.I)
ROLES = {
    "decompiler": ("docs/process/playbooks/30-lift-module.md",
                   "Run `make packet UNIT={unit}` and read build/packets/{unit}.md. If src/ has no file for "
                   "the unit, `make skeleton UNIT={unit}`. Decompile the functions that are not at L2, in "
                   "address order, as many as you can match.",
                   "complete|partial|blocked"),
    "reviewer": ("docs/process/playbooks/70-review.md",
                 "Copy work/reviews/TEMPLATE.md to work/reviews/{unit}.md and review the unit as the playbook says.",
                 "review-passed|review-failed"),
}


class Stopped(Exception):
    """SIGTERM from `make stop-run`: stop the session, settle the claim, exit."""


def prompt(agent: str, role: str, key: str, branch: str, wt: Path, rejected: str = "", tier: int = 1) -> str:
    unit = key[7:] if key.startswith("review-") else key
    playbook, first, states = ROLES[role]
    if role == "decompiler" and common.open_must_fix(unit):
        first = ("The unit's review, work/reviews/%s.md, has open must-fix findings: that is why you have it, "
                 "even if every function is at L2. Fix each finding from the evidence it cites (a name: `make "
                 "rename`; a comment or a type: in the source), keep `make match FILE=...` passing, tick the "
                 "finding `- [x]` with a note of what you changed, commit, and hand off STATE=complete. Then, if "
                 "functions remain below L2: " % unit) + first
    if tier == 2:
        first = ("Earlier sessions of a cheaper model gave up on this unit without matching more of it: "
                 "read the notes of its functions in re/symbols.toml and `git log main..HEAD` before you "
                 "start, and take one hard function at a time. ") + first
    if rejected == "LEFT":
        first = ("Your last session on this branch ended with uncommitted changes, often a commit the "
                 "pre-commit hook refused (`git commit` prints why). Run `git status` and `git diff --cached`, "
                 "fix what the hook reports, commit, and hand off. Then continue: ") + first
    elif rejected:
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


def left_claims(agent: str) -> list[tuple[str, str]]:
    """(key, reason) of this agent's claims with no hand-off whose worktree has uncommitted changes: a
    session ended before it committed (often a commit the pre-commit hook refused)."""
    out = []
    for p in sorted(common.state_dir("eob-locks").glob("*.lock"), key=lambda p: p.stat().st_mtime):
        f = common.fields(p.read_text())
        key, wt = f.get("key", p.stem), common.worktree_of(f.get("key", p.stem))
        if f.get("agent") != agent or common.handoff_path(key).exists() or not wt.exists():
            continue
        if common.git("status", "--porcelain", "--untracked-files=no", cwd=wt, check=False):
            out.append((key, "LEFT"))
    return out


# ---------------------------------------------------------------- one session

def session_cost(session_id: str) -> float | None:
    """What OpenCode reports spent in the session so far (USD), or None when it cannot say."""
    if not session_id:
        return None
    try:
        r = subprocess.run(["opencode", "api", "get", "/api/session/%s" % session_id], capture_output=True,
                           text=True, timeout=20, stdin=subprocess.DEVNULL)
        return float(json.loads(r.stdout)["data"]["cost"]) if r.returncode == 0 else None
    except (OSError, subprocess.TimeoutExpired, ValueError, KeyError, TypeError):
        return None


def interrupt_session(session_id: str, log) -> None:
    """Stop an OpenCode session on the background service; killing its client does not."""
    try:
        r = subprocess.run(["opencode", "api", "post", "/api/session/%s/interrupt" % session_id],
                           capture_output=True, text=True, timeout=30, stdin=subprocess.DEVNULL)
        log.write("$ interrupt %s: exit %d\n" % (session_id, r.returncode))
    except (OSError, subprocess.TimeoutExpired) as e:
        log.write("$ interrupt %s failed: %s\n" % (session_id, e))


def kill_group(p: subprocess.Popen) -> None:
    for sig in (signal.SIGTERM, signal.SIGKILL):
        try:
            os.killpg(p.pid, sig)
            p.wait(timeout=KILL_GRACE_SECONDS)
            return
        except ProcessLookupError:
            return
        except subprocess.TimeoutExpired:
            continue


def run_session(template: str, wt: Path, env: dict, text: str, timeout_s: float, log: Path,
                model_flag: str = "", title: str = "", cost_cap: float = 0.0, on_start=None) -> dict:
    """Run one session in its own process group; returns {code, session_id, cost, refused, seconds}.
    code is the exit code, "timeout", "cost-cap" or "stopped"."""
    prompt_file = log.with_suffix(".prompt.md")
    prompt_file.write_text(text)
    session_id = "ses_eob" + uuid.uuid4().hex[:16] if "{session_id}" in template else ""
    cmd = template.format(prompt_file=shlex.quote(str(prompt_file)), worktree=shlex.quote(str(wt)),
                          model_flag=model_flag, title=shlex.quote(title), session_id=session_id)
    stdin = subprocess.DEVNULL if "{prompt_file}" in template else subprocess.PIPE
    start = time.monotonic()
    code = None
    with open(log, "a") as out:
        out.write("$ %s   (%s)\n" % (cmd, common.now()))
        out.flush()
        p = subprocess.Popen(cmd, shell=True, cwd=wt, env=env, stdin=stdin, stdout=out, stderr=subprocess.STDOUT,
                             text=True, start_new_session=True)
        if on_start:
            on_start(session_id)
        try:
            if stdin is subprocess.PIPE:
                try:
                    p.stdin.write(text)
                    p.stdin.close()
                except BrokenPipeError:
                    pass
            deadline = start + timeout_s
            while code is None:
                step = min(COST_POLL_SECONDS if cost_cap and session_id else 5, max(deadline - time.monotonic(), 0.01))
                try:
                    p.wait(timeout=step)
                    code = str(p.returncode)
                except subprocess.TimeoutExpired:
                    if time.monotonic() >= deadline:
                        code = "timeout"
                    elif cost_cap and session_id and (session_cost(session_id) or 0.0) >= cost_cap:
                        code = "cost-cap"
        except Stopped:
            code = "stopped"
        if code in ("timeout", "cost-cap", "stopped"):
            kill_group(p)
            if session_id:
                interrupt_session(session_id, out)
        out.write("$ exit %s after %.0f s\n" % (code, time.monotonic() - start))
    tail = log.read_text(errors="replace")[-20000:]
    refused = PROVIDER_ERROR.search(tail)
    return {"code": code, "session_id": session_id, "cost": session_cost(session_id),
            "refused": refused.group(0) if refused else "", "seconds": round(time.monotonic() - start)}


def settle(agent: str, key: str, branch: str, wt: Path, head_before: str = "") -> str:
    """What became of the claim after a session: handed-off, auto-handed-off, released, or left. A
    rejected hand-off is handed off again only when the session committed something new (otherwise
    the integrator would reject the same commit again and again)."""
    h = common.handoff_path(key)
    if h.exists() and common.fields(h.read_text()).get("integrate") != "rejected":
        return "handed-off"
    if h.exists() and (not head_before or common.git("rev-parse", branch, check=False) == head_before):
        return "left"                       # rejected, nothing new (or not known: make stop-run)
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


# ---------------------------------------------------------------- the loop

def stop_requested() -> bool:
    return (common.main_checkout() / "build/worker/STOP").exists()


def wait(seconds: float) -> bool:
    """Sleep, waking for STOP; False when a stop was requested."""
    end = time.time() + seconds
    while time.time() < end:
        if stop_requested():
            return False
        time.sleep(max(0.0, min(5, end - time.time())))
    return not stop_requested()


HARNESS = [Path(__file__).resolve().parent / f for f in ("worker.py", "common.py", "claim.py", "next.py")]


def harness_signature() -> tuple:
    return tuple(p.stat().st_mtime_ns if p.exists() else 0 for p in HARNESS)


def loop(a) -> int:
    logs = common.main_checkout() / "build/worker" / a.agent
    logs.mkdir(parents=True, exist_ok=True)
    model = "%s#%s" % (a.model, a.variant) if a.variant else a.model
    model_flag = (" --model " + shlex.quote(model)) if a.model else ""
    status = dict(role=a.role, tier=a.tier, model=a.model, variant=a.variant, pid=os.getpid())
    retried, n = set(), 0
    signature = harness_signature()
    while n < a.sessions:
        if stop_requested():
            print("worker: build/worker/STOP exists; stopping")
            break
        if not a.unit and harness_signature() != signature:
            # the harness changed on main (make integrate merged a fix): run the new code from the next
            # session on, without a restart of the run
            print("worker: tools/agent changed; restarting the worker with the new code", flush=True)
            common.agent_status(a.agent, state="reloading", **status)
            os.execv(sys.executable, [sys.executable] + sys.argv)
        rejected = [r for r in rejected_handoffs(a.agent) + left_claims(a.agent) if r[0] not in retried][:1]
        retried.update(k for k, _ in rejected)
        try:
            if rejected:
                key = rejected[0][0]
                key, wt = claim_mod.claim(a.agent, key[7:] if key.startswith("review-") else key, key.startswith("review-"))
            else:
                key, wt = claim_mod.claim(a.agent, a.unit if n == 0 else "", a.role == "reviewer",
                                          tier=a.tier, max_attempts=a.max_attempts)
        except (ValueError, RuntimeError) as e:
            if a.unit and n == 0:
                print("worker: %s" % e)
                return 1
            common.agent_status(a.agent, state="idle", note=str(e)[:200], **status)
            if not wait(a.idle):
                break
            continue
        n += 1
        branch = common.fields(common.lock_path(key).read_text())["branch"]
        uid = key[7:] if key.startswith("review-") else key
        log = logs / ("%s-%d-%s.log" % (time.strftime("%Y%m%dT%H%M%S"), n, key))
        before, fixes_before = common.l2_count(wt, uid), common.must_fix_count(wt, uid)
        head_before = common.git("rev-parse", branch, check=False)
        print("worker: session %d on %s (%s%s) in %s, log %s" % (n, key, model or "session_cmd",
              (", left uncommitted" if rejected[0][1] == "LEFT" else ", after a rejection") if rejected else "", wt, log), flush=True)
        started = common.now()

        def on_start(session_id, key=key, log=log, started=started):
            common.agent_status(a.agent, state="session", key=key, session_id=session_id, started=started,
                                log=str(log), **status)
        r = run_session(a.session_cmd, wt, dict(os.environ, AGENT=a.agent), prompt(
            a.agent, a.role, key, branch, wt, rejected[0][1] if rejected else "", a.tier),
            a.timeout * 60, log, model_flag, "%s %s" % (a.agent, key), a.cost_cap, on_start)
        outcome = settle(a.agent, key, branch, wt, head_before)
        if outcome == "left" and rejected and rejected[0][1] == "LEFT" and wt.exists():
            # left uncommitted again after its retake: set the changes aside (recoverable, never
            # deleted) and settle what is committed, so the claim is not held with no one on it
            note = "worker %s: %s left uncommitted twice" % (a.agent, key)
            common.git("stash", "push", "-m", note, cwd=wt, check=False)
            print("worker: %s: uncommitted changes stashed (git stash list: %s)" % (key, note), flush=True)
            outcome = settle(a.agent, key, branch, wt, head_before)
        after = common.l2_count(wt, uid) if wt.exists() else common.l2_count(common.ROOT, uid)
        fixes_after = common.must_fix_count(wt, uid) if wt.exists() else fixes_before
        if a.role == "decompiler" and not r["refused"] and r["code"] != "stopped":
            # progress: more functions at L2, or fewer open must-fix review findings. A session that only
            # rewrote drafts is an attempt, so a unit tier 1 cannot finish reaches tier 2
            common.record_attempt(uid, a.tier, after > before or fixes_after < fixes_before)
        common.event("worker-session", agent=a.agent, key=key, tier=a.tier, model=a.model, outcome=outcome,
                     exit=r["code"], cost=r["cost"], seconds=r["seconds"], session_id=r["session_id"],
                     l2_before=before, l2_after=after, refused=r["refused"])
        print("worker: session %d on %s ended (%s, %ds, USD %s): %s; L2 %d -> %d%s" % (
            n, key, r["code"], r["seconds"], "%.2f" % r["cost"] if r["cost"] is not None else "?", outcome,
            before, after, "; PROVIDER REFUSED: %s" % r["refused"] if r["refused"] else ""), flush=True)
        if r["code"] == "stopped":
            break
        if r["refused"]:
            common.agent_status(a.agent, state="backoff", note=r["refused"], **status)
            if not wait(PROVIDER_BACKOFF_SECONDS):
                break
    common.agent_status(a.agent, state="stopped", **status)
    return 0


def main(argv=None) -> int:
    env = os.environ.get
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--agent", default=env("AGENT", ""))
    ap.add_argument("--role", default=env("ROLE", "decompiler"), choices=sorted(ROLES))
    ap.add_argument("--model", default=env("MODEL", ""))
    ap.add_argument("--variant", default=env("VARIANT", ""))
    ap.add_argument("--tier", type=int, default=int(env("TIER") or 1), choices=(1, 2))
    ap.add_argument("--unit", default=env("UNIT", ""))
    ap.add_argument("--sessions", type=int, default=int(env("SESSIONS") or 1))
    ap.add_argument("--timeout", type=float, default=float(env("TIMEOUT") or 90))
    ap.add_argument("--cost-cap", type=float, default=float(env("COST_CAP") or 0))
    ap.add_argument("--max-attempts", type=int, default=int(env("MAX_ATTEMPTS") or 2))
    ap.add_argument("--idle", type=float, default=float(env("IDLE") or 300))
    ap.add_argument("--session-cmd", default=env("SESSION_CMD") or DEFAULT_SESSION_CMD)
    ap.add_argument("--dry", action="store_true", default=bool(env("DRY")))
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
                     common.worktree_of(key), tier=a.tier))
        mf = (" --model %s#%s" % (a.model, a.variant)) if a.model else ""
        print("\n$ " + a.session_cmd.format(worktree=common.worktree_of(key), session_id="ses_eob<id>",
                                            title="'%s %s'" % (a.agent, key), model_flag=mf, prompt_file="<prompt file>"))
        return 0

    def on_term(sig, frame):
        raise Stopped()
    signal.signal(signal.SIGTERM, on_term)
    try:
        return loop(a)
    except Stopped:
        print("worker: stopped by make stop-run")
        common.agent_status(a.agent, state="stopped", role=a.role, tier=a.tier, model=a.model)
        return 0


if __name__ == "__main__":
    sys.exit(main())

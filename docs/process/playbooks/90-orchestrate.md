# 90 Run the agents and integrate their work (the human)

The process from `../poolrad-decomp`, cut to what this project needs: agents claim units and work in their own git worktrees; a hand-off says a branch is ready; `make integrate` merges it into `main` mechanically, through `make check`. Nothing a model decides is on the path to `main`, and `main` only ever moves by a fast-forward to a checked merge commit.

**The human starts and stops every agent.** Scripts never start a session on their own; `make worker` is a supervisor the human runs.

## The pieces

| Command | Who | What |
|---|---|---|
| `make queue WRITE=1` | human | `work/queue.toml`: the claim order (game units of EOB.EXE, smallest first; reorder by hand) |
| `make next [REVIEW=1]` | anyone | the next unit to decompile (not complete, or with open must-fix review items; not claimed; no hand-off waiting), or to review |
| `make claim AGENT= [UNIT=] [REVIEW=1] [TASK=]` | agent | lock (atomic, in the clone's git directory, seen by every worktree) and worktree `../eob-worktrees/<key>` on `unit/<id>`, `review/<id>` or `task/<name>`; the untracked tools are linked in |
| `make packet UNIT=`, `make skeleton UNIT=` | agent | start a unit (playbook 30) |
| `make handoff AGENT= UNIT= STATE= NOTE=` | agent | the branch is ready (`complete`, `partial`, `blocked`, `review-passed`, `review-failed`), or a question for the human (`escalate`) |
| `make handoffs` | human | what waits: escalations first |
| `make integrate [KEY=] [DRY=1]` | human, or a timer | merge every waiting branch |
| `make handoff-done KEY= DECISION="..."` | human | settle an escalation |
| `make locks`, `make unlock UNIT= AGENT= [FORCE=1 REASON=]` | owner, human | claims; give one back |
| `make progress` | anyone | functions and bytes at L2, units complete and reviewed, claims, hand-offs |
| `make worker AGENT= [ROLE=] [SESSIONS=] [TIMEOUT=] [SESSION_CMD=]` | human | run model sessions on claimed units, one worker by hand |
| `make start-run [DRY=1]`, `run-status`, `run-add NAME=`, `stop-run [WAIT=]` | human | a run of `work/roster.toml`: workers and the integrator in the background |
| `make hooks` (= `make merge-driver`) | human, once per clone | the pre-commit guard and the registry's merge driver (`make claim` and `make integrate` set them too) |

## What `make integrate` does for one hand-off

1. A temporary worktree `build/integrate/<key>` on branch `integrate/<key>` from `main`.
2. The branch's changes are checked (`tools/agent/safety.py`): nothing under `eob/` or `build/`, no en or em dash in an added line, and from a `unit/` or `review/` branch nothing in `docs/spec/`, `re/inputs.toml`, `tools/re/tc_toolchain.toml`, `tools/match/`, `tests/match/negative/` or `tools/agent/`. Those change only on a `task/` branch the human asked for, so a decompiler can never make its own work pass by changing the proof. The pre-commit hook (`make hooks`) applies the same rules at each commit.
3. `git merge --no-ff --no-commit` of the commit that was handed off.
4. `make check` in the temporary worktree: `make inputs`, the fast tests, `make match-gate`, the slow tests (every golden). The check is of exactly the tree that lands (about 40 seconds).
5. One merge commit (`Merge unit/<id> (<state>): <note>`, trailer `Agent:`), `main` fast-forwarded to it, the hand-off archived, the claim released, the agent's worktree removed when clean, its branch deleted.

A failing step leaves `main` untouched and marks the hand-off rejected, with the reason and the log `build/integrate/<key>.log`. The owner runs `make claim` with the same AGENT, gets the worktree back with the reason, fixes, and hands off again. When the fix belongs on `main` instead (a tool), `make integrate KEY=<key>` retries the rejected hand-off.

## The loop

```sh
make handoffs       # escalations first: each needs a human decision
make integrate      # merge everything waiting
make progress
```

## Workers

`make worker AGENT=w1 [ROLE=decompiler|reviewer] [UNIT=<id>] [SESSIONS=3] [TIMEOUT=60]` claims a unit, runs `SESSION_CMD` (default `claude -p --permission-mode acceptEdits`) in its worktree with the prompt of `tools/agent/worker_prompt.md` on standard input, and logs to `build/worker/<agent>/`. After each session the claim must end somewhere: the session's own hand-off; or, when it committed matching work without handing off, a `partial` hand-off by the supervisor; or, when it committed nothing, the claim released. It stops after SESSIONS, when nothing is left to claim, after two sessions in a row that produced nothing, or when `build/worker/STOP` exists. `DRY=1` prints the prompt. Run several workers with different AGENT names; claims keep them apart.

## Runs

`work/roster.toml` names the processes of a run: workers (`role = "decompiler"` or `"reviewer"`, each a `make worker` with the entry's name as its AGENT) and at most one integrator. `[run]` sets `session_cmd` (the model session a worker runs per unit: a CLI that reads the prompt on standard input and works in the current directory), `timeout` (minutes per session) and `sessions` (per worker); an entry may override them.

- `make start-run` checks first and refuses to start unless: main is clean and checked out; `.venv`, the toolchain and the overlays are there; every worker's session command is on the PATH; `make match-gate` passes; no run is active. Then it starts every entry in the background (logs in `build/run/<name>.log`, process ids in the clone's git directory). `DRY=1` prints the commands and what the check would refuse.
- `make run-status`: each entry alive or stopped, its claim, the last line of its log; then `make progress`.
- `make run-add NAME=<name>` starts (or restarts) one entry.
- The integrator (`tools/agent/integrator.py`) runs `make integrate` every `interval` seconds when a branch waits. After `idle_stop` minutes with no claim, nothing waiting and no worker alive, it ends the run (creates `build/worker/STOP`) and exits.
- `make stop-run [WAIT=30]` drains: `build/worker/STOP` makes every worker stop after its current session; after WAIT minutes the rest are sent SIGTERM, which stops their session and settles their claim (a hand-off when the work matches, a release when there is none). The integrator finishes the merge it is in, then stops. The claims the run's workers still hold are settled; those that cannot be (uncommitted or unmatched work) are listed for the human. A last `make integrate`, then the run's state is cleared.

Only one `make integrate` runs at a time (a lock in the clone's git directory), so a human can run it during a run.

## The registry merges entry by entry

Every worker writes `re/symbols.toml` (`make match WRITE=1`), and may note something in the entry of another unit's function (a signature, a note). Git's line merge conflicts when two branches change one entry, even different fields of it. `.gitattributes` gives `re/symbols.toml` and `re/units.toml` a merge driver (`tools/agent/merge_registry.py`): entries by `id`, fields by key; a field one side changed wins; both changed: `level` keeps the higher, `review` the later, `notes` both joined; any other field (a name, a signature) is a real conflict, and `make integrate` rejects the later hand-off with it.

## Rehearsal (2026-11-01)

The whole loop ran in a throwaway clone: claim, skeleton, hand-off; `make integrate` rejected the first merge because two goldens depended on `eob/LEVELS.TMP`, a file the game writes that existed in one checkout and not in a clean one (fixed: the oracle now hides unpinned files and derives `LEVELS.TMP` from the session's save, whitepaper 4.4); after the fix `KEY=` retried and merged it. The hook refused a matcher change and a dash; `make handoff` refused a forced commit to the matcher, a unit that did not match, and an agent that did not own the claim; `make unlock` refused a branch with commits without FORCE; a review went from claim to `review-pass` to merge; a worker with a stand-in session handed off a session's commit as `partial` and stopped after two empty sessions.

Two runs followed, with stand-in sessions. In the first, two decompilers and a reviewer worked through two sessions each; the integrator merged the four hand-offs and ended the run when idle. It also showed a race: two workers started together chose the same unit, and the loser gave up; `make claim` now takes the next unit instead. In the second, three workers started together took three different units; `make stop-run WAIT=0.1` stopped them mid-session, handed off their committed work, merged it, and left no claim, worktree or process behind. No run has used a real model session yet.

## Escalations and rejections

Escalations are never merged: `make handoffs` lists them first; the human decides and records it with `make handoff-done KEY=<key> DECISION="..."`. A rejected hand-off goes back to its owner: a worker takes its own rejected hand-offs first (once per worker run), with the reason in the prompt; it hands the branch off again only when the session committed something new, so the integrator never rejects the same commit twice. A real conflict (two names for one function) is resolved from evidence on the owner's branch, never on `main`.

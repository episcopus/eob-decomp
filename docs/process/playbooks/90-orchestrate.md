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
| `make smoke-agent [NAME=] [UNIT=] [TIMEOUT=] [WORKTREE=1]` | human | one real session of a roster model on one unit, in a scratch clone, merged through `make check`: does the model do the job |
| `make watch-agents [INTERVAL=5]` | human | the live view of a run |

## What `make integrate` does for one hand-off

1. A temporary worktree `build/integrate/<key>` on branch `integrate/<key>` from `main`.
2. The branch's changes are checked (`tools/agent/safety.py`): nothing under `eob/` or `build/`, no en or em dash in an added line, and from a `unit/` or `review/` branch nothing in `docs/spec/`, `re/inputs.toml`, `tools/re/tc_toolchain.toml`, `tools/match/`, `tests/match/negative/` or `tools/agent/`. Those change only on a `task/` branch the human asked for, so a decompiler can never make its own work pass by changing the proof. The pre-commit hook applies the same rules at each commit.
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

## Git settings of the clone

Two settings live in the clone's git configuration, which git never versions, so they are set by the tools rather than committed: `core.hooksPath` (the pre-commit guard in `tools/agent/hooks/`, the same rules as `safety.py` at every commit, also yours in the main checkout: no change to `eob/` or `build/`, no en or em dash) and the merge driver of the registry (below). `make claim`, `make integrate` and `make start-run` set both (`common.install_git_config`); there is nothing to run by hand.

## Workers

A worker (`tools/agent/worker.py`) claims a unit, runs one model session in its worktree, settles the claim, counts the attempt, and claims again:

- **The session** is OpenCode on its background service (which holds the OpenRouter credentials): `cd <worktree> && opencode run --auto --session <id> --title "<agent> <unit>" --model <model>#<variant> "$(cat <prompt file>)"`, the prompt from `tools/agent/worker_prompt.md`. The worker picks the session id, so it can read the session's cost from the service (`opencode api get /api/session/<id>`) and interrupt it there when it kills the client (timeout, cost cap, `stop-run`); killing the client alone would leave the session editing the worktree. A session the provider refused (no credits, an unknown model) counts no attempt; the worker waits 15 minutes.
- **The claim** ends in the session's own hand-off; or a `partial` hand-off of new commits that match; or a release when nothing was committed; or it is left for the human.
- **Tiers.** A decompiler session that records no new function at L2 counts one attempt of its tier on the unit; progress clears the count, so a big unit goes on as long as sessions keep matching. Tier 1 (DeepSeek) gives a unit up after two attempts; tier 2 (Sonnet) takes only those units, up to `max_attempts` sessions each; units both gave up on wait for the human (`make watch-agents` lists them; the counts are in the clone's git directory, `eob-attempts.json`).
- **Idle.** Nothing to claim: the worker waits (`idle` seconds) and looks again; tier 2 waits for tier 1 to give up on something, reviewers for complete units.

`make worker AGENT=<name> MODEL=<id> VARIANT=<v> [ROLE=] [TIER=] [UNIT=] [SESSIONS=] [TIMEOUT=] [COST_CAP=]` runs one by hand; `DRY=1` prints its prompt and command. Logs: `build/worker/<agent>/` (one per session, with its prompt beside it).

## Before the first run: the smoke test

`make smoke-agent [NAME=dec-deepseek] [UNIT=e19ec] [TIMEOUT=30] [WORKTREE=1]` proves that a roster entry's model can do the job: in `build/smoke/repo`, a fresh clone of HEAD (WORKTREE=1 adds the uncommitted changes), it runs that entry's worker for one session on one unit, exactly as a run would, then `make integrate` in the clone. PASS: the provider accepted the session, the claim ended in a hand-off, the unit gained functions at L2, and the merge passed `make check`. It prints the cost, the time, the log and where the clone is. Nothing in the main checkout changes. Run it for each model of the roster before its first run, and again after a change to the prompt or the tools.

## Runs

`work/roster.toml` names the processes of a run: the poolrad roster of 2026-10-14 (three DeepSeek V4.1 Flash decompilers at tier 1, two Claude Sonnet 5.5 escalation decompilers at tier 2 with a 150-minute and USD 8 limit per session, one Sonnet reviewer) and a script integrator. Each worker entry has `model` (an id of `opencode models`) and `variant`; `timeout`, `cost_cap`, `max_attempts`, `sessions`, `idle` and `session_cmd` may be set per entry or under `[run]`.

- `make start-run` checks first and refuses to start unless: main is clean and checked out; `.venv`, the toolchain and the overlays are there; the session program (`opencode`) is on the PATH and its service answers; every model is in `opencode models`; `make match-gate` passes; no run is active. Then it starts every entry in the background (logs in `build/run/<name>.log`, process ids in the clone's git directory). `DRY=1` prints the commands and what the check would refuse.
- `make watch-agents`: the live view. Progress on main as bars and percentages (functions, bytes, units) and what the worktrees matched but have not merged yet; the velocity since the run started (functions and bytes merged per hour) with an ETA for the rest at that pace (by bytes) and what it would cost; each agent with its state (a session on a unit with its minutes and cost so far, idle, backing off, stopped) and the last line of its log; the costs (per agent: sessions, functions matched, USD finished and running, USD per function matched; per model; this run with USD per hour; every run so far); hand-offs (escalations and rejections first); attempts and the units both tiers gave up on; the latest events. `ONCE=1` prints it once.
- `make run-status`: a shorter list; `make run-add NAME=<name>` starts (or restarts) one entry.
- The integrator (`tools/agent/integrator.py`) runs `make integrate` every `interval` seconds when a branch waits. After `idle_stop` minutes with no claim and nothing waiting (the queue is done, or what is left waits for the human), it ends the run (creates `build/worker/STOP`) and exits.
- `make stop-run [WAIT=30]` drains: `build/worker/STOP` makes every worker stop after its current session; after WAIT minutes the rest are sent SIGTERM, which stops their session (on the service too) and settles their claim. The integrator finishes the merge it is in, then stops. The claims the run's workers still hold are settled; those that cannot be (uncommitted or unmatched work) are listed for the human. A last `make integrate`, then the run's state is cleared.

Only one `make integrate` runs at a time (a lock in the clone's git directory), so a human can run it during a run.

## The registry merges entry by entry

Every worker writes `re/symbols.toml` (`make match WRITE=1`), and may note something in the entry of another unit's function (a signature, a note). Git's line merge conflicts when two branches change one entry, even different fields of it. `.gitattributes` gives `re/symbols.toml` and `re/units.toml` a merge driver (`tools/agent/merge_registry.py`): entries by `id`, fields by key; a field one side changed wins; both changed: `level` keeps the higher, `review` the later, `notes` both joined; any other field (a name, a signature) is a real conflict, and `make integrate` rejects the later hand-off with it.

## Rehearsal (2026-11-01)

The whole loop ran in a throwaway clone: claim, skeleton, hand-off; `make integrate` rejected the first merge because two goldens depended on `eob/LEVELS.TMP`, a file the game writes that existed in one checkout and not in a clean one (fixed: the oracle now hides unpinned files and derives `LEVELS.TMP` from the session's save, whitepaper 4.4); after the fix `KEY=` retried and merged it. The hook refused a matcher change and a dash; `make handoff` refused a forced commit to the matcher, a unit that did not match, and an agent that did not own the claim; `make unlock` refused a branch with commits without FORCE; a review went from claim to `review-pass` to merge; a worker with a stand-in session handed off a session's commit as `partial` and stopped after two empty sessions.

Two runs followed, with stand-in sessions. In the first, two decompilers and a reviewer worked through two sessions each; the integrator merged the four hand-offs and ended the run when idle. It also showed a race: two workers started together chose the same unit, and the loser gave up; `make claim` now takes the next unit instead. In the second, three workers started together took three different units; `make stop-run WAIT=0.1` stopped them mid-session, handed off their committed work, merged it, and left no claim, worktree or process behind. Then the real models, by `make smoke-agent` (2026-11-01): DeepSeek V4.1 Flash (high) matched all 3 functions of `e19ec` in 83 s for USD 0.02; Claude Sonnet 5.5 (xhigh, tier 2) all 3 of `e1a47` in 4.3 minutes for USD 0.62; the Sonnet reviewer passed `e0cf7` in 63 s for USD 0.17 with findings that cite addresses (one of them a gap in `make packet`: near calls to targets below 10 were missed, fixed). Each hand-off merged through `make check` in the scratch clone. A last stand-in run with `make watch-agents` open found two more things, both fixed: a test that pinned the roster's entries (so editing the roster would have failed `make check`; it now checks the roster's rules), and `make stop-run` handing a rejected claim off again unchanged (it now leaves it to its owner).

## Escalations and rejections

Escalations are never merged: `make handoffs` lists them first; the human decides and records it with `make handoff-done KEY=<key> DECISION="..."`. A rejected hand-off goes back to its owner: a worker takes its own rejected hand-offs first (once per worker run), with the reason in the prompt; it hands the branch off again only when the session committed something new, so the integrator never rejects the same commit twice. A real conflict (two names for one function) is resolved from evidence on the owner's branch, never on `main`.

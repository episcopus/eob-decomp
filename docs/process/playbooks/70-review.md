# 70 Review a complete unit

**Inputs:** a unit whose functions are all at L2 on `main` and nobody has reviewed (`make next REVIEW=1`).

The matcher already proved the bytes. A review judges what matching code can still get wrong: a name that claims more than the evidence shows, a type that compiles to the same bytes but means something else (`int` for a count that is really a flag set, a struct whose fields are guesses), a comment that is wrong or missing, a global that `src/` names differently elsewhere. Read for meaning, not for bytes; an independent reader is the point, so a unit is never reviewed by the agent that decompiled it.

## Steps

1. `make claim AGENT=<you> REVIEW=1 [UNIT=<id>]`: lock `review-<id>`, worktree `../eob-worktrees/review-<id>`, branch `review/<id>`.
2. `cp work/reviews/TEMPLATE.md work/reviews/<id>.md`; fill in `Reviewer:` and the commit.
3. Read `src/<ID>.C` (or `.ASM`) with `make packet UNIT=<id>` beside it, against `docs/process/READABILITY.md` ("What the reviewer judges"). For each name, ask what evidence backs it (an address, a caller, a data file, a string). For each comment, check it against the code. Look up every global and callee in the other files of `src/` (the packet lists the names they already use).
4. Write findings as checkboxes: `- [ ] must-fix` for a claim without evidence or a wrong one; `should` for clarity; `note` for the next reader. A placeholder name with no evidence yet is fine and not a finding; a confident name without evidence is a must-fix.
5. No must-fix: `make review-pass UNIT=<id> AGENT=<you>` (checks that the unit is complete and matches, that the review names you and has no open must-fix, then stamps `review = "passed <date> <you>"` on every function in `re/symbols.toml`). Commit both files; `make handoff AGENT=<you> UNIT=<id> STATE=review-passed NOTE="..."`.
6. Must-fix items: commit the review alone; `make handoff ... STATE=review-failed`. Once merged, `make next` offers the unit to decompilers again until someone fixes the items (ticks them `[x]` with the commit) and hands off; then it can be reviewed again.

## Later rounds: what a re-review judges

A unit whose review file already has a round was read in full once; a later round does not read it all again (2026-11-02: eight units went 12 to 18 rounds, each finding something new in text earlier rounds had accepted, and reviews were half of the run's cost). Add a round at the top of the findings and judge only:

- each must-fix of the previous round: fixed, and fixed right (tick stays), or not (it stays open, a must-fix again);
- what changed since the commit the previous round reviewed: `git diff <that commit> main -- src/<ID>.* re/symbols.toml re/globals.toml`. A wrong claim in a changed line is a must-fix, as in round 1.

Something you notice in text no fix touched is at most a `note`, never a must-fix: round 1 accepted it. When the previous round's must-fix items are fixed right and the changed lines are sound, the unit passes (`make review-pass`).

**Citations.** A comment points to other code by its address (`1A72:0200`, `ovl3:0A7C`), which never moves, not by a line of another file of `src/`, which moves whenever that file changes. A line citation into another file that has gone stale is a `should` (replace it with the address), never a must-fix; a citation of the wrong code (the address does not do what the comment says) is a must-fix.

A reviewer does not edit the source: findings go in the review file. A suspected matcher or spec problem is an escalation (`STATE=escalate`).

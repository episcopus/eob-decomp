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

A reviewer does not edit the source: findings go in the review file. A suspected matcher or spec problem is an escalation (`STATE=escalate`).

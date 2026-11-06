# Phase 6 review: one reviewer per subsystem

Decided by the human on 2026-11-04 (playbook 65, "The review sign-off"). Each subsystem below is
one reviewer session. The reviewer must not be the agent that did the naming (`operator`). Every unit
already passed a Phase 4 review, so each unit gets a **later round** (playbook 70): the reviewer judges
what changed since the commit its last round reviewed. In Phase 6 that means the names, the struct
fields and the comments.

| # | Subsystem | Units |
|---|---|---|
| 1 | Start-up, memory, files, overlay manager | e07c8 e07a9 e19bc e1990 e1b82 e1c0e e1e8b e1a63 e1d8c e1ba8 e1bb1 e1c00 |
| 2 | Graphics and the screen | e17a6 e19ec e19f3 e19fc e1a16 e1a19 e1a1f e1a29 e1a47 e1a65 e1aa6 e1ba4 e1ddd e1de4 e1de7 e1d9a e19ea e14ef |
| 3 | Input, mouse, screen nodes, text entry | e16a5 e1658 e0cf7 e1a72 e1aad e1bbe e1bf3 e082c e1d8f |
| 4 | Sound | e_text e1948 e177d |
| 5 | Characters and the rules | e0a72 e0d1c e1547 e1aaa |
| 6 | Monsters and combat | e08e6 e0de8 |
| 7 | Spells and timed effects | e0ef3 e1414 |
| 8 | Maze, doors, movement, level loading | e122e eov3 |
| 9 | Items and the adventure-screen actions | e0bd2 e0acd eov2 |
| 10 | The first-person view | eov4 |
| 11 | Panels, camp, rest, the character screen | eov0 eov1 eov5 eov6 |
| 12 | INTRO.EXE and START1.EXE | iov0 i0892 i0aed i0921 i0923 i0b10 i0cda i_text s_text |

A subsystem is signed off when every unit in it has passed again (`make review-pass`).

**Signed off 2026-11-06**: every unit of all twelve subsystems has passed its P6 round (reviewers
p6-rev-1 to p6-rev-13; the last round, p6-rev-13, re-reviewed e1a29, eov2 and e082c after their
fixes). The review is closed; `src/` is open for step D of playbook 80.

## The prompt

Paste this into a new agent session started in `<your clone>`, with `<N>` replaced by
the subsystem's number from the table and `<AGENT>` by a reviewer name not used before (for
example `p6-rev-<N>`). Several subsystems can run at once; each claims its own units.

```text
You are an independent reviewer for a matching decompilation of Eye of the Beholder (DOS) into C,
in <your clone>. Read AGENTS.md, docs/process/playbooks/70-review.md (especially
"Later rounds"), docs/process/playbooks/65-consistency.md and docs/process/READABILITY.md
("What the reviewer judges") before anything else. Then read work/p6-review.md and take
subsystem <N>. Your agent name is <AGENT>.

What happened: Phase 6 renamed about 1,100 functions, globals and struct fields from names built
from their addresses to real names, and rewrote comments to match. The names never change the
machine code (make match proves that); your job is to judge whether each new name and comment is
TRUE: backed by evidence, not claiming more than the code shows, and the same word for the same
thing across files. You did not write these names. Be skeptical.

For each unit of your subsystem, in the table's order:

1. make claim AGENT=<AGENT> REVIEW=1 UNIT=<id>, then cd into the worktree it prints.
2. Open work/reviews/<id>.md. Its "Commit reviewed" line names the commit the last round read.
   Add a new round at the top of the findings: "Round P6 (2026-11-04), commit <git rev-parse
   HEAD>", and update "Reviewer:" and "Commit reviewed:".
3. Read git diff <that commit> HEAD -- src/<ID>.* (the file is src/<ID>.C or src/<ID>.ASM with the
   id in upper case). For every name the diff introduces, find its evidence: grep re/renames.toml
   and re/fields.toml for the address or the new name (each entry logs a WHY). Check the WHY
   against the code at that address: does the function or global do what the name says? Read its
   callers and callees in the other files of src/ (grep the name). For every changed comment,
   check it against the code it describes.
4. Write findings as checkboxes in the new round:
   - must-fix: a name or comment that is wrong or claims more than the evidence shows (say what
     the code shows instead, with an address);
   - should: a clearer name or a missing explanation;
   - note: anything for the next reader.
   Something wrong in text the diff did not touch is at most a note. A name marked [H] or
   "inferred" that says so honestly is fine. Do not edit src/ or re/: findings only.
5. No open must-fix: make review-pass UNIT=<id> AGENT=<AGENT>, commit the review file and
   re/symbols.toml, then make handoff AGENT=<AGENT> UNIT=<id> STATE=review-passed
   NOTE="P6 round: <one line>". Must-fix items: commit the review file alone, then
   make handoff ... STATE=review-failed NOTE="P6 round: <n> must-fix".
6. Go back to <your clone> and take the next unit.

Rules: no em dashes or en dashes in any file or message. Never touch eob/ or build/, never run
git push, never run make integrate (the human integrates). A problem with the matcher, the spec
or the tools is make handoff ... STATE=escalate, not a fix. When the subsystem is done, reply with
one short paragraph: units passed, units failed, and the must-fix items that matter most.
```

When the reviewers are done, `make handoffs` lists their branches and `make integrate` merges them
through `make check`. The operator fixes the must-fix items, and those units go back to a reviewer.

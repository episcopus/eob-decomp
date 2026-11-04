# AGENTS.md: operating manual for AI agents

You are working on a **matching decompilation of Westwood's Eye of the Beholder (DOS) into C**: source that the original Turbo C++ toolchain turns into the original's machine code, function by function. The process follows `../poolrad-decomp` (its SPEC 0.8 and playbooks), adapted from Turbo Pascal to Turbo C++. Read this file fully at the start of every session.

## Read first

1. `docs/spec/SPEC.md`: goals, approach (ADR-001), verification ladder, phases. It wins over anything else except a direct human instruction.
2. `work/status.md`: where the project stands.
3. The playbook for your task in `docs/process/playbooks/`.
4. `docs/design/WHITEPAPER.md`: what is already known. Do not rediscover it; extend it.

## Facts you must not get wrong

- The programs (`EOB.EXE`, `INTRO.EXE`, `START1.EXE`) were built with **Borland Turbo C++ 1.00, `-ml -O -Z` (`START1.EXE`: `-ms`, small model), and its TLINK 3.0**, with VROOMM overlays appended to the EXE as FBOV data. Not 1.01 (`make rtl-compare`, whitepaper 2.1). The assembler is provisionally TASM 2.0 [H].
- **The copy in `eob/` is the fan patch "1.9"**, not the retail release (SPEC section 2). Some bytes may be hex edits no compiler wrote; record them as patch sites, never force them with C.
- `LEVELS.TMP` and `EOBDATA.SAV` are written by the game and not pinned. Never run the game inside `eob/` from a tool; copy the files first. The oracle hides them: a session that needs a saved game names one (`option save fixtures/EOBDATA.SAV`), and `LEVELS.TMP` is made from it.
- **The Borland runtime library is library**: linked, never decompiled.
- **The compiler is proprietary**: it lives in `build/toolchain/` (ignored by git), is installed by the human (`docs/setup/SETUP-tc.md`), and is never committed.
- Segment boundaries come from TLINK's segment table (`make overlays`), not from guesses.
- **Switches can differ by unit** (`-G` in `e1b82`); the file says which (`/*@flags ... */`).

## Ground rules

1. **Evidence or it did not happen.** Every name, type and claim cites an address, a tool output or a test. Prior art and recall are leads, tagged [H] until proven on our bytes. How Turbo C++ compiles an idiom is settled by compiling it, never by recall.
2. **Tools write verdicts, you do not.** No raw machine code (`__emit__`, `db` bytes, inline byte strings) to force a match.
3. **Never touch** `eob/` (pinned originals) or `build/` contents in commits, and never copy code from GPL sources (`docs/reference/prior-art.md`).
4. **Small commits.** Message format: `<module>: <what> (<function ids>) [Lx]`.
5. **Stop and ask the human** before changing the spec, `re/inputs.toml`, or anything under `eob/`, and before downloading software or signing in anywhere.
6. **Write plainly.** No em dashes or en dashes in any file or message; use commas, colons, semicolons or parentheses.

## Commands (from the repository root)

| Command | Purpose |
|---|---|
| `make setup` | Create `.venv` with the Python tools |
| `make doctor` | Check the environment and pinned inputs |
| `make inputs` | Verify hashes of `eob/` against `re/inputs.toml` |
| `make overlays` | TLINK segment table and FBOV overlay manifest of each program (`build/re/<program>.json`, overlay code in `build/re/<program>-overlays/`) |
| `make pak` | Verify every PAK archive, byte-exact round trip |
| `make emu2` | Build the headless DOS emulator for the compiler |
| `make tc-install` / `make tc-check` | Install the Turbo C++ and TASM releases from `build/toolchain/dist/`; verify them |
| `make rtl-compare` | Which Turbo C++ release's runtime is in `EOB.EXE` (several minutes) |
| `make map` | Enumerate functions; write `re/units.toml` and `re/symbols.toml` (playbook 10) |
| `make match [FILE=src/X.C] [WRITE=1]` | Prove C and assembly units against their program (`EOB.EXE`, `INTRO.EXE`); `WRITE=1` records L2 (playbook 30) |
| `make rebuild [PROGRAM=START1.EXE] [V=1]` | Build a program from `src/` and compare it with the original (playbook 50) |
| `make twin [UNIT=] [WRITE=1] [FIND=1]` | Prove an `INTRO.EXE` unit by its `EOB.EXE` twin, the same module linked elsewhere (playbook 30) |
| `make data` | Decode every data file: the PAKs, the fonts, the test save (whitepaper 4) |
| `make oracle-run [INSNS=] [ARGS=] [SCRIPT=]` | Run the original `EOB.EXE` (or `--program INTRO.EXE`) in the emulator (playbook 25) |
| `make golden [NAME=] [CHECK=1] [REBUILT=1]` | Record or verify the golden sessions in `tests/sessions/`; `REBUILT=1` replays them on the programs `make rebuild` linked (playbook 50) |
| `make test-slow` | The tests that run the game (every golden, on the originals and on the rebuilds) |
| `make test` | Python tests (the fast set) |
| `make p0-check` | Phase 0 exit criteria |
| `make match-gate` | Every `src/` unit matches, every L2 function is still proven, every negative fixture is rejected |
| `make check` | Everything a merge must pass: inputs, fast tests, match-gate, slow tests (`make integrate` runs it) |
| `make progress`, `make next [REVIEW=1]`, `make queue [WRITE=1]` | Where things stand; what to take next; the claim order (`work/queue.toml`) |
| `make claim AGENT= [UNIT=] [REVIEW=1] [TASK=]` | Lock a unit and get its worktree `../eob-worktrees/<key>` (playbooks 30, 70) |
| `make rename OLD= NEW= WHY=`, `make readability [UNIT=]` | One name per address, everywhere, with its evidence; the naming and comment rules (`docs/process/READABILITY.md`) |
| `make names [UNIT=] [SHARED=1]`, `make field STRUCT= AT= NEW= WHY=` | What is still named by its address and where files disagree; one struct field name in every file that declares the struct (playbook 65) |
| `make packet UNIT=`, `make skeleton UNIT=` | Everything to start a unit (`build/packets/<id>.md`); its source file with every function a stub |
| `make handoff AGENT= UNIT= STATE= NOTE=` | From your worktree: the branch is ready, or a question for the human (`STATE=escalate`) |
| `make review-pass UNIT= AGENT=` | Record a passed review (playbook 70) |
| `make handoffs`, `make integrate`, `make handoff-done`, `make locks`, `make unlock`, `make worker` | The human's side (playbook 90) |
| `make smoke-agent [NAME=] [UNIT=]` | One real session of a roster model on one unit, in a scratch clone (playbook 90) |
| `make start-run [DRY=1]`, `make watch-agents`, `make run-status`, `make run-add NAME=`, `make stop-run [WAIT=]` | A run of `work/roster.toml`: workers and the integrator in the background (playbook 90) |

## Playbooks

| Playbook | When |
|---|---|
| `00-bootstrap.md` | Setting up or repairing the environment |
| `10-map.md` | Segments, units, functions |
| `25-oracle.md` | Running the original in the emulator |
| `30-lift-module.md` | Decompiling a unit (C or assembly) to L2: claim, packet, annotations, stubs, drafts, hand-off, the idioms found so far |
| `50-rebuild.md` | Phase 5: building a program from `src/` identical to the original |
| `65-consistency.md` | Phase 6: one vocabulary, every symbol named where the evidence allows |
| `70-review.md` | Reviewing a complete unit |
| `90-orchestrate.md` | The human: running workers, integrating hand-offs, escalations |

Later playbooks are brought over from poolrad as each phase starts (SPEC section 10).

## Working as one of several agents

Work only in the worktree `make claim` gives you, on its branch; never in the main checkout, never on `main`. Commit small; finish with `make handoff` from the worktree, then stop. A unit or review branch cannot change the spec, the pins, the matcher, its negative fixtures or the agent tools (the pre-commit hook and `make handoff` refuse it): a problem there is `STATE=escalate`.

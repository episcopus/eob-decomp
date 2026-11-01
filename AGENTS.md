# AGENTS.md: operating manual for AI agents

You are working on a **matching decompilation of Westwood's Eye of the Beholder (DOS) into C**: source that the original Turbo C++ toolchain turns into the original's machine code, function by function. The process follows `../poolrad-decomp` (its SPEC 0.8 and playbooks), adapted from Turbo Pascal to Turbo C++. Read this file fully at the start of every session.

## Read first

1. `docs/spec/SPEC.md`: goals, approach (ADR-001), verification ladder, phases. It wins over anything else except a direct human instruction.
2. `work/status.md`: where the project stands.
3. The playbook for your task in `docs/process/playbooks/`.
4. `docs/design/WHITEPAPER.md`: what is already known. Do not rediscover it; extend it.

## Facts you must not get wrong

- The programs (`EOB.EXE`, `INTRO.EXE`, `START1.EXE`) were built with **Borland Turbo C++ (1990 banner) and TLINK 3.0**, with VROOMM overlays appended to the EXE as FBOV data. The exact release is a Phase 1 result; until then do not assume 1.0 or 1.01.
- **The copy in `eob/` is the fan patch "1.9"**, not the retail release (SPEC section 2). Some bytes may be hex edits no compiler wrote; record them as patch sites, never force them with C.
- `LEVELS.TMP` and `EOBDATA.SAV` are written by the game and not pinned. Never run the game inside `eob/` from a tool; copy the files first.
- **The Borland runtime library is library**: linked, never decompiled.
- **The compiler is proprietary**: it lives in `build/toolchain/` (ignored by git), is installed by the human (`docs/setup/SETUP-tc.md`), and is never committed.
- Segment boundaries come from TLINK's segment table (`make overlays`), not from guesses.

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
| `make test` | Python tests |
| `make p0-check` | Phase 0 exit criteria |

## Playbooks

| Playbook | When |
|---|---|
| `00-bootstrap.md` | Setting up or repairing the environment |

Later playbooks are brought over from poolrad as each phase starts (SPEC section 10).

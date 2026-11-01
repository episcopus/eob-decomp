# Readability: what "readable" means for this source

The C in `src/` is the starting point of every port and the most exact description of the game there is. A function that matches but is called `f1DDD_0006(int a, int b)`, reads `gFE6C` and compares with `19` proves the bytes and explains nothing; whoever ports it has to decompile it a second time. So readability is part of decompiling, not a later phase: **a function is done when it matches and reads well**, and a unit is done when it is complete, readable and reviewed (from poolrad-decomp's READABILITY.md, adapted to C).

Names and comments never change the code: `make match` proves a renamed, commented function exactly as before. There is no reason to postpone them.

## When

- **The decompiler** makes every function readable before committing it (playbook 30): it names the function, its parameters, its locals and the globals it uses as far as the evidence goes, comments it, and replaces magic numbers by named constants (`#define` or `enum`). `make handoff` and `make check` refuse a file whose functions break a checkable rule (below).
- **The reviewer** judges what no tool can (playbook 70): whether each name and comment is true, specific and backed by evidence.
- **Phase 6** then harmonises names across units once the whole game is known; it is not where naming happens.

## Names

| What | Style | Examples |
|---|---|---|
| Function | PascalCase verb phrase saying what it does; a function says what it returns; a test reads as a question | `WaitFrames`, `LoadPacked`, `RandomRange`, `IsPartyDead` |
| Global | `g` + PascalCase noun naming what it holds | `gObjSeg`, `gDrawText`, `gSoundOn` |
| Parameter, local | camelCase, its role in this function; `i`, `j`, `k`, `n` as counters, `x`, `y` as coordinates | `count`, `handle`, `srcSeg` |
| Constant | `#define` or `enum` in upper case with underscores; the meaning, not the value | `KEY_ESC`, `MAX_ITEMS` |
| Type | `struct` and `typedef` names in PascalCase | `struct Item`, `struct Obj28` until the meaning is known |
| Unit | the file name stays the id (`E1B82.C`); what the unit is goes in its first comment | |

- **A name is a claim and needs evidence**: an address, a string, a caller, a data file, behaviour seen in the oracle. The evidence goes in the function's comment, or in the `WHY` of `make rename` (logged in `re/renames.toml`), or in the whitepaper.
- **Name what is known, not more.** `DrawsAtRow24` is better than a guess like `ShowCombatLog`. When nothing is known yet, keep the placeholder and say `Purpose unknown:` in the function's comment with what is known (callers, globals, strings); the reviewer accepts that when the evidence really is missing.
- **One address, one name.** Every file declares what it uses (`extern int gFE6C; /*@ ds:FE6C */`), so a global or a callee can get a different name in each file. Give it its name everywhere with `make rename OLD=<name or address> NEW=<Name> WHY="<evidence>"`: every file that declares the address, `re/symbols.toml` (functions) or `re/globals.toml` (globals), and the whitepaper, in one step, logged in `re/renames.toml`. When you declare an address that already has a name (`make packet` lists the names `src/` uses), use that name. `make integrate` applies the recorded renames to every merge (`make rename APPLY=1`), so a branch that used an old name is corrected. On a unit branch, `make rename` changes only your unit's file (and the registry and the log); the other files that declare the address are renamed when your branch merges, so branches never edit each other's units.
- **Types say what values are**: `unsigned char` for bytes, a `struct` instead of offsets into a byte array once the layout is known, `far` pointers where the code uses them. Keep whatever compiles to the same code (`make match` decides).

## Comments

- **Unit:** the file starts with a comment: the unit's address and size, what it is for, its main functions, the whitepaper section.
- **Function:** right above its definition: its address (`/* 1A16:000B: ...`), what it does in game terms, what its parameters and result mean, the globals it reads or changes, its side effects (screen, files, sound, hardware), and anything surprising.
- **Inside a function:** only where the code is not obvious: why, not what.
- **Quirks:** every behaviour a port must keep although it looks wrong is marked `QUIRK <address>:` with what happens (dead code, unused results, off-by-ones). Never fixed.
- **Placeholder globals** (`gFE6C`): their declaration carries a comment with what is known.

## What the tool checks (`make readability [UNIT=<id>]`; the gate is part of `make check`)

For every function a file defines (not its stubs), and for the file:

| Rule | Checks |
|---|---|
| R1 | The file starts with a comment of 8 words or more that is not the skeleton's |
| R2 | The function has a real name, not a placeholder (`f1DDD_0006`, `Ovl2_0110`), or its comment says `Purpose unknown:` |
| R3 | A comment right above the function, 8 words or more, with its address (C) |
| R4 | No parameter or local is a single letter (except `i`, `j`, `k`, `n`, `x`, `y`) or a placeholder (`tmp`, `val`, `a1`, `arg2`) (C) |
| R5 | The file uses the recorded name of every address in `re/renames.toml`, and one name per address across `src/` |

The gate fails on findings that are not in `work/readability-baseline.txt` (the findings that existed when the rules came in, 2026-11-01; fix them and delete their lines). In `make integrate` it judges only the files the hand-off changed: touching a file means making it pass.

## What the reviewer judges (playbook 70)

1. Every name is true (the code does what the name says) and specific (which thing, not just "data" or "handle").
2. Every name beyond the obvious has evidence; `Purpose unknown:` is used only when evidence is really missing.
3. The comments are accurate, say what the code means in game terms, and cover parameters, result, globals and side effects.
4. Magic numbers that mean something (keys, colours, limits, interrupt functions, offsets into records) are named constants or struct fields.
5. Types fit the values.
6. Quirks are marked `QUIRK <address>:`.
7. Names agree with the rest of `src/` and the whitepaper (the same thing has one name).

A wrong name or comment is a must-fix finding. A vague or weakly backed one is a should.

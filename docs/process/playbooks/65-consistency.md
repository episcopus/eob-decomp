# 65 Consistency (Phase 6)

**Inputs:** a tree where `make check` passes (every function L2, the rebuilds as playbook 50 leaves
them); `docs/process/READABILITY.md`; `re/renames.toml` and `re/fields.toml` (every rename so far
with its evidence). Adapted from poolrad-decomp's playbook 65 (Turbo Pascal) to C.

**Goal:** the same thing has one name everywhere, every symbol has a real name where the evidence
allows one, and a shared value is defined once (SPEC P6, outcome O4). Names and comments never
change the code, so the tools that already exist check this phase: `make match` (the units you
touched), `make readability`, `make check` (the match gate and the rebuild tests).

## What phase 6 covers

| Kind | Rule | Tool |
|---|---|---|
| Function | no function keeps a placeholder (`f0D1C_0670`, `Ovl4_0D1B`, `Obj0150`, `Action0034`) | `make rename OLD= NEW= WHY=` |
| Global | every address-built name (`gA9C0`, `gTable8DD2`, `gFont1A47`) the code reads or writes has a real name; one that is written and never read gets a name that says so | `make rename` |
| Struct field | one record has one vocabulary: an offset of `struct CharRec` has one name in all 16 files that declare it, and no field is named by its offset (`m1B`) once its use is known | `make field STRUCT= AT= NEW= WHY=` |
| Struct type | the placeholder types (`Entry14`, `Obj28`, `Unk8894`, `T16`) get the record's name; two names for one record (`Entry14` and `Item14` at ds:8DD2) become one | by hand in each file, then `make match` |
| Alias | one object has one declaration: `gItems` (ds:8DD0) and `gTable8DD2` (ds:8DD2) are one array seen from two bases | by hand, then `make match` (same machine code, the matcher proves it) |
| Constant | a magic number with a meaning (a status bit, a key, a mask) is a `#define` or `enum`; a constant several units use is defined the same way, with one name, in each (a shared header is a decision, below) | by hand, then `make match` |

## Where to start

`make names` lists, by file, the placeholder functions, globals and struct fields left and the
`Purpose unknown` comments. `make names SHARED=1` lists what to name first: the placeholders the
most files declare (the game's vocabulary: `MemberIs`, declared by 12 files, was `f0D1C_0670`) and
the struct offsets two files name differently. `make names UNIT=<id>` is one file's list.

## Procedure

1. Pick from `make names SHARED=1`. Read the definition, its comment and every use (the callers'
   context): the name says what the value or function is for in the game, the evidence (an address,
   a string, a caller, a data file) goes in `WHY`. Name what is known, not more (READABILITY.md).
2. `make rename` (functions, globals) or `make field` (struct fields). Then edit the comments the
   rename made stale: a `Purpose unknown:` that the evidence now answers becomes the function's
   description, and prose that still says the old name follows (`make field` renames the accesses
   written in comments; it cannot rewrite free prose like "the record's maxhp").
3. `make match FILE=src/<X>` for every file the tool reports changing. A wrong field access does not
   compile, so a match proves the field rename too.
4. Overlay functions: a rename changes Turbo C's public order and so TLINK's stub numbering. The
   rebuild tests check the kind of every difference (stub entries, stub table, link date), not
   their count, so they still pass; `make rebuild PROGRAM=EOB.EXE` shows the new counts.
5. A conflict evidence cannot settle (two units read one byte two ways) goes in the findings below,
   not into a guess.
6. `make check`, commit one kind of change at a time (`p6: <what> [L2]`).

## Pitfalls

- **Never `git checkout` a file to undo a field rename** on a branch with uncommitted renames: it
  reverts those too. Recover with `make rename APPLY=1`, which applies `re/renames.toml` again
  (field renames are not replayed: commit them before the next experiment).
- **A name is only as good as the evidence of its first use** (poolrad's lesson): scan for names two
  units read differently, comments a later rename made false, and constants copied into a unit after
  a shared definition existed.
- On a unit branch both tools change only the unit's own file; `make integrate` replays
  `re/renames.toml` on other files, but not yet `re/fields.toml`.

## Decision for the human: a shared header

Every C file declares its own copy of the structs, externs and constants it uses: `struct CharRec`
is in 16 files, `struct Obj28` in 11. One header (`src/EOB.H`) with the shared structs and constants
would give one vocabulary by construction, as poolrad's shared block in `POOLVARS.PAS` did. It needs
the matcher and the rebuild to copy the header next to the unit they compile (both write one file
today, `tools/match/cmatch.py` and `tools/rebuild/rebuild.py`), so it is a change of the matcher:
asked before it is made (AGENTS.md). Until then `make field` keeps the copies in step.

## Done when

`make check` passes; `make names` reports no placeholder function, no placeholder global the code
reads and no struct offset named two ways; every remaining `Purpose unknown:` says what is missing;
the findings below are empty or each has an owner; the reviewer signs each subsystem off
(`make review-pass`).

## Done so far

- 2026-11-03, the tools and a pilot: `make names`, `make field`; the rebuild tests check differences
  by kind. Renamed: `LoadFont` (1A47:0005), `SetFont` (1A47:0169), `TextWidth` (1A47:018E), `gCurFont` (ds:FE4E),
  `MemberIs` (0D1C:0670), `DrawShape` (1A29:000F), `gDrawShape` (ds:80C2); fields `CharRec.status`
  (+01), `CharRec.hp` (+1B), `CharRec.maxHp` (+1C).
- 2026-11-03, the shared vocabulary, first pass:
  - every named offset of `struct CharRec` has one name in all 16 files (abilities, `armorClass`,
    `race`, `classIndex`, `alignment`, `food`, `levels`, `experience`, `portrait`, `spellBook`,
    `items`, `timers`, `effectFlags`, `lastDamage`, `lastAction`);
  - the maze: `gMaze` (ds:B894, nine bytes per cell: four walls, the monster count, the item list,
    the script), `gMazeItems`, `gWallTypeFlags`, `gDirCellOffset`, `gOppositeSide`, `StepCell`,
    `StepIfOpen`, `RunCellScript`, `gScriptCell`;
  - the party: `gPartyDir`, `MoveParty`, `gStepCount`, the movement keys `MoveForward`,
    `MoveBackward`, `MoveLeft`, `MoveRight`, `TurnLeft`, `TurnRight`;
  - the panel: `gPanelMode`, `gInventoryMember`; the 6x6 font `gFont6x6`.

## Findings

- Settled 2026-11-03: gPartyCell is the party's map cell. E0ACD and E122E had read it as a sound
  volume (`SetSoundVolume`), from the movement keys' table walk; 122E:0001 moves the party and runs
  the cells' scripts, and the "volume message" is a carriage return.
- `gEFB2` is the floor position (0..3) PickListItem 0ACD:0BA6 last took an item from; turning and
  moving reset it to 0xFF. Its old comment called it the sound page.
- `gF51A`: E0D1C reads it as an index into the inventory's 27 slots, E0ACD as an action-menu row
  0..2 with 0x10 for the own list. Not settled.
- `rename.replace_word` treats an apostrophe in a comment as the start of a character literal, so
  a rename can miss a name between two apostrophes ("the character's ... LevelUpSlot's"); `make
  names` and a grep for the old name catch it.
- `struct Region` +08 to +0E: four files call the words `attr1`, `attr2`, `column`, `row`, three call
  them `saved0` to `saved3`; which is right has not been read yet.
- ds:8DD2 is declared as `struct Entry14` (10 files) and `struct Item14` (1), and ds:8DD0 as
  `struct ItemRecord gItems[]`: one item array seen from two bases (an alias to merge).

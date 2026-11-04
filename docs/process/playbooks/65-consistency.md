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

## The shared header (decided 2026-11-04)

Every C file declares its own copy of the structs, externs and constants it uses: `struct CharRec`
is in 16 files, `struct Monster` in 11. The human decided (2026-11-04) on one shared header with
**the structs, enums and constants only**, for all three programs; each file keeps its own extern
declarations with their address tags (`/*@ ds:FBC8 */`), since INTRO.EXE's addresses differ from
EOB.EXE's. It needs the matcher and the rebuild to copy the header next to the unit they compile
(both write one file today, `tools/match/cmatch.py` and `tools/rebuild/rebuild.py`): that matcher
change is approved. A struct that is a variant in one file (`struct Node` +34) keeps one
declaration with a union. Until the header lands, `make field` keeps the copies in step.

## The review sign-off (decided 2026-11-04)

Independent reviewer agents, one per subsystem (`work/p6-review.md`: the subsystems, their units,
and the prompt to start one). Each unit is a later round of playbook 70: the reviewer judges what
changed since the commit its last round reviewed, which in Phase 6 is the names and comments,
against the evidence each rename logged (`re/renames.toml`, `re/fields.toml`). The operator fixes
the must-fix items; the subsystem is signed off when every unit in it has passed again.

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

- 2026-11-03, every placeholder named (608 functions and globals): nine read-only research agents
  each read one group of units and proposed a name with its evidence for every placeholder; the
  operator applied them one group at a time through `make rename` (which logs each WHY), matched
  every touched unit and committed per group. The evidence came from the game's own tables and
  strings read from the image: the spell table ds:1344 (each spell's apply, projectile and remove
  callbacks, so `Cast<Spell>`, `End<Spell>`, `Hit<Spell>`), the screen-node tables of ds:5102
  (each action handler's box on screen), the graphics driver's export table at ds:8046 (filled by
  MCGA.OVL, so the driver pointers and their front ends), character creation's name and limit
  tables, and the INTRO.EXE twins. The struct types are named (`Monster`, `MonsterType`,
  `Projectile`, `ArgVector`), the item view at ds:8DD2 is merged into `gItems`, and the timer
  views of EOV3 are `gEffects` fields. `make names`: no placeholder function or global is left.
- Names a closer reading showed wrong were corrected, among them `gDrawPage` (ds:8115, was
  "status line"), `gCurLevel` (ds:FBC8, was "the current action"), the spell callbacks that were
  named for their mechanics (`EnterSwapMode` is `CastDetectMagic`), `PickPortrait`, `ShowFileError`
  and `ShowMessageBox` (two functions had shared `ShowMessage`).

- 2026-11-03, the names that were true to the mechanics but not to the game, second pass (three
  read-only agents, each checking one area against the settled vocabulary): the monsters
  (`UpdateMonsters`, `DamageMonster`, `FlashMonsterHit`, the Monster fields), the doors and wall
  handlers of E122E, which had been read as party-member logic (`OpenDoor`, `CloseDoor`,
  `ToggleDoor`, `StepDoor`, the wall handlers and script opcodes), the AD&D rule lookups
  (`GetSavingThrowTarget`, `GetTurnUndeadTarget`), the level loader (`LoadLevel`, `LoadMaze`,
  `LoadMonsterShapesA` to `C`), and about 250 comments. A sweep replaced the 128 old names comments
  still used. `make readability` has no finding and its baseline is empty.

## Findings

- ds:00EF to 00F5 are gRegions[5] (the maze view window) and ds:0139, 0159, 0169, 0179 the
  textAttr2 of gRegions[9], [11], [12], [13] (the fill colour SetTextWindow loads): the files now
  write them as gRegions fields (the same code). gPanels (ds:017F) starts where gRegions[14] would,
  so the region table has 14 entries.
- Open (the human, 2026-11-04: fix these three from the code, guesses marked [H]): gIconTable (ds:FB34) is declared as a byte (the member being moved) in several units and as
  an array of far pointers in E0EF3 (CastBurningHands reads entries 1 to 3, from ds:FB38); one of the
  two readings is a different object that shares the address. gSpellsTakenByTab (ds:FDE7) has 5
  bytes but BuildSpellSlots clears 10, into ds:FDEC, which it clears again (a QUIRK). ds:DCAA
  (gMenuPhase) has no store of 5 and its stepper 0ACD:0753 no caller found: the "rest phase"
  reading is unsupported.
- Open, for the oracle (the human, 2026-11-04: trace each one; results go in the whitepaper as QUIRKs with evidence): gLevelFlags (ds:FBD4) is indexed by gCurLevel 1 to 12, so
  level 12's flags are the bytes of gGameFlags (ds:FC04), a QUIRK if the game relies on it; the
  door code (122E:0137, ToggleDoor) tests only bit 0 of a cell's monster count; the 32-byte maps
  named gCgaMap* hold only the values 0 to 3 and are only passed as a colour map (the CGA reading
  is [medium], the driver is not in src/); Monster +0C maxHp is inferred (InitMonster writes the
  same roll to +0C and +0E, nothing in src/ reads +0C); gPosSearchOrder searches the rear positions
  first for a west-facing monster only; ds:E06B (gSlotRegions) is gWallRecs + 0x2B, which view
  position that shape is was not read; the label kModifierState (1AAD:0180) is a copy of the
  keyboard option word.
- ds:DCA4 and ds:DD10 are one variable with two uses each: overlay 0 keeps the new character's hit
  point limits there (the seventh words after two six-word tables), the game keeps the attacked
  monster and a hit flag; named for both (`gHpMaxOrAttackTarget`, `gHpMinOrHitFlag`).
- `struct Node` +34 is a variant record: tile nodes keep two tile ids there (E0CF7, E16A5), the
  button nodes of overlay 1 a far pointer to their definition (`def`, +34 to +37). Both readings are
  right; `make names SHARED=1` lists it, and it is the one clash left on purpose.
- An overlay function's resident stub takes the function's name (2116:005C `DrawViewFrame`):
  `make rename` refuses a name another address has, so a stub is renamed with
  `rename.rename_address` and logged by hand (see the commits of 2026-11-03).
- QUIRKs found on the way, for a port to keep: the four-byte table gAt5717 is read for members 4 and
  5 past its end; CreateItemRecord (0EF3:0418) clears item 0's cellPos by a fixed address; the
  door clip table ds:6F6D has 12 bytes but is indexed by level 1 to 12; ds:DD12 is read and never
  written; in overlay 0, the byte 255 in ds:6101 indexes ds:60D5 at -1.

- Settled 2026-11-03: gPartyCell is the party's map cell. E0ACD and E122E had read it as a sound
  volume (`SetSoundVolume`), from the movement keys' table walk; 122E:0001 moves the party and runs
  the cells' scripts, and the "volume message" is a carriage return.
- `gEFB2` is the floor position (0..3) PickListItem 0ACD:0BA6 last took an item from; turning and
  moving reset it to 0xFF. Its old comment called it the sound page.
- `gCursorSlot` (ds:F51A, was gF51A) is the keyboard cursor's item slot 0..26 (0 and 1 the hands);
  StepActionRow (0ACD:03CD) wraps it to 0..2, the one reader that disagrees.
- `rename.replace_word` treats an apostrophe in a comment as the start of a character literal, so
  a rename can miss a name between two apostrophes ("the character's ... LevelUpSlot's"); `make
  names` and a grep for the old name catch it.
- Settled: `struct Region` +08 to +0E are the window's saved `textAttr1`, `textAttr2`, `textCol`,
  `textRow`; the view at ds:8DD2 is merged into `gItems`.

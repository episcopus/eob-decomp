# Review: eov3

# Round P6 re-review (2026-11-04), commit 24711ec

Reviewer: p6-rev-r2
Commit reviewed: 24711ec (P6 re-review; the P6 round read ce0608f)

Judged: the open P6 must-fix (the LoadGame header field name) and the diff since ce0608f for src/EOV3.C. `make match FILE=src/EOV3.C`: 23 of 23 (6961 bytes), exit 0. `make readability UNIT=eov3`: 0 findings.

- [x] must-fix src/EOV3.C:827 and :829 (LoadGame): fixed right. The header now says `status` nonzero at both places, matching `rec->status` at :892 and :904. `identifiedNameIndex` is gone. The `flag` at :953 sentence is unchanged and still matches the code.
- [ ] note The older P6 and earlier must-fix items in this file are unchanged and remain ticked. No new false claim in the changed lines.

# Round P6 (2026-11-04), commit ce0608f367ed01b1b1df84e254eef04d1a734b27

Reviewer: p6-rev-8
Commit reviewed: ce0608f367ed01b1b1df84e254eef04d1a734b27 (main tip when claimed for the P6 round)

Playbook 70. Judge what a matching file can still get wrong: names, types, comments, structure.
The matcher already proved the bytes; do not re-prove them, read for meaning.

## Findings

Each finding is a checkbox. `must-fix` blocks `make review-pass` until a decompiler ticks it off
(with the commit that fixes it); `should` and `note` do not block.

### Round P6 (2026-11-04), commit ce0608f367ed01b1b1df84e254eef04d1a734b27

Judged: the changes since 6118e25 in src/EOV3.C and re/symbols.toml. The diff is the Phase 6 rename of the unit's functions, globals and struct fields, with the comments rewritten to the maze, door and level vocabulary. I checked the evidence for the new names in re/renames.toml and re/fields.toml and the code they describe. The rename log backs the new names: gDecorShapes (ds:EFD4), gWallRecCount (ds:F538), gCopyToPlanar (ds:808E), gWallPlaneTableA (ds:F4D6), gWallPlaneTableB (ds:FB4D), gCgaMapInvent (2067:0000), gCgaMapPlayfld (2069:0000), gTunesOn (ds:80E8), gBarGraphsOn (ds:F43C), gSpellItemTypes (ds:AD20), gWallBlocks (ds:DCA6), gGetSpriteCellRle (ds:8082), gFont6x6 (ds:A9C0), gGameLoadState (ds:A998), gViewToCellPos (ds:1A1B), gPartyDir (ds:FBCC), gMouseItem (ds:FBCE), gStagingPage (ds:DF4E), gDecorationRects (ds:E062), gMaze (ds:B894), gSavedLevels (ds:FBD0), gScratchBuf (ds:DCFC), gWallVmpSet (ds:1879), gMonsters (ds:8A82), gGameTicks (ds:8492), gWallTypeFlags (ds:194B), gWallDecorHead (ds:18BF), gWallHandlerIndex (ds:1905), gDoorEffectCell (ds:5C74), gDoorMoveDir (ds:5C76), gDoorEffectActive (ds:5C77), gProjectiles (ds:DD2C), gItems (ds:8DD0), gDoorShapes (ds:FB5E), gDoorButtons (ds:A99C), gLoadedMonsterShapes (ds:B802), gMonsterRectsA (ds:6930), gMonsterRectsB (ds:69C0), gMonsterRectsC (ds:6A50), gDrawPage (ds:8115), gCgaMapDefault (ds:113F), gCurLevel (ds:FBC8), gMazeItems (ds:B899), gDoorRects (ds:6840), gDoorButtonRects (ds:68A0), gDoorButtonPos (ds:6900), gLevelCgaMapIndex (1F1D:0000), gTimerEnableFlags (ds:849E), gLevelTimerMode (ds:F51B), gLevelTimerPeriod (ds:F510), gLevelTimerSteps (ds:FB32), gStepCount (ds:F540), gLevelPalettePending (ds:FB45), gLevelInf (ds:8DCA), gWallBlockMaps (ds:F43E), gPaletteBuf (ds:FE68), gEffects (ds:5BA8); and the functions DefineWallType (ovl3:01C8), LoadDoorShapes (ovl3:03A2), FreeMonsterShapes (ovl3:05D2), LoadMonsterShapesA (ovl3:0624), LoadMonsterShapesB (ovl3:069E), LoadMonsterShapesC (ovl3:0718), LoadMaze (ovl3:09AA), LoadLevel (ovl3:0A7C), InitMonster (08E6:131D), MemberIs (0D1C:0670), SetFont (1A47:0169), NormalizePtr (1BA8:0072); and the CharRec fields status (01), portraitIndex (22), portrait (33), timers (AD), ItemRecord.level (0C), key (06), cellPos (05), Monster.cell (02), attackFrame (0A), Projectile.item (08). The comments match the code at those addresses except the must-fix below. `make match FILE=src/EOV3.C` 23 of 23 (6961 of 6961 bytes); `make readability UNIT=eov3` 0 findings.

#### Round P6 must-fix

- [x] must-fix src/EOV3.C:827 and :829 (LoadGame header, changed in this diff): the header says the sprite cut runs "for each character whose identifiedNameIndex is nonzero" and "a character with identifiedNameIndex nonzero ... sets `flag`". The code tests `rec->status != 0` at :892 and :904, not `identifiedNameIndex`. `status` is the byte at +01; `identifiedNameIndex` is a different field. Say `status`. Evidence: src/EOV3.C:892 and :904. Fixed in def28b2.

#### Round P6 should

- [ ] should src/EOV3.C:932 (ResetLevel header, changed in this diff): "keeping only the low two bits of the item record its item indexes" is unclear. The code at :941 keeps the low two bits of `gItems[gProjectiles[i].item].cellPos`; say "the item's cellPos". Evidence: src/EOV3.C:941.

#### Round P6 notes

- note src/EOV3.C:32: the gSpellItemTypes declaration still says "a 96-byte saved-game buffer (Purpose unknown)"; the rename why says it is the six 16-byte item-type records for spells. The comment was not changed by the diff, so this is for the next reader. Evidence: re/renames.toml ds:AD20.
- note src/EOV3.C:499: the LoadLevel header says "the 30 14-byte object records"; the Monster record is 28 bytes. The line was changed only to rename InitMonster, so the size error is pre-existing. Evidence: the old Obj28 is 28 bytes (src/EOV3.C:107 in the diff).
- note src/EOV3.C:91: the gWallHandlerIndex comment "a 256-byte table gMaze indexes into" could say per wall type, the handler index. Evidence: re/renames.toml ds:1905.
- note src/EOV3.C:130: the gMazeItems comment "the nine-byte record list head" could say each cell's item list head. Evidence: re/renames.toml ds:B899.

Round 4 (at 6118e25, rev-haiku, a later round: round 3 must-fix items and the diff since a2c0a9e only).
The diff since a2c0a9e is 8db5863 and the merges after it. src/EOV3.C changes are comments and declaration types
only (the type changes keep the bytes: `make match` is 23 of 23). re/symbols.toml changes are other units' review
stamps and renames, with no eov3 entry changed. re/globals.toml adds the gTable1879 "why" text, which cites
src/EOV3.C:60 (that declaration, correct). `make match FILE=src/EOV3.C`: cmatch 23 of 23 functions match.
`make readability UNIT=eov3`: 0 findings.
Round 3 must-fix: both fixed right, ticks stay. The SaveGame header now cites `if (ok)` at :854, the INVENT
reload at :871 and the `ok &=` writes at :857-869, and the LoadGame header says the m22 < 0 character sets `flag`
at :953 (after the `else` at :952); checked against the code. The round 3 shoulds are fixed right too: the
OUTTAKE sentence says "cuts", and g1A1B is declared per-page with src/E0ACD.C:131 (that line is the g1A1B
declaration with the per-page comment). The round 3 cross-unit should is still open (below).

#### Round 4 must-fix

None open.

#### Round 4 should (changed lines since a2c0a9e; none blocks)

- [ ] should src/EOV3.C:143 (gFB5E declaration, changed in 8db5863): "the per-column size records" cites
  src/E122E.C:106, which is the 3-byte SlotSize layout, not a column index. The column evidence is the use in
  src/EOV4.C:790 (`gFB5E[col]`, col from the heading) and the EOV4 declaration at :766 ("per column"). This
  file also says "per-slot" for the same table (the FreeSlotTables header at :345, and re/renames.toml:2206).
  Cite the column use (by address of the EOV4 draw code, not by line) and use one word here and in the header.
- [ ] should src/EOV3.C:15 (gRectKinds declaration, changed in 8db5863): "the per-kind size records
  (src/E122E.C:91)". Line 91 is the `pad00` field of struct RectKind; its header is E122E.C:90. In this unit the
  array is indexed by a wall `cell` (RegisterWallObject :333-336, FreePtrTable :246-248), in E122E.C:79 by `kind`,
  and EOV4.C:670 calls its entries per-cell. Say which index this unit uses and cite the kind reading by
  address (the ds:EFD4 declaration is E122E.C:97). Not a must-fix: the per-kind claim is E122E's own.
- [ ] should src/EOV3.C:56 and :63: gE062 is "the 8-byte map cell rectangles" in the declaration and "the 8-byte
  sprite rectangles" in the struct Cell8 comment. The code (:336) passes its four words to gGetSpriteCell with
  MAPSHEET, so "sprite rectangles in the map sheet" is the evidence. Use one phrase in both places.
- [ ] should, cross-unit (not this file, carried from round 3): src/EOV5.C:207 declares FreeEffectTables, :1053
  and :1174 call it, and :1158 names it in a comment. The round 3 lines (:210, :1042, :1162) have moved. The EOV5
  owner should use FreeSlotTables; the match is unaffected.

#### Round 4 note

- The round 1 and 2 shoulds that are still open (the scriptWord split, decided in 8db5863 with the evidence that
  two locals make LoadMapScreen drop to 22 of 23) and the round 2 and 3 notes are not re-judged. They do not block.

Summary of round 4: no must-fix open. Round 3's two must-fix cites are fixed right and ticked; the changed lines
are sound in what they claim, with three wording and cite shoulds (gFB5E, gRectKinds, gE062) and one cross-unit
should. The unit passes.

Round 3 (at a2c0a9e, rev-haiku, a later round: round 2 must-fix items and the diff since cc4ce8b only).
The diff since cc4ce8b is 89e940a, comments in src/EOV3.C only; no code byte, no re/symbols.toml entry for
this unit. The re/globals.toml changes touch gPartyCell (the key the LoadGame header names), not this text.
`make match FILE=src/EOV3.C`: 23 of 23, exit 0. `make readability UNIT=eov3`: 0 findings.
Round 2 must-fix: all four are fixed right in substance (the INVENT reload runs after the writes whether or
not they succeeded; the cut needs m22 >= 0; srcB slices three cells at dstB; gWallRecs is not cleared and its
records stay until RegisterWallObject overwrites from slot 0). Their ticks stay. The round 2 header edit added
a line to the SaveGame comment, so the line cites in the new text are one or two lines off; those are the
two must-fix items below. Round 2 should items: the :560/:774 cites and the per-cell sound wording are fixed.

#### Round 3 must-fix

- [x] must-fix src/EOV3.C:833-835 (SaveGame header, changed in 89e940a): the three line cites are off by one.
  `ok = 1` is at :853 and `if (ok)` at :854 (not :853); the INVENT reload is the call at :871 (not :870, which
  is FileClose); the `ok &=` writes are at :857-869 (not :856-868). Cite `if (ok)` at :854, the reload at :871
  and the writes at :857-869. The claim itself is right.
  Fixed in 8db5863: the header now cites `if (ok)` at :854, the INVENT reload at :871 and the `ok &=` writes
  at :857-869 (checked against src/EOV3.C:854, :871, :857).
- [x] must-fix src/EOV3.C:885 (LoadGame header, changed in 89e940a): "sets `flag` at :951 instead" is wrong.
  `flag = 1` is at :953 (the `else` at :952). Line :951 is the `farfree(cell)` of the first pass. Say :953.
  The condition ("m01 nonzero and m22 < 0") is right.
  Fixed in 8db5863: the header now says the m22 < 0 character sets `flag` at :953 (src/EOV3.C:953, after the
  `else` at :952).

#### Round 3 should

- [x] should src/EOV3.C:886-887 (LoadGame header; the sentence is unchanged since round 1): "loads the OUTTAKE
  screen and draws the m22 < 0 characters' sprites into it". The OUTTAKE pass (:958-965) cuts each m22 < 0
  character's 4 by 32 cell out of gOverlayBuf (the OUTTAKE screen loaded at :956), unpacks it into rec->sprite
  and frees it; no draw call is in that loop. Say "cuts", as the first pass does. This wording is my error:
  round 1 asked for "draws", and the fix wrote it.
  Fixed in 8db5863: the header now says the OUTTAKE path cuts the m22 < 0 characters' 4 by 32 cells out of it
  the same way (src/EOV3.C:958-965 cuts, unpacks and frees; no draw).
- [x] should src/EOV3.C:45 (declaration of g1A1B, "per-cell sound table (src/E0ACD.C)"): the table is indexed
  `gFBCC * 4 + roll` (:977), so it is per page, not per cell. src/E0ACD.C:131 and src/E0DE8.C:121 both declare
  it as the per-page sound table. Say per-page and cite src/E0ACD.C:131.
  Fixed in 8db5863: the declaration now reads "per-page sound table (src/E0ACD.C:131)".
- [ ] should, cross-unit (not this file, carried from round 2): src/EOV5.C:210, :1042 and :1162 still call
  FreeEffectTables, and :1147 names it in a comment. The EOV5 owner or the integrator should use FreeSlotTables.
  Not blocking, and the match is unaffected.

#### Round 3 note

- The round 1 shoulds still open (scriptWord split at LoadMapScreen; the type shoulds for Triple6, gFB5E,
  gRectKinds and gE062) and the round 2 note are not re-judged. They do not block.
- Once the two must-fix cites are fixed and ticked, the unit passes: the should items do not block.

Round 2 (at cc4ce8b, rev-haiku, a later round: round 1 must-fix items and the diff since 01237b0 only).
`make match FILE=src/EOV3.C`: cmatch 23 of 23 functions match. `make readability UNIT=eov3`: 0 findings.
Round 1 must-fix items: all fixed right in their text, except the two round 1 should items reopened
below (unticked), whose new wording is wrong. Four must-fix items in total; the rest are shoulds.

#### Round 2 must-fix

- [x] must-fix src/EOV3.C:826-834 (SaveGame header; reopens the round 1 should on the INVENT reload, now
  unticked): "only when every write succeeded does it reload the INVENT screen" is false. `ok = 1` (:852)
  is followed by `if (ok)` (:853), which is always true, so `LoadScreenImage("INVENT", ...)` (:870) runs
  after the writes whether or not they succeeded; the `ok &=` writes (:856-868) only set the return value.
  Say that the reload runs after the writes unconditionally and that the return value reports them. The
  source matches the original (`make match` 23 of 23), and it has no test of the write results before :870.
  Fixed in 89e940a: the header now says the `if (ok)` at :853 follows `ok = 1`, so the INVENT reload at
  :870 runs after the writes whether or not they succeeded, and the `ok &=` writes only compute the
  return value.
- [x] must-fix src/EOV3.C:877-890 (LoadGame header; reopens the round 1 should on "alive" and "occupied",
  now unticked): "for each occupied character (m01 nonzero), cuts its 4 by 32 cell out of gOverlayBuf".
  The cut (:946-949) runs only when `rec->m01 != 0 && rec->m22 >= 0`. An occupied character with m22 < 0
  is not cut there (it sets `flag` at :951), and it is cut in the OUTTAKE pass (:958-961). Say "with m22 >= 0".
  Fixed in 89e940a: the header now says a character with m01 nonzero and m22 >= 0 is cut, and that one with
  m01 nonzero and m22 < 0 sets `flag` at :951 instead.
- [x] must-fix src/EOV3.C:369-370 (LoadMapCells header, from cd7af2f): "it slices the two cells at `dstB`
  the same way". The srcB loop (:391) is `for (i = 0; i < 3; i++)` and fills gFB5E[dstB+i] and
  gA99C[dstB+i] for three cells, as srcA does. Say three.
  Fixed in 89e940a: the header now says the srcB branch slices the three cells at `dstB`.
- [x] must-fix src/EOV3.C:237-238 (FreePtrTable header, from 0bd5b44): "resets the wall-record slot count
  ds:F538, which also empties gWallRecs". The body (:250) sets `gF538 = 0` and nothing else; gWallRecs is
  not cleared. Its only readers (RegisterWallObject, :326-340) index it by gF538, so the old records stay
  in memory until the next RegisterWallObject overwrites them from slot 0. Say that, not "empties".
  Fixed in 89e940a: the header now says gWallRecs itself is not cleared and that its records stay in
  memory because it is only read through gF538, until the next RegisterWallObject overwrites from slot 0.

#### Round 2 should

- [x] should src/EOV3.C:560 (LoadMapScreen header, from cd7af2f) and :774 (SaveLevelState header): the
  line cites into src/E122E.C are off. :1287 is `SaveLevelState(gFBC8)` (E122E.C:1287) and the LoadMapScreen
  call is at :1288; :1268 is the LoadLevelOp definition line. The address 122E:1AAB and the stubs (210C:007A,
  210C:002F) are right; cite the address and drop the line.
  Fixed in 89e940a: both headers now cite LoadLevelOp (122E:1AAB, through the 210C:007A / 210C:002F stub)
  and the src/E122E.C line numbers are dropped.
- [ ] should, cross-unit (not this file): src/EOV5.C:210, :1042, :1147 and :1162 still declare and call
  `FreeEffectTables`. The round 1 fix said `make rename` renamed it "everywhere"; re/renames.toml:2183 has
  the entry, but the EOV5 references were not changed. The EOV5 owner (or the integrator, with
  `make rename`) should use FreeSlotTables. The C name is not in the bytes, so the match is unaffected.
- [x] should src/EOV3.C:886-889 (LoadGame header): "inserts it with a RollDice-chosen per-cell sound". The
  code (:974-976) takes the sound from `g1A1B[gFBCC * 4 + RollDice(1, 2, -1)]` and passes gPartyCell as the
  key. Nothing here shows "per-cell" as the sound's meaning; say "a sound from the g1A1B table, picked by
  RollDice(1, 2, -1)" or give the evidence.
  Fixed in 89e940a: the header now says the item is inserted with a sound from the g1A1B table picked by
  RollDice(1, 2, -1), keyed by gPartyCell.

#### Round 2 note

- [ ] note: the round 1 shoulds still open (scriptWord split at LoadMapScreen; the type shoulds for
  Triple6, gFB5E, gRectKinds and gE062) and all round 1 notes are unchanged since round 1 and not re-judged.
  They do not block.

Round 1 (at 01237b0). `make match FILE=src/EOV3.C`: cmatch 23 of 23 functions match, exit 0.
`make readability UNIT=eov3`: 0 findings. The findings below are about meaning.

### Must-fix

- [x] must-fix src/EOV3.C:69-73, 291-299, 313-316, 325: the WallRec layout and the RegisterWallObject
  header misdescribe the chain. The copy is `movmem(..., gWallRecs[gF538].cells, 0x34)` (:313-314), so
  the 52-byte decoration lands at +0x2a of its slot, not at the slot base. Its first ten bytes are the
  cell indices (:316), and its byte 10 lands at +0 of the next slot. The link read at :325
  (`gWallRecs[gF538].next` after `gF538++` at :324) is therefore decoration byte 10 of the record just
  copied, not a stored "next record index". The header ("copies the decoration ... into
  gWallRecs[gF538]") and the WallRec comment ("+0: the next record index") are wrong. Say the copy is
  52 bytes at +0x2a and the link is read from the next slot's +0, or redeclare WallRec to match.
  Fixed in 0bd5b44: the WallRec comment now says the 0x34-byte decoration is copied into cells (+0x2a)
  and its byte 0x0a lands at +0 of the following slot; the RegisterWallObject header now says the copy
  is 52 bytes at +0x2a of the slot and the link is that decoration byte read from gWallRecs[gF538].next
  after the bump.
- [x] must-fix src/EOV3.C:452-459, 464-467, 484: the LoadWallTextures header says "or an empty name when
  `name` is NULL" and "It leaves gFileNameBuf empty". When name is NULL the code copies the kept name
  gWallNameBuf into gFileNameBuf (:466-467), and at the end copies gFileNameBuf into gWallNameBuf (:484),
  so gFileNameBuf still holds the file name. Evidence: :35 (gWallNameBuf is "kept between
  LoadWallTextures calls").
  Fixed in 0bd5b44: the header now says a NULL `name` reuses the name kept in gWallNameBuf and that at
  the end the name just used is kept by copying gFileNameBuf into gWallNameBuf.
- [x] must-fix src/EOV3.C:460-471 (header 452-459): "unpacks three 16-byte plane tables ... into ds:F4D6,
  ds:FB4D and gSegDCA6". Only the first two are 16-byte tables; the third (:471) is the 0xCB20-byte
  texture block into gSegDCA6. The header also says "through ovl3:0FD1's counterpart", but the unpack
  is Unpack0 (1BA8:0006, :469-471) and ovl3:0FD1 is PackLevelState (:692). Name the 1BA8:0006 unpack and
  the 0xCB20 block.
  Fixed in 0bd5b44: the header now names Unpack0 (1BA8:0006) and says the third block is the
  0xCB20-byte texture block into gSegDCA6; the two 16-byte plane tables are the first two.
- [x] must-fix src/EOV3.C:752: "Called by overlay 0 (122E:1AAB)". Address 122E is the resident EOB.EXE
  unit src/E122E.C ("Unit at EOB.EXE 122E:0001", line 1), not overlay 0; overlay 0 is src/EOV0.C with
  ovl0 addresses. The caller is LoadLevelOp (122E:1AAB, src/E122E.C:1268), which calls SaveLevelState
  through the 210C:002F stub (E122E.C:66, call at :1287). Name it as LoadLevelOp.
  Fixed in 0bd5b44: the header now cites LoadLevelOp (122E:1AAB, src/E122E.C:1268, through the
  210C:002F stub).
- [x] must-fix src/EOV3.C:804-810 (SaveGame header): "It first writes the current level state to
  LEVELS.TMP slot gFBC8-1 through ovl3:0FD1". This function does not write LEVELS.TMP. It reads it into
  gSegEFC2 (:820-822), packs into the in-memory image at gSegEFC2 + slot*0x7F8 (:823-824), and writes the
  0x5FA0-byte image to EOBDATA.SAV (:838-839). Correct the sentence.
  Fixed in 0bd5b44: the header now says it runs ResetLevel, reads the whole LEVELS.TMP image into
  gSegEFC2, packs the level state into the in-memory image at gSegEFC2 + (gFBC8-1)*0x7F8, and writes
  the 0x5FA0-byte image to EOBDATA.SAV.
- [x] must-fix src/EOV3.C:851-862, 900-904 (LoadGame header, line 855): "it writes the level-state block
  to LEVELS.TMP slot gFBC8". FileOpen mode 2 deletes and recreates the file (1C0E:00DB, src/E1C0E.C
  `case 2`: DeleteFile, ResetFile), and no SeekFile precedes FileWrite (:902-903). The write starts at
  offset 0 and carries the whole gBlockSizes[3] image (the 0x5FA0 bytes SaveGame wrote at :838-839), not
  one slot. Say the whole LEVELS.TMP image is recreated from block 3.
  Fixed in 0bd5b44: the header now says mode 2 deletes and recreates LEVELS.TMP and the FileWrite at
  offset 0 carries the whole gBlockSizes[3] image.
- [x] must-fix src/EOV3.C:858-859, 916-924, 931 (LoadGame header, "the front row into the OUTTAKE screen
  when any character sits there"): the flag is set by the `else` at :922-923, which also runs for an
  empty slot (`m01 == 0`; the CharRec comment at :116 says 0 when the slot is empty). So OUTTAKE loads
  whenever any slot is empty or any occupied character has m22 < 0. The header states a condition the
  code does not have. Say what the flag tests, and that the OUTTAKE pass draws the m22 < 0 characters
  (:931).
  Fixed in 0bd5b44: the header now says the flag trips when any slot is empty or any occupied character
  has m22 < 0, and that only the m22 < 0 pass draws from OUTTAKE.
- [x] must-fix src/EOV3.C:861-862, 944-949 (LoadGame header): "either restores the mouse item or, when a
  mouse item is shown, inserts it". The test is gMouseDisabled: when it is 0, SetMouseItem(gFBCE) runs
  (:944-945); when the mouse is disabled and gFBCE is nonzero, the item is inserted with a RollDice-chosen
  sound (:946-949). Fix the condition in the sentence.
  Fixed in 0bd5b44: the header now says the item is restored when the mouse is enabled (gMouseDisabled
  is 0) and, when the mouse is disabled and gFBCE is nonzero, inserted with a RollDice-chosen sound.
- [x] must-fix src/EOV3.C:105-109 (struct Entry14, comment at :107) and :956-960, :972 (ResetLevel
  header): "its low two bits are cleared here" and "disarming the low two bits of its item's m03".
  `m03 &= 3` keeps the low two bits and clears bits 2 to 7 (:972). The record is gTable8DD2[m08], an
  Entry14 found through DDRec.m08 (:101; src/E1414.C:57), not "its item". Fix both comments.
  Fixed in 0bd5b44: the Entry14 comment now says ResetLevel keeps the low two bits and clears bits 2 to
  7; the ResetLevel header now names the ds:8DD2 record its m08 indexes and says only the low two bits
  are kept.
- [x] must-fix src/EOV3.C:189-191 (gMapTiles at 1F1D:0000) vs src/EOV5.C:165-166 (gSeg1D0 at 01D0:0000):
  the same tile table, read by the same expression `[gFBC8-1] << 5` plus the 1D8 sheet (MAPSHEET at
  EOV3.C:191; EOV5.C:1285), has two addresses in two units. One cite is wrong. Check the segment in the
  EXE (`make map`, build/re/eob.json) and make both units agree. Its type is a separate should, below.
  Fixed in 0bd5b44: EOV3.C's cite is the right one. The overlay stores the segment-table index
  01D0 (58*8) at the reference; build/re/eob.json segments[58].para is 0x1F1D, and `make match` confirms
  it: with 01D0:0000 the matcher reports "segment 01D0, original 1F1D", with 1F1D:0000 it passes. src/EOV5.C's
  gSeg1D0 (and gSeg1D8/gSeg1F0/gSeg1F8) are raw fixup indices, not paragraphs (its UseItemPanel already
  reports "segment 01F8, original 2069"); the EOV5 owner should correct them. EOV3.C keeps 1F1D:0000.
- [x] must-fix src/EOV3.C:334-336, 530 (FreeEffectTables, and "frees the effect/map/pointer tables" in
  the LoadMapScreen header; callers EOV5.C:210, :1038, :1158): the name "Effect" has no evidence in
  this unit, and the src says these tables are per-slot size records. gFB5E holds size-record pointers
  (src/EOV2.C:146, src/EOV4.C:756 "a far size record"), and gA99C holds the six-byte SlotPos records
  (src/E122E.C:107-117, "the six-byte slot-position records at ds:A99C"). Rename the function with
  make rename (it is an overlay entry, 210C:003E, used by EOV5), giving the evidence, or write
  "Purpose unknown:" with what is known.
  Fixed in 0bd5b44: `make rename OLD=ovl3:031A NEW=FreeSlotTables` renamed it everywhere with the
  evidence (the per-slot size-record table ds:FB5E and the SlotPos records at ds:A99C); the header and
  the LoadMapScreen comment now name the freed tables.

### Should

- [x] should src/EOV3.C:765-767 (AgeCharTimers header): "a smaller nonzero one is set to 1". The else
  branch (:779-780) also covers a countdown equal to g8492 (not greater, so not reduced, and nonzero), and
  sets it to 1 too. Say "a nonzero countdown not greater than g8492".
  Fixed in cd7af2f: the header now says "a nonzero countdown not greater than g8492 is set to 1".
- [x] should src/EOV3.C:228-229 (FreePtrTable): "resets the counter ds:F538". gF538 is the wall-record
  count that RegisterWallObject advances (:307-324; the declaration at :15 has no description). Say so,
  and that this reset also empties the wall records.
  Fixed in 0bd5b44: the FreePtrTable header now says it resets the wall-record slot count ds:F538 and
  the gF538 declaration carries that description.
- [x] should src/EOV3.C:352-357 (LoadMapCells): "loads the overlay screen at ds:6A98" is ambiguous. Name
  the screen string "DOOR" (:363) and say where it sits. The srcB branch reads its ds:6900 pair with srcA
  (QUIRK ovl3:0597 at :383); the header says it "does the same at dstB", which hides the quirk. Say it.
  The header also says "copies the two gA99C[dstA].c bytes" (:356-357): c[0] and c[1] are the slot's x and
  y (see the SlotPos point below).
  Fixed in cd7af2f: the header now names the "DOOR" screen, states the srcB quirk, and calls c[0]/c[1]
  the x and y bytes of gA99C[dstA].
- [x] should src/EOV3.C:405-407, 421-422, 436-438 (LoadMapIcons6, LoadMapIcons6b, LoadNamedIcons): "No
  direct caller found" is true but misleading. The three are called through the `handlers` table of
  LoadMapScreen (:562, :603, :608) and the ds:6A80 initialiser (:217-220). Say so.
  Fixed in cd7af2f: all three headers now say they are called through the `handlers` table LoadMapScreen
  builds (ds:6A80 initialiser).
- [x] should src/EOV3.C:3 (unit header): "loads and unpacks a level state from LEVELS.TMP (ovl3:09AA,
  109D, 1147)". ovl3:09AA (LoadObjects) loads an object table, and calls LoadLevelState only when the level
  was saved (:515-516). The level state is read by 1147 and unpacked by 109D. Reword.
  Fixed in cd7af2f: the unit header now says LoadObjects loads an object table and, for a saved level,
  reads its LEVELS.TMP slot (ovl3:1147) and unpacks it (ovl3:109D).
- [x] should src/EOV3.C:527-539 (LoadMapScreen header): the callers omit 122E:1AAB LoadLevelOp
  (src/E122E.C:1287-1288, through the 210C:007A stub). It also omits: PlaySound(0) (:685), gFB45 = 1
  (:686), the gFlags849E bit 2 set and cleared (:566, :684), and the clearing of the 0x400 item-list heads
  (:670-671). Add them.
  Fixed in cd7af2f: the header now names LoadLevelOp (122E:1AAB, through the 210C:007A stub), the
  gFlags849E bit 2 clear and set, the 0x400 item-list head clear, PlaySound(0) and gFB45 = 1.
- [x] should src/EOV3.C:804-810 (SaveGame header): omits `ResetLevel()` at :819, which runs first. Say that
  the INVENT reload (:846) happens only when every write succeeded (:829). The probe FileOpen/FileClose of
  EOBDATA.SAV before the mode-2 open (:825-826) is not mentioned.
  Fixed in 0bd5b44: the header now names ResetLevel, the EOBDATA.SAV probe, and that the INVENT reload
  runs only when every write succeeded. Reopened in round 2: the reload is not gated on the writes.
  Fixed in 89e940a (the reopened point): the header now says the `if (ok)` at :853 follows `ok = 1`, so the
  reload runs after the writes unconditionally and the return value reports them.
- [x] should src/EOV3.C:857-858 (LoadGame header): "draws each alive character's 0x514-byte sprite out of
  gOverlayBuf". The code cuts a 4 by 32 cell with gFn8082 (:919), unpacks it into rec->sprite (:920) and
  frees it (:921). It does not draw. "alive" is not evidenced: rec->m01 is "status bits; 0 when the slot is
  empty" (:116), so the test is "occupied". Say "cuts" and "occupied", or give the evidence for alive.
  Fixed in 0bd5b44: the header now says it cuts a 4 by 32 cell, unpacks it into the sprite buffer and
  frees it, for each occupied character (m01 nonzero). Reopened in round 2: the cut also needs m22 >= 0.
  Fixed in 89e940a (the reopened point): the header now says a character with m01 nonzero and m22 >= 0 is
  cut, and one with m01 nonzero and m22 < 0 sets `flag` at :951 instead.
- [x] should src/EOV3.C:610-615, 618-620 (LoadMapScreen, `recIndex`): it is a timer slot index (index*2 into
  g5BCC and g5BDC), not a record index. RegisterWallObject uses `recordIndex` for the wall chain. Rename the
  local (e.g. timerIdx) so one word does not name two things.
  Fixed in cd7af2f: the local is now `timerIdx`.
- [ ] should src/EOV3.C:666-680 (LoadMapScreen, `scriptWord`): it holds the item-table offset (:666-667)
  and then an item word (:678-680). Two meanings, one name. Split the variable.
  Not split (8db5863): splitting `scriptWord` into two locals (e.g. itemTableOffset and itemWord) gives
  LoadMapScreen an extra frame slot and the matcher drops to 22 of 23 (LoadMapScreen differs); the original
  reuses one slot, so the single variable stays. Kept as one local; the two uses are the level-script item
  table offset at src/EOV3.C:688-689 and the item word at :700-702.
- [x] should src/EOV3.C:654-660 (LoadMapScreen, `nObjects`): counted at :660 and never read. Mark it as
  `QUIRK ovl3:0A7C:` (an unused result), or say in the header that it is dead.
  Fixed in cd7af2f: the declaration is now `/* QUIRK ovl3:0A7C: counted but never read (dead result) */`.
- [x] should src/EOV3.C:172 (struct ItemRecord, `m03`): the comment is "/* +5 */". The field is the sound
  that InsertListItem takes: RebuildItemList passes rec->m03 (:496), and src/E0BD2.C:420 passes
  gItems[saved].m03 as the sound. Give the field that meaning.
  Fixed in cd7af2f: the m03 comment now says it is the sound InsertListItem plays (src/E0BD2.C:420).
- [x] should src/EOV3.C:222-226 (struct WallDec): idx[0..9] are the cell indices that RegisterWallObject reads
  (:316), and idx[10] is the chain link (see the first must-fix). Say so in the struct comment.
  Fixed in cd7af2f: the WallDec comment now says idx[0..9] are the ten cell indices and idx[0xa] the
  chain link.
- [x] should src/EOV3.C:138, 140-145, 14, 55, 189 (types): the file declares types that disagree with the
  other units, and the placeholder `Triple6` is now a known record. gA99C is `struct Triple6` (c[2]) here;
  src/E122E.C:110-117 names it SlotPos {rec, x, y} and says "src/EOV3.C calls them Triple6". Use SlotPos, so
  c[0] and c[1] become x and y. gFB5E is `long` here but `struct SlotSize far *` in src/EOV4.C:756 and
  src/E122E.C. gRectKinds is `void far *` here but `struct RectKind far *` in src/E122E.C:98 and
  src/EOV4.C:661. gE062 is `void far *` here but is read as a Cell8 array (:321). Make each match the
  declaring unit (the matcher checks the bytes).
  Fixed in 8db5863: gA99C is now `struct SlotPos` with `rec`/`x`/`y` (usages at src/EOV3.C:360-362, 384-388,
  396-400; the header at :369 names .rec); gFB5E is `struct SlotSize far *` (:143, :382, :394); gRectKinds is
  `struct RectKind far *` (:15); gE062 is `struct Cell8 far *` (:56). SlotSize and RectKind are used only
  through pointers here, so they are referenced by tag and defined in their declaring units (src/E122E.C);
  Cell8 is defined in this file (:63). All four keep the bytes: `make match FILE=src/EOV3.C` 23 of 23, exit 0.
- [x] should src/EOV3.C:189 (gMapTiles type): `signed char` here, but src/EOV5.C:165 declares the same table
  `unsigned char` and uses the same expression at :1285. Sign extension changes the code for `<< 5`, so
  both cannot be the same bytes unless the matcher finds otherwise. Test the type with `make match` on
  EOV3 and on EOV5, and record the result in the two units. (Matcher question for the human if both match.)
  Fixed in cd7af2f: tested on EOV3. With `signed char` all 23 functions match; with `unsigned char` the
  matcher drops to 20 of 23 (RegisterWallObject, LoadMapCells, LoadMapScreen differ). So EOV3's
  `signed char` is proven; src/EOV5.C's `unsigned char` for the same table is the suspect one (its
  consuming function eov5:1CDD is a draft), for the EOV5 owner to test.
- [x] should src/EOV3.C:285-286 (IsBitSet): "Purpose unknown." Use the colon the rule uses, "Purpose unknown:"
  (as src/E0ACD.C:156 does).
  Fixed in 0bd5b44: the comment is now "Purpose unknown:".
- [x] should src/EOV3.C:57 (gFBD0) and :15 (gF538): placeholder globals with no comment on what is known.
  gFBD0 is the saved-level bit mask: set by SaveLevelState (:764) and SaveGame (:824), tested by LoadObjects
  (:515) and LoadMapScreen (:622). Give each declaration a comment, as the rule for placeholder globals asks.
  Fixed in 0bd5b44: both declarations now carry their description (gFBD0 the saved-level bit mask, gF538
  the wall-record slot count).

### Note

- [ ] note src/EOV3.C:38, :89 and :131: the placeholder callees (f1A47_0169, f0D1C_0670, f1BA8_0072 and the
  others) are declared, not defined here; their names are left to their owners. Fine as they are.
- [ ] note src/EOV3.C:36 and :160: gFn8082 (ds:8082) and gGetSpriteCell (ds:8086) are two names for
  two pointers with the same signature. Both are used across src (src/EOV0.C:16, src/EOV5.C:170-171,
  src/EOV2.C:166). The same job has two names; for the integrator to decide with make rename.
- [ ] note src/EOV3.C:162 and the gFBC8 uses (:515, :567, :818): gFBC8 is "the current action" in
  src/E0BD2.C:95 and src/E0ACD.C:112, but this unit uses it as the level number (LEVEL<n>.INF via
  FormatName(level), gFBD0 bit gFBC8-1, the LEVELS.TMP slot, gMapTiles[gFBC8-1]). The name "action" is
  the src name; the header uses "level" and "action" for the same thing (LoadLevelState's `action`
  parameter). For the human to decide a cross-unit rename.
- [ ] note src/EOV3.C:267-273 and :278-281 (LoadWallSet): the self-assignment loops compile to a load and a
  store of the same word. The header says so; the purpose is unknown. Keep it as "Purpose unknown:" for the
  loop, not as a plain description.
- [ ] note src/EOV3.C:982-985 (ResetLevel): the while loop has no bound. It ends only when the g194B flag of
  the slot has bit 0x30 set; say that in the header.
- [ ] note cross-unit (not this file): src/E1414.C:65, :227-228 and :234 say `m03 &= 3` "clears the low two
  bits". It keeps them and clears bits 2 to 7. The e1414 owner should fix it; the same error is copied into
  the ResetLevel header above.
- [ ] note the `make match` line "unit eov3 not complete: ours 6961 bytes, the unit 6961" is the overlay-unit
  print that the eov6 review already notes as a matcher question for the human (tools/match/cmatch.py). It is
  not a finding in this file.

## Summary

The unit is understandable from its source, and most names are backed by evidence: the LEVEL<n>.INF and
EOBDATA.SAV formats, the VCN/ECN wall files, the saved-game block sizes and the save and load callers all
check out, and the callee names agree with the other units. There are eleven must-fix items. Wrong facts in the
comments: the LoadWallTextures name handling and plane tables (two items), the SaveLevelState caller (122E
is the resident unit, not overlay 0), the SaveGame and LoadGame LEVELS.TMP behaviour (whole image, not one
slot), and the LoadGame OUTTAKE and mouse conditions. Wrong structure or bit facts: the wall-record copy and
link (WallRec, RegisterWallObject) and the `&= 3` reading in ResetLevel. One cross-unit cite disagrees
(gMapTiles at 1F1D:0000 against 01D0:0000), and one name has no evidence (FreeEffectTables: the tables are
slot size records). Placeholder names that stay
placeholders with no evidence are fine: the callees f0D1C_0670, f1A47_0169 and f1BA8_0072, IsBitSet,
objM07/objM12/objM14. Left for the human: the matcher question on signed and unsigned gMapTiles, and the
gFBC8 "action" name, which crosses units.

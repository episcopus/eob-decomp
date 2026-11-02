# Review: eov3

Reviewer: rev-haiku
Commit reviewed: 01237b0 (main when I started; src/EOV3.C is identical in this worktree at 8443107)

Playbook 70. Judge what a matching file can still get wrong: names, types, comments, structure.
The matcher already proved the bytes; do not re-prove them, read for meaning.

## Findings

Each finding is a checkbox. `must-fix` blocks `make review-pass` until a decompiler ticks it off
(with the commit that fixes it); `should` and `note` do not block.

Round 1 (at 01237b0). `make match FILE=src/EOV3.C`: cmatch 23 of 23 functions match, exit 0.
`make readability UNIT=eov3`: 0 findings. The findings below are about meaning.

### Must-fix

- [ ] must-fix src/EOV3.C:69-73, 291-299, 313-316, 325: the WallRec layout and the RegisterWallObject
  header misdescribe the chain. The copy is `movmem(..., gWallRecs[gF538].cells, 0x34)` (:313-314), so
  the 52-byte decoration lands at +0x2a of its slot, not at the slot base. Its first ten bytes are the
  cell indices (:316), and its byte 10 lands at +0 of the next slot. The link read at :325
  (`gWallRecs[gF538].next` after `gF538++` at :324) is therefore decoration byte 10 of the record just
  copied, not a stored "next record index". The header ("copies the decoration ... into
  gWallRecs[gF538]") and the WallRec comment ("+0: the next record index") are wrong. Say the copy is
  52 bytes at +0x2a and the link is read from the next slot's +0, or redeclare WallRec to match.
- [ ] must-fix src/EOV3.C:452-459, 464-467, 484: the LoadWallTextures header says "or an empty name when
  `name` is NULL" and "It leaves gFileNameBuf empty". When name is NULL the code copies the kept name
  gWallNameBuf into gFileNameBuf (:466-467), and at the end copies gFileNameBuf into gWallNameBuf (:484),
  so gFileNameBuf still holds the file name. Evidence: :35 (gWallNameBuf is "kept between
  LoadWallTextures calls").
- [ ] must-fix src/EOV3.C:460-471 (header 452-459): "unpacks three 16-byte plane tables ... into ds:F4D6,
  ds:FB4D and gSegDCA6". Only the first two are 16-byte tables; the third (:471) is the 0xCB20-byte
  texture block into gSegDCA6. The header also says "through ovl3:0FD1's counterpart", but the unpack
  is Unpack0 (1BA8:0006, :469-471) and ovl3:0FD1 is PackLevelState (:692). Name the 1BA8:0006 unpack and
  the 0xCB20 block.
- [ ] must-fix src/EOV3.C:752: "Called by overlay 0 (122E:1AAB)". Address 122E is the resident EOB.EXE
  unit src/E122E.C ("Unit at EOB.EXE 122E:0001", line 1), not overlay 0; overlay 0 is src/EOV0.C with
  ovl0 addresses. The caller is LoadLevelOp (122E:1AAB, src/E122E.C:1268), which calls SaveLevelState
  through the 210C:002F stub (E122E.C:66, call at :1287). Name it as LoadLevelOp.
- [ ] must-fix src/EOV3.C:804-810 (SaveGame header): "It first writes the current level state to
  LEVELS.TMP slot gFBC8-1 through ovl3:0FD1". This function does not write LEVELS.TMP. It reads it into
  gSegEFC2 (:820-822), packs into the in-memory image at gSegEFC2 + slot*0x7F8 (:823-824), and writes the
  0x5FA0-byte image to EOBDATA.SAV (:838-839). Correct the sentence.
- [ ] must-fix src/EOV3.C:851-862, 900-904 (LoadGame header, line 855): "it writes the level-state block
  to LEVELS.TMP slot gFBC8". FileOpen mode 2 deletes and recreates the file (1C0E:00DB, src/E1C0E.C
  `case 2`: DeleteFile, ResetFile), and no SeekFile precedes FileWrite (:902-903). The write starts at
  offset 0 and carries the whole gBlockSizes[3] image (the 0x5FA0 bytes SaveGame wrote at :838-839), not
  one slot. Say the whole LEVELS.TMP image is recreated from block 3.
- [ ] must-fix src/EOV3.C:858-859, 916-924, 931 (LoadGame header, "the front row into the OUTTAKE screen
  when any character sits there"): the flag is set by the `else` at :922-923, which also runs for an
  empty slot (`m01 == 0`; the CharRec comment at :116 says 0 when the slot is empty). So OUTTAKE loads
  whenever any slot is empty or any occupied character has m22 < 0. The header states a condition the
  code does not have. Say what the flag tests, and that the OUTTAKE pass draws the m22 < 0 characters
  (:931).
- [ ] must-fix src/EOV3.C:861-862, 944-949 (LoadGame header): "either restores the mouse item or, when a
  mouse item is shown, inserts it". The test is gMouseDisabled: when it is 0, SetMouseItem(gFBCE) runs
  (:944-945); when the mouse is disabled and gFBCE is nonzero, the item is inserted with a RollDice-chosen
  sound (:946-949). Fix the condition in the sentence.
- [ ] must-fix src/EOV3.C:105-109 (struct Entry14, comment at :107) and :956-960, :972 (ResetLevel
  header): "its low two bits are cleared here" and "disarming the low two bits of its item's m03".
  `m03 &= 3` keeps the low two bits and clears bits 2 to 7 (:972). The record is gTable8DD2[m08], an
  Entry14 found through DDRec.m08 (:101; src/E1414.C:57), not "its item". Fix both comments.
- [ ] must-fix src/EOV3.C:189-191 (gMapTiles at 1F1D:0000) vs src/EOV5.C:165-166 (gSeg1D0 at 01D0:0000):
  the same tile table, read by the same expression `[gFBC8-1] << 5` plus the 1D8 sheet (MAPSHEET at
  EOV3.C:191; EOV5.C:1285), has two addresses in two units. One cite is wrong. Check the segment in the
  EXE (`make map`, build/re/eob.json) and make both units agree. Its type is a separate should, below.
- [ ] must-fix src/EOV3.C:334-336, 530 (FreeEffectTables, and "frees the effect/map/pointer tables" in
  the LoadMapScreen header; callers EOV5.C:210, :1038, :1158): the name "Effect" has no evidence in
  this unit, and the src says these tables are per-slot size records. gFB5E holds size-record pointers
  (src/EOV2.C:146, src/EOV4.C:756 "a far size record"), and gA99C holds the six-byte SlotPos records
  (src/E122E.C:107-117, "the six-byte slot-position records at ds:A99C"). Rename the function with
  make rename (it is an overlay entry, 210C:003E, used by EOV5), giving the evidence, or write
  "Purpose unknown:" with what is known.

### Should

- [ ] should src/EOV3.C:765-767 (AgeCharTimers header): "a smaller nonzero one is set to 1". The else
  branch (:779-780) also covers a countdown equal to g8492 (not greater, so not reduced, and nonzero), and
  sets it to 1 too. Say "a nonzero countdown not greater than g8492".
- [ ] should src/EOV3.C:228-229 (FreePtrTable): "resets the counter ds:F538". gF538 is the wall-record
  count that RegisterWallObject advances (:307-324; the declaration at :15 has no description). Say so,
  and that this reset also empties the wall records.
- [ ] should src/EOV3.C:352-357 (LoadMapCells): "loads the overlay screen at ds:6A98" is ambiguous. Name
  the screen string "DOOR" (:363) and say where it sits. The srcB branch reads its ds:6900 pair with srcA
  (QUIRK ovl3:0597 at :383); the header says it "does the same at dstB", which hides the quirk. Say it.
  The header also says "copies the two gA99C[dstA].c bytes" (:356-357): c[0] and c[1] are the slot's x and
  y (see the SlotPos point below).
- [ ] should src/EOV3.C:405-407, 421-422, 436-438 (LoadMapIcons6, LoadMapIcons6b, LoadNamedIcons): "No
  direct caller found" is true but misleading. The three are called through the `handlers` table of
  LoadMapScreen (:562, :603, :608) and the ds:6A80 initialiser (:217-220). Say so.
- [ ] should src/EOV3.C:3 (unit header): "loads and unpacks a level state from LEVELS.TMP (ovl3:09AA,
  109D, 1147)". ovl3:09AA (LoadObjects) loads an object table, and calls LoadLevelState only when the level
  was saved (:515-516). The level state is read by 1147 and unpacked by 109D. Reword.
- [ ] should src/EOV3.C:527-539 (LoadMapScreen header): the callers omit 122E:1AAB LoadLevelOp
  (src/E122E.C:1287-1288, through the 210C:007A stub). It also omits: PlaySound(0) (:685), gFB45 = 1
  (:686), the gFlags849E bit 2 set and cleared (:566, :684), and the clearing of the 0x400 item-list heads
  (:670-671). Add them.
- [ ] should src/EOV3.C:804-810 (SaveGame header): omits `ResetLevel()` at :819, which runs first. Say that
  the INVENT reload (:846) happens only when every write succeeded (:829). The probe FileOpen/FileClose of
  EOBDATA.SAV before the mode-2 open (:825-826) is not mentioned.
- [ ] should src/EOV3.C:857-858 (LoadGame header): "draws each alive character's 0x514-byte sprite out of
  gOverlayBuf". The code cuts a 4 by 32 cell with gFn8082 (:919), unpacks it into rec->sprite (:920) and
  frees it (:921). It does not draw. "alive" is not evidenced: rec->m01 is "status bits; 0 when the slot is
  empty" (:116), so the test is "occupied". Say "cuts" and "occupied", or give the evidence for alive.
- [ ] should src/EOV3.C:610-615, 618-620 (LoadMapScreen, `recIndex`): it is a timer slot index (index*2 into
  g5BCC and g5BDC), not a record index. RegisterWallObject uses `recordIndex` for the wall chain. Rename the
  local (e.g. timerIdx) so one word does not name two things.
- [ ] should src/EOV3.C:666-680 (LoadMapScreen, `scriptWord`): it holds the item-table offset (:666-667)
  and then an item word (:678-680). Two meanings, one name. Split the variable.
- [ ] should src/EOV3.C:654-660 (LoadMapScreen, `nObjects`): counted at :660 and never read. Mark it as
  `QUIRK ovl3:0A7C:` (an unused result), or say in the header that it is dead.
- [ ] should src/EOV3.C:172 (struct ItemRecord, `m03`): the comment is "/* +5 */". The field is the sound
  that InsertListItem takes: RebuildItemList passes rec->m03 (:496), and src/E0BD2.C:420 passes
  gItems[saved].m03 as the sound. Give the field that meaning.
- [ ] should src/EOV3.C:222-226 (struct WallDec): idx[0..9] are the cell indices that RegisterWallObject reads
  (:316), and idx[10] is the chain link (see the first must-fix). Say so in the struct comment.
- [ ] should src/EOV3.C:138, 140-145, 14, 55, 189 (types): the file declares types that disagree with the
  other units, and the placeholder `Triple6` is now a known record. gA99C is `struct Triple6` (c[2]) here;
  src/E122E.C:110-117 names it SlotPos {rec, x, y} and says "src/EOV3.C calls them Triple6". Use SlotPos, so
  c[0] and c[1] become x and y. gFB5E is `long` here but `struct SlotSize far *` in src/EOV4.C:756 and
  src/E122E.C. gRectKinds is `void far *` here but `struct RectKind far *` in src/E122E.C:98 and
  src/EOV4.C:661. gE062 is `void far *` here but is read as a Cell8 array (:321). Make each match the
  declaring unit (the matcher checks the bytes).
- [ ] should src/EOV3.C:189 (gMapTiles type): `signed char` here, but src/EOV5.C:165 declares the same table
  `unsigned char` and uses the same expression at :1285. Sign extension changes the code for `<< 5`, so
  both cannot be the same bytes unless the matcher finds otherwise. Test the type with `make match` on
  EOV3 and on EOV5, and record the result in the two units. (Matcher question for the human if both match.)
- [ ] should src/EOV3.C:285-286 (IsBitSet): "Purpose unknown." Use the colon the rule uses, "Purpose unknown:"
  (as src/E0ACD.C:156 does).
- [ ] should src/EOV3.C:57 (gFBD0) and :15 (gF538): placeholder globals with no comment on what is known.
  gFBD0 is the saved-level bit mask: set by SaveLevelState (:764) and SaveGame (:824), tested by LoadObjects
  (:515) and LoadMapScreen (:622). Give each declaration a comment, as the rule for placeholder globals asks.

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

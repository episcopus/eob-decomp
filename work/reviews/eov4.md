# Review: eov4

# Round P6 re-review (2026-11-04), commit 24711ec

Reviewer: p6-rev-r2
Commit reviewed: 24711ec (P6 re-review; the P6 round read ce0608f)

Judged: the 18 open P6 must-fix items and the diff since ce0608f for src/EOV4.C. `make match FILE=src/EOV4.C`: 18 of 18 (8259 bytes), exit 0. `make readability UNIT=eov4`: 0 findings.

- [x] must-fix The global declarations are fixed right. `gWallTypeFlags` is per-wall-type, `gWallVmpSet` is the per-wall-type 1-based block set in the level's .VMP/.EMP, `gViewSlotRow` is a per-view-slot row index, `gDoorShapeSet` and `gDoorStepBase` are per wall type, the six `gDoorYLevels` tables and `gDoorClipLeftX`/`gDoorClipRightX` are per level (1..12, with the right x bound), and `gWallDecorHead`, `gDecorShapes`, `gViewRelativePos`, `gCompassDir`, `gCompassIcons` and `gProjectileDrawOrder` use wall type, facing or view slot as the registry and the code require. The `gDoorClipRightX` comment is now per slot x, level 4..6, matching `right = gDoorClipRightX[slot * 2] >> 3`.
- [x] must-fix The four headers are fixed right. `BuildViewBlockMap` now describes the 22 by 15 word block map at ds:ADB0, the per-facing gViewCells offsets, the cell and facing parity, and the .VMP blocks copied into the map. `DrawViewFrame` says the block map, `DrawCompass` says the current facing, and `DrawSlotProjectiles` uses facing and the cell at +4. These match re/renames.toml, ovl4:044B and the code.
- [ ] note No new false claim was introduced in the changed lines. The three P6 shoulds on `PreloadViewOverlay`, `gViewBlockMap` and `gMonsterDrawOrder` remain non-blocking.

# Round P6 (2026-11-04), commit ce0608f

Reviewer: p6-rev-10
Commit reviewed: ce0608f367ed01b1b1df84e254eef04d1a734b27

## Round P6 (2026-11-04), commit ce0608f367ed01b1b1df84e254eef04d1a734b27

Judged: the P6 changes in `src/EOV4.C` since 366bb3b (the round 4 base), 414 insertions and 507 deletions. Method: read the whole diff, checked every new function, global and struct field against `re/renames.toml` and `re/fields.toml`, read the code at each named address, and grepped the other files of `src/` for the new names. The names are mostly backed by the WHY entries. The comments on the changed lines still use the old vocabulary in many places: "page" for the party's facing (ds:FBCC), "action" for the dungeon level (ds:FBC8), "per-slot" for a table indexed by a wall type (ds:194B), "per-object" for a table indexed by a view slot (ds:1A09), "per-heading" for a table indexed by a wall type (ds:716E and ds:714E), "per-cell" for a table indexed by a wall type (ds:EFD4), and "per-object-kind" for a per-wall-type table (ds:18BF). These are wrong in changed lines.

Must-fix:

- [x] must-fix src/EOV4.C:12 `gWallTypeFlags` (ds:194B): the comment "per-slot byte flags" is wrong. The table is per wall type: `re/renames.toml` ds:194B says "flags per wall type, indexed by a wall byte of gMaze", and the code indexes it by a wall type (`gWallTypeFlags[tile]` at DrawSlotItems, `gWallTypeFlags[slot]` where slot is a wall type at BuildViewBlockMap). Say "per-wall-type byte flags". Fixed in def28b2.
- [x] must-fix src/EOV4.C:13 `gWallVmpSet` (ds:1879): "per-object count-1" is wrong. The entry says "the 1-based block set in the level's .VMP/.EMP file", and CopyViewColumn reads `(gWallVmpSet[page] - 1) * 0x35E`. Say "per-wall-type 1-based block set in the level's .VMP/.EMP". Fixed in def28b2.
- [x] must-fix src/EOV4.C:130 `gViewSlotRow` (ds:1A09): "per-object index" is wrong. The table is indexed by a view slot row: the code uses `gViewSlotRow[slot]` and `gViewSlotRow[col]` where slot and col are view slots. Say "per-view-slot row index". Fixed in def28b2.
- [x] must-fix src/EOV4.C:691 `gDoorShapeSet` (ds:716E): "per heading" is wrong. The entry at ds:716E (line 6535) says "per wall type". Say "per wall type". Fixed in def28b2.
- [x] must-fix src/EOV4.C:692-697 the six `gDoorYLevels...` tables (ds:70F0, ds:70F4, ds:70F8, ds:70FC, ds:70FF, ds:7102): the comments say "action 1..3" etc., but the code uses `gCurLevel` (the dungeon level). Say "level 1..3", "level 4..6", "level 7..9", "level 10..11" (twice) and "level 12". Fixed in def28b2.
- [x] must-fix src/EOV4.C:698 `gDoorClipLeftX` (ds:7106): "per slot x, action 4..6" should say "level 4..6". Fixed in def28b2.
- [x] must-fix src/EOV4.C:699 `gDoorClipRightX` (ds:7108): "per slot y, action 4..6" is wrong twice. The entry at ds:7108 (line 6598) says "ds:7108 is the right word two bytes on" (an x bound), and the code uses `right = gDoorClipRightX[slot * 2] >> 3`. Say "per slot x, level 4..6". Fixed in def28b2.
- [x] must-fix src/EOV4.C:700 `gDoorStepBase` (ds:714E): "per heading" is wrong. The entry at ds:714E (line 6616) says "Per wall type". Say "per wall type". Fixed in def28b2.
- [x] must-fix src/EOV4.C:593 `gWallDecorHead` (ds:18BF): "per-object-kind table" is wrong. The entry at ds:18BF (line 9658) calls it one of "the four per-wall-type tables". Say "per-wall-type table". Fixed in def28b2.
- [x] must-fix src/EOV4.C:605 `gDecorShapes` (ds:EFD4): "per-cell far size records" is wrong. The entry at ds:EFD4 (line 6301) says "Six far shape pointers" cut per wall type. Say "per-wall-type far size records". Fixed in def28b2.
- [x] must-fix src/EOV4.C:1037 `gCompassDir` (ds:7296): "the page the rows were last drawn for" is wrong. The entry at ds:7296 (line 6773) says "the facing the compass was last drawn for", and the code compares it to `gPartyDir`. Say "the facing". Fixed in def28b2.
- [x] must-fix src/EOV4.C:841 `gProjectileDrawOrder` (ds:7246): "per page and low-2-bit index" is wrong. The entry at ds:7246 (line 6679) says "Per facing", and the code uses `gPartyDir * 5 + ...`. Say "per facing". Fixed in def28b2.
- [x] must-fix src/EOV4.C:1040 `gCompassIcons` (ds:A92A): "per-page, per-row" is wrong. The entry at ds:A92A (line 8033) says "three rows by four facings". Say "per facing, per row". Fixed in def28b2.
- [x] must-fix src/EOV4.C:357 `gViewRelativePos` (ds:1A2B): "per-page sprite base" is wrong. The entry at ds:1A2B (line 3781) says "indexed [gPartyDir * 4 + view position]". Say "per facing". Fixed in def28b2.
- [x] must-fix src/EOV4.C:239-246 the `BuildViewBlockMap` header (ovl4:044B): "draws the whole view page", "per-page gViewCells target offsets", "at the page index", "the page's tile records" and "from the page buffer" still use the old "page". The entry at ovl4:044B (line 6800) says it "Clears the 22 by 15 word map at ds:ADB0 ... copies each visible wall's .VMP blocks into the map". Say "the view block map" and "per facing". Fixed in def28b2.
- [x] must-fix src/EOV4.C:1076 the `DrawViewFrame` header (ovl4:1F51): "draws the objects into the page buffer" still says "page buffer". The function's name is `BuildViewBlockMap` and the entry says it builds the map at ds:ADB0. Say "block map" or "view buffer". Fixed in def28b2.
- [x] must-fix src/EOV4.C:1045 the `DrawCompass` header (ovl4:1EDE): "for the current page" should say "for the current facing". Fixed in def28b2.
- [x] must-fix src/EOV4.C:854 and :862 the `DrawSlotProjectiles` header (ovl4:19C4): "seen from the current page" and "neither the page nor the page xor 2" should say "facing". Fixed in def28b2.

Should:

- [ ] should src/EOV4.C:1204 the `PreloadViewOverlay` header (ovl4:2039): "Purpose unknown" is stale. The entry at ovl4:2039 (line 6863) says its callers bring overlay 4 into memory; say that instead.
- [ ] should src/EOV4.C:175 the `gViewBlockMap` declaration (ds:ADB0): "the view pixel buffer" is the old term; the name is `gViewBlockMap`, so say "the view block map" or "the 22 by 15 word block map".
- [ ] should src/EOV4.C:349 the `gMonsterDrawOrder` declaration (ds:7001): "the far half first" paraphrases "back to front" from the entry; say "back to front".

Notes (for the next reader; they do not block):

- [ ] note src/EOV4.C:45 and :3052 `gTeleporterCells` (ds:F4BE) and `DrawSlotTeleporter` (ovl4:1D20) are marked [H] in `re/renames.toml` (ScummVM's EOB1 teleporter is wall type 0x34); the source comments do not say [H], so a reader may take the name as established. The wall type 0x34 and the two-frame animation are established; the "teleporter" reading is the hypothesis.
- [ ] note src/EOV4.C:143-145 the QUIRK on ds:6F6D (12 bytes indexed by gCurLevel 1 to 12, level 12 reads gDoorClipBottomRow[0]) matches the table addresses ds:6F6D and ds:6F79 12 bytes apart. The note stands.

Summary: the P6 names are backed, but the changed lines still carry the old "page", "action", "per-slot", "per-object", "per-heading", "per-cell" and "per-object-kind" wording in 18 places. These are must-fix. Two shoulds (the PreloadViewOverlay header and two vague descriptions). Nothing escalated: no matcher, spec or pin question.

Playbook 70. Judge what a matching file can still get wrong: names, types, comments, structure.
The matcher already proved the bytes; do not re-prove them, read for meaning.

Evidence used: `make match FILE=src/EOV4.C` (18 of 18), `make readability UNIT=eov4` (0 findings), `build/packets/eov4.md`, the other files of `src/` (grep), and a capstone disassembly of `build/re/eob-overlays/ovl04.bin` (16-bit, offset = ovl4 offset) to check every call-site address cited in a comment. Call sites found: ComputeViewSpan 1E3C (from DrawViewSlots); Ovl4_1091 1E50; Ovl4_0AD0 1E8C; Ovl4_1411 1EA2; Ovl4_0D1B 1EB9; Ovl4_19C4 1EBF; Ovl4_1D20 1ECB; Ovl4_044B 1F5D (from DrawViewFrame); DrawViewRows 202F; DrawViewSlots 1F75; ComputeViewSpan also from Ovl4_0AD0 at 0C41 and 0CFB.

Round 1 fixed by dec-deepseek in commit 2020279: all 13 must-fix and all 16 should items
addressed. Comment and name changes only, no code change. `make match FILE=src/EOV4.C` 18 of 18
(8259 bytes), `make readability UNIT=eov4` 0 findings. Names changed with `make rename`:
ds:1879 -> gTable1879, ds:A92A -> gIconTableA92A (re/globals.toml, re/renames.toml).

## Round 4 (rev-haiku, main at 366bb3b)

Judged: the round 3 must-fix item and the round 3 shoulds on this unit's lines, and the changes since ea7fcbe in `src/EOV4.C` and `re/globals.toml` (`re/symbols.toml` has no eov4 entry changed; the diff is other units' notes and stamps). Method: `ovl4:1F51` was disassembled from `build/re/eob-overlays/ovl04.bin` with capstone (16-bit: `push bp`, `mov bp,sp`, and the `call 0x44b` at 1F5D the header describes). The code the new text describes was read with sed (:1182-1189, :459). The `ds:8102` list was checked against src/E07C8.C:42-45, and the `ds:80C6`, `ds:8104` and `ds:8111` cites against src/E1C0E.C:553, :176, :177 and :165. The new globals entries of other units (ds:628C, ds:80E8, ds:F43C, ds:FE2C, ds:1278, ds:12E0, ds:5737) were spot-checked: the labels and message numbers match src/EOV1.C:1263, :1265 and :1422, and the g5737 cite matches src/E0EF3.C:227. The spell-name counts (25 mage, 24 cleric) were not decoded from the EXE. `make match FILE=src/EOV4.C` 18 of 18 (8259 bytes), exit 0; `make readability UNIT=eov4` 0 findings.

Round 3 must-fix (fixed right; tick stays): the DrawViewFrame header now reads "when gFB45 is set and gGraphicsChoice is 3 (MCGA/VGA, re/globals.toml ds:8102)". It matches the `gFB45 && gGraphicsChoice == 3` test at :1182, and the palette load and the clear (:1188-1189) are outside the `full` block. No "video mode" wording is left in src/EOV4.C.

Round 3 shoulds on this unit's lines (fixed right; ticks stay): the gGraphicsChoice declaration (:529) now gives the list 0 CGA, 1 Tandy, 2 EGA, 3 MCGA/VGA, 4 VGA, which matches src/E07C8.C:42-45 and the ds:8102 entry. The Ovl4_0AD0 header (:403) gates the ds:6F86 recompute with "when that byte is not -1", which matches `if (spanSlot != -1)` at :459.

Round 3 should still open, not this unit's to fix: src/E07A9.C:87 (owner E07A9) still reads "video mode 3" for the gFB45 test and cites the stale src/EOV4.C lines (:1049, :1054, :1060, :1061). Left unticked; it is a should under playbook 70, and the owner's review will take it.

Changed lines since ea7fcbe, other units' registry entries: the ds:8102 why (E07C8.C:44-45, the gOverlayFiles order) and the ds:80C6, ds:8104 and ds:8111 cites are right, as above.

Must-fix: none.

Should:

- [ ] should src/EOV4.C:1174-1175 (DrawViewFrame header, the round 3 edit): the rewrap leaves "with pattern 0xc). Then it waits for gCount849A" on one line and "to reach zero, and, when `full` is set" on the next, so the sentence breaks in the middle of a phrase at a different width from the lines around it. Reflow the paragraph (the words are right).

Notes (for the next reader; they do not block):

- [ ] note src/EOV4.C:545 (Ovl4_0D1B header, text no fix touched): "through the sprite blitter the video mode selects". This is the word round 3 asked to stop using for gGraphicsChoice. If the blitter depends on the graphics choice (the 6-bit cells for choice 3 and 4, src/E1DE7.C:169 and :202), say so; otherwise leave it. The re/globals.toml ds:8102 entry cites E1DE7.C:163-165 for the same fact; that stale cite is the E1DE7 owner's.

Summary: the round 3 must-fix is fixed right and the changed lines of this unit are sound. No must-fix open. One new should (the rewrap of the DrawViewFrame header, a changed line), one round 3 should still open in src/E07A9.C (its owner's). Nothing escalated: no matcher, spec or pin question.

## Round 3 (rev-haiku, main at ea7fcbe)

Judged: the round 2 must-fix items, and the changes since ac24cbe in `src/EOV4.C`, `re/globals.toml` and `re/symbols.toml`. Method: the cited addresses were disassembled from `build/re/eob-overlays/ovl04.bin` (ovl4) and `ovl03.bin` (ovl3) with capstone, 16-bit; the cited source lines were read with sed. `make match FILE=src/EOV4.C` 18 of 18 (8259 bytes), `make readability UNIT=eov4` 0 findings.

Round 2 must-fix items (all seven fixed right; ticks stay):

- Ovl4_1091 `ovl4:10AD` and `ovl4:11FD`: right. 10AD is `mov ax, word [bx+0x7018]` (the slot*2+1 face test, drawn first); 11FD is `mov al, [bp-3]` (slot*2, drawn second), which `jmp 0x11fd` at 10B7 reaches and 11F8 falls into.
- Ovl4_1411 region-5 fit `ovl4:1614`: right (`lcall 0x1b8,0x2fe` at 1614, the same call as the `SetRegionExtents` at :862).
- Ovl4_1411 actions 4..6 "x shift": right (:863-870 add the shift to x at :868 and subtract it at :870).
- Ovl4_0AD0 `ds:6F86[slot]`: right (:458 `spanSlot = g6F86[slotReg]`). A should below adds the `-1` gate.
- gFB5E `ovl3:031A`: right. The ovl3 loop at 031E-0320 walks the entries; 034D and 0353 clear both words of the far pointer, and `farfree` is pushed at 0338-033C; EOV3.C:356-358 is the same loop.
- DrawViewFrame palette and blank: right on the claim the round 2 fix made. The palette load (:1188) and the gFB45 clear (:1189) are outside `full`, and only the blank (:1183-1186) is under it. The phrase "video mode 3" in the same header is a new must-fix below; round 2 suggested that wording and it is wrong.
- IsSlotLive `src/EOV3.C:1012`: right (`slot = (g194B[gB894[who * 9]] & 8) ? 0 : 1;`).

Changed since ac24cbe, besides the above:

- `re/globals.toml` ds:FBCA (gPartyCell): checked each cite. E0ACD.C:1004 is `gPartyCell = 0x1EA;`, EOV6.C:439 is the `Distance(gPartyCell, ...)` test, E08E6.C:334 and :469 are the compares, E08E6.C:349, :561 and :604 are `PlaySoundAtCell` calls, E0D1C.C:44 indexes gB894 by it. All right.
- `re/globals.toml` ds:8102 (gGraphicsChoice): E07C8.C:229 (`gGraphicsChoice = argv[1][0] + 1`), :296 (the 0x36 check), :298 (`-= 0x31`). Right.
- `re/globals.toml` ds:80C6, ds:8104, ds:8111 (src/E1C0E.C lines): off by one, see the should below. These are another unit's lines.
- `re/symbols.toml`: 173 added or changed entries, all `review = "passed 2026-11-02 rev-haiku"` stamps on other units (e0d1c, e0de8, e1414, e17a6, eov0, eov3, eov5). No `eov4` entry changed.

Must-fix:

- [x] must-fix src/EOV4.C:1172 (DrawViewFrame header): "when gFB45 and video mode 3 are set". The code tests `gGraphicsChoice == 3` (:1182), which is the graphics choice, not a video mode. Two pieces of evidence: whitepaper 2.6 gives argv[1] as the graphics choice and shows the oracle run (`3 4 0 homie 3`) with choice 3 loading `MCGA.OVL` and setting mode 13h; the `3` that is a video mode is argv[5] (ds:84A0, `gRestoreMode`, "the video mode restored on exit"). `re/globals.toml` ds:8102 also names choice 3 as MCGA/VGA. Fix: "when gFB45 is set and gGraphicsChoice is 3 (MCGA/VGA, re/globals.toml ds:8102)".
  (fixed: the DrawViewFrame header now reads "when gFB45 is set and gGraphicsChoice is 3 (MCGA/VGA, re/globals.toml ds:8102) it loads the palette gFE68 and clears gFB45", matching the `gFB45 && gGraphicsChoice == 3` test at src/EOV4.C:1182.)

Should:

- [x] should src/EOV4.C:529 (the gGraphicsChoice declaration): "the video mode/format (src/E07A9.C)" has the same conflation. Say the graphics choice (0 CGA, 1 Tandy, 2 EGA, 3 MCGA/VGA, 4 VGA) and cite re/globals.toml ds:8102.
  (fixed: the declaration now reads "the graphics choice: 0 CGA, 1 Tandy, 2 EGA, 3 MCGA/VGA, 4 VGA (re/globals.toml ds:8102; src/E07C8.C:42-45)", matching the list at src/E07C8.C:42-45.)
- [x] should src/EOV4.C:403-404 (Ovl4_0AD0 header): "also recomputes the span of ds:6F86[slot]" reads as unconditional. The `ComputeViewSpan` call is only made when that byte is not -1 (:459-460). Add "when that byte is not -1".
  (fixed: the header now reads "also recomputes the span of ds:6F86[slot] (the view slot's span slot), when that byte is not -1, and pulls the object up 4 pixels", matching the `if (spanSlot != -1)` gate at src/EOV4.C:459.)
- [x] should re/globals.toml, the ds:80C6, ds:8104 and ds:8111 whys (owner: the E1C0E unit; the same three items are the open round 4 must-fix in `work/reviews/e1c0e.md` on main, not this unit's): ds:80C6 cites src/E1C0E.C:554, which is `MemAlloc` (the `gSaveRectSize` call is :553); ds:8104 cites :177 and :178 (the `gFatalHandler` calls are :176 and :177; :178 is `ShutdownGame`); ds:8111 cites :166, the closing brace (the `RetryFile` call is :165). Line cites into another file are a should, not a must-fix, under playbook 70.
  (fixed elsewhere: the E1C0E unit fixed these on main in commit a72407e, which is in this branch's history; re/globals.toml:472 now cites src/E1C0E.C:553, :482 cites :176, :177 and :487 cites :165, matching the calls.)
- [ ] should src/E07A9.C:87 (owner: E07A9, not this unit): the gFB45 comment repeats "video mode 3" for the same test and cites src/EOV4.C:1049, :1054, :1060 and :1061. Those lines are stale: DrawViewFrame is at :1177, the test at :1182, the palette load at :1188 and the clear at :1189. Replace with the ovl4 addresses or the current lines, and use the same word as the fix above.
  (still open: owner E07A9; src/E07A9.C:87 still reads "video mode 3" and cites the stale src/EOV4.C lines. Not changed on this unit branch.)

Notes (for the next reader; they do not block):

- The round 2 notes stand unchanged (gEF6A signedness, gFn80D2 and the Region struct, :584 `page5`, the `0x12` nudge at :603-605, the 0x34 object, the EOV2 cites, NoOp2039). They were not re-read this round.

Summary: one must-fix open, the "video mode 3" wording in the DrawViewFrame header (src/EOV4.C:1172), which the fix at :529 should match. Everything else in the round 2 fixes and the changed lines is right. The registry and cross-file items are the owners' to fix. `make match FILE=src/EOV4.C` 18 of 18 and `make readability UNIT=eov4` 0 findings. Nothing escalated: no matcher, spec or pin question.

## Round 2 (rev-haiku, main at ac24cbe)

Judged: the round 1 must-fix items (checked against the diff; the ticks below stay, since each fix is right
except where a new must-fix says otherwise) and the changes since 467a5b5 in `src/EOV4.C`, `re/symbols.toml` and
`re/globals.toml`. Method: every `ovl4:` address the changed headers cite was checked against
`build/re/eob-overlays/ovl04.bin` (capstone, 16-bit). The round 1 addresses are right: 1E3C, 0C41, 0CFB (ComputeViewSpan),
1E50, 1EA2, 1ECB, 1F5D, 202F, 1F75, 19A0, 0563, 05D8. The placeholder names (Ovl4_0097, Ovl4_1E18, Ovl4_1F51) and `g1879`
and `gFarA92A` are gone from `src/EOV4.C`. The `gTable1879` and `gIconTableA92A` renames agree with `re/globals.toml`.

Must-fix (new, in changed lines; the Ovl4_1091, Ovl4_1411, Ovl4_0AD0 and DrawViewFrame headers):

- [x] must-fix src/EOV4.C, Ovl4_1091 header: "drawn first (ovl4:0693)" and "drawn second (ovl4:0715)" are not addresses in
  Ovl4_1091. ovl4:0693 and ovl4:0715 are in Ovl4_044B (the `les bx,[ef6e]` record read and the `call 0x42c` IsSlotLive
  test at 0697/0711; the round 1 line numbers :693 and :715 were turned into addresses). The face at slot*2+1 (`sideB`,
  first in the C) starts at ovl4:10AD (`mov ax,[bx+0x7018]`); the face at slot*2 (`sideA`) starts at ovl4:11FD (`10B7`
  jumps there when sideB is empty, and `11F8` falls into it after the sideB loop). Fix: "drawn first (ovl4:10AD)" and
  "drawn second (ovl4:11FD)".
  (fixed: the Ovl4_1091 header now cites ovl4:10AD and ovl4:11FD. Both checked against build/re/eob-overlays/ovl04.bin: 10AD is `mov ax, word [bx+0x7018]`, the sideB face test, and 11FD is the sideA entry the `jmp 0x11fd` at 10B7 reaches.)
- [x] must-fix src/EOV4.C, Ovl4_1411 header: "fit region 5 once (ovl4:0852)". ovl4:0852 is in the Ovl4_044B range (it is a
  `push ax` in an object-draw sequence), not in Ovl4_1411 (which starts at 1411). The region-5 fit for actions 4..6 is
  the `lcall 0x1b8,0x2fe` at ovl4:1614 (SetRegionRect's entry at ovl4:008D makes the same call, so 02FE is
  SetRegionExtents). The DrawRegion000F and DrawRegion004D calls at ovl4:1688 and 16B5 follow it, in the order of the
  code at :862-870. Fix: "fit region 5 once (ovl4:1614)".
  (fixed: the Ovl4_1411 header now says "fit region 5 once (ovl4:1614)"; ovl04.bin shows the `lcall 0x1b8,0x2fe` at 1614.)
- [x] must-fix src/EOV4.C, Ovl4_1411 header, actions 4..6: "with a y shift from ds:7196 below 0x1e and ds:719A at or
  above". The shift is an x shift: `shift = -((heading - g714E[heading]) * g7196[idx])` (:864), then
  `DrawRegion000F(2, rec, x + shift, y, 5)` and `DrawRegion004D(2, rec, x - shift, y, 5)` (:868-870). Fix: "with an x
  shift from ds:7196 below 0x1e and ds:719A at or above".
  (fixed: the header now reads "with an x shift from ds:7196 below 0x1e and ds:719A at or above"; the shift is added to x at :868 and subtracted from x at :870.)
- [x] must-fix src/EOV4.C, Ovl4_0AD0 header: "recomputes the span of the object's ds:6F86 span slot". The code
  indexes it by the view slot: `spanSlot = g6F86[slotReg]` (:458), and `g6F86` is declared a per-slot byte (:380). Fix:
  "recomputes the span of ds:6F86[slot] (the view slot's span slot)".
  (fixed: the header now says "recomputes the span of ds:6F86[slot] (the view slot's span slot)", matching `spanSlot = g6F86[slotReg]` at :458.)
- [x] must-fix src/EOV4.C, DrawViewFrame header: "when gFB45 and video mode 3 are set, and `full` is set, it blanks the
  view box (gFillRect with pattern 0xc) and loads the palette gFE68, then clears gFB45". Only the blank is under
  `full`. The code (:1182-1189) loads the palette (`gFn80BA(gFE68)`) and clears gFB45 whenever gFB45 and
  gGraphicsChoice == 3 are set, and only the blank (:1183-1186) needs `full`. Fix: "when gFB45 and video mode 3 are set it
  loads the palette gFE68 and clears gFB45; when `full` is also set, it first blanks the view box (gFillRect with
  pattern 0xc)".
  (fixed: the DrawViewFrame header now reads "when gFB45 and video mode 3 are set it loads the palette gFE68 and clears gFB45; when `full` is also set, it first blanks the view box (gFillRect with pattern 0xc)", matching the blank under `if (full)` at :1183-1186 and the palette load and clear outside it at :1188-1189.)
- [x] must-fix src/EOV4.C:766 (gFB5E): "(src/E0EF3.C:031A; ...)". src/E0EF3.C has no gFB5E and no 031A, so the cite is
  wrong. The code that frees and clears the ds:FB5E entries is ovl3:031A (src/EOV3.C:356-358 `if (gFB5E[i]) farfree`),
  as re/renames.toml:2206 records. Fix: cite ovl3:031A (the EOV2.C:155 and EOV3.C:143 `long` declarations are right; the
  E0EF3 cite is the one to replace).
  (fixed: the gFB5E comment now cites ovl3:031A and src/EOV3.C:356-358; src/E0EF3.C no longer appears. grep finds no gFB5E in src/E0EF3.C.)

Should:

- [x] should src/EOV4.C, IsSlotLive header: "src/EOV3.C:1010" for the bit 3 read with other meanings. EOV3.C:1010 is
  `who = gDragMember;`; the `& 8` read is at src/EOV3.C:1012. Cite :1012 (stale by two lines).
  (fixed: the IsSlotLive header now cites src/EOV3.C:1012, where `slot = (g194B[gB894[who * 9]] & 8) ? 0 : 1;` reads bit 3.)

Notes (no change needed):

- The `push cs` at ovl4:0211 (ComputeViewSpan's call to SetRegionRect, the SetRegionRect header) and at ovl4:04D3
  (InitViewSlots' call from Ovl4_044B) open a near call; the call instructions are at ovl4:0212 and ovl4:04D8. The round 1
  addresses are the start of the call sequence and are accepted.
- The round 1 note on the gTable1879 signedness (EOV3.C:60 `unsigned char`) still stands; EOV4 keeps signed char (the
  cbw at ovl4:02C9). It is the other file's owner's to reconcile.

## Round 1 findings

Must-fix: a wrong claim, or a name or citation with no evidence.

- [x] must-fix src/EOV4.C:58: ComputeViewSpan says `left`/`right` receive the span in "2-pixel columns". They are 8-pixel columns: SetRegionRect (:38-41) takes 8-pixel columns and ComputeViewSpan passes `*left`/`*right` straight to it (:117); VIEW_RIGHT (:51) is 0x16 columns = 0xb0 pixels; Ovl4_1411 shifts the ds:7106/ds:7108 pixel bounds right by 3 (:846-847). (fixed: the header now says the span is in 8-pixel columns and notes the two values go straight to SetRegionRect.)
- [x] must-fix src/EOV4.C:61-64: the ComputeViewSpan header ties ADJUST_SKIP to a zero ds:1879 count. The code (:89-92) ignores a record when its ds:1879 count is zero (for any adjustment) and ignores ADJUST_SKIP (for any count). Say both conditions. (fixed: now "the record is ignored when its ds:1879 count is zero or its adjustment is ADJUST_SKIP".)
- [x] must-fix src/EOV4.C:68-69: the QUIRK note is wrong. `pair` is a separate local set at :85; `j` is not the pair index before the loop (it is only the gViewSlots index in the `if` at :79-81). Inside the loop `j = (slot + i) * 2` (:103), and `slot` was already multiplied by VIEW_SLOTS (:86), so `j` is not `2 * slot`. Delete or rewrite the note. (fixed: note deleted.)
- [x] must-fix src/EOV4.C:66: "Called by Ovl4_1E18 at ovl4:1E23". The DrawViewSlots call is at ovl4:1E3C (ovl4:1E23 is inside the `jmp` at 1E22). The callers also include Ovl4_0AD0 at ovl4:0C41 and ovl4:0CFB, which the comment omits. Use the current name, DrawViewSlots, for the first caller. (fixed: "Called by DrawViewSlots at ovl4:1E3C and Ovl4_0AD0 at ovl4:0C41 and ovl4:0CFB".)
- [x] must-fix src/EOV4.C:254: Ovl4_044B "Called by DrawViewFrame at ovl4:1F6B". The call is at ovl4:1F5D; ovl4:1F6B is the operand of the `lcall` at 1F6A (_DrawTile). (fixed: ovl4:1F5D.)
- [x] must-fix src/EOV4.C:675: Ovl4_1091 "Called by DrawViewSlots at ovl4:1E2E". The call is at ovl4:1E50; 1E2E is inside the `mov ax,5` at 1E2C. (fixed: ovl4:1E50.)
- [x] must-fix src/EOV4.C:792: Ovl4_1411 "Called by DrawViewSlots at ovl4:1E2E". The call is at ovl4:1EA2. (Same wrong address as the Ovl4_1091 header; both cannot be right.) (fixed: ovl4:1EA2.)
- [x] must-fix src/EOV4.C:1046: Ovl4_1D20 "Called by Ovl4_1E18 at ovl4:1E2A". The call is at ovl4:1ECB; ovl4:1E2A is `mov si,ax`. (fixed: "Called by DrawViewSlots at ovl4:1ECB".)
- [x] must-fix src/EOV4.C:402-403: Ovl4_0AD0 says "After the list, when that span slot was set, the slot's own span is recomputed". The `ComputeViewSpan(slotReg, ...)` call (:468-469) is inside the `while` loop (the loop closes at :470), so it runs after each item (call at ovl4:0CFB, after the DrawRegion000F at 0CCF). Say it is per item. (fixed: "After each item, when that span slot was set, the slot's own span is recomputed (the second call is inside the loop)".)
- [x] must-fix src/EOV4.C:951-954 and :947: Ovl4_19C4 says the cell +2 test is "the cell +2 modulo 8" and that `<< 3 >> 3` is needed. For a signed char in a 16-bit int, `(c << 3) >> 3` is exactly `c` (for -128..127 the shift stays in int), so the test compares the sign-extended cell byte with g725A[frame]. It is not a modulo 8. Fix the wording: the pair is a codegen form that keeps the value unchanged. (fixed: :947 now "the sign-extended cell +2 byte matches ds:725A[frame]"; the source-form note says the `<< 3 >> 3` leaves a signed char unchanged.)
- [x] must-fix src/EOV4.C:1163-1164: DrawViewFrame says that when `full` is set and gF442 is clear the function "blanks the view box and clips a rectangle". The gF442 branch (:1182-1186) only calls ClipRect. The blank (gFillRect over 0,0,0xb0,0x78 with pattern 0xc) is in the gFB45 branch (:1171-1176), which the comment does not describe. (fixed: the gFB45/full branch now describes the blank, and the gF442 branch says it clips a rectangle.)
- [x] must-fix src/EOV4.C:1045: Ovl4_1D20 says "Column 0 is nudged by -4 on both axes". Only idx 0 (column 0, first of its two cells, :1068-1071) is nudged; idx 1 is not. Say "the first cell (idx 0)". (fixed: "The first cell (idx 0) is nudged by -4 on both axes".)
- [x] must-fix src/EOV4.C:232-233: IsSlotLive's comment says "(the slot is a live map object)". Nothing in the unit or the cited code shows that; ComputeViewSpan reads the same ds:194B bit 3 as "carries ds:6CC6 bounds" (:55, :64, :102), and src/E1414.C:321 and src/EOV3.C:1010 read it with other meanings. Keep the name only with the evidence it has, or state what the function tests: "returns 1 when slot is nonzero and ds:194B bit 3 is clear". (fixed: the comment now states the test and the callers' use, and cites the other meanings; the "live map object" claim is gone.)

Should: clearer, or a name that differs from the rest of `src/`.

- [x] should src/EOV4.C:1-4: the file header says "The unit is still being lifted." The unit is complete (make match 18 of 18); drop the sentence. (fixed: sentence dropped.)
- [x] should src/EOV4.C:21-22 and :59: InitViewSlots "reset the slot to -1 in both halves of the view-slot array" and gViewSlots "(two words per slot)". The code (:32) sets all 36 words (words i and i+18 for i < 18), i.e. all 18 pairs; say that. (fixed: :21-22 now says all 36 words are reset (word i and word i + 18 for i < 18, both words of all 18 pairs); :59 now calls gViewSlots 36 words = 18 {left, right} pairs.)
- [x] should src/EOV4.C:416 and :468: `spanSlot` is set only inside the drawn-item branch (:437-440 region, `spanSlot = -1` at :437). For a first item that is skipped, the per-item check at :468 reads an indeterminate value; for later skipped items it reads the previous item's value. The original reads the same stack slot (the bytes match). Say so in the Ovl4_0AD0 comment. (fixed: added QUIRK ovl4:0AD0 describing exactly this and that the original reads the same stack slot.)
- [x] should src/EOV4.C:397-401: Ovl4_0AD0's "bit 3 of the flags also recomputes the span ... and pulls the object up 4 pixels" omits the condition at :451 (`sprite < 2 && frame != 0`), and the gate at :423-428 (slot 0x10 forces the flags to 0x80; the return needs a nonzero ds:1879 count and bit 7 clear) is only described as "gate the slot". (fixed: the header now gives the return condition and the `sprite < 2 && frame != 0` and slot 0x10 conditions.)
- [x] should src/EOV4.C:670-676: Ovl4_1091 calls the face at ds:7018[slot*2] "the second face (slot*2)", but it is processed second (sideA at :715-716) while ds:7018[slot*2+1] is listed first in the same comment and processed first (:693). Name the faces by their index: "the face at slot*2 (drawn second)". (fixed: the header lists ds:7018[slot*2+1] drawn first and ds:7018[slot*2] drawn second, and calls the flag reader "the face at slot*2 (drawn second)".)
- [x] should src/EOV4.C:789-790 (Ovl4_1411 header): "nudged by ds:718E/ds:7192 when the heading is below 0x1e". The code uses g718E (with the g714E base) below 0x1e and g7192 with no heading term at or above it (:831-834). Say which table each case uses. (fixed: the header now names ds:718E (with the ds:714E base) below 0x1e and ds:7192 with no heading term at or above, per case group.)
- [x] should src/EOV4.C:788-790: for actions 4..6 the comment says "then redraws region 5 at the column extent". The code fits region 5 once (:852, with ds:7106/ds:7108 and gF1/gF5); there is no column-extent call in 4..6. The final SetRegionRect (:907) fits the visible span for every case. (fixed: 4..6 now says it fits region 5 once (ovl4:0852) and the final SetRegionRect (ovl4:19A0) fits the visible span for every case.)
- [x] should src/EOV4.C:1086 (DrawViewSlots header) and :40, :66, :1090, :1130: the comments name functions by placeholder names that no longer exist in this file: "Ovl4_0097" (ComputeViewSpan, :40 and :1086), "Ovl4_1E18" (DrawViewSlots, :66, :1046), "Ovl4_1F51" (DrawViewFrame, :1090, :1130). Use the current names. (fixed: all replaced by ComputeViewSpan, DrawViewSlots and DrawViewFrame; no placeholder names remain in comments.)
- [x] should src/EOV4.C:1137-1139: DrawViewRows "hides the pointer over the view box". The zone it hides (HidePointerZone 0x46, 0x79, 0xb0, 0xaf, :1138) is x 70 to 176, y 121 to 175, which is below the view box (DrawViewFrame hides 0,0,0xb0,0x78 for the box). Say "below the view box". (fixed: "hides the pointer below the view box (0x46, 0x79, 0xb0, 0xaf)".)
- [x] should src/EOV4.C:761-762: gF1 cites "(src/EOV5.C)" and gF5 "(this unit)". src/E0EF3.C:395-397 names both as "the view region's top/height (src/EOV4.C)", and src/EOV5.C:222 also declares gF1. Use one owner and make the citations agree. (fixed: both now read "the view region's top/height (src/E0EF3.C:395/:397)".)
- [x] should src/EOV4.C:13 and :156 (g1879): src/EOV3.C:60 names ds:1879 `gTable1879` (`unsigned char`, "the four parallel per-kind tables"). Rename to gTable1879 (`make rename`) and agree the signedness (here it is signed char). g1879 is also declared twice in this file (:13 and :156); the same for g194B (:12, :230), gA9C6 (:11, :1083), HidePointerZone and ShowPointerZone (:1124-1125, :1154-1155). Remove the second copies. (fixed: changed to gTable1879 with make rename (ds:1879 in re/globals.toml and re/renames.toml); the duplicate externs removed. Signed char stays: the cbw at ovl4:02C9 sign-extends the load, which an unsigned char would not. EOV3.C's `unsigned char` is the side that disagrees; that file's owner must reconcile it.)
- [x] should src/EOV4.C:1122: gFarA92A names ds:A92A, which src/EOV2.C:122 names `gIconTableA92A` (same type, `void far *[]`). Rename to match. (fixed: renamed ds:A92A -> gIconTableA92A with make rename.)
- [x] should src/EOV4.C:11 and :1083 (gA9C6): cites "(src/E0ACD.C)", but src/E0ACD.C:78 says "an offset into a gB894 record (src/E0DE8.C)", src/E122E.C:100 says "the member row the mouse is over", and src/E1414.C:101 says "an offset added into gEF6A's targets" (the meaning used here). Cite src/E1414.C:101 and note that the other files disagree. (fixed: the declaration now reads "an offset added into gEF6A's targets (src/E1414.C:101; src/E0ACD.C:78, src/E122E.C:99 differ)"; the duplicate at :1083 was removed.)
- [x] should src/EOV4.C:748-760 and :759 (gFB5E): this file declares `struct SlotSize far *gFB5E[]` (the code dereferences `->width`, so the far pointer type is the one the bytes need). src/EOV2.C:155 and src/EOV3.C:143 declare `long gFB5E[]`. Tell those owners; keep the far-pointer type here. (fixed: the far-pointer type stays; the comment now cites src/E0EF3.C:031A and says src/EOV2.C:155 and src/EOV3.C:143 declare it long, which the `->width` use contradicts. Those two files' owners are told here.)
- [x] should src/EOV4.C:526-527: g1CD3 and g1CE3 have no comment. They are the two colour rows the colour callback is passed (src/E08E6.C:103-104, :719, :729-731). Add that, with the citation. (fixed: both now carry a comment citing src/E08E6.C:110-111 and the callback calls at :750/:752.)
- [x] should src/EOV4.C:915: DDRec m06 "low two bits index ds:7246 and ds:7262". ds:7262 is indexed by `m07 * 4 + m06` with the whole m06 (:1007, :1016), and the g7262 comment (:931) says "per record +6". Say that ds:7246 uses the low two bits and ds:7262 the whole byte. (fixed: "its low two bits index ds:7246; ds:7262 is indexed by the whole byte (m07 * 4 + m06)".)

Notes: for the next reader; they do not block.

- [ ] note src/EOV4.C:10 (gEF6A signedness): this file declares `signed char far *gEF6A[]`; src/E1414.C:100 declares `unsigned char`. The signed read is backed by src/E08E6.C:532 (`(signed char)gB894[...]` before indexing g194B). Also gB894 is `signed char` here, `unsigned char` in src/E08E6.C:85 and `char` in src/E0ACD.C:76.
- [ ] note src/EOV4.C:525 and :1153 (function-pointer types): gFn80D2 is `void (far *)(char far *map)` here, `void far *colors` in src/E07A9.C:249; gFillRect's fifth parameter is "pattern" here and in src/E0BD2.C:113 but "colour" in src/E082C.C:64. Same bytes; pick one set of names.
- [ ] note src/EOV4.C:119-128 (struct Region): the same layout is declared in src/E082C.C:24, src/E16A5.C:16, src/E1A29.C:20, src/E1DE7.C:21 and src/EOV2.C:22 with different owner citations. The struct in this file cites src/E1DE7.C; fine, but the file should be the one place the struct is described.
- [x] note src/EOV4.C:44 (SetRegionRect): height is scaled by 8 but width (right - left) is not; the header says left and right are in 8-pixel columns, so width is in columns too. Say so. (fixed: the SetRegionRect header now says the width, right - left, is in columns too and is not scaled.)
- [ ] note src/EOV4.C:584 and :572 (Ovl4_0D1B): `page5` holds the g7001 base (`gFBCC * 5`) and is then reused as the loop counter. The reuse matches the bytes; say why in a comment or leave it.
- [ ] note src/EOV4.C:603-605 and :582-583 (Ovl4_0D1B): the `0x12` sprite nudge for `obj->m07` and the `0xfe`/`0xff` overrides of `m0A` (:593-598) have no comment; their meaning is unknown, so "Purpose unknown" is enough.
- [ ] note src/EOV4.C:1046 (Ovl4_1D20): the header says "the 0x34 object" (the heading value DrawViewSlots tests at :1114). That is a description of the value, not a name with evidence; fine as written.
- [ ] note src/EOV4.C:1033 (cross-file, owner EOV2): src/EOV2.C:116 says "src/EOV4.C:955 indexes the table" and src/EOV2.C:155 says "src/EOV4.C:756"; both lines are stale (now 1072 and 759). The owner of src/EOV2.C should cite the addresses instead (ovl4:1D20 and ovl4:0000 region of the SlotSize use). Not a must-fix here.
- [ ] note src/EOV4.C:1192-1199 (NoOp2039): the body has no observable effect; the name says so. Fine as it is.

## Summary

The unit is understandable from its source: the names that carry evidence (ComputeViewSpan, SetRegionRect, CopyViewColumn, ToggleViewColumn, InitViewSlots, DrawViewFrame, DrawViewRows) match what the code does, and the struct layouts agree with the offsets the bytes use. Most of the named placeholders (Ovl4_044B, Ovl4_0AD0, Ovl4_1091, Ovl4_1411, Ovl4_19C4, Ovl4_1D20, Ovl4_0D1B) are still "Purpose unknown" with no evidence beyond the call graph, which is a fine reason to keep them. The problems are in the comments: several call-site addresses are wrong (checked against the disassembly), some behaviour is misdescribed (the ComputeViewSpan adjustment rule and its QUIRK note, the Ovl4_0AD0 per-item recompute, the DrawViewFrame blank, the Ovl4_1D20 nudge, the Ovl4_19C4 `modulo 8`), and two names need evidence or a rename (IsSlotLive's "live" claim, and the `g1879`/`gFarA92A` names that src/ already gives other names). Nothing escalated: no matcher, spec or pin question.
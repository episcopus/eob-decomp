# Review: eov5

Reviewer: rev-haiku
Commit reviewed: 5767eb38ee1512e5c9994792c9e8fc61ed181448 (round 1), 5fadbc39b1563e907468dc17cae60b2c3d78d625 (round 2, main)

Playbook 70. Judge what a matching file can still get wrong: names, types, comments, structure.
The matcher already proved the bytes; do not re-prove them, read for meaning.

## Findings

Each finding is a checkbox. `must-fix` blocks `make review-pass` until a decompiler ticks it off
(with the commit that fixes it); `should` and `note` do not block.

Evidence used: the code in `src/EOV5.C` (line numbers below); the call lists in
`build/packets/eov5.md`; other files of `src/` (`EOV0.C`, `EOV2.C`, `EOV3.C`, `E08E6.C`,
`E0D1C.C`, `E0EF3.C`, `E1414.C`, `EOV4.C`, `E122E.C`); and the string literals of the pinned
`eob/EOB.EXE`, read only (`strings`, `grep -b`). The EXE literals line up with the ds addresses
(file-offset differences equal ds-address differences, within one byte, e.g. ds:7B9F "CANCEL" to
ds:7D9E "Which should be resurrected?").

### Round 2 (2026-11-02, reviewed at 5fadbc3: the round 1 fixes of 3eefda8 are on main)

Judged as playbook 70 "Later rounds" asks: each round 1 must-fix, checked again against the current code, comments and the EXE, and every changed line of `src/EOV5.C`, the `eov5` entries of `re/symbols.toml` and the changed lines of `re/globals.toml` since 5767eb3 (`git diff 5767eb38 main`). The round 1 items are not read again in full.

Round 1 must-fix: all 20 are fixed right (ticks stay). Evidence: the eight dispatcher panels and the include (:2-3, :11); DrawCharPortrait's mode table and tables (:281-317); SelectMenu's callers, each of which calls it (:557, :612, :656, :660, :693, :702, :875, :903, :905, :935, :963, :994, :1020), while 0AB1, 0B16 and 1087 do not; the gFn8082 call (:594); the QueueItem list keyed by gPartyCell with gFBCC for the sound (:639-640); ItemPanelDispatch (:450, :762); the ResurrectPanel remap (:906-907); bits 0x2000000 (:1009) and 0x4000000 (:1031, default return :1029); the ShuffleGrid save and restore (:1400, :1402); fOvl3_07A5 (:1302, :1342) against src/EOV3.C:469-477; TileAnimation's origin (:624); EndingAnimation's draws (:1195-1196); the WinnerPanel callers (:1215; src/E08E6.C:814). Menu labels: the far-pointer tables in `eob/EOB.EXE`, read by following the pointers (ds:7B23 yes/no; ds:7BA6, 7BB2, 7BBA, 7BC2, 7BCA, 7BD2, 7BDE, 7BE6, 7BF2, 7BFA). The file names ds:7E43 to ds:7F38 match the EXE strings. The password text: ds:7EDB is "\r", ds:7EDD is "\r\rPassword: %04x\r".

#### Wrong claims (must-fix)

- [ ] must-fix src/EOV5.C:603-604 and :779 (AddCharPanel, BuildDeathList): "what m0B means is not established, 0D1C:0276". 0D1C:0276 is inside RemoveCarriedItems (0D1C:01CA, 274 bytes, `re/symbols.toml`), not at the statement. The statement is RemoveCarriedItems's header, src/E0D1C.C:285 ("What m02, m0B and m04 mean is not established"), which is at 0D1C:01CA. The round 1 line cite src/E0D1C.C:276 is now a `return 0;` of f0D1C_0163 (0D1C:0163). Cite 0D1C:01CA.

- [ ] must-fix src/EOV5.C:857-859 (ResurrectPicker): "so the fixed 20 is only safe because that many rarely qualify (the dead members plus a few kind-8 items)". Nothing in src/ or the EXE says how often the count is large, so this safety argument has no evidence. The bound is right (6 + 6*25 = 156, :797 and :802-809). Say what is established: the match fixes names at 20 entries; nothing in the code caps the count at 20; whether the original's data can exceed 20 is not established.

- [ ] must-fix src/EOV5.C:680 (GiveItemPanel) and :984 (SurrenderPanel): "queues ... on the current page" and "queues the item on the current page list". The list is keyed by gPartyCell (`InsertListItem(&gB899[gPartyCell * 9], gPartyCell, ...)`, :640 and :1002-1003); the current page gFBCC only picks the sound (:639, :1003). This is the wording round 1 removed from QueueItem (:632-633). Say the gPartyCell list with a sound from gFBCC (GiveItemPanel can say "through QueueItem").

#### Clarity and names (should)

- [ ] should src/EOV5.C:336-337 (the key-code comment before the enum): "UpdateOptionList's (src/E14EF.C:222)" is a line cite into another file, and line 222 is the `enum {` of E14EF's key table. Cite the address: UpdateOptionList is 14EF:0274 (src/E14EF.C:8). "0x5c and 0x66 are SelectMenu's own left/right (prev/next) keys": "left/right" is not evidenced. The code names them KEY_PREV (:503) and KEY_NEXT (:508), and src/E14EF.C:218-236 gives 0x5B, 0x5D, 0x65 and 0x67 as first and last keys without naming 0x5C or 0x66. Say prev and next.

- [ ] should src/EOV5.C:1403: a stray blank line before the closing brace of ShuffleGrid's inner loop (added with the round 1 fix). Delete it; no code changes.

- [ ] should re/globals.toml, ds:FBCA (gPartyCell; changed since 5767eb3): "(src/E17A6.ASM:1466)" is a line in the middle of another function (`add si, 3`). Distance is at 17A6:0F61 (src/E17A6.ASM:1489-1532). The claim is right: E17A6.ASM:1490-1491 says each argument is a packed y * 32 + x cell with x in the low five bits. Cite 17A6:0F61 by address. This is another unit's file; the owner of E17A6 and `re/globals.toml` takes it.

#### Notes for the next reader

- [ ] note src/EOV5.C:1227 (CharOverlay header): "loads a wall texture set for the current level". The call (:1302) passes a NULL name and the sheet gMapSheet + (gMapTiles[gFBC8 - 1] << 5). "Current level" is inferred from the gMapTiles comment (:162); src/EOV3.C:469-477 says only that a NULL name reuses gWallNameBuf. Keep it if the comment says what the sheet offset selects.

- [ ] note the other units' names in `re/globals.toml` changed in this range (gSoundMap, gMemTopSeg, gSaveRectSize, gFatalHandler, gRetryHandler, gFont8x8, gTable1879, gIconTableA92A, gStatusLine) are other units' claims. I read the gPartyCell line (above) and gGraphicsChoice (ds:8102: its dispatch is in DrawTile, 17A6:03E7, E17A6.ASM:251-255, which holds). The rest are left to their owners.

- [ ] note the round 1 note on gPartyCell (src/E122E.C:197) is still open; it is outside this unit.

#### Verified (no finding)

- MessagePanel: the two rectangles and the gFE42 mapping (:369-379; callers :1126, :1239, :1324); the dismiss keys (:392-403); the MORE prompt ds:7CB7 in the EXE.
- DrawCharPortrait: each mode's cells (:281-315) against its header.
- ResurrectPicker: a negative entry -(k+1) and RemoveCarriedItems(0x21, k+1) (:877-880) against its header.
- BuildDeathList: m09 low 7 bits (:807) and the stored -m0B (:809).
- CallOverlayOp (src/E122E.C:1412) calls fOvl5_1A82 (stub 211E:0057, src/E122E.C:190), as the header's first line says.
- KEY_CODE_MASK 0x8FF is bits 0 to 7 and bit 11 (:339).
- The thirteen gFileName* renames in `re/globals.toml` (ds:7E43 to ds:7F38).
- `re/symbols.toml`: the eov5 names match the code (ItemPanelDispatch at ovl5:0B58).

#### Summary (round 2)

All 20 round 1 must-fix items are fixed right. The changed lines hold three must-fix items: a wrong address (0D1C:0276), an unsupported safety claim in ResurrectPicker, and the "current page" wording repeated in two changed functions. Fix those and the should items, then the unit can pass on the next round. Not escalated: no matcher, spec or pin question.

### Round 1: wrong claims (must-fix)

- [x] must-fix src/EOV5.C:2-3: the unit comment says 1A82 dispatches modes 0..7 "to the seven panels", then lists eight addresses (0924, 0B58, 0EB9, 0F77, 102C, 1087, 10A6, 119E). Eight modes, eight panels (CharOverlay cases, lines 1262-1287). [fixed 3eefda8: seven -> eight panels, named the caller CallOverlayOp (122E:1D76), dropped the inventory key]
- [x] must-fix src/EOV5.C:11: the include comment says `abs` is "the inline __abs__ the select lists use". No select list calls `abs` (no `abs(` call in this file; the only mention is the comment at line 852). Either the include is needed for a stated matching reason, which the comment gives, or it goes. [fixed 3eefda8: removed the stdlib.h include; nothing calls abs and the match holds without it]
- [x] must-fix src/EOV5.C:267-272 (DrawCharPortrait): the comment calls ds:7B77, ds:7B7D and ds:7B83 "per-character cell-index tables" and says modes 1 and 2 draw "the current character's own cells". The third table is not a cell index: line 298 passes `feet[gEFC6]` as the y argument of DrawSheetCell (x = 0x74, y = feet). Mode 1 draws the fixed cells 0 and 5 (lines 286-289) and never indexes by gEFC6. The fixed-cell list (2, 7, 6, 0x12, 0x11, 4) omits 0, 3 and 5, which the code also draws. [fixed 3eefda8: ds:7B77/ds:7B7D are cell index tables, ds:7B83 a y offset; the fixed cells are 2,0,5,3,7,6,0x12,0x11,4]
- [x] must-fix src/EOV5.C:275-277 (DrawCharPortrait locals): `head`, `body`, `feet` name body parts with no evidence, and `feet` is a y coordinate table (see above). A confident name without evidence is a must-fix: name them by what the code does (cell index, y offset), or keep a placeholder with `Purpose unknown:`. [fixed 3eefda8: head/body/feet -> cellIdxA/cellIdxB/yOff]
- [x] must-fix src/EOV5.C:436-445 (SelectMenu): "Called by every menu panel ... (067B, 0828, 0924, 09C5, 0AB1, 0B16, 0EB9, 0F77, 102C, 1087, 10A6, 119E)" is wrong both ways. 0AB1 (CheckChar0F7), 0B16 (CheckRemove6) and 1087 (QuitPanel) do not call SelectMenu (their call lists in `build/packets/eov5.md` have no 0455). The real caller ResurrectPicker (0DBB, line 868) is missing from the list. [fixed 3eefda8: callers are 067B,0828,0924,09C5,0DBB,0EB9,0F77,102C,10A6,119E]
- [x] must-fix src/EOV5.C:398 (PrintTableString): "The strings are the overlay's own yes/no menu labels." They are message texts: PrintTableString is called with message numbers (8 at line 646, 3 at 684, 0x1c at 734, ...), and the EXE holds them as messages (for example "You may only have six characters in your party. Select the one you wish to drop.", the ChooseChar title at ds:7CBC, and "I wish you luck in your travels."). The yes/no labels are the SelectMenu tables, not these strings. [fixed 3eefda8: the indexed strings are the overlay's message texts (TEXT.DAT); the labels are the ds:7B tables]
- [x] must-fix src/EOV5.C:561 and 587 (AddPartyMember): the comment says the cell comes from "gGetSpriteCell(which*4, 0xa0, 4, 0x20, gOverlayBuf)", but the code calls `gFn8082` (ds:8082, line 587). ds:8082 and ds:8086 are different far pointers (lines 170-171; `gFn8082` in src/EOV0.C:16 and src/EOV3.C:37, `gGetSpriteCell` in src/EOV2.C:175 and src/EOV3.C:165). Name the callee the code calls. [fixed 3eefda8: the comment now names gFn8082 (ds:8082), not gGetSpriteCell]
- [x] must-fix src/EOV5.C:624 (QueueItem): "for the current sound page (gPartyCell)". gPartyCell is the party's map cell (src/E08E6.C:94: object ids are compared with it; src/E1414.C:99; src/EOV4.C:524). The sound page is gFBCC (src/EOV4.C:132: "the current page 0..3"; line 631 indexes g1A1B with gFBCC * 4). Say the list is keyed by gPartyCell and the sound comes from gFBCC. See the note below on the conflict in src/E122E.C. [fixed 3eefda8: list keyed by gPartyCell (the map cell), sound from gFBCC]
- [x] must-fix src/EOV5.C:744 (Menu1): "if none of them did anything it draws 'Leave' and the message 025F". Line 760 draws gText7D77, and in EOB.EXE that address holds the farewell line "I wish you luck in your travels." (between "%s joins the party." at ds:7D0E and "Which should be resurrected?" at ds:7D9E, by file offset). It is not a "Leave" label; "Leave" is a menu entry in the 7BA6 to 7BFA tables. [fixed 3eefda8: draws the farewell gText7D77 and the modal box; renamed Menu1 -> ItemPanelDispatch]
- [x] must-fix src/EOV5.C:636 (TemplePanel): ds:7BB2 is labelled ("Heal Party"/"Leave"). In the EXE literals, in source order, the two-entry menu after the three-entry ds:7BA6 menu ("Tend his wounds", "Talk", "Leave") is "Tend his wounds"/"Leave"; "Heal Party"/"Leave" is ds:7BCA, the two-entry ResurrectPanel menu (line 897, after the "Which should be resurrected?" title). Choice 0 of the 7BB2 menu runs AddCharPanel (join), which fits "Tend his wounds". [fixed 3eefda8: ds:7BB2 is Tend his wounds/Leave; ds:7BA6 is Tend his wounds/Talk/Leave (EXE pointer tables)]
- [x] must-fix src/EOV5.C:883 (ResurrectPanel): "Choice 0 runs 0C9D and choice 1 runs 0DBB". In the two-entry branch (no death list, ds:7BCA) choice 1 is mapped to 2 (lines 898-899) and runs nothing. Only the three-entry menu ds:7BD2 reaches ResurrectPicker (lines 906-908). [fixed 3eefda8: choice 1 is remapped to 2 and does nothing in the two-entry branch; only ds:7BD2 reaches the picker]
- [x] must-fix src/EOV5.C:670-672 (GiveItemPanel): "clears gFC04 bit 0x8000" on the second pass. Bit 0x8000 is set at line 683 and no path clears it (lines 697-703). Also "choice 1 ... the panel is left": after choice 1 the code draws message 5 (line 705) and returns; say that. [fixed 3eefda8: no clear of 0x8000; choice 1 draws message 5 and returns; whole -> showSecond]
- [x] must-fix src/EOV5.C:976 (SurrenderPanel): "Always sets gFC04 bit 0x20000". Line 1001 sets 0x2000000. Bit 0x20000 is ResurrectPanel's (line 904). [fixed 3eefda8: 0x20000 -> 0x2000000]
- [x] must-fix src/EOV5.C:1005 (FreeHimPanel): "Always sets gFC04 bit 0x40000". Line 1022 sets 0x4000000, and the default case (choice 2) returns at line 1020 before it is set, so it is not "always" either. [fixed 3eefda8: 0x40000 -> 0x4000000, and the default returns before it]
- [x] must-fix src/EOV5.C:1341-1342 (ShuffleGrid): "draws the cell ... through the far pointers ds:8072 and ds:809E". src/E0EF3.C:389-391 documents ds:8072 as "saves the pixels under (x, y), returns a handle" and ds:809E as "puts a saved handle back". The function saves and restores pixels under positions; it does not draw a cell. [fixed 3eefda8: ds:8072 saves the pixels, ds:809E restores them]
- [x] must-fix src/EOV5.C:1215 (CharOverlay): "copies a byte table into overlay 3". The call at line 1291 is `fOvl3_07A5(0, gMapSheet + ...)`, which is LoadWallTextures (ovl3:07A5): it loads a wall texture set for a name (src/EOV3.C:469-477). [fixed 3eefda8: fOvl3_07A5 is LoadWallTextures, which loads a texture set]
- [x] must-fix src/EOV5.C:1305 (UseItemPanel): "frees the wall textures into gOverlayBuf". Line 1330 calls `fOvl3_07A5(0, gOverlayBuf)`, which loads the textures (src/EOV3.C:469); nothing is freed there. [fixed 3eefda8: fOvl3_07A5 loads the wall textures; nothing is freed]
- [x] must-fix src/EOV5.C:1028 (TileAnimation): "shuffles the 0x18 x 0x90 grid". The call at line 1062 is `ShuffleGrid(0x18, 0, 0x90, 0x68, 2, 0)`: origin (0x18, 0), 0x90 columns, 0x68 rows. [fixed 3eefda8: ShuffleGrid origin (0x18,0), 0x90 columns, 0x68 rows]
- [x] must-fix src/EOV5.C:1150 (EndingAnimation): "four from the sliced cell (sound 0x2a)". The four draws at lines 1182-1184 are `sprites[i - 6]`, the 2x2 grid of 0xE x 0x58 cells made at line 1167. The sliced cell is `cell` (line 1169), drawn by the explosion loop at line 1195. [fixed 3eefda8: the last four frames draw sprites[i-6], the 2x2 grid; the sliced cell draws in the explosion loop]
- [x] must-fix src/EOV5.C:1101 (WinnerPanel): "Called from the menu dispatcher 1A82 (mode 0xb)". CharOverlay has no case 0xb (lines 1235-1245, 1262-1287). The callers are EndingAnimation (line 1203) and 08E6:0EC3 Obj0EC3 (packet). [fixed 3eefda8: callers are EndingAnimation (17C8) and 08E6:0EC3 Obj0EC3]
### Round 1: clarity and names (should)

- [x] should src/EOV5.C:2: "the character/item overlay the map loop opens on the 'inventory' key": the key is not shown in this unit. The only "inventory" in `src/` is src/E0ACD.C:969 ("map and inventory (LoadMapScreen, ovl3:0A7C)"). Name the caller (122E:1D76, CallOverlayOp) and the key, or drop the key. [fixed 3eefda8: named CallOverlayOp (122E:1D76) and dropped the key]
- [x] should src/EOV5.C:5: "draws the item grid through 0455": 0455 draws labelled rows of a menu (`names[]`, DrawMenuRow). Call it a menu. [fixed 3eefda8: called a menu]
- [x] should src/EOV5.C:29: two fields on one line (`unsigned char flags; ... char name[0x0B];`). Split them. [fixed 3eefda8: split flags and name onto two lines]
- [x] should src/EOV5.C:34: the comment cites `src/E0D1C.C:0415`, a line of another file that goes stale. Cite the address of that code. [fixed 3eefda8: cite 0D1C:0415]
- [x] should src/EOV5.C:243: "the avatar panel 00A1": "avatar" is not evidenced; 00A1 is the character portrait (DrawCharPortrait). [fixed 3eefda8: named DrawCharPortrait]
- [x] should src/EOV5.C:255-258 (DrawSheetCell): `c1`..`c4` are placeholders by role. They are the four bytes of a gCellDef7B2B entry (x, y, w, h). `struct Cell4` (line 14) describes exactly that and is unused here; use it or name the four values. [fixed 3eefda8: c1..c4 -> cx,cy,cw,ch, the four gCellDef7B2B bytes]
- [x] should src/EOV5.C:157: `int (far *gFn8072)(int a, int b)`: single-letter parameter names. Use `x, y` as src/E0EF3.C:389 does. [fixed 3eefda8: a,b -> x,y]
- [x] should src/EOV5.C:375-382 (MessagePanel): raw key codes 0x2B, 0x34, 0x3D, 0x41, 0x42 and mask 0x8FF. The enum at lines 424-434 names some of them but not 0x34. Use named constants (READABILITY rule 4) and name the mask. [fixed 3eefda8: the enum now names the mask and 0x34; the switch uses the constants]
- [x] should src/EOV5.C:334-335 (MessagePanel): "the two player windows" has no evidence. gFE42 is 0 in CharOverlay (line 1228) and 1 in UseItemPanel (1312) and WinnerPanel (1115), so it selects the small or the wide message box. Say that, or make gFE42 `Purpose unknown:`. [fixed 3eefda8: comment says 0 selects the CharOverlay box and 1 the UseItemPanel/WinnerPanel box]
- [x] should src/EOV5.C:334-339: "draws ds:7CB7" is the "MORE" prompt in EOB.EXE. Name the string. [fixed 3eefda8: named the 'MORE' prompt]
- [x] should src/EOV5.C:595 (AddCharPanel): "removes the item class 0x21 from character who+1". src/E0D1C.C:276 says what m02 and m0B mean is not established; RemoveCarriedItems matches m02 == class and m0B == fieldB. Say what the code relies on: the carried items of class 0x21 whose m0B is who+1 (the pool index plus one, as ResurrectPicker reads it). [fixed 3eefda8: carried items of type 0x21 with m0B == who+1]
- [x] should src/EOV5.C:593: the label "join" for choice 0 of ds:7B23 is not in the EXE strings I checked (the two entries at ds:7B23 did not show). Check the two labels before naming the choice. [fixed 3eefda8: the ds:7B23 entries are yes/no]
- [x] should src/EOV5.C:598 (AddCharPanel): `head` and `other` name the message numbers for choice 0 and choice 1; `head` is also a DrawCharPortrait local with another meaning. Name them by role (join message, other message). [fixed 3eefda8: head/other -> yesMsg/noMsg]
- [x] should src/EOV5.C:115 and 604: ds:7B23 is a menu table of far string pointers, declared `char gText7B23[]` and cast to `void far *` at each use. Declare it as the pointer table it is. [fixed 3eefda8: declared char far *gText7B23[] and dropped the casts]
- [x] should src/EOV5.C:709-710 and 730: CheckChar0F7 and CheckRemove6 say "menu strings"; they draw messages through PrintTableString with the modal box (flag 1). [fixed 3eefda8: messages, not menu strings]
- [x] should src/EOV5.C:677 and 670-671 (GiveItemPanel): the local `whole` is 1 when the second menu (ds:7BC2) is shown and 0 when it is skipped. The name and "records that the whole party is to be used" do not say that. Name it for what it decides. [fixed 3eefda8: whole -> showSecond]
- [x] should src/EOV5.C:916-917 (AttackPanel): "the 'attack / bribe / ...' NPC panel": the menu is exactly "Attack"/"Bribe" (EOB.EXE, ds:7BDE); drop the "...". "clears the type-8 objects to 5" is "changes type 8 to 5" (lines 933-934). [fixed 3eefda8: the menu is Attack/Bribe; changes every type-8 object to 5]
- [x] should src/EOV5.C:966-967 (QuitPanel): "the 'quit' menu entry": the panel has no menu. It draws message 0x15 and sets a bit. Say that. [fixed 3eefda8: the panel has no menu]
- [x] should src/EOV5.C:746 (Menu1): a placeholder-style name for a function whose comment says it dispatches three panels. Name it for that dispatch, or keep the placeholder and write `Purpose unknown:` in the comment. [fixed 3eefda8: renamed Menu1 -> ItemPanelDispatch]
- [x] should src/EOV5.C:1029-1030 (TileAnimation): "(or the last cell when the step is 0)": the else branch draws `extra[0]`, the 8 x 0x4d cell (line 1080). Say that. The names `barsTop`, `barsBot` and `figures` are guesses (strips and cells, from sizes only); name them by size and y offset, or mark `Purpose unknown:`. [fixed 3eefda8: barsTop/barsBot/figures -> stripA/stripB/block, named by size and slice offset]
- [x] should src/EOV5.C:1039: `void far *extra[11] = { 0 }` but only `extra[0]` is used. Declare one pointer. [fixed 3eefda8: tried one pointer; it shrinks the frame (0x44 vs 0x6c) and breaks the match, so the array stays with a comment]
- [x] should src/EOV5.C:1097: "all six party bits of gFC04 (gFC04 & 0x1FFE == 0x1FFE)": 0x1FFE is twelve bits (1..12). Say which bits are the party bits. [fixed 3eefda8: gFC04 bits 1..12]
- [x] should src/EOV5.C:1097-1101 (WinnerPanel): "the password text" is an inference. The file read is TEXT.DAT (see the file names below), and the format is "Password: %04x" at ds:7EDD (EOB.EXE). Say the checksum is formatted with that string. [fixed 3eefda8: TEXT.DAT and the Password: %04x format]
- [x] should src/EOV5.C:124-150, 1045-1052, 1165-1170, 1246-1258, 1317-1332 (file names): gText7E43, gText7E4B, gText7E53, gText7EEF, gText7EF7, gText7EFF, gText7F07, gText7F0F, gText7F18, gText7F20, gText7F27, gText7F30 and gText7F38 are file names (passed to FileOpen, LoadScreenImage or LoadOverlayScreen; EOB.EXE has PORTALA, PORTALB and TEXT.DAT among them). The `gText` prefix names them as text. Rename with `make rename` (the file-name prefix, e.g. gFileName...). [fixed 3eefda8: renamed to gFileName* with make rename, logged in re/renames.toml]
- [x] should src/EOV5.C:773-806 (BuildDeathList): the list gets `-gTable8DD2[item].m0B` for kind-8 items, and ResurrectPicker reads a negative entry as pool index k = -n - 1. What m0B is is not established (src/E0D1C.C:276). The comment should say that the pool-list reading is an inference from ResurrectPicker and the RemoveCarriedItems(0x21, who + 1) calls. [fixed 3eefda8: the pool-index reading is marked an inference; m0B stays unestablished]
- [x] should src/EOV5.C:855 (ResurrectPicker): `names[20]` holds gCountDDE0 entries, but BuildDeathList can add up to 6 + 6 * 25 = 156 entries. State the bound, or say why 20 is safe for the original. [fixed 3eefda8: stated the 6 + 6*25 = 156 bound of BuildDeathList]
- [x] should src/EOV5.C:1344-1347 (ShuffleGrid), 769-772 (BuildDeathList), 851-852 (ResurrectPicker), 1216-1220 (CharOverlay): these describe the matcher and compiler (register locals, `cmatch`, far-object fixups, `abs` code shape). Move them to the notes or the whitepaper and keep the game meaning in the function comment. [fixed 3eefda8: moved the matcher notes out of CharOverlay, ShuffleGrid, BuildDeathList and ResurrectPicker]
- [x] should src/EOV5.C:1347-1392 (ShuffleGrid): `swapped` holds the handle `gFn8072` returns (line 1390) and is also the Fisher-Yates temp (lines 1369-1371); give the handle its own name (for example `handle`). `cy` is the shuffle index and the y coordinate in the draw loop; name it for one role. `attr1` and `attr2` are the gStatusLine values; name them for that. Renaming may change register allocation: `make match` decides whether it is possible. [fixed 3eefda8: handle/idx/saveStatus/restoreStatus; a second local breaks the frame, so one reused slot stays with a comment]
- [x] should src/EOV5.C:112-114: the externs `gTable7B77`, `gTable7B7D` and `gTable7B83` are shadowed by DrawCharPortrait's locals and unused. Remove them or say why they stay. [fixed 3eefda8: removed the unused externs]

### Round 1: notes for the next reader

- [ ] note src/E122E.C:197 calls gPartyCell "the global sound volume" (and writes it at line 213), while src/E08E6.C:94, src/E1414.C:99 and src/EOV4.C:524 call it the party's map cell. The files disagree. This unit follows E08E6 (its uses compare object ids with it). This is outside this unit; the human should decide the name.
- [ ] note `make readability UNIT=eov5` reports 0 findings. Its checks are mechanical; the findings above are not.
- [ ] note the names `gFillRect`, `gFn804A`, `gFn807E`, `gFn8072`, `gFn809E`, `gFn8082` and `gGetSpriteCell` agree with the other files that declare the same addresses (src/E082C.C:64, src/E0ACD.C:46, src/E0EF3.C:373, 389-391, src/EOV0.C:16, src/EOV2.C:175). The packet lists `x`/`x1` for some of them: those are placeholder names in other files, and the names here are the better ones.

### Round 1: verified (no finding)

- The three menu labels "kill her / hear her out / let her go" (ds:7BE6), "free him / kill him / leave" (ds:7BFA) and "surrender / attack" (ds:7BF2) are in EOB.EXE in the order the code uses.
- ds:7B9F "CANCEL", ds:7D0E "%s joins the party.", ds:7CBC (the ChooseChar title) and ds:7D9E "Which should be resurrected?" are at the EOB.EXE file offsets that their ds addresses predict, and are used as the comments say. The WinnerPanel strings ("Password: %04x" and the winner title) are in EOB.EXE; their ds addresses are not checked.
- ClipRect's parameters (width, height at positions 5 and 6) match src/E1DE7.C:281 and src/EOV6.C:405, and every call here passes them in that order.
- CharOverlay's eight cases in source order 0,1,2,3,4,6,5,7 are as the comment says, and the cs:1CCD mapping note is consistent with the case order.

## Summary (round 1)

The unit is understandable from its source: the panel and menu names mostly follow the menu labels in EOB.EXE, the struct fields that matter are named, and the code is clear. But twenty comments state wrong bit values, callers, labels, table meanings or call targets (the must-fix items above: the bit values in SurrenderPanel and FreeHimPanel, the GiveItemPanel clear, the EndingAnimation sprite source, the LoadWallTextures calls, the ShuffleGrid save/restore, the SelectMenu caller list). Those are comment and naming errors, not code errors, and each has a fix that does not move a byte. Placeholder names that stay until evidence names them: `Menu1`, the `head`, `body` and `feet` locals, `barsTop`, `barsBot`, `figures`, `whole`, `swapped`, `cy`, and the `gText7xxx` file-name globals (renamed in the should items). The unit passes after the must-fix items are ticked and the changed lines are reread; this is round 1.

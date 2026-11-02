# Review: eov5

Reviewer: rev-haiku
Commit reviewed: 5767eb38ee1512e5c9994792c9e8fc61ed181448

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

### Wrong claims (must-fix)

- [ ] must-fix src/EOV5.C:2-3: the unit comment says 1A82 dispatches modes 0..7 "to the seven panels", then lists eight addresses (0924, 0B58, 0EB9, 0F77, 102C, 1087, 10A6, 119E). Eight modes, eight panels (CharOverlay cases, lines 1262-1287).
- [ ] must-fix src/EOV5.C:11: the include comment says `abs` is "the inline __abs__ the select lists use". No select list calls `abs` (no `abs(` call in this file; the only mention is the comment at line 852). Either the include is needed for a stated matching reason, which the comment gives, or it goes.
- [ ] must-fix src/EOV5.C:267-272 (DrawCharPortrait): the comment calls ds:7B77, ds:7B7D and ds:7B83 "per-character cell-index tables" and says modes 1 and 2 draw "the current character's own cells". The third table is not a cell index: line 298 passes `feet[gEFC6]` as the y argument of DrawSheetCell (x = 0x74, y = feet). Mode 1 draws the fixed cells 0 and 5 (lines 286-289) and never indexes by gEFC6. The fixed-cell list (2, 7, 6, 0x12, 0x11, 4) omits 0, 3 and 5, which the code also draws.
- [ ] must-fix src/EOV5.C:275-277 (DrawCharPortrait locals): `head`, `body`, `feet` name body parts with no evidence, and `feet` is a y coordinate table (see above). A confident name without evidence is a must-fix: name them by what the code does (cell index, y offset), or keep a placeholder with `Purpose unknown:`.
- [ ] must-fix src/EOV5.C:436-445 (SelectMenu): "Called by every menu panel ... (067B, 0828, 0924, 09C5, 0AB1, 0B16, 0EB9, 0F77, 102C, 1087, 10A6, 119E)" is wrong both ways. 0AB1 (CheckChar0F7), 0B16 (CheckRemove6) and 1087 (QuitPanel) do not call SelectMenu (their call lists in `build/packets/eov5.md` have no 0455). The real caller ResurrectPicker (0DBB, line 868) is missing from the list.
- [ ] must-fix src/EOV5.C:398 (PrintTableString): "The strings are the overlay's own yes/no menu labels." They are message texts: PrintTableString is called with message numbers (8 at line 646, 3 at 684, 0x1c at 734, ...), and the EXE holds them as messages (for example "You may only have six characters in your party. Select the one you wish to drop.", the ChooseChar title at ds:7CBC, and "I wish you luck in your travels."). The yes/no labels are the SelectMenu tables, not these strings.
- [ ] must-fix src/EOV5.C:561 and 587 (AddPartyMember): the comment says the cell comes from "gGetSpriteCell(which*4, 0xa0, 4, 0x20, gOverlayBuf)", but the code calls `gFn8082` (ds:8082, line 587). ds:8082 and ds:8086 are different far pointers (lines 170-171; `gFn8082` in src/EOV0.C:16 and src/EOV3.C:37, `gGetSpriteCell` in src/EOV2.C:175 and src/EOV3.C:165). Name the callee the code calls.
- [ ] must-fix src/EOV5.C:624 (QueueItem): "for the current sound page (gPartyCell)". gPartyCell is the party's map cell (src/E08E6.C:94: object ids are compared with it; src/E1414.C:99; src/EOV4.C:524). The sound page is gFBCC (src/EOV4.C:132: "the current page 0..3"; line 631 indexes g1A1B with gFBCC * 4). Say the list is keyed by gPartyCell and the sound comes from gFBCC. See the note below on the conflict in src/E122E.C.
- [ ] must-fix src/EOV5.C:744 (Menu1): "if none of them did anything it draws 'Leave' and the message 025F". Line 760 draws gText7D77, and in EOB.EXE that address holds the farewell line "I wish you luck in your travels." (between "%s joins the party." at ds:7D0E and "Which should be resurrected?" at ds:7D9E, by file offset). It is not a "Leave" label; "Leave" is a menu entry in the 7BA6 to 7BFA tables.
- [ ] must-fix src/EOV5.C:636 (TemplePanel): ds:7BB2 is labelled ("Heal Party"/"Leave"). In the EXE literals, in source order, the two-entry menu after the three-entry ds:7BA6 menu ("Tend his wounds", "Talk", "Leave") is "Tend his wounds"/"Leave"; "Heal Party"/"Leave" is ds:7BCA, the two-entry ResurrectPanel menu (line 897, after the "Which should be resurrected?" title). Choice 0 of the 7BB2 menu runs AddCharPanel (join), which fits "Tend his wounds".
- [ ] must-fix src/EOV5.C:883 (ResurrectPanel): "Choice 0 runs 0C9D and choice 1 runs 0DBB". In the two-entry branch (no death list, ds:7BCA) choice 1 is mapped to 2 (lines 898-899) and runs nothing. Only the three-entry menu ds:7BD2 reaches ResurrectPicker (lines 906-908).
- [ ] must-fix src/EOV5.C:670-672 (GiveItemPanel): "clears gFC04 bit 0x8000" on the second pass. Bit 0x8000 is set at line 683 and no path clears it (lines 697-703). Also "choice 1 ... the panel is left": after choice 1 the code draws message 5 (line 705) and returns; say that.
- [ ] must-fix src/EOV5.C:976 (SurrenderPanel): "Always sets gFC04 bit 0x20000". Line 1001 sets 0x2000000. Bit 0x20000 is ResurrectPanel's (line 904).
- [ ] must-fix src/EOV5.C:1005 (FreeHimPanel): "Always sets gFC04 bit 0x40000". Line 1022 sets 0x4000000, and the default case (choice 2) returns at line 1020 before it is set, so it is not "always" either.
- [ ] must-fix src/EOV5.C:1341-1342 (ShuffleGrid): "draws the cell ... through the far pointers ds:8072 and ds:809E". src/E0EF3.C:389-391 documents ds:8072 as "saves the pixels under (x, y), returns a handle" and ds:809E as "puts a saved handle back". The function saves and restores pixels under positions; it does not draw a cell.
- [ ] must-fix src/EOV5.C:1215 (CharOverlay): "copies a byte table into overlay 3". The call at line 1291 is `fOvl3_07A5(0, gMapSheet + ...)`, which is LoadWallTextures (ovl3:07A5): it loads a wall texture set for a name (src/EOV3.C:469-477).
- [ ] must-fix src/EOV5.C:1305 (UseItemPanel): "frees the wall textures into gOverlayBuf". Line 1330 calls `fOvl3_07A5(0, gOverlayBuf)`, which loads the textures (src/EOV3.C:469); nothing is freed there.
- [ ] must-fix src/EOV5.C:1028 (TileAnimation): "shuffles the 0x18 x 0x90 grid". The call at line 1062 is `ShuffleGrid(0x18, 0, 0x90, 0x68, 2, 0)`: origin (0x18, 0), 0x90 columns, 0x68 rows.
- [ ] must-fix src/EOV5.C:1150 (EndingAnimation): "four from the sliced cell (sound 0x2a)". The four draws at lines 1182-1184 are `sprites[i - 6]`, the 2x2 grid of 0xE x 0x58 cells made at line 1167. The sliced cell is `cell` (line 1169), drawn by the explosion loop at line 1195.
- [ ] must-fix src/EOV5.C:1101 (WinnerPanel): "Called from the menu dispatcher 1A82 (mode 0xb)". CharOverlay has no case 0xb (lines 1235-1245, 1262-1287). The callers are EndingAnimation (line 1203) and 08E6:0EC3 Obj0EC3 (packet).
### Clarity and names (should)

- [ ] should src/EOV5.C:2: "the character/item overlay the map loop opens on the 'inventory' key": the key is not shown in this unit. The only "inventory" in `src/` is src/E0ACD.C:969 ("map and inventory (LoadMapScreen, ovl3:0A7C)"). Name the caller (122E:1D76, CallOverlayOp) and the key, or drop the key.
- [ ] should src/EOV5.C:5: "draws the item grid through 0455": 0455 draws labelled rows of a menu (`names[]`, DrawMenuRow). Call it a menu.
- [ ] should src/EOV5.C:29: two fields on one line (`unsigned char flags; ... char name[0x0B];`). Split them.
- [ ] should src/EOV5.C:34: the comment cites `src/E0D1C.C:0415`, a line of another file that goes stale. Cite the address of that code.
- [ ] should src/EOV5.C:243: "the avatar panel 00A1": "avatar" is not evidenced; 00A1 is the character portrait (DrawCharPortrait).
- [ ] should src/EOV5.C:255-258 (DrawSheetCell): `c1`..`c4` are placeholders by role. They are the four bytes of a gCellDef7B2B entry (x, y, w, h). `struct Cell4` (line 14) describes exactly that and is unused here; use it or name the four values.
- [ ] should src/EOV5.C:157: `int (far *gFn8072)(int a, int b)`: single-letter parameter names. Use `x, y` as src/E0EF3.C:389 does.
- [ ] should src/EOV5.C:375-382 (MessagePanel): raw key codes 0x2B, 0x34, 0x3D, 0x41, 0x42 and mask 0x8FF. The enum at lines 424-434 names some of them but not 0x34. Use named constants (READABILITY rule 4) and name the mask.
- [ ] should src/EOV5.C:334-335 (MessagePanel): "the two player windows" has no evidence. gFE42 is 0 in CharOverlay (line 1228) and 1 in UseItemPanel (1312) and WinnerPanel (1115), so it selects the small or the wide message box. Say that, or make gFE42 `Purpose unknown:`.
- [ ] should src/EOV5.C:334-339: "draws ds:7CB7" is the "MORE" prompt in EOB.EXE. Name the string.
- [ ] should src/EOV5.C:595 (AddCharPanel): "removes the item class 0x21 from character who+1". src/E0D1C.C:276 says what m02 and m0B mean is not established; RemoveCarriedItems matches m02 == class and m0B == fieldB. Say what the code relies on: the carried items of class 0x21 whose m0B is who+1 (the pool index plus one, as ResurrectPicker reads it).
- [ ] should src/EOV5.C:593: the label "join" for choice 0 of ds:7B23 is not in the EXE strings I checked (the two entries at ds:7B23 did not show). Check the two labels before naming the choice.
- [ ] should src/EOV5.C:598 (AddCharPanel): `head` and `other` name the message numbers for choice 0 and choice 1; `head` is also a DrawCharPortrait local with another meaning. Name them by role (join message, other message).
- [ ] should src/EOV5.C:115 and 604: ds:7B23 is a menu table of far string pointers, declared `char gText7B23[]` and cast to `void far *` at each use. Declare it as the pointer table it is.
- [ ] should src/EOV5.C:709-710 and 730: CheckChar0F7 and CheckRemove6 say "menu strings"; they draw messages through PrintTableString with the modal box (flag 1).
- [ ] should src/EOV5.C:677 and 670-671 (GiveItemPanel): the local `whole` is 1 when the second menu (ds:7BC2) is shown and 0 when it is skipped. The name and "records that the whole party is to be used" do not say that. Name it for what it decides.
- [ ] should src/EOV5.C:916-917 (AttackPanel): "the 'attack / bribe / ...' NPC panel": the menu is exactly "Attack"/"Bribe" (EOB.EXE, ds:7BDE); drop the "...". "clears the type-8 objects to 5" is "changes type 8 to 5" (lines 933-934).
- [ ] should src/EOV5.C:966-967 (QuitPanel): "the 'quit' menu entry": the panel has no menu. It draws message 0x15 and sets a bit. Say that.
- [ ] should src/EOV5.C:746 (Menu1): a placeholder-style name for a function whose comment says it dispatches three panels. Name it for that dispatch, or keep the placeholder and write `Purpose unknown:` in the comment.
- [ ] should src/EOV5.C:1029-1030 (TileAnimation): "(or the last cell when the step is 0)": the else branch draws `extra[0]`, the 8 x 0x4d cell (line 1080). Say that. The names `barsTop`, `barsBot` and `figures` are guesses (strips and cells, from sizes only); name them by size and y offset, or mark `Purpose unknown:`.
- [ ] should src/EOV5.C:1039: `void far *extra[11] = { 0 }` but only `extra[0]` is used. Declare one pointer.
- [ ] should src/EOV5.C:1097: "all six party bits of gFC04 (gFC04 & 0x1FFE == 0x1FFE)": 0x1FFE is twelve bits (1..12). Say which bits are the party bits.
- [ ] should src/EOV5.C:1097-1101 (WinnerPanel): "the password text" is an inference. The file read is TEXT.DAT (see the file names below), and the format is "Password: %04x" at ds:7EDD (EOB.EXE). Say the checksum is formatted with that string.
- [ ] should src/EOV5.C:124-150, 1045-1052, 1165-1170, 1246-1258, 1317-1332 (file names): gText7E43, gText7E4B, gText7E53, gText7EEF, gText7EF7, gText7EFF, gText7F07, gText7F0F, gText7F18, gText7F20, gText7F27, gText7F30 and gText7F38 are file names (passed to FileOpen, LoadScreenImage or LoadOverlayScreen; EOB.EXE has PORTALA, PORTALB and TEXT.DAT among them). The `gText` prefix names them as text. Rename with `make rename` (the file-name prefix, e.g. gFileName...).
- [ ] should src/EOV5.C:773-806 (BuildDeathList): the list gets `-gTable8DD2[item].m0B` for kind-8 items, and ResurrectPicker reads a negative entry as pool index k = -n - 1. What m0B is is not established (src/E0D1C.C:276). The comment should say that the pool-list reading is an inference from ResurrectPicker and the RemoveCarriedItems(0x21, who + 1) calls.
- [ ] should src/EOV5.C:855 (ResurrectPicker): `names[20]` holds gCountDDE0 entries, but BuildDeathList can add up to 6 + 6 * 25 = 156 entries. State the bound, or say why 20 is safe for the original.
- [ ] should src/EOV5.C:1344-1347 (ShuffleGrid), 769-772 (BuildDeathList), 851-852 (ResurrectPicker), 1216-1220 (CharOverlay): these describe the matcher and compiler (register locals, `cmatch`, far-object fixups, `abs` code shape). Move them to the notes or the whitepaper and keep the game meaning in the function comment.
- [ ] should src/EOV5.C:1347-1392 (ShuffleGrid): `swapped` holds the handle `gFn8072` returns (line 1390) and is also the Fisher-Yates temp (lines 1369-1371); give the handle its own name (for example `handle`). `cy` is the shuffle index and the y coordinate in the draw loop; name it for one role. `attr1` and `attr2` are the gStatusLine values; name them for that. Renaming may change register allocation: `make match` decides whether it is possible.
- [ ] should src/EOV5.C:112-114: the externs `gTable7B77`, `gTable7B7D` and `gTable7B83` are shadowed by DrawCharPortrait's locals and unused. Remove them or say why they stay.

### Notes for the next reader

- [ ] note src/E122E.C:197 calls gPartyCell "the global sound volume" (and writes it at line 213), while src/E08E6.C:94, src/E1414.C:99 and src/EOV4.C:524 call it the party's map cell. The files disagree. This unit follows E08E6 (its uses compare object ids with it). This is outside this unit; the human should decide the name.
- [ ] note `make readability UNIT=eov5` reports 0 findings. Its checks are mechanical; the findings above are not.
- [ ] note the names `gFillRect`, `gFn804A`, `gFn807E`, `gFn8072`, `gFn809E`, `gFn8082` and `gGetSpriteCell` agree with the other files that declare the same addresses (src/E082C.C:64, src/E0ACD.C:46, src/E0EF3.C:373, 389-391, src/EOV0.C:16, src/EOV2.C:175). The packet lists `x`/`x1` for some of them: those are placeholder names in other files, and the names here are the better ones.

### Verified (no finding)

- The three menu labels "kill her / hear her out / let her go" (ds:7BE6), "free him / kill him / leave" (ds:7BFA) and "surrender / attack" (ds:7BF2) are in EOB.EXE in the order the code uses.
- ds:7B9F "CANCEL", ds:7D0E "%s joins the party.", ds:7CBC (the ChooseChar title) and ds:7D9E "Which should be resurrected?" are at the EOB.EXE file offsets that their ds addresses predict, and are used as the comments say. The WinnerPanel strings ("Password: %04x" and the winner title) are in EOB.EXE; their ds addresses are not checked.
- ClipRect's parameters (width, height at positions 5 and 6) match src/E1DE7.C:281 and src/EOV6.C:405, and every call here passes them in that order.
- CharOverlay's eight cases in source order 0,1,2,3,4,6,5,7 are as the comment says, and the cs:1CCD mapping note is consistent with the case order.

## Summary

The unit is understandable from its source: the panel and menu names mostly follow the menu labels in EOB.EXE, the struct fields that matter are named, and the code is clear. But twenty comments state wrong bit values, callers, labels, table meanings or call targets (the must-fix items above: the bit values in SurrenderPanel and FreeHimPanel, the GiveItemPanel clear, the EndingAnimation sprite source, the LoadWallTextures calls, the ShuffleGrid save/restore, the SelectMenu caller list). Those are comment and naming errors, not code errors, and each has a fix that does not move a byte. Placeholder names that stay until evidence names them: `Menu1`, the `head`, `body` and `feet` locals, `barsTop`, `barsBot`, `figures`, `whole`, `swapped`, `cy`, and the `gText7xxx` file-name globals (renamed in the should items). The unit passes after the must-fix items are ticked and the changed lines are reread; this is round 1.

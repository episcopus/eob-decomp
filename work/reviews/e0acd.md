# Review: e0acd

Reviewer: p6-rev-9
Commit reviewed: ce0608f367ed01b1b1df84e254eef04d1a734b27

Playbook 70. Judge what a matching file can still get wrong: names, types, comments, structure.
The matcher already proved the bytes; do not re-prove them, read for meaning.

## Findings

Each finding is a checkbox. `must-fix` blocks `make review-pass` until a decompiler ticks it off
(with the commit that fixes it); `should` and `note` do not block.

Line numbers are src/E0ACD.C unless another file is named. Callee addresses are the ones the
comments should cite.

## Round P6 (2026-11-04), commit ce0608f367ed01b1b1df84e254eef04d1a734b27

Later round (playbook 70). Judged the names and comments the Phase 6 diff changed since 7ff86a7 in `src/E0ACD.C`, against `re/renames.toml` and `re/fields.toml`. The diff is a large name and comment rewrite: the 35 action handlers and the globals they use take Phase 6 names, the private `Entry14` and `gTable8DD2` views give way to `gItems`, and the comments follow. `make match FILE=src/E0ACD.C` 35 of 35 (4182 bytes); `make readability UNIT=e0acd` 0 findings.

The names are backed: `MoveForward` (0ACD:04F8), `MoveBackward` (0529), `TurnLeft` (056C), `TurnRight` (0593), `MoveLeft` (05BA), `MoveRight` (05FF), `OpenInventory` (0034), `SelectPrevMember` (008C), `SelectNextMember` (00EB), `ToggleCharacterInfo` (018E), `ClickInventorySlot` (01C0), `RestorePortraitPanel` (01E3), `PickOrSwapMember` (0264), `CancelMemberSwap` (0393), `ClickHandSlot` (0415), `ThrowMouseItem` (0494), `ClickFloorItem` (0644), `FeedInventoryMember` (08D1), `ActivateWallAhead` (08E7), `MoveHandCursor` (091C), `MoveKeyCursor` (0A34), `HandleMapKey` (0D9D) and `PollMapInput` (0E6C), and the globals `gMatchedNodeKey` (ds:FC82), `gPickViewPos` (ds:EFB2), `gPickFirstItem` (ds:F3BA), `gViewPosAhead` (ds:374A), `gViewToCellPos` (ds:1A1B), `gGameLoadState` (ds:A998), `gMouseItem` (ds:FBCE), `gFont6x6` (ds:A9C0), `gViewFrontSide` (ds:A9C6), `gMapPhase` (ds:DCAA), `gHandSplitY` (ds:36B0), `gLeftButtonMode` (ds:5E1C), `gSlotNeighbours` (ds:36D0), `gFrontPosByDir` (ds:373C), `gMemberSide` (ds:3744), `gDirectAction` (ds:F542), `gNextRandomSoundTick` (ds:FC5A), `gUnreadPartyCellCopy` (ds:FC58), `gGameTicks` (ds:8492), `gUnreadButtonChanges` (ds:8134), `gCgaMapPlayfld` (2069:0000), `gCgaMapInvent` (2067:0000), `gWallTrigger` (ds:EFD2) and `gRestoreRectToPage` (ds:804A). Each `why` in `re/renames.toml` matches the code at the address.

The comments the diff rewrote are correct where they make new claims: `OpenInventory` names the portrait callback, `MoveBlocked` names the movement keys, the six movement handlers say what they do, `RestorePortraitPanel` and `PickOrSwapMember` use the gStagingPage buffer, `ActivateWallAhead` names the wall handler, `HandleMapKey` records that gMapPhase 5 never occurs, and `RunMapLoop` names the random-sound cell. What remains is old wording that the diff did not touch, now contradictory with the new names. It is noted below, not a must-fix: the previous review accepted it, and the later-round rule says text no fix touched is a note.

Should (clarity; not blocking):

- [ ] should src/E0ACD.C:183 (OpenInventory): the first sentence is new and right, but the second says "the action mode gMatchedNodeKey". Its why (ds:FC82) says the value is the matched node key (1 the first key word, 2 the second, 0 a mouse hit), not a mode. Say "when gMatchedNodeKey is nonzero".
- [ ] should src/E0ACD.C:402 (ThrowMouseItem): "the 'use item on the selected character' action" is the old label; its why says the two view nodes launch the mouse item through AddDDEffect. Say it throws the mouse item.
- [ ] should src/E0ACD.C:77 (gViewFrontSide): "an offset into a gMaze record" is vague; its why says it is `(gPartyDir + 2) & 3`, the wall side of the visible cell that faces the party.
- [ ] should src/E0ACD.C:131 (gGameTicks): the comment cites `src/EOV6.C`, but its why says the INT 1Ch handler is at E1D8F.ASM 1D8F:006E.
- [ ] should src/E0ACD.C:636 (MoveHandCursor): "the 'turn' action" is the old label; its why says it moves the keyboard cursor over the portraits and "does not turn the party".
- [ ] should src/E0ACD.C:696 (MoveKeyCursor): "the row/character turn dispatcher" is the old label; its why says it moves the keyboard cursor over the hands (mode 0) or the inventory slots (mode 1).

Notes (for the next reader):

- [ ] note src/E0ACD.C:28 (gMatchedNodeKey): "an action mode" is old text (the old gFC82 comment); the name now says matched node key, and re/renames.toml ds:FC82 gives 1/2 for the matched key word and 0 for a mouse hit.
- [ ] note src/E0ACD.C:75 (gMaze): "nine-byte object records" is old text; re/renames.toml ds:B894 says one nine-byte record per map cell (walls, monster count at +4, item list at +5).
- [ ] note src/E0ACD.C:76 (gWallTypeFlags): "per-object bit flags" is old text; re/renames.toml ds:194B says flags per wall type, and 0ACD:0644 tests bits 0, 1 and 3 (`& 0xB`).
- [ ] note src/E0ACD.C:106 (gHandSplitY): "a per-slot word" is old text; re/renames.toml ds:36B0 says six words, one per party member, the y split of the two hands.
- [ ] note src/E0ACD.C:113 (gFrontPosByDir): "two entries per page" is old text; the index is gPartyDir * 2 at 0ACD:0A61, and re/renames.toml ds:373C says two per facing.
- [ ] note src/E0ACD.C:134-135 (gCgaMapPlayfld): "the PLAYFLD sheet" is old text; re/renames.toml 2069:0000 says a 32-byte 16 to 4 colour map passed as LoadScreenImage's `extra` argument, not a pixel sheet.
- [ ] note src/E0ACD.C:136-137 (gCgaMapInvent): "the INVENT sheet" is old text; re/renames.toml 2067:0000 says a 32-byte 16 to 4 colour map, "never a pixel buffer".
- [ ] note src/E0ACD.C:955 (EnterMapScreen): "LoadScreenImage (into gCgaMapPlayfld and gCgaMapInvent)" is old text with the new names; the two are the `extra` colour-map argument (lines 972-973), not the destination (page 5, srcKind 5).
- [ ] note src/E0ACD.C:983 (ResetMapScreen): "the page gPartyDir" is old text; gPartyDir is the party's facing 0..3 (re/renames.toml ds:FBCC), not a page.
- [ ] note src/E0ACD.C:201 and :222 (SelectPrevMember, SelectNextMember): the comments still say "the action mode" for gMatchedNodeKey; the lines were not changed by the diff.
- [ ] note src/E0ACD.C:505-512 (ClickFloorItem): "The per-page, per-slot kind" and "then RunCellScript" are old text; the code indexes slotKind[gPartyDir][slot] (537) and runs RunCellScript(cell, 8) (541) before SetMouseItem(removed) (542).
- [ ] note src/E0ACD.C:623 (ActivateWallAhead): "Derives a key ... passes the key" is old text; StepCell returns a cell and DispatchWallHandler runs on the cell ahead (re/renames.toml 0ACD:08E7).
- [ ] note src/E0ACD.C:74 (gViewDirty): "when an action took effect" is old text; re/renames.toml ds:DF4C says whenever something visible changes, and RunMapLoop redraws while it is set.
- [ ] note src/E0ACD.C:129 (gNextRandomSoundTick): "the map timeout counter" is old text; re/renames.toml ds:FC5A says the next tick for the random sound 0x1E on levels 1..3.
- [ ] note src/E0ACD.C:130 (gUnreadPartyCellCopy): "the party's cell saved by the map loop" does not say no code reads it; re/renames.toml ds:FC58 says RunMapLoop copies gPartyCell into it and no file in src/ reads it.
- [ ] note src/E0ACD.C:132 (gUnreadButtonChanges): "set to 0 by the map screen" is old text; re/renames.toml ds:8134 says the INT 33h handler ORs the changed button bits into it and no code reads it.
- [ ] note the `re/renames.toml` rows for gCgaMapPlayfld and gCgaMapInvent say kind = "function" although both are data segments; the source declares them as arrays and the matcher accepts them. A registry metadata point for the tool owner, not a source fix.
- [ ] note the Phase 6 names are used in the other units: StepCell and MemberIs in E0BD2.C and E122E.C, CanPlaceItemInSlot in E0BD2.C, RunCellScript in E1414.C and E122E.C, LoadLevel and NoOpAfterMapLoad in the overlay units. No old name remains in src/ for the checked functions and globals.

Round P6 verdict: review passed. No must-fix is open. The Phase 6 names are backed by re/renames.toml and re/fields.toml, the changed comments that make new claims are right, and the old wording that contradicts them is noted for the next reader.

Evidence: `make match FILE=src/E0ACD.C` 35 of 35 (4182 of 4182 bytes); `make readability UNIT=e0acd` 0 findings. Nothing escalated: no matcher, spec or pin question.

## Round 3 (at 7ff86a7)

Later round (playbook 70). Judged: each round 2 must-fix (both ticks stay: the ActRec comment and the gIconTable comment are now right in substance, checked against src/E0EF3.C:292, E0ACD.C:18 and src/E1414.C:107; the stale line cite in the gIconTable tick is a should below). The changes since 6003a3d in src/E0ACD.C, re/symbols.toml and re/globals.toml: the DrawMemberLevel to DrawMemberFood rename (1547:04A0; src/E1547.C:323 to 324 describes the food bar, and src/E1414.C:130 and :677 and src/E1547.C:560 already use the name; re/renames.toml:2282 logs it), the EatFood call, the m97, m0B, gHitY, g5E1C, DispatchKeyWait, Action0E6C, EnterMapScreen and PickListItem comments. Claims checked against the code: m97 (RemoveListItem at :758, InsertListItem at :780 and :851), gA998 (src/EOV2.C:418, 425, 427: 0xff, 0, 0xfe), the PickListItem loop (`while (flag == 0 && count < 5)`, zero item only clears gF3BA, a fresh item sets flag). `make match FILE=src/E0ACD.C`: 35 of 35 (4182 bytes), exit 0. `make readability UNIT=e0acd`: 0 findings. No wrong claim in a changed line. Only stale line cites into other files, which are shoulds.

- [ ] should src/E0ACD.C:49 (gIconTable): "src/E1414.C:106 agrees" cites the wrong line now. E1414.C:106 is the g5CAB declaration; the gIconTable declaration is E1414.C:107 (its text agrees). Cite the file without a line, or the reader's address (E1414's comment cites QUIRK 1414:0037 for the save and restore).
- [ ] should src/E0ACD.C:136 (g5E1C): "src/E16A5.C:506 tests g5E1C for the left button" cites a line that is now `if ((flags & 2) && (state & NODE_STATE_ON))`. The test is src/E16A5.C:513 (`leftFlags != 0 && g5E1C == 3`), with leftFlags set at :343. Cite :513 or the walk address 16A5:00BD.
- [ ] should src/E0ACD.C:174 (the struct Node comment): "(src/E16A5.C:185 to 188 and :523 to 524)". Lines 185 to 188 are DispatchKeyWait's call and its closing brace, not a mouse-only return. The mouse-only zero is src/E16A5.C:174 to 175 (`key = 0` unless a key is pending), and `if (result == 0) result = key;` is now at :530 to 531 (was :523 to 524). Cite those lines.
- [ ] note src/E0ACD.C:606 to 607 (EatFood, 0ACD:07E0): "redraws the icon" is loose. The call that changed (line 629, DrawMemberFood) redraws the member's food bar (src/E1547.C:323 to 324), and line 628 clears the mouse item with SetMouseItem(0). Name the food bar and the cleared item. Not a changed line, so a note.
- [ ] note the PickListItem comment (0ACD:0BA6) has a short line "volume's list. It sets gDF4C and stops. A" that breaks the paragraph raggedly. Reflow when next touched; no content change.
- [ ] note for the other owners (not this unit's to edit): src/E1547.C:323 cites "EatFood (0ACD:07E0, src/E0ACD.C:577)", and EatFood is at src/E0ACD.C:611 (the address 0ACD:07E0 is right); src/E1414.C:108 cites "src/E0ACD.C:42" for gIconTable, which is at src/E0ACD.C:48. Round 2's note on src/E16A5.C:189 to 192 ("both callers ignore the value") still stands for the E16A5 owner.

Round 3 verdict: review passed. No must-fix is open: both round 2 must-fix items are fixed right, and the changed lines are sound. The shoulds are stale line cites; the unit's comments and names hold.

## Round 2 (at 6003a3d)

Later round (playbook 70). Judged: each round 1 must-fix (all ticked; each fix re-read against the code, and all are fixed right in substance, so the ticks stay); the changes since 3648a5c in src/E0ACD.C (the e6c03df comment, type and name edits, and the build flag line `-B` to `-v`). `make match FILE=src/E0ACD.C`: 35 of 35 (4182 bytes), exit 0. `make readability UNIT=e0acd`: 0 findings.

- [x] must-fix src/E0ACD.C:97 (the `struct ActRec;` comment): "the same layout as struct Action above" is wrong. The two records agree at the +4 result word, but the +0x38 member is a signed char in ActRec (src/E0EF3.C:292, `signed char m38`) and an int in struct Action (src/E0ACD.C:18, `int slot`). Say that instead, for example "the same +4 result word and +0x38 member, a byte in ActRec (src/E0EF3.C:292) and a word here (:18)".
  Fixed: the comment now reads "the same +4 result word and +0x38 member, a byte in ActRec (src/E0EF3.C:292) and a word here (:18)".
- [x] must-fix src/E0ACD.C:49 (gIconTable): "src/E1414.C:101 names the byte 'the character the blink highlights'" is no longer true. E1414.C:101 is now the `gFB78` declaration. The gIconTable declaration is at src/E1414.C:106 and reads "the index of the party member being moved, -1 ...", which is what this unit says (the e1414 round fixed it). Cite E1414.C:106 as agreeing, and keep only the disagreement with src/E0EF3.C:227 (`void far *gIconTable[]`, still at that line).
  Fixed: the comment now cites src/E1414.C:106 as agreeing ("the index of the party member being moved, -1 when none") and keeps only the src/E0EF3.C:227 disagreement.

- [x] should src/E0ACD.C:129, :166, :182 and :183: stale line citations into other files. E16A5.C:107 is now `gDraw8066`; gHitY is at src/E16A5.C:125 (the same "the y the hit test uses"). E16A5.C:144 is now part of DispatchKey; DispatchKeyWait is at src/E16A5.C:166. EOV4.C:1049 and :1076 are now DrawViewFrame at src/EOV4.C:1047 and NoOp2039 at :1074. Cite the addresses instead (ds:5E12, 16A5:005A, ovl4:1F51, ovl4:2039), which do not move.
  Fixed: gHitY's comment now names the walk (16A5:00BD, src/E16A5.C), the DispatchKeyWait note cites 16A5:005A without a line, and DrawViewFrame and NoOp2039 cite ovl4:1F51 and ovl4:2039 with only the file name.
- [x] should src/E0ACD.C:167 and :919 (DispatchKeyWait, Action0E6C): "the event it consumed, or 0" leaves out a case. DispatchNodeEvent returns the key a node took (`result = keyCode`), or when no node takes it the key itself (`if (result == 0) result = key;`, src/E16A5.C:523 to 524), or 0 for a mouse-only call (src/E16A5.C:186 to 188). Name the cases or cite that comment.
  Fixed: the DispatchKeyWait note now names the key a node took, the key itself when no node takes it (`if (result == 0) result = key;`) and 0 for a mouse-only call (src/E16A5.C:185 to 188 and :523 to 524); Action0E6C now says it passes DispatchNodeEvent's result (16A5:00BD).
- [x] should src/E0ACD.C:109 (CharRec m97): "browsed by RemoveListItem (0BD2:02B9) and InsertListItem (0BD2:01D0)". Those two calls remove an item from and insert an item into the sub-table (0A61 and 0BA6). Neither browses it. Say what they do.
  Fixed: the m97 comment now says RemoveListItem takes an item from it and InsertListItem puts one back (0ACD:0A61 and 0BA6).
- [x] should src/E0ACD.C:115 and :123 (Entry14 m0B): "the food value" is what EatFood (07E0) does with it: it adds m0B to m23 only when m02 is 0x1F or 0x31 (src/E0ACD.C:615). src/E0D1C.C:275 says "What m02, m0B and m04 mean is not established." Say "the amount EatFood adds to m23" and cite that line, so the reader sees the meaning is inferred. The m02 "item type" is supported by the 0x1F and 0x31 test and can stay.
  Fixed: the Entry14 comment and the m0B field now say "the amount EatFood (07E0) adds to m23" and cite src/E0D1C.C:275 for the inference.
- [x] should src/E0ACD.C:785 (PickListItem): "sets the flag" is ambiguous. The local `flag` is what ends the page loop (line 826); gF3BA is set only when it is -1 (lines 827 to 828). Name which one.
  Fixed: it now says the fresh item "sets the local flag (which ends the page loop)".
- [x] should src/E0ACD.C:792 (PickListItem): "a zero item does nothing, both clearing gF3BA" reads as if a zero item does nothing at all. The code (lines 851 to 856) runs nothing for a zero item and sets gF3BA = -1 for both zero and repeated items. Say "a zero item runs nothing but clears gF3BA".
  Fixed: it now reads "a zero item runs nothing, both clearing gF3BA".
- [x] should src/E0ACD.C:958 (EnterMapScreen): "when it is set (0xfe after a saved game loaded, src/EOV2.C)". The test is `!gA998`, so 0xff (a load running, src/EOV2.C:383) takes the same branch. Say "nonzero (0xff while a load runs, 0xfe after a saved game loads)".
  Fixed: it now says "when it is nonzero (0xff while a load runs, 0xfe after a saved game loads, src/EOV2.C)".

- [x] note src/E0ACD.C:132 (g5E1C): "the left-button mode word" is supported by src/E16A5.C:336 to 343 and :506 (leftFlags tested with g5E1C, rightFlags with g5E1E), not by src/E1658.C, which the comment cites. Cite E16A5.C:506 as well.
  Fixed: the g5E1C comment now also cites src/E16A5.C:506 (g5E1C for the left button, g5E1E for the right) alongside src/E1658.C.
- [x] note src/E16A5.C:186 to 188 says "both callers ignore the value" (DispatchKey and DispatchKeyWait). This unit reads that value in 0ACD:0E6C. The E16A5 owner should reconcile it; not a fix for this unit.
  Left for the E16A5 owner: this unit reads DispatchNodeEvent's result in 0ACD:0E6C, so the E16A5.C:186 to 188 comment is out of step. A unit branch cannot change another unit's source.
- [x] note src/EOV2.C:139 cites "src/E0ACD.C:911" for EnterMapScreen, which is now 0ACD:0F72. For the EOV2 owner.
  Left for the EOV2 owner: a unit branch cannot change another unit's source.
- [x] note the PickListItem first sentence (not changed by round 1) says it takes "the item list for the current volume (or, when that list is empty, for the key from f0D1C_00DA)". The code (lines 805 to 809) only tests both lists for emptiness. Not a must-fix: the sentence was accepted in round 1.
  Kept unchanged.
- [x] note the build flag line `/*@flags -ml -O -Z -v */` (was `-B`) is what the matcher now checks against (35 of 35 at `-v`); nothing to fix.
  Nothing to fix.

Round 2 verdict: review failed. Two must-fix are open: the ActRec "same layout" claim (new in round 1) and the gIconTable claim about E1414.C (true at round 1, false now). The round 1 fixes hold in substance. Nothing escalated: no matcher, spec or pin question.

## Round 1 (at 3648a5c)

- [x] must-fix src/E0ACD.C:4: the header says the handlers "turn undead". No handler in this unit turns undead. The only call to TurnUndead (0D1C:0A43, src/E0D1C.C:631) is in RunMapLoop (0ACD:0E8A, line 900). Turn091C (0ACD:091C, lines 603 to 649) is the "turn" action: it moves the selected character (gA99A) and the action row.
  Fixed: the header now lists the member/item/sound actions and says RunMapLoop (0ACD:0E8A) is the unit's only caller of TurnUndead.
- [x] must-fix src/E0ACD.C:19: gF51A is documented as "the selected action row 0..2". This unit compares it with 0x10 in three places: `gF51A != 0x10` (lines 699, 718, 777) and `gF51A == 0x10` (line 725). Row 0x10 selects the character's own item list (0ACD:0A61 and 0ACD:0BA6). src/E122E.C:471 and :485 also test row 0x10. The 0..2 range is only the StepActionRow wrap (lines 347 to 350).
  Fixed: the gF51A comment now names 0..2 for the action menu (wrapped by StepActionRow 0ACD:03CD) and 0x10 for the character's own item list (0ACD:0A61, 0BA6; src/E122E.C:471, :485).
- [x] must-fix src/E0ACD.C:48: gDCAA is documented as "a 1..2 step counter". This unit tests `gDCAA == 5` (the rest phase) at lines 668 (0ACD:0A34), 697 (0ACD:0A61) and 823 (0ACD:0D9D). 0ACD:0753 wraps it to 1 and 2, and EnterMapScreen sets it to 1 (line 922). The range must include the value 5 and say what sets it.
  Fixed: the gDCAA comment now gives 1 and 2 as the menu steps (0ACD:0753, EnterMapScreen 0ACD:0F72) and 5 as the rest phase (0ACD:0A34, 0A61, 0D9D), and records that no writer of 5 was found in the program (checked the EXE for a store of 5 to ds:DCAA).
- [x] must-fix src/E0ACD.C:76: `RedrawPartyPanel(char far *text)` gives the argument a text type, and the RedrawPartyPanel call sites pass gF542 as a message. The definition in src/E0EF3.C:1960 is `int RedrawPartyPanel(struct ActRec far *rec)`. It reads rec->m04 as its result, and it draws no text (its comment at E0EF3.C:1956 says it redraws the party panel). src/E0EF3.C:2091 casts gF542 to `struct ActRec far *`. Take the parameter type from the definition. The same wrong prototype is in src/E122E.C:64 and src/EOV5.C:206 (cross-unit, not ours to fix).
  Fixed: the prototype is now `int RedrawPartyPanel(struct ActRec far *rec)` with struct ActRec forward-declared; the two Action0264 calls pass `(struct ActRec far *)gF542`.
- [x] must-fix src/E0ACD.C:129 and :866: DispatchKeyWait is declared `int DispatchKeyWait(void far *list)`, but src/E16A5.C:144 defines it `void DispatchKeyWait(struct Node far *head)`. Its last statement is DispatchNodeEvent(head, key), which returns "the event it consumed, or 0" (src/E16A5.C:163 to 164). The comment at line 866, "gets the current list node from 16A5:005A(gList)", is wrong. The function waits for one key event, dispatches it to the list head, and what Action0E6C gets back is the event consumed (or 0). Fix the comment and take the prototype from the definition or say why it is int.
  Fixed: the prototype now uses `struct Node far *head`; a comment cites src/E16A5.C:144 and says why this unit declares it int (it consumes the event DispatchNodeEvent leaves in AX). Action0E6C's comment now says it waits for one key event and passes the consumed event to Action0D9D.
- [x] must-fix src/E0ACD.C:85: the m97 comment cites "0D1C:02B9 and 0D1C:01D0". Those two calls are RemoveListItem and InsertListItem at 0BD2:02B9 and 0BD2:01D0 (this file's own externs at lines 126 and 127, and the packet's callers table). Cite the 0BD2 addresses.
  Fixed: the m97 comment now cites RemoveListItem (0BD2:02B9) and InsertListItem (0BD2:01D0).
- [x] must-fix src/E0ACD.C:285 (Action0264, 0ACD:0264): "The gFlag redraws bracket both ends of the swap" is wrong. The leading `if (gFlag) RedrawPartyPanel` (line 295) runs on both paths. The second one (lines 299 and 300) is only in the pick-up branch (gIconTable == -1). The swap branch (lines 306 to 316) has no gFlag redraw after the swap.
  Fixed: the Action0264 comment now says the gFlag-gated RedrawPartyPanel runs before both branches and a second one only in the pick-up branch; the swap branch has none after the swap.
- [x] must-fix src/E0ACD.C:394 (Action04F8, 0ACD:04F8): "plays the sound for the current page ... played through SetSoundVolume" is wrong. SetSoundVolume (src/E122E.C:207 to 222, 122E:0001) switches the sound volume: it sets gPartyCell, runs the driver scripts, sets the page and action dirty flags, and draws the volume message gText5AB5. It does not play a sound. Fix the comment for 04F8 (and check 0529, 05BA, 05FF, whose comments say the same thing).
  Fixed: 04F8 now says the lookup returns a volume that SetSoundVolume (122E:0001) applies; 0529, 05BA and 05FF call it a volume dispatch. Nothing in them plays a sound.
- [x] must-fix src/E0ACD.C:477 and :480 (Action0644, 0ACD:0644): (a) "uses the item in the slot at `action` on the selected character" is wrong. The function never reads gA99A (the selected character). It puts the item in the volume's list (gB899 + vol * 9, lines 510 and 517) and runs the script on vol. (b) "bits 0..3 of g194B" is wrong: the mask at line 504 is `& 0xB`, which is bits 0, 1 and 3 (bit 2 is not tested).
  Fixed: the comment now says the action moves the slot's item into or out of the volume's list and never reads gA99A, and names the mask as bits 0, 1 and 3 (`& 0xB`).
- [x] must-fix src/E0ACD.C:682, :688 and :740 (MoveListItem 0ACD:0A61, PickListItem 0ACD:0BA6): "redraws gA99A through ComputeArmourClass" is not evidenced. src/E0DE8.C:665 to 671 says ComputeArmourClass computes the armour class m1D of character `who`; it does not mention a redraw. Say what the call does ("recomputes the armour class of gA99A") or cite the evidence for a redraw.
  Fixed: both functions now say ComputeArmourClass (0DE8:0D5E) recomputes the armour class of gA99A.
- [x] must-fix src/E0ACD.C:734, :737 and :739 (PickListItem 0ACD:0BA6): three claims are wrong. (a) "arming gF3BA when the page changed" (734): the code does not set gF3BA when the page changes. It clears gF3BA when gEFB2 == -1 (lines 761 and 762), and it steps the page only while gF3BA == -1 (lines 763 to 766). (b) "marks itself used (f0BD2_0AF2)" (737): f0BD2_0AF2 is a check that returns 0 or 1, and src/E0BD2.C:626 to 630 says so ("checks that character `who` may wear the item type ... Returns 0 when the worn item's flags have 0x20, 1 otherwise"). It does not mark anything. (c) "an empty or repeated item runs InsertListItem" (739): the else branch at lines 800 to 803 inserts only when `item != 0`, so an empty item runs nothing. Only a repeated item runs InsertListItem.
  Fixed: the PickListItem comment now says gF3BA is cleared when gEFB2 is -1 and the page steps only while gF3BA is -1; f0BD2_0AF2 is described as the "may wear the item type" check; and only a repeated nonzero item runs InsertListItem while a zero item runs nothing.
- [x] must-fix src/E0ACD.C:815 (Action0D9D, 0ACD:0D9D): "key 0x23 calls Action0BA6". There is no Action0BA6 in this unit. The function at 0ACD:0BA6 is PickListItem (line 744), and the switch calls PickListItem (line 844).
  Fixed: the Action0D9D comment now says key 0x23 calls PickListItem (0ACD:0BA6).
- [x] must-fix src/E0ACD.C:875 (RunMapLoop, 0ACD:0E8A): "arms the timeout gFC5A from one 1d10 roll times 0x12". The call is `RollDice(1, 10, 3)` (lines 882 and 895). src/E082C.C:270 to 272 says RollDice returns "the sum of the rolls plus the bonus", so the roll is 1d10+3. The comment must say so.
  Fixed: the RunMapLoop comment now says the timeout is armed from one (1d10 + 3) roll times 0x12 (RollDice(1, 10, 3)).
- [x] must-fix src/E0ACD.C:904 and :905 (EnterMapScreen, 0ACD:0F72): "shows the inventory when gA998 is set" is wrong. gA998 is set to 0xfe after a saved game loads (src/EOV2.C:388 to 397). When it is set, the branch calls fOvl3_0A7C(gFBC8) (line 916), which is LoadMapScreen(level) (src/EOV3.C:540). That function loads the map and the INVENT screen for the level (src/EOV3.C:530 to 539). "Shows the pointer" in the same comment is not in that branch: the pointer is shown at line 930, after the screen loads. Rewrite the sentence to say the saved-game load path loads level gFBC8 and clears gA998.
  Fixed: the EnterMapScreen comment now says the set-gA998 branch shows the mouse item, loads level gFBC8's map and inventory through LoadMapScreen (ovl3:0A7C) and clears gA998; the pointer is shown later, after the screen loads.
- [x] must-fix src/E0ACD.C:934 and :935 (ResetMapScreen, 0ACD:1033): "resets the map screen's music" has no evidence in the code: the function sets gPartyCell to 0x1EA (line 939), gFBCC to 0 (line 940), and clears the mouse item (lines 941 and 942). "selects overlay front end 1" is LoadMapScreen(1) (line 938; src/EOV3.C:540 takes `int level`). Say that it loads map level 1.
  Fixed: the ResetMapScreen comment now says it loads map level 1 through LoadMapScreen (ovl3:0A7C), sets gPartyCell and gFBCC, and clears the mouse item.

### Shoulds

- [x] should src/E0ACD.C:46: gPartyCell is "the global sound volume". The other files disagree. src/E122E.C:212 to 213 (SetSoundVolume) assigns it, which supports "volume". src/E177D.C:92 and :199 to 212 and src/EOV4.C:524 call it the party's map cell (the sound's listener cell, and the "current map cell id"). The unit uses the name both ways (vol, "the volume's list"). Choose one reading, or write both with their sources, and check the wording in the other files too.
  Fixed: the comment now gives both readings with their sources (SetSoundVolume 122E:0001 assigns it as the volume; src/E177D.C:92 and src/EOV4.C:524 read it as the party's map cell). This unit uses it as the volume.
- [x] should src/E0ACD.C:42 to 43: gIconTable's name suggests an icon table, but the value is a party member index (-1 when none) here. src/E1414.C:101 to 147 calls the same byte "the character the blink highlights". src/E0EF3.C:227 declares the same address ds:FB34 as `void far *gIconTable[]` (far sprite pointers). Note the disagreement; the owner of src/E0EF3.C has to settle the type.
  Fixed: the comment now notes src/E1414.C:101 and the `void far *gIconTable[]` declaration at src/E0EF3.C:227, and says this unit reads the byte as a member index.
- [x] should src/E0ACD.C:52: "a message buffer passed to 0EF3:2541". src/E0EF3.C:211 says the same, but src/E0EF3.C:2091 casts it to `struct ActRec far *`. Say what the caller passes (see the RedrawPartyPanel must-fix).
  Fixed: the comment now says it is the action record passed as `struct ActRec far *` to RedrawPartyPanel when gFlag is set.
- [x] should src/E0ACD.C:47: gEFB2 is documented only as "set to 0xff when the sound page changes". PickListItem (lines 761 to 771) also uses it as the page index 0..3 (`(gEFB2 + 1) & 3`, `g374A[gEFB2]`, `g1A1B[gFBCC * 4 + gEFB2]`). Say both.
  Fixed: the comment now says it is set to 0xff on a page change (056C, 0593) and used by PickListItem (0BA6) as the page index 0..3 into g374A and g1A1B.
- [x] should src/E0ACD.C:106: g373C is "a per-page byte table". It is indexed `gFBCC * 2 + g3744[gA99A]` (line 710), so it has two entries per page, chosen by the per-character byte in g3744. Say that.
  Fixed: the comment now says the table has two entries per page and cites g373C[gFBCC * 2 + g3744[gA99A]].
- [x] should src/E0ACD.C:103 and :104: gHitY has no comment (src/E16A5.C:107 says "the y the hit test uses"); g5E1C only says "src/E1658.C" and does not say what it holds (the value is tested with `& 0x7F` at line 369, selecting Swap or UseItemSlot).
  Fixed: gHitY cites src/E16A5.C:107; g5E1C says the low seven bits choose SwapSelectedItem (1) or UseItemSlot.
- [x] should src/E0ACD.C:121 and :122: gSheetDECORATE is "the DECORATE sheet (src/EOV2.C)", and gSheet2067 is "the sheet at segment 2067". src/EOV3.C:26 and :27 name them "the INVENT sheet" (gSheet2067) and "the PLAYFLD sheet" (gSheetDECORATE), and EnterMapScreen loads PLAYFLD into gSheetDECORATE and INVENT into gSheet2067 (lines 924 and 925). Use the screen names.
  Fixed: they are now the PLAYFLD sheet (gSheetDECORATE, src/EOV3.C:27) and the INVENT sheet (gSheet2067, src/EOV3.C:26).
- [x] should src/E0ACD.C:110: gA998 says only "src/EOV2.C". State what it means: 0xff while a load is started, 0xfe after a saved game loads (src/EOV2.C:388 to 397), 0 otherwise. This is the flag EnterMapScreen tests.
  Fixed: gA998 now says 0xff while a load runs, 0xfe after a saved game loads and 0 otherwise (src/EOV2.C).
- [x] should src/E0ACD.C:142 and :144: fOvl3_0A7C and fOvl4_2039 are named in src/EOV3.C:540 (`LoadMapScreen(int level)`) and src/EOV4.C:1076 (`NoOp2039`, "discards its work ... Purpose unknown"). Use those names. fOvl4_2039 is called at line 919 in EnterMapScreen with no comment; say it is a no-op (or "Purpose unknown").
  Fixed: they are now LoadMapScreen (src/EOV3.C:540) and NoOp2039 (src/EOV4.C:1076), the latter noted as a no-op.
- [x] should src/E0ACD.C:143: DrawViewFrame's parameter is `int n`, but src/EOV4.C:1049 calls it `int full` and uses it to blank the view box (lines 1055 and 1065). Rename it `full`.
  Fixed: the parameter is now `full`.
- [x] should src/E0ACD.C:78 and :9: the CharRec comment says "the name and the hunger byte are the fields this unit touches", but this unit also touches m97 (lines 704, 726, 791 and 795). The Action comment says "60-byte action record", but the named fields end at +0x3A (58 bytes); cite the 60-byte grid it relies on (src/E0CF7.C:9) or say the size is the grid pitch.
  Fixed: the CharRec comment now includes m97; the Action comment notes the 60 bytes are the DGROUP record grid pitch (src/E0CF7.C:9) and the named fields end at +0x3A.
- [x] should src/E0ACD.C:95 and :97: Entry14.m04 has no comment (EatFood sets it to -1 at line 575), and m0B is "a second field" (EatFood adds it to m23 as the food value, lines 571 to 574). Name what they are.
  Fixed: m04 is named the record's key (-1 when free) and m0B the food value EatFood (07E0) adds to m23.
- [x] should src/E0ACD.C:555 to 556 (EatFood, 0ACD:07E0): "clears the item's m04". The code sets it to -1 (line 575). Say "sets".
  Fixed: it now says EatFood sets the item's m04 to -1 (freeing the record).
- [x] should src/E0ACD.C:338 to 339 (StepActionRow, 0ACD:03CD): the comment omits the first call, f0D1C_0078(gF51A) (line 342), which runs before the row moves.
  Fixed: the comment now includes the first f0D1C_0078(gF51A) call.
- [x] should src/E0ACD.C:285 (Action0264): the comment says "redraws it and calls ArmEffect(0, 1)" but also omits BuildList4() (line 301).
  Fixed: the comment now includes BuildList4().
- [x] should src/E0ACD.C:379 (Action0494): "a nonzero result redraws the icon": the code calls SetMouseItem(0) (line 387), which sets the item on the mouse to none. Say that.
  Fixed: it now says a nonzero result calls SetMouseItem(0), setting the mouse item to none.
- [x] should src/E0ACD.C:687 (MoveListItem): "SetMouseItem, or RedrawMember at row 0x10" omits the condition. RedrawMember is used only when gMouseDisabled is nonzero and the row is 0x10 (line 718).
  Fixed: it now says RedrawMember(gA99A) is used only when the mouse is disabled and the row is 0x10.
- [x] should src/E0ACD.C:481 (Action0644): "(and the sound RunMemberScript)". RunMemberScript (122E:1DA8) is called with (vol, 8) and (vol, 4), it is not a sound.
  Fixed: RunMemberScript is described as the script run on the volume, not a sound.
- [x] should src/E0ACD.C:609: "how is the direction." is a fragment after the comment about cases. Make it "`how` is the direction (the switch argument)".
  Fixed: it now reads "`how` is the direction (the switch argument)".
- [x] should src/E0ACD.C:733 onward (PickListItem): the push and show cases (line 738) leave out the cur/PushListItem path when a second item is already held (lines 778 to 783).
  Fixed: the comment now says a second item already held is pushed back into the volume's list before the new item is shown with SetMouseItem.

### Notes

- [x] note the packet's "Globals src/ already names" line lists ds:804A as `x (E0ACD.C)`, but this file declares gFn804A (line 40) and EOV1.ASM, EOV2.C, EOV5.C and E0EF3.C all use gFn804A. The name table, not the source, is out of step. Log owners.
  Left for the owners: a unit branch cannot change the name table; the source keeps gFn804A, which agrees with the other units.
- [x] note the compiler explanations in comments (lines 507, 609 to 610, 664 to 665, 740 to 743) are accurate matcher findings. They explain why the code is written this way, so keep them; the matcher notes in the packet are the better home for the longer ones.
  Kept unchanged.
- [x] note the 0D9D comment's "nine-entry (key, handler) table at cs:0E48" is not checked here. The switch has nine cases (lines 827 to 857); whether the compiler built a table at 0ACD:0E48 is not something I verified.
  Fixed: the comment no longer asserts a table at cs:0E48; it lists the keys and their effects.
- [x] note gFlag: this file says it "picks the a table set" (src/E0CF7.C). src/E0EF3.C:253 and :1962 say RedrawPartyPanel clears it as a "key-flag", and src/EOV4.C:1071 tests it as the view-rows gate. Several meanings share one byte; the comment should name the one this unit depends on (the redraw gate in 0264 and 0F72).
  Fixed: the gFlag comment now names the redraw gate in Action0264 (0ACD:0264) and the clear in EnterMapScreen (0ACD:0F72).
- [x] note there is no blank line before the 0494 comment (src/E0ACD.C:375 to 376).
  Fixed: a blank line now precedes the 0494 comment.
- [x] note the functions the review checked against their code and found sound (name, comment, parameters): MoveBlocked, Action0034, Action008C, Action00EB, Action018E, Action01E3, Action0393, Action056C, Action0593, Action05BA, Action05FF, Action078C, Action08E7, SetActionRow, Action0A34, StepMenuPhase, Action0415 and GetActionResult.
  Checked: they are unchanged apart from the volume wording in 05BA and 05FF.

## Round 1 summary

The unit matches (35 of 35, 4182 bytes) and most of its names are placeholders with a description
of what the function does, which is the right state for them. The comments are not yet trustworthy:
fifteen claims are wrong against the code or against other units (the header's "turn undead", the
gF51A and gDCAA ranges, the RedrawPartyPanel and DispatchKeyWait signatures, a wrong 0BD2 citation,
the Action0264 gFlag bracket, "plays the sound" in 04F8, the selected character and bit mask in 0644,
the ComputeArmourClass redraw, three PickListItem claims, a non-existent Action0BA6, the 1d10 roll
bonus, the inventory and pointer claims in EnterMapScreen, and the "music" in ResetMapScreen). The
gPartyCell and gIconTable meanings are contested across src/ and need a decision by the owners of
the other files, not by this unit alone. Nothing is escalated: there is no matcher, spec or pin
question, only comment and name evidence.

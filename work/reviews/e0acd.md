# Review: e0acd

Reviewer: rev-haiku
Commit reviewed: 3648a5c (main when I started)

Playbook 70. Judge what a matching file can still get wrong: names, types, comments, structure.
The matcher already proved the bytes; do not re-prove them, read for meaning.

## Findings

Each finding is a checkbox. `must-fix` blocks `make review-pass` until a decompiler ticks it off
(with the commit that fixes it); `should` and `note` do not block.

Line numbers are src/E0ACD.C unless another file is named. Callee addresses are the ones the
comments should cite.

- [ ] must-fix src/E0ACD.C:4: the header says the handlers "turn undead". No handler in this unit turns undead. The only call to TurnUndead (0D1C:0A43, src/E0D1C.C:631) is in RunMapLoop (0ACD:0E8A, line 900). Turn091C (0ACD:091C, lines 603 to 649) is the "turn" action: it moves the selected character (gA99A) and the action row.
- [ ] must-fix src/E0ACD.C:19: gF51A is documented as "the selected action row 0..2". This unit compares it with 0x10 in three places: `gF51A != 0x10` (lines 699, 718, 777) and `gF51A == 0x10` (line 725). Row 0x10 selects the character's own item list (0ACD:0A61 and 0ACD:0BA6). src/E122E.C:471 and :485 also test row 0x10. The 0..2 range is only the StepActionRow wrap (lines 347 to 350).
- [ ] must-fix src/E0ACD.C:48: gDCAA is documented as "a 1..2 step counter". This unit tests `gDCAA == 5` (the rest phase) at lines 668 (0ACD:0A34), 697 (0ACD:0A61) and 823 (0ACD:0D9D). 0ACD:0753 wraps it to 1 and 2, and EnterMapScreen sets it to 1 (line 922). The range must include the value 5 and say what sets it.
- [ ] must-fix src/E0ACD.C:76: `RedrawPartyPanel(char far *text)` gives the argument a text type, and the RedrawPartyPanel call sites pass gF542 as a message. The definition in src/E0EF3.C:1960 is `int RedrawPartyPanel(struct ActRec far *rec)`. It reads rec->m04 as its result, and it draws no text (its comment at E0EF3.C:1956 says it redraws the party panel). src/E0EF3.C:2091 casts gF542 to `struct ActRec far *`. Take the parameter type from the definition. The same wrong prototype is in src/E122E.C:64 and src/EOV5.C:206 (cross-unit, not ours to fix).
- [ ] must-fix src/E0ACD.C:129 and :866: DispatchKeyWait is declared `int DispatchKeyWait(void far *list)`, but src/E16A5.C:144 defines it `void DispatchKeyWait(struct Node far *head)`. Its last statement is DispatchNodeEvent(head, key), which returns "the event it consumed, or 0" (src/E16A5.C:163 to 164). The comment at line 866, "gets the current list node from 16A5:005A(gList)", is wrong. The function waits for one key event, dispatches it to the list head, and what Action0E6C gets back is the event consumed (or 0). Fix the comment and take the prototype from the definition or say why it is int.
- [ ] must-fix src/E0ACD.C:85: the m97 comment cites "0D1C:02B9 and 0D1C:01D0". Those two calls are RemoveListItem and InsertListItem at 0BD2:02B9 and 0BD2:01D0 (this file's own externs at lines 126 and 127, and the packet's callers table). Cite the 0BD2 addresses.
- [ ] must-fix src/E0ACD.C:285 (Action0264, 0ACD:0264): "The gFlag redraws bracket both ends of the swap" is wrong. The leading `if (gFlag) RedrawPartyPanel` (line 295) runs on both paths. The second one (lines 299 and 300) is only in the pick-up branch (gIconTable == -1). The swap branch (lines 306 to 316) has no gFlag redraw after the swap.
- [ ] must-fix src/E0ACD.C:394 (Action04F8, 0ACD:04F8): "plays the sound for the current page ... played through SetSoundVolume" is wrong. SetSoundVolume (src/E122E.C:207 to 222, 122E:0001) switches the sound volume: it sets gPartyCell, runs the driver scripts, sets the page and action dirty flags, and draws the volume message gText5AB5. It does not play a sound. Fix the comment for 04F8 (and check 0529, 05BA, 05FF, whose comments say the same thing).
- [ ] must-fix src/E0ACD.C:477 and :480 (Action0644, 0ACD:0644): (a) "uses the item in the slot at `action` on the selected character" is wrong. The function never reads gA99A (the selected character). It puts the item in the volume's list (gB899 + vol * 9, lines 510 and 517) and runs the script on vol. (b) "bits 0..3 of g194B" is wrong: the mask at line 504 is `& 0xB`, which is bits 0, 1 and 3 (bit 2 is not tested).
- [ ] must-fix src/E0ACD.C:682, :688 and :740 (MoveListItem 0ACD:0A61, PickListItem 0ACD:0BA6): "redraws gA99A through ComputeArmourClass" is not evidenced. src/E0DE8.C:665 to 671 says ComputeArmourClass computes the armour class m1D of character `who`; it does not mention a redraw. Say what the call does ("recomputes the armour class of gA99A") or cite the evidence for a redraw.
- [ ] must-fix src/E0ACD.C:734, :737 and :739 (PickListItem 0ACD:0BA6): three claims are wrong. (a) "arming gF3BA when the page changed" (734): the code does not set gF3BA when the page changes. It clears gF3BA when gEFB2 == -1 (lines 761 and 762), and it steps the page only while gF3BA == -1 (lines 763 to 766). (b) "marks itself used (f0BD2_0AF2)" (737): f0BD2_0AF2 is a check that returns 0 or 1, and src/E0BD2.C:626 to 630 says so ("checks that character `who` may wear the item type ... Returns 0 when the worn item's flags have 0x20, 1 otherwise"). It does not mark anything. (c) "an empty or repeated item runs InsertListItem" (739): the else branch at lines 800 to 803 inserts only when `item != 0`, so an empty item runs nothing. Only a repeated item runs InsertListItem.
- [ ] must-fix src/E0ACD.C:815 (Action0D9D, 0ACD:0D9D): "key 0x23 calls Action0BA6". There is no Action0BA6 in this unit. The function at 0ACD:0BA6 is PickListItem (line 744), and the switch calls PickListItem (line 844).
- [ ] must-fix src/E0ACD.C:875 (RunMapLoop, 0ACD:0E8A): "arms the timeout gFC5A from one 1d10 roll times 0x12". The call is `RollDice(1, 10, 3)` (lines 882 and 895). src/E082C.C:270 to 272 says RollDice returns "the sum of the rolls plus the bonus", so the roll is 1d10+3. The comment must say so.
- [ ] must-fix src/E0ACD.C:904 and :905 (EnterMapScreen, 0ACD:0F72): "shows the inventory when gA998 is set" is wrong. gA998 is set to 0xfe after a saved game loads (src/EOV2.C:388 to 397). When it is set, the branch calls fOvl3_0A7C(gFBC8) (line 916), which is LoadMapScreen(level) (src/EOV3.C:540). That function loads the map and the INVENT screen for the level (src/EOV3.C:530 to 539). "Shows the pointer" in the same comment is not in that branch: the pointer is shown at line 930, after the screen loads. Rewrite the sentence to say the saved-game load path loads level gFBC8 and clears gA998.
- [ ] must-fix src/E0ACD.C:934 and :935 (ResetMapScreen, 0ACD:1033): "resets the map screen's music" has no evidence in the code: the function sets gPartyCell to 0x1EA (line 939), gFBCC to 0 (line 940), and clears the mouse item (lines 941 and 942). "selects overlay front end 1" is LoadMapScreen(1) (line 938; src/EOV3.C:540 takes `int level`). Say that it loads map level 1.

### Shoulds

- [ ] should src/E0ACD.C:46: gPartyCell is "the global sound volume". The other files disagree. src/E122E.C:212 to 213 (SetSoundVolume) assigns it, which supports "volume". src/E177D.C:92 and :199 to 212 and src/EOV4.C:524 call it the party's map cell (the sound's listener cell, and the "current map cell id"). The unit uses the name both ways (vol, "the volume's list"). Choose one reading, or write both with their sources, and check the wording in the other files too.
- [ ] should src/E0ACD.C:42 to 43: gIconTable's name suggests an icon table, but the value is a party member index (-1 when none) here. src/E1414.C:101 to 147 calls the same byte "the character the blink highlights". src/E0EF3.C:227 declares the same address ds:FB34 as `void far *gIconTable[]` (far sprite pointers). Note the disagreement; the owner of src/E0EF3.C has to settle the type.
- [ ] should src/E0ACD.C:52: "a message buffer passed to 0EF3:2541". src/E0EF3.C:211 says the same, but src/E0EF3.C:2091 casts it to `struct ActRec far *`. Say what the caller passes (see the RedrawPartyPanel must-fix).
- [ ] should src/E0ACD.C:47: gEFB2 is documented only as "set to 0xff when the sound page changes". PickListItem (lines 761 to 771) also uses it as the page index 0..3 (`(gEFB2 + 1) & 3`, `g374A[gEFB2]`, `g1A1B[gFBCC * 4 + gEFB2]`). Say both.
- [ ] should src/E0ACD.C:106: g373C is "a per-page byte table". It is indexed `gFBCC * 2 + g3744[gA99A]` (line 710), so it has two entries per page, chosen by the per-character byte in g3744. Say that.
- [ ] should src/E0ACD.C:103 and :104: gHitY has no comment (src/E16A5.C:107 says "the y the hit test uses"); g5E1C only says "src/E1658.C" and does not say what it holds (the value is tested with `& 0x7F` at line 369, selecting Swap or UseItemSlot).
- [ ] should src/E0ACD.C:121 and :122: gSheetDECORATE is "the DECORATE sheet (src/EOV2.C)", and gSheet2067 is "the sheet at segment 2067". src/EOV3.C:26 and :27 name them "the INVENT sheet" (gSheet2067) and "the PLAYFLD sheet" (gSheetDECORATE), and EnterMapScreen loads PLAYFLD into gSheetDECORATE and INVENT into gSheet2067 (lines 924 and 925). Use the screen names.
- [ ] should src/E0ACD.C:110: gA998 says only "src/EOV2.C". State what it means: 0xff while a load is started, 0xfe after a saved game loads (src/EOV2.C:388 to 397), 0 otherwise. This is the flag EnterMapScreen tests.
- [ ] should src/E0ACD.C:142 and :144: fOvl3_0A7C and fOvl4_2039 are named in src/EOV3.C:540 (`LoadMapScreen(int level)`) and src/EOV4.C:1076 (`NoOp2039`, "discards its work ... Purpose unknown"). Use those names. fOvl4_2039 is called at line 919 in EnterMapScreen with no comment; say it is a no-op (or "Purpose unknown").
- [ ] should src/E0ACD.C:143: DrawViewFrame's parameter is `int n`, but src/EOV4.C:1049 calls it `int full` and uses it to blank the view box (lines 1055 and 1065). Rename it `full`.
- [ ] should src/E0ACD.C:78 and :9: the CharRec comment says "the name and the hunger byte are the fields this unit touches", but this unit also touches m97 (lines 704, 726, 791 and 795). The Action comment says "60-byte action record", but the named fields end at +0x3A (58 bytes); cite the 60-byte grid it relies on (src/E0CF7.C:9) or say the size is the grid pitch.
- [ ] should src/E0ACD.C:95 and :97: Entry14.m04 has no comment (EatFood sets it to -1 at line 575), and m0B is "a second field" (EatFood adds it to m23 as the food value, lines 571 to 574). Name what they are.
- [ ] should src/E0ACD.C:555 to 556 (EatFood, 0ACD:07E0): "clears the item's m04". The code sets it to -1 (line 575). Say "sets".
- [ ] should src/E0ACD.C:338 to 339 (StepActionRow, 0ACD:03CD): the comment omits the first call, f0D1C_0078(gF51A) (line 342), which runs before the row moves.
- [ ] should src/E0ACD.C:285 (Action0264): the comment says "redraws it and calls ArmEffect(0, 1)" but also omits BuildList4() (line 301).
- [ ] should src/E0ACD.C:379 (Action0494): "a nonzero result redraws the icon": the code calls SetMouseItem(0) (line 387), which sets the item on the mouse to none. Say that.
- [ ] should src/E0ACD.C:687 (MoveListItem): "SetMouseItem, or RedrawMember at row 0x10" omits the condition. RedrawMember is used only when gMouseDisabled is nonzero and the row is 0x10 (line 718).
- [ ] should src/E0ACD.C:481 (Action0644): "(and the sound RunMemberScript)". RunMemberScript (122E:1DA8) is called with (vol, 8) and (vol, 4), it is not a sound.
- [ ] should src/E0ACD.C:609: "how is the direction." is a fragment after the comment about cases. Make it "`how` is the direction (the switch argument)".
- [ ] should src/E0ACD.C:733 onward (PickListItem): the push and show cases (line 738) leave out the cur/PushListItem path when a second item is already held (lines 778 to 783).

### Notes

- [ ] note the packet's "Globals src/ already names" line lists ds:804A as `x (E0ACD.C)`, but this file declares gFn804A (line 40) and EOV1.ASM, EOV2.C, EOV5.C and E0EF3.C all use gFn804A. The name table, not the source, is out of step. Log owners.
- [ ] note the compiler explanations in comments (lines 507, 609 to 610, 664 to 665, 740 to 743) are accurate matcher findings. They explain why the code is written this way, so keep them; the matcher notes in the packet are the better home for the longer ones.
- [ ] note the 0D9D comment's "nine-entry (key, handler) table at cs:0E48" is not checked here. The switch has nine cases (lines 827 to 857); whether the compiler built a table at 0ACD:0E48 is not something I verified.
- [ ] note gFlag: this file says it "picks the a table set" (src/E0CF7.C). src/E0EF3.C:253 and :1962 say RedrawPartyPanel clears it as a "key-flag", and src/EOV4.C:1071 tests it as the view-rows gate. Several meanings share one byte; the comment should name the one this unit depends on (the redraw gate in 0264 and 0F72).
- [ ] note there is no blank line before the 0494 comment (src/E0ACD.C:375 to 376).
- [ ] note the functions the review checked against their code and found sound (name, comment, parameters): MoveBlocked, Action0034, Action008C, Action00EB, Action018E, Action01E3, Action0393, Action056C, Action0593, Action05BA, Action05FF, Action078C, Action08E7, SetActionRow, Action0A34, StepMenuPhase, Action0415 and GetActionResult.

## Summary

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

# Review: eov0

Reviewer: rev-haiku
Commit reviewed: 2ba0b7c (main, when I started); src/EOV0.C is unchanged from be85fef to 2ba0b7c

Playbook 70. Judge what a matching file can still get wrong: names, types, comments, structure.
The matcher already proved the bytes; do not re-prove them, read for meaning.

## Findings

Each finding is a checkbox. `must-fix` blocks `make review-pass` until a decompiler ticks it off
(with the commit that fixes it); `should` and `note` do not block.

Round 1 (2ba0b7c). Evidence is the code in src/EOV0.C at the line given, plus the cross-file lines named.

- [ ] must-fix src/EOV0.C:175: the comment on `g5E20` says "nonzero when the mouse moved (src/E16A5.C)", but src/E16A5.C:130 says it "forces the walk to rebuild and redraw the whole" list. This unit sets it in BuildRowNodeList (src/EOV0.C:368), and src/EOV1.ASM:2454 sets it too. Change the comment to the list-rebuild meaning and cite the rebuild, not mouse motion.
- [ ] must-fix src/EOV0.C:103: the comment on `gDCA4` says "the max damage ovl0:135D recomputes", but ovl0:135D stores `MaxHitPoints(...)` into it (src/EOV0.C:896), and src/E0A72.C:206 says that function is max hit points. No evidence supports "damage". Say max hit points, and cite EOV0.C:896 and E0A72.C:206 for it.
- [ ] must-fix src/EOV0.C:104: the comment on `gDD10` says "the min damage ovl0:135D recomputes (src/E08E6.C)", but src/E08E6.C uses `gDD10` as a 0/1 flag: DamageObject sets it to 1 (src/E08E6.C:820) and the routine at 08E6:0E48 clears it (src/E08E6.C:781, `gF518 = gDD10 = 0`). The value ovl0:135D writes is `MinHitPoints` (src/EOV0.C:898, src/E0A72.C:231). Say what ovl0:135D writes, and cite E0A72.C, not E08E6.C, for it.
- [ ] must-fix src/EOV0.C:417-418 (DrawCharPanel comment): "the ratio of m0D against m0F" is wrong. `FormatStrengthPercent` (src/E0A72.C:263-275) formats the score and the percentile as text `"%d/%02d"` (or `"%d"`), it does not compute a ratio. Say the strength string "score/percentile".
- [ ] must-fix src/EOV0.C:421-422 (DrawCharPanel "Called by"): the list omits `Ovl0_1ADD` (ovl0:1ADD), which calls `DrawCharPanel(gDCB0, 3)` at src/EOV0.C:1038 and 1063. Add it. The packet's caller list agrees.
- [ ] must-fix src/EOV0.C:320-321 (FlashPanelIcon comment): "draws it in its selected form (mode 1)". The code is `DrawPanelIcon(index, 1, 0)` (src/EOV0.C:325). The 1 is the second parameter, `rowBase` (the row offset of the selected cell), not `mode`; mode is the third argument, 0. Say "row + 1 (the selected cell row)" and "mode 0".
- [ ] should src/EOV0.C:421: "each in its plain form, mode 2": `DrawPanelIcon(...,0,2)` (src/EOV0.C:457-469) draws at the icon's own origin without hiding the pointer. That is not the "plain" form that FlashPanelIcon's comment calls mode 0 (src/EOV0.C:325-328). Reword so "plain" is not used for two different modes.
- [ ] must-fix src/EOV0.C:497 (PickPartySlot): "only at the ends (or with the mouse disabled), slides the slot". The code is `if (pos == 0 || !gMouseDisabled)` (src/EOV0.C:569): it slides when `pos` is at an end or the mouse is enabled, and steps `pos` otherwise. Replace "disabled" with "enabled" (or with the negated form).
- [ ] must-fix src/EOV0.C:500 (PickPartySlot "Called by"): "Called by ovl0:0DDC and ovl0:0EE5." Only Ovl0_0DDC calls it (src/EOV0.C:638, 659). Ovl0_0EE5 does not. Remove 0EE5.
- [ ] must-fix src/EOV0.C:627-632 (Ovl0_0DDC): "the enter key (0x61) and key 0x8004 flash icon 6 and keep looping" is wrong. Those keys set `key = 1` (src/EOV0.C:662-666), which reaches `return 0` (src/EOV0.C:676-680). They end the loop.
- [ ] must-fix src/EOV0.C:627-632 (Ovl0_0DDC): "any other key exits after drawing the current row list" is wrong. The default case sets `key = 0` (src/EOV0.C:667-669), and `if (key == 0) continue;` (src/EOV0.C:676-677) keeps the loop going.
- [ ] must-fix src/EOV0.C:627-632 (Ovl0_0DDC): "keys 0x8001/0x8002/0x8003 ... run ... the race picker (Ovl0_1ADD) or the item list (ovl0:0A5E)". Ovl0_1ADD edits the seven ability/stat fields (its own comment, src/EOV0.C:1026-1027; the call at src/EOV0.C:654). ovl0:0A5E is the party-position screen (src/EOV0.C:490-491), not an item list. Fix both names.
- [ ] must-fix src/EOV0.C:630-631 (Ovl0_0DDC "Called by"): "Called by no direct caller found". CreateCharacter calls it (src/EOV0.C:1305, packet caller list ovl0:23EC). Name CreateCharacter as the caller.
- [ ] must-fix src/EOV0.C:683-689 (Ovl0_0EE5): "the enter/left/right keys flash icon 9". The keys in that case group are 0x2b, 0x3d, 0x61, 0x6e and 0x8006 (src/EOV0.C:719-724), which are enter/confirm keys. The left/right keys are 0x5c/0x66 and 0x60/0x62, handled by the separate cases (src/EOV0.C:736-743), and the comment says those correctly. Replace "left/right" with the confirm keys.
- [ ] must-fix src/EOV0.C:859-860 (EditStatField): "`percent` ... (only read for index 0)". The code reads it for every index: `count = *percent;` runs unconditionally (src/EOV0.C:893). Callers pass a null far pointer for index 1..6 (src/EOV0.C:1045-1060). Say it is read on every call but used only for index 0.
- [ ] should src/EOV0.C:864-867 (EditStatField clamp): "A changed value is clamped (index 0 to gOutDC98[0] & 0xff and its percentile reset, ...)" omits the lower bound (`gOutDD04[0]`, src/EOV0.C:961) and the rule that strength above 0x12 moves into the percentile (src/EOV0.C:951-958). Say both.
- [ ] should src/EOV0.C:866-867 (EditStatField): "index 4/3/6 refresh the derived hit points (m1B/m1C from gDCA4/gDD10) or m1D". Index 6 only sets `m1C = amount` (src/EOV0.C:974-975). Index 4 recomputes the hit points, and index 3 sets m1D. Describe index 6 correctly.
- [ ] must-fix src/EOV0.C:1319 (InitPanelScreens): "ten row cells gPtrsFD54 (0, i*4)" gives (x, y) as (0, i*4). The code is `gFn8082(i * 4, 0, 4, 0x20, gOverlayBuf)` (src/EOV0.C:1335), so x = i*4 and y = 0. Swap the tuple.
- [ ] should src/EOV0.C:1320: "in video modes 0 and 1, blits gOverlayBuf with f19EC_002A and clips the panel". The else branch also blits (`f19EC_002A(3, 2, ...)`, src/EOV0.C:1341). Say the two modes take the (3,4) blit with a clip, every other mode the (3,2) blit.
- [ ] should src/EOV0.C:1434-1436 (FreePanelBuffers) and src/EOV0.C:118: "the 0x43-entry table gPtrsFC8C". InitPanelScreens fills only 0x2c entries (src/EOV0.C:1329-1330). The free loop covers 0x43 (src/EOV0.C:1441). Entries 0x32..0x3b are the same storage as gPtrsFD54 (ds:FD54). Say what the 0x43 bound is, and that entries past 0x2c are not set by this unit.
- [ ] must-fix src/EOV0.C:1401 (InitCharSlotValues): "whose class byte ... has bit 2 set". The test is `g1231[...] & 2` (src/EOV0.C:1408), which is 0x2. The same file says "bit 2" for mask -5, which clears 0x4 (src/EOV0.C:1190; PickClass comment at 1172-1173). Say "bit 1 (value 2)", or rewrite with the value.
- [ ] must-fix src/EOV0.C:1413 (ShiftCharFields): "copies the nine bytes at +0x0D ... +0x1B". The list is eight fields (0D, 0F, 11, 13, 15, 17, 19, 1B) and the code has eight copies (src/EOV0.C:1423-1430). Say eight.
- [ ] must-fix src/EOV0.C:1449-1451 (RunCharacterPanel): "the two function-key toggles (gDCB0 ^= 1, ^= 2) on the 0x5c..0x66 range" is wrong. `^= 1` is on 0x5c/0x66 and `^= 2` is on 0x60/0x62 (src/EOV0.C:1474-1481). Describe both pairs. Also "the enter/left/right keys (0x61, 0x2b, 0x3d)" (src/EOV0.C:1494): those are the enter/confirm keys, not left/right. Fix both.
- [ ] must-fix src/EOV0.C:1031 (Ovl0_1ADD "Called by"): "Called by ovl0:0DDC and ovl0:0EE5." Only Ovl0_0DDC calls it (src/EOV0.C:654). Ovl0_0EE5 (src/EOV0.C:690-757) has no call to it. Remove 0EE5.
- [ ] must-fix src/EOV0.C:1231 (PickAlignment): "or 0x6e when the mouse finished". `done` is also set by the keyboard keys 0x6e and 0x32 (src/EOV0.C:1259-1260), and the comment itself says so one sentence earlier. The return is 0x6e for those keys too (src/EOV0.C:1266-1268). Drop "when the mouse finished" or say "the mouse or the keys 0x6e/0x32".
- [ ] should src/EOV0.C:1177 (PickClass "Called by"): "Called by ovl0:23EC for the race member." CreateCharacter calls it for the class step (src/EOV0.C:1294, `PickClass(m1F)` returns m20). "The race member" is unclear: it is the new character's class pick, given the race. Reword.
- [ ] should src/EOV0.C:1274 (CreateCharacter): "each step that the picker reported as confirmed (0x6e, the mouse's accept) steps the index back". Only steps 1 and 2 can be confirmed. Step 0 (PickOption) sets no `confirmed` (src/EOV0.C:1290-1291). Say so.
- [ ] should src/EOV0.C:1154-1155 (FindNthSetBit): "the index (0-based) of the nth set bit". `remaining = n` and the break at `remaining == 0` (src/EOV0.C:1161-1166) make n 1-based: n = 1 is the lowest set bit. Say "the nth set bit (n = 1 is the lowest)".
- [ ] should src/EOV0.C:397 (DrawCharName): "(0x3c - width * gFontWidth) / 2 for a name `len` characters wide". The code is `(0x3c - gFontWidth * len) >> 1` (src/EOV0.C:408). `width` is undefined in the comment; say `len * gFontWidth` (pixels).
- [ ] should src/EOV0.C:1075-1076 (RollCharacterStats): "clamped up to gOutDD04 and down to gOutDC98, and when the strength roll is exactly 18 the low byte gets a percentile roll". The code clamps up (src/EOV0.C:1092-1093), then ORs the percentile in (src/EOV0.C:1094-1095), then clamps down (src/EOV0.C:1096-1097). The order matters for the 0x12 case; state it.
- [ ] should src/EOV0.C:100-102 and 149: the prototypes name the first parameter `who` (RollHitPoints, MaxHitPoints, MinHitPoints) and `level`/`percent`/`value`. src/E0A72.C:180, 206, 231 name it `classIndex` (the class index, passed `gChars[].m20`) and `m17`. src/E0A72.C:263 names the strength parameters `score` and `percent`. Rename to match E0A72.C, or cite why the names differ.
- [ ] should src/EOV0.C:166 (gDrawText): the prototype names the parameters `a, b, c, d`. src/E082C.C:65 declares the same far pointer with `x, y, colour, extra`, and the two call sites in this file are colour/extra (src/EOV0.C:410-411, 1020). Use the E082C.C names.
- [ ] should src/EOV0.C:119 (gDCB0): "a character index read by ovl0:11BA (Purpose unknown)". It is the current character index for the whole panel: RunCharacterPanel writes it (src/EOV0.C:1476-1490), and every character routine indexes `gChars[gDCB0]`. Say that, and keep "Purpose unknown" for what the value means beyond "current character".
- [ ] should src/EOV0.C:57 (CharRec `pad00`): named as padding, but ovl0:26C6 writes the character index `i` into it (src/EOV0.C:1368). A name that says "pad" claims the byte is unused. Rename it (for example `index00`) or note the evidence.
- [ ] should src/EOV0.C:79 and 116: the struct comment says m20 "indexes g1231 (src/E0DE8.C)" and the global comment at line 116 says g1231 is "(src/E0BD2.C)". Cite one owner for g1231 and make both comments agree.
- [ ] should src/EOV0.C:1131: `}/* ovl0:1F66:` puts the closing brace of BuildCombatTables and the next comment on one line. Start the comment on its own line like every other function here.
- [ ] should src/EOV0.C:4: the header says the unit shows "the status/rest screens". Nothing in this unit draws a rest screen; it draws the party-status report (DrawPartyStatus, src/EOV0.C:375-395) and the report lines. Name what it does.
- [ ] should src/EOV0.C:103: type conflict on `ds:DCA4`. This unit declares it `int` and EditStatField writes hit points into it (src/EOV0.C:896). src/E0DE8.C:132 declares the same byte as `char` ("the object acting this step, set by 0359"), and E0DE8.C:342 sets it to a target. The two units give the byte different meanings. Not decidable from this file; the owner of E0DE8.C should say which unit's use is the right one.
- [ ] note src/EOV0.C:21: `g3A0` (0000:03A0) is described as "a flag in the resident segment (Purpose unknown)". That is fine as a placeholder. The address is outside this unit's overlay; the next reader should not look for it in the overlay buffers.
- [ ] note build/packets/eov0.md "Globals src/ already names" lists `ds:8082` as `x`, while the file names it `gFn8082` (src/EOV0.C:16) and src/EOV3.C:36 uses the same name. This is a symbol-table name, not a source problem; `make rename` can settle it.
- [ ] note `make match FILE=src/EOV0.C` prints "unit eov0 not complete" even when all 35 functions match and the packet says `complete = True`. tools/match/cmatch.py:619-620 requires `ovl is None`, so an overlay unit can never print "complete" there. `make review-pass` checks completeness with tools/agent/common.unit_complete, not this line, so it is not blocked by it. Human to decide whether the wording should change.
- [ ] note the functions whose behaviour is clear from the code and read well: DrawPanelIcon's mode and row logic (src/EOV0.C:285-290), FindSlot (src/EOV0.C:592-596), InitRowNode (src/EOV0.C:330-333), and Ovl0_1224's frame stepping (src/EOV0.C:829-834). Their names and address comments agree with the code.

## Summary

The unit is mostly understandable from its source alone. The structure, the struct layouts and the
address comments are sound, and the character-creation path (PickOption, PickClass, PickAlignment,
RollCharacterStats) reads right. Most of the 20 must-fix items are not about the bytes: they are
comments that describe the wrong key, the wrong caller, the wrong argument, or the wrong byte count.
The biggest cross-file problems are the meanings of gDCA4 and gDD10 (their comments claim damage,
while E0A72.C and E08E6.C say hit points and a 0/1 flag) and g5E20 (cited to E16A5.C for the wrong
meaning). The placeholder names (Ovl0_*, f1A29_000F, gStr*, the Purpose unknown globals) are fine
where no evidence names them; none of the placeholders I checked claims more than its evidence.
Nothing escalated: no matcher, spec or pin question. The "not complete" line from cmatch is a
printing quirk for overlay units (see the note above), not a unit fault.

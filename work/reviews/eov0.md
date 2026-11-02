# Review: eov0

Reviewer: rev-haiku
Commit reviewed: round 2 at 96459a0 (main); round 1 at 2ba0b7c (main, when I started); src/EOV0.C is unchanged from be85fef to 2ba0b7c

Playbook 70. Judge what a matching file can still get wrong: names, types, comments, structure.
The matcher already proved the bytes; do not re-prove them, read for meaning.

## Findings

Each finding is a checkbox. `must-fix` blocks `make review-pass` until a decompiler ticks it off
(with the commit that fixes it); `should` and `note` do not block.

Round 2 (96459a0, main, after round 1 fixes 4ac62b0 and ada5f3d). Reviewer rev-haiku. Playbook 70, later round: the
round 1 must-fix items and the changes since 2ba0b7c (`git diff 2ba0b7c main -- src/EOV0.C re/symbols.toml
re/globals.toml`; the EOV0.C part is comments, names and types only, no function body moved).

Round 1 must-fix items, checked against the code at each place they cite: the g5E20, gDCA4 and gDD10 meanings
(BuildRowNodeList sets g5E20 at EOV0.C:368; MaxHitPoints/MinHitPoints are the calls at EOV0.C:902-905 and E0A72.C:206,
:231), FlashPanelIcon rowBases (EOV0.C:325, 328), PickPartySlot's mouse test and caller (EOV0.C:570, 576, 640, 661),
Ovl0_0DDC's key outcomes (EOV0.C:664-680), Ovl0_0EE5's confirm keys (EOV0.C:722-730), EditStatField's clamp order
and the index 3/4/6 branches (EOV0.C:956-1008), InitPanelScreens' tuple and both blits (EOV0.C:1346-1352),
InitCharSlotValues' value 2 (EOV0.C:1419), ShiftCharFields' eight copies (EOV0.C:1434-1441), the RunCharacterPanel
toggles and confirm keys (EOV0.C:1485-1505), the Ovl0_1ADD and PickAlignment callers, and the prototypes
(E0A72.C:180, :206, :231, :263; E082C.C:65). All fixed right in substance: the ticks stay. The round 1 items that
were only about citations are covered by the round 2 should items below.

- [x] must-fix src/EOV0.C:1083-1084 (RollCharacterStats): "when the strength roll is exactly 18 the low byte gets a
  percentile roll (1d100) ORed into its high half". The code is `scores[0] |= RollDice(1, 0x64, 0) << 8`
  (src/EOV0.C:1102-1103): the roll goes into the high byte of the word, and the low byte (the strength, m0D) stays
  0x12. The high byte is the percentile m0F (src/EOV0.C:1107-1108). Say the high byte gets the 1d100 roll (ORed in)
  and the low byte stays 18. This line was changed in 4ac62b0 and still names the wrong byte. (fixed cd1070f: the
  sentence now says the 1d100 percentile roll is ORed into the high byte and the low byte stays 18.)
- [x] should src/EOV0.C:103-104: the cites `src/EOV0.C:896` and `src/EOV0.C:898` are stale; the MaxHitPoints and
  MinHitPoints calls are now at EOV0.C:902 and 904 (and 992, 994). `src/E08E6.C:784, :823` are stale: the 0/1 flag
  uses are E08E6.C:766 (`gF518 = gDD10 = 0`) and :805 (`gDD10 = 1`). The claims hold. Replace the line numbers
  with addresses (ovl0:135D for the writes, 08E6:xxxx for the flag uses, as the owner of E08E6.C gives them).
  (fixed cd1070f: both comments now cite ovl0:135D and 0A72:02B3/0A72:03A4 for the writes, and 08E6:0E48,
  08E6:0FBE for the flag uses.)
- [x] should src/EOV0.C:175: the cite `src/E16A5.C:130` is stale; the declaration with the same meaning is now
  src/E16A5.C:132 (its writers are at :283 and :300). The meaning holds. Use the address of the list walk instead
  of the line. (fixed cd1070f: the comment cites 16A5:00BD, the DispatchNodeEvent walk, and ovl0:042F for the set.)
- [x] should src/EOV0.C:57: the cite `(src/EOV0.C:1368)` for `index00` is stale; `rec->index00 = i` is now at
  src/EOV0.C:1379 (ovl0:26C6). (fixed cd1070f: the comment cites only the address ovl0:26C6.)
- [x] should src/EOV0.C:118: `(src/EOV0.C:1329-1330)` for the 0x2c cells is stale; the loop is at src/EOV0.C:1340-1341.
  (fixed cd1070f: the comment cites ovl0:2547.)
- [x] should src/EOV0.C:119: `(src/EOV0.C:1476-1490)` for the gDCB0 toggles is stale; the toggles are at
  src/EOV0.C:1485-1491 (RunCharacterPanel starts at 1466). (fixed cd1070f: the comment cites ovl0:28F9.)
- [x] should src/EOV0.C:634: `CreateCharacter (src/EOV0.C:1305)` is stale; the `Ovl0_0DDC()` call is at
  src/EOV0.C:1315 (1305 is the PickClass step). The caller claim holds. (fixed cd1070f: the comment cites
  ovl0:23EC.)
- [x] should src/EOV0.C:862-863 (EditStatField): `(src/EOV0.C:893)` for the read of `*percent` is stale (the read,
  `count = *percent;`, is at EOV0.C:899; 893 is LockHidePointer). `(src/EOV0.C:1045-1060)` for the callers is stale;
  they are at EOV0.C:1049-1067. The "read on every call, used only for index 0" claim is right. (fixed cd1070f:
  the comment cites ovl0:135D for the read and ovl0:1ADD for the callers.)
- [x] should src/EOV0.C:870-872 (EditStatField): the index-6 description says only that it copies the value to m1C.
  The code also recomputes gDCA4 and gDD10 on entry for index 6 (src/EOV0.C:901-906), which the comment does not
  mention. Add it. (fixed cd1070f: the comment now says index 6 also recomputes gDCA4/gDD10 from
  MaxHitPoints/MinHitPoints on entry.)
- [x] should src/EOV0.C:1032-1038 (Ovl0_1ADD): line 1035 ends in "m0F;" and the next line starts a new
  sentence, and line 1032 runs past 110 columns. Reflow the paragraph like the rest of the file. (fixed cd1070f:
  the paragraph is reflowed, the first line is under 110 columns, and the m0F sentence runs on into the fields
  1..6 sentence.)
- [x] should src/EOV0.C:1241 (PickAlignment "Called by ovl0:23EC for the class member."): the same unclear wording
  round 1 asked to fix for PickClass ("the race member"). CreateCharacter calls PickAlignment for step 2, the new
  member's alignment pick (src/EOV0.C:1307). Say so. (fixed cd1070f: now "Called by ovl0:23EC (CreateCharacter)
  for the new member's alignment pick, given its class.")
- [x] note src/EOV0.C:491-492 (PickPartySlot): "FindSlot from slot 0x1a when the last option key was odd". gFC8A is
  the parity of the PickOption choice (src/EOV0.C:1160), not of the last key. This header line was not changed by
  round 1; it is a note. (fixed cd1070f: the header now says gFC8A is the parity of the race choice PickOption
  returned, and the gFC8A declaration comment now matches.)
- [ ] note routed to the EOV1 owner (not edited here): src/EOV1.ASM:97 says g5E20 is "nonzero when the mouse moved
  (E16A5.C)", the same wrong meaning round 1 removed from src/EOV0.C:175.
- [ ] note routed to the E0BD2 and E0DE8 owners (not decided here): src/E0BD2.C:102 declares g1231 and cites
  "(src/E0DE8.C)", and src/E0DE8.C:119 calls it "a byte table used by 0D5E". This unit cites E0BD2.C as the owner
  (round 1 item 51, with the packet). The two owners should agree on it.

Round 2 verdict: one must-fix (the RollCharacterStats byte, a wrong claim in a changed line). Nothing escalated: no
matcher, spec or pin question. `make match FILE=src/EOV0.C` 35 of 35 (10873 bytes, the "not complete" line is the
round 1 note); `make readability UNIT=eov0` 0 findings.

Round 1 (2ba0b7c). Evidence is the code in src/EOV0.C at the line given, plus the cross-file lines named.
Fixed by commit 4ac62b0 (comment and name changes only; `make match FILE=src/EOV0.C` 35 of 35, `make readability UNIT=eov0` 0 findings).

- [x] must-fix src/EOV0.C:175: the comment on `g5E20` says "nonzero when the mouse moved (src/E16A5.C)", but src/E16A5.C:130 says it "forces the walk to rebuild and redraw the whole" list. This unit sets it in BuildRowNodeList (src/EOV0.C:368), and src/EOV1.ASM:2454 sets it too. Change the comment to the list-rebuild meaning and cite the rebuild, not mouse motion. (fixed 4ac62b0: now says it forces the row-list walk to rebuild and redraw the whole list, cites E16A5.C:130 and BuildRowNodeList at EOV0.C:368.)
- [x] must-fix src/EOV0.C:103: the comment on `gDCA4` says "the max damage ovl0:135D recomputes", but ovl0:135D stores `MaxHitPoints(...)` into it (src/EOV0.C:896), and src/E0A72.C:206 says that function is max hit points. No evidence supports "damage". Say max hit points, and cite EOV0.C:896 and E0A72.C:206 for it. (fixed 4ac62b0: now says max hit points through MaxHitPoints, cites EOV0.C:896 and E0A72.C:206, and notes the E0DE8.C:132 char conflict.)
- [x] must-fix src/EOV0.C:104: the comment on `gDD10` says "the min damage ovl0:135D recomputes (src/E08E6.C)", but src/E08E6.C uses `gDD10` as a 0/1 flag: DamageObject sets it to 1 (src/E08E6.C:820) and the routine at 08E6:0E48 clears it (src/E08E6.C:781, `gF518 = gDD10 = 0`). The value ovl0:135D writes is `MinHitPoints` (src/EOV0.C:898, src/E0A72.C:231). Say what ovl0:135D writes, and cite E0A72.C, not E08E6.C, for it. (fixed 4ac62b0: now says min hit points through MinHitPoints, cites EOV0.C:898 and E0A72.C:231, and notes the E08E6.C:784/823 flag use.)
- [x] must-fix src/EOV0.C:417-418 (DrawCharPanel comment): "the ratio of m0D against m0F" is wrong. `FormatStrengthPercent` (src/E0A72.C:263-275) formats the score and the percentile as text `"%d/%02d"` (or `"%d"`), it does not compute a ratio. Say the strength string "score/percentile". (fixed 4ac62b0.)
- [x] must-fix src/EOV0.C:421-422 (DrawCharPanel "Called by"): the list omits `Ovl0_1ADD` (ovl0:1ADD), which calls `DrawCharPanel(gDCB0, 3)` at src/EOV0.C:1038 and 1063. Add it. The packet's caller list agrees. (fixed 4ac62b0: added 1ADD.)
- [x] must-fix src/EOV0.C:320-321 (FlashPanelIcon comment): "draws it in its selected form (mode 1)". The code is `DrawPanelIcon(index, 1, 0)` (src/EOV0.C:325). The 1 is the second parameter, `rowBase` (the row offset of the selected cell), not `mode`; mode is the third argument, 0. Say "row + 1 (the selected cell row)" and "mode 0". (fixed 4ac62b0: now says rowBase 1, mode 0 for both draws.)
- [x] should src/EOV0.C:421: "each in its plain form, mode 2": `DrawPanelIcon(...,0,2)` (src/EOV0.C:457-469) draws at the icon's own origin without hiding the pointer. That is not the "plain" form that FlashPanelIcon's comment calls mode 0 (src/EOV0.C:325-328). Reword so "plain" is not used for two different modes. (fixed 4ac62b0: reworded to "drawn through DrawPanelIcon mode 2, at the icon's own origin with the pointer left shown".)
- [x] must-fix src/EOV0.C:497 (PickPartySlot): "only at the ends (or with the mouse disabled), slides the slot". The code is `if (pos == 0 || !gMouseDisabled)` (src/EOV0.C:569): it slides when `pos` is at an end or the mouse is enabled, and steps `pos` otherwise. Replace "disabled" with "enabled" (or with the negated form). (fixed 4ac62b0: now "slides ... when `pos` sits at an end (or the mouse is enabled), else steps `pos`".)
- [x] must-fix src/EOV0.C:500 (PickPartySlot "Called by"): "Called by ovl0:0DDC and ovl0:0EE5." Only Ovl0_0DDC calls it (src/EOV0.C:638, 659). Ovl0_0EE5 does not. Remove 0EE5. (fixed 4ac62b0.)
- [x] must-fix src/EOV0.C:627-632 (Ovl0_0DDC): "the enter key (0x61) and key 0x8004 flash icon 6 and keep looping" is wrong. Those keys set `key = 1` (src/EOV0.C:662-666), which reaches `return 0` (src/EOV0.C:676-680). They end the loop. (fixed 4ac62b0: comment now says they set key to 1, which draws the current row list and returns 0.)
- [x] must-fix src/EOV0.C:627-632 (Ovl0_0DDC): "any other key exits after drawing the current row list" is wrong. The default case sets `key = 0` (src/EOV0.C:667-669), and `if (key == 0) continue;` (src/EOV0.C:676-677) keeps the loop going. (fixed 4ac62b0: now "any other key sets key to 0, so the loop continues".)
- [x] must-fix src/EOV0.C:627-632 (Ovl0_0DDC): "keys 0x8001/0x8002/0x8003 ... run ... the race picker (Ovl0_1ADD) or the item list (ovl0:0A5E)". Ovl0_1ADD edits the seven ability/stat fields (its own comment, src/EOV0.C:1026-1027; the call at src/EOV0.C:654). ovl0:0A5E is the party-position screen (src/EOV0.C:490-491), not an item list. Fix both names. (fixed 4ac62b0: now "edit the seven ability/stat fields (Ovl0_1ADD) or pick the party slot (ovl0:0A5E)".)
- [x] must-fix src/EOV0.C:630-631 (Ovl0_0DDC "Called by"): "Called by no direct caller found". CreateCharacter calls it (src/EOV0.C:1305, packet caller list ovl0:23EC). Name CreateCharacter as the caller. (fixed 4ac62b0.)
- [x] must-fix src/EOV0.C:683-689 (Ovl0_0EE5): "the enter/left/right keys flash icon 9". The keys in that case group are 0x2b, 0x3d, 0x61, 0x6e and 0x8006 (src/EOV0.C:719-724), which are enter/confirm keys. The left/right keys are 0x5c/0x66 and 0x60/0x62, handled by the separate cases (src/EOV0.C:736-743), and the comment says those correctly. Replace "left/right" with the confirm keys. (fixed 4ac62b0: now "the enter/confirm keys 0x2b/0x3d/0x61/0x6e".)
- [x] must-fix src/EOV0.C:859-860 (EditStatField): "`percent` ... (only read for index 0)". The code reads it for every index: `count = *percent;` runs unconditionally (src/EOV0.C:893). Callers pass a null far pointer for index 1..6 (src/EOV0.C:1045-1060). Say it is read on every call but used only for index 0. (fixed 4ac62b0.)
- [x] should src/EOV0.C:864-867 (EditStatField clamp): "A changed value is clamped (index 0 to gOutDC98[0] & 0xff and its percentile reset, ...)" omits the lower bound (`gOutDD04[0]`, src/EOV0.C:961) and the rule that strength above 0x12 moves into the percentile (src/EOV0.C:951-958). Say both. (fixed 4ac62b0.)
- [x] should src/EOV0.C:866-867 (EditStatField): "index 4/3/6 refresh the derived hit points (m1B/m1C from gDCA4/gDD10) or m1D". Index 6 only sets `m1C = amount` (src/EOV0.C:974-975). Index 4 recomputes the hit points, and index 3 sets m1D. Describe index 6 correctly. (fixed 4ac62b0: now "index 6 copies it to m1C, index 4 recomputes the derived hit points ... or index 3 the armour class m1D".)
- [x] must-fix src/EOV0.C:1319 (InitPanelScreens): "ten row cells gPtrsFD54 (0, i*4)" gives (x, y) as (0, i*4). The code is `gFn8082(i * 4, 0, 4, 0x20, gOverlayBuf)` (src/EOV0.C:1335), so x = i*4 and y = 0. Swap the tuple. (fixed 4ac62b0.)
- [x] should src/EOV0.C:1320: "in video modes 0 and 1, blits gOverlayBuf with f19EC_002A and clips the panel". The else branch also blits (`f19EC_002A(3, 2, ...)`, src/EOV0.C:1341). Say the two modes take the (3,4) blit with a clip, every other mode the (3,2) blit. (fixed 4ac62b0.)
- [x] should src/EOV0.C:1434-1436 (FreePanelBuffers) and src/EOV0.C:118: "the 0x43-entry table gPtrsFC8C". InitPanelScreens fills only 0x2c entries (src/EOV0.C:1329-1330). The free loop covers 0x43 (src/EOV0.C:1441). Entries 0x32..0x3b are the same storage as gPtrsFD54 (ds:FD54). Say what the 0x43 bound is, and that entries past 0x2c are not set by this unit. (fixed 4ac62b0: the table runs ds:FC8C up to the word gFD98 at ds:FD98, 0x43 far pointers; the comment says this unit sets only the first 0x2c and that 0x32..0x3b alias gPtrsFD54.)
- [x] must-fix src/EOV0.C:1401 (InitCharSlotValues): "whose class byte ... has bit 2 set". The test is `g1231[...] & 2` (src/EOV0.C:1408), which is 0x2. The same file says "bit 2" for mask -5, which clears 0x4 (src/EOV0.C:1190; PickClass comment at 1172-1173). Say "bit 1 (value 2)", or rewrite with the value. (fixed 4ac62b0.)
- [x] must-fix src/EOV0.C:1413 (ShiftCharFields): "copies the nine bytes at +0x0D ... +0x1B". The list is eight fields (0D, 0F, 11, 13, 15, 17, 19, 1B) and the code has eight copies (src/EOV0.C:1423-1430). Say eight. (fixed 4ac62b0.)
- [x] must-fix src/EOV0.C:1449-1451 (RunCharacterPanel): "the two function-key toggles (gDCB0 ^= 1, ^= 2) on the 0x5c..0x66 range" is wrong. `^= 1` is on 0x5c/0x66 and `^= 2` is on 0x60/0x62 (src/EOV0.C:1474-1481). Describe both pairs. Also "the enter/left/right keys (0x61, 0x2b, 0x3d)" (src/EOV0.C:1494): those are the enter/confirm keys, not left/right. Fix both. (fixed 4ac62b0.)
- [x] must-fix src/EOV0.C:1031 (Ovl0_1ADD "Called by"): "Called by ovl0:0DDC and ovl0:0EE5." Only Ovl0_0DDC calls it (src/EOV0.C:654). Ovl0_0EE5 (src/EOV0.C:690-757) has no call to it. Remove 0EE5. (fixed 4ac62b0.)
- [x] must-fix src/EOV0.C:1231 (PickAlignment): "or 0x6e when the mouse finished". `done` is also set by the keyboard keys 0x6e and 0x32 (src/EOV0.C:1259-1260), and the comment itself says so one sentence earlier. The return is 0x6e for those keys too (src/EOV0.C:1266-1268). Drop "when the mouse finished" or say "the mouse or the keys 0x6e/0x32". (fixed 4ac62b0: now "confirmed by the mouse or the keys 0x6e/0x32".)
- [x] should src/EOV0.C:1177 (PickClass "Called by"): "Called by ovl0:23EC for the race member." CreateCharacter calls it for the class step (src/EOV0.C:1294, `PickClass(m1F)` returns m20). "The race member" is unclear: it is the new character's class pick, given the race. Reword. (fixed 4ac62b0.)
- [x] should src/EOV0.C:1274 (CreateCharacter): "each step that the picker reported as confirmed (0x6e, the mouse's accept) steps the index back". Only steps 1 and 2 can be confirmed. Step 0 (PickOption) sets no `confirmed` (src/EOV0.C:1290-1291). Say so. (fixed 4ac62b0.)
- [x] should src/EOV0.C:1154-1155 (FindNthSetBit): "the index (0-based) of the nth set bit". `remaining = n` and the break at `remaining == 0` (src/EOV0.C:1161-1166) make n 1-based: n = 1 is the lowest set bit. Say "the nth set bit (n = 1 is the lowest)". (fixed 4ac62b0.)
- [x] should src/EOV0.C:397 (DrawCharName): "(0x3c - width * gFontWidth) / 2 for a name `len` characters wide". The code is `(0x3c - gFontWidth * len) >> 1` (src/EOV0.C:408). `width` is undefined in the comment; say `len * gFontWidth` (pixels). (fixed 4ac62b0.)
- [x] should src/EOV0.C:1075-1076 (RollCharacterStats): "clamped up to gOutDD04 and down to gOutDC98, and when the strength roll is exactly 18 the low byte gets a percentile roll". The code clamps up (src/EOV0.C:1092-1093), then ORs the percentile in (src/EOV0.C:1094-1095), then clamps down (src/EOV0.C:1096-1097). The order matters for the 0x12 case; state it. (fixed 4ac62b0.)
- [x] should src/EOV0.C:100-102 and 149: the prototypes name the first parameter `who` (RollHitPoints, MaxHitPoints, MinHitPoints) and `level`/`percent`/`value`. src/E0A72.C:180, 206, 231 name it `classIndex` (the class index, passed `gChars[].m20`) and `m17`. src/E0A72.C:263 names the strength parameters `score` and `percent`. Rename to match E0A72.C, or cite why the names differ. (fixed 4ac62b0: renamed to classIndex, m17 and score.)
- [x] should src/EOV0.C:166 (gDrawText): the prototype names the parameters `a, b, c, d`. src/E082C.C:65 declares the same far pointer with `x, y, colour, extra`, and the two call sites in this file are colour/extra (src/EOV0.C:410-411, 1020). Use the E082C.C names. (fixed 4ac62b0.)
- [x] should src/EOV0.C:119 (gDCB0): "a character index read by ovl0:11BA (Purpose unknown)". It is the current character index for the whole panel: RunCharacterPanel writes it (src/EOV0.C:1476-1490), and every character routine indexes `gChars[gDCB0]`. Say that, and keep "Purpose unknown" for what the value means beyond "current character". (fixed 4ac62b0.)
- [x] should src/EOV0.C:57 (CharRec `pad00`): named as padding, but ovl0:26C6 writes the character index `i` into it (src/EOV0.C:1368). A name that says "pad" claims the byte is unused. Rename it (for example `index00`) or note the evidence. (fixed 4ac62b0: renamed to `index00` with the evidence at src/EOV0.C:1368.)
- [x] should src/EOV0.C:79 and 116: the struct comment says m20 "indexes g1231 (src/E0DE8.C)" and the global comment at line 116 says g1231 is "(src/E0BD2.C)". Cite one owner for g1231 and make both comments agree. (fixed 4ac62b0: both now cite E0BD2.C, the owner the packet and src/EOV1.ASM:92 also name.)
- [x] should src/EOV0.C:1131: `}/* ovl0:1F66:` puts the closing brace of BuildCombatTables and the next comment on one line. Start the comment on its own line like every other function here. (fixed 4ac62b0.)
- [x] should src/EOV0.C:4: the header says the unit shows "the status/rest screens". Nothing in this unit draws a rest screen; it draws the party-status report (DrawPartyStatus, src/EOV0.C:375-395) and the report lines. Name what it does. (fixed 4ac62b0: the header now names the party-status report and the report lines.)
- [x] should src/EOV0.C:103: type conflict on `ds:DCA4`. This unit declares it `int` and EditStatField writes hit points into it (src/EOV0.C:896). src/E0DE8.C:132 declares the same byte as `char` ("the object acting this step, set by 0359"), and E0DE8.C:342 sets it to a target. The two units give the byte different meanings. Not decidable from this file; the owner of E0DE8.C should say which unit's use is the right one. (fixed 4ac62b0: the comment now records both meanings and leaves the decision to E0DE8.C; this unit keeps `int` because it stores a MaxHitPoints result.)
- [ ] note src/EOV0.C:21: `g3A0` (0000:03A0) is described as "a flag in the resident segment (Purpose unknown)". That is fine as a placeholder. The address is outside this unit's overlay; the next reader should not look for it in the overlay buffers.
- [ ] note routed to the tools owner (agent tools are not on this branch): build/packets/eov0.md "Globals src/ already names" lists `ds:8082` as `x`, while the file names it `gFn8082` (src/EOV0.C:16) and src/EOV3.C:37, src/EOV5.C:171 use the same name. `make rename` cannot settle it: tools/agent/packet.py:109 (`known_names`) reads the first word before `(`/`[`/`;`/`=`/`,` on each annotated line, and the function-pointer declaration `void far * (far *gFn8082)(int x, ...)` yields the parameter `x`; the registry is not consulted. Same cause as the e07c8 packet note (tooling).
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

Round 1 fixes: commit 4ac62b0 ticks all 20 must-fix items and every should item except the two
notes left for other owners. Comment-only: no code changed, `make match FILE=src/EOV0.C` still 35 of
35 and `make readability UNIT=eov0` still 0 findings.

Round 2 fixes: commit cd1070f ticks the one must-fix and every should item, plus the in-file note
(PickPartySlot `gFC8A`). Comment-only: no code changed, `make match FILE=src/EOV0.C` 35 of 35 and
`make readability UNIT=eov0` 0 findings. The two notes for other owners (EOV1.ASM:97, the
E0BD2/E0DE8 g1231 owner) stay routed; the `ds:8082 x` packet note is routed to the tools owner (it
is the packet parser at tools/agent/packet.py:109, not this unit's source).

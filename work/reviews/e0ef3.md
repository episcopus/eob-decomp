# Review: e0ef3

Reviewer: rev-haiku
Commit reviewed: 50a428c74364a60f97ecc541728e4cbae3720042 (main when I started; src/E0EF3.C is unchanged since a729f44 on both main and review/e0ef3)

Playbook 70. Judge what a matching file can still get wrong: names, types, comments, structure.
The matcher already proved the bytes; do not re-prove them, read for meaning.

## Findings

Each finding is a checkbox. `must-fix` blocks `make review-pass` until a decompiler ticks it off
(with the commit that fixes it); `should` and `note` do not block.

### Must-fix: wrong claims, wrong names, stale text

- [ ] must-fix src/E0EF3.C:199: gFC74 comment says "saved with gFC76 by 0EF3:026F and restored". The save is 0EF3:28ED (src/E0EF3.C:2090-2091); 0EF3:026F only restores them (:524-525).
- [ ] must-fix src/E0EF3.C:252 and :790: gFnEFBA is called "a redraw hook". 0EF3:2676 sets it from the cast spell's apply callback (:2038, `gFnEFBA = spell->apply;`) and runs it there (:2076); 0EF3:08CF runs whatever spell is current (:807). Say so (the Spell.apply comment at :160 already does).
- [ ] must-fix src/E0EF3.C:211: gF542 is called "a message buffer passed to RedrawPartyPanel". src/E0ACD.C:72 says it is the action record passed to RedrawPartyPanel. This unit passes it as `struct ActRec far *` (:2082, :2143) and RedrawPartyPanel reads its +4 word (:1969).
- [ ] must-fix src/E0EF3.C:316 and :891: HalveSaveDamageB is declared `char far *type` here, but defined at src/E0DE8.C:782 as `int HalveSaveDamageB(struct Obj28 far *objRec, int value)`. The call at :891 casts `&gObjs[*list]` to `char far *` to fit the wrong declaration. Declare the Obj28 pointer and drop the cast (the e0de8 review routed this item here).
- [ ] must-fix src/E0EF3.C:409-412 (SortSpellSlots comment): the loop does not swap "the first two bytes of a row". It bubble-sorts all six bytes of each row, comparing row[j] with row[j+1] for j = 0..4 (:429-445), so "a row's first two bytes are its sort key" is also wrong.
- [ ] must-fix src/E0EF3.C:684-685 and :712-713: gFB44 is read as "while a status line is up" and "no panel is open". gFB44 is a screen or list mode byte: 0 is the plain character screen, 1 and 2 are other modes (src/E1547.C:81 "a screen mode: 0 the plain character screen"; src/E0ACD.C:26; src/E0ACD.C:280-289 "list mode byte gFB44"). The status line is gStatusLine (ds:8115). The fixed (0xB5, 3) branch runs when gFB44 is nonzero (:698-704).
- [ ] must-fix src/E0EF3.C:1253-1256 and :1462-1464 (UpdateCellObjectsBit1 and UpdateCellObjectsBit2): "the object's character record" is wrong. The record is struct Entry27 in gTable1A3B (ds:1A3B), indexed by obj->b0 (:1272 and :1481; Obj28.b0 at :109). It is not a character record (gChars).
- [ ] must-fix src/E0EF3.C:1331-1334 (ClearObjectsFacingCell): the same wrong "character record" at :1332 and :1334 (the code uses gTable1A3B[obj->b0] at :1348). "Called by the wand/turn-undead paths" has no evidence: the packet finds no direct caller of 0EF3:1702, and the wand and turn-undead paths (0BD2:0EE8, 0D1C:0A43) call 0EF3:06E7 FlashViewDamage (see the comment at :745-746), not this function.
- [ ] must-fix src/E0EF3.C:1090-1091 (ClearAttackEffect "Called by 0DE8:0D5E"): ComputeArmourClass (0DE8:0D5E, src/E0DE8.C:704-760) calls GetClassSlot24Value and does not call 0EF3:1176, and no src/ file names 1176. Name the real caller with its evidence, or say it is reached through a table and cite the table.
- [ ] must-fix src/E0EF3.C:1557-1559 (UndoBandage "Called by 0BD2:0D2D and 0EF3:1F0B"): 0EF3:1F0B calls ClearPoisonEffect (0EF3:1CE8, :1690), not UndoBandage (:1687-1693). The packet finds no direct caller of 1C45, and no src/ file other than this one names UndoBandage or 0EF3:1C45. The caller list looks copied from ClearPoisonEffect (1CE8, :1588-1589).
- [ ] must-fix src/E0EF3.C:646-648 (GetClassSlot24Value "Called by the panel actions and 0DE8:0763"): the only direct caller in src/ is ComputeArmourClass (src/E0DE8.C:757-758). UseItem (0DE8:0763, src/E0DE8.C:484) does not call it; the packet lists 0DE8:0763 only as a caller of 0EF3:12A1 (ClearEffectFlag4).
- [ ] must-fix src/E0EF3.C:1612-1613 (SetMemberLevels, "+0x23 level byte"): src/E0ACD.C:108 documents +0x23 as "a hunger/food value clamped to 100 by EatFood (07E0)", and src/E0ACD.C:623-626 is EatFood clamping rec->m23 at 100. The function fills every member's food to 100 (the same write as 0BD2 case 5, src/E0BD2.C:711). Rename it (`make rename`) to a name the food evidence supports, and cite that evidence. Also give the CharRec m23 field (:30) its meaning.
- [ ] must-fix src/E0EF3.C:1536-1539 (ApplyAidEffect): "Either way it applies a 1-turn effect" is wrong. The ApplyTimedCharEffect call (:1552) is inside the success branch; the else branch (:1553-1555) only says "The aid spell fails!" and applies nothing.
- [ ] must-fix src/E0EF3.C:1768-1770 (RunMelee0): the "DRAFT: ... make match differs at the frame" note is stale. The function is L2 and the unit matches (`make match FILE=src/E0EF3.C`: 104 of 104). Delete the DRAFT paragraph.
- [ ] must-fix src/E0EF3.C:2272-2275 (AnimateConeOfCold): "clears bit 1 of ds:849E" is wrong at the end of the function. It clears gFlags849E bit 1 on entry (:2316) and sets it again on exit (:2429, `gFlags849E |= 2`).
- [ ] must-fix src/E0EF3.C:398-401 (gFC62 and gFC66): "0EF3:2A53's and 0EF3:2EBC's loop index" and "state". 0EF3:2EBC uses its own locals `i` and `state` (:2282-2303, :2365). Both globals are used only in 0EF3:2A53 (:2206-2225); no other src/ file names them.
- [ ] must-fix src/E0EF3.C:213-214, :1438 and :2278 (gText5737): the "gText" prefix is this file's prefix for strings (gText57A3 and the rest), but this is a word table of animation frames (:2415 reads it as `frameTable[row6[i] >> 8]` of int, and the comment at :213 says so). No other src/ file names ds:5737. Rename it to a placeholder (for example g5737) until the frame table has a name the evidence supports.
- [ ] must-fix src/E0EF3.C:1838-1840 (OpenSpellPanel): "Restore the panel rectangle ... when gFlag is clear". The code at :1859-1860 calls gSaveRect, which saves (gSaveRect is ds:80AE, src/E0BD2.C:112; the restore is gRestoreRect, ds:804E, used at :1955). Say "save".
- [ ] must-fix src/E0EF3.C:1869-1870 (DrawSpellPanel): "3/1/-1 style (gA9C4/gA928/gFB7B)" invents values. For the other rows the colours are the variables gA9C4, gA928 and gFB7B (:1902-1904); the numbers in the comment belong to the gGraphicsChoice == 0 branch (:1896-1900) only.
- [ ] must-fix src/E0EF3.C:1874 (DrawSpellPanel "clips a 9 by 0x35 box"): ClipRect(9, 0x7a, 9, 0x7a, 0xd, 0x35, 2, 0) (:1942) has width 0xD and height 0x35; 9 is the x origin. Fix the size.
- [ ] must-fix src/E0EF3.C:1974 (f0EF3_25BD "runs 0EF3:2295 (the item panel)"): 0EF3:2295 is DrawSpellPanel, the spell panel (its own comment at :1867-1868 says so).
- [ ] must-fix src/E0EF3.C:2085-2087 (StartPanelAction "Called by the item-use and turn-undead paths"): the packet's callers are 0BD2:0EE8 (UseWandItem, src/E0BD2.C) and 0BD2:1001 (UseItemSlot, src/E0ACD.C). There is no turn-undead caller.
- [ ] must-fix src/E0EF3.C:1017-1018 and :1033-1036 (DrawMemberIcons): the comment says the far sprite is `gThrownIcon0[abs(icon) - 1]`, but the code indexes `gIconTable[abs(icon)]` with no minus one (declaration at :227-229).

### Should: clarity, type fit, incomplete claims

- [ ] should src/E0EF3.C:62-63 and :556-558: CreateItemType's parameters word00 and word02 are `unsigned char` (:556), but the comments and the ItemType fields call them words. Name and type disagree; check with the matcher before changing the type, since int and char can compile differently.
- [ ] should src/E0EF3.C:556-558: parameters named m05, m0A, m0B, m0C and word0E name struct offsets, not what they hold.
- [ ] should src/E0EF3.C:1098-1103 (CreateItemForEffect0): "0EF3:035A creates the drawing node" contradicts 035A's own comment (:551-555: it allocates the first free item-type record). The local `slot` holds the item-type index that 0EF3:0418 stores in ItemRecord.m02 (:612). Rename `slot` to something like itemType and say "item type".
- [ ] should src/E0EF3.C:1308-1310 and :1598-1600 (CreateItemForEffect6 and 4): the class-slot halving is the item type argument in 1656 (:1316), but in 1D38 it is the effect duration (:1609), not the item. The 1D38 comment copies the 1656 sentence. "A different row" is vague.
- [ ] should src/E0EF3.C:625-627 (GetClassSlot1Value): "for the current member when the action is not yet 7". who == -1 means no member (:635), not the current member.
- [ ] should src/E0EF3.C:159: the Spell.flags comment says "bit 1 by 0EF3:2676", but 2676 also tests bit 0 (:2064), bit 2 (:2077), bit 3 (:2073) and bits 1 and 2 together (:2071).
- [ ] should src/E0EF3.C:513-517 (FinishMemberEffect): "It clears a bit at index g8DCE" but :521 XORs (toggles). "Called by 0EF3:08CF" omits 0EF3:2676 CastSpell (packet row 22: callers 08CF and 2676).
- [ ] should src/E0EF3.C:715 (FlashPartyMembers): the callers omit 0EF3:2676 CastSpell (:2072, :2074; packet row 32).
- [ ] should src/E0EF3.C:1045-1046 (EnterSwapMode): the comment omits RedrawCharacters (:1052).
- [ ] should src/E0EF3.C:1071-1072 (RunMelee1d4): the call is 1d4 with bonus +1 and n = (value - 1) >> 1 (:1075). "Half the member's class-slot 1 value" describes only n, and not exactly.
- [ ] should src/E0EF3.C:1569-1571 (ApplyConditionEffect): "say the message at ds:58AF" but the code calls DrawText (:1578), not SayAndPlaySound. "A 0x7FF8-tick effect" is (1 + class-slot value) times 0x7FF8 (:1576).
- [ ] should src/E0EF3.C:1685-1686 (CureOrReportPoison): "when the current record is alive (m01 nonzero)". Alive is m01 bit 0 (src/E1547.C:15 "bit 0 alive"; the same test at :788 and :730). The code tests m01 != 0 (:1689). Say which test it is.
- [ ] should src/E0EF3.C:1492-1493 and :1505 (ToggleSwapFlag, ClearSwapFlag): gFBD2 is a bit set. Bit 0 is EnterSwapMode's swap mode (:1049), bit 4 is cleared here, and src/E0DE8.C:198 says bits 4 and 5 raise a d20 roll. "The display byte" is not evidenced. ToggleSwapFlag sets the bit once (:1496-1502); it does not toggle it.
- [ ] should src/E0EF3.C:1175-1176 and :1196-1198 (DispelMagic): "the effect flags (cleared twice) and the record's +0xE3 long" names one field twice. The duplicate `gCurRec->mE3 = 0;` (:1197-1198) needs a reason in the comment (a source-form choice that matches the original's two stores) or one line deleted if the matcher allows.
- [ ] should src/E0EF3.C:1256 and :1464 (UpdateCellObjectsBit1 and 2): "Returns 1 when any object was listed". found is set only for objects that 0D1C:0BAB does not reject (:1268-1271); say "not rejected".
- [ ] should src/E0EF3.C:1331-1335 (ClearObjectsFacingCell): "the value 0A72:0006 gives for the +2 index". The +2 byte is the `limit` argument (:1350, `GetIndexedByte(0, rec->m02, 4)`), which 0A72:0006 clamps to g341E[index] (src/E0A72.C:73-80). Say that.
- [ ] should src/E0EF3.C:2085-2087 (StartPanelAction): the comment's `a`, `b` and `c` are not the parameters (who, action, kind; :2088).
- [ ] should src/E0EF3.C:2100-2105 (HandleSpellList): "until it lands on a nonzero slot", but the code tests > 0 (:2118, :2129). "Clamping it into 0..6" is ClampOrWrap with its last argument set to 1 (:2128), and that name says clamp or wrap. "Reached through the list node's callback30" has no evidence in this unit or in src/.
- [ ] should src/E0EF3.C:1838 (OpenSpellPanel): "first nonzero slot", but the test is > 0 (:1855).
- [ ] should src/E0EF3.C:264-265 (gNameTable): declared `char far *`, but used as `char far **` (:1928, :1934) and set from g12E0 and g1278 (:1847, :1850), which are arrays of far pointers (:266-267). Declare the element type.
- [ ] should src/E0EF3.C:1947-1948 (RedrawPartyPanel): "clear the key-flag gFlag". No source says gFlag is a key flag: src/E0ACD.C:143 says it picks a table set, and src/E0ACD.C:73 says a redraw is gated by it. Say what the evidence shows.
- [ ] should src/E0EF3.C:2150-2161 (f0EF3_2A53): the comment does not say that the function clears bit 1 of ds:849E on entry (:2182) and sets it on exit (:2259), as the AnimateConeOfCold comment does.
- [ ] should src/E0EF3.C:2263-2278 (AnimateConeOfCold): "the animation value word follows it" is unclear. "With no held particle under the pointer" joins two conditions (row7 hold and handle, :2418); name each.
- [ ] should src/E0EF3.C:209: gFileNameBuf has no declaration comment. re/globals.toml (ds:FE6E) records it as the scratch text buffer that EOV2 also formats file names into, and this unit uses it for messages. Add that to the comment; the name is registered and stays.
- [ ] should src/E0EF3.C:256: gFn804A has no declaration comment. The 06E7 comment says it "restores the cells" (:765); src/EOV1.ASM:110 calls it "far blit routine" and src/E0ACD.C:315 uses it. Give the declaration the blit meaning with that cite.
- [ ] should src/E0EF3.C:991-992 (FixArmourClass): "raise its +0xDF penalty byte to eight over" describes a set (:1002), to the class-slot 1 value plus 8. Say "set".
- [ ] should src/E0EF3.C:2015-2016 (CastSpell comment, "in context `kind`"): vague; kind is the slot passed to UseItemRow (:2060), and HandleSpellList passes 0 (:2141). Say which.

### Notes for the next reader

- [ ] note src/E0EF3.C:2105: the QUIRK address 0EF3:2969 cannot be checked from the source. The decompiler should confirm it in the listing before the next review.
- [ ] note src/E0EF3.C:1226-1227 (MarkLivingMembers), :1539-1540 (ApplyAidEffect) and :1420-1421 (CastConeOfCold): several comments say a function is "reached through the spell table" or "called through a table". The packet shows no direct caller for these. The table evidence exists only for DispelMagic (record 12) and CastConeOfCold (record 24), in re/renames.toml. Cite the table for the others when it is known.
- [ ] note src/E0EF3.C:191: the gCurMember comment says "redrawn last". No source file shows this; check it or drop it.
- [ ] note cross-unit: the e0de8 routed item (src/E0EF3.C:316 and :891) is a must-fix above. The g1231 owner conflict (src/E0BD2.C:102 against src/E0DE8.C:124) is not this unit's.

## Summary

The unit is mostly understandable from its source alone. Names of the form `f0EF3_2A53`, `f0EF3_25BD`, `EmptyFunction*` and the `g` globals with a number are honest placeholders, and the `Purpose unknown:` notes on f0EF3_2A53, f0EF3_25BD and DrawMemberIcons are acceptable. What fails the review is the text that claims more than the evidence: several callers that the packet does not show (ClearAttackEffect, UndoBandage, ClearObjectsFacingCell, GetClassSlot24Value, StartPanelAction), two record identities (struct Entry27 called a "character record" in 1530, 1A30 and 1702), the meaning of gFB44 (a mode byte, not a status line), the food meaning of +0x23 (SetMemberLevels), a misdescribed branch in ApplyAidEffect, a stale DRAFT note on a matched function, a word table named gText5737, a save called a restore, and a wrong declaration of HalveSaveDamageB (the e0de8 routed item). Evidence: `make match FILE=src/E0EF3.C` 104 of 104 (13219 bytes), exit 0; `make readability UNIT=e0ef3` 0 findings. Nothing escalated: no matcher, spec or pin question. `make review-pass` is not run: there are open must-fix items.

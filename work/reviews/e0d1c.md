# Review: e0d1c

Reviewer: rev-haiku
Commit reviewed: afdb33a (main; src/E0D1C.C and re/symbols.toml are identical on review/e0d1c at 7d4b343)

Playbook 70. Judge what a matching file can still get wrong: names, types, comments, structure.
The matcher already proved the bytes; do not re-prove them, read for meaning.

## Findings

Each finding is a checkbox. `must-fix` blocks `make review-pass` until a decompiler ticks it off
(with the commit that fixes it); `should` and `note` do not block.

- [x] must-fix src/E0D1C.C:39: gPartyCell is described as "the global sound volume (src/E177D.C)". E177D.C:92 and EOV4.C:524 say it is the party's map cell, and this unit uses it as a cell: 0D1C:035A indexes the nine-byte cell records (`gB894 + gPartyCell * 9 + 5`) and passes it as the InsertListItem key, and 0D1C:09CB passes it as the second point of DirectionFromPoints (17A6:0314 takes packed map cells). Say "the party's map cell" with those addresses. (Cross-unit conflict: see the note on E0ACD.C:55.)
- [x] must-fix src/E0D1C.C:311: f0D1C_02DC's comment inverts the test. The code returns gFBCE when the mouse is enabled (`if (!gMouseDisabled) return gFBCE;`, line 315) and the slot otherwise. The comment says "gFBCE when the mouse is disabled". SetCurrentItem (0304, lines 319-321) states it the right way round, so the two comments now disagree.
- [x] must-fix src/E0D1C.C:507: f0D1C_07DB with `flag` 0 does not return 1 "when slot 0 holds an item and ItemClassAllowed also accepted it". Line 525 is `return slot0 == 0 || result0 != 0;`: it returns 1 when slot 0 is empty or the item is allowed.
- [x] must-fix src/E0D1C.C:57 and :690, :693: ItemInfo m00 and 0D1C:0BEB say "bit 4", but line 693 tests `m00 & 4`, which is bit 2 (the numbering that Entry27 m0E "bit 2" on line 79 uses for the same mask). Fix both comments to bit 2. The same wrong claim is in src/E0DE8.C:100 (for that unit's reviewer).
- [x] must-fix src/E0D1C.C:624: TurnUndead says it lists "the objects whose ds:8DD2 record's m02 equals key". ds:8DD2 holds the item records (Entry14). f08E6_17FD fills its list with gObjs indices (ds:8A82 object records) whose m02 equals key (declaration comment, lines 116-118). Say the ds:8A82 object records.
- [x] must-fix src/E0D1C.C:562-564 (f0D1C_0903) and :630-631 (TurnUndead): the comments say "DRAFT: matches every byte except ...". Both functions are matched now: `make match` reports ok for each (26 of 26), and re/symbols.toml records e0d1c_0903 as matched 2026-11-02 and e0d1c_0a43 as matched 2026-11-01 (the m0F bit-test idiom is already explained there and in the Entry27 comment, lines 80-81). Delete the DRAFT sentences, including the "(below)" in the TurnUndead one, which points at nothing.
- [x] must-fix src/E0D1C.C:125: RedrawMember is declared `void RedrawMember(int dir)`. Its definition at 1547:0991 is `RedrawMember(int who)` (src/E1547.C:493), its comment says the argument is the member index, and every call in this unit passes a character index (0D1C:003F, 0D1C:0583, 0D1C:0304, 0D1C:01CA). `dir` claims a direction. Rename the parameter `who`. Same wrong name in src/E08E6.C:147, src/E0ACD.C:37, src/E0EF3.C:307 (cross-unit).
- [x] must-fix src/E0D1C.C:146: `DirectionFromPoints(int obj, int volume)` is wrong on both names. 17A6:0314 takes two packed map cells (`y * 32 + x`, E17A6.ASM:110-116), its callers pass (an object's cell, gPartyCell), and 0D1C:09CB passes (obj->m02, gPartyCell). `volume` is not a volume. Name both as cells (for example `fromCell`, `toCell`) and give the extern the 17A6 address comment it already has. The same parameter name is in src/E08E6.C:138, which the E08E6 review already logged.

- [x] should src/E0D1C.C:175: SelectCharacter(int value). The argument is the selected character index that gA99A holds (the rest of the unit says `who`); rename it `who`, and the comment at 170-174 follows.
- [x] should src/E0D1C.C:139: `FindClassSlot(int value, int classIndex)` names its first argument `value`. E0A72.C:89 names it `slotId`, and the unit's calls pass a class-slot id (2 and 4, 0903). Use `slotId`.
- [x] should src/E0D1C.C:130: `ItemClassAllowed(int who, int slot)`. E0BD2.C:165 and :906 name the second argument `item`, and this unit passes an item index (07DB: `slot0 = gChars[who].m77[0]`). Use `item`, and in the 07DB comment (line 506) say ItemClassAllowed, not "f0BD2_11FD".
- [x] should src/E0D1C.C:507: the 16-byte ItemInfo records start at ds:A9F5 (gA9F5, line 63), so "indexes the 16-byte records at ds:A9F6" should say ds:A9F5 (A9F6 is +1, the m01 byte).
- [x] should src/E0D1C.C:313: f0D1C_02DC is the getter of the pair that SetCurrentItem (0304) sets, and SetMouseItem (E07A9.C:375-377) calls the setter. Its comment states what it returns, and E0ACD, E0BD2, E122E and E0EF3 all call it as "the current item". Name it GetCurrentItem (make rename), and drop "Purpose unknown:" from its comment.
- [x] should src/E0D1C.C:469-474: f0D1C_070F "draws the item icon of the ds:8DD2 record n at (x, y)" from gIconsDDE4 (the item icon pointers, E07A9.C), yet its comment says "Purpose unknown:". The name claim is supported: rename it DrawItemIcon (callers E0BD2.C:148, E1547.C:123 use the placeholder), and keep "Purpose unknown" only for the colour-row path if that is what is unknown.
- [x] should src/E0D1C.C:68 and :70: Obj28 +2 and +5 are named m02 and m05 here. E08E6.C:15 and :167 name them `id` (the cell the formula reads: `DirectionFromPoints(obj->id, gPartyCell)`) and `orient` (`obj->orient = (DirectionFromPoints(...) ^ 4) >> 1`, the same formula as line 613 here). Adopt `id` and `orient` (make rename), with the comments at 612-613 and 66-70 following.
- [x] should src/E0D1C.C:88-93: Row8 fields m0, m2, m4, m6 are the DrawBox rectangle (the comment says so, and lines 382-397 use them as x1, y1, x2, y2). Name them x1, y1, x2, y2.
- [x] should src/E0D1C.C:203: the 0D1C:0080 comment says "pass gFBCC as b", a name that does not exist here; the parameter is `page` (signature 205). Likewise 0C21 (lines 695-698) says "size w by h" and "when `a`" and "when `b`": the parameters are width, height, first, second.
- [x] should src/E0D1C.C:691: f0D1C_0BEB(int obj, int flag): `flag` is an index into gA9F5 (ItemInfo records), not a flag. Name it `item`. f0D1C_0BAB(int obj) takes an object index while 0D1C:09CB's `obj` is a pointer; name it `objIndex` or note it in the comment.
- [x] should src/E0D1C.C:446-449: the f0D1C_0670 comment labels its tests "Bit 1" through "Bit 16", but these are values of `mask` (1, 2, 4, 8, 16). Say "mask 2" and so on, so the reader does not confuse them with bit numbers (see the m00 finding above).
- [x] should src/E0D1C.C:420: "for attack 4, 0 or 1" is given only for the 3-or-5 case; the level-4 case subtracts for attack 4 or 1 only (lines 440-441). Say both.
- [x] should src/E0D1C.C:32: gF51A is described as "the selected row (src/E0ACD.C)". Inside this unit it is an index into the 27 m77 slots (02DC and 0304 index `gChars[gA99A].m77[gF51A]`), and 16 is the own-list head that 0D1C:035A drains (line 357). E0ACD.C:22-25 gives it a different meaning (an action-menu row 0..2) and 0x10 for the own list. Say what this unit proves, and cite the E0ACD disagreement.
- [x] should src/E0D1C.C:108: gFB44 "gates 0D1C:01CA's redraws", but 0D1C:0304 (line 329) also branches on it (RedrawMember vs DrawItemRow). Name both.
- [x] should src/E0D1C.C:56 and :98: ItemInfo "read by 0D1C:07DB and 0D1C:0BEB" also misses 0D1C:08B1 (line 552, m09). Entry14 "icon, read by 07A9:0141" also misses 0D1C:070F (line 478, `gTable8DD2[n].icon`).
- [x] should src/E0D1C.C:1-7: the unit comment has the address, size and purpose, but READABILITY asks for the whitepaper section too. Add it.

- [ ] note gPartyCell (ds:FBCA) has conflicting meanings across src/. E0ACD.C:55-59 calls it "the sound volume: SetSoundVolume (122E:0001) assigns it" and says E177D.C:92 and EOV4.C:524 read it as the map cell. This unit, E177D.C:92, EOV4.C:524 and E08E6.C:94 all use it as a cell. The corrected comment here rests on this unit's own use, not on E0ACD. Whether one word has two uses, or E0ACD is wrong, is for the E0ACD review or the human; it is not settled here.
- [ ] note DrawItemRow(int who, int slot, int one): 0BD2:0917 says "the item row of character who". This unit passes a character in 01CA (`DrawItemRow(who, slot, 1)`) but gF51A in 0304 (`DrawItemRow(gF51A, 1, 0)`, line 332), and E0ACD.C:704, E1547.C:579 do the same, and E122E.C:489 passes 0x10. One of the two readings is wrong. Belongs to the 0BD2 review.
- [ ] note Parameter names of shared callees disagree across units: EOV6.C:81-84 declares f0D1C_0670(member, what) and f0D1C_0163(member, what), while this unit uses who, mask, fieldB. Settle them at the rename step.
- [ ] note Placeholder names with an honest "Purpose unknown:" are accepted (f0D1C_0006, 006D, 0078, 0080, 00DA, 035A, 059B, 0670, 08B1, 0903, 09CB, 0BAB, 0BEB, 0C21 apart from the should on its name). The evidence for f0D1C_035A ("queues the character's items into the cell's record list") is enough for a name later, but it is not a finding now.

### Round 1 resolution (dec-deepseek-2, src/E0D1C.C commit 41d9d6b)

Comments, one rename pair and prototypes only, no code byte changed: `make match FILE=src/E0D1C.C`
26 of 26 (3265 bytes), `make readability UNIT=e0d1c` 0 findings. Each tick above is what changed.

- gPartyCell (:39): now "the party's map cell", with 0D1C:035A (the gB894 + gPartyCell * 9 + 5
  InsertListItem key), 0D1C:09CB (DirectionFromPoints) and E177D.C:92, EOV4.C:524; it names
  E0ACD.C:55 as the disagreeing reading.
- GetCurrentItem (was f0D1C_02DC, :310): "gFBCE when the mouse is enabled", the getter of the
  SetCurrentItem pair; renamed with `make rename` (registry and log updated).
- f0D1C_07DB (:505): flag 0 returns 1 when slot 0 is empty or ItemClassAllowed accepted it; the
  comment names ItemClassAllowed (was f0BD2_11FD) and ds:A9F5 (was ds:A9F6).
- ItemInfo m00 (:57), f0D1C_0BEB comment (:690) and body (:693): bit 2, not bit 4.
- TurnUndead (:622): lists the gObjs indices whose ds:8A82 object record's id/m02 equals key.
- DRAFT notes: deleted from f0D1C_0903 (:562) and TurnUndead (:630); both are matched
  (re/symbols.toml records 0903 matched 2026-11-02, 0A43 matched 2026-11-01).
- RedrawMember (:125): parameter `dir` -> `who` (the member index, E1547.C:493). The other units
  that declare it (E08E6.C, E0ACD.C, E0EF3.C) are theirs; this unit's call sites already pass an
  index.
- DirectionFromPoints (:146): `obj`/`volume` -> `fromCell`/`toCell` (two packed map cells,
  E17A6.ASM:0314).
- Shoulds: SelectCharacter `who` (comment and body); FindClassSlot `slotId`; ItemClassAllowed
  `item`; the ds:A9F5 index fixed; DrawItemIcon (was f0D1C_070F) named with `make rename` and
  "Purpose unknown" kept only on the colour-row path; Obj28 m02/m05 -> id/orient and the 09CB
  comment follow; Row8 m0/m2/m4/m6 -> x1/y1/x2/y2; gF51A says what this unit proves and cites the
  E0ACD disagreement; gFB44 also gates 0D1C:0304; ItemInfo read-by adds 0D1C:08B1 and Entry14.icon
  adds 0D1C:070F; the f0D1C_0080 "pass gFBCC as b" is now `page`; f0D1C_0C21 w/h/a/b are
  width/height/first/second; f0D1C_0670 says "mask 1".."mask 16"; f0D1C_059B gives both the
  3-or-5 (4, 0 or 1) and level-4 (4 or 1) attack cases; f0D1C_0BEB `flag` -> `item` with its
  comment; f0D1C_0BAB `obj` -> `objIndex` with the pointer distinction; the unit header cites
  whitepaper 2.1.1.
- Notes: left as written. gPartyCell's E0ACD conflict is for the E0ACD review or the human;
  DrawItemRow's first argument is for the 0BD2 review; the f0D1C_0670/f0D1C_0163 parameter names
  EOV6 uses (`member`, `what`) are for the rename step across units.

## Summary

Every function is matched (26 of 26, 3265 of 3265 bytes) and `make readability UNIT=e0d1c` reports 0 findings. The unit is understandable from its source for the named functions (FindCarriedSlot, RemoveCarriedItems, DrawRowList, TurnUndead, SkipStrings and the rest), and the placeholders are honest: most are "Purpose unknown:" with the callers and tables recorded, which the playbook accepts. What blocks the pass is eight wrong claims, not missing evidence: two comments that invert or misdescribe a result (02DC mouse test, 07DB flag 0), a wrong bit (ItemInfo m00, bit 2 not 4), a wrong table (TurnUndead's objects are ds:8A82, not ds:8DD2), two stale DRAFT notes on functions that now match, and two parameter names that claim the wrong thing (RedrawMember's `dir`, DirectionFromPoints' `volume`) plus the gPartyCell comment that calls the cell a sound volume. The shoulds are mostly names the unit already has evidence for (GetCurrentItem, DrawItemIcon, id/orient, x1..y2) and comments that name parameters the function does not have. Nothing escalated: no matcher, spec or pin question. The gPartyCell conflict with E0ACD.C is a cross-unit question, recorded as a note.

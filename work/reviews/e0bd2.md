# Review: e0bd2

# Round P6 re-review (2026-11-04), commit 24711ec

Reviewer: p6-rev-r2
Commit reviewed: 24711ec (P6 re-review; the P6 round read ce0608f)

Judged: the 10 open P6 must-fix items and the diff since ce0608f for src/E0BD2.C. `make match FILE=src/E0BD2.C`: 22 of 22 (4681 bytes), exit 0. `make readability UNIT=e0bd2`: 0 findings.

- [x] must-fix The item-record wording is fixed right in all ten places: `TakeItemFromSlot` uses `key = -1` and `value`; `InsertListItem` uses the owner `key`, `level`, `nextLink` and `prevLink`; `PushListItem` uses `nextLink`; `DrainListItem` uses `type`, `key`, `level` and `cellPos`; `DrawItemName` uses `nameIndex` and `identifiedNameIndex`; `AddDDEffect` uses `rec->m02`; `CopyItemToFreeSlot` tests `key`; `ApplyItemEffect` tests the character's `status` bit 1; and `UseWandItem` tests the Monster `cell`. Each matches the code and re/fields.toml.
- [ ] should src/E0BD2.C:713-714 (UseWandItem): the table index is now `gWandSpells[value]`, but the same sentence still says "the item's m0B is 0" and "when it is not 5". `m0B` is the old name for `value`. Make the three consistent when the file is next edited; it does not block.
- [ ] note The P6 round's other shoulds and notes remain (the `ApplyItemEffect` m0D/m0E wording, `gCursorSlot`, `gWallTypeFlags`, the panel colours, `gStagingPage`, `gViewDirty`, and the stale notes). They are non-blocking.

# Round P6 (2026-11-04), commit ce0608f

Reviewer: p6-rev-9
Commit reviewed: ce0608f367ed01b1b1df84e254eef04d1a734b27

Playbook 70. Judge what a matching file can still get wrong: names, types, comments, structure.
The matcher already proved the bytes; do not re-prove them, read for meaning.

Checked this round: `make match FILE=src/E0BD2.C` (22 of 22, 4681 bytes), `make readability UNIT=e0bd2` (0 findings).
Read against: the callers and definitions in src/E0ACD.C, E0D1C.C, E08E6.C, E0DE8.C, E0EF3.C, E122E.C, E1414.C,
E07A9.C, E177D.C, EOV5.C, and the registry rows in re/symbols.toml and re/globals.toml. No game run, nothing under eob/ touched.

## Round P6 (2026-11-04), commit ce0608f367ed01b1b1df84e254eef04d1a734b27

Judged only what the later-round rule asks: the names and comments the Phase 6 diff changed since 8c882ae in `src/E0BD2.C`, against the evidence in `re/renames.toml` and `re/fields.toml`. The diff is 218 insertions and 295 deletions: the private struct declarations are replaced by `#include "EOB.H"`, the globals and functions take their Phase 6 names, and the field accesses and comments follow.

The names are backed: `MemberIs` (0D1C:0670), `StepCell` (0D1C:00DA), `CanUseHand` (0D1C:07DB), `StartItemCast` (0EF3:28ED), `MoveMonsterToCell` (08E6:0297), `IdentifyListItems` (0BD2:040E), `GetHandOfSlot` (0BD2:1244), `gInventoryMember` (ds:A99A), `gCursorSlot` (ds:F51A), `gPanelMode` (ds:FB44), `gCurLevel` (ds:FBC8), `gPartyDir` (ds:FBCC), `gDrawPage` (ds:8115), `gWallTypeFlags` (ds:194B), `gClassBaseBits` (ds:1231), `gSlotItemMask` (ds:1C8D), `gPanelFillColour` (ds:F57E), `gPanelShadow` (ds:F53A), `gPanelHighlight` (ds:FB4C), `gStagingPage` (ds:DF4E), `gViewDirty` (ds:DF4C), `gMaze` (ds:B894), `gMazeItems` (ds:B899), `gInvSlotX` (ds:17CE), `gInvSlotY` (ds:1804), `gInvSlotKind` (ds:3875), `gInvSlotBoxSize` (ds:3890), `gWandSpells` (ds:38FA), `gPotionFeelings` (ds:3893), `gRingNames` (ds:37B4), `gPotionNames` (ds:37E1), `gWandNames` (ds:3837), `gProjectiles` (ds:DD2C) and `gMonsters` (ds:8A82). Each `why` in the registries matches what the code at that address does, and the three shared-function names are used in the defining units (`StartItemCast` E0EF3.C:2005, `MoveMonsterToCell` E08E6.C:233, `IdentifyListItems` E0BD2.C:298) and in E122E.C.

The comments the diff rewrote do not all survive the field renames. A global replacement hit the wrong struct in several places: `m01` in `CharRec` became `identifiedNameIndex` (an ItemRecord field) and `m00` in ItemRecord became `slotMask` (an ItemType field). ItemRecord's `m04`/`m08` became `cell`/`item` (Projectile fields) in comments that mean `key`/`nextLink`, and `m0A`, `m02` and `m0B` were left behind after their fields became `level`, `type` and `value`. The code is right; the prose is not. These are the must-fix items.

Must-fix (wrong names in lines the diff changed):

- [x] must-fix src/E0BD2.C:134 (TakeItemFromSlot): "marks the record free (cell = -1)" and "returns the item's m0B". The code sets `gItems[item].key = -1` (143) and returns `gItems[item].value` (153). `cell` is Projectile +04 and Monster +02, not the item record's key; `m0B` is now `value`. Say `key = -1` and `value`. Fixed in def28b2.
- [x] must-fix src/E0BD2.C:167 (InsertListItem): "the list's owner key `key` (cell)" and "m0A becomes gCurLevel". The code stores `rec->key = key` (190) and `rec->level = gCurLevel` (192). The field is `key`; `cell` is a Projectile/Monster field. `m0A` is now `level`. Fixed in def28b2.
- [x] must-fix src/E0BD2.C:171-173 (InsertListItem): "the new item's item becomes the old head", "its m06 becomes the tail", "the old head's m06 and the tail's item change", "the lists walked by item". The code sets `rec->nextLink = tailRec->nextLink`, `rec->prevLink = tail`, `tailRec->nextLink = item`, `cur->prevLink = item` (206-209), and the walks follow `nextLink` (255, 288, 314). The link fields are `nextLink` and `prevLink`, not `item` and `m06`. The comment should say the new item's `nextLink` becomes the old head and its `prevLink` the tail, so the old head's `prevLink` and the tail's `nextLink` change. Fixed in def28b2.
- [x] must-fix src/E0BD2.C:319 (PushListItem): "When the queued item still links onward (its item is not itself), the record's head word moves to that item." The code tests `gItems[item].nextLink != item` and stores `gItems[item].nextLink` (324-325). The link field is `nextLink`, not `item`. Fixed in def28b2.
- [x] must-fix src/E0BD2.C:328-330 (DrainListItem): "Items whose m02 is `type`" and "freed (cell = -1, m0A = 0)"; the code tests `gItems[item].type == type` (345) and sets `key = -1`, `level = 0` (346-347). The words are `type`, `key` and `level`; `m02`, `cell` and `m0A` are stale. The same comment's "keeping each item's tag" (330) is a stale word for `cellPos` (357). Fixed in def28b2.
- [x] must-fix src/E0BD2.C:364 (DrawItemName): "both slotMask and identifiedNameIndex offset that base". The code uses `rec->nameIndex` and `rec->identifiedNameIndex` (384-385). `slotMask` is an ItemType field; the ItemRecord field is `nameIndex`. Fixed in def28b2.
- [x] must-fix src/E0BD2.C:443 (AddDDEffect): "QUIRK 0BD2:07DE: rec->type is stored 1 twice". The code stores `rec->m02 = 1` twice (458, 467), and `struct Projectile` has no `type` field (its +02 field is still `m02`). Say `rec->m02`. Fixed in def28b2.
- [x] must-fix src/E0BD2.C:476 (CopyItemToFreeSlot): "when item record `item` is live (cell != -1)". The code tests `src->key == -1` (487) and `cand->key == -1` (493). The field is `key`, not `cell`. Fixed in def28b2.
- [x] must-fix src/E0BD2.C:659 (ApplyItemEffect): "except for effect 4 when identifiedNameIndex bit 1 is set". The code tests `(rec->status & 2) == 0` (707), where `rec` is `struct CharRec *`. `identifiedNameIndex` is an ItemRecord field; the CharRec field at +01 is `status` (re/fields.toml). Fixed in def28b2.
- [x] must-fix src/E0BD2.C:716 (UseWandItem): "moves every gMonsters record whose m02 equals the cell x". The code tests `obj->cell == x` (747). The Monster field at +02 is `cell`; `m02` is its old name. Fixed in def28b2.

Should (clarity; not blocking):

- [ ] should src/E0BD2.C:656 (ApplyItemEffect): "restores m0D/strengthPct from m0E/m10" mixes old and new names. The fields are `strength`, `strengthPct`, `baseStrength` and `baseStrengthPct` (EOB.H +0D, +0E, +0F, +10), and the runner restores the first pair from the second.
- [ ] should src/E0BD2.C:53 and :713-714: `gWandSpells` is "indexed by an item's m0B" and `gWandSpells[m0B]`; the item field is `value` (the local `m0B` in DrawItemName is a leftover, not the field). Say `value`.
- [ ] should src/E0BD2.C:22: `gCursorSlot` is "the selected row"; re/renames.toml says it is the keyboard cursor's item slot 0..26 (0 and 1 the hands, 0x10 the list). Say item slot.
- [ ] should src/E0BD2.C:32: `gWallTypeFlags` is only "byte table"; its why says flags per wall type, indexed by a wall byte of gMaze, with bits 0..1 for a step and bit 2 tested by the path search. Say that.
- [ ] should src/E0BD2.C:35-37: the three panel colours are "a colour read by 0BD2:0917"; their whys name them the bevel's light edge (`gPanelHighlight`), dark edge (`gPanelShadow`) and panel fill (`gPanelFillColour`). Say which is which.
- [ ] should src/E0BD2.C:39: `gStagingPage` is "slot 5's buffer segment"; its why says page 5 (f19EC_0002(5)), which is also where LoadMapScreen reads LEVEL<n>.INF and saved panel pixels. Say page 5 and what it stages.
- [ ] should src/E0BD2.C:40: `gViewDirty` is "set to 1 when an action took effect"; its why says whenever something visible changes (TurnLeft, MoveParty, floor item moves, 0D1C:0006), and RunMapLoop redraws while it is set. Say that.

Notes (for the next reader):

- [ ] note src/E0BD2.C:214 (RemoveListItem): "whose tag equals `key`" is a stale word for `cellPos`; the code matches `rec->cellPos == key` (234). The line was not changed by the diff.
- [ ] note src/E0BD2.C:262 (ScanListItem): "the item whose m02 is `type`" is stale; the code tests `rec->type` (280). The line was not changed.
- [ ] note src/E0BD2.C:653 (ApplyItemEffect): "the item's m0B selects the effect" is stale; the item field is `value` and the local `kind` is the returned value. The line was not changed.
- [ ] note src/E0BD2.C:368 (DrawItemName): "per-m0B" refers to the local `m0B` (374, `m0B = rec->value`), not to a field. The local could be `value`, but the line was not changed.
- [ ] note src/E0BD2.C:847 (ItemClassAllowed): "gItemTypes[m02].m05" is stale; the code tests `gItems[item].type` (855). The line was not changed.
- [ ] note src/E0BD2.C:443 (AddDDEffect): "`type` is the item's m02" is stale; the ItemRecord field is `type`. The line was not changed.
- [ ] note src/E0BD2.C:512 (DrawItemRow): the comment stores `drawMode` in `gDrawPage` "for the duration". re/renames.toml names `gDrawPage` the graphics driver's page; the temporary store is a reuse of the drawing page. The code matches the comment; a QUIRK note would help the next reader.
- [ ] note src/EOB.H:190 (`Projectile.dir`): the header comment says "the page the record is seen from", while re/fields.toml +07 says "the projectile's flight direction 0..3" and AddDDEffect sets it from `gPartyDir`. The header is shared, not this unit's file, so this belongs to the header review.

Round P6 count: 10 must-fix open, 7 should, 8 notes. No previous must-fix is open. The Phase 6 names themselves pass their evidence; the failures are in the comments that follow them.

Evidence: `make match FILE=src/E0BD2.C` 22 of 22 (4681 of 4681 bytes); `make readability UNIT=e0bd2` 0 findings. Nothing escalated: no matcher, spec or pin question.

## Round 3 (at 8c882ae, main)

Judged only what the later-round rule asks: the two must-fix items of round 2 (ticked by 7f051c6) and the diff since a746c03 in `src/E0BD2.C`, `re/symbols.toml` and `re/globals.toml`. The `src/E0BD2.C` diff is two comment hunks (105-107 and 517-522); no code line changed. The `re/symbols.toml` diff adds review stamps to i0000 and s0000 rows and no e0bd2 row. The `re/globals.toml` diff changes one "why" cite for intro ds:28C4 (gMemTopSeg, not used by this unit; its store is src/I0923.ASM:328, checked).

Round 2 must-fix, both fixed right (ticks stay):

- gPartyCell (105-107): this unit's only use of gPartyCell besides the declaration is UseWandItem's `f0D1C_00DA(gPartyCell, page)` (816). E0ACD.C indexes the nine-byte records with it (E0ACD.C:766, `gB899 + gPartyCell * 9`, with `vol = gPartyCell` at 552). The clause is right.
- AddDDEffect (520): "most callers pass gPartyCell, E0ACD.C:434, :769, E0DE8.C:406" is right (the callers are E0ACD.C:434, :769, E0DE8.C:406, E0DE8.C:465 with gPartyCell, E08E6.C:587 with obj->id, E122E.C:751 with the script's id). The script-id claim is right in substance; its line cite is stale (see the should below).

Changed lines:

- must-fix: none.
- should src/E0BD2.C:521 "E122E.C:728 passes the script's id": at a746c03 line 728 of E122E.C was the AddDDEffect call. Commit 49a7f00 (merged in bc30519) moved it: at 8c882ae the call is line 751, inside AddEffectOp (122E:0C4D), and the script's id is read at E122E.C:744. The claim holds, so the cite is stale: replace it with 122E:0C4D. The same sentence also reads as if E122E is the only caller that passes something other than gPartyCell; E08E6.C:587 (`AddDDEffect(-1, item, obj->id, ...)`) passes an object's cell id too. Name it, or say that the other callers pass an object's or the script's cell id.

Round 3 count: 0 must-fix open, 1 should (new), 0 new notes. The round 2 notes and the round 1 notes still open stay with their owners (E0ACD.C:56, E0D1C.C:307, E08E6.C:793, the DrawItemName unset buffer, the m0A placeholder, the MarkListItems citation, and the three cross-file prototype notes). The `re/symbols.toml` review stamps on i0000 and s0000 rows are not this unit's.

Evidence: `make match FILE=src/E0BD2.C` 22 of 22 (4681 of 4681 bytes); `make readability UNIT=e0bd2` 0 findings (0 new); `make review-pass UNIT=e0bd2 AGENT=rev-haiku` stamped the 22 rows of re/symbols.toml. Nothing escalated: no matcher, spec or pin question.

## Round 2 (at a746c03, main)

Judged only what the later-round rule asks: the 12 must-fix and 16 should of round 1, and the diff since 21ed7a2 in `src/E0BD2.C` and `re/symbols.toml` (plus `re/renames.toml` for the two renames). Each changed comment was read against its function body and against the callers it cites (E0ACD.C, E0D1C.C, E0DE8.C, E08E6.C, E122E.C, E1547.C, E1414.C, E177D.C, E0EF3.C, EOV3.C, EOV5.C, E07A9.C). No game run, nothing under eob/ touched.

Round 1 items: all 12 must-fix and all 16 should are fixed right against the code (the ticks stay). The renames have no stale references in src/ (`grep` for f0BD2_0106, f0BD2_0AF2, fOvl5_1CDD finds none), and the overlay entry UseItemPanel matches EOV5.C:1321 and the registry row ovl5:1CDD.

Changed lines since 21ed7a2: the InsertListItem links (tail, tailRec, the m08/m06 updates), the DrawItemRow and CanPlaceItemInSlot and UseSelectedItem comments, ApplyItemEffect (case 1 arms giant strength, E1414.C:05ED and the 07CA runner agree), UseWandItem, DrawItemName, AddDDEffect (the QUIRK and the sound choice), the struct comments, and the re/symbols.toml notes all match the code except for the two must-fix items below.

Must-fix (wrong claims in lines this round changed):

- [x] must-fix src/E0BD2.C:105-107 (the gPartyCell declaration): "and indexes the nine-byte records with it" is not true of this unit. Its only use of gPartyCell is UseWandItem's `f0D1C_00DA(gPartyCell, page)` (816). Every nine-byte record access here is by `volume` (402, 417, 436, 535). The indexing by gPartyCell is in other units (E0ACD.C:552-568 sets `vol` from gPartyCell and indexes gB899 with it). Drop the clause, or name the unit that indexes with it. Fixed (branch unit/e0bd2): the declaration now says this unit passes gPartyCell to f0D1C_00DA (UseWandItem) as the cell, and names E0ACD.C as the unit that indexes the nine-byte records with it. Comment text only, line count kept so the cross-file cites do not move.
- [x] must-fix src/E0BD2.C:520 (AddDDEffect comment): "the callers pass gPartyCell, E0ACD.C:434, :769" is wrong for one caller. E122E.C:728 passes the script's object id (read at E122E.C:721) as the volume. E0ACD.C:434, :769 and E0DE8.C:406 pass gPartyCell. Say "most callers pass gPartyCell (E0ACD.C:434, :769, E0DE8.C:406); E122E.C:728 passes the script's id". Fixed: the comment now reads "most callers pass gPartyCell, E0ACD.C:434, :769, E0DE8.C:406; E122E.C:728 passes the script's id" (checked against src/E122E.C:721-728 and src/E0DE8.C:406).

Notes (for the next reader, not blocking):

- [ ] note src/E0BD2.C:374-376 (MarkListItems, "so 0x40 is the identified flag"): the name rests on 05B5's reading of the bit. The caller that sets it over a party's pack and a cell's list is E122E.C:1390-1404 (MarkListItems on the +0x97 sub-table and on gB899 + id * 9). The name is acceptable; citing that caller would make the evidence explicit.
- [ ] note: the 4th parameter of InsertListItem is still named `sound` in five other files' prototypes (E0ACD.C:167, E0D1C.C:137, E122E.C:183, EOV3.C:169, EOV5.C:195). Their reviews should take `tag`. EOV3.C:177 (struct field m03) also calls it "the sound InsertListItem plays" and cites src/E0BD2.C:420, which is now DrainListItem's re-queue line.
- [ ] note: DrawItemRow's prototypes name its first two parameters `who` and `one` (E0ACD.C:169, E0D1C.C:136, E1547.C:160, E122E.C:45). Their reviews should take `row` and `drawMode`. E0D1C.C:307 (the wrong argument) stays for the E0D1C review.

Round 2 count: 2 must-fix open, 0 should, 3 notes. The round 1 notes still open (E0ACD.C:56, E0D1C.C:307, E08E6.C:793, the DrawItemName unset-buffer note and the m0A placeholder) belong to their owners' reviews.

Evidence: `make match FILE=src/E0BD2.C` 22 of 22 (4681 of 4681 bytes), exit 0; `make readability UNIT=e0bd2` 0 findings. Nothing escalated: no matcher, spec or pin question.

## Findings

Each finding is a checkbox. `must-fix` blocks `make review-pass` until a decompiler ticks it off
(with the commit that fixes it); `should` and `note` do not block.

Must-fix (wrong or contested claims):

- [x] must-fix src/E0BD2.C:19, :236-237, :246, :251-260, :272, :304, :423: the 4th argument of InsertListItem is named `sound`, and the m03 comment says m03 is "compared with 01D0's key and 02B9's key". The code stores the 4th argument in m03 (260, 272), RemoveListItem matches m03 against its `key` (304), and DrainListItem re-queues with m03 as the 4th argument (423). Nothing shows a sound: E0ACD.C:561-568 passes the same `slot` both as RemoveListItem's key and as InsertListItem's 4th argument, E08E6.C:807 passes a roll, and AddDDEffect (521) passes kind | 4. Give the field and the parameter a neutral name (for example `tag`, Purpose unknown, with those callers as the evidence), and correct the m03 comment: 01D0 does not compare m03, it stores its 4th argument there. Note that E08E6.C:793 repeats the "sound argument" claim, so the rename needs E08E6 too (through make rename). Fixed (branch unit/e0bd2): the ItemRecord field is now `tag` (src/E0BD2.C:19, with the callers as the evidence and `Purpose unknown`) and InsertListItem's 4th parameter is `tag`; the comment now says 01D0 stores its 4th argument there and 02B9 matches it. E08E6.C:793's `sound argument` comment is E08E6's own (the review summary sends that conflict to its review); it is a comment, not an address, so `make rename` cannot reach it. make match FILE=src/E0BD2.C 22 of 22.
- [x] must-fix src/E0BD2.C:26: m0B "the value 01D0/02B9 return". 01D0 is void (246), 02B9 returns the removed item index (322), and no caller reads m0B from them. f0BD2_0106 returns m0B (223). m0B is the item's bonus or effect value: "%s +%d" and "Cursed %s %d" (05B5, 466-468), the spell index into ds:1278 and ds:12E0 (474, 478), the wand effect index g38FA[m0B] (0EE8, 790), and the scroll and potion values (1001, 882, 888). Rewrite the comment to say so. Fixed: the m0B comment now names the item's bonus or effect value and cites the `%s +%d`/`Cursed %s %d` strings, the ds:1278/ds:12E0 spell tables, g38FA[m0B] and the scroll/potion values.
- [x] must-fix src/E0BD2.C:99: gPartyCell is declared "the global sound volume (src/E177D.C)". E177D.C:92 says the opposite ("the party's map cell", the sound's cell for Distance), and so do E08E6.C:97 and E0D1C.C:43. This unit uses it as a map cell (UseWandItem, 795: `f0D1C_00DA(gPartyCell, page)`). Replace the comment with the map cell and the citation E177D.C:92. Fixed: the declaration now reads the party's map cell (src/E177D.C:92) and says UseWandItem passes it to f0D1C_00DA as the cell.
- [x] must-fix src/E0BD2.C:571-589 (DrawItemRow, first parameter `who`): the parameter is a panel row, not a character. Evidence: 589 `gChars[gA99A].m77[who]` (the selected character's slot), 608 `gF51A == who` (gF51A is the selected row, declared at 95 and set by E0ACD.C:704-709), g1C8D is a "per-row" mask indexed by row (649, 701). Callers pass rows: SetActionRow passes old and gF51A (E0ACD.C:710-711), f0BD2_0106 passes row (219), UseSelectedItem passes slot (670). Rename the parameter to `row`, and fix the comment "draws the item row of character `who`" (the character is gA99A). The globals g17CE, g1804, g3875 are commented "per-character" (119-121) but are indexed by this row (587-594, 611-615); say per-row. Note: E0D1C.C:307 calls `DrawItemRow(who, slot, 1)` with a character index inside a loop over characters. One of the two callers has the wrong argument; the E0D1C.C review must settle it (not a fix in this unit). Fixed: the first parameter is `row`, the comment says the character is gA99A, and g17CE/g1804/g3875 are commented per-row. The E0D1C.C:307 wrong argument stays for the E0D1C review.
- [x] must-fix src/E0BD2.C:573 (DrawItemRow comment): "when `slot` is nonzero and the row is the party's, the item icon". The icon is drawn when item != 0 and g3875[who] != 1 (611-616). `slot` only decides whether the frame panel is drawn (584-590). "The row is the party's" appears nowhere in the code. Fix the comment. Fixed: the comment now says `slot` draws the frame panel and the icon is drawn when the row holds an item and g3875[row] is not 1, offset by -4 when it is 2.
- [x] must-fix src/E0BD2.C:655-657 (UseSelectedItem comment): "uses the item in slot `slot` ... installs the item (slot 0x10) or swaps the mouse's item". The function exchanges the held item with the slot. For slot other than 16 it gives the slot's item to the hand (SetMouseItem(saved), 668) and puts the held item in the slot (669). For slot 16 with an empty hand it takes the first item out of the pack list at m77[16] (673). For slot 16 with an item in hand it puts that item into the pack list with key 0xFFFE (677) and empties the hand (679). Rewrite the comment to say that. Fixed: the comment now describes the exchange (slot item to the hand, held item to the slot; slot 0x10 with an empty hand takes the first pack item, with an item in hand queues it with key 0xFFFE and empties the hand).
- [x] must-fix src/E0BD2.C:236-245 and :249-270 (InsertListItem): the comment says "their m06 and m08 now name `item`, whose own take the old values". Only two links change: the old head's m06 (280) and the old head's predecessor's m08 (279). The new item takes m06 = predecessor and m08 = old head (277-278). Also the locals misname the list: `afterHead` (252, 269) is head.m06, which RemoveListItem shows is the link before a record (311-313): the predecessor, the tail of the walk by m08. `next` (249, 270) is that predecessor record, not the item after the head. Fix the comment, and rename the locals (for example `tail` and `tailRec`; the register constraint in the comment still holds because they are locals). Fixed: the comment now says the new item's m08 becomes the old head and its m06 the tail, so only the old head's m06 and the tail's m08 change; the locals are `tail` (the index) and `tailRec` (the record).
- [x] must-fix src/E0BD2.C:506-508 (AddDDEffect comment): "and the kind-7 sound". The sound test uses `type`, the 6th argument (E0ACD.C:769 passes gTable8DD2[item].m02), not kind: line 534 plays 0Bh unless type is 7, which plays 1Ah. Fix the comment. The comment also does not say what the parameters are: `volume` is the map cell the callers pass as gPartyCell (E0ACD.C:434, 769); the list is the cell's record at gB899 + volume * 9 (521). Fixed: the comment says the 0x0B/0x1A sound is chosen by `type` (the 6th argument, the item's m02), and that `volume` is the map cell and the list is gB899 + volume * 9.
- [x] must-fix src/E0BD2.C:711 (ApplyItemEffect comment): "1 restores the strength score to 22 and heals". Case 1 sets m0D to 22 (729-730) and arms a timed effect with ApplyCharEffect(who, ..., 7, 1) (731). Value 7 is the giant-strength effect type: E1414.C (05ED, 421-430) stores the value as the type byte, and the 07CA runner (E1414.C:490-500) prints "%s no longer has giant strength" and restores m0D/m0F from m0E/m10 when it runs out. Case 1 does not heal; cases 2 and 3 heal (HealCharacter). Fix the comment: sets the strength score to 22 and arms a timed giant-strength effect. Fixed: case 1 now reads sets m0D to 0x16 and arms a timed giant-strength effect (value 7, E1414.C:05ED and the 07CA runner); cases 2 and 3 heal.
- [x] must-fix src/E0BD2.C:770-773 (UseWandItem comment) and :803: "activates every gObjs record whose m02 matches the page index". The test is `obj->m02 == x` (803), where x is the cell from f0D1C_00DA(gPartyCell, page) (795), not the page. Each match is moved to cell y with MoveObjectToId(obj, y, -1) (804); E08E6.C:295 names those arguments (obj, id, orient). Fix the comment to say so. Fixed: the comment says the test is obj->m02 == x (the cell from f0D1C_00DA(gPartyCell, page)) and each match is moved to cell y with MoveObjectToId(obj, y, -1).
- [x] must-fix src/E0BD2.C:147 (`fOvl5_1CDD`): the same routine at ovl5:1CDD is named UseItemPanel in EOV5.C:1321 (`void UseItemPanel(int msg)`) and in re/symbols.toml:9361-9365. The name must agree across src/ (playbook 70, rule 7). Rename to UseItemPanel and its parameter to msg (the caller passes gItems[item].m0B, 893). Fixed: the extern is UseItemPanel(int msg) at 211E:0093 (ovl5:1CDD) and the call passes gItems[item].m0B. The stub address 211E:0093 is not the registry address ovl5:1CDD, so `make rename` cannot connect them; the source edit is the fix.
- [x] must-fix src/E0BD2.C:430-439 (DrawItemName comment): "The record's first two bytes index two name tables at segment 0x1F28, 0x23 bytes apart". It is one table of 0x23-byte entries at 1F28:0000: both 452 and 453 use the same base 0x1F280000 with m00 * 0x23 and m01 * 0x23. Fix the wording. Fixed: the comment now says one table of 0x23-byte entries at 1F28:0000 with both m00 and m01 offsetting the same base.

Should (clarity; not blocking):

- [x] should src/E0BD2.C:41: "read by 0BD2:00": a truncated address. ReportLostItem is 0BD2:000E (line 174). Use the full address. Fixed: 0BD2:000E.
- [x] should src/E0BD2.C:170: "functions of this unit, in address order (stubs not yet written)" is stale; every function is written. Fixed: dropped the stale `(stubs not yet written)`.
- [x] should src/E0BD2.C:54-55: CharRec m1E and m1F have no comment. m1F bit 0 selects "her" over "his" (ReportLostItem, 187-190); m1E is the per-slot bit UseItemSlot tests (840). Fixed: m1E is the per-slot status bits UseItemSlot tests, m1F bit 0 selects her over his in ReportLostItem.
- [x] should src/E0BD2.C:236-245 (InsertListItem): the `key` is not explained. Callers pass a list owner (vol at E0ACD.C:568, gPartyCell at E0D1C.C:362, obj->id at E08E6.C:807, volume at 521) and 0xFFFE for a character's own pack (E0ACD.C:780, 851, and here 677). A negative key gives m0A = -1 (262-265, 273-276). Say so in the comment. The key stored in m04 is -1 when the record is free (CopyItemToFreeSlot, 552). Note the two different fields behind the one name `key`: this function's key goes to m04, RemoveListItem's key is matched against m03 (see the first must-fix). Fixed: the comment says the key is a list owner (a map cell, an object id or 0xFFFE for a character's own pack) and a negative key gives m0A = -1; it notes InsertListItem's key goes to m04 while RemoveListItem's is matched against the tag.
- [x] should src/E0BD2.C:330-332 (ScanListItem comment): "otherwise the item `target` itself" applies only when `type` is -1. With both target and type set, nothing matches (349-350). Say it. Fixed: the comment now says that with both target and type set, nothing matches.
- [x] should src/E0BD2.C:385-386 (PushListItem comment): "makes that head the item's next" is unclear. When the item links onward, the list head moves to the item's m08 (391). Reword. Fixed: reworded to say the record's head word moves to the item's m08 when the item links onward.
- [x] should src/E0BD2.C:575 and :620-625 (DrawItemRow, RedrawRowArea): the parameter `one` is the draw mode passed as DrawItemIcon's `mode` (E0D1C.C:484, 613) and stored in gStatusLine (586). Name it drawMode and drop "input mode" from the comment, which is not evidenced. In DrawItemRow, drop "Purpose unknown" (the name and body say it draws the row) and fix "Called by 0D1C and 0ACD": the callers also include 0BD2:0106, 0BD2:0BA0, 122E:06D0 and 1547:0991 (packet). Fixed: `one` is `drawMode` in DrawItemRow and RedrawRowArea, `Purpose unknown` dropped, and the caller list names 0ACD:09EF, 0BD2:0106, 0BD2:0BA0, 0D1C:01CA, 0D1C:0304, 122E:06D0 and 1547:0991.
- [x] should src/E0BD2.C:430-435 (DrawItemName comment): "Called by 0BD2:000E" misses SetMouseItem (07A9:010C, E07A9.C:380). Fixed: the comment now names SetMouseItem (07A9:010C) as a caller.
- [x] should src/E0BD2.C:205-223 (f0BD2_0106): "Purpose unknown" with a comment that already says what happens. The three callers (ApplyItemEffect 724, UseItemRow 0EF3:2604, DrainItemOp 122E:1A3B) all consume an item and use its m0B. That is enough to name it (take the item out of a slot and return its value). Fixed: renamed to TakeItemFromSlot with `make rename` (other files that declare 0BD2:0106 get the name on merge).
- [x] should src/E0BD2.C:629-631 (f0BD2_0AF2): "Purpose unknown" while the comment states the checks (class mask, flag 0x20, wear type) and the messages (638, 651) say which. A name from that (for example CanPlaceItemInSlot) is better than a placeholder. Fixed: renamed to CanPlaceItemInSlot with `make rename`.
- [x] should src/E0BD2.C:688-703 (SwapSelectedItem): `slot` holds the item in the hand (GetCurrentItem, 695), an item index. Rename it (for example heldItem). Fixed: the local is `heldItem`.
- [x] should src/E0BD2.C:387, :397 and :509: `vol` (PushListItem, DrainListItem) and `volume` (AddDDEffect) name the same cell index. Pick one name for the file. Fixed: PushListItem, DrainListItem and AddDDEffect all use `volume`.
- [x] should src/E0BD2.C:124, :128: the extern prototypes use placeholder parameter names. Use the names of the defining units: StartPanelAction(char who, int action, int kind) (E0EF3.C:2132) and MoveObjectToId(obj, id, orient) (E08E6.C:295). Fixed: StartPanelAction(int who, int action, int kind) and MoveObjectToId(obj, id, orient). The first parameter must stay `int` here: `char who` changed the callers' code and broke UseWandItem and UseItemSlot, so only the names changed.
- [x] should src/E0BD2.C:436-439, :241-245, :714-717, :820-822: matcher notes (jump-table order, register variables, `-v`, the if-shape the compiler needs) sit inside function comments. Move them to the notes of the symbol in re/symbols.toml; keep each comment to what the code means in game terms. Fixed: the register-variable note (InsertListItem), the jump-table/if-shape note (DrawItemName), the -v note (ApplyItemEffect) and the case-order/-v note (UseItemSlot) moved to the re/symbols.toml notes of those functions.
- [x] should src/E0BD2.C:907-917 (ItemClassAllowed): it returns the masked byte, not a boolean (912 casts it to signed char); the comment says so, but the name suggests a boolean. Say "nonzero when the class may use the item". Fixed: the comment now says the result is nonzero when the character's class may use the item.
- [x] should src/E0BD2.C:364-379 (MarkListItems): it sets flag 0x40, which DrawItemName reads to show the full name and bonus (458-459, 498). The name and comment do not say what the bit means. If the evidence is enough, name the bit in the comment (for example "identified") with 05B5 as the evidence; otherwise say that the meaning is not established. Fixed: the comment calls bit 0x40 the identified flag, read by 0BD2:05B5.

Notes (for the next reader):

- [ ] note src/E0ACD.C:56 says gPartyCell is the sound volume, citing SetSoundVolume (122E:0001). E177D.C:92, E08E6.C:97 and E0D1C.C:43 say the party's map cell. The E0ACD review must settle it.
- [ ] note src/E0D1C.C:307 passes a character index to DrawItemRow (see the DrawItemRow must-fix). Settle it in the E0D1C review.
- [x] note src/E0BD2.C:523 and :532: AddDDEffect stores rec->m02 = 1 twice. The second store is in the matched code, so leave it; say so in a comment if it is kept. Fixed: a QUIRK comment on 0BD2:07DE records the second rec->m02 = 1.
- [ ] note src/E0BD2.C:457-497 (DrawItemName): for types 9, 10, 14, 16 and 18, when str2 is 0 nothing writes buf, and DrawText(buf) then draws an unset buffer. Check whether SkipStrings or the spell tables can return 0 for the item values in the data; if they can, mark the path `QUIRK 05B5:`.
- [ ] note src/E0BD2.C:25: m0A holds gFBC8 (the current action in E0D1C.C) or -1; the comment is fine, but the name "m0A" stays a placeholder until its readers are reviewed.

## Summary

The unit is understandable from its source only in part. The bytes are proven and the mechanical rules pass, but several names and comments say something the code does not: the list field m03 is called a sound, the item-row function's first parameter is called a character, the potion effect is said to heal, the wand effect is said to test the page, and the list-insert comment gives the wrong links. Each must-fix has its evidence in the findings above (addresses, callers, and other files of src/). The overlay routine must take the registry name UseItemPanel. Placeholders that stay with reasons: m0B, m0A, m02, m03 (with a neutral name), the struct DDRec fields, and the globals that only other units can name (g194B, g1231, g38FA, g3893 and the string tables). Names that the evidence already supports and should be made real: f0BD2_0106 and f0BD2_0AF2 (the shoulds above).

Round 1: 12 must-fix open, 16 should, 5 notes. Nothing escalated: no matcher, spec or pin question. The cross-unit conflicts (E0D1C.C:307, E0ACD.C:56, E08E6.C:793) go to their own reviews.

## Decompiler response (branch unit/e0bd2)

All 12 must-fix findings are fixed in src/E0BD2.C without changing a byte of code: the record field
and InsertListItem's 4th parameter are `tag`, the comments for m0B, gPartyCell, DrawItemRow,
UseSelectedItem, InsertListItem, AddDDEffect, ApplyItemEffect, UseWandItem and DrawItemName now say
what the code does, DrawItemRow's first parameter is `row` with g17CE/g1804/g3875 called per-row, and
the overlay entry is `UseItemPanel(int msg)`. All 16 should findings are also fixed: the truncated
address, the stale unit comment, the m1E/m1F comments, the InsertListItem key, the ScanListItem,
PushListItem, DrawItemRow/RedrawRowArea, DrawItemName, ItemClassAllowed and MarkListItems comments,
the `heldItem` and `volume` locals, the extern parameter names, and the matcher notes moved to
re/symbols.toml. Notes: the m02 QUIRK comment is added; the two cross-unit notes (E0ACD.C:56,
E0D1C.C:307) and the DrawItemName unset-buffer note stay for their owners.

Two names were real, so `make rename` recorded them: 0BD2:0106 is TakeItemFromSlot and 0BD2:0AF2 is
CanPlaceItemInSlot (other files that declare them get the names on merge). Two renames could not go
through `make rename`: the struct field m03/tag and the 4th parameter are not addresses, and the
overlay stub is declared at 211E:0093 while the registry names ovl5:1CDD, so UseItemPanel is a source
edit (the extern keeps the `@ 211E:0093` annotation with the `ovl5:1CDD` comment).

One correction to a should: the StartPanelAction prototype keeps its first parameter as `int`, not
`char`. `char who` truncates the argument at the call sites and broke UseWandItem (`jmp 0x10c` vs
`jmp 0x10b`) and UseItemSlot (`sub sp,0xa` vs `sub sp,0xc`), so only the parameter names changed.

Evidence: `make match FILE=src/E0BD2.C` 22 of 22 (4681 of 4681 bytes), exit 0; `make readability
UNIT=e0bd2` 0 findings. All 22 functions are at L2, none below, so there is nothing to decompile and
nothing is escalated.

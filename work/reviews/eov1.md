# Review: eov1

Reviewer: rev-haiku
Commit reviewed: 5ab5e506c9de9a707573ad68b289af65bc246347 (main, round 1)

Playbook 70. Judge what a matching file can still get wrong: names, types, comments, structure.
The matcher already proved the bytes; do not re-prove them, read for meaning.

## Findings

Each finding is a checkbox. `must-fix` blocks `make review-pass` until a decompiler ticks it off
(with the commit that fixes it); `should` and `note` do not block.

Evidence used: the code of `src/EOV1.C` (line numbers below); the other files of `src/`
(`E0D1C.C` headers for 0D1C:00F1, 0163, 0670, 0C21, 0454, 035A; `E0A72.C` for FindClassSlot;
`EOV6.C` for the spell slots and g159); `make match FILE=src/EOV1.C` (16 of 16, exit 0);
`make readability UNIT=eov1` (0 findings); and the quoted strings, read only, in `eob/EOB.EXE`.

### Round 1 (2026-11-02, reviewed at 5ab5e50)

- [x] must-fix src/EOV1.C:168 and :311: fixed (commit ca0b550). The declaration now reads "five
  9-byte lists, one per spell level 1 to 5 ... the memset clears 5 * 9 bytes" and the
  BuildSpellSlots header "the five 9-byte level lists".
- [x] must-fix src/EOV1.C:1055: fixed (ca0b550). The header now says "FindCarriedSlot item type
  0x22, kind i + 1, one search per spell number 1 to 32".
- [x] must-fix src/EOV1.C:965-966: fixed (ca0b550). The ShowMessage header now says "Called by
  RunCampMenu, SelectPartyMember and ScribeScrolls (a plain message box; DrawListNode does not
  call it)." ScribeScrolls calls it at :1098 and :1116; DrawListNode makes none.
- [x] must-fix src/EOV1.C:913-915: fixed (ca0b550). The BuildMenuButtons header now names 0416
  (menu 3), 097A (menu 4), 1691 (menu 6), 1ACD (menus 0 to 2) and 243A (menu 5), and says the
  header messages are edited in place by ovl1:1ACD (menus 0 to 2) and ovl1:0416 (menu 3).
- [x] must-fix src/EOV1.C:903: fixed (ca0b550). The header now says the pointer is hidden only
  around the final ClipRect, and that each node hides it itself inside DrawListNode.
- [x] should src/EOV1.C:905 and :1013: fixed (ca0b550). "bevelled panel" is now "framed rectangle"
  in both places, matching the 0D1C:0C21 header.
- [x] should src/EOV1.C:1022 and :1036: fixed (ca0b550). DrawListNode's parameters are renamed
  panelFirst/panelSecond to first/second, the names 0D1C:0C21 uses, and the header says they are its
  two mode arguments, not a panel shape.
- [x] should src/EOV1.C:1019 to :1020: fixed (ca0b550). SpellSlotScreen is now in DrawListNode's
  "Called by" list (its calls at :739, :742, :802, :804 and :815).
- [x] should src/EOV1.C:452: fixed (ca0b550). The SelectPartyMember header now says "FindCarriedSlot
  item type 0x1D".
- [x] should src/EOV1.C:492: tried a `msg` local (commit ca0b550's working tree). It changes the
  frame: `sub sp, 0x18` instead of `0x16`, so the original genuinely reuses `count`. The source form
  is kept and the comment now says `count` carries the message index and why a named local cannot be
  used. Same for BoolString at :885 (a local makes `cmp word ptr [bp+6],0` where the original keeps
  `on` in SI); kept with a note.
- [x] should src/EOV1.C:181 and :200: fixed (ca0b550). The duplicate `gNeedsRest` is removed; the
  one near gSoundOn stays.
- [x] should placeholder globals: fixed (ca0b550). gFD98, gFDEA, g62F2, g62F8, gKeyRemapFrom,
  gKeyRemapTo and gFileNameBuf each now carry a note of what is known.
- [x] should src/EOV1.C:172 to :173 (gFDEC): fixed (ca0b550). The comment now reads "a pair of
  counters per spell number i, byte 2 * (i - 1) the copies held in all and the next byte the copies
  ready".
- [x] should src/EOV1.C:176 (gFD9A): fixed (ca0b550). The comment now adds "a spell not on the
  0-terminated map stays 0 (the memset, and the slot >= 0 test)".
- [x] should src/EOV1.C:83 (ButtonDef flags): fixed (ca0b550). Bit 1 is now documented as marking
  the button selectable, tested by the key walk in RunCampMenu and AskYesNo.
- [x] should src/EOV1.C:1189 to :1213 (RunCampMenu header): fixed (ca0b550). The header now explains
  `lit` (the highlighted node, 0 when none) and `hot` (the node to light next), and that their
  difference is drawn at the end of each pass.
- [x] should src/EOV1.C:1532: fixed (ca0b550). The AskYesNo header now writes the calls: DrawListNode
  (node, 1, 1, 1), WaitFrames(5), then DrawListNode(node, 0, 1, 1).
- [x] should src/EOV1.C:1213: fixed (ca0b550). The header now lists the inline QUIRK ovl1:1D48 next
  to QUIRK ovl1:20FF.
- [x] should src/EOV1.C:1318: checked against the binary instead (ca0b550). gStrings2003 is
  segment 2003:0000, file offset 0x22830 in eob/EOB.EXE (MZ header 0x2800 plus para 0x20030); index
  0x2d (45) is " You cannot have\r less than four\r characters." So the comment's quote is the whole
  string. "You have not created four characters yet!  Do you wish to continue anyway?" sits at file
  offset 0x26AB6, not in gStrings2003, so it is not this index. The comment now says "string 0x2d,
  whole".
- [x] note src/EOV1.C:1-9 (unit header): fixed (ca0b550). The TASM claim is tagged "[H: the
  escalation record]".
- [x] note src/EOV1.C:124 to :125 and :372 to :374: fixed (ca0b550). The SpendSpellSlot header now
  uses "a spell still to be memorised" (the negative encoding of the CharRec slot fields), the same
  wording as the struct.
- [x] note src/EOV1.C:1190, :1067 to :1068, :9, quirk addresses: checked the overlay 1 ones against
  build/re/eob-overlays/ovl01.bin (ca0b550). ovl1:1277 holds 26 8B 87 05 00; ovl1:1D48 is the
  confirm-key code with no test of `lit`; ovl1:20FF is `cmp word ptr [bp-0xa],0x6e`; ovl1:2558
  stores `si + 0x8021`; ovl1:0401 is `dec [bp-2]; inc [bp-2]` (the net-zero index). The ds:
  addresses (3B8E, 3BBE, 63CB, 640B, 644B) are still not checked to a listing.
- [x] note src/EOV1.C:1525 and :1318: checked (ca0b550). Index 0x37 (55) is " Are you sure you\r
  wish to LOAD a\r saved game?" and index 0x2d (45) is " You cannot have\r less than four\r
  characters."; both comments quote the whole strings. The fragments the EXE grep showed are other
  tables (0x22DA9 "wish to drop.") or another message (0x26AB6).
- [x] note src/EOV1.C:167 (gFileNameBuf): confirmed a project-wide name question for phase 6; the
  unit keeps the name and now says it is the text buffer sprintf formats into.
- [x] note src/EOV1.C:1333: left as is; the comment explains the kept `break;`.
- [x] note (registry, a question for the human): not fixed in this branch, as the reviewer said.
  The matcher still proves 16 functions and re/symbols.toml still lists 12 with DrawListNode 1404
  and SpellSlotScreen 2009. This needs `make map` (or a human decision) and is carried in the
  hand-off note; it is not a source change.

## Summary

Read for meaning, the file is clear in its large parts: the header and function comments for
DropCharacter, BuildSpellSlots, SpendSpellSlot, SelectPartyMember, SpellSlotScreen, DrawSpellRow and
ScribeScrolls describe the code step by step, the key tables match the switch statements, and the
struct offsets (CharRec 0xF3 bytes with its fields at +01, +13, +1B, +20, +23, +24, +37, +55, +73,
+77, +E7; Node 0x3C bytes with the fields at +04 to +38) agree with the code. The shared globals agree
with the other units (gSaveRect, gFillRect, gDrawText, gStatusLine, gClickY, g159 and EOV6.C:57).
Each callee is named as src/ names it, and the 0D1C:0163 and 0D1C:0670 uses match their headers.

Five must-fix items block the pass: the gFDBA list count (five, not nine), the 0x22 argument of
ScribeScrolls (an item type, not a kind), the ShowMessage callers, the BuildMenuButtons callers and
menu headers, and the pointer claim in the BuildMenuButtons header. All are wording or one comment
with the code unchanged.

Names that are still placeholders (g62F2, g62F8, gFD98, gFDEA, gKeyRemapFrom, gKeyRemapTo,
gFileNameBuf and the `g`-names of the shared globals) are acceptable where the evidence is thin, but
each declaration should say what is known (should above). The functions' names are backed by
their addresses and by callers in src/, so no function name is a must-fix.

Question for the human (registry, see the last note): re/symbols.toml does not list four of the 16
functions of this unit and gives two of them the wrong size, so review-pass cannot stamp the unit
as a whole. This needs `make map` or a human decision; it is not in this branch.

## Resolution (dec-deepseek-2, 2026-11-02, commit ca0b550)

Every finding above is ticked. The five must-fix items were comments only; all 16 functions still
match (`make match FILE=src/EOV1.C` 16 of 16, 9864 of 9864 bytes) and `make readability UNIT=eov1`
reports 0 findings. Two `should`s were checked with the matcher rather than changed: a named local
for SelectPartyMember's message and for BoolString's `on` do move the bytes, so the original reuse
of `count`/`on` is kept and noted. The quote findings were checked against `eob/EOB.EXE`: index 0x2d
of gStrings2003 is " You cannot have\r less than four\r characters.", so the comment was right; the
"You have not created four characters yet!" string is a different message. The ovl1 addresses in the
comments were checked against `build/re/eob-overlays/ovl01.bin`. The registry row sizes (last note)
are still wrong; that needs `make map` or a human and is carried in the hand-off.

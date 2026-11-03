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

- [ ] must-fix src/EOV1.C:168 and :311: gFDBA is "nine 9-byte lists, one per spell level", and the
  BuildSpellSlots header says "the nine 9-byte level lists". The code has five: memset(gFDBA, 0,
  0x2d) at :330 clears 45 bytes (5 x 9), the spell levels are 1 to 5 (gFD9A, :176), and the tabs
  are 0 to 4 (wantTab > 4 at :734, the tab keys 0x801b to 0x801f at :807 to :811). Say five.
- [ ] must-fix src/EOV1.C:1055: ScribeScrolls calls FindCarriedSlot "kind 0x22". The second
  argument of 0D1C:00F1 is the item type (field2, the m02 byte), the third the kind (fieldB, m0B)
  (src/E0D1C.C:234-237). The call at :1090 passes 0x22 as the type and i + 1 (the spell number) as
  the kind. Say "item type 0x22, kind i + 1".
- [ ] must-fix src/EOV1.C:965-966: ShowMessage says it is called by "RunCampMenu, SelectPartyMember
  and DrawListNode". DrawListNode (:1022 to :1052) makes no ShowMessage call. ScribeScrolls does
  (:1098 and :1116) and is missing from the list. (Evidence: the bodies of the functions; the
  packet's called-by row for ShowMessage lists DrawListNode only because of the stale registry, see
  the note below.)
- [ ] must-fix src/EOV1.C:913-915: the BuildMenuButtons header names the callers wrongly. "1ACD
  (menus 1 and 6)": menu 6 is built only by ScribeScrolls (:1133); RunCampMenu builds menus 0, 1
  and 2 (:1277, :1386). It also says the menu records are "edited in place by ovl1:1ACD": 1ACD sets
  the headers of menus 0 to 2 (:1270 to :1276), and SelectPartyMember sets menu 3's header (:518).
  Name the callers: 0416 (menu 3), 097A (menu 4), 1691 (menu 6), 1ACD (menus 0 to 2), 243A (menu 5).
- [ ] must-fix src/EOV1.C:903: BuildMenuButtons "returns its head, with the pointer hidden while the
  window is drawn". The only LockHidePointer in the function is at :951, around the final ClipRect
  (:952). The window set-up (:923 to :929: SetTextWindowAndClear, 0C21, the header's DrawShadowText)
  runs with the pointer shown; each node hides it inside DrawListNode. Say what is hidden.
- [ ] should src/EOV1.C:905 and :1013: "bevelled panel" (0D1C:0C21). The 0C21 header in src/E0D1C.C:704
  says it "draws a framed rectangle" with colours picked by its first and second arguments, and is
  Purpose unknown. "Bevelled" is not in the evidence. Say "framed rectangle".
- [ ] should src/EOV1.C:1022 and :1036 (DrawListNode, panelFirst and panelSecond): the names claim a
  panel. The two values are the first and second arguments of 0D1C:0C21 (E0D1C.C:709, named first
  and second there) and also choose gDrawText over DrawShadowText (:1046). Name them by that role,
  or say in the header that "panel" is a guess.
- [ ] should src/EOV1.C:1019 to :1020: DrawListNode's "Called by" list omits SpellSlotScreen, which
  calls it at :739, :742, :802, :804 and :815.
- [ ] should src/EOV1.C:452: "FindCarriedSlot item 0x1D" is the item type (field2 of 0D1C:00F1,
  E0D1C.C:234-237), the spell book. Say "item type 0x1D".
- [ ] should src/EOV1.C:492: SelectPartyMember reuses `count` for the message index (`count was 0: it
  carries the message index`). A named local for the message (for example `msg`) keeps `count` a
  count. The match decides whether the source form changes the bytes; try it.
- [ ] should src/EOV1.C:885: BoolString overwrites its parameter `on` with the message index (0x15 or
  0x16). Use a local.
- [ ] should src/EOV1.C:181 and :200: `extern int gNeedsRest; /*@ ds:FE2C */` is declared twice. Keep one.
- [ ] should placeholder globals with no note on what is known (READABILITY, "Placeholder globals"):
  gFD98 (:163, set when a member is not selectable, SelectPartyMember header :442), gFDEA (:166, the
  count byte of a spell, DrawSpellRow :635 and SpellSlotScreen :821), g62F2 and g62F8 (:182, :183,
  the icon x in 8-pixel columns and the y of each member, SelectPartyMember :498 to :501),
  gKeyRemapFrom and gKeyRemapTo (:209, :210, a key-to-button map read at :1287 to :1289), and
  gFileNameBuf (:167; the other units' declarations say "the text buffer sprintf formats into", and
  this unit uses it the same way).
- [ ] should src/EOV1.C:172 to :173 (gFDEC): "32 pairs of counters per spell number - 1" is garbled.
  Say: a pair per spell, byte 2 * (spell number - 1) the copies held in all, the next byte the copies
  ready (SpendSpellSlot, :392 to :396).
- [ ] should src/EOV1.C:176 (gFD9A): "the level (1 to 5) of spell number i + 1" omits that a spell
  not on the 0-terminated map has 0 (memset at :333, the slot >= 0 test at :357). Add it.
- [ ] should src/EOV1.C:83 (ButtonDef flags): bit 1 is missing. The key list selects a button by it
  (RunCampMenu :1471, :1481; AskYesNo :1588), and the RunCampMenu header cites "definition flag bit 1".
- [ ] should src/EOV1.C:1189 to :1213 (RunCampMenu header): `lit` (the highlighted node) and `hot` (the
  node to light next) are used throughout the function but never explained here (AskYesNo explains
  its own, :1526 to :1529).
- [ ] should src/EOV1.C:1532: "(DrawListNode 0, 1, 1 after a pressed look)" is unclear. The code draws
  (node, 1, 1, 1), waits 5 frames, then (node, 0, 1, 1) (:1599 to :1602). Write those calls.
- [ ] should src/EOV1.C:1213: the header lists only QUIRK ovl1:20FF. The inline QUIRK ovl1:1D48 (:1294)
  belongs in the same list, or the header should say it is inline.
- [ ] should src/EOV1.C:1318: the quote "You cannot have less than four characters." is cut short.
  eob/EOB.EXE holds "You cannot have less than four characters yet! Do you wish to continue anyway?"
  (read only, grep). Quote the whole string of index 0x2d, after checking which string that index is.
- [ ] note src/EOV1.C:1-9 (unit header): "which TASM cannot" is a claim about the assembler, given
  as fact without a cite. Tag it [H] or cite the escalation record.
- [ ] note src/EOV1.C:124 to :125 (CharRec known) and :372 to :374 (SpendSpellSlot): the struct says a
  negative slot is "still to be memorised", the SpendSpellSlot header says "a copy that is spent until
  the party rests". EOV6.C:27 uses the first wording. They agree (a spent spell is memorised again),
  but the two comments should use the same words.
- [ ] note src/EOV1.C:1190 (RunCampMenu header: "ds:3B8E", "2102:0020 sits at ds:3BBE"), :1067 to :1068
  (ScribeScrolls: "ds:63CB", "ds:640B", "ds:644B"), :9 ("26 8B 87 05 00 at ovl1:1277") and the quirk
  addresses ovl1:20FF, ovl1:1D48, ovl1:2558, ovl1:0401: not checked. The matcher proves the code, not
  the addresses in comments, and the pre-C transcription (git df48650^:src/EOV1.ASM) gives none of
  them to compare with. The next reader should trust them only once cited to a listing.
- [ ] note src/EOV1.C:1525 (AskYesNo): "Are you sure you wish to LOAD a saved game?" and :1318 are quoted
  whole in comments. The EXE shows "Are you sure you", "wish to LOAD a" and "wish to drop." as
  fragments; the whole strings were not read.
- [ ] note src/EOV1.C:167 (gFileNameBuf): the name says file name, but in this unit and the others it
  is the text buffer. A project-wide name question for phase 6, not for this unit.
- [ ] note src/EOV1.C:1333: the `break;` after the BTN_DROP block is kept so the match holds, and the
  comment says so. Fine as it is.
- [ ] note (registry, a question for the human): re/symbols.toml lists 12 ovl1 functions. DrawListNode
  is 1404 bytes (ovl1:1551 to ovl1:1ACD) and SpellSlotScreen 2009 (ovl1:097A to ovl1:1153). The
  matcher proves 16 functions with other sizes: DrawListNode 320, SpellSlotScreen 1892, and four
  missing rows, BoolString (ovl1:10DE, 41 bytes), SaveOptionField (ovl1:1107, 38), DrawOptionField
  (ovl1:112D, 38), ScribeScrolls (ovl1:1691, 1084). build/packets/eov1.md has the same stale rows,
  so its called-by and calls columns are wrong (DrawListNode "calls" SelectPartyMember). I did not run
  `make map` or edit re/symbols.toml. Until it is regenerated, `make review-pass` would stamp 12 of
  the 16 functions. Please regenerate, or confirm the registry is right and tell me why.

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

# Review: eov2

Reviewer: rev-haiku
Commit reviewed: c2b1de9 (main; round 3 reviewed c2b1de9, round 2 e8be50a, round 1 4649fc4)

Playbook 70. Judge what a matching file can still get wrong: names, types, comments, structure.
The matcher already proved the bytes; do not re-prove them, read for meaning.

## Findings

Each finding is a checkbox. `must-fix` blocks `make review-pass` until a decompiler ticks it off
(with the commit that fixes it); `should` and `note` do not block.

### Round 3 (rev-haiku, at c2b1de9)

Playbook 70, later round. Judged: the round 2 must-fix items (fix commit 067dd64, merged in 52b8ce6)
and the changes since e8be50a on main (`git diff e8be50a main -- src/EOV2.C re/symbols.toml
re/globals.toml`: src/EOV2.C, comments and declarations only; re/globals.toml names ds:FE5A gFont8x8;
re/symbols.toml has no eov2 entry). `make match FILE=src/EOV2.C` exits 0, 11 of 11 (3332 bytes);
`make readability UNIT=eov2` 0 findings. Checked against the code: the flash tables (ds:5621, 5629,
562D from eob/EOB.EXE DGROUP; gFlashFrame2 and gFlashShift2: all 16 entries give (mask >> shift) at
most 3) and the two readers (E0EF3.C:696, :771); the routine at 0000:6815 (disassembled with
tools/re/disasm.py: the far pointer is returned in dx:ax); struct SlotSize against src/E122E.C:107-111;
the LoadScreenImage and LoadOverlayScreen mode tests; the INTRO call (src/EOV2.C:383, mode 2);
g65C0 and g65C3 (the loop at src/EOV2.C:366-368 indexes both by i); the font loads (src/E07C8.C:376-377,
src/EOV0.C:168, src/EOV3.C:42); the FindWordPanel font call. Fixed right, ticks stay: the LoadGamePanel
font wording (its line citation is a should below), the header's mode -1 claim, the per-row g65C0 and
g65C3 comments, the SlotSize struct and gA99C type, the LoadIntroScreen region, and the f6815 comment.
Two round 2 items are not fixed right, because a changed line states a wrong claim: gFlashFrames and
gIconTable (both below, must-fix again).

- [x] must-fix src/EOV2.C:117 (gFlashFrames comment; reopens the round 2 gFlashFrames item): "Both
  readers only produce frames 1 to 3". The same comment says "both only ever give 0 to 3", and the
  code gives 0 to 3: DrawFlashFrame (src/E0EF3.C:696) and FlashViewDamage (src/E0EF3.C:771) both
  run `if (frame-- == 0) continue;`, so frame 0 is skipped and frames 1 to 3 index gFlashFrames[0..2].
  Say that: the masks give 0 to 3, 0 means no frame and is skipped, so the array holds the three
  entries DD1C, DD20 and DD24. The table values and the range are right.
  Fixed: the comment now says the readers compute a frame 0 to 3 and then skip 0 (if (frame-- == 0)
  continue;, src/E0EF3.C:696, :771), so 0 means no frame and frames 1 to 3 index gFlashFrames[0..2],
  and the array holds the three entries DD1C, DD20 and DD24 that LoadDecorateIcons fills
  (src/EOV2.C:117).
- [x] must-fix src/EOV2.C:151 (gIconTable comment; reopens the round 2 gIconTable item): "src/E1414.C:101
  ... the character the blink highlights (src/E1414.C:133 names it the highlighted member)". Neither
  line says that now. E1414.C:106 declares gIconTable as "the index of the party member being moved,
  -1 when none", and the BlinkSelected comment (1414:0000, src/E1414.C:141-145) saves gIconTable (the
  member being moved), replaces it with 0xFF and restores it. The blink-highlight wording exists only
  as a quote in src/E0ACD.C:50. Cite E1414 as it reads now: it reads the byte as the member being moved,
  like this unit, src/E0ACD.C and src/E1547.C. The cross-unit conflict with src/E0EF3.C:227 (an array of
  far sprite pointers) stays as it is.
  Fixed: the comment now cites src/E1414.C:106 (the index of the party member being moved, -1 when
  none) and the BlinkSelected comment (1414:0000, src/E1414.C:141-145), which saves and restores it as
  that member; the blink-highlight wording is gone (src/EOV2.C:151).
- [x] should src/EOV2.C:9-13 (header): the cites into src/E082C.C are one or two lines off. The mode -1
  test is at src/E082C.C:313-314 (not :314-315), the LoadScreenImage call in LoadOverlayScreen at :337
  (not :338), and the "With mode -1" comment at :329-330 (not :330-331). The facts cited are right.
  Fixed: the header now cites src/E082C.C:313-314, :337 and :329-330 (src/EOV2.C:9-13).
- [x] should src/EOV2.C:113-114 (g65C0 and g65C3): "(src/EOV2.C:357)" is this file's own line, which is
  now the loop at src/EOV2.C:366-368. Cite 366-368.
  Fixed: both comments now cite src/EOV2.C:366-368 (src/EOV2.C:113-114).
- [x] should src/EOV2.C:376-377 (LoadIntroScreen comment) and src/EOV2.C:177 (gFillRect): the 40 by 200
  ClipRect is at src/E082C.C:317 (not :318), and the gFillRect colour parameter at src/E082C.C:64 (not :63).
  Fixed: the LoadIntroScreen comment and the gFillRect declaration now cite src/E082C.C:317 and :64
  (src/EOV2.C:177, :376-377).
- [x] should src/EOV2.C:399 (LoadGamePanel entry 1): "src/E1C0E.C:53" is the gRestoreRect line. The
  declaration "the 8x8 font loaded from Font8.fnt" is src/E1C0E.C:57. Cite :57.
  Fixed: the entry 1 comment now cites src/E1C0E.C:57 (src/EOV2.C:399).
- [x] should src/EOV2.C:151 (gIconTable): src/E0ACD.C:42 is now the declaration at src/E0ACD.C:48, and
  src/E1547.C:107 is now the declaration at src/E1547.C:128. The readings they state are right.
  Fixed: the comment now cites src/E0ACD.C:48 and src/E1547.C:128 (src/EOV2.C:151); same edit as the
  must-fix above.
- [x] should re/globals.toml, ds:FE5A (changed in this range): "EOV2.C:124 declare the same address" is
  now src/EOV2.C:133. A log file, so the owner of the globals log corrects it.
  Fixed: re/globals.toml ds:FE5A now says EOV2.C:133 declares the same address.
- [x] note the f6815 comment summarises the decode as "... the loop, mov dx,ds, retf". The routine
  ends with `xchg dx,ax` at 0000:6832 before `mov dx,ds` at 0000:6833, which puts the offset in ax.
  The dx:ax return is right; the summary could cite 0000:6832-6838.
  Fixed: the comment now reads "the loop, then 0000:6832-6838: xchg dx,ax, mov dx,ds, pop ds, pop si,
  pop bp, retf" (src/EOV2.C:207-210).
- [ ] note the round 2 notes (the item table base at src/EOV2.C:29 and :83, and the gIconTable type against
  src/E0EF3.C:227) are not re-judged here and stay open as notes.

Round 3 result: two must-fix items open, both round 2 items whose changed lines state a wrong claim.
Shoulds are stale line cites (the facts are right). Nothing escalated: no matcher, spec or pin question.

### Round 2 (rev-haiku, at e8be50a)

Playbook 70, later round. Judged: the round 1 must-fix and should items (fix commits 5b7df34 and
867f836, merged in d3dc267) and the changes since 4649fc4 on main (`git diff 4649fc4 main -- src/EOV2.C
re/symbols.toml re/globals.toml`: src/EOV2.C; re/globals.toml has only gClickX and gClickY of other
units; re/symbols.toml has no ovl2 entry). Round 1 items: all fixed right and their ticks stay, except
the gFlashFrames item (reopened below). The round 1 fixes added five wrong or unevidenced claims in
changed lines, and two shoulds. `make match FILE=src/EOV2.C` exits 0, 11 of 11 functions match (3332
bytes); the changes are comments, declarations and names only. Checked against the code and the other
units: the LoadIntroScreen and LoadGamePanel bodies, the InitGameTables and SetPointerShape bodies, the
f6815 bytes (below), and the cited lines in src/E082C.C, src/E19EC.C, src/E0EF3.C, src/E0ACD.C,
src/EOV0.C, src/EOV3.C, src/EOV4.C, src/EOV5.C, src/E122E.C, src/E1414.C, src/E1547.C, src/E1BB1.ASM,
src/E07A9.C, src/E07C8.C, src/E1C0E.C, src/E0D1C.C and re/symbols.toml.

- [x] must-fix src/EOV2.C:386-387 (LoadGamePanel comment, entry 1): "selects the 6x6 font (f1A47_0169
  with gFE5A) ... then restores the 8x8 font". The code selects gFE5A (:418) and restores gA9C0 (:420),
  and the two fonts are the other way round: gFE5A is the 8x8 font (src/E07C8.C:376 loads Font8.fnt
  into it; src/E1C0E.C:53 "the 8x8 font"), and gA9C0 is the 6x6 font (src/E07C8.C:377 loads FONT6.FNT;
  src/EOV0.C:168, src/EOV3.C:42). Say that entry 1 selects the 8x8 font gFE5A, runs the character
  panel and restores the 6x6 font gA9C0.
  Fixed: the comment now says entry 1 selects the 8x8 font gFE5A (Font8.fnt, src/E07C8.C:376,
  src/E1C0E.C:53) and restores the 6x6 font gA9C0 (FONT6.FNT, src/E07C8.C:377, src/EOV0.C:168,
  src/EOV3.C:42) (src/EOV2.C:395-399).
- [x] must-fix src/EOV2.C:9-10 (header): "gOverlayBuf ... is the DGROUP image buffer the same load hands
  to 19EC:002A as its `extra` pointer". LoadOverlayScreen (src/E082C.C:338) passes mode -1, and
  LoadScreenImage returns before any f19EC_002A call when mode is -1 (src/E082C.C:314-315; its comment
  at :330-331 says the same). The buffer reaches 19EC:002A only from loads with another mode, such as the
  INTRO load at src/EOV2.C:371 (mode 2). Fix the sentence so it does not say the art loads reach 19EC.
  Fixed: the header now says gOverlayBuf is the DGROUP buffer the art loads land in, that
  LoadOverlayScreen passes mode -1 so LoadScreenImage returns before the 19EC:002A blit
  (src/E082C.C:314-315, :338), and that only a load with another mode, such as the INTRO load below
  (ovl2:078D, mode 2), hands the buffer to 19EC:002A as its `extra` pointer (src/E082C.C:330-331,
  src/E19EC.C:29-35) (src/EOV2.C:9-13).
- [x] must-fix src/EOV2.C:108 (gFlashFrames; reopens the round 1 item): "that reader uses only the three
  entries at DD1C, DD20 and DD24 (the mask and shift give frames 1 to 3)". No mask, shift or frame value
  is cited, and src/ does not show it. The first reader's tables are gFlashFrameMask (ds:5629) and
  gFlashFrameShift (ds:562D) (src/E0EF3.C:350-351), with gFlashPassMask (ds:5621, :349). A second reader
  at src/E0EF3.C:768-773 loops over sixteen frames with gFlashPass2, gFlashFrame2 and gFlashShift2
  (src/E0EF3.C:362-364, the 0EF3:06E7 reader) and also indexes gFlashFrames[frame], so "that reader"
  names only one of them. Either give the table values and the frame range each reader can produce
  (the bytes at ds:5621, 5629, 562D, and the long tables at 564B, 5677 and 56B7), or mark the frame range
  [H] and drop the three-entry claim. The claim decides whether DD28 (gIconDD28) is a separate variable.
  Fixed: the comment now gives the table values and both readers. I decoded eob/EOB.EXE DGROUP (para
  0x212E plus the 0x2800 header): gFlashPassMask at ds:5621 is 40 90 e4 b9 6e 1b 06 01, gFlashFrameMask
  at ds:5629 is c0 30 0c 03, gFlashFrameShift at ds:562D is 06 04 02 00, so DrawFlashFrame's
  (pass & frameMask[i]) >> frameShift[i] gives 0 to 3 only (max index 2); FlashViewDamage's
  gFlashPass2 (ds:564B), gFlashFrame2 (ds:5677, c0000000 30000000 ... 0000000c 00000003) and
  gFlashShift2 (ds:56B7, 1e 1c ... 02 00) also give 0 to 3. So the array holds the three entries
  DD1C, DD20, DD24 and DD28 is the separate gIconDD28 (src/EOV2.C:108).
- [x] must-fix src/EOV2.C:104-105 (g65C0 and g65C3): "the three per-column cell heights" and "the three
  per-column cell y origins". The loop at src/EOV2.C:355-357 indexes both by i, the row:
  gIconTableA92A[i * 4 + j], with x = j * 3 by column j. They are per row, not per column.
  Fixed: both comments now say per-row, indexed by the row i (0..2) (src/EOV2.C:104-105).
- [x] must-fix src/EOV2.C:142 (gIconTable): "src/E1414.C:101 ... read it as the party member being moved
  (-1 when none)". src/E1414.C:101 says "the character the blink highlights", and E1414.C:133 says "the
  highlighted member"; neither says "being moved" or -1. Only src/E0ACD.C:42 and src/E1547.C:107 say that.
  Cite E1414 for what it says; the cross-unit note below stays.
  Fixed: the comment now says this unit, src/E0ACD.C:42 and src/E1547.C:107 read it as the party member
  being moved (-1 when none), and src/E1414.C:101 as the character the blink highlights (:133 the
  highlighted member) (src/EOV2.C:142).
- [x] should src/EOV2.C:50-51 (struct SlotPos): `void far *rec` with "the far slot-size record". The same six
  bytes are declared `struct SlotSize far *rec` in src/E122E.C:113 (the record type is at :107-111).
  Use that type (define struct SlotSize, or say in the comment why this unit keeps void far *).
  Fixed: struct SlotSize is defined as in src/E122E.C:107-111 and gA99C's rec is
  `struct SlotSize far *` (src/EOV2.C:50-59); make match still 11 of 11.
- [x] should src/EOV2.C:365 (LoadIntroScreen comment): "clips the region (0, 0)-(0x28, 0xc8)". The call is
  ClipRect(0, 0, 0, 0, 0x28, 0xc8, 2, 0): x1 to y2 are 0, and 0x28 and 0xc8 are the width and height
  (src/E082C.C:318 makes the same 40 by 200 rectangle). Say "a 40 by 200 region at (0, 0)".
  Fixed: the comment now says it clips a 40 by 200 region at (0, 0), 0x28 and 0xc8 being the width and
  height (src/E082C.C:318) (src/EOV2.C:374-375).
- [x] note verified, no change asked: the f6815 declaration (src/EOV2.C:198-199, "0000:6822-682C lowercases
  'A' to 'Z'") is right. I decoded eob/EOB.EXE at 0000:6815 (file offset 0x2800 + 0x6815): push bp, mov bp,sp,
  push si, cld, push ds, lds si,[bp+6], then the loop at 6822 (sub al,0x41; cmp al,0x19; ja; add al,0x61;
  mov [si-1],al) with lodsb and jnz back to 6822, then mov dx,ds, pop ds, pop si, pop bp, retf with the
  pointer in dx:ax. The comment could cite those bytes instead of [H].
  Fixed: the [H] is gone; the comment now cites the byte decode and that the pointer is returned in
  dx:ax (src/EOV2.C:198-201).

Round 2 result: five must-fix items open (four new wrong or unevidenced claims in changed lines, one
reopened round 1 item). Nothing else needs a decompiler's decision; nothing escalated: no matcher,
spec or pin question.

### Round 1 (rev-haiku, at 4649fc4)

- [x] must-fix src/EOV2.C:351-355 (LoadIntroIcons, name and comment): the comment says it fills the DROW/creature icon tables through gGetSpriteCell (ds:F4BE, ds:A92A) and slices with ds:65C0/65C3/65C6. The body (359-365) loads the INTRO screen into gOverlayBuf (LoadScreenImage), sets gStatusLine, fills a rectangle, draws two DrawFilledPanel frames and calls ClipRect. It calls no gGetSpriteCell and writes no table; that description belongs to LoadDecorateIcons (328-349). Rename it to what the body does (for example LoadIntroScreen) and rewrite the comment. The argument also needs explaining: `handle` is gFn8072's return value, which src/E0EF3.C:389-390 says is a saved-pixel handle, but it is passed as the colour of gFillRect (src/E082C.C:63 names that parameter colour).
  Fixed: renamed to LoadIntroScreen with `make rename` (logged in re/renames.toml), and the comment now says it loads the INTRO page into gOverlayBuf, marks the status line, fills the rectangle (0,0x9f)-(0x13f,0xc7) with the saved-pixel handle gFn8072(0,0) returns, passed as gFillRect's colour argument (src/E0EF3.C:389-390, src/E082C.C:63), draws the two panel frames and clips the region; it cuts no sprite cells (src/EOV2.C:361-368).
- [x] must-fix src/EOV2.C:324-325 (LoadDecorateIcons comment): "six cells from the INTRO sheet (ds:113F)". Line 334 calls LoadOverlayScreen("DECORATE", 5, 3), and src/E082C.C:335-337 makes LoadOverlayScreen load into gOverlayBuf (ds:113F, E082C.C:74). The six gIntroCells (336-337) are cut from the DECORATE image. Name the DECORATE image.
  Fixed: the comment now names the DECORATE image (its page lands in gOverlayBuf, ds:113F; src/E082C.C:335-338) as the source of the six gIntroCells, not an INTRO sheet (src/EOV2.C:332-337).
- [x] must-fix src/EOV2.C:326 (comment) and :347 (code): "whose cell width comes from g65C0 and y from g65C3". The call is gGetSpriteCell(j * 3, g65C3[i], 3, g65C0[i], sheet): the width is the constant 3, g65C0[i] is the height and g65C3[i] the y. Say height.
  Fixed: the comment now says the cells are 3 wide, with y from g65C3 and height from g65C0 (src/EOV2.C:335-336).
- [x] must-fix src/EOV2.C:370-372 (LoadGamePanel comment): "give up after three tries" is not in the code. A failed load sets sel to 3 and gA998 to 0 (389-395); the do/while loop (378-412) redraws the panel and waits again. The only ways out are entry 0 succeeding, entry 1, and entry 2 (exit). Remove the claim. The same comment's "3 0FFh/0FEh/0" is garbled (see the should on gA998 below).
  Fixed: the claim is gone; the comment now says a failed load clears gA998 and redraws the panel to wait again, and the only exits are entry 0 succeeding, entry 1 and entry 2 (exit) (src/EOV2.C:381-389).
- [x] must-fix src/EOV2.C:197 and :402 (fOVL0_28F9): the comment says Purpose unknown, but src/EOV0.C:1447-1455 names the same address ovl0:28F9 RunCharacterPanel (the character panel's main loop, "Called by LoadGamePanel (ovl2:0846)"). Use that name here (one address, one name). The entry 1 comment (372: "restores the fonts and redraws") then misses the call; whitepaper 2.6 lists the title menu as "Load game in progress", "Start a new party", "Exit to DOS" (docs/design/WHITEPAPER.md:137), and the switch's entry 0 (load) and entry 2 (exit) fit that order, so entry 1 is probably "Start a new party" [H] until the option strings are read.
  Fixed: the stub is now RunCharacterPanel (its target ovl0:28F9, the character panel's main loop in src/EOV0.C:1447-1455), and case 1 is noted as the title menu's "Start a new party" [H] (src/EOV2.C:201, :419).
- [x] must-fix src/EOV2.C:300-301 (InitGameTables comment), with the types at :143 and :146: "clears the item, spell and creature tables" has no evidence. What the function clears: gIconTable (ds:FB34) to -1 (src/E0ACD.C:42: the member being moved, -1 when none), gEFC8, gEFCA, gB802 (36 entries; src/EOV3.C:146 declares them `void far *`), gFB5E (6 longs, src/EOV3.C:138), gA99C (6 records, of which only the far size-record pointer is cleared; src/E122E.C:105-117), and gRectKinds (200 far pointers to size records, src/E122E.C:98; src/EOV3.C:227-240 frees them). Rewrite the comment that way. gB802 and gRectKinds are declared `long` here, but gB802 is `void far *` in EOV3.C:146 and gRectKinds is `void far *` in EOV3.C:14 (and `struct RectKind far *` in E122E.C:98). Declare them to match.
  Fixed: the comment now lists what it clears (gIconTable to -1, gEFC8, gEFCA, the 36 gB802 pointers, the six gFB5E longs, the gA99C size-record pointer and the 200 gRectKinds pointers) with the cited readings; gB802 and gRectKinds are declared `void far *` (src/EOV2.C:304-310, :145, :148).
- [x] must-fix src/EOV2.C:42-45 (struct Pair): the comment says it is "the record InitGameTables clears", but InitGameTables uses no two-word record, and nothing in src/ uses struct Pair (grep finds only its definition). Remove the struct or give it a use with evidence.
  Fixed: struct Pair was dead and is gone; gA99C now uses struct SlotPos with rec, x and y (src/EOV2.C:50-54).
- [x] must-fix src/EOV2.C:416 (SetUiPalette comment): "for the VGA/Tandy modes (gGraphicsChoice 4 or 3)". Tandy is graphics choice 1 (docs/design/WHITEPAPER.md:119: 1 Tandy, 3 VGA; src/E082C.C:65-67: 0 cga, 1 tandy, 2 ega, 3 mcga/VGA, 4 vga). The branch covers 3 and 4 only ("VGA modes"); the else branch covers 0, 1 and 2.
  Fixed: the comment now says the VGA modes (gGraphicsChoice 3 and 4) and the other modes (0 CGA, 1 Tandy, 2 EGA; src/E082C.C:66-67) (src/EOV2.C:432-435).
- [x] must-fix src/EOV2.C:104-107 (gFlashFrames, gIconDD20, gIconDD24, gIconDD28): declared here as one scalar pointer at DD1C, but src/E0EF3.C:356 declares DD1C as `char far *gFlashFrames[]`, "far image pointers, indexed by frame - 1", indexed at E0EF3.C:707 and :773; its four per-frame mask bytes (gFlashFrameMask) suggest four frames. So DD1C, DD20, DD24 and DD28 may be one four-entry array, and gIconDD20 and gIconDD24 name two of its entries under other names. src/EOV1.ASM:87 calls DD28 "the icon table". Settle the array with E0EF3.C's reader (or give the evidence against it), declare it as an array, and make the names agree.
  Fixed: gFlashFrames is declared `char far *gFlashFrames[]` (matching src/E0EF3.C:356) and LoadDecorateIcons fills entries 0 to 2; the evidence against four entries is that the reader's mask and shift give frames 1 to 3, so it indexes 0 to 2, and the DD28 slot is the separate icon pointer gIconDD28 (src/EOV1.ASM:87) (src/EOV2.C:108, :348-350).
- [x] must-fix src/EOV2.C:159-161 (gGetSpriteCell comment): "a 16x16 cell position" holds only for the ITEMICN loop (289-290: width 2, height 0x10). The other calls use widths 3 to 5 and heights 0x08 to 0x20 (282-286, 338-347). Say what x, y, w and h are and that the cells differ in size.
  Fixed: the comment now gives x and y as the cell's origin and w and h as its width and height, notes the unit is not established, and contrasts the 2 by 0x10 ITEMICN icons with the 3 to 5 wide and 8 to 0x20 tall cells (src/EOV2.C:161-166).
- [x] should src/EOV2.C:6-9 (header): "The sprite sheets ... loaded through 082C:05EC/082C:051B and sliced into cells": LoadOverlayScreen loads into gOverlayBuf (E082C.C:335-337), while the sheet segments 20A3 to 20A9 are sliced in LoadItemSheets (261-290). SetPointerShape (209) and LoadDecorateIcons (337) slice from gOverlayBuf. Two sources for ITEMICN (209 and 290) are not explained; say which slice reads what and why (what f19EC_002A does with `extra`, src/E19EC.C:29-35).
  Fixed: the header now says LoadItemSheets slices the sheet paragraphs, SetPointerShape and the gIntroCells loop slice gOverlayBuf, and ITEMICN is sliced twice (the pointer cell from gOverlayBuf, the 0x58 icon cells from its 20A9 paragraph), citing f19EC_002A's `extra` pointer (src/E082C.C:301-338, src/E19EC.C:29-35) (src/EOV2.C:1-14).
- [x] should src/EOV2.C:162-163: gFn8072 and gFillRect have parameter names a, b, c, d, e. Use the names the other units use: gFillRect (x1, y1, x2, y2, colour; src/E082C.C:63) and gFn8072 (x, y; src/E0EF3.C:389).
  Fixed: gFn8072 is (int x, int y) and gFillRect is (int x1, int y1, int x2, int y2, int colour) (src/EOV2.C:167-168).
- [x] should src/EOV2.C:47-50 and :145 (struct Triple6 for gA99C): the fields v and c are guesses. src/E122E.C:105-117 names the same six bytes struct SlotPos: a far pointer rec at +0, x at +4, y at +5. Use those fields; InitGameTables clears only rec. src/E122E.C:113 uses struct SlotPos gA99C[]; EOV2 and EOV3.C:136-145 use Triple6.
  Fixed: struct Triple6 is gone; gA99C is struct SlotPos and InitGameTables clears only rec (src/EOV2.C:50-54, :322).
- [x] should src/EOV2.C:196 (LoadSavedGame at 210C:0039): that stub is the entry of ovl3:15B9, which src/EOV3.C:863 names LoadGame (EOV3.C:851-863; the entry is listed at EOV3.C:5). Use LoadGame with the stub address in the comment (src/EOV1.ASM:145 calls it _LoadSavedGame at the same stub).
  Fixed: EOV2.C now uses LoadGame (its target ovl3:15B9, src/EOV3.C:863), and src/EOV1.ASM's `_LoadSavedGame` was renamed `_LoadGame` so the stub address has one name everywhere (src/EOV2.C:200, src/EOV1.ASM:145, :3798).
- [x] should src/EOV2.C:195 and :508 (f6815): a runtime library routine at 0000:6815 (re/symbols.toml:1051, unit e_lib). The FindWordPanel comment does not say what it does to the typed word before strcmp (509). Say it with evidence, or keep the placeholder and mark it "Purpose unknown:" at the declaration.
  Fixed: the declaration now says f6815 is the library strlwr (0000:6822-682C lowercases 'A' to 'Z' and stores them back) and the FindWordPanel comment says the typed word is lowercased through it before the strcmp (src/EOV2.C:198-199, :480-487).
- [x] should src/EOV2.C:448 (LoadItemData comment): "load all the item tables", but the function also calls LoadIntroIcons and LoadGamePanel (451-457).
  Fixed: the comment now says LoadItemData loads the item data and builds the start-up UI through all seven calls (src/EOV2.C:466-468).
- [x] should src/EOV2.C:203 (SetPointerShape): "Purpose unknown." contradicts the name and the body (209-212), which state the purpose. Drop the prefix.
  Fixed: the Purpose unknown. prefix is gone (src/EOV2.C:207).
- [x] should src/EOV2.C:370-371 (gA998 in LoadGamePanel): the values written (0xFF while loading, 0xFE on success, 0 otherwise) are not explained, and "3 0FFh/0FEh/0" is garbled. Say what each value means in game terms; src/E0ACD.C:904-911 (EnterMapScreen) reads gA998 to show the inventory.
  Fixed: the comment now explains 0xff as the value while the load runs (src/EOV3.C:937 skips its map reload on it), 0xfe on success, 0 when cleared, and any nonzero as what EnterMapScreen (src/E0ACD.C:909-918) uses to show the loaded map and the inventory (src/EOV2.C:381-389).
- [x] should src/EOV2.C:461-466 (FindWordPanel comment): "draws the panel frame (through 082C:09D5, 082C:0898 ...)": 082C:0898 is DrawShadowText, which draws the text (500), not the frame (the DrawFilledPanel calls at 485-488).
  Fixed: the comment now names 082C:09D5 DrawFilledPanel for the frame and 082C:0898 DrawShadowText for the text, and 0000:6815 for the lowercasing (src/EOV2.C:480-488).
- [x] should placeholder globals without a comment (READABILITY.md, "Placeholder globals"): gColour179 (98), gCellDX (86), g65C0 (100), g65C3 (101), gIntroCells (103), gPtrDD00 (117), and the palette globals gF57E, gFB4C, gF53A, gF53B, gFB7A, gFB7B, gA9C4, gA928, gA95A, g8A6C, g8A7D (128-139). Each declaration should say what is known, or the placeholder stays with no claim.
  Fixed: gColour179, gCellDX, g65C0, g65C3, gIntroCells, gPtrDD00 and the eleven palette globals each now carry a comment naming their reader (and gIconTable's conflict with src/E0EF3.C:227) (src/EOV2.C:90, :102-108, :119, :130-142).
- [ ] note src/EOV2.C:29 and :83 (item table base): the 14-byte ITEM.DAT records start at ds:8DD0 here (LoadItems reads into gItems, 224-232), and src/E0BD2.C:13 agrees (m00 at ds:8DD0). src/E07A9.C:64 and :220, src/E08E6.C:66 and :78, and src/E0ACD.C:89 and :100 put the same records at ds:8DD2 (flags at +0) under the name gTable8DD2. All agree that the key word is at ds:8DD6 (word6 here, m04 there), so the disagreement is where record 0 starts, which decides every field offset. Not this unit's to settle alone: check record 0's address in the emulator (`make oracle-run`) before any rename, then rename with `make rename`.
- [x] note src/EOV2.C:36 and :318 (CharRec index at +00): InitGameTables writes it, but nothing in src/ reads it as an index, and src/E122E.C:116-118 says +00 is padding with status at +01. The comment should say it is written here and not read by the units so far.
  Fixed: the +00 comment now says InitGameTables writes it and no unit reads it as an index (src/E122E.C:119-121 treats +00 as padding with the status at +01) (src/EOV2.C:40-42).
- [ ] note src/EOV2.C:140 (gIconTable, ds:FB34): char here (a member index, -1 when none; src/E0ACD.C:42, src/E1414.C:101), but src/E0EF3.C:227 declares ds:FB34 as an array of far pointers. Cross-unit; the whole-game phase has to settle it.
- [x] note src/EOV2.C:173 and :186: f19EC_002A and FlushKeyBuffer are declared and never called in this file.
  Fixed: both unused declarations were removed (src/EOV2.C).
- [ ] note matcher output: `make match FILE=src/EOV2.C` exits 0 with 11 of 11 functions matching, but prints "unit eov2 not complete" even though the byte counts agree (3332 and 3332). tools/match/cmatch.py:606 requires `ovl is None`, so no overlay unit can print complete. The packet says complete True and the agent tools use re/symbols.toml, so this does not block the review; it is a matcher message for the human.
- [ ] note verified: build/re/eob.json overlays[2] (stub 2106) lists entry 2 at ovl2:09CC and entry 8 at ovl2:0577, as the header says. The stub addresses 2106:002A and 2106:0048 were not checked.

### Round 1 summary

The matcher proves all 11 functions (readability 0 findings), and most names are backed by the calls and strings they cite (LoadItems, LoadItemTypes, LoadItemSheets, FindWordPanel, the item and palette globals). The unit is not yet understandable from its source alone: four comments describe code that is not there or says the wrong thing (LoadIntroIcons, LoadDecorateIcons, LoadGamePanel's "three tries", InitGameTables, SetUiPalette's Tandy mode), and three declarations disagree with the other units that read the same bytes (fOVL0_28F9 is RunCharacterPanel in EOV0.C, gA99C and gRectKinds and gB802 have other types or layouts, gFlashFrames is an array). Placeholders that are fine as they are: f1A47_0169, f1A29_000F, the library routine f6815 once its comment says what it does, and gFn804A/gFn807E, whose names agree with src/E0ACD.C, src/E0EF3.C and src/EOV5.C. The item table base (8DD0 or 8DD2) is open between units and is a note, not a must-fix here.

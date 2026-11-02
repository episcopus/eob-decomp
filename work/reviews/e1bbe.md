# Review: e1bbe

Reviewer: rev-haiku
Commit reviewed: 68d16c2 for round 9 (main; src/E1BBE.ASM last changed at e831c19, the round 8 fixes; src/E1A72.ASM at b00a8c7, and the round 8 label cites still hold against it; the review branch is identical to main). Round 8: 20871bc (main; src/E1BBE.ASM last changed at d1e1a32, src/E1A72.ASM at 2af4699). Round 7: 07c010f (main; src/E1BBE.ASM, src/E1A72.ASM and the other files the cites name are the same in the review branch at 1d5a0ef). Round 6: a53aa55 (main; src/E1BBE.ASM last changed at 887afda, src/E1A72.ASM at e3ea88d; the review branch is identical to main in this worktree). Round 5: d1538ee for round 5 (main; src/E1BBE.ASM last changed at 6ddaa61, src/E1A72.ASM at 65ebeb1; both are identical in this worktree). Round 4: 433dc71 (main; src/E1BBE.ASM is unchanged since 5cfa933). Earlier: 421cd1e7a31c20710200fe18247bdf159a34d76c (round 3, main; src/E1BBE.ASM is identical to the review branch at 8225102, which is on main). Round 2 was reviewed at 089dfd1 and round 1 at 0cfccb1 (see below). E1A72.ASM has moved on main since round 2 (commit e856ab9), which is why round 3 finds stale citations.

Playbook 70. Judge what a matching file can still get wrong: names, types, comments, structure.
The matcher already proved the bytes; do not re-prove them, read for meaning.

## Findings

Each finding is a checkbox. `must-fix` blocks `make review-pass` until a decompiler ticks it off
(with the commit that fixes it); `should` and `note` do not block.

### Round 9 (review of commit 68d16c2, src/E1BBE.ASM unchanged since e831c19)

Checked: `make match FILE=src/E1BBE.ASM` 7 of 7 (860 bytes), exit 0; `make readability UNIT=e1bbe` 0 findings. The
addresses inside the unit were counted again from the instruction sizes, from 1BBE:0002, 1BBE:00ED, 1BBE:01FE and
1BBE:02C2 onward, and the header addresses hold: 0015, 0021, 002B, 0055, 005A, 0075, 008B, 0090, 00FD, 00FF, 010F,
0117, 0127, 0154, 0159, 015B, 021E, 0229, 022D, 0232, 0234, 0238, 023C, 0240, 0244, 025B, 0272, 0299, 029F, 02AE,
02B6, 02DE, 0300, 0307, 030A, 0332, 0335, and the push cs sites 02AE, 02F8, 0324, 034F. The cites into E1A72.ASM by label hold at
b00a8c7: L01E2 test and `je L020C` (:394-395), the drop at L01E9 (:396), the set at L020C (:405), the g80EC test and
`je L0221` (:406-407), the stores (:408-409), L0221 (:410), the 8000h test (:419) before the zone test, L01EC (:397-403,
the 4000h at :403 after the bracket), L025C (:430-435) and L0279 (:437). The cross-unit cites hold: E1BB1.ASM:21-25,
:53, :130, :162-164, :198; E1BF3.ASM:35 (`mov ds:[8138h], cx`) and :65-66; E14EF.C:75-76; E16A5.C:91-92, :536 and
:676; E1A29.C:57; E1C0E.C:49-50; E0BD2.C:112; E07A9.C:215 (the gMouseDisabled declaration). The packet caller lists
hold: 14 callers for 1BBE:01FE and 1BBE:02C2 (the header's "0EF3, 16A5, 1DE7 and the overlays"), and the Lock lists
(packet lines 25 and 26) show 0ACD:0E8A, 0ACD:0F72, ovl0:2547 and ovl5:17C8 on the show side only and ovl0:28F9 on
the hide side only. The registry names (re/globals.toml) match every ds: name in the unit, with the underscore aside.

- [x] must-fix src/E1BBE.ASM:96-101 (_gRestoreRect and _gSaveRect): the shift addresses are wrong. The header says
  "HidePointer shifts the left edge right by three, 1BBE:147-149" and "ShowPointer ... 1BBE:248-250". Neither is the
  shift. HidePointer's three `sar ax,1` are at 1BBE:004B-0050 (the `mov ax,[_gRectLeft]` before them is 0048-004A).
  1BBE:0147-0149 is ShowPointer's `mov ax,[_gRectLeft]`, and ShowPointer's three `sar` are at 1BBE:014A-014F. 1BBE:0248-0250
  is inside HidePointerZone (`sub ax,[_gPointerHotX]`, `and ax,0FFF8h`, `mov [_gRectLeft],ax`) and holds no shift.
  (Evidence: counted instruction sizes from 1BBE:0002 (the header's 0021, 0055 and 0075 hold) and from 1BBE:00ED (its
  0117, 0127 and 0154 hold). The `sar` lines are at src/E1BBE.ASM:158-160 and :259-261.) Cite 1BBE:004B-0050 for
  HidePointer and 1BBE:014A-014F for ShowPointer, in both declarations, and keep the column and row units as they are.
  Fixed: `_gRestoreRect` now cites HidePointer's three `sar ax,1` at 1BBE:004B-0050 (after the
  `mov ax,[_gRectLeft]` at 1BBE:0048-004A) and `_gSaveRect` cites ShowPointer's at 1BBE:014A-014F (after
  1BBE:0147-0149); the column and row units are unchanged. Checked against the disassembly at
  `tools/re/disasm.py eob/EOB.EXE 1BBE`; `make match FILE=src/E1BBE.ASM` still 7 of 7, 860 bytes.
- [x] must-fix src/E1BBE.ASM:66-69 (_gZoneLeft): "the handler's left test is skipped (QUIRK 1A72:0248)" is wrong. The
  left compare runs: `cmp cx,[_gZoneLeft]` and `jl L0250` (src/E1A72.ASM:421-422, now :432-433 with labels). What the
  quirk does is send a left
  miss past the right-edge test (`cmp cx,[_gZoneRight]`, `jg L025C`, :424-425) to the y tests at L0250 (:426). So the
  zone's left edge is never enforced, and only x past the right edge and y outside top and bottom take the pointer
  out of the zone. (Evidence: src/E1A72.ASM:352-353 says it in those words: "the hide zone's left edge is never
  enforced".) Say that, not "skipped".
  Fixed: `_gZoneLeft` now says the left compare runs but QUIRK 1A72:0248 sends a left miss past the right-edge test
  to the y tests at L0250, so the left edge is never enforced and only x past the right edge and y outside top and
  bottom leave the zone. `make match FILE=src/E1BBE.ASM` still 7 of 7, 860 bytes.
- [x] should src/E1BBE.ASM:104 (_gDraw80C2) and :1-3 (unit header): the `gDraw` prefix is a family guess. src/E1A29.C:53
  says so ("The `gDraw` prefix is a family guess: only ds:80C2 is evidenced as drawing"). This unit's evidence for the
  draw is ShowPointer passing the shape record and the position through it (the push sequence at src/E1BBE.ASM:305-312
  gives mode, src, x, y, and 1BBE:011E and 012E store gShapeX and gShapeY), so the name can stay, but the EXTRN comment
  should give that evidence and say the prefix is a guess, as E1A29.C does. Not a placeholder: the call is evidenced.
  Fixed: the `_gDraw80C2` EXTRN now says the `gDraw` prefix is a family guess (src/E1A29.C:53) and gives the evidence
  (ShowPointer's call at 1BBE:01F4 with mode 0 and the shape record at the unclipped gShapeX/gShapeY stored at
  1BBE:011E and 012E, push order at src/E1BBE.ASM:305-312). The name is kept; `make match FILE=src/E1BBE.ASM` still
  7 of 7, 860 bytes.
- [ ] note the bare spellings g8172, g8174, g80AA and g80A2 (EXTRN lines 79-80 and 102-103) are still open for the human's
  convention ruling. Round 8 left the same point open; the unit is consistent within itself, and no matcher or spec
  question is involved, so it is not escalated.
- [ ] note routed to the e1a29 owner: src/E1A29.C:50-52 cites "ShowPointer (1BBE:00ED, src/E1BBE.ASM:252-258)" and
  "1BBE:01F4 (`mov ax, 0` at 1BBE:01F0)". Those addresses are consistent with HidePointerZone at 1BBE:01FE (the
  header's address): counted back, `mov ax,0` at 01F0, `push ax` at 01F3, the call at 01F4, `add sp,0Ah`, the pops
  and the `ret` ending at 01FD. The source range :252-258 is stale; the draw call is at src/E1BBE.ASM:305-312 now.
- [ ] note still open, routed to the e1a72 owner: src/E1A72.ASM:339 (the header of the mouse handler) says 8000h is
  "(hidden over the hide zone)", the wording rounds 4 and 7 routed. This unit reads 8000h as "a zone hide is in force".
- [ ] note _f1BBE_0001, g80AA and g80A2 stay placeholders on the evidence the headers give.

### Round 8 (review of commit 20871bc, src/E1BBE.ASM unchanged since d1e1a32, src/E1A72.ASM at 2af4699)

Checked: `make match FILE=src/E1BBE.ASM` 7 of 7 (860 bytes); `make readability UNIT=e1bbe` 0 findings. The cites into
E1BB1.ASM (:21-25, :53, :57-59, :130, :162-164), E1BF3.ASM:35, E1C0E.C:49-50 and :52, E1A29.C:57 and
tools/re/map_units.py:42-46 hold, and so do the caller lists in build/packets/e1bbe.md. The code agrees with the
headers: the hide and show gates, the rectangle arithmetic and clip bounds, the zone stores, the four push cs far
calls and the difference between the Lock pair. The two must-fix items are stale citations, not wrong meanings.

- [x] must-fix src/E1BBE.ASM:36-39 (_gPointerBusy), :42-44 (_gCursorX), :60-62 (_gPointerState) and :127-128 and
  :222-223 (the HidePointer and ShowPointer callers): the src/E1A72.ASM line numbers are stale again (the file is
  at 2af4699). Current lines: the busy test `cmp word ptr [_gPointerBusy],0` at :386 (label L01E2), the free path
  `je L020C` at :387, the drop `L01E9: jmp L027F` at :388, the set to 1 at :397 (label L020C) and the clear to 0 at
  :429 (label L0279). The cursor: `cmp byte ptr [g80EC],2` at :398, `je L0221` at :399, the write
  `mov word ptr [_gCursorX],cx` at :400 and the reload at :402 (label L0221); InitMouse writes 0A0h at :269 and 64h at
  :270. The state: `test word ptr [_gPointerState],8000h` at :411, the zone test at :413-421, and
  `or word ptr [_gPointerState],4000h` at :395 on the in-zone path (label L01EC, whose `call far ptr _HidePointer` is
  at :393 inside the bracket at :389-394). The handler's other calls: `call far ptr _HidePointer` at :426 (label
  L025C) and `call far ptr _ShowPointer` at :427. (Evidence: `grep -n "_gPointerBusy\|_gCursorX\|_gPointerState\|
  _HidePointer\|_ShowPointer" src/E1A72.ASM`.) The round 7 numbers (:382-384, :393, :425, :396-397, :268-269, :407,
  :389, :391, :422, :423) were right for an earlier E1A72.ASM. The behaviour they describe is unchanged. Fix: cite
  the handler by label, with its grep string beside it (L01E2, L020C, L01E9, L01EC, L0221, L0235, L025C, L0279), or
  pin the commit (`E1A72.ASM@2af4699:386`). Re-grep at fix time.
  Fixed (e831c19): every cite into src/E1A72.ASM now names the handler's label with its grep string, so it stops
  drifting. _gPointerBusy cites label L01E2 (grep `_gPointerBusy`) for the test, `je L020C` for the free path, the
  drop at label L01E9, the set at label L020C and the clear at label L0279; _gCursorX cites label L020C (grep
  `_gCursorX`) for the store skipped by `je L0221`, label L0221 for the reload, and InitMouse by grep `_gCursorX`;
  _gPointerState cites grep `_gPointerState` for the 8000h test and label L01EC for the 4000h set on the hide path;
  HidePointer and ShowPointer name labels L01EC and L025C. No line number into E1A72.ASM remains. The behaviour is
  unchanged; make match FILE=src/E1BBE.ASM still reports 7 of 7, 860 bytes.
- [x] must-fix src/E1BBE.ASM:445 (LockHidePointer comment): "src/E14EF.C:69-70 call this pair 'start a frame' and
  'end a frame'" is a stale cite. The two wordings are at src/E14EF.C:75 (`LockHidePointer ... start a frame`) and
  :76 (`LockShowPointer ... end a frame`). (Evidence: `grep -n "start a frame\|end a frame" src/E14EF.C`.)
  Fixed (e831c19): the comment now cites src/E14EF.C:75 (LockHidePointer "start a frame") and :76 (LockShowPointer
  "end a frame"). Re-grepped at fix time; the two lines hold.
- [x] should src/E1BBE.ASM:11-13 (unit header, the zone pair): "the caller's rectangle, gZoneLeft..gZoneBottom" reads as
  if the stored zone is the caller's rectangle. HidePointerZone stores left and top less the sprite width and height
  (1BBE:0234/025B and 0238/0272; the EXTRN comments at :64-70 say so) and right and bottom as given. Say the zone is
  the caller's rectangle with left and top reduced by the sprite size when stored.
  Fixed (e831c19): the unit header now says the zone is "the caller's rectangle, stored in gZoneLeft..gZoneBottom
  with left and top reduced by the sprite size", matching the EXTRN comments.
- [ ] should src/E1BBE.ASM:77-78 (g8172, g8174) and :100-101 (g80AA, g80A2): still bare, while the C-declared names in
  this file carry the underscore (`_gMouseDisabled` at :48, `_gDraw80C2` at :102). The round 7 item on gMouseDisabled
  and gDraw80C2 is resolved in this file. The four bare names are bare in src/E1BB1.ASM:88-95 too, and src/E1A72.ASM
  spells the same ds:814A byte bare (`gMouseDisabled` at :85, :297 and :356). Which spelling the EXTRNs use across
  the units is a convention for a human to rule. Not decided here.
- [ ] note the caller lists in the headers hold: build/packets/e1bbe.md has 1BB1:0002 (SetMousePointerShape), 1BF3:000E
  and 1A72:00DA (InitMouse) as callers of 1BBE:0002; the same 14 callers for 1BBE:01FE and 1BBE:02C2; and the Lock lists
  differ as the headers say (0ACD:0E8A, 0ACD:0F72, ovl0:2547 and ovl5:17C8 call only the show; ovl0:28F9 only the hide).
- [ ] note build/packets/e1bbe.md lines 63-66 name ds:8182 and ds:8184 `_gPointerHeight` and ds:8186 and ds:8188
  `_gPointerWidth8`. re/globals.toml:250-267 names them gSpriteHeight, gNegSpriteHeight, gSpriteWidth and
  gNegSpriteWidth, which is what this unit uses (:88-93). The mismatch is in the packet, not in this file. Carried from
  round 7; for the tools owner.
- [ ] note resolved, no action: the round 1 item on src/E07C8.C:37 ("0 disables the mouse handler body") is gone.
  src/E07C8.C:50 now says "nonzero disables the mouse handler", the same sense as src/E1BBE.ASM:48.
- [ ] note not rechecked this round: the addresses with no label (1A72:0244, the 1BBE:0213, 02C6, 030A and 0335 test
  sites, and the push cs call sites at 1BBE:02AE, 02F8, 0324 and 034F). The labels they stand for are in this file
  and in the right order. Two addresses were recounted from 1BBE:0002 with the instruction sizes and hold: 1BBE:0015
  is the `jne` after the hide depth test, and 1BBE:0021 is the `sub` of gPointerHotX.

### Round 7 (review of commit 07c010f, src/E1BBE.ASM unchanged since 41c6802)

Checked: `make match FILE=src/E1BBE.ASM` 7 of 7 (860 bytes), exit 0; `make readability UNIT=e1bbe` 0 findings. The
cites into E1BB1.ASM (:21-25, :53, :130, :162-164, :193-200), E1BF3.ASM:35 and :65-66, E14EF.C:69-70, E16A5.C:91-92,
:536 and :676, E1A29.C:57, E0BD2.C:112, E1C0E.C:49-50 and the InitMouse address (re/symbols.toml:5703) hold. The
E1A72.ASM cites do not: that file moved on main (latest 7aabad8), so the busy flag, the cursor writes, the state bits
and the handler's calls are at other lines now. The handler's behaviour still matches what this unit's headers say;
each new line below was read from the current file. The review worktree is at 1d5a0ef, behind main at 07c010f, but
the files cited are identical on both.

- [x] must-fix src/E1BBE.ASM:36-37 (_gPointerBusy): the handler cites are stale. In src/E1A72.ASM the busy test is at
  :382 (`cmp word ptr [_gPointerBusy],0`), the free path `je L020C` is at :383, the drop `L01E9: jmp L027F` is at :384
  (no position update), the set to 1 is at :393 (label `L020C`) and the clear to 0 is at :425 (label `L0279`). Cite
  :382-384, :393 and :425. (Evidence: `grep -n _gPointerBusy src/E1A72.ASM` gives :41 (EXTRN), :382, :393 and :425 for
  the handler, and :483, :485 and :501 in _IsMouseInRect, E1A72.ASM:472-508.)
  Fixed (d1e1a32): the _gPointerBusy comment now cites src/E1A72.ASM:382-384 (the test at :382, the free path
  `je L020C` at :383 and the drop `L01E9: jmp L027F` at :384), :393 (the set to 1, label L020C) and :425 (the clear
  to 0, label L0279), with the labels named so the cite survives a move. Re-grepped at fix time.
- [x] must-fix src/E1BBE.ASM:40-41 (_gCursorX): the handler's cursor writes are at src/E1A72.ASM:396-397 (`mov
  [_gCursorX],cx`, `mov [_gCursorY],dx`), skipped by `je L0221` at :395 when `cmp byte ptr [g80EC],2` at :394 is 2.
  InitMouse sets 0A0h and 64h at :268-269, not :267-268. (Evidence: `grep -n _gCursorX src/E1A72.ASM` gives :59 (EXTRN),
  :268, :396 and :398 (the reload `mov cx,[_gCursorX]`).)
  Fixed (d1e1a32): the _gCursorX comment now cites src/E1A72.ASM:396-397 (the writes), the g80EC test at :394 with
  `je L0221` at :395, and InitMouse at :268-269 (0A0h and 64h); src/E1BF3.ASM:35 is unchanged. Re-grepped at fix time.
- [x] must-fix src/E1BBE.ASM:57-59 (_gPointerState): the handler lines are stale. The 8000h test is at
  src/E1A72.ASM:407, before its zone test at :409-417. The 4000h set is at :391, after the in-zone hide (`call far ptr
  _HidePointer` at :389, inside the bracket of :385-390, which starts at label `L01EC`). Cite :389, :391, :407 and
  :409-417. (The header says :383, :385 and :400.)
  Fixed (d1e1a32): the _gPointerState comment now cites src/E1A72.ASM:407 (the 8000h test) before its zone test at
  :409-417, and the 4000h set at :391 on the in-zone path (label L01EC) that calls _HidePointer at :389, inside the
  bracket at :385-390. Re-grepped at fix time.
- [x] must-fix src/E1BBE.ASM:118-119 and :212-213 (HidePointer and ShowPointer headers): the handler's calls are
  stale. The handler calls _HidePointer at src/E1A72.ASM:389 (the in-zone hide) and :422 (the hide in the L025C path),
  and _ShowPointer at :423. The headers name :383 and :415 for HidePointer and :416 for ShowPointer. (Evidence:
  `grep -n "_HidePointer\|_ShowPointer" src/E1A72.ASM` gives :28-29 (EXTRN), :389, :422 and :423.)
  Fixed (d1e1a32): the HidePointer header now names src/E1A72.ASM:389 (the in-zone hide, label L01EC) and :422 (the
  hide in the L025C path); the ShowPointer header names :423 (the L025C path, after the hide at :422). Re-grepped at
  fix time.
- [x] should src/E1BBE.ASM:90-91 (_gRestoreRect, _gSaveRect, "far fn (x, y, w, h, buf)"): the declarations do not
  give the units the code passes. x and w are in 8-pixel columns and y and h are in pixel rows: HidePointer shifts the
  left edge right by three (src/E1BBE.ASM:147-149) before its restore call (:190-191), and ShowPointer does the same
  (:248-250) before its save call (:291-292). Say so in both declarations.
  Fixed (d1e1a32): both declarations now say x and w are in 8-pixel columns and y and h in pixel rows, and name the
  shift (HidePointer 1BBE:147-149, ShowPointer 1BBE:248-250) that produces the column value.
- [x] should src/E1BBE.ASM:36-41, :57-59 and :118-119, :212-213 (every cite into src/E1A72.ASM): cite the handler's
  label as well as the line (`L020C`, `L0279`, `L01EC`, `L025C`), so the cite still points at the code when the file
  moves. Rounds 3, 5, 6 and 7 each found E1A72.ASM cites in this unit pointing at other code.
  Fixed (d1e1a32): every cite into the handler now carries its label: L020C and L0279 in the _gPointerBusy comment,
  L0221 in the _gCursorX comment, L01EC in the _gPointerState comment, L01EC and L025C in the HidePointer and
  ShowPointer headers.
- [ ] note still open, routed to the e1a72 owner: src/E1A72.ASM:108-109 (the _gPointerState declaration) says 8000h is
  "the pointer is hidden over the hide zone". This unit reads it as "a zone hide is in force" (round 4), and the
  handler's test at src/E1A72.ASM:407 fits this unit's reading.
- [ ] note the packet names ds:8182 to ds:8188 wrongly (build/packets/e1bbe.md lines 63-66): the packet takes the first
  name in each comment, and the comments at src/E1BBE.ASM:84-89 begin "copy of _gPointerHeight" and "copy of
  _gPointerWidth8". re/globals.toml:250-252 names ds:8182 gSpriteHeight, which is right. A packet-tool question for the
  human; the source is correct.
- [ ] note the Lock caller lists in the packet are cut at 2000 characters. Line 25 (LockHidePointer) shows ovl0:28F9 and
  none of 0ACD:0E8A, 0ACD:0F72, ovl0:2547 or ovl5:17C8 in its visible part; line 26 (LockShowPointer) shows those four
  and not ovl0:28F9. The hide list's tail is cut, so "ovl5:17C8 calls only the show" is checked only as far as the
  visible part.
- [ ] note names checked and supported this round: _gPointerBusy, _gCursorX and _gPointerState and their meanings
  match the handler's code (the busy test and drop, the writes skipped at g80EC 2, the 8000h test before the zone test,
  the 4000h set after the in-zone hide); InitMouse is 1A72:00DA (re/symbols.toml:5703); HidePointer and ShowPointer are
  called by src/E1BB1.ASM:130 and :198 and by src/E1BF3.ASM:65-66, as the headers say; LockHidePointer and
  LockShowPointer match src/E14EF.C:69-70. The handler entry 1A72:017A and the zone test address 1A72:0244 are as
  src/E1A72.ASM's own header gives them; they are not checked against the binary this round.
- [ ] note _f1BBE_0001, g80AA and g80A2 stay placeholders on the evidence the headers give.

### Round 6 (review of commit a53aa55, src/E1BBE.ASM unchanged since 887afda)

Checked: `make match FILE=src/E1BBE.ASM` 7 of 7 (860 bytes), exit 0; `make readability UNIT=e1bbe` 0 findings. The
round 5 wording fixes ("busy flag", the handler's set and clear, the underscored EXTRNs, the named callers) are in the
code. Every citation into another unit was re-grepped against current main. The E1A72.ASM cites have gone stale again:
that file moved on main (65ebeb1, 1578a9b, e3ea88d) after the round 5 fixes were made, and E14EF.C moved too. The names
and the comment meanings hold against the code; the problems are the cites and one caller count.

- [x] must-fix src/E1BBE.ASM:36-38 (_gPointerBusy): the handler cites are stale. In src/E1A72.ASM the busy test is at
  :376-377 (`cmp word ptr [_gPointerBusy],0`, `je L020C`), the drop is at :378 (`L01E9: jmp L027F`), the set to 1 is at
  :387 (label `L020C`) and the clear to 0 is at :418 (label `L0279`). The cites :344-346, :355 and :386 no longer hold
  these. (Evidence: `grep -n _gPointerBusy src/E1A72.ASM` gives :376, :387 and :418.) Cite :376-378, :387 and :418.
  Fixed (41c6802): the _gPointerBusy comment now cites src/E1A72.ASM:376-378 (the test, the drop at :378 and the
  free path at :387) and :418 (the clear to 0); re-grepped at fix time.
- [x] must-fix src/E1BBE.ASM:40-42 (_gCursorX): the cites are stale. The handler writes the cursor at
  src/E1A72.ASM:390-391, skipped when g80EC is 2 (`cmp byte ptr [g80EC],2` at :388, `je L0221` at :389). InitMouse sets
  0A0h at :267 and 64h at :268. The cites :358-359, :356-357 and :236-237 no longer hold these. src/E1BF3.ASM:35 is still
  right (`mov ds:[8138h],cx`). (Evidence: `grep -n _gCursorX src/E1A72.ASM` gives :58, :267, :390 and :392.)
  Fixed (41c6802): the _gCursorX comment now cites src/E1A72.ASM:390-391 (the handler writes), :388-389 (the g80EC
  test) and :267-268 (InitMouse); src/E1BF3.ASM:35 is unchanged. Re-grepped at fix time.
- [x] must-fix src/E1BBE.ASM:56-57 (_gPointerState): the cites are stale. The handler tests 8000h at src/E1A72.ASM:400,
  before its zone test at :402, and sets 4000h for an in-zone hide at :385, on the path that calls _HidePointer at :383.
  The cites :368 and :353 no longer hold these. (Evidence: `grep -n _gPointerState src/E1A72.ASM` gives :107, :270,
  :385 and :400.)
  Fixed (41c6802): the _gPointerState comment now cites src/E1A72.ASM:400 (the 8000h test) before its zone test at
  :402, and :385 (4000h set for a hide in the zone) on the path that calls _HidePointer at :383.
- [x] must-fix src/E1BBE.ASM:115-116 and :210-211 (HidePointer and ShowPointer headers): "the mouse handler's two calls
  (src/E1A72.ASM:351 and :383)" is wrong. :351 is no longer a call. The INT 33h handler (1A72:017A) calls _HidePointer
  at src/E1A72.ASM:383 and :415 and _ShowPointer at :416, so ShowPointer has one handler call, not two. The handler is
  also inside _InitMouse's proc (src/E1A72.ASM:258-425; the handler label is at :337), so "InitMouse (1A72:00DA)" and
  "the handler" name the same proc. InitMouse's own setup code (:258-336) calls neither function. (Evidence:
  `grep -n "_HidePointer\|_ShowPointer" src/E1A72.ASM` gives :27-28, :383, :415 and :416.) Name the calls by line:
  HidePointer at :383 and :415, ShowPointer at :416.
  Fixed (41c6802): both headers now name the mouse handler inside _InitMouse (1A72:00DA; the handler entry is
  1A72:017A): HidePointer at src/E1A72.ASM:383 and :415, ShowPointer at :416. The separate "InitMouse" caller is
  dropped because the handler is inside that proc and its setup calls neither function.
- [x] must-fix src/E1BBE.ASM:431 (LockHidePointer comment): "src/E14EF.C:44-45" is stale. The pair is declared at
  src/E14EF.C:59-60 with the comments "start a frame" (:59) and "end a frame" (:60). E14EF.C:44-45 are now the gFC7A and
  g5D1A externs. Cite :59-60; the quoted words still hold. (Evidence: `grep -n "start a frame" src/*.C` gives
  src/E14EF.C:59 only.)
  Fixed (41c6802): re-grepped at fix time: E14EF.C moved again when the e14ef unit merged (b708ffb), so the pair now
  sits at src/E14EF.C:69-70, not :59-60. The quoted "start a frame" (:69) and "end a frame" (:70) hold and the
  comment cites :69-70.
- [x] should src/E1BBE.ASM:49 (_gPointerHotX): "hot spot, x in pixels (subtracted from gCursorX)" is right for
  ShowPointer (1BBE:00ED subtracts it from gCursorX, :228), but HidePointer subtracts it from gDrawnX (1BBE:0002,
  :131). Say both.
  Fixed (41c6802): _gPointerHotX now says HidePointer subtracts it from gDrawnX (1BBE:0021) and ShowPointer from
  gCursorX (1BBE:0117); _gPointerHotY says the same for gDrawnY (1BBE:002B) and gCursorY (1BBE:0127).
- [x] should src/E1BBE.ASM:381-382, :421-422, :449-450 and :472-473 (`push cs` then `call near ptr _HidePointer` or
  `_ShowPointer`): this is a far call to the FAR procs in this segment. Both procs are `PROC FAR` (:119, :213), so their
  `ret` is RETF and pops the CS that `push cs` put on the stack. A reader will take the pair for a near call. Say so once,
  in the HidePointerZone header, and point the other sites to it. (Evidence: the `ret` at :198 and :301.)
  Fixed (41c6802): the HidePointerZone header now explains that the push cs before the near call to 1BBE:0002
  (1BBE:02AE-02AF) makes it a far call: that proc is FAR, so its RETF (1BBE:00EC) pops the pushed CS, and it names
  the other three sites (1BBE:02F8, 0324 and 034F). Each push cs in the file carries the note "far call: the FAR ret
  pops this CS (see 1BBE:01FE)".
- [ ] note routed to the e1a72 owner: src/E1A72.ASM:107-108 (the _gPointerState declaration) says 8000h is "the pointer is
  hidden over the hide zone". This unit reads 8000h as "a zone hide is in force" (round 4). The handler's own test
  (src/E1A72.ASM:400-402) fits this unit's reading, so the two declarations should agree.
- [ ] note resolved, no action here: the item routed in rounds 1 to 4 (src/E07C8.C:37 "0 disables") is gone. src/E07C8.C:50
  now says nonzero disables the mouse handler, as this unit's _gMouseDisabled declaration does.
- [ ] note names checked and supported this round: the registry names in re/globals.toml for ds:8154 to ds:8188, ds:812E,
  ds:8138, ds:813A, ds:8146, ds:814A, ds:804E and ds:80AE match this file's EXTRN names (the underscore aside);
  gSaveRect and gRestoreRect match src/E0BD2.C:112 and src/E1C0E.C:49-50; gDraw80C2 matches src/E1A29.C:57; the far
  argument order (x, y, w, h, buf) and (mode, src, x, y) match the pushes at :278-297; HidePointerZone and ShowPointerZone
  match src/E16A5.C:91-92 and their calls at :536 and :676; the E1BB1 citations (:21-25, :53, :57-59, :130, :162-164)
  and src/E1BF3.ASM:35 hold; the c_like reading (tools/re/map_units.py:42-46) holds.
- [ ] note the Lock caller lists hold: the show-only callers (0ACD:0E8A, 0ACD:0F72, ovl0:2547, ovl5:17C8) and the
  hide-only caller (ovl0:28F9) are as the comment says in build/packets/e1bbe.md. The packet cuts its lines at 2000
  characters; the visible part of the show list has no ovl0:28F9 between ovl0:2547 and ovl1:0000.
- [ ] note _f1BBE_0001, g80AA and g80A2 stay placeholders on the evidence the header gives.

### Round 5 (review of commit d1538ee, src/E1BBE.ASM unchanged since 6ddaa61)

Checked: `make match FILE=src/E1BBE.ASM` 7 of 7 (860 bytes), exit 0; `make readability UNIT=e1bbe` 0 findings. The
round 4 fixes are in the code. The addresses in the headers agree with the code: I counted the labels and jumps of
HidePointer, ShowPointer, HidePointerZone, ShowPointerZone and the two Lock wrappers (1BBE:0015, 0055, 005A, 0075,
0234, 0238, 023C, 0240, 025B, 0272, 0281, 0299, 029F, 02B6, 02DE, 0300, 021E, 0229, 022D, 0232, 00FD, 00FF, 010A,
010F, 0213, 02C6, 030A, 0335). The cross-unit citations to E1BB1.ASM (:21-25, :53, :73, :88-95, :130, :162-164,
:193-200), E1BF3.ASM:35, E14EF.C:44-45, E16A5.C:91-92, E1A29.C:57, E1C0E.C:49-50 and E0BD2.C:112 hold. The
citations to E1A72.ASM at :353 and :368 hold; those at :355-356 and :233-234 do not (see the first must-fix).

- [x] must-fix src/E1BBE.ASM:36-38 (_gCursorX): the citations are stale. E1A72.ASM has moved since round 3: the
  handler's writes are at :358-359 (`mov [_gCursorX],cx`, `mov [_gCursorY],dx`), the g80EC test is at :356-357
  (`cmp byte ptr [g80EC],2`, `je`), and :355-356 is now `mov [_gPointerBusy],1` and that compare. InitMouse's
  `mov [_gCursorX],0A0h` is at :236-237, not :233-234. (Evidence: grep `_gCursorX` in src/E1A72.ASM gives :49, :236,
  :358 and :360. src/E1BF3.ASM:35 is still right.) Cite :358-359, :356-357 and :236-237. The e1a72 review routed the
  same stale citations to this unit (work/reviews/e1a72.md, the note on src/E1BBE.ASM:36-37), and this is the
  third round the cites have drifted; re-grep each E1A72 line before citing it.
  Fixed (887afda): the _gCursorX comment now cites src/E1A72.ASM:358-359 (handler writes), :356-357 (g80EC test)
  and src/E1A72.ASM:236-237 (InitMouse); src/E1BF3.ASM:35 is unchanged. Re-grepped at fix time.
- [x] must-fix src/E1BBE.ASM:16, :17, :32, :301, :313, :395 and :426 (the word "semaphore" for _gPointerBusy): a
  semaphore promises mutual exclusion, and this unit says it does not (:427-429: "not mutual exclusion", "not
  atomic"). The e1a72 review already ruled the same word wrong for this flag (work/reviews/e1a72.md, must-fix at
  :19, and the note at :141: "The E1BBE owner should use the same term"). re/globals.toml:297 and src/E1BB1.ASM:53
  and :73 say "busy flag". Use "busy flag" in all seven places ("takes the busy flag", "releases the busy flag");
  keep the name `_gPointerBusy`.
  Fixed (887afda): "semaphore" is now "busy flag" in all seven places (and in the two additional places at :391
  and :453); the name `_gPointerBusy` is kept.
- [x] must-fix src/E1BBE.ASM:32-35 (_gPointerBusy): "each taker tests then increments" is false for the mouse
  handler, which is also a taker of this flag. The handler tests it and drops its event when it is set (src/E1A72.ASM
  :344-346, `jmp L027F` at :346 with no position update), sets it to 1 when free (:355) and clears it to 0 at :386.
  It does not increment and it does not wait. A porter needs both behaviours: the wrappers in this unit wait while
  the flag is set (1BBE:0213, 02C6, 030A, 0335) and then increment it, and the handler drops its event. Say so, with
  the citations to :344-346, :355 and :386.
  Fixed (887afda): the _gPointerBusy comment now says the wrappers wait while it is set and then increment it
  (1BBE:0213, 02C6, 030A, 0335), while the mouse handler tests it and drops its event when set
  (src/E1A72.ASM:344-346), sets it to 1 when free (:355) and clears it to 0 (:386); it does not increment and does
  not wait.
- [x] should src/E1BBE.ASM:42 and :89 (gMouseDisabled, gDraw80C2): the bare spellings are still open (rounds 1 to 4).
  The playbook gives the form for a C-declared object: docs/process/playbooks/30-lift-module.md:27, `EXTRN _gDrvReg:BYTE
  ; @ 0000:3B36`. Both names are C declarations (src/E07A9.C:203 `extern int gMouseDisabled`, src/E1A29.C:57
  `extern void (far *gDraw80C2)`), and this unit already uses the underscored form for every other C-declared name.
  Applying the playbook gives `_gMouseDisabled` and `_gDraw80C2`. The same bare form is in E1A72.ASM:67, E1C00.ASM:40
  and EOV1.ASM:84, so the fix crosses units. Not escalated: it is a naming convention, and the matcher does not see
  the spelling. The human should confirm the rule once, then the units follow it.
  Fixed (887afda): this unit now uses _gMouseDisabled and _gDraw80C2 in the EXTRNs and everywhere they are
  referenced; the cross-unit spelling is left for the human's rule.
- [x] should src/E1BBE.ASM:112-113 and :206-207 (HidePointer and ShowPointer headers, "Called by 1BB1:0002, 1BF3:000E and
  1A72"): "1A72" names no function. The packet lists the callers: InitMouse (1A72:00DA) and the mouse handler's two
  calls (src/E1A72.ASM:351 and :383). Name them, so the list can be checked against the packet.
  Fixed (887afda): both headers now name InitMouse (1A72:00DA) and the mouse handler's two calls
  (src/E1A72.ASM:351 and :383).
- [ ] note names checked and supported this round: the far pointers g8172 and g8174 and the far functions g80AA and
  g80A2 are declared in src/E1BB1.ASM:88-95 with the same meanings, and their pushes and calls (src/E1BB1.ASM:193-200,
  src/E1A72.ASM:347-352 and :379-381) match this unit's; gSaveRect and gRestoreRect (src/E1C0E.C:49-50,
  src/E0BD2.C:112) take (x, y, w, h, buf) as the pushes at src/E1BBE.ASM:174-184 and :274-284 give; gDraw80C2 takes (mode, src, x,
  y) as the pushes at :287-293 give and as src/E1A29.C:57 declares; HidePointerZone and ShowPointerZone match
  src/E16A5.C:91-92; LockHidePointer and LockShowPointer match src/E14EF.C:44-45 ("start a frame", "end a frame").
- [ ] note the cross-unit citations routed in rounds 1 to 4 (src/E1A72.ASM header cites, src/E1A29.C:50 and :54,
  src/E1C00.ASM:41, :287, src/E07C8.C:37) are not rechecked this round; they belong to their owners.
- [ ] note _f1BBE_0001 stays a placeholder (its evidence is the segment start).

### Round 4 (review of commit 433dc71, src/E1BBE.ASM unchanged since 5cfa933)

Checked: `make match FILE=src/E1BBE.ASM` 7 of 7 (860 bytes), exit 0; `make readability UNIT=e1bbe` 0 findings; the
round 3 fixes are in the code and their citations hold (E1A72.ASM:233-234 and :355-356 with the g80EC test at :353,
E1BB1.ASM:130 before the size stores at :162-164, E1BF3.ASM:35). The call lists in build/packets/e1bbe.md match the
Lock and Zone header claims (the Lock lists differ as the header says; the two Zone lists are identical). Each
function's header was traced against its code path by path, including the jumps that skip the hide and show calls.

- [x] must-fix src/E1BBE.ASM:305-307 (HidePointerZone header): "Either way the hide call [ds:80AA], 1BBE:0002,
  [ds:80A2] runs" is wrong. When the pointer's rectangle misses the argument rectangle, the compares at
  src/E1BBE.ASM:356-363 jump to L02B6 (`jl`, `jg`, `jle`, `jg`), which skips the hide and the bracket at L029F
  (:365-371). Only gPointerState = 8000h is left. The hide runs on the already-hidden path (:332 `jmp short L029F`)
  and when the rectangles overlap (:364 sets 4000h, then falls into L029F). Say that; "either way" must go.
  Fixed (6ddaa61): the header now says a disjoint rectangle jumps to 1BBE:02B6, skipping the hide and the bracket
  and leaving only 8000h, and that the hide runs on the already-in-force path (1BBE:0232 jmp short L029F) and when
  the rectangles overlap (1BBE:0299 sets 4000h, then L029F).
- [x] must-fix src/E1BBE.ASM:49, :297 and :305 (and the unit header, :11-13): bit 8000h is called "hidden" (the
  `_gPointerState` declaration says "bit 8000 hidden"; the HidePointerZone header says "when the pointer is already
  hidden (gPointerState bit 8000)" and "8000h (hidden)"). A disjoint zone hide (see the finding above) sets 8000h and
  hides nothing, so the pointer is not hidden. The handler's own use shows the meaning: src/E1A72.ASM:365 tests 8000h
  before the zone test (:367-372), and the in-zone path at 1A72:01EC then hides the pointer and sets 4000h
  (:350). So 8000h means a zone hide is in force; 4000h means a redraw is owed at the show (set by the first hide
  when the rectangles overlap, and by the handler's in-zone hide). Reword the three places.
  Fixed (6ddaa61): the _gPointerState declaration now says bit 8000 a zone hide is in force and bit 4000 a redraw is
  owed at the show (with the handler citations); the HidePointerZone header says "a zone hide is already in force"
  and "8000h (a zone hide is in force)" and "4000 (a redraw is owed at the show)"; the unit header now names both
  meanings. (Current lines: src/E1A72.ASM:368 tests 8000h, :353 sets 4000h in the in-zone hide.)
- [x] should src/E1BBE.ASM:297-298 and :305-306: a repeat hide (the already-hidden path, :021E to :029F) runs the
  hide without recomputing the rectangle or the overlap, so bit 4000h is decided by the first hide only. The show
  balances the count first (:394-400), so the code is consistent; the header should say the overlap test is made
  only at the first hide, so the reader does not take 4000h as per-hide.
  Fixed (6ddaa61): the header now says the repeat hide runs "without recomputing the zone or the rectangle, so bit
  4000 is decided by the first hide alone (1BBE:0232 jmp short L029F)".
- [x] should src/E1BBE.ASM:116 and :120 (HidePointer): the label `disabled` names the gMouseDisabled exit (:115-116),
  but it is also the target of the no-buffer exit (`je disabled`, :120, taken when _gPointerBackSeg is zero).
  Rename the label for both cases (a label rename changes no bytes; the matcher confirms).
  Fixed (6ddaa61): renamed `disabled` to `skipHide`, which names both exits; make match still reports 7 of 7, 860 bytes.
- [x] should src/E1BBE.ASM:307 (HidePointerZone header, "always paired with 1BBE:02C2"): the packet call lists are
  identical (build/packets/e1bbe.md: the same 14 callers for both), which supports "at the same call sites"; the code
  does not show that every hide is followed by its show. "Always" is the overclaim round 2 removed from the Lock
  comments. Soften it the same way.
  Fixed (6ddaa61): the header now says "the packet lists the same 14 callers for 1BBE:02C2, so the pair is called at
  the same sites" (no "always").
- [x] should src/E1BBE.ASM:11-13 and :295-304 (zone wording): the zone is the caller's rectangle, not "the rectangle
  the pointer was hidden over". src/E16A5.C:536 passes the node's box grown by the sprite size, and HidePointerZone
  stores it with left and top reduced again by the sprite size (1BBE:0234-0272). Say "the zone (the caller's
  rectangle)" in both headers.
  Fixed (6ddaa61): the unit header and the HidePointerZone header now say "the zone (the caller's rectangle)".
- [ ] should src/E1BBE.ASM:41 and :84 (gMouseDisabled, gDraw80C2): the bare spellings are still open from round 1, and
  the other EXTRNs of C-declared globals in this unit carry the underscore (_gHideDepth for gHideDepth, _gSaveRect for
  gSaveRect and _gRestoreRect, _gPointerBusy). The C declarations are src/E07A9.C:200 (gMouseDisabled) and
  src/E1A29.C:57 (gDraw80C2). A bare EXTRN names a different symbol than the underscored one, so if these names are
  linked as written they do not resolve. The matcher (cmatch) strips leading underscores, so it cannot tell. This is
  a convention across E1A72, E1C00, E1BBE and E1BB1, not a matcher or spec question: the human should decide it
  once; the unit should follow the decision. Left open (6ddaa61): no decision yet; the bare spellings are unchanged
  and the finding needs the human's convention ruling before this unit can follow it.
- [ ] note routed to the e1a72 owner: src/E1A72.ASM:85-86 and :292-293 say 8000h is "hidden over the hide zone", the
  same wording as the finding above. Its header comments at :81-83, :88, :92 and :420 still cite E1BBE.ASM lines
  (188, 193, 328-332, 338-339) that no longer hold the cited code; round 3 routed this and it is still open.
- [ ] note routed (round 3, not rechecked this round): src/E1A29.C:50, :54, :73 and src/E1C00.ASM:41, :287 cite
  E1BBE.ASM lines that have moved. Owners: e1a29, e1c00.
- [ ] note still open from round 1, another owner: src/E07C8.C:37 still says "0 disables the mouse handler body";
  the code and src/E07A9.C:200 say non-zero disables. Route to the E07C8 owner.
- [ ] note names checked and supported this round: the far calls to g80AA (pushed g8172/g8174) and g80A2 (no
  arguments) match src/E1BB1.ASM:195, :200 and src/E1A72.ASM:346, :349; the HidePointer and ShowPointer callers
  (src/E1A72.ASM:348, :380-381; src/E1BF3.ASM:65-66; src/E1BB1.ASM:130, :198) match the header's list; gSaveRect and
  gRestoreRect take (x, y, w, h, buf) as the pushes at :166-177 and :266-277 give, and as src/E1C0E.C:533, :576 and
  src/E0BD2.C:622 call them; HidePointerZone's (left, top, right, bottom) matches src/E16A5.C:91; ShowPointerZone
  matches src/E16A5.C:92 and :676; LockHidePointer and LockShowPointer match src/E14EF.C:44-45 ("start a frame",
  "end a frame").
- [ ] note _f1BBE_0001 stays a placeholder (its evidence is the segment start); the g80AA and g80A2 names rest on their
  call sites, which the header says.

### Round 3 (review of commit 421cd1e, src/E1BBE.ASM at 8225102)

Checked: `make match FILE=src/E1BBE.ASM` 7 of 7 (860 bytes) and `make readability UNIT=e1bbe` 0 findings. Every
function's behaviour in its header was re-read against the code (HidePointer, ShowPointer, HidePointerZone,
ShowPointerZone, LockHidePointer, LockShowPointer), and the prototypes in src/ were checked (see the notes).

- [x] must-fix src/E1BBE.ASM:35-37 (_gCursorX): the cited writers are wrong. src/E1A72.ASM:338-339 is the
  button-state store (`mov [g8134],ax`, `mov [g8130],bx`), not a cursor write. src/E1A72.ASM:218-219 no longer
  holds InitMouse; it has moved to :233-234 (`mov word ptr [_gCursorX],0A0h`, `[_gCursorY],64h`). The handler
  writes the cursor at src/E1A72.ASM:355-356 (`mov [_gCursorX],cx`, `[_gCursorY],dx`), skipped when g80EC is 2
  (:353-354). src/E1BF3.ASM:35 is right. Cite :233-234, :355-356 and E1BF3.ASM:35. This reopens the round 2
  item: its fix took the lines from an E1A72.ASM that main has since changed (e856ab9).
  Fixed: the _gCursorX comment now cites src/E1A72.ASM:355-356 (handler, skipped when g80EC is 2 at :353-354),
  src/E1A72.ASM:233-234 (InitMouse sets 0A0h) and src/E1BF3.ASM:35; re-grepped at fix time (grep for `_gCursorX`
  in src/E1A72.ASM gives :233, :355, :357, :360).
- [x] must-fix src/E1BBE.ASM:102-104 (HidePointer header): "The height and width used are the ones 1BB1 stored
  before it calls here" is backwards. 1BB1 calls _HidePointer (src/E1BB1.ASM:130) before it stores the new hot
  spot, height and width (src/E1BB1.ASM:133-164: `mov [_gPointerHeight],ax` at :162, `[_gPointerWidth8],dx`
  at :164). HidePointer therefore reads the sizes still in place, the old shape's. src/E1BB1.ASM:42-44 says
  the same ("1BBE:0002 hides the pointer first, while the old buffer and sizes are still in place"). Say that.
  Fixed: the header now says the height and width used are the ones still in place, the old shape's, because
  1BB1 calls here (src/E1BB1.ASM:130) before it stores the new sizes (src/E1BB1.ASM:162-164), which keeps the
  old buffer valid while the shape is replaced.
- [x] must-fix src/E1BBE.ASM:299-300 (HidePointerZone header): "top as drawn" is wrong. The rectangle top is the
  drawn y less the hot spot y, not rounded (src/E1BBE.ASM:344-346: `mov ax,[_gDrawnY]`, `sub ax,[_gPointerHotY]`,
  `mov [_gRectTop],ax`). The left is formed the same way (:333-336, then `and ax,0FFF8h`). Say "top is the
  drawn y less the hot spot y (no rounding)".
  Fixed: the header now says top = the drawn y less the hot spot y (no rounding).
- [x] should src/E1BBE.ASM:414-416 (LockHidePointer, LockShowPointer): the name "Lock" is the callers' name
  (src/E14EF.C:44-45 describe them as "start a frame" and "end a frame"), but the comment never says what is
  locked. The busy flag is a test then an increment, not a lock (src/E1BB1.ASM:53 says "not a lock"; this
  unit's own _gPointerBusy comment says it too). Say that Lock means the frame bracket (the busy flag and the
  [ds:80AA]/[ds:80A2] calls), not mutual exclusion. Keep the name, src/ uses it.
  Fixed: both comments now say "Lock" is the callers' name for the frame bracket, not mutual exclusion, and
  cite src/E14EF.C:44-45 ("start a frame"/"end a frame"), the test-then-increment at 1BBE:030A and
  src/E1BB1.ASM:53; the name is kept.
- [x] should src/E1BBE.ASM:136-147 (HidePointer header, "1BBE:0055, 005A, 008B and 0090 jump to the increment at
  1BBE:00E0"): the x tests (`jle L0075`, `jge L0075`, :136-139) reach 1BBE:00E0 through the `jmp short L00E0`
  at L0075 (:147), not directly. The y tests (:153, :155) jump straight to L00E0. Say "reach".
  Fixed: the header now says 1BBE:0055 and 005A reach the increment at 1BBE:00E0 through the jmp short at
  1BBE:0075, while 1BBE:008B and 0090 jump there directly.
- [x] should src/E1BBE.ASM:379-383 (ShowPointerZone header): "the pointer is shown only when bit 4000 (redraw)
  was set" applies to the zero-count path only. A non-zero count shows the pointer regardless of bit 4000
  (src/E1BBE.ASM:393-397 `dec word ptr [_gPointerState]`, then `jmp short L02E9`). Put "at zero" into that
  sentence so the reader does not take it for both.
  Fixed: the header now says a non-zero count decrements and shows the pointer regardless of bit 4000, and
  "at zero" the state is cleared and the pointer is shown only when bit 4000 was set.
- [x] should src/E1BBE.ASM:22-24, :40, :64 (underscore spelling): the header lists "gPointerHeight, gPointerWidth8,
  gMouseDisabled" without the underscore, the EXTRNs for the same names use _gPointerHeight, _gPointerWidth8, and
  gMouseDisabled is bare at :40 while _gCursorX has the underscore. src/E1A72.ASM does the same mix (`_gCursorX`
  at :49, bare `gMouseDisabled` at :66), as does src/E1C00.ASM:40. Still undecided from rounds 1 and 2; it is a
  convention for the three units, not this unit alone. Keep the header in the spelling the EXTRNs use.
  Fixed: the header now spells the names as this file's EXTRNs do (_gPointerHeight, _gPointerWidth8,
  _gPointerHotX, _gPointerHotY, _gPointerBackOff, _gPointerBackSeg, _gPointerShapeOff, _gPointerShapeSeg;
  gMouseDisabled stays bare, matching its bare EXTRN at :41). The EXTRNs are unchanged.
- [ ] note stale citations into this unit from other owners (routed, not changed here): src/E1A72.ASM:81, :83
  and :420 cite E1BBE.ASM:188 and :193 (the ShowPointer copies of gCursorX/gCursorY are now at :215 and :220);
  src/E1A72.ASM:88 cites :328-332 and :92 cites :338-339 (the zone shifts are now at :337-341 and :347-348);
  src/E1A29.C:50, :54 and :73 cite E1BBE.ASM:252-258 (the draw call is now at :279-285); src/E1C00.ASM:41 and :287
  cite E1BBE.ASM:89-90 and :178 (the disabled tests are now at :111-112 and :205-206). Owners: e1a72, e1a29, e1c00.
- [ ] note src/E07C8.C:37 still says "0 disables the mouse handler body" (round 2 note, another owner). Route to the
  E07C8 owner; not this unit.
- [ ] note build/packets/e1bbe.md lists ds:8182 and ds:8184 as _gPointerHeight and ds:8186 and ds:8188 as
  _gPointerWidth8. This file declares them _gSpriteHeight, _gNegSpriteHeight, _gSpriteWidth and
  _gNegSpriteWidth (src/E1BBE.ASM:73-78). The packet tool takes the first name in each comment; the source is right.
- [ ] note names checked and supported this round: _HidePointer and _ShowPointer (src/E1BB1.ASM:130, :198;
  src/E1A72.ASM:26-27; src/E1BF3.ASM:14-15); HidePointerZone and ShowPointerZone match the prototypes in
  src/E16A5.C:91-92 (left, top, right, bottom); gDraw80C2 matches src/E1A29.C:57 (mode, src, x, y), and the
  pushes at E1BBE.ASM:279-285 give that order; gSaveRect (src/E0BD2.C:112) and gRestoreRect (src/E1C0E.C:49) match
  the argument order (x, y, w, h, buf) of the pushes at :166-177 and :266-277; the overlap test in HidePointerZone
  (:352-360) is the one the header describes; the call-site lists quoted for the Lock wrappers match
  build/packets/e1bbe.md; _f1BBE_0001 stays a placeholder (its evidence is the segment start).

### Round 2 (review of commit 089dfd1)

- [x] must-fix src/E1BBE.ASM:33-34 (_gCursorX): "written at src/E1A72.ASM:324-325" is wrong; those lines do not write the cursor. The writers are src/E1A72.ASM:338-339 (the handler stores the clamped position) and src/E1A72.ASM:218-219 (InitMouse sets 0A0h and 64h). The round 1 fix copied the wrong lines. (Evidence: grep for `_gCursorX` in src/E1A72.ASM on main gives lines 46, 218, 338 and 340; src/E1BF3.ASM:35 is right.) Fixed: the _gCursorX comment now cites src/E1A72.ASM:338-339 (handler), 218-219 (InitMouse) and src/E1BF3.ASM:35.
- [x] must-fix src/E1BBE.ASM:184 (ShowPointer header): "1BBE:0154, 0159, 018B and 0190 jump to ShowDone" is wrong for 0159. The `jl` at 1BBE:0159 (line 230) jumps to 1BBE:015E, the on-screen path; when it falls through it reaches the `jmp ShowDone` at 1BBE:015B (line 231). 1BBE:0154 (`jle` to 015B), 018B and 0190 are the jumps that reach ShowDone. (Evidence: lines 228-232, 243-246; the label L015E is line 232.) Fixed: the header now says 0154, 018B and 0190 reach ShowDone after the decrement, and the off-screen fall-through at 0159 reaches the jmp ShowDone at 015B.
- [x] must-fix src/E1BBE.ASM:88-89 (HidePointer header): "Does nothing when the pointer is disabled (gMouseDisabled) or when no save-under buffer exists" is wrong for a hide while already hidden. The hidden test (lines 108-109) runs before the buffer test (lines 110-111), so with gHideDepth non-zero and no buffer the call still increments gHideDepth (1BBE:0015 to 00E0). The no-buffer exit applies only when the pointer is not hidden. (Evidence: lines 108-111 and 173.) Fixed: the header now says a hide while already hidden only increments gHideDepth even with no buffer, and the no-buffer exit applies only when not hidden.
- [x] should src/E1BBE.ASM:173-175 (HidePointer): the depth guard (`inc`, `jne HideDone`, `dec` on wrap) is not explained here, though the same guard for the state byte is explained in the HidePointerZone header (lines 285-287). Say that gHideDepth saturates at 0FFFFh and why (a wrap to zero would make a hidden pointer look shown). Fixed: added a comment at L00E0 saying gHideDepth saturates at 0FFFFh and the increment is undone on the wrap to zero because a zero depth would make a hidden pointer look shown.
- [x] should src/E1BBE.ASM:46-47 (_gZoneLeft): "less the sprite width" does not say the unit. The code subtracts _gPointerWidth8 shifted left three times (lines 328-332), so it is the sprite width in pixels (_gPointerWidth8 times 8). The top is reduced by _gPointerHeight with no shift (lines 338-339), since the height is already in pixels. Say so. Fixed: _gZoneLeft now says the sprite width in pixels (_gPointerWidth8 shifted left three times), and _gZoneTop says _gPointerHeight already in pixels, no shift.
- [x] should src/E1BBE.ASM:14-15 (unit header): "Every wrapper takes the pointer semaphore ... and brackets its work with [ds:80AA] and [ds:80A2]" is not true of the no-redraw exit of ShowPointerZone: with a zero count and bit 4000h clear it clears gPointerState and releases the semaphore with no bracket (lines 389-391 and 399-401). Say "every wrapper that hides or draws". Fixed: the header now says every wrapper that hides or draws, and names the ShowPointerZone no-redraw exit (1BBE:02DE/0300) that clears the state and releases the semaphore with no bracket.
- [x] note Round 1 note on g8172 and g8174 cited src/E1A72.ASM:69-70; in the current E1A72.ASM the pair is declared at lines 81-82. Checked: no source comment cites E1A72.ASM:69-70; the pair is declared at src/E1A72.ASM:81-82. The reference was in the Round 1 review note only.
- [x] note Still open, another owner: src/E07C8.C:37 on main still says "0 disables the mouse handler body". src/E1BBE.ASM:37, src/E07A9.C:184 and src/E1C00.ASM:40 say non-zero disables. Route to the E07C8 owner. Routed: not this unit; src/E1BBE.ASM:37 already says non-zero disables.

### Round 1 (commit cf49f97, fixed in 8868249)

- [x] must-fix src/E1BBE.ASM:76 (HidePointer, 1BBE:0002 comment): "Nothing happens ... when it is already hidden (gHideDepth non-zero)" is wrong. When gHideDepth is non-zero the code skips the restore (lines 92-93 and 125) but still increments gHideDepth (lines 157-158), so a nested hide is counted. Say that a hide while hidden only increments the depth. (Evidence: lines 92-93, 125, 157-159.) Fixed: the comment now says a hide while already hidden (1BBE:0015) only increments gHideDepth instead of restoring, and cites the off-screen increments (1BBE:0055/005A/008B/0090 to 1BBE:00E0).
- [x] must-fix src/E1BBE.ASM:268-269 (HidePointerZone comment): "the four arguments are stored in gZoneLeft..gZoneBottom" is wrong for left and top. Left is stored as the argument less the sprite width in pixels (lines 300 and 312), top as the argument less the sprite height (lines 301 and 318-319). Right and bottom are stored as given (lines 302-303). (Evidence: E1BBE.ASM:300-303, 312, 318-319; E1A72.ASM:64-68 reads the stored values.) Fixed: the gZoneLeft..gZoneBottom declaration now says left and top are stored as the argument less the sprite width/height (1BBE:0234/025B and 0238/0272) and right and bottom as given (1BBE:023C/0240).
- [x] must-fix src/E1BBE.ASM:270-271 (HidePointerZone comment): "the pointer's rectangle is recomputed exactly as in 1BBE:0002 ... and the left and top reduced by the sprite size" is wrong in two ways. The rectangle is in pixels, not in columns, and is not clipped (lines 304-321, no shift and no clip; 1BBE:0002 clips at lines 118-143). The sprite size is subtracted from the zone's left and top, not from the rectangle (lines 312 and 319). (Evidence: E1BBE.ASM:304-321 against 98-143.) Fixed: the HidePointerZone comment now says the rectangle is computed in pixels without clipping (1BBE:0244..0278) and that the sprite width/height are subtracted from the zone's left and top.
- [x] must-fix src/E1BBE.ASM:268 (HidePointerZone comment, "the low byte ... is advanced (not past zero)"): wrong. The low byte saturates at 0FFh: lines 294-298 increment AL and undo the increment on wrap (`inc al; jne; dec al`), so the count stops at FFh, not at zero. (Evidence: E1BBE.ASM:294-298.) Fixed: the comment now says the low byte is incremented and saturates at 0FFh (1BBE:0229 `inc al`, 1BBE:022D `dec al` on wrap).
- [x] must-fix src/E1BBE.ASM:56 and :58 (_gSpriteHeight, _gSpriteWidth): "the upper y clip bound" and "the upper x clip bound" are wrong. These are the sprite size, added to the top and left to get the bottom and right edges (lines 119, 135, 215, 230). The upper bounds are the constants 28h and 0C8h (lines 116, 126, 132, 141 and 211, 221, 227, 236). The lower bounds _gNegSpriteWidth and _gNegSpriteHeight (lines 56-59 comment) are right. (Evidence: E1BBE.ASM:114-143 and 209-238.) Fixed: _gSpriteHeight/_gSpriteWidth are now the sprite height/width added to the top/left to get the bottom/right, with the clip bounds 0 and 0C8h/28h named; the _gNeg* comments stay.
- [x] must-fix src/E1BBE.ASM:50-53 (_gRectLeft, _gRectTop, _gRectRight, _gRectBottom): "clipped rectangle, ... in 8-pixel columns" and "in pixel rows" describe only HidePointer and ShowPointer. HidePointerZone (lines 307, 314, 317, 321) writes them unclipped and in pixels, and HidePointerZone's rectangle is then compared with the zone in pixels (lines 323-330). Say that the rectangle is clipped in columns and rows in HidePointer and ShowPointer, and unclipped pixels in HidePointerZone. (Evidence: E1BBE.ASM:98 and 118 against 307 and 314.) Fixed: the _gRect* declaration now says the rectangle is in 8-pixel columns/pixel rows and clipped in HidePointer/ShowPointer but in pixels and unclipped in HidePointerZone, which compares it with the zone.
- [x] must-fix src/E1BBE.ASM:65 (g80C6): "the sprite size query (1BB1:0094)" is wrong. It is the save-under buffer size query, called as (width, height) with its result the new buffer size (src/E1BB1.ASM:33, and the call at E1BB1.ASM:166); src/E1C0E.C:52 names it "the save-under buffer size in bytes". 1BB1:0094 is the MemAlloc site that allocates the result (src/E1BB1.ASM:39), not the query. (Evidence: E1BB1.ASM:33, 39, 166; E1C0E.C:52.) Fixed: the g80C6 EXTRN was unused in this unit (finding on line 28) and was dropped, so the wrong comment is gone.
- [x] should src/E1BBE.ASM:165-172 (ShowPointer comment): "Nothing happens ... when no save-under buffer exists" is incomplete. gHideDepth is decremented before the buffer test (lines 182-186), so the count still changes. The comment also does not say that an off-screen rectangle skips the screen work while the depth still moves: HidePointer jumps to L00E0, which increments (lines 114-117, 131-133, 157), and ShowPointer goes to ShowDone after its decrement (lines 209-213, 226-228). src/E1BB1.ASM:47-48 relies on this. Fixed: the ShowPointer comment now says gHideDepth is decremented first (1BBE:00FF) and that the off-screen jumps (1BBE:0154/0159/018B/0190) still move the count.
- [x] should src/E1BBE.ASM:350-354 (ShowPointerZone comment): "otherwise the call does nothing" is not quite right. When the low byte is zero and bit 4000h is clear, gPointerState is still cleared to 0 (line 368) and the semaphore is released. Say that it clears the state and draws nothing. Fixed: the comment now says it clears gPointerState (1BBE:02DE) and releases the semaphore (1BBE:0300) and draws nothing.
- [x] should src/E1BBE.ASM:384-386 and :406-408 (LockHidePointer, LockShowPointer comments): "always paired with" is not shown by the packet. The HidePointerZone and ShowPointerZone call-site lists match exactly, but the Lock lists do not: LockShowPointer has 0ACD:0E8A (RunMapLoop), 0ACD:0F72 (EnterMapScreen), ovl0:2547 and ovl5:17C8 which LockHidePointer does not list, and LockHidePointer has ovl0:28F9 which LockShowPointer does not. Soften to "usually" or cite the pairing per caller. (Evidence: build/packets/e1bbe.md call lists.) Fixed: both Lock comments now say "usually paired" and name the unpaired call sites.
- [x] should src/E1BBE.ASM:15-17 (header): "which the compiler's C functions never do" is a claim about Turbo C++ code generation with no evidence in the unit. "The unit is hand-written assembly" is also untagged. src/E1BB1.ASM:57-59 gives the only evidence for the hand-written reading ([H]: the c_like test in tools/re/map_units.py:42-46). Cite that, tag the reading [H], and either drop the compiler claim or prove it by compiling it. Fixed: the header now tags the hand-written reading [H], cites tools/re/map_units.py:42-46 and src/E1BB1.ASM:57-59, and drops the compiler claim.
- [x] should src/E1BBE.ASM:28 (_gPointerBusy, "busy-wait acquire"): this reads as a lock. The acquire is a test, then an increment, not atomic. src/E1BB1.ASM:53 says so ("not a lock"). The wrappers here do the same test-then-increment (lines 289-291, 360-361, 391-393, 413-415). Say so. Fixed: the _gPointerBusy declaration now says each taker tests then increments, not atomic, and cites src/E1BB1.ASM:53 and the wrappers (1BBE:0213/02C6/030A/0335).
- [x] should src/E1BBE.ASM:40-43 (_gZoneLeft, _gZoneTop, _gZoneRight, _gZoneBottom): the comments say "hide zone, left/top in pixels", but the stored left and top are the argument values less the sprite size (lines 312, 319), so they are not the zone as given. Say so in the comment. The mouse handler never tests the zone's left edge (src/E1A72.ASM:268-270, QUIRK 1A72:0248), so "1A72:0244 tests it" is only partly true; say that too. Fixed: the _gZone* declaration now says left and top are stored less the sprite width/height (1BBE:0234/025B, 0238/0272), right and bottom as given, and that the handler's left test is skipped (QUIRK 1A72:0248).
- [x] should src/E1BBE.ASM:33-34 (_gPointerWidth8): "sprite width in 8-pixel columns (byte +2 + 1)" is stated as fact. src/E1BB1.ASM:23-25 marks the meaning of the width byte as a reading [H] (the byte is probably a count one less than the width). Tag it the same way. Fixed: the _gPointerWidth8 comment now says the stored value is record byte +2 plus one and tags the 8-pixel-column meaning [H] with src/E1BB1.ASM:21-25.
- [x] should src/E1BBE.ASM:21 and :65 (g80C6): the EXTRN is not used by any code in this unit (grep finds only the header line 21 and the EXTRN at line 65). Drop it, or say why it is kept. Assembling without it should leave the bytes unchanged; the matcher confirms. Fixed: the g80C6 EXTRN was dropped and its header mention removed; make match still reports 7 of 7, 860 bytes.
- [ ] should src/E1BBE.ASM:32 and :64 (gMouseDisabled, gDraw80C2): the EXTRN spelling has no leading underscore, while the other C-declared names here carry one (_gRestoreRect, _gSaveRect, _gHideDepth). src/E1A72.ASM:53 and src/E1C00.ASM:40 use the same bare spelling for gMouseDisabled. The convention is not settled across the files; choose one for all of them after checking how TASM and TLINK resolve the names. Not decided here.
- [x] should src/E1BBE.ASM:29 (_gCursorX, "(1BF3/1A72 own it)"): cite the writers, src/E1A72.ASM:324-325 and src/E1BF3.ASM:35, instead of naming the owners in words. Fixed: the _gCursorX comment now cites src/E1A72.ASM:324-325 and src/E1BF3.ASM:35.

- [ ] note src/E07C8.C:37 (another owner): the comment "0 disables the mouse handler body" contradicts src/E1C00.ASM:40, src/E1BBE.ASM:32 and src/E07C8.C:154-156 (the code sets gMouseDisabled = 1 on failure and = !mouse otherwise, so non-zero disables). Route to the E07C8 owner.
- [ ] note Names checked and supported: _HidePointer, _ShowPointer (called from src/E1BB1.ASM:130 and 198 and the E1A72 InitMouse site); _HidePointerZone and _ShowPointerZone match the prototypes in src/E16A5.C:91-92 and their parameter order (left, top, right, bottom); LockHidePointer and LockShowPointer match src/E14EF.C:44-45, src/E0ACD.C:32-33, src/EOV6.C:62-63, src/E1547.C:100-101 and src/E1C0E.C:39-40; gDraw80C2, gRestoreRect and gSaveRect match the signatures in src/E1A29.C:57, src/E1C0E.C:49 and src/E0BD2.C:112 (the argument order (x, y, w, h, buf) matches the stack pushes at E1BBE.ASM:144-155 and 239-251).
- [ ] note _f1BBE_0001 is a placeholder with the segment-start evidence the header gives (packet: no direct call). Keep it.
- [ ] note g80AA, g80A2 are names with no evidence beyond the call sites (the header says so). They are placeholders and fine as they are; g8172 and g8174 are the far pointer argument pair (src/E1A72.ASM:69-70).

## Summary

Round 9 (68d16c2). The unit's bytes and readability gates are clean, and the names are supported by their callers
and the registry. Two must-fix items block a pass, both wording errors in the comments on the EXTRNs, not in the
headers: the shift addresses cited for the rectangle units in the _gRestoreRect and _gSaveRect declarations (they are
1BBE:004B-0050 and 1BBE:014A-014F, not 147-149 and 248-250), and "the handler's left test is skipped" for the zone
(the left compare runs; the quirk is that a left miss skips the right-edge test, so the left edge is never enforced).
One should on the gDraw prefix, which E1A29.C calls a family guess. The E1A72 label cites from round 8 still hold at
b00a8c7. Nothing escalated: no matcher, spec or pin question. Result: review-failed. The decompiler fixes the two
must-fix items, ticks them with the commit, and hands off again.

Round 8 (20871bc). The unit's bytes and readability gates are clean, and the names and comment meanings hold against
the code. Two must-fix items block a pass, both stale citations: the src/E1A72.ASM line numbers in the busy flag,
cursor, state and caller comments (rounds 3, 5, 6, 7 and 8 each found E1A72 cites out of date; the labels still point
at the right code), and the src/E14EF.C:69-70 cite in the LockHidePointer comment. One should on the zone wording in
the unit header, and the bare-spelling question stays open for a human ruling. Nothing escalated: no matcher or spec
question. Result: review-failed. The decompiler fixes the two must-fix items, preferably by label or with a pinned
commit so the cites stop drifting, ticks them with the commit, and hands off again.

Round 7 (07c010f). The unit's bytes and readability gates are clean, and the names and comment meanings hold against
the code. Four must-fix items block a pass, and all four are citations into src/E1A72.ASM: the busy flag, the cursor
writes, the state bits and the handler's calls. That file moved on main after round 6, so those lines now point at
other code, while the behaviour they describe still matches the current handler. Shoulds: give the units of the
rectangle calls, and cite the handler by label as well as by line. Nothing escalated: every item is a citation or a
comment, not a matcher or spec question. Result: review-failed. The decompiler re-greps each handler line at fix time
(or cites the label), ticks the four items with the commit, and hands off again.

Round 6 (a53aa55). The unit's bytes and readability gates are clean, the round 5 wording fixes are in the code, and the
names and comment meanings hold against the code. Five must-fix items block a pass, all of them citations or a caller
count: the E1A72.ASM lines for the busy flag, the cursor writes and the state bits, the handler's calls (one of them
is two calls where ShowPointer has one), and the E14EF.C cite for the Lock pair. The E1A72.ASM cites went stale because
that file moved on main (e3ea88d) after the round 5 fixes, so they are wrong again, not the round 5 fixes themselves.
The decompiler should re-grep each cited line at fix time, and a reviewer should check cites against the current main,
not against the commit the last round used. Shoulds: the hot-spot comment, and the `push cs` / near call form, which a
reader will take for a near call. Nothing escalated: every item is a citation, a count or a comment, not a matcher or
spec question. Result: review-failed. The decompiler fixes the five must-fix items, ticks them with the commit, and
hands off again.

Round 5 (d1538ee). The unit's bytes and readability gates are clean, and the addresses in the headers agree with the
code. Three statements still block a pass. (1) The _gCursorX citations into E1A72.ASM point at the wrong lines, because
that file moved after round 3. (2) "semaphore" is still used for _gPointerBusy here, though the e1a72 review ruled it
wrong for this flag and routed it to this unit. (3) "each taker tests then increments" is false for the mouse handler,
which sets the flag to 1, clears it to 0 and drops its event when it is set. Shoulds: the bare EXTRN spellings (the
playbook form is underscored for C-declared data; a cross-unit rule for the human to confirm) and the "1A72" caller
names. Nothing escalated: every item is a citation, a term or a comment, not a matcher or spec question.
Result: review-failed. The decompiler fixes the three must-fix items, ticks them with the commit, and hands off again.

Round 4 (433dc71). The unit's bytes and readability gates are clean, the round 3 fixes hold, and the names are
supported by their callers and their evidence. Two statements are still wrong and block a pass: the HidePointerZone
header says the hide call runs whether or not the rectangles overlap (a disjoint rectangle skips it), and bit 8000h
is called "hidden" when a disjoint zone hide leaves the pointer visible (the handler uses it as "zone in force").
Shoulds: the repeat-hide and 4000h wording, the `disabled` label used for two exits, "always paired" in the Zone
header, the zone wording, and the open bare-EXTRN spelling (a convention the human should decide across units).
Result: review-failed. The decompiler fixes the two must-fix items, then hands off again.

Round 3 (421cd1e). The unit is understandable from its source: the names are supported by their callers and their
evidence, the prototypes in src/ agree, and the bytes and readability gates are clean. Three statements are still
wrong and must be fixed before a pass: the _gCursorX writers (the round 2 citations no longer point at the cursor
code, because E1A72.ASM moved on main), the HidePointer header's claim that it uses the sizes 1BB1 stores before the
call (1BB1 hides first and stores after, so it uses the sizes still in place), and "top as drawn" in the
HidePointerZone header (the top is the drawn y less the hot spot). Two shoulds (the Lock name and the underscore
spelling) and one ShowPointerZone clarity point are open. Other owners have stale citations into this unit (e1a72,
e1a29, e1c00), listed as notes. Result: review-failed. The decompiler fixes the three must-fix lines, and the
shoulds if it can, then hands off again.

Round 2 (089dfd1). The names hold against the evidence, and the round 1 fixes are in the code, but this round found three wrong statements, one of them introduced by the round 1 fix: the cursor writers cited for gCursorX (lines 324-325 are not writes), the 0159 exit in ShowPointer's header (a fall-through, not a jump to ShowDone), and HidePointer's claim that a hide with no buffer does nothing (it nests while hidden). I counted the instruction sizes from the labels, and the other addresses in the header, HidePointer, ShowPointer, HidePointerZone and ShowPointerZone hold (for example 1BBE:0015, 0055, 005A, 008B, 0090, 00FD, 010F, 021E, 0229, 022D, 0234, 025B, 0272). The far-call argument order matches the pushes. Still open from round 1: the underscore spelling of the bare EXTRN names (gMouseDisabled, gDraw80C2, g80AA, g80A2, g8172, g8174), which is an unchecked should to decide across units, and the note on src/E07C8.C:37. Placeholders kept on purpose: _f1BBE_0001 (its evidence is the segment start) and the far pointers g80AA, g80A2, g8172 and g8174 (their evidence is the call sites). Result: review-failed. The decompiler fixes the three must-fix lines and the shoulds, then hands off again.

### Round 1 summary

The seven functions match (860 bytes, make match 7 of 7) and make readability reports 0 findings. The names are mostly supported: the function names follow from their callers (src/E1BB1.ASM, src/E1A72.ASM, src/E14EF.C, src/E0ACD.C) and from what each function does, and the external names agree with the C prototypes that use them. The problems are in the comments, not the names. Seven must-fix items are wrong statements: HidePointer's "nothing happens when already hidden" (it nests), HidePointerZone's storage of the zone (left and top are reduced by the sprite size), its rectangle ("exactly as 1BBE:0002", but in pixels and unclipped), the count's limit (it saturates at FFh, not zero), the sprite-size bounds comments (the sizes are not clip bounds), the rectangle comments that ignore HidePointerZone, and g80C6 (a buffer size query, not a sprite size query). With those corrected the unit can be understood from its source alone. Placeholders left: _f1BBE_0001 (kept on purpose), g80AA and g80A2 (call sites only). The unit is reviewed as round 1; the decompiler should fix the must-fix items and hand off again.

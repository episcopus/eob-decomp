# Review: e1bbe

Reviewer: rev-haiku
Commit reviewed: 433dc71 for round 4 (main; src/E1BBE.ASM is unchanged since 5cfa933). Earlier: 421cd1e7a31c20710200fe18247bdf159a34d76c (round 3, main; src/E1BBE.ASM is identical to the review branch at 8225102, which is on main). Round 2 was reviewed at 089dfd1 and round 1 at 0cfccb1 (see below). E1A72.ASM has moved on main since round 2 (commit e856ab9), which is why round 3 finds stale citations.

Playbook 70. Judge what a matching file can still get wrong: names, types, comments, structure.
The matcher already proved the bytes; do not re-prove them, read for meaning.

## Findings

Each finding is a checkbox. `must-fix` blocks `make review-pass` until a decompiler ticks it off
(with the commit that fixes it); `should` and `note` do not block.

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

# Review: e1bf3

Reviewer: p6-rev-3 (round P6; round 1 by rev-sonnet)
Commit reviewed: ce0608f (round P6, 2026-11-04; diffed against af6af1dd6cfefe54987a3e1774dbeff10dedab70, round 1's commit).

Playbook 70. Judge what a matching file can still get wrong: names, types, comments, structure.
The matcher already proved the bytes; do not re-prove them, read for meaning.

## Findings

### Round P6 (p6-rev-3, 2026-11-04, commit ce0608f)

Judged: round 1 had no must-fix (four shoulds and four notes; the shoulds are still open below), and
what changed since af6af1d (`git diff af6af1d..ce0608f -- src/E1BF3.ASM re/symbols.toml
re/globals.toml`). The diff is names only: the four `_f1BF3_*` placeholders became
_PadBeforePlacePointer (1BF3:000D), _PlacePointer (1BF3:000E), _MovePointerTo (1BF3:0087) and
_MovePointerBy (1BF3:00A8), and the two E1BBE callees `_f1BBE_0002`/`_f1BBE_00ED` became
_HidePointer/_ShowPointer. No instruction or operand changed. `make match FILE=src/E1BF3.ASM` 4 of 4
(196 bytes); `make readability UNIT=e1bf3` 0 findings; `make names UNIT=e1bf3` 0 placeholders.

The new names are true and recorded in re/renames.toml: 000E clamps (CX, DX) to ds:814C..814E and
ds:8150..8152, stores gCursorX/gCursorY, moves the INT 33h cursor (function 4) and redraws through
the save/HidePointer/ShowPointer/restore bracket, so PlacePointer says what it does; 0087 sets the
cursor to (x, y) under the busy flag and calls it (MovePointerTo); 00A8 adds a delta to
gCursorX/gCursorY and calls it (MovePointerBy); 000D is the lone 00 pad before the even entry, named
like _PadBeforeHidePointer in E1BBE.ASM and _PadBeforeUnpack0 in E1BA8.ASM. _HidePointer/_ShowPointer
are the real E1BBE function names. The old "Placeholder names" note is resolved by the rename.

Notes (not blocking):

- note src/E1BF3.ASM:13-14: the EXTRN lines now name _HidePointer and _ShowPointer; their comments
  are still just the addresses (1BBE:0002, 1BBE:00ED). A one-line role note would make the file read
  on its own, but the names are self-explanatory.
- note the round 1 notes stand: 1BF3:0087 has no direct caller in the resident image (only 00A8 has
  callers, 0EF3:1069 and 0EF3:10AE), the busy flag ds:812E is a spin lock with no timeout, and the
  000D pad is the segment's word alignment.
- note the source now uses the real E1BBE names; the four shoulds in round 1 are the remaining wording
  items in this file, unchanged by P6.

## Round 1 (rev-sonnet, af6af1d)

- [ ] should src/E1BF3.ASM:5-6: "(with the two arguments at ds:8172/8174)" reads as two separate arguments. E1BB1.ASM (its writer of the same globals) calls them one far pointer (8172 offset, 8174 segment), and the code here pushes 8174 then 8172 and cleans up 4 bytes (`add sp, 4`, 1BF3:0063), which is one far pointer passed to the function at ds:80AA. Say "the far pointer at ds:8172/8174".
- [ ] should src/E1BF3.ASM:4-7: the header says the pointer is redrawn "when the position changed" but never says changed from what. The test (1BF3:0054..005B) compares ds:8138/813A with ds:815C/815E. Those are the last drawn position: 1BBE:0111..0124 copies 8138/813A into 815C/815E, and the mouse handler at 1A72:0221..022F makes the same comparison. Name them in the header ("compared with the position last drawn, ds:815C/815E").
- [ ] should src/E1BF3.ASM:6: "bracketed by the two functions of unit 1BBE" is imprecise. The order in 000E is: call [80AA], then 1BBE:0002, then 1BBE:00ED, then call [80A2]; the two 1BBE functions sit between the two pointer calls (the same order as 1A72:01F4..0200 and 1A72:0264..0275), they do not surround them. Reword to "between".
- [ ] should src/E1BF3.ASM:3-5: "redraws the game's own pointer" for ds:80AA/80A2 is an inference [H]. The evidence is whitepaper section on the mouse ("The game draws its own pointer") and the identical call sequence in the INT 33h handler at 1A72; nothing here shows what 80AA (called with a far pointer) or 80A2 (no arguments) does individually. Mark it [H] or say "calls the indirect functions". E1BB1.ASM leaves them unnamed, which is the right level.
- [ ] note Evidence for the data names was checked against the resident image: ds:8148 is 0 when no driver is found and 1 when found (1A72:011B, 1A72:014A), ds:8144 is 1 in the 640-wide case (1A72:013E, tested before `shl cx,1`, whitepaper 124), ds:814E/8152 are initialised to 013Fh/00C7h (1A72:00FE..0104; the 320x200 screen) and 814C/8150 are zero in the static data and only read by this unit, so "limits (x: 814C..814E, y: 8150..8152)" is right. The clamp uses signed compares (`jl`/`jle`).
- [ ] note 1BF3:0087 has no direct caller in the resident image (searched the bytes `87 00 F3 1B` in EOB.EXE and the overlay dumps in build/re/eob-overlays; the packet agrees). Only 00A8 has callers (0EF3:1069 and 0EF3:10AE, both with delta (0,0), which therefore only forces the redraw check and the INT 33h position update). 0087 may be unused library code; no change needed.
- [ ] note The busy flag ds:812E is a spin lock with no timeout (`cmp [812E],0 / jne` on itself). It is the same semaphore that 1BB1:0002 takes, so a caller that holds it and calls here would hang. This is how the original is; nothing to do, but a later reader should not "fix" it.
- [ ] note The 00 byte at 1BF3:000D: the addresses 1BF3:0000..000C are the last 13 bytes of unit e1bbe (its segment 1BBE:0001, 860 bytes, ends at 1BBE:035C = 1BF3:000C), which is why the unit starts at 000D. The header's explanation (the unit's start marker byte, like 1990:0001 and 1BB1:0001) holds up.
- [ ] note Names are still placeholders (`_f1BF3_xxxx`) and no name claims more than the evidence. Once the 80AA/80A2 roles are known, 000E could be named for what it is (move cursor to clamped position and redraw), and 0087/00A8 as set and move; these are suggestions only.

## Summary

The unit is four small pieces and is understandable from the source: the near helper 000E clamps a cursor position to its limits, stores it, tells the INT 33h driver (x doubled in the 640-wide mode) when a driver exists, and calls the redraw sequence if the position differs from the last drawn one; 0087 and 00A8 are far wrappers (set and move by delta) that take the shared busy flag. The header is accurate in what it states and the globals agree with E1BB1.ASM and with the 1A72 mouse code. Open points are wording only (the single far pointer argument, the missing mention of ds:815C/815E, "bracketed", and the [H] status of the redraw role). No must-fix. `make match FILE=src/E1BF3.ASM` reports 4 of 4 functions matching (196 bytes, unit complete).

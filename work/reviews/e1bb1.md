# Review: e1bb1

Reviewer: rev-haiku
Commit reviewed: a618630 (main when I started; src/E1BB1.ASM is identical on review/e1bb1 at b04b797)

Playbook 70. Judge what a matching file can still get wrong: names, types, comments, structure.
The matcher already proved the bytes; do not re-prove them, read for meaning.

## Findings

Each finding is a checkbox. `must-fix` blocks `make review-pass` until a decompiler ticks it off
(with the commit that fixes it); `should` and `note` do not block.

- [x] must-fix src/E1BB1.ASM:5-7: the header says the caller passes a hot-spot offset and a shape record "both taken from gNamesDDE4[n]". Only the record comes from the table. The caller passes the literal hot spot (8, 8) when n > 0 and (0, 0) otherwise, and the record gNamesDDE4[n] in both cases (src/E07A9.C:91-94). Say that (for example "hot spot (8,8) or (0,0) by whether the caller's n is above 0; record gNamesDDE4[n]"). Fixed in d87afe9: the header (now lines 5-8) says the caller passes the literal hot spot (8, 8) when its n is above 0 and (0, 0) otherwise, and the record gNamesDDE4[n] in both cases (E07A9.C:91-94).
- [x] must-fix src/E1BB1.ASM:14-17: "Both are unsigned char counts, so a stored 0 means 1" is wrong for the height. Byte +3 goes into the height unchanged (1BB1:0034, src/E1BB1.ASM:91, no +1); only the width gets the +1 (1BB1:003E, `inc dx` at src/E1BB1.ASM:94). A stored height of 0 is 0 pixels. Keep the +1 for the width only. Fixed in d87afe9: the header (now lines 15-20) says byte SHAPE_HEIGHT_BYTE (+3) is the height in pixels, taken unchanged, and byte SHAPE_WIDTH_BYTE (+2), plus 1, is the width in 8-pixel units; a stored width of 0 means 1 column, a stored height of 0 is 0 pixels.
- [x] must-fix src/E1BB1.ASM:24-26: "1BBE:0002 does nothing when ds:814A is set or the hide count ds:8146 is nonzero" is wrong for the count. With ds:8146 nonzero, 1BBE:0010 jumps to 1BBE:0075, which jumps to 1BBE:00E0, and that increments ds:8146 (a nested hide) and returns without restoring the background. It also does nothing when the count is 0 and ds:816C is 0 (1BBE:0017, 1BBE:000D). Evidence: `tools/re/disasm.py eob/EOB.EXE 1BBE` at 1BBE:0006..0017, 0075, 00E0. Correct it to: does nothing when ds:814A is set; when the count is nonzero only increments it; when the count is 0 and there is no buffer does nothing; otherwise restores the background and increments ds:8146. Fixed in d87afe9: the header (now lines 29-32) reads "does nothing when ds:814A is set; when the hide count ds:8146 is nonzero it only increments it (a nested hide); when the count is 0 and ds:816C (no buffer) is 0 it does nothing; otherwise it restores the background and increments ds:8146"; verified against `tools/re/disasm.py eob/EOB.EXE 1BBE` at 1BBE:0006..0017 (je 0010, jne 0075, je 000D), 0075 (jmp 00E0) and 00E0 (inc [8146]).
- [x] should src/E1BB1.ASM:18-20: "a margin, probably heap overhead [H]" ignores that LargestFreeBlock already takes 16 off (src/E19BC.C:142, `max -= 0x10`, and its header "less one 16 byte block"). So the caller's +16 is on top of the callee's -16: a raw free block of result + 32 is what passes. State both 16s, and drop the heap-overhead guess or keep it tagged [H] with that arithmetic. Fixed in d87afe9: the header (now lines 21-25) states the check needs LargestFreeBlock to return at least result + 16 (1BB1:0076 `add ax,16`), that the function already subtracts 16 from the raw largest free block (19BC:0142 `max -= 0x10`), so a raw free block of result + 32 passes, and that MemAlloc allocates the result (si minus the same 16). The heap-overhead guess is dropped.
- [x] should src/E1BB1.ASM:27-29: "Around the change (and every pointer redraw) the pair [ds:80AA] ... and [ds:80A2] brackets the work" is loose. In 1BB1:00A9..00BD the pair brackets only the show call 1BBE:00ED (src/E1BB1.ASM:142-149). The resize and the 1BBE:0002 hide are outside it. Say "brackets the show call at 1BB1:00B8". Fixed in d87afe9: the header (now lines 33-35) says the pair [ds:80AA] and [ds:80A2] brackets only the show call at 1BB1:00B8 (1BB1:00B1 and 1BB1:00BD); verified against `tools/re/disasm.py eob/EOB.EXE 1BB1` (lcall [80AA] at 00B1, lcall 1BBE:00ED at 00B8, lcall [80A2] at 00BD).
- [x] should src/E1BB1.ASM:157-159 (second QUIRK 1BB1:00C6): "the hide done by 1BBE:0002 (which incremented ds:8146) is never undone" holds only when ds:816C was nonzero on entry (1BB1:0013). When there is no buffer, 1BBE:0017 returns before the increment, so the first-time failure case (the first QUIRK) leaks nothing. Say "when ds:816C was nonzero when 1BBE:0002 was called". Fixed in d87afe9: the QUIRK (now lines 170-174) says the leak holds only when ds:816C was nonzero when 1BBE:0002 was called (1BB1:0013), and that when ds:816C was 0 on entry 1BBE:0002 returned before incrementing (1BBE:0017), so the first-time failure leaks no hide.
- [x] should src/E1BB1.ASM:31 and :39: the word ds:812E is "semaphore" here and "busy flag" in src/E1BF3.ASM:7, 71 and 92. Use one term in both files. "Busy flag" is the accurate one: the acquire is a test and then an increment, not an atomic operation (see the note below). The global has no entry in re/globals.toml; a `make rename` to a name in both files would settle it. Fixed in d87afe9: `make rename OLD=ds:812E NEW=gPointerBusy` names the global (now recorded in re/globals.toml and re/renames.toml), src/E1BB1.ASM uses `gPointerBusy` and says "busy flag" throughout, and src/E1BF3.ASM already says "busy flag" (it uses the raw address, so the name applies where the address is declared).
- [x] should src/E1BB1.ASM:33-34: "The unit is hand-written assembly (the compiler's C functions never push dx/es after the frame ...)" is stated as fact, but the top of the header says "kind asm [H] by the map". The c_like test (tools/re/map_units.py:42-46) looks only at the first three instructions after the frame, so "never" overstates it. Say "[H]: the map's c_like test finds no C-like frame here". Fixed in d87afe9: the header (now lines 40-44) says "[H]: the map's c_like test, tools/re/map_units.py:42-46, looks only at the instructions after the frame and finds no C-like frame here".
- [x] should src/E1BB1.ASM:91 and :93 (`es:[si+3]`, `es:[si+2]`): the record offsets are bare numbers. Rule 4 of docs/process/READABILITY.md asks for named constants or a struct for offsets into records. The header carries the meaning, so this is a clarity item: for example `SHAPE_WIDTH8_BYTE EQU 2` and `SHAPE_HEIGHT_BYTE EQU 3` in the header, with the same meaning in the comments. Fixed in d87afe9: the file defines `SHAPE_WIDTH_BYTE EQU 2` and `SHAPE_HEIGHT_BYTE EQU 3` (now lines 46-47) and the code reads `es:[si+SHAPE_HEIGHT_BYTE]` (line 104, "height in pixels, stored unchanged") and `es:[si+SHAPE_WIDTH_BYTE]` (line 106, "width in 8-pixel units"); the header (lines 15-19) uses the same names and meaning.
- [x] should src/E07A9.C:47 (not this file; the owner of e07a9 should take it): the prototype `void SetMousePointerShape(int a, int b, void far *name)` has placeholder parameter names. This unit's evidence gives them hot-spot x, hot-spot y and the shape record (src/E1BB1.ASM:42-43, 46-47 and the function comment). The name `name` is also wrong: work/reviews/e07a9.md:15 says the table holds item icon images, so the parameter is an icon or shape record. Fixed in d87afe9 (the evidence lives in this unit, so the change was made here): src/E07A9.C:47 is now `void SetMousePointerShape(int hotX, int hotY, void far *icon)`, matching the hot spot (ds:8158/815A) and the shape record (ds:816E/8170); the parameter is `icon` and not `shape` because the e07a9 review renamed the table to gIconsDDE4 (item icon images), which main merged in. make match FILE=src/E07A9.C is still 2 of 2.
- [x] should re/renames.toml:163 (the WHY of SetMousePointerShape, not in src/): "(8,8) for the sprite centre" is an inference. The code only subtracts the hot spot from the drawn position. Mark it [H], or say "(8,8) is the centre of a 16 by 16 cell if the record is 16 by 16 (e07a9 review)". Fixed in d87afe9: the WHY now reads "(8,8) for the sprite centre [H] ... The (8,8) is the centre of a 16 by 16 cell if the record is 16 wide by 16 tall, which is not proven here [H] (review of e07a9)".
- [x] note src/E1BB1.ASM:31: the acquire is not atomic: `cmp [812E],0 / jne / inc [812E]` can be split by an interrupt. If the mouse handler (1A72, whitepaper section "Mouse") runs in the gap and calls these routines, both take the flag [H]. This is how the original is; keep it, and do not "fix" it. The comment could say "busy flag, not a lock". Fixed in d87afe9: the comment now says "busy flag, not a lock" (header line 37, EXTRN comment lines 50-51); the non-atomic acquire is kept as the original has it.
- [x] note The name SetMousePointerShape has enough evidence: the function writes the hot spot (ds:8158/815A), the shape record (ds:816E/8170) and the save-under buffer, and 1BBE:00ED draws from the record (1BBE:01E8..01F4). The whitepaper says the game draws its own pointer (docs/design/WHITEPAPER.md:124). The caller in e07a9 passes item icon images (work/reviews/e07a9.md); that a carried item is shown on the pointer is [H]. "Mouse" here means the game's own pointer, which the INT 33h handler moves (docs/design/WHITEPAPER.md:124), not the driver itself. No change needed: the name stands.
- [x] note The EXTRN names `_f1BBE_0002` and `_f1BBE_00ED` (src/E1BB1.ASM:59-60) are placeholders for unit 1BBE, which is not lifted yet. Their header comments say what they do (hide, show). When 1BBE is lifted, rename them to the names it earns (for example HidePointer and ShowPointer), and E1BF3.ASM and this file follow with `make rename`. No change needed: kept as placeholders until 1BBE is lifted; the comments say hide and show. (Resolved by main: unit 1BBE is now lifted as src/E1BBE.ASM, and the `make rename` entries 1BBE:0002=HidePointer and 1BBE:00ED=ShowPointer renamed the callees; the merged src/E1BB1.ASM uses `_HidePointer` and `_ShowPointer`.)
- [x] note The globals ds:80AA, ds:80A2, ds:80C6 and the pair ds:8172/8174 stay placeholders: their EXTRN comments say what is known (the arguments, that the writer of 8172/8174 is unknown). That is the right level; nothing in 1BB1, 1BBE or E1BF3 writes 8172/8174 (checked in the full 1BBE disassembly: every use is a push). No change needed: kept as placeholders with their evidence in the EXTRN comments.

## Decompiler response (d87afe9)

All three must-fix items and every `should` item are addressed in src/E1BB1.ASM, src/E07A9.C and
re/renames.toml (commit d87afe9). The header now says the caller passes the literal hot spot (8, 8)
when n is above 0 and (0, 0) otherwise, with the record gNamesDDE4[n] in both cases; the height is
byte +3 taken unchanged and only the width is byte +2 plus 1; 1BBE:0002 does nothing when ds:814A is
set, only increments the count when it is nonzero, and does nothing when the count is 0 and ds:816C
is 0. The LargestFreeBlock check is stated with both 16s (result + 16 compared against a raw block
minus 16, so result + 32 passes); the [ds:80AA]/[ds:80A2] pair is said to bracket only the show call
at 1BB1:00B8; the second QUIRK holds only when ds:816C was nonzero when 1BBE:0002 was called. The
word ds:812E is named gPointerBusy (`make rename`) and called a busy flag, the assembly kind carries
its [H] and the c_like caveat, and the record offsets are the named constants SHAPE_WIDTH_BYTE (2)
and SHAPE_HEIGHT_BYTE (3). The e07a9 prototype is now (hotX, hotY, icon) and the SetMousePointerShape
WHY marks the sprite-centre reading [H]. Verified: `make match FILE=src/E1BB1.ASM` is 1 of 1 (208 of
208 bytes), `make match FILE=src/E07A9.C` is 2 of 2 (487 of 487), `make match FILE=src/E1BF3.ASM` is
4 of 4 (196 of 196) and `make readability UNIT=e1bb1` is 0 findings.

## Summary

The unit is understandable from its source alone. It holds one function, SetMousePointerShape (1BB1:0002, 207 bytes), with the start marker byte at 1BB1:0001. The function hides the mouse pointer through 1BBE:0002, sets the hot spot and the shape record, resizes the save-under buffer when the sprite size changed (freeing the old buffer, asking [ds:80C6] for the size, then MemAlloc), and shows the pointer again through 1BBE:00ED. The name and the overall structure check out against the bytes, the callers (src/E07A9.C:91-94, src/EOV2.C), the 1BBE disassembly, the E19BC allocator signatures and the E1BF3 readers. Three claims are wrong and must be fixed: who passes the hot spot (the header says it comes from the table), the height rule (only the width gets +1), and what 1BBE:0002 does when the hide count is nonzero (it increments it). The rest are wording: the margin arithmetic, the scope of the 80AA/80A2 pair, the QUIRK condition, the semaphore and busy flag terms, the [H] tag on the assembly kind, and the record offsets. Placeholders left on purpose: g812E (no evidence beyond the flag use), g80AA, g80A2, g80C6, g8172, g8174 (their behaviour is not known individually), and _f1BBE_0002 and _f1BBE_00ED (unit 1BBE not lifted yet).

## Second review (rev-haiku, at fe31532)

Commit reviewed: fe31532 (main). src/E1BB1.ASM last changed at 3396a84, which kept the d87afe9 fixes. The first review's findings are all fixed and ticked above; I rechecked them against the bytes.

Checks (tools/re/disasm.py on eob/EOB.EXE, read only): 1BB1:0034/003A/003E (height from byte +3 unchanged, width byte +2 plus 1); 1BB1:0013/00B1/00B8/00BD (call order); 1BB1:0050..005C (resize path, MemFree on ds:816A/816C); 1BB1:0076, 0080..0086, 0091 (margin and LargestFreeBlock test); 1BB1:00A0..00A5 (store only when MemAlloc returns a nonzero segment); 1BBE:0006..0017, 0075, 00E0 (hide: nested count, no-buffer return); 1BBE:008D and 00AB..00B3 (y clip to 200); 1BBE:0051..005A (x clip to 40); 1BBE:00B9..00D9 (restore through [804E]); 1BBE:01B9..01D9 (save through [80AE]); 1BBE:0111..012E (drawn position copied from the mouse position at show time). The MemAlloc(unsigned long, int) and MemFree(void far *) push orders match src/E19BC.C:34 and :82; LargestFreeBlock returns unsigned long and subtracts 0x10 (E19BC.C:131, :142). `make match FILE=src/E1BB1.ASM` 1 of 1; `make readability UNIT=e1bb1` 0 findings.

### Findings

- [x] must-fix src/E1BB1.ASM:32-34 and :59-60: "so the writer is unknown" and "(writer unknown)" are wrong. ds:8172/8174 are written by 1A72:02FE (`_SetFarPointer`, src/E1A72.ASM:420-460: `mov [8172],ax` at 1A72:031C and `mov [8174],bx` at 1A72:031F) and read by 1A72:033A (`_GetFarPointer`, src/E1A72.ASM:462-473). This unit and E1BF3 only read them (1BB1:00A9..00AD). Name the writer; the statement that this unit and E1BF3 do not write them holds. Fixed in b9bf160: the header (now lines 35-38) says the writer is _SetFarPointer (1A72:02FE, src/E1A72.ASM:442-443) and the reader is _GetFarPointer (1A72:033A), and the two EXTRN comments (now lines 66-69) say "written by _SetFarPointer 1A72:02FE ... read by _GetFarPointer 1A72:033A"; the claim that this unit and E1BF3 only read them stays. Verified at 1A72:031C/031F and 1A72:0344/0347 with tools/re/disasm.py.
- [x] must-fix src/E1BB1.ASM:5-6 and re/renames.toml:176-181 (WHY of SetMousePointerShape): the caller citation "E07A9.C:92-94" points at the purpose comment of f07A9_010C, not at the calls. The calls are at src/E07A9.C:119-122 (`SetMousePointerShape(8, 8, ...)` and `(0, 0, ...)`). The renames WHY also calls the caller "Level07A9"; its current name is f07A9_010C (re/renames.toml:428). Correct both. Fixed in b9bf160: the header (now lines 5-7) says the caller passes the hot spot (8,8) when its n is above 0 and (0,0) otherwise and the record gIconsDDE4[n] in both cases, citing the two calls at E07A9.C:127-130 (the review's 119-122 is stale; the current calls are at src/E07A9.C:127-130). re/renames.toml:181 now names the caller f07A9_010C (07A9:010C) and cites E07A9.C:127-130, and the same stale citation in re/renames.toml:208/:217 and re/globals.toml:67/:72 is corrected too.
- [x] should src/E1BB1.ASM:7: "nothing here says "level" or "name"" is review history, not a fact about the code. Drop it and cite the evidence that the table holds icon images: src/E07A9.C:40-42 (ovl2:03D5..0413 fills entries 1..0x58 from the ITEMICN sheet). Fixed in b9bf160: the header (now lines 5-7) drops that sentence and cites the item icon images at E07A9.C:41-43 (the gIconsDDE4 comment; it moved to 41-43 in the current file).
- [x] should src/E1BB1.ASM:21-24: the check is skipped when the high word of LargestFreeBlock is nonzero (1BB1:0080..0082, `or dx,dx; jne alloc`), so a free block of 64 KB or more passes without the comparison. The header says the check "needs LargestFreeBlock to return at least that result plus 16"; add the exception. Fixed in b9bf160: the header (now lines 21-27) says the comparison is skipped when the block's high word DX is nonzero (1BB1:0080 `or dx,dx`; 1BB1:0082 `jne`), so a free block of 64 KB or more passes without the test.
- [x] should src/E1BB1.ASM:169-173 (QUIRK 1BB1:00C6, second paragraph): "holds only when ds:816C was nonzero when 1BBE:0002 was called" is true in practice but gives the wrong reason. The nested path (1BBE:0015 `jne 0075`, then 00E0 `inc [8146]`) increments the count without looking at ds:816C. The count can only be nonzero after a hide that saw a buffer, because ds:816C is stored once (1BB1:00A5, only with a nonzero MemAlloc segment) and never cleared (a byte scan of eob/EOB.EXE finds one store to 816C). Say both ways the hide can be counted: the count was already nonzero (nested), or ds:816C was nonzero on entry (1BBE:0017). Fixed in b9bf160: the QUIRK (now lines 178-186) gives both ways the count can be nonzero (it was already nonzero, so 1BBE:0015 `jne 0075` reaches 1BBE:00E0 `inc [8146]` with no buffer test; or ds:816C was nonzero on entry, so 1BBE:0017 passes the buffer test and increments), adds that ds:816C is stored once at 1BB1:00A5 and never cleared, and keeps that when ds:816C is 0 and the count is 0 the hide is not counted (1BBE:0017).
- [x] should src/E1BB1.ASM:130 and :144: the 16 in `add ax,16` (1BB1:0076) and `sub ax,16` (1BB1:0091) is the heap block that LargestFreeBlock takes off (src/E19BC.C:5, "less one 16 byte block"; :142). Rule 4 of docs/process/READABILITY.md: name it (for example `HEAP_BLOCK EQU 16` beside the record offsets) and keep the citation in the comment. Fixed in b9bf160: `HEAP_BLOCK EQU 16` is defined beside the record offsets (now lines 51-53) with the E19BC.C:142 citation, and the code reads `add ax,HEAP_BLOCK` (line 139) and `sub ax,HEAP_BLOCK` (line 153), each with a short comment; make match still 1 of 1 (208 bytes), so the immediate is unchanged.
- [x] should src/E1BB1.ASM:14-36: the header uses raw addresses where the global already has a name: ds:8146 is gHideDepth (re/globals.toml:180) and ds:812E is gPointerBusy (the EXTRN at line 49). Use the names (rule 7). ds:814A, ds:8138/813A and ds:8176 have no names and stay raw. Fixed in b9bf160: the header now writes gHideDepth (ds:8146) at lines 32 and 34 and gPointerBusy (ds:812E) at line 40, each keeping its address; ds:814A and the other unnamed globals stay raw.
- [x] should src/E1BB1.ASM:16-17: "the same unit as the region records of 1A29" has no citation. src/E1A29.C:19 and :21 say the region's left edge and width are in 8-pixel columns; cite them. Fixed in b9bf160: the header (now line 18) cites E1A29.C:19-22, the Region struct whose left edge and width are in 8-pixel columns.
- [ ] note src/E1BBE.ASM:75-76 (unit 1BBE, for its own review, not this one): "Nothing happens when ... already hidden (gHideDepth non-zero)" is wrong. 1BBE:0015 `jne 0075` goes to 00E0 `inc [8146]`, so a nested hide increments the count. This unit's header has it right.
- [ ] note src/E1A72.ASM:7 and :32 (unit 1A72, for its owner): ds:812E is "semaphore" there and "busy flag" in src/E1BB1.ASM and src/E1BF3.ASM. The first review asked for one term in both files; E1A72 was not covered.
- [ ] note src/E07A9.C:57-58 (unit e07a9, for its owner): "stores the hot-spot offset" is loose: 1BB1:0002 stores two words, the hot spot x and y (ds:8158/815A).

### Summary (second review)

The unit is understandable from its source. The byte-level claims in the header hold against the 1BB1 and 1BBE disassembly and the E19BC allocator, and the first review's fixes are in place. Two claims must be fixed before a pass: the writer of ds:8172/8174 (1A72:02FE, not unknown) and the caller citation and name (in the header and in re/renames.toml). The shoulds are precision: the high-word case of the margin check, the reason for QUIRK 2, the magic 16, raw addresses where a name exists, and one citation. Placeholders g80AA, g80A2 and g80C6 stay: their evidence is in the EXTRN comments. Verdict: review-failed.

## Decompiler response (second review, b9bf160)

Both must-fix items and all six `should` items are addressed in commit b9bf160 (src/E1BB1.ASM,
re/renames.toml, re/globals.toml); each is ticked above with what changed. The header now names
_SetFarPointer (1A72:02FE) as the writer and _GetFarPointer (1A72:033A) as the reader of
ds:8172/8174, and the two EXTRN comments name them; the caller is f07A9_010C (07A9:010C), cited at
E07A9.C:127-130 (the review's 119-122 is stale, the calls are at 127-130 in the current file), and
re/renames.toml:181 is corrected too, as is the same stale citation in re/renames.toml:208/:217 and
re/globals.toml:67/:72. The header drops the review-history sentence and cites the
item icon images (E07A9.C:41-43); it states the margin comparison is skipped when the block's high
word DX is nonzero (1BB1:0080/0082); the second QUIRK gives both ways the hide count can be nonzero
(nested, or ds:816C nonzero on entry) and that ds:816C is stored once (1BB1:00A5) and never cleared;
`HEAP_BLOCK EQU 16` names the heap block at 1BB1:0076/0091; the header uses gHideDepth and
gPointerBusy; and the 8-pixel column claim cites E1A29.C:19-22. Verified: `make match
FILE=src/E1BB1.ASM` 1 of 1 (208 of 208 bytes), `make readability UNIT=e1bb1` 0 findings. Rechecked
against tools/re/disasm.py: 1A72:031C/031F (writes), 1A72:0344/0347 (reads), 1BBE:0015/0075/00E0
(nested increment), 1BBE:0017 (buffer test), 1BB1:00A5 (the single store to 816C), 1BB1:0076/0080/0082/0091.

The three `note` items are for other units, not this one: src/E1BBE.ASM:75-76 is unit 1BBE's own
file, src/E1A72.ASM:7/:32 is unit 1A72's, and src/E07A9.C:57-58 is unit e07a9's. This unit's branch
does not change other units' files, so they stay open for their owners.

## Third review (rev-haiku, at 572030b)

Commit reviewed: 572030b (main when I started; review/e1bb1 is at main and src/E1BB1.ASM is the b9bf160 version). The
two earlier rounds are ticked above. I rechecked them against the bytes with tools/re/disasm.py on eob/EOB.EXE (read
only): 1BB1:0008, 000F and 00C8 (the busy flag); 1BB1:0034, 003A and 003E (the shape reads); 1BB1:0044..0050 (the buffer
and size tests); 1BB1:0054..005C (the free); 1BB1:0069..006F (the stores and the size query); 1BB1:0076..0095 (the margin
test and MemAlloc's push order); 1BB1:00A9..00BD (the 80AA, ShowPointer and 80A2 order). The E19BC.C flags comment
(lines 29-32), the E07A9.C calls (127-130), the E1A29.C citation (19-22) and the E1A72.ASM writes (442-443) all match.
`make match FILE=src/E1BB1.ASM` is 1 of 1 and `make readability UNIT=e1bb1` is 0 findings.

### Findings

- [ ] must-fix src/E1BB1.ASM header, the paragraph "If the size changed, the old buffer is freed and the size query
  [ds:80C6] is called", and the function comment "resizing the save-under buffer when the sprite size changed": the
  size query and MemAlloc do not run only on a size change. 1BB1:0044 `je 0066` skips the free and goes to the query
  whenever ds:816C is 0, so the first call always queries and allocates (no free). With a buffer, only a size change
  frees it (1BB1:0050 `je 00A9` goes to ready when the size is the same, 1BB1:0058..005C frees otherwise). Say: "with a
  buffer and a changed size, the old buffer is freed (1BB1:0058..005C); the size query and MemAlloc run when there is
  no buffer yet (1BB1:0044) or the size changed; with a buffer and the same size the call goes straight to the show
  (1BB1:0050)." Change the function comment to match.
- [ ] should src/E1BB1.ASM header, "The size comes from the shape record": the store addresses are off. 1BB1:0034 only
  reads byte +3, and the store into ds:8154 is 1BB1:0066 (`mov [8154],ax`). The width store into ds:8156 is 1BB1:006A
  (`mov [8156],dx`); 1BB1:003E is the `inc dx`. Cite the stores at 0066 and 006A, and keep 0034, 003A and 003E for
  the reads and the +1.
- [ ] should src/E1BB1.ASM:88..94 (the MemAlloc call) and its EXTRN line: the second argument is flags 0 (`mov ax,0`
  and `push ax` at 1BB1:0088..008B), and the unit never says so. E19BC.C:29-32 gives the bits: 0x10 zeroes the block,
  0x20 aligns it to a paragraph; 0 is neither. Say so in the call's comment.
- [ ] should src/E1BB1.ASM, the HEAP_BLOCK comment and the header's margin paragraph: the quote "less one 16 byte
  block" is E19BC.C:5 (the unit header), and the subtraction is E19BC.C:142 (`max -= 0x10`). Cite both, not only :142
  with the quote.
- [ ] should src/E1BB1.ASM header, the ASSUME paragraph (the last lines of the header): the header says `ASSUME CS:NOTHING`
  around each direct far call is what keeps TASM from turning `lcall` into `push cs; call`. The playbook gives another
  mechanism for that: `call far ptr` with the callee declared as EXTRN is the 9A form (docs/process/playbooks/30-lift-module.md:29).
  Either show the ASSUME lines are needed (for example, remove them in a scratch copy and see whether `make match` still
  reports 1 of 1 with the same bytes), or tag the claim [H].
- [ ] note packet: `make packet UNIT=e1bb1` lists ovl2:0000 Ovl2_0000 (EOV2.C) as a caller of 1BB1:0002. EOV2.C:14 defines
  Ovl2_0000 as an empty stub, and no build/re/eob-overlays/*.bin contains the bytes 9A 02 00 B1 1B. That search does not
  settle it (an overlay may hold the segment unrelocated), so the header keeps the caller it cites, f07A9_010C (E07A9.C:128
  and :130). For the map owner: check whether the packet's caller is a real call.
- [ ] note the return value: the function returns 1 in AX on success (1BB1:00C1) and 0 on failure (1BB1:00C6). The
  prototype at src/E07A9.C:63 is void and both callers (E07A9.C:128, :130) ignore the value. The header says "returns 1,
  or 0 when no block can be allocated", but 0 is also returned when the largest-block test fails (1BB1:0084..0086, before
  MemAlloc). Say "0 when the block is too small or MemAlloc fails". No change is needed in src/E07A9.C.
- [ ] note src/E07A9.C:127-130 (the caller, for the e07a9 owner): the else branch passes gIconsDDE4[0] with (0, 0). E07A9.C:43
  says index 0 is "none". If that entry is a null far pointer, 1BB1:0034 reads the shape at 0000:0003. Whether n can be
  0 there (a zero icon number reaches the call through gTable8DD2[n].icon) is not checked here.
- [ ] note src/E1BBE.ASM:65 (for the 1BBE owner): "the sprite size query (1BB1:0094)" cites the wrong address. The call to
  [ds:80C6] is at 1BB1:006F (`lcall [0x80c6]`); 1BB1:0094 is the `sub ax,16` before MemAlloc.
- [ ] note the open notes from the earlier rounds still stand for their owners: src/E1BBE.ASM:72..76 ("already hidden"
  is wrong, a nested hide increments the count, 1BBE:0015); src/E1A72.ASM:7 and :32 and src/E1BBE.ASM:28, :273 and :353
  call ds:812E "semaphore" where this unit and E1BF3 say "busy flag" (the global is gPointerBusy).

### Summary (third review)

The unit is understandable from its source alone. The names have evidence in the header and the registry: SetMousePointerShape
(one caller, f07A9_010C, E07A9.C:127-130), the shape and buffer globals (the 1BB1 and 1BBE reads and writes), gPointerBusy
(test and increment, 1BB1:0008/000F and release 00C8), gHideDepth, HidePointer and ShowPointer (1BBE:0002 and 00ED), and
HEAP_BLOCK. The placeholders are g80AA, g80A2 and g80C6 (the callees behind them are not lifted; their evidence is the
call sites and the comments in E1BBE.ASM and E1BF3.ASM), and g8172/g8174 (the writer and reader are named, the argument's
meaning is not). One must-fix remains: the header says the size query and reallocation happen only on a size change, but
the first call with no buffer does them too. Verdict: review-failed.

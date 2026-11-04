# Review: e19bc

# Round P6 (2026-11-04), commit ce0608f

Reviewer: p6-rev-1
Commit reviewed: ce0608f (main; review/e19bc was ce0608f when this round began and no fast-forward was needed). The last review round was Round 3 at e1ffddc. Since e1ffddc, src/E19BC.C changed by the Phase 6 rename: `f1BA8_0072` is `NormalizePtr`, `f1DDD_002D` is `SetVideoMode`, `g84A2` is `gBiosVideoModes` in the OutOfMemory comment, and `MEMALLOC_ALIGNED` moved to `EOB.H` (`#define MEMALLOC_ALIGNED 0x20` became `#include "EOB.H"`). The diff is 11 insertions and 11 deletions. The names do not change the machine code; this round judges only whether each new name and comment is true, specific and consistent.

Later round (playbook 70). Judged: the diff since e1ffddc for src/E19BC.C (the Phase 6 rename and its comments) and the names and comments it introduces. The Round 3 must-fix and should items are all ticked and their byte claims are unchanged, so they are not reopened here.

Checked: `make match FILE=src/E19BC.C` 5 of 5 (728 bytes), exit 0; `make readability UNIT=e19bc` 0 findings. Evidence used this round: `git diff e1ffddc HEAD -- src/E19BC.C` (the rename and its comments); `grep -n -A7 "addr = \"1BA8:0072\"" re/renames.toml` (re/renames.toml:7602, the NormalizePtr why); `grep -n -A7 "addr = \"ds:84A2\"" re/renames.toml` (re/renames.toml:7728, the gBiosVideoModes why); `grep -n "MEMALLOC_ALIGNED" src/E07A9.C src/E1B82.C src/EOB.H` (the define is at src/EOB.H:360; the uses at src/E07A9.C:322 and src/E1B82.C:158, :167); `grep -n "0x10L" src/E07C8.C` (the size is at :376); `sed -n '84p' src/E082C.C` (the LargestFreeBlock declaration); `sed -n '99p' src/E082C.C` (the WaitGameKey declaration); `sed -n '124p;132p' src/E07C8.C` (the lines the OutOfMemory comment cites).

## Findings

Each finding is a checkbox. `must-fix` blocks `make review-pass` until a decompiler ticks it off
(with the commit that fixes it); `should` and `note` do not block.

- [ ] must-fix src/E19BC.C:32-34 (the NormalizePtr declaration), 1BA8:0072: "the name stays a placeholder" is false. The P6 rename replaced the placeholder f1BA8_0072 with NormalizePtr (re/renames.toml:7602, old = f1BA8_0072, new = NormalizePtr), and the why backs the name: "Loads its far pointer argument into DX:AX and calls NormFar (1BA8:0081), which returns offset & 0Fh and segment + offset >> 4 (src/E1BA8.ASM:80-104); callers normalise buffers before large reads and writes (src/E1C0E.C:320, 363, 810; MemAlloc src/E19BC.C:94; src/EOV3.C:780)". Drop "the name stays a placeholder"; the "normalizes a far pointer to DX:AX (its body calls _NormFar, src/E1BA8.ASM:81-90)" part stays.
- [ ] should src/E19BC.C:44-49 (the MEMALLOC_ALIGNED comment): the define moved to the shared header. The P6 diff replaced `#define MEMALLOC_ALIGNED 0x20` with `#include "EOB.H"`, and the define is now at src/EOB.H:360. The comment still says "src/E07A9.C:37 defines it and uses it at src/E07A9.C:276; src/E1B82.C:43 defines it and uses it at src/E1B82.C:165 and :174." The current uses are src/E07A9.C:322 and src/E1B82.C:158 and :167. Say that EOB.H defines it and the units use it.
- [ ] should src/E19BC.C:46-49 (the MEMALLOC_ZERO_FILL comment): "the 0x10L at src/E07C8.C:259 is a size" is stale. The 0x10L is at src/E07C8.C:376 (`gPaletteBuf = MemAlloc(0x10L, 0)`); :259 is `else` in Main. Update the cite.
- [ ] should src/E19BC.C:210-216 (the OutOfMemory comment): "src/E07C8.C:132" for the allocation failure hook and "src/E07C8.C:124" for argv[5] are stale. The hook install is src/E07C8.C:236 (`gAllocFailHook = AllocationFailure;`) and argv[5] is read at src/E07C8.C:226 (`gRestoreMode = atol(argv[5]);`). Update the cites.
- [ ] should src/E19BC.C:183-186 (the LargestFreeBlock comment): "src/E082C.C:86 declare" is stale; the declaration is at src/E082C.C:84. The type claim (long) holds. Update the cite.
- [ ] note src/E19BC.C:17-22 (the gAllocCount comment) cites src/E07A9.C:148 (declaration) and :286 (clear). The current E07A9.C lines are :194 (the declaration) and :332 (the clear). The P6 diff did not touch this comment, so it is a note, not a must-fix; the next edit of either file should update it.
- [ ] note the P6 names SetVideoMode (1DDD:002D) and gBiosVideoModes (ds:84A2) are backed by re/renames.toml (SetVideoMode: INT 10h AH=00h through the seven-word table; gBiosVideoModes: the seven words 4, 9, 0Dh, 13h, 13h, 7, 2), and the comments that use them now agree. The WaitGameKey comment's src/E082C.C:99 is the current declaration. The NormalizePtr name is backed; only the "placeholder" sentence is left.

## Summary

One must-fix item is open. The NormalizePtr comment (src/E19BC.C:32-34) still says "the name stays a placeholder" after the P6 rename replaced f1BA8_0072 with NormalizePtr, a name the rename backs with the DX:AX normalisation and the NormFar call. The should items are stale cites: the MEMALLOC_ALIGNED comment still names the old per-unit defines after the move to EOB.H, and the MEMALLOC_ZERO_FILL, OutOfMemory and LargestFreeBlock comments cite lines that moved. The matcher is 5 of 5 and readability is 0 findings; the names themselves are backed and the machine code is unchanged. The unit does not pass (review-failed) until the NormalizePtr comment is rewritten.

Reviewer: rev-haiku
Commit reviewed: b628deba92bbe741f650e67e40dd0b37d40146f0 (main, when I started)

Playbook 70. Judge what a matching file can still get wrong: names, types, comments, structure.
The matcher already proved the bytes; do not re-prove them, read for meaning.

Checks run: `make match FILE=src/E19BC.C` 5 of 5 (728 bytes); `make readability UNIT=e19bc` 0 findings.
Evidence read: src/E19BC.C, build/packets/e19bc.md, src/E1C00.ASM:140-290, src/E07C8.C:35,56,69,104,124,127-132,359-363,
src/E07A9.C:29-35,108,227-242, src/E1B82.C:43,165,174, src/E1C0E.C:31-46,628-652, src/E1DDD.C:1-51,
src/E1AAD.ASM:440-486, src/E1BA8.ASM:81-90, src/E1BB1.ASM:164-175, src/E082C.C:65,78,133, src/E1DDD.C,
re/globals.toml:10-12, re/renames.toml:50-73, build/toolchain/tc100/INCLUDE/ALLOC.H:12-13.

## Findings

Each finding is a checkbox. `must-fix` blocks `make review-pass` until a decompiler ticks it off
(with the commit that fixes it); `should` and `note` do not block.

- [x] must-fix src/E19BC.C:129-130 (MemRealloc comment): "the caller tests it (src/E1C00.ASM:152-158)" is wrong. The caller's `or ax,dx; je short fail` does not catch a failed realloc of the 0x20 block: MemRealloc's 0x20 path returns MK_FP(FP_SEG(blk)+1, 0) (E19BC.C:142-145), so a NULL realloc comes back as 0001:0000 and the caller calls 0001:AX (src/E1C00.ASM:221-225). The cite 152-158 is the segment-register comment, not the test. Fix: say that a failed realloc is not reported here or at the caller, mark it `QUIRK 19BC:018A:` with that behaviour (the 0x20 path also writes `(blk - 1)[hdrLow]` through the NULL result before returning, which lands near 0000:000F after the offset wraps [H], to be confirmed in the oracle), and cite src/E1C00.ASM:221-225. The non-aligned path (E19BC.C:148, `*blk = flagBits`) dereferences the NULL at once; no caller in src/ uses it (the packet lists MemRealloc's only caller as 1C00:0055), so say so. Fix (60990d2): rewrote the comment's tail: it now marks `QUIRK 19BC:018A`, says the result is untested here and at its only caller 1C00:0055 (cite src/E1C00.ASM:221-225), gives the 0001:0000 return and the 0000:000F-wrapped header store [H], and notes the non-aligned path dereferences the NULL with no caller in src/.
- [x] must-fix src/E19BC.C:43 (MEMALLOC_ALIGNED comment): "src/E07A9.C:29 already names this bit and uses it at src/E07A9.C:208" is wrong. The define is at src/E07A9.C:35 and the use at src/E07A9.C:232 (`gBuf8DCA = MemAlloc(7500L, MEMALLOC_ALIGNED)`). Also src/E1B82.C:43 defines it and uses it at :165 and :174. Fix the cites. Fix (60990d2): the comment now cites src/E07A9.C:35 and :232, and src/E1B82.C:43, :165 and :174.
- [x] must-fix src/E19BC.C:20 (g810E comment): "it is g810E in src/E07A9.C:96" is wrong; line 96 is a comment that does not name the global. The declaration is src/E07A9.C:108 and the clear is at :242 (`g810E = 0;` in StartupAlloc, 07A9:00E6). Fix the cite. Fix (60990d2): the comment now cites src/E07A9.C:108 (declaration) and :242 (the clear).
- [x] should src/E19BC.C:179 (OutOfMemory comment): "which MemAlloc lcalls when farmalloc returns NULL" is incomplete. MemAlloc calls the hook only after farmalloc fails and `EvictFile(0, 0xff)` returns 0 (E19BC.C:68-74, 73). Say that. Fix (60990d2): the comment now says MemAlloc lcalls the hook only after farmalloc fails and the EvictFile retry returns 0 (E19BC.C:68-74, the test at :73).
- [x] should src/E19BC.C:17-22 (g810E): the evidence supports naming what the global holds: MemAlloc adds one per success (E19BC.C:97), MemFree subtracts one per call (:120), and StartupAlloc clears it (src/E07A9.C:242). Purpose is unknown (the E07A9.C:96-97 comment says so), but "net allocation counter" is what it holds. Consider `make rename OLD=g810E NEW=gAllocCount WHY="..."` so the placeholder sentence can go; if the placeholder stays, say which evidence is missing. Fix (60990d2): renamed ds:810E to `gAllocCount` (`make rename`, logged in re/renames.toml and re/globals.toml); the declaration comment now says what it holds and that what reads the count is unknown. E07A9.C still says g810E until this branch merges.
- [x] should src/E19BC.C:39 (`extern void WaitGameKey(void)`): the routine returns the key in AX (1AAD:053A ends in `call _ProcessKeyEvent; retf`, src/E1AAD.ASM:444-483), and src/E1C0E.C:46 and src/E082C.C:78 declare `int` and use the value (src/E1C0E.C:566, src/E082C.C:133). src/E07C8.C:56 and src/EOV2.C:185 say void. Declare `int` so the type fits the value, or say why void is used here. Fix (60990d2): declared `int`; the annotation cites src/E1AAD.ASM:481-483 and the two int usages. `make match` still 5 of 5.
- [x] should src/E19BC.C:35-36 and :69 (EvictFile): the `0xff` in `EvictFile(0, 0xff)` is a magic number that reaches FindEvictableFile as its flags (src/E1C0E.C:637, 631). Name it or explain what it selects. Also src/E19BC.C:90-95: the 0xFA00 zero-fill chunk is a magic number; name it (a chunk size) and say why the chunks exist, or say the reason is unknown. Fix (60990d2): added `EVICT_ANY 0xff` (FindEvictableFile keeps a slot whose flags are `<= maxFlags`, src/E1C0E.C:617, so any slot qualifies) and `MEMALLOC_FILL_CHUNK 0xFA00L` (memset's size_t count is 16-bit, MEM.H:32 and :53, so a block over 64K must be split; the exact value is not evidenced); the call and the zero-fill loop use them.
- [x] should src/E19BC.C:56-57 (MemAlloc comment): "The two stage retry is the nested loop the original compiles to." does not say what the stages are. Rewrite as: first the EvictFile retry, then the hook. Fix (60990d2): the comment now says the inner loop retries farmalloc while `EvictFile(0, EVICT_ANY)` succeeds and the outer loop runs the failure hook when the inner retry gives up.
- [x] should src/E19BC.C:156 (LargestFreeBlock comment): "_HEAPOK, which is 2" is correct (build/toolchain/tc100/INCLUDE/ALLOC.H:13), but it cites only "ALLOC.H". Cite that header line. Fix (60990d2): now cites build/toolchain/tc100/INCLUDE/ALLOC.H:13.
- [x] should src/E19BC.C:158 (LargestFreeBlock comment): "src/E1C0E.C:32" is stale; the `extern long LargestFreeBlock(void)` declaration is src/E1C0E.C:34 (line 32 is MemAlloc). src/E07C8.C:69 is correct. Fix (60990d2): now cites src/E1C0E.C:34.
- [x] should src/E19BC.C:178-180 (OutOfMemory comment): the claim that the image stores 19BC:02B6 at ds:810A has evidence in re/globals.toml:12 (the `lcall [0x810a]` at 19BC:0063 and the pointer stored at DGROUP 810A), but the comment cites none. Cite it. Fix (60990d2): the comment now cites re/globals.toml:12.
- [ ] note src/E1BB1.ASM:168 (other owner): its comment says "the 16 that LargestFreeBlock takes off (E19BC.C:142)". LargestFreeBlock's subtraction is at src/E19BC.C:172; 142 is MemRealloc. Leave for the e1bb1 owner.
- [ ] note f1DDD_002D's parameter is `mode` here and in src/E07C8.C:58, but `i` in src/E1DDD.C:43. The name is fine (not a single letter in this file's use per R4); the E1DDD owner may want the same name.
- [ ] note MemAlloc's `int flags` is a bit mask (0x10, 0x20 and the header's low nibble), so `unsigned` would fit the value better; no caller passes a sign-dependent value, so this is a should at most, and I have not made it one.
- [ ] note LargestFreeBlock's type is `long` here and in src/E07C8.C:69 and src/E1C0E.C:34, but `unsigned long` in src/E082C.C:65. The comment already records this; the e082c owner must change it.

## Summary

The unit is understandable from its source alone, apart from the MemRealloc failure behaviour, which the comment
gets wrong (must-fix 1). The names MemAlloc, MemFree, MemRealloc, LargestFreeBlock and OutOfMemory each have
evidence in the comments (callers, the message string, the header flags, and the ALLOC.H calls), and the names
src/ already uses for the shared globals (gAllocFailHook at ds:810A, g810E at ds:810E) and callees (EvictFile,
f1DDD_002D, f1BA8_0072, PrintString, WaitGameKey, ShutdownGame) agree across src/. Three cites point at the
wrong lines and must be fixed. The remaining placeholders are f1BA8_0072 (its comment says it normalizes a far
pointer; the name stays a placeholder, which is fine) and g810E (the counter's purpose is unknown, so the
placeholder is defensible, though a should suggests a more specific name). Review result: review-failed,
3 must-fix items.

## Response (dec-deepseek, commit 60990d2)

All three must-fix findings and all seven shoulds are fixed; `make match FILE=src/E19BC.C` is still 5 of 5
(728 bytes) and `make readability UNIT=e19bc` is 0 findings. The three notes are left for the owners named in
them. ds:810E is now `gAllocCount` (the review's should), so the placeholder sentence is gone. The MemRealloc
comment marks `QUIRK 19BC:018A` and no longer claims the caller tests the realloc result.

## Round 2 (rev-haiku, main 819ba10)

Reviewer: rev-haiku. Commit reviewed: 819ba10 (main; src/E19BC.C is identical on review/e19bc and main and has the round 1 fixes of 60990d2). Round 1 is kept above and its items are ticked.
Checks run: `make match FILE=src/E19BC.C` 5 of 5 (728 bytes); `make readability UNIT=e19bc` 0 findings.
Checked against the original: `tools/re/disasm.py eob/EOB.EXE 19BC` (0007 to 02E0) and `tools/re/disasm.py eob/EOB.EXE 07A9` (00E0 to 00F8), and the image bytes at ds:810A (B6 02 BC 19, that is 19BC:02B6). Verified: 19BC:0129 (inc of ds:810E), 19BC:0182 (the dec, reached by the NULL jump at 0143), 19BC:0031 to 0037 (EvictFile(0, 0xff)), 19BC:0063 (lcall [810A]), the runtime targets, the `add sp,n` sites in the header, 07A9:00E6 (mov [810E],0), and the callers' flags (src/ passes only 0 and 0x20, so MEMALLOC_ZERO_FILL has no caller, as the comment says). ShutdownGame ends in exit(0) (src/E07C8.C:345).

## Findings

- [x] must-fix src/E19BC.C:143-146 (MemRealloc QUIRK): the caller cite `src/E1C00.ASM:221-225` is wrong; those lines are the size and CX setup. The caller's `call far ptr _MemRealloc` is src/E1C00.ASM:237, its `or ax,dx` / `je short fail` is :244-245, and the `call dword ptr ds:[818Eh]` that calls 0001:AX is :263 (the comment for it is :239-243). Cite those. Fixed: the caller cite now names 1C00:0055, `call far ptr _MemRealloc` at src/E1C00.ASM:243, the `or ax,dx; je short fail` at :250-251, and the `call dword ptr ds:[818Eh]` at :269 (comment :245-249); those lines moved after the round 16 E1C00 fixes, so the current lines are cited.
- [x] must-fix src/E19BC.C:146-148 (MemRealloc QUIRK): "lands near 0000:000F after the offset wraps" gives the wrong mechanism. 19BC:01F2 loads bx = blk offset 0, 19BC:01F5 adds the pad (`add bx,ax`, ax = 0x10), so bx = 0x10, and 19BC:01FF stores to `es:[bx-1]`; the -1 is a displacement. The store lands at exactly 0000:000F, nothing wraps, and the byte written is 0x0F | flagBits. Say that; keep [H] only for the oracle check. Fixed: the comment now gives 19BC:01F2 (BX = block offset 0), 19BC:01F5 (`add bx,ax`, hdrLow = 0x10), and 19BC:01FF (`mov es:[bx-1],al`, a displacement), says the byte lands at exactly 0000:000F with value 0x0F | flagBits and nothing wraps, and keeps [H] only for the oracle. The disassembly was rechecked (tools/re/disasm.py 19BC:018A to 020C).
- [x] must-fix src/E19BC.C:17-21 (gAllocCount): `src/E07A9.C:108` (declaration) and `:242` (clear) are stale. On main they are src/E07A9.C:131 and :266. The 07A9:00E6 address is right (verified above). Fixed: the comment now cites src/E07A9.C:148 (declaration) and :286 (the clear). Those moved again after the e07a9 round 9 fixes landed on main, so the current lines are cited.
- [x] must-fix src/E19BC.C:44-46 (MEMALLOC_ALIGNED): `src/E07A9.C:35` and `:232` are stale. The define is src/E07A9.C:36 and the use is src/E07A9.C:256 (`gBuf8DCA = MemAlloc(7500L, MEMALLOC_ALIGNED)`). The src/E1B82.C:43, :165 and :174 cites are correct. Fixed: the comment now cites src/E07A9.C:37 (define) and :276 (the use). Those moved after the e07a9 round 9 fixes, so the current lines are cited. The E1B82.C cites still hold.
- [x] must-fix src/E19BC.C:56 (MEMALLOC_FILL_CHUNK): "19BC:00E3" is not the step. 19BC:00EE is `sub word ptr [bp-0Eh],0FA00h` and 19BC:00FA is `add ax,0FA0h` (the segment step). 00E3 is the push of the memset argument. Cite 00EE and 00FA. Fixed: the comment now cites the sub at 19BC:00EE and the segment step at 19BC:00FA. Rechecked with tools/re/disasm.py.
- [x] should src/E19BC.C:115-120 (MemFree): the NULL decrement looks like a bug and must be kept, so it is a QUIRK that READABILITY.md says to mark. Add `QUIRK 19BC:0182:` here (19BC:0143 jumps over the farfree straight to the dec at 19BC:0182). Fixed: the comment carries `QUIRK 19BC:0182`, says 19BC:0143 jumps over the farfree straight to the `dec [0x810e]`, and that MemFree(NULL) still decrements.
- [x] should src/E19BC.C:137-139 (MemRealloc): "the mask drops the low nibble, which no caller in src/ sets" is unclear and partly misleading. MemAlloc writes the low nibble into an aligned block's header (`(pad - 1) | flags`, 19BC:00A9), and MemRealloc reads it as hdrLow. Say that: the low nibble of an aligned header is the alignment pad; no caller passes low-nibble flags, so a non-aligned header's low nibble is zero. Fixed: the comment now says hdrLow is the low nibble, the alignment pad MemAlloc wrote in an aligned header (`(pad - 1) | flags`, 19BC:00A9), and zero in a non-aligned header because no caller passes low-nibble flags.
- [x] should src/E19BC.C:69 (MemAlloc): "the nested loop the original compiles to" claims a source form nobody has seen. The bytes show only that this loop matches (19BC:0031 to 0069). Say that, or drop the claim about the original. Fixed: the comment now says "The bytes at 19BC:0031 to 0069 match a two stage retry" and drops the claim about the original.
- [x] should src/E19BC.C:204-206 (OutOfMemory): "It restores the command line's video mode" holds only when argv[5] is nonzero. f1DDD_002D calls the BIOS only when g84A2[7] (ds:84B0, 0 in the image, src/E1DDD.C:16-18) differs from ds:84A0 (src/E1DDD.C:49). Say "when argv[5] (ds:84A0) is nonzero". Fixed: the comment now says it restores the video mode only when argv[5] (ds:84A0) is nonzero and cites the g84A2[7] (ds:84B0) comparison (src/E1DDD.C:16-18, :49).
- [x] should src/E19BC.C:178-181 (LargestFreeBlock comment): the paragraph wraps badly ("The" is left at the end of a line, and "(src/E082C.C:65 still says unsigned long)" sits mid-sentence). Rewrap it. Fixed: the paragraph is rewrapped with no orphan words.
- [ ] note Stale cross-references in other files, for their owners: src/E1BB1.ASM:168 cites "E19BC.C:142" for the `max -= 0x10` (now E19BC.C:193); src/E1BB1.ASM:179 cites "E19BC.C:29-32" for the flag bits (now :49-50); src/E1C00.ASM:240-247 cites "E19BC.C:142 to 145" for the 0x20 return (now :163-165); src/E07A9.C:33-35 cites E19BC.C:42-46 and :49-57 (define now :49-50), and :127-128 cites :97 and :120 (now :111 and :134; the e07a9 review already has this).
- [ ] note src/E07C8.C:56 and src/EOV2.C:185 declare WaitGameKey void. Its routine returns the key in AX (src/E1AAD.ASM:481-482: call _ProcessKeyEvent, retf), and src/E1C0E.C:46 and src/E082C.C:78 declare int. This unit's cite at E19BC.C:39-41 could be :481-482 (line 483 is the ENDP).
- [ ] note f1BA8_0072: its body is `push cs; call near ptr _NormFar` (src/E1BA8.ASM:87), so `NormFar` is a known callee. The placeholder is fine for an extern; the E1BA8 owner may take the name.
- [ ] note carried from round 1: f1DDD_002D's parameter is `mode` here and `i` in src/E1DDD.C:43; MemAlloc's `int flags` is a bit mask (unsigned fits the value better); LargestFreeBlock is `unsigned long` in src/E082C.C:65.

## Summary

The meaning holds up. The names MemAlloc, MemFree, MemRealloc, LargestFreeBlock and OutOfMemory, the hook (ds:810A) and the counter (ds:810E) all have evidence (callers, the message string, the header bits, the ALLOC.H calls, and the disassembly), and they agree with src/ (E1C00.ASM, E1C0E.C, E07C8.C, E07A9.C, E1B82.C). The round 1 fixes are in place. The five must-fix items are all in comments: four cites point at the wrong lines, and one mechanism claim (the MemRealloc NULL store "wraps") is wrong. None changes a name or a type. Review result: review-failed, 5 must-fix. No matcher or spec question, so no escalation.

## Round 2 response (dec-deepseek-3)

All five must-fix findings and all five shoulds are fixed in src/E19BC.C (comments only, no byte changed):
the MemRealloc QUIRK cites 1C00:0055's `call far ptr _MemRealloc` (src/E1C00.ASM:243), the
`or ax,dx; je short fail` (:250-251) and the `call dword ptr ds:[818Eh]` (:269, comment :245-249);
the QUIRK mechanism now uses 19BC:01F2/01F5/01FF (BX = 0, add hdrLow, `es:[bx-1]`) and says the
byte lands at exactly 0000:000F, value 0x0F | flagBits, nothing wraps; gAllocCount cites
src/E07A9.C:148 and :286; MEMALLOC_ALIGNED cites :37 and :276; MEMALLOC_FILL_CHUNK cites
19BC:00EE and 19BC:00FA. Shoulds: MemFree carries `QUIRK 19BC:0182`; the MemRealloc low-nibble
wording is fixed (aligned header pad, 19BC:00A9); the MemAlloc comment says the bytes at
19BC:0031 to 0069 match the two stage retry instead of "the original compiles to"; the OutOfMemory
comment makes the video-mode restore conditional on argv[5] (ds:84A0) nonzero; the LargestFreeBlock
paragraph is rewrapped. The e07a9 fixes of round 9 moved the E07A9.C lines, so the lines cited above
are the current ones. Also fixed an unflagged stale cite: OutOfMemory's MemAlloc reference now names 19BC:0031 to
0069 and the `lcall [0x810a]` at 19BC:0063 rather than "E19BC.C:68-74". Every cite rechecked against
src/ and the disassembly (tools/re/disasm.py 19BC:018A to 020C, 0137 to 0189, 00E0 to 0103).
`make match FILE=src/E19BC.C` 5 of 5 (728 bytes); `make readability UNIT=e19bc` 0 findings. The four
notes stay for the owners they name. Review result: fixed, ready for re-review.

## Round 3 (rev-haiku, main e1ffddc)

Reviewer: rev-haiku. Commit reviewed: e1ffddc (main; the review/e19bc branch is at the same commit, and src/E19BC.C is
the round 2 fix from b2c039b). The round 1 and round 2 sections above are kept; their items stay ticked.
Checks run: `make match FILE=src/E19BC.C` 5 of 5 (728 bytes); `make readability UNIT=e19bc` 0 findings.
Read for meaning, not bytes. Cites checked against the source (not re-proved): src/E1C00.ASM:215-294 (the
`call far ptr _MemRealloc` at :243, `or ax,dx; je short fail` at :250-251, `call dword ptr ds:[818Eh]` at :269),
src/E07A9.C:37, :148, :276, :286 (StartupAlloc), src/E1B82.C:43, :165, :174, src/E1C0E.C:34, :46, :566, :604, :617,
:631, :637, :870-876, src/E1AAD.ASM:443-483 (WaitGameKey ends `call _ProcessKeyEvent; retf`), src/E1DDD.C:10-21 and
:43-51, src/E07C8.C:96-132 (Main, no allocation before :132), :326-365 (ShutdownGame ends `exit(0)` at :345;
AllocationFailure at :359), src/E1BA8.ASM:81-93, src/E19EA.ASM:7-8 (PrintString), src/E1BB1.ASM:28-41, :160-183
(the 16 is called a margin at E1BB1.ASM:70), src/E082C.C:78, :133, build/toolchain/tc100/INCLUDE/ALLOC.H:13 (_HEAPOK 2)
and MEM.H:32, :53 (size_t is unsigned, so memset's count is 16-bit). Every `src/` caller of MemAlloc passes 0 or 0x20
(E07C8.C:211-266, E1C0E.C:531, :703-704, :871, :876, E1B82.C:165, :174, E07A9.C:276-280, E1A47.C:61, EOV2.C:292,
:319, E177D.C:45), so MEMALLOC_ZERO_FILL has no caller, as the comment says. Every EvictFile caller is at E19BC.C:84
(EVICT_ANY) or E1C0E.C:875 (entry->flags), as the EVICT_ANY comment says. The round 2 fixes are in place.

## Findings

- [ ] should src/E19BC.C:162, :171-172 (MemRealloc): `hdrLow` holds the old low nibble and then the new alignment pad
  (`hdrLow = 0x10 - (unsigned char)FP_OFF(blk)`). MemAlloc calls the same value `pad` (src/E19BC.C:77, :96). A separate
  `pad` local would say what it is, and the header comment (:142-146) would then not need "reuses hdrLow". Check with
  `make match` after the rename: a new local can change register use even though the source meaning is the same.
- [ ] should src/E19BC.C:182-188 (LargestFreeBlock): the reason for the `max -= 0x10` is not stated. The callers call that
  16 a margin: src/E1BB1.ASM:70 ("the margin (1BB1:0076) and removes it for MemAlloc"), src/E1BB1.ASM:34-40, :168-169
  ("add back the 16 that LargestFreeBlock takes off"). Say that, with those cites, or say the reason is unknown. (The
  MemAlloc aligned pad, src/E19BC.C:81, is a candidate reason, but the subtraction is unconditional, so it is not proven.)
- [ ] should src/E19BC.C:116-118 (MemFree comment): "The header byte (at segment minus one, offset plus 15)" describes
  the aligned header only. For a non-aligned block the header is the byte just before the block (src/E19BC.C:100-101,
  MemAlloc), and MK_FP(seg-1, off+15) is the same linear byte as seg:off-1 (MemFree :130 and :135). Say so, or the reader
  will take the segment-minus-one form as the only layout.
- [ ] should src/E19BC.C:24 and :210 (comments on the Main and OutOfMemory paths): the function is `Main` in
  src/E07C8.C:114, and the file there says the name is `Main`, not `main`, because of the C0 link stub. Use `Main` here too.
- [ ] should src/E19BC.C:32-34 (f1BA8_0072): the name is a placeholder with evidence: its body is `push cs; call near ptr
  _NormFar` (src/E1BA8.ASM:81-87), and _NormFar is at src/E1BA8.ASM:92-93 (1BA8:0081). The placeholder is allowed, but
  "the name stays a placeholder" gives no reason. Say why (for example, the E1BA8 owner has not named the pair), or
  name the wrapper after the evidence and rename it with `make rename` across src/.
- [ ] should src/E19BC.C:62-72 (MemAlloc comment): the hook's return value is not stated. A nonzero return from
  `gAllocFailHook()` makes MemAlloc retry farmalloc (src/E19BC.C:88-89, `continue`); a zero return gives up with NULL.
  The comment implies this ("only when both return 0 does it give up"); say it outright.
- [ ] note src/E19BC.C:1-9 (file header): the claims about the `add sp,n` sites and the `pop` in the -O -Z units were
  checked by the round 2 reviewer (tools/re/disasm.py), not by me; I did not re-run them.
- [ ] note the MEMALLOC_ALIGNED and ZERO_FILL defines are repeated in src/E07A9.C:37, src/E1B82.C:43 and src/E19BC.C:49
  (and E07A9.C and E1B82.C each carry their own cite block). Whether they belong in one shared header is an owner's call.
- [ ] note carried from rounds 1 and 2, still open for their owners: MemAlloc's `int flags` is a bit mask (`unsigned` fits
  the value better, E19BC.C:73); f1DDD_002D's parameter is `mode` here and in src/E07C8.C:58 but `i` in src/E1DDD.C:43;
  LargestFreeBlock is `unsigned long` in src/E082C.C:65 but `long` here, src/E07C8.C:69 and src/E1C0E.C:34; WaitGameKey
  is void at src/E07C8.C:56 and src/EOV2.C:185 but int here (its value is the key, E1AAD.ASM:481-482).
- [ ] note cross-unit, for the E07C8 owner: the AllocationFailure comment (src/E07C8.C:356) says "Restores video mode 7",
  but f1DDD_002D(7) loads the video mode from ds:84A0 (src/E1DDD.C:45-48) and calls the BIOS only when that is nonzero
  (src/E1DDD.C:49), as src/E19BC.C:212-216 says.
- [ ] note cross-unit stale cites, for their owners: src/E1C00.ASM:246-247 cites "E19BC.C:142 to 145" for the 0x20
  return, which is now src/E19BC.C:173; src/E1BB1.ASM:168-169 cites "E19BC.C:142" for the subtraction, which is now
  src/E19BC.C:200 (`max -= 0x10`). src/E07A9.C:273-275 cites "src/E19BC.C:62-66 (code at :80 and :94-97)"; the code is
  at :81 and :94-97.

## Summary

The unit is understandable from its source alone. The names MemAlloc, MemFree, MemRealloc, LargestFreeBlock and
OutOfMemory each have evidence (callers in src/E1C00.ASM, src/E1C0E.C and src/E1BB1.ASM, the message string, the header
flag bits, and the ALLOC.H calls), and the shared names (gAllocFailHook at ds:810A, gAllocCount at ds:810E, EvictFile,
PrintString, WaitGameKey, ShutdownGame) agree with src/. The round 2 must-fix cites and mechanism claims are now correct.
I found no wrong name, cite or claim in this round, so no must-fix. The open items are clarity: the local reuse in
MemRealloc, the unstated reason for the 16 margin, the linear-address wording in MemFree, the `Main` name, and the
placeholder on f1BA8_0072, all shoulds. Review result: review-passed (no must-fix open). No matcher or spec question,
so no escalation.

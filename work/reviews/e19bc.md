# Review: e19bc

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

- [ ] must-fix src/E19BC.C:129-130 (MemRealloc comment): "the caller tests it (src/E1C00.ASM:152-158)" is wrong. The caller's `or ax,dx; je short fail` does not catch a failed realloc of the 0x20 block: MemRealloc's 0x20 path returns MK_FP(FP_SEG(blk)+1, 0) (E19BC.C:142-145), so a NULL realloc comes back as 0001:0000 and the caller calls 0001:AX (src/E1C00.ASM:221-225). The cite 152-158 is the segment-register comment, not the test. Fix: say that a failed realloc is not reported here or at the caller, mark it `QUIRK 19BC:018A:` with that behaviour (the 0x20 path also writes `(blk - 1)[hdrLow]` through the NULL result before returning, which lands near 0000:000F after the offset wraps [H], to be confirmed in the oracle), and cite src/E1C00.ASM:221-225. The non-aligned path (E19BC.C:148, `*blk = flagBits`) dereferences the NULL at once; no caller in src/ uses it (the packet lists MemRealloc's only caller as 1C00:0055), so say so.
- [ ] must-fix src/E19BC.C:43 (MEMALLOC_ALIGNED comment): "src/E07A9.C:29 already names this bit and uses it at src/E07A9.C:208" is wrong. The define is at src/E07A9.C:35 and the use at src/E07A9.C:232 (`gBuf8DCA = MemAlloc(7500L, MEMALLOC_ALIGNED)`). Also src/E1B82.C:43 defines it and uses it at :165 and :174. Fix the cites.
- [ ] must-fix src/E19BC.C:20 (g810E comment): "it is g810E in src/E07A9.C:96" is wrong; line 96 is a comment that does not name the global. The declaration is src/E07A9.C:108 and the clear is at :242 (`g810E = 0;` in StartupAlloc, 07A9:00E6). Fix the cite.
- [ ] should src/E19BC.C:179 (OutOfMemory comment): "which MemAlloc lcalls when farmalloc returns NULL" is incomplete. MemAlloc calls the hook only after farmalloc fails and `EvictFile(0, 0xff)` returns 0 (E19BC.C:68-74, 73). Say that.
- [ ] should src/E19BC.C:17-22 (g810E): the evidence supports naming what the global holds: MemAlloc adds one per success (E19BC.C:97), MemFree subtracts one per call (:120), and StartupAlloc clears it (src/E07A9.C:242). Purpose is unknown (the E07A9.C:96-97 comment says so), but "net allocation counter" is what it holds. Consider `make rename OLD=g810E NEW=gAllocCount WHY="..."` so the placeholder sentence can go; if the placeholder stays, say which evidence is missing.
- [ ] should src/E19BC.C:39 (`extern void WaitGameKey(void)`): the routine returns the key in AX (1AAD:053A ends in `call _ProcessKeyEvent; retf`, src/E1AAD.ASM:444-483), and src/E1C0E.C:46 and src/E082C.C:78 declare `int` and use the value (src/E1C0E.C:566, src/E082C.C:133). src/E07C8.C:56 and src/EOV2.C:185 say void. Declare `int` so the type fits the value, or say why void is used here.
- [ ] should src/E19BC.C:35-36 and :69 (EvictFile): the `0xff` in `EvictFile(0, 0xff)` is a magic number that reaches FindEvictableFile as its flags (src/E1C0E.C:637, 631). Name it or explain what it selects. Also src/E19BC.C:90-95: the 0xFA00 zero-fill chunk is a magic number; name it (a chunk size) and say why the chunks exist, or say the reason is unknown.
- [ ] should src/E19BC.C:56-57 (MemAlloc comment): "The two stage retry is the nested loop the original compiles to." does not say what the stages are. Rewrite as: first the EvictFile retry, then the hook.
- [ ] should src/E19BC.C:156 (LargestFreeBlock comment): "_HEAPOK, which is 2" is correct (build/toolchain/tc100/INCLUDE/ALLOC.H:13), but it cites only "ALLOC.H". Cite that header line.
- [ ] should src/E19BC.C:158 (LargestFreeBlock comment): "src/E1C0E.C:32" is stale; the `extern long LargestFreeBlock(void)` declaration is src/E1C0E.C:34 (line 32 is MemAlloc). src/E07C8.C:69 is correct.
- [ ] should src/E19BC.C:178-180 (OutOfMemory comment): the claim that the image stores 19BC:02B6 at ds:810A has evidence in re/globals.toml:12 (the `lcall [0x810a]` at 19BC:0063 and the pointer stored at DGROUP 810A), but the comment cites none. Cite it.
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

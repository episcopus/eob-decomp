# Review: e19bc

Reviewer: rev-haiku
Commit reviewed: 361a2fe482d6ab636b6224fb44bcbdebda5f68b0

Playbook 70. Judge what a matching file can still get wrong: names, types, comments, structure.
The matcher already proved the bytes; do not re-prove them, read for meaning.

## Findings

Each finding is a checkbox. `must-fix` blocks `make review-pass` until a decompiler ticks it off
(with the commit that fixes it); `should` and `note` do not block.

- [ ] must-fix src/E19BC.C:15 and 148-149: the hook at ds:810A is said to hold 19BC:02B6 (OutOfMemory),
  "installed at ds:810A". The only support is re/globals.toml:12 (an `lcall [810A]` at 19BC:0063 and a
  pointer stored at DGROUP 810A). This unit's callers show a different run-time value: src/E07C8.C:132
  runs `gAllocFailHook = AllocationFailure;` in Main before any allocation, and src/E07C8.C:356-358 says
  main stores 07C8:0625 at ds:810A. Say what ds:810A holds before main (the image value, if that is what
  re/globals.toml:12 shows) and that main replaces it with AllocationFailure; do not describe OutOfMemory
  as the hook in use.
- [ ] must-fix src/E19BC.C:23: `extern int f1DDD_002D(int i)` is wrong. The definition is
  `void f1DDD_002D(int i)` (src/E1DDD.C:43), and src/E07C8.C:58 declares it void. Declare it void.
  (The same finding is in work/reviews/e1ddd.md:22.)
- [ ] must-fix src/E19BC.C:149: "It restores video mode 7" is wrong. The call `f1DDD_002D(7)` does not set
  mode 7: src/E1DDD.C:45-48 takes the table entry when i is below 7 and otherwise takes g84A0, which is the
  video mode from argv[5] (src/E07C8.C:124, `g84A0 = atol(argv[5])`). The call restores the command line
  mode. The same wrong sentence is in src/E07C8.C:356 (route to the e07c8 owner) and re/renames.toml:55.
- [ ] must-fix src/E19BC.C:150: "runs the two cleanups" is wrong. The second call is WaitGameKey
  (1AAD:053A), which waits for a key and does no cleanup. The only
  cleanup is ShutdownGame (07C8:058A), which ends in `exit(0)` (src/E07C8.C:345). Say: prints the message,
  waits for a key, then shuts down.
- [ ] must-fix src/E19BC.C:32: "ds:810E counts the live blocks" is wrong. MemFree decrements the counter
  even for a NULL pointer (src/E19BC.C:95 is unconditional, and line 81 of the same comment says so), and
  MemAlloc adds one only on success (src/E19BC.C:73). So the counter is the net count of successful
  MemAlloc calls minus MemFree calls, and it can fall below the number of live blocks. Say that. The
  global keeps its placeholder name `g810E` (src/E07A9.C:96).
- [ ] should src/E19BC.C:22 and 29: EvictFile is not a hook. src/E1C0E.C:509-511 defines it as the first
  retry, which drops the least important cached file that can hold the request and returns 1 or 0. Say
  "retry" in both comments. The extern's parameter names should match the definition
  (`unsigned size, unsigned char flags`, src/E1C0E.C:512); the extern now has `a` and `b`.
- [ ] should src/E19BC.C:131: `unsigned long LargestFreeBlock(void)` disagrees with the other units:
  `long` in src/E07C8.C:69 and src/E1C0E.C:32, `unsigned long` in src/E082C.C:65. The function never
  returns a negative value (lines 142-145 clamp it), so the bytes are not at stake, but pick one type and
  make the units agree.
- [ ] should src/E19BC.C:29-31: the flag bits are named in one place only. `#define MEMALLOC_ALIGNED 0x20`
  (src/E07A9.C:29) is the 0x20 bit (used at src/E07A9.C:208). The 0x10 zero-fill bit has no caller in src/
  (a grep finds no MemAlloc call that passes 0x10; src/E07C8.C:259 passes 0x10L as the size), so its
  meaning comes from the code alone. Say where each name comes from. The phrase "paragraph aligned for flag
  0x20" should say what the bytes do: the block starts at offset 0 of a paragraph, and its header sits at
  offset 15 of the paragraph before (the MemFree comment at line 79 says this).
- [ ] should src/E19BC.C:110-120: in MemRealloc `low` holds two things: the header's low nibble (line 110),
  then the alignment byte (line 115). `high` holds `flags & 0xf0` (line 111), which is the masked flags
  byte, not the high nibble. Rename or comment both (for example `hdrLow` and `flagBits`). The header at
  line 102 says "restores the flags byte"; say that the mask drops the low nibble, which no src/ caller sets
  (see the 0x10 note above). The header should also say that the result of realloc is not tested here; the
  caller test is at src/E1C00.ASM:152-158.
- [ ] should src/E19BC.C:158 (OutOfMemory): "returns 0, which tells MemAlloc not to retry" describes
  a return that is never reached, because ShutdownGame (line 157) ends in exit(0) (src/E07C8.C:345). Say the
  return is there to meet the hook's `int` type.
- [ ] note src/E19BC.C:8: `/*@flags -ml -O -Z -G */` has no explanation. AGENTS.md says switches can differ
  by unit (the `-G` case is e1b82). Say what `-G` changes in this unit, or which bytes need it.
- [ ] note the names that stay placeholders, with the reason: `g810E` (the net counter, see the finding
  above); `f1BA8_0072` (src/E1BA8.ASM:80-90 calls _NormFar, so "normalizes a far pointer" is the evidence;
  a name is still open); `f1DDD_002D` (named in the e1ddd review, a rename touches every caller, so it is
  not changed here). `MemAlloc`, `MemFree`, `MemRealloc`, `LargestFreeBlock` and `OutOfMemory` have names
  from their code, their callers and the message string ("Memory Allocation Error!", ds:7FEC, in
  work/reviews/e19ea.md:16).
- [ ] note evidence checked and held, so the next reader need not repeat it: `_HEAPOK` is 2
  (build/toolchain/tc100/INCLUDE/ALLOC.H:13), so "farheapwalk returns 2" is right; the callers that add 16
  back (src/E1BB1.ASM:34 and 168) support the +16 claim in the LargestFreeBlock comment; the 0x20 header
  arithmetic in MemFree and MemRealloc (line 79 and lines 110-116) is right.
- [ ] note re/renames.toml:55 says OutOfMemory "restores video mode 7 and exits", with the same error as the
  must-fix above. It is outside this file; whoever owns re/ should correct the WHY text.

## Summary

The unit is understandable from its source: five functions, each with its address, names taken from their
code, their callers and the message string, and every function matches (`make match FILE=src/E19BC.C`:
5 of 5, 728 bytes; `make readability UNIT=e19bc`: 0 findings). What a matching file can still get wrong is
in the comments: the hook at ds:810A is described as OutOfMemory although main installs AllocationFailure
(src/E07C8.C:132), the callee f1DDD_002D is declared int but defined void, "restores video mode 7" and "the
two cleanups" misreport what the calls do, and the byte counter is called a live-block count when it is a
net count. Five must-fix items; the placeholders g810E, f1BA8_0072 and f1DDD_002D stay where no further
evidence is given. Nothing is escalated: the matcher and the spec are not in question.

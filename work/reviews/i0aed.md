# Review: i0aed

Reviewer: rev-haiku
Commit reviewed: 6003a3d (main when I started; the branch review/i0aed was at aa13987 and I fast-forwarded it to main first; src/I0AED.C is the same in both)

Playbook 70. Judge what a matching file can still get wrong: names, types, comments, structure.
The matcher already proved the bytes; do not re-prove them, read for meaning.

## Round 1 (6003a3d)

`make match FILE=src/I0AED.C`: 4 of 4 functions match (547 of 547 bytes). `make readability UNIT=i0aed`:
0 findings. Those gates check bytes and structure only; the items below are what they cannot see.
Every citation was checked against the file it names on main. Addresses are `0AED:xxxx` (INTRO) unless
marked otherwise. `make twin UNIT=i0aed FIND=1` finds no EOB twin (see the should about the twin claim).

## Findings

- [ ] must-fix src/I0AED.C:33-34: the codec labels of formats 2 and 3 are said to come "from the bytes of
  0CE9:0181 and 0CE9:0109 only". 0CE9:0181 is Unpack1, format 1 (12-bit LZW, pinned [V] in
  WHITEPAPER.md:161). No bytes of format 2 (0CE9:02C1) are cited. The 14-bit label of format 2 has its
  evidence in the twin: src/E1D9A.ASM:11 and :542 (1D9A:03CC reads the 14-bit codes of format 2); cite
  that. Format 3's "literal and fill stream" is [H] in src/E1D9A.ASM:7 and no pinned file uses format 3
  (WHITEPAPER.md:161). Fix the sentence, and tag FORMAT_LITERAL_FILL (src/I0AED.C:38) as [H] unless the
  0CE9:0109 bytes are read and cited.

- [ ] must-fix src/I0AED.C:196-197: "its own entry is not in re/symbols.toml" is no longer true. On main
  (6003a3d, after the e17a6 split) re/symbols.toml has `i0aed_021b`: SetUnpackBuffer, 0AED:021B, 21 bytes,
  L2, and Unpack is 336 bytes (0AED:00CB). Say that the map used to fold it into Unpack and that the split
  gave it its own entry (e17a6), or drop the sentence.

- [ ] must-fix src/I0AED.C:162-163: "AllocationFailure ends in ShutdownGame ... (src/E07C8.C:359-366)". Those
  lines are the gSegsFC0C setup in another function (the MemAlloc cases for PAGE_65000, PAGE_32000 and
  PAGE_52000), not AllocationFailure. The right code is AllocationFailure at 07C8:0625 (src/E07C8.C:492, it
  calls ShutdownGame at :497) and ShutdownGame at 07C8:058A (src/E07C8.C:447). Cite those addresses. The
  claim is true; its citation points at the wrong code.

- [ ] should src/I0AED.C:1 (also :10-11): the twin claim ("the same module EOB.EXE links at 1B82:0009 ...
  which this unit is a twin of", and "as its twin E1B82 is") is not proven by the matcher. `make twin UNIT=i0aed
  FIND=1` finds no twin. `check('e1b82', 'i0aed')` in tools/match/twin.py fails at +0204: "the byte is not
  an address". +0204 is the jump table (1B82:020D, `jmp cs:[bx+20d]` in WHITEPAPER.md:43; 0AED:0211 here),
  whose code offsets twin.py does not relocate. Either say the twin rests on a hand comparison and name the
  table as the only difference (and show it), or call it "the same source as src/E1B82.C" without "twin".
  Matcher question for the human, not blocking: should twin.py relocate jump-table entries?

- [ ] should src/I0AED.C:1: "(547 bytes, 3 functions)". The file defines four functions and re/symbols.toml
  now has four for i0aed (LoadFile 0AED:000D, LoadPacked 0AED:0045, Unpack 0AED:00CB, SetUnpackBuffer
  0AED:021B). Say four.

- [ ] should src/I0AED.C:42 and :161-162: line citations into src/E19BC.C are stale or point at the wrong
  lines. MEMALLOC_ALIGNED is defined at src/E19BC.C:49 (not :46). The block shape is at :94-102 (not :80-83).
  The NULL rule is in the comment at :62-72 and the code at :88-92; "73-77" is the MemAlloc signature and
  "49-53" the flag defines. Cite 19BC:0007 and the range 19BC:0031 to 0069 instead of lines.

- [ ] should src/I0AED.C:33, :44, :110, :131, :185, and the unit header :11-12: WHITEPAPER line citations are
  stale. Section 4.2's header paragraph is at WHITEPAPER.md:157 (not 151), the format-4 and format-1
  paragraphs at :159 and :161 (not 153, 155), and "2.1.1, line 41" for the -P front end is at :43. Line 39
  (the -G note) is still right. Cite the section ("4.2", "2.1.1") instead of a line; the whitepaper moves.

- [ ] should src/I0AED.C:135: "(src/I0CE9.ASM, if written, :156, :245)" names a file that is not in src/. The
  claim (the decoders take segments) is supported by src/E1D9A.ASM:156 and :245 (the twin). Cite those
  or the INTRO 0CE9 addresses, not a hedged file.

- [ ] should src/I0AED.C:196: "StartupAlloc calls it once (0AED:0221/0229 store ds:1FB6 and ds:1FB8)" reads as
  if StartupAlloc does the stores; they are inside SetUnpackBuffer. Say: "StartupAlloc (07A9:000A) calls it
  once (src/E07A9.C:348); its stores are at 0AED:0221 and 0229".

- [ ] should src/I0AED.C:131 and :185: "WHITEPAPER 4.2 calls it palette bytes [H]". The whitepaper says "u16 bytes
  of palette that follow" with no [H] tag. Say that, and keep [H] as this unit's own tag. The claim itself
  is right (see the note below).

- [ ] note: the palette claim ("0 in all 131 format-4 entries") is true. The reviewer read every EOBDATA*.PAK
  with tools/re/pak.py and wwfmt.header: 131 format-4 entries, all with a palette field of 0; 5 format-1 entries.
  tests/unit/test_wwfmt.py skips the palette bytes without asserting them; an assertion there would make the
  claim checkable.

- [ ] note: names checked and consistent with src/: LoadFile (src/E1B82.C:86, same name; I0892 Main calls it),
  LoadPacked (src/E1B82.C:100), Unpack (src/E1B82.C:143), SetUnpackBuffer (src/E07A9.C:279, 1B82:0217),
  FileOpen/FileRead/FileClose (src/E1C0E.C:174, :277, :259), MemAlloc/MemFree (src/E19BC.C:73, :124),
  gBufSeg/gBufSize (ds:1FB6/1FB8 here, ds:812A/812C in EOB; names as src/E1B82.C:52-53). No finding.

- [ ] note: the FileOpen/FileRead/FileClose comments match src/E1C0E.C: HANDLE_MASK and FILE_IN_MEMORY (:85-86),
  `return -1` (:187, :201, :231), the FileClose read of gFileHandles[index] (:261), SLOT_FREE and FatalError
  (:285-287), ReadFromMemory (:288-289). No finding.

- [ ] note: f19EC_0002 is the placeholder name every file gives it (src/E07A9.C, EOV5.C, E1B82.C); its own
  definition is src/E19EC.C:19-21 (MK_FP(gSegsFC0C[kind | 1], 0)), so the comment here is right. Not this unit's
  name to change.

- [ ] note: the "-P front end" claim (C++ keeps the cwd after the format byte) rests on WHITEPAPER 2.1.1 (section
  "The C and C++ front ends differ"), which matches the `/*@flags -ml -O -Z -G -P */` line and the cbw/cwd bytes.

## Summary

The unit is understandable from its source: four functions with real names that the twin (src/E1B82.C),
the file layer (src/E1C0E.C) and the allocator (src/E19BC.C) use too, and comments that give addresses for
almost every claim. Three things are wrong or out of date: the codec-label evidence for formats 2 and 3, the
"not in re/symbols.toml" sentence that main has overtaken, and the AllocationFailure citation that points at
the wrong lines. The twin claim is not proven by `make twin`; that may be a matcher limit (jump tables), so I
have left it as a should and raised it as a question. No placeholder names remain in the unit's own functions;
f19EC_0002 is a placeholder every file shares, and it is not this file's to rename.

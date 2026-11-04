# Review: i0aed

Reviewer: p6-rev-12
Commit reviewed: ce0608f367ed01b1b1df84e254eef04d1a734b27 (P6 round; the diff since round 2 at 41f1cda). Round 2 at 41f1cda (main; round 1 reviewed 6003a3d, the branch review/i0aed was at aa13987 and I fast-forwarded it to main first; src/I0AED.C is the same in both)

Playbook 70. Judge what a matching file can still get wrong: names, types, comments, structure.
The matcher already proved the bytes; do not re-prove them, read for meaning.

## Round P6 (2026-11-04), commit ce0608f367ed01b1b1df84e254eef04d1a734b27

Judged: the names and comments changed since round 2 at 41f1cda (`git diff 41f1cda HEAD -- src/I0AED.C`). The diff renames `f19EC_0002` to `GetPagePtr` and `gSegsFC0C` to `gPageSegs`, and replaces the local format constants and `MEMALLOC_ALIGNED` with `#include "EOB.H"`. Checked each rename against its `why` in `re/renames.toml` and `re/globals.toml`, against the code at its address, and against the EOB twin's name in `src/`. The `re/symbols.toml` changes for this unit are review stamps only.

### Round P6 findings

- [ ] must-fix src/I0AED.C:8, :63, :93, :105, :106, :133 (and the comments that name it): the function at 097F:0002 is named `GetPagePtr`, but its EOB twin 19EC:0002 is named `GetPageBuffer` in src/E19EC.C:20 and src/E1B82.C:58 (the rename's own why says "Twin of EOB's 19EC:0002"). The same module linked at two addresses must have one name. Rename it to `GetPageBuffer` in src/I0AED.C and src/IOV0.C. The evidence in the why is otherwise right: the function returns MK_FP(gPageSegs[kind | 1], 0).

- [ ] should re/renames.toml, the `intro/097F:0002=GetPagePtr` row: the why cites `I0AED.C:112`, but the calls are at :105 and :106 (the line moved when the file changed). Cite the address 0AED:0045 or the current lines.

### Round P6 notes (not findings)

- The other changes check out: `gPageSegs` (intro/ds:28CC) matches EOB's ds:FC0C name; the `#include "EOB.H"` replaces the five `FORMAT_` constants, `MEMALLOC_ALIGNED`, `FORMAT1_BUFFER_SIZE` and `FORMAT2_BUFFER_SIZE`, all defined in src/EOB.H:360-383. The comments that name the new symbols are accurate.

### Round P6 summary

One must-fix: `GetPagePtr` diverges from the EOB twin's name (`GetPageBuffer`). One should: the stale line cite in the rename's why. Nothing escalated: no matcher, spec or pin question.

## Round 2 (41f1cda)

Later round (playbook 70): the round 1 must-fix items, and what changed since 6003a3d in src/I0AED.C,
re/symbols.toml and re/globals.toml. No line for this unit in re/symbols.toml or re/globals.toml changed
(the hunks there are other units). `make match FILE=src/I0AED.C` 4 of 4 functions, 547 of 547 bytes;
`make readability UNIT=i0aed` 0 findings.

Round 1 must-fix items, each checked against the cited code:

- Codec labels (round 1 must-fix 1): fixed right. Format 2 is 14-bit: src/E1D9A.ASM:11 and :542 (1D9A:03CC,
  `_ReadCode2` in the header at :13). Format 3's label is [H] at src/E1D9A.ASM:7; WHITEPAPER 4.2 says "No
  pinned file uses formats 2 or 3". FORMAT_LITERAL_FILL carries [H]. The tick stays.
- SetUnpackBuffer not in re/symbols.toml (round 1 must-fix 2): fixed right. re/units.toml gives the unit
  `functions = 3`; re/symbols.toml has i0aed_021b, 0AED:021B, 21 bytes. The tick stays.
- AllocationFailure and ShutdownGame (round 1 must-fix 3): fixed right. 07C8:0625 is src/E07C8.C:492, which
  calls ShutdownGame at :497; ShutdownGame is 07C8:058A at src/E07C8.C:447. The tick stays.

Changed lines since 6003a3d, checked:

- Twin header: with the ten jump-table bytes at 0AED:0211 (1B82:020D) treated as data, `tools/match/twin.py`'s
  `check('e1b82', 'i0aed')` passes (reason None; a scratch run, no file changed). The EOB table words are
  00E7, 011E, 0176, 010D, 01CC and the INTRO words 00EB, 0122, 017A, 0111, 01D0, each exactly +4. The header's
  "547 bytes, 4 functions" matches re/units.toml (e1b82 and i0aed both 547, functions 4 and 3 respectively
  for the INTRO unit; the file says 4 because the map's i0aed_021b entry is now separate).
- MEMALLOC_ALIGNED: defined at src/E19BC.C:49, block shape at :94-102 (both read). The QUIRK addresses
  19BC:0031 to 0069 (retry, src/E19BC.C:70 and :209) and the NULL return at 19BC:0073 to 0077 were read from
  the MemAlloc listing (`cmatch.py --show MemAlloc`: NULL return at function offsets 006C to 0070, retry at
  002A to 0062, which is 19BC:0031 to 0069 and 19BC:0073 to 0077 after the +7 of the function start).
- WHITEPAPER: 4.2 is the section with the "bytes of palette that follow" quote and the formats 1, 2, 3 and 4
  statements; 2.1.1 has the -P front end and the -G switches note. All section names, no line numbers.
- src/E1D9A.ASM:163-174 (Unpack3 takes src at [bp+6] and dst at [bp+8]) and :285 (`mov es,[bp+8]` inside
  `_Unpack1`, which starts at :273): both correct.
- StartupAlloc (07A9:000A) calls SetUnpackBuffer at src/E07A9.C:348 (read); the 0AED:0221 and 0229 stores are
  in SetUnpackBuffer's range (021B to 0230).

## Findings (round 2)

- [ ] should src/I0AED.C:204-205: "StartupAlloc (07A9:000A) calls it once (src/E07A9.C:348); its stores are at
  0AED:0221 and 0229". "its" follows StartupAlloc in the sentence, and the stores belong to SetUnpackBuffer.
  Say "the stores in SetUnpackBuffer are at 0AED:0221 and 0229". The claim is right; only the pronoun can be
  read wrongly.

No must-fix: the three round 1 must-fix items are fixed right and every changed line is sound.

## Round 1 (6003a3d)

`make match FILE=src/I0AED.C`: 4 of 4 functions match (547 of 547 bytes). `make readability UNIT=i0aed`:
0 findings. Those gates check bytes and structure only; the items below are what they cannot see.
Every citation was checked against the file it names on main. Addresses are `0AED:xxxx` (INTRO) unless
marked otherwise. `make twin UNIT=i0aed FIND=1` finds no EOB twin (see the should about the twin claim).

## Findings

- [x] must-fix src/I0AED.C:33-34: the codec labels of formats 2 and 3 are said to come "from the bytes of
  0CE9:0181 and 0CE9:0109 only". 0CE9:0181 is Unpack1, format 1 (12-bit LZW, pinned [V] in
  WHITEPAPER.md:161). No bytes of format 2 (0CE9:02C1) are cited. The 14-bit label of format 2 has its
  evidence in the twin: src/E1D9A.ASM:11 and :542 (1D9A:03CC reads the 14-bit codes of format 2); cite
  that. Format 3's "literal and fill stream" is [H] in src/E1D9A.ASM:7 and no pinned file uses format 3
  (WHITEPAPER.md:161). Fix the sentence, and tag FORMAT_LITERAL_FILL (src/I0AED.C:38) as [H] unless the
  0CE9:0109 bytes are read and cited.
  Fixed: the format comment now says format 2 is 14-bit because its code reader reads 14-bit codes
  (1D9A:03CC ReadCode2, src/E1D9A.ASM:11 and :542) and calls format 3's "literal and fill stream" this
  unit's own reading of the bytes, [H] (src/E1D9A.ASM:7). FORMAT_LITERAL_FILL carries [H]. The format
  1 and 4 [V] claims now cite WHITEPAPER 4.2 by section, no line number.

- [x] must-fix src/I0AED.C:196-197: "its own entry is not in re/symbols.toml" is no longer true. On main
  (6003a3d, after the e17a6 split) re/symbols.toml has `i0aed_021b`: SetUnpackBuffer, 0AED:021B, 21 bytes,
  L2, and Unpack is 336 bytes (0AED:00CB). Say that the map used to fold it into Unpack and that the split
  gave it its own entry (e17a6), or drop the sentence.
  Fixed: the SetUnpackBuffer comment says the map folded it into Unpack's range (re/units.toml counts
  three functions) and that the e17a6 split gave it its own re/symbols.toml entry (i0aed_021b, 21 bytes).

- [x] must-fix src/I0AED.C:162-163: "AllocationFailure ends in ShutdownGame ... (src/E07C8.C:359-366)". Those
  lines are the gSegsFC0C setup in another function (the MemAlloc cases for PAGE_65000, PAGE_32000 and
  PAGE_52000), not AllocationFailure. The right code is AllocationFailure at 07C8:0625 (src/E07C8.C:492, it
  calls ShutdownGame at :497) and ShutdownGame at 07C8:058A (src/E07C8.C:447). Cite those addresses. The
  claim is true; its citation points at the wrong code.
  Fixed: the QUIRK now cites AllocationFailure (07C8:0625, src/E07C8.C:492) and ShutdownGame (07C8:058A,
  src/E07C8.C:447), and the NULL rule as 19BC:0007 with the retry at 19BC:0031 to 0069 and the NULL
  return at 19BC:0073 to 0077 (src/E19BC.C:88-92).

- [x] should src/I0AED.C:1 (also :10-11): the twin claim ("the same module EOB.EXE links at 1B82:0009 ...
  which this unit is a twin of", and "as its twin E1B82 is") is not proven by the matcher. `make twin UNIT=i0aed
  FIND=1` finds no twin. `check('e1b82', 'i0aed')` in tools/match/twin.py fails at +0204: "the byte is not
  an address". +0204 is the jump table (1B82:020D, `jmp cs:[bx+20d]` in WHITEPAPER.md:43; 0AED:0211 here),
  whose code offsets twin.py does not relocate. Either say the twin rests on a hand comparison and name the
  table as the only difference (and show it), or call it "the same source as src/E1B82.C" without "twin".
  Matcher question for the human, not blocking: should twin.py relocate jump-table entries?
  Fixed: the header now says "the same source as src/E1B82.C", calls the correspondence a hand comparison
  and names the table at 0AED:0211 (1B82:020D) with its five entries each four higher (0AED:00EB for
  1B82:00E7, and so on) as what twin.py does not relocate. Checked: with those ten bytes treated as data,
  the whole 547-byte module passes twin.py's comparison (every other differing byte is a relocated word
  or an address field). The matcher question is left for the human.

- [x] should src/I0AED.C:1: "(547 bytes, 3 functions)". The file defines four functions and re/symbols.toml
  now has four for i0aed (LoadFile 0AED:000D, LoadPacked 0AED:0045, Unpack 0AED:00CB, SetUnpackBuffer
  0AED:021B). Say four.
  Fixed: the header says "(547 bytes, 4 functions)" and notes that the map folded SetUnpackBuffer into
  Unpack's range.

- [x] should src/I0AED.C:42 and :161-162: line citations into src/E19BC.C are stale or point at the wrong
  lines. MEMALLOC_ALIGNED is defined at src/E19BC.C:49 (not :46). The block shape is at :94-102 (not :80-83).
  The NULL rule is in the comment at :62-72 and the code at :88-92; "73-77" is the MemAlloc signature and
  "49-53" the flag defines. Cite 19BC:0007 and the range 19BC:0031 to 0069 instead of lines.
  Fixed: the MEMALLOC_ALIGNED comment cites src/E19BC.C:49 and :94-102; the QUIRK cites 19BC:0007, the
  retry at 19BC:0031 to 0069 and the NULL return at 19BC:0073 to 0077, plus src/E19BC.C:88-92.

- [x] should src/I0AED.C:33, :44, :110, :131, :185, and the unit header :11-12: WHITEPAPER line citations are
  stale. Section 4.2's header paragraph is at WHITEPAPER.md:157 (not 151), the format-4 and format-1
  paragraphs at :159 and :161 (not 153, 155), and "2.1.1, line 41" for the -P front end is at :43. Line 39
  (the -G note) is still right. Cite the section ("4.2", "2.1.1") instead of a line; the whitepaper moves.
  Fixed: every WHITEPAPER citation in the file now names the section only (4.2, 2.1.1, the -G switches
  note).

- [x] should src/I0AED.C:135: "(src/I0CE9.ASM, if written, :156, :245)" names a file that is not in src/. The
  claim (the decoders take segments) is supported by src/E1D9A.ASM:156 and :245 (the twin). Cite those
  or the INTRO 0CE9 addresses, not a hedged file.
  Fixed, with a correction: src/E1D9A.ASM:156 is a pop in Unpack4's exit and :245 is blank, so those lines
  do not support the claim. The text now cites src/E1D9A.ASM:163-174 (Unpack3 takes the segment src at
  [bp+6] and dst at [bp+8]) and src/E1D9A.ASM:285 (`mov es,[bp+8]` in Unpack1).

- [x] should src/I0AED.C:196: "StartupAlloc calls it once (0AED:0221/0229 store ds:1FB6 and ds:1FB8)" reads as
  if StartupAlloc does the stores; they are inside SetUnpackBuffer. Say: "StartupAlloc (07A9:000A) calls it
  once (src/E07A9.C:348); its stores are at 0AED:0221 and 0229".
  Fixed as suggested.

- [x] should src/I0AED.C:131 and :185: "WHITEPAPER 4.2 calls it palette bytes [H]". The whitepaper says "u16 bytes
  of palette that follow" with no [H] tag. Say that, and keep [H] as this unit's own tag. The claim itself
  is right (see the note below).
  Fixed at both places: they quote "bytes of palette that follow" and tag only this unit's reading [H].

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

## Fix round 1 (dec-deepseek-3)

All three must-fix and all seven should findings are ticked above with what changed. Every change is in
src/I0AED.C comments; no code, name or type changed. Two reviewer citations did not hold and are corrected in
the tick notes: src/E1D9A.ASM:156 and :245 do not show the decoders taking segments (they are a pop in
Unpack4's exit and a blank line), so the unit cites src/E1D9A.ASM:163-174 and :285 instead. Gates after the
fix: `make match FILE=src/I0AED.C` 4 of 4 functions, 547 of 547 bytes; `make readability UNIT=i0aed` 0
findings. The twin still fails `make twin` at the unrelocated jump table (a matcher question for the human,
not blocking). The note about asserting the palette field in tests/unit/test_wwfmt.py is left open: it is a
test change, not this unit's.

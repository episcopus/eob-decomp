# Review: i0cda

Reviewer: rev-haiku
Commit reviewed: 0cbdc72 (round 5; round 4 reviewed a97a0d7; round 3 reviewed 6ea511f; round 1 reviewed aa139872dfa3474f683144e3ec4f289840e9c58d)

Playbook 70. Judge what a matching file can still get wrong: names, types, comments, structure.
The matcher already proved the bytes; do not re-prove them, read for meaning.

## Findings

Each finding is a checkbox. `must-fix` blocks `make review-pass` until a decompiler ticks it off
(with the commit that fixes it); `should` and `note` do not block.

### Round 5 (rev-haiku, 2026-11-02, main 0c8ba51)

Judged: the round 4 must-fix (src/I0CDA.ASM:46-47) and should (:28-29), fixed in 0cbdc72, and the diff
since a97a0d7 (`git diff a97a0d7 HEAD -- src/I0CDA.ASM re/symbols.toml re/globals.toml re/renames.toml`:
src/I0CDA.ASM lines 25 to 26 and 46 to 47 only; the other hunks are other units (E0EF3 and E07C8 whys,
e0bd2_01d0 and e122e_06d0 notes, review stamps). The RandomByte rows (re/renames.toml:2579 and :2582,
re/symbols.toml:10984) are unchanged. `make match FILE=src/I0CDA.ASM` 1 of 1 exit 0 (50 bytes).

- Round 4 should (:28-29) stays ticked: the two shr are at lines 62 and 63 and the rcr at line 69
  (`grep -n 'shr\|rcr' src/I0CDA.ASM`).
- Round 4 must-fix (:46-47) is reopened. Its drop half is right: src/IOV0.C:545 is the statement
  `RandomByte();`, and ovl0:1015 drops the result. Its "only reduce it" half is wrong, and I wrote
  that wording in round 4 without checking the callers. Some callers scale or offset the masked byte:
  src/IOV0.C:680 `y[i] = (RandomByte() & 3) * 8 + 0xb4;` and src/IOV0.C:1018 and :1027
  `(RandomByte() & 7) + 0x19`. The bytes agree: at ovl0:1397 `and ax,3` is followed by three
  `shl ax,1` (times 8), and at ovl0:26F6 and 2797 `and ax,7` is followed by `add ax,0x19` (round 3
  checked these bytes). "Only reduce" is therefore false, and the `test` forms (`test ax,7`,
  `test al,1`) are masks used as tests, not reductions.

New findings:

- [x] must-fix src/I0CDA.ASM:46-47: drop "only reduce it". Say that the callers mask or take the byte
  modulo (`% 31`, `& 7`, `& 3`, `& 1`), that some then scale or offset the result (`* 8 + 0xb4`,
  `+ 0x19`), and that two drop it (src/IOV0.C:545 and ovl0:1015). For example: "the callers take it
  modulo or masked (`% 31`, `& 7`, `& 3`, `& 1`), some then scale or offset the result (`* 8 + 0xb4`,
  `+ 0x19`), and two drop it (src/IOV0.C:545 and ovl0:1015)."
  FIXED: the comment now reads "No caller gives the byte a more specific meaning: the callers mask or
  take it modulo (`% 31`, `& 7`, `& 3`, `& 1`), some then scale or offset the result (`* 8 + 0xb4`,
  `+ 0x19`), and two drop it (src/IOV0.C:545 and ovl0:1015)." "only reduce it" is gone; the scaling
  callers (src/IOV0.C:680, :1018, :1027) and the two drops (src/IOV0.C:545, ovl0:1015) are named.

Round 5 is not a pass: one must-fix is open (the caller sentence). Everything else is fixed right.

### Round 4 (rev-haiku, 2026-11-02, main 3c9d29a)

Judged: the round 3 must-fix (src/I0CDA.ASM:43-44) and should (:45), fixed in a97a0d7 and ticked in
d34d7a9, and the diff since 6ea511f (`git diff 6ea511f HEAD -- src/I0CDA.ASM re/renames.toml`: src/I0CDA.ASM
lines 43 to 47 and re/renames.toml:2584 only; the re/symbols.toml change at i0cda_0006 is the notes line
moving). `make match FILE=src/I0CDA.ASM` 1 of 1 exit 0 (50 bytes); `make readability UNIT=i0cda` 0 findings.

- Round 3 must-fix (43-44, "returns the XOR") is fixed right and stays ticked: line 69 (`rcr`) writes new
  b0, line 65 (`rcl byte ptr es:[si+1]`) writes new b1, and line 71 (`xor al,es:[si+1]`) computes
  new b0 ^ new b1. re/renames.toml:2584 says the same.
- Round 3 should (45, "narrows") stays ticked: the sentence it asked to fix is gone. Its replacement
  has a wrong claim, which is the must-fix below.

New findings:

- [x] must-fix src/I0CDA.ASM:46-47 (a line the a97a0d7 comment changed; reopened in round 5, its 0cbdc72
  wording is still wrong): "each only reduces it" is
  false. The call at src/IOV0.C:545 is the statement `RandomByte();`, so it discards the byte, and the
  ovl0:1015 site drops it too (the comment's own lines 37 and 38 say so). Say that: "the callers that
  use it only reduce it (`% 31`, `& 7`, `& 3`, `& 1`), and two drop it (src/IOV0.C:545 and ovl0:1015)."
  FIXED 0cbdc72: the sentence now reads "No caller gives the byte a more specific meaning: the callers
  that use it only reduce it (`% 31`, `& 7`, `& 3`, `& 1`), and two drop it (src/IOV0.C:545 and
  ovl0:1015)."
  TICKED: the final wording at bf2f1da (the round 5 item above) drops "only reduce it": src/I0CDA.ASM:46-48
  now says the callers mask or take it modulo (`% 31`, `& 7`, `& 3`, `& 1`), some then scale or offset the
  result (`* 8 + 0xb4`, `+ 0x19`), and two drop it (src/IOV0.C:545 and ovl0:1015), re-verified against
  src/IOV0.C:545 (`RandomByte();`), :680 (`(RandomByte() & 3) * 8 + 0xb4`) and :1018, :1027
  (`(RandomByte() & 7) + 0x19`). bf2f1da added one comment line above the code, so the round 4 should and
  round 5 line cites went stale by one; src/I0CDA.ASM:28-29 is refreshed to the two shr at lines 63 and 64
  and the rcr at line 70 (`grep -n 'shr\|rcr' src/I0CDA.ASM`).
- [x] should src/I0CDA.ASM:28-29: "the two shr (lines 60 and 61)" and "the rcr at line 67" are stale.
  They were right at 6ea511f, but a97a0d7 added two comment lines above them: `grep -n 'shr\|rcr'
  src/I0CDA.ASM` now gives 62, 63 (shr) and 69 (rcr). Round 3 accepted the old numbers and the fix made
  them stale. Update them to 62, 63 and 69, or drop the numbers and name the instructions.
  FIXED 0cbdc72: the numbers are now lines 62 and 63 (the shr) and line 69 (the rcr).

Round 4 is not a pass: one must-fix is open (the caller sentence at :46-47). The byte, the name and the
step list are fixed right.

### Round 3 (rev-haiku, 2026-11-02, main a49342c)

Judged: the round 2 must-fix items and should, and the changes since the round 2 review (`git diff
99b517e HEAD -- src/I0CDA.ASM`: the one commit 6ea511f, comments only; `make match` 1 of 1 exit 0,
`make readability UNIT=i0cda` 0 findings).

- Round 2 must-fix 1 (callers) is fixed right and stays ticked. The eleven `9a 06 00 d0 00` sites are
  at 05F5, 0644, 0650, 1015, 1397, 157A, 2043, 2092, 209E, 26F6 and 2797 in `build/re/intro-overlays/
  ovl00.bin`, each inside the function the comment names (ovl0:015D 1413 bytes, 0FD9 76, 1265 916,
  1DF3 1456, 23A3 1392; re/symbols.toml). The continuations match the bytes: `mov ah,0` at the eight
  sites, `a8 01 74 05` (test al,1; je) at 0650 and 209E, and at 1015 `mov ax,[2326h]` then the
  `jne` at 1021, which jumps back to 0FE5 (the loop before the call), so "the caller's own loop
  test" holds. The forms (`idiv` by 31, `test ax,7`, `and ax,3` then three `shl ax,1` at 1397,
  `test ax,3`, `and ax,7; add ax,0x19`) match the bytes.
- Round 2 must-fix 2 (name) is fixed right and stays ticked: re/symbols.toml:10860 reads `_RandomByte`,
  and re/renames.toml:2579 logs the rename.
- Round 2 must-fix 3 (shift wording) is fixed right and stays ticked: the two shr are at lines 60 and
  61, the rcr that writes DGROUP:2310 at line 67, and the step list (lines 19 to 26) agrees with
  lines 58 to 69 instruction by instruction.
- Round 2 should (AX = new b0 ^ new b1) is fixed and stays ticked.

New findings:

- [x] must-fix src/I0CDA.ASM:43-44 (a line the 6ea511f name comment changed): "returns a byte of it"
  is wrong. The routine returns AL = new b0 ^ new b1 (line 26; lines 68 and 69), a value computed
  from two state bytes, not one byte of the state. Say that: "and returns the XOR of the two bytes
  it has just written (new b0 ^ new b1)". The same phrase is in re/renames.toml:2584 (a note there,
  outside this round's file list; fix it with the comment). FIXED a97a0d7: the comment now reads
  "and returns the XOR of the two bytes it has just written (new b0 ^ new b1)", and re/renames.toml
  2584's `why` was reworded the same way.
- [x] should src/I0CDA.ASM:45: "No caller narrows it further" reads as the opposite of what the
  callers do: they reduce the byte with `% 31`, `& 7`, `& 3` (src/IOV0.C:293, :296, :680, :706). The
  sentence means that no caller gives it a more specific meaning; say that. FIXED a97a0d7: the
  sentence now reads "No caller gives the byte a more specific meaning; each only reduces it
  (`% 31`, `& 7`, `& 3`, `& 1`)."
- [ ] note: the far twin E1D8C.ASM still keeps `_f1D8C_0000`, carried from round 2; not a finding of
  this unit.

Round 3 is not a pass: one must-fix is open (the name comment's "returns a byte of it"). Every other
item is fixed right or is a note.

### Round 2 (rev-haiku, 2026-11-02, main e1422b3)

Judged: the round 1 must-fix items, and the diff since aa139872 (`src/I0CDA.ASM` only; the
`re/symbols.toml` and `re/globals.toml` changes in that range are other units).

- Round 1 must-fix 1 (callers) and must-fix 2 (the DGROUP word) are fixed right and stay ticked. The
  counts and places are as the comment now says: 11 relocatable `9a 06 00 d0 00` in
  `build/re/intro-overlays/ovl00.bin`, in iov0_015d (3), iov0_0fd9 (1), iov0_1265 (2), iov0_1df3 (3)
  and iov0_23a3 (2); the EOB calls at file offsets 0x146D2 and 0x1478F; `9f 0e` at 0CDA:000C.
- Round 1 should 3 (the step list) is right: each step matches lines 53 to 63. Its new sentence is
  not (see the third must-fix below).

- [x] must-fix src/I0CDA.ASM:32-36 (callers): "each continues `mov ah,0`" is wrong for three of the
  eleven sites. Eight continue with `mov ah,0` (ovl0:05F5, 0644, 1397, 157A, 2043, 2092, 26F6, 2797).
  ovl0:0650 and ovl0:209E continue with `test al,1; je`, and ovl0:1015 continues with
  `mov ax,[2326h]; or ax,[2328h]`, which is not `mov ah,0` and not a small value either. The forms
  named in the list are also off: the bytes are `test ax,7` at ovl0:0644 and 2092, `test ax,3` at
  157A, `and ax,7` at 26F6 and 2797, `and ax,3` at 1397, `idiv` by 31 at 05F5 and 2043. Say what
  the bytes do, per site or as counts (the ovl0 offsets are the `ovl00.bin` offsets, `make overlays`
  rebuilds that file). FIXED 6ea511f: the comment now lists the sites per containing function
  (ovl0:015D 05F5/0644/0650, ovl0:0FD9 1015, ovl0:1265 1397/157A, ovl0:1DF3 2043/2092/209E,
  ovl0:23A3 26F6/2797), says eight continue `mov ah,0` (05F5, 0644, 1397, 157A, 2043, 2092, 26F6,
  2797), 0650 and 209E only `test al,1`, and 1015 drops the result (its next bytes are the caller's
  `mov ax,[2326h]; or ax,[2328h]` loop test); the forms are given as the bytes do them: `idiv` by 31
  (05F5, 2043), `test ax,7` (0644, 2092), `and ax,3` then three `shl ax,1` (1397), `test ax,3`
  (157A), `test al,1` (0650, 209E), `and ax,7; add ax,0x19` (26F6, 2797).
- [x] must-fix src/I0CDA.ASM:38-41 and re/symbols.toml:10610 (name): the comment says "Purpose
  unknown ... so the placeholder name stays", but the source now names the routine `_RandomByte`
  (line 41) and re/symbols.toml still has `name = "_f0CDA_0006"`. The name itself has evidence: the
  routine is a 24-bit state step that returns a byte (the seed in the image), and src/IOV0.C:87
  already calls it `RandomByte`. So the comment is what is wrong. Fix the comment to say that
  (the step, the seed, and the callers using the byte as a random value), drop "placeholder", and
  run `make rename` so re/symbols.toml carries `_RandomByte` too. FIXED 6ea511f: the comment now
  says the name `_RandomByte` has evidence (the 24-bit step over DGROUP:2310, the seed 34 12 76,
  and the callers using the byte as a random value) with no placeholder wording; `make rename
  OLD=intro/0CDA:0006 NEW=RandomByte` logged it, and `make match WRITE=1` set the asm-convention
  name `_RandomByte` in re/symbols.toml:10631.
- [x] must-fix src/I0CDA.ASM:28 (the sentence added for round 1 should 3): "The shifts act on the
  copy in AL, not on DGROUP:2310" is false. The `rcr byte ptr es:[si],1` at line 61 shifts
  DGROUP:2310 itself, and the next sentence of the comment says the rcr writes it. Only the two
  `shr al,1` (lines 54 and 55) act on AL. Reword to "The two shr (lines 54 and 55) act on the copy
  in AL; DGROUP:2310 is only read until the rcr at line 61 writes it." FIXED 6ea511f: the comment
  reads "The two shr (lines 60 and 61) act on the copy in AL; DGROUP:2310 is only read until the
  rcr at line 67 writes it." (the two shr and the rcr moved to lines 60, 61 and 67 when the caller
  and name comments grew).
- [x] should src/I0CDA.ASM:26: "AX = b0 ^ b1" uses the values after the rcl and the rcr (lines
  62 and 63 read the bytes they wrote); say "new b0 ^ new b1" so the last step is checkable.
  FIXED 6ea511f: the step now reads "AX = new b0 ^ new b1, AH = 0 (the bytes the rcl and rcr just
  wrote)".
- [ ] note: the far twin E1D8C.ASM keeps `_f1D8C_0000` (re/symbols.toml:6982). If `RandomByte` is
  the name, the twin needs the same name at its next review; not a finding of this unit.

Round 2 is not a pass: three must-fix items are open. The unit is complete (`make readability
UNIT=i0cda` 0 findings) and both round-1 must-fixes stay fixed.

### Round 1 (rev-haiku, aa139872)

- [x] must-fix src/I0CDA.ASM:19: "no near or far call reaches either copy" is false for the EOB copy. EOB.EXE holds two far calls to 1D8C:0000 (`9a 00 00 8c 1d`) at 0EF3:2FA2 and 0EF3:305F (file offsets 0x146D2 and 0x1478F, unit e0ef3, kind c in re/units.toml), which src/E1D8C.ASM:15-17 records and work/reviews/e1d8c.md confirms. The INTRO copy has no caller (no `9a 06 00 da 0c` in INTRO.EXE). Say that: the INTRO copy has no caller, and the EOB copy is called twice from e0ef3, so the placeholder's question is the same as E1D8C's. FIXED 99b517e: the sentence is gone. The premise is inverted for the INTRO copy, though: ovl0 reaches 0CDA:0006 by eleven relocatable far calls (bytes `9a 06 00 d0 00`, the segment word a fixup to segment index 26 = 0CDA, file offsets 0x12DE5 to 0x14F87), in five functions (ovl0:015D, 0FD9, 1265, 1DF3, 23A3), each continuing `mov ah,0` and using the byte as a small value (`idiv` by 31, `and ax,7`, `test al,1`, `and ax,3`); no literal `9a 06 00 da 0c` exists because the overlay call is relocatable. The EOB copy's two calls from e0ef3 (0EF3:2FA2, 0EF3:305F) are as cited. The comment now states both.
- [x] must-fix src/I0CDA.ASM:27-28: "which spells the same value 212Eh" is wrong. INTRO's word is 0E9F (bytes `9f 0e` at 0CDA:000C; the file image holds the seed `34 12 76` at 0E9F:2310, so 0E9F is DGROUP in this image). src/E1D8C.ASM spells 212Eh, which is DGROUP's paragraph in EOB.EXE: the same group, a different value. The relocation claim is right: an MZ relocation entry covers 0CDA:000C, as one covers the immediate of every `mov ax,DGROUP` in both EXEs (19 in INTRO, 22 in EOB). Fix: "the same DGROUP as src/E1D8C.ASM, which spells 212Eh in EOB.EXE". FIXED 99b517e: the comment now reads "the word (0E9F, at 0CDA:000C) is a relocation to the group, fixed up at load; the same DGROUP as src/E1D8C.ASM, which spells 212Eh in EOB.EXE".
- [x] should src/I0CDA.ASM:15-17: "It shifts the byte at DGROUP:2310 right twice" describes a copy: the shifts act on AL (`mov al,es:[si]` and `shr al,1` twice, src/I0CDA.ASM:32-34), and DGROUP:2310 is only read there until the `rcr` writes it. "rotates DGROUP:2310 right through the low bit of its own shifted value" is unclear: the carry is bit 0 of AL after `sbb al,es:[si]` and the following `shr al,1`, where AL is (old DGROUP:2310 >> 2) - old DGROUP:2310 - !bit 7 of old DGROUP:2311. The `cmc` before the `sbb` (which gives that complement) is not mentioned at all. Reword so a reader can check each step against the instructions. FIXED 99b517e: the step list now acts on AL (`al = b0; al = al >> 2`, `sbb al,es:[si]`, `al = al >> 1`), names the `cmc` (`carry = !(bit 7 of the old b1)`) and the `rcr` that writes DGROUP:2310, and says the byte there is only read until that `rcr` writes it.
- [ ] note src/I0CDA.ASM:5, :10-12: the lone byte checks out. 0CDA:0005 is `00` after the `retf` (`cb`) at 0CDA:0004, and 0CDA:0006 starts `55 8b ec` (push bp; mov bp,sp), the same as the EOB copy's start at 1D8C:0000. 51 bytes, 1 function, kind asm agree with the packet.
- [ ] note src/I0CDA.ASM:14: "2310h appears nowhere else" holds: the two bytes `10 23` occur once in INTRO.EXE, as the operand of `mov si,2310h` at 0CDA:0010. The seed `34 12 76` at 0E9F:2310 is in the file image. Every step in the list (shr/shr, rcl x2, cmc, sbb, shr, rcr, the final xor, AH 0) matches its instruction.
- [ ] note The name `_f0CDA_0006` is a placeholder. The Purpose unknown comment is right. The two copies run the same generator on different state (INTRO DGROUP:2310, EOB DGROUP:8484), so the EOB caller evidence does not name the INTRO copy's state; no finding.

## Summary

The unit is one 50-byte far function plus a lone padding byte, and it is understandable from its source: the step list describes the instructions, and the seed, the `2310h` access and the DGROUP relocation are all true. Two comments are wrong: the sentence on callers ("no near or far call reaches either copy" is false for the EOB copy, which has two far callers in e0ef3) and "the same value 212Eh" (INTRO's word is 0E9F). The step description also needs rewording (AL is shifted, not the memory byte, and the `cmc` is missing). The name stays a placeholder, which is fine: its only evidence is the EOB callers, which use the result as `& 3` and `& 1` (src/E1D8C.ASM:15-17). Two must-fix items are open.

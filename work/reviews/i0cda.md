# Review: i0cda

Reviewer: rev-haiku
Commit reviewed: aa139872dfa3474f683144e3ec4f289840e9c58d

Playbook 70. Judge what a matching file can still get wrong: names, types, comments, structure.
The matcher already proved the bytes; do not re-prove them, read for meaning.

## Findings

Each finding is a checkbox. `must-fix` blocks `make review-pass` until a decompiler ticks it off
(with the commit that fixes it); `should` and `note` do not block.

### Round 2 (rev-haiku, 2026-11-02, main e1422b3)

Judged: the round 1 must-fix items, and the diff since aa139872 (`src/I0CDA.ASM` only; the
`re/symbols.toml` and `re/globals.toml` changes in that range are other units).

- Round 1 must-fix 1 (callers) and must-fix 2 (the DGROUP word) are fixed right and stay ticked. The
  counts and places are as the comment now says: 11 relocatable `9a 06 00 d0 00` in
  `build/re/intro-overlays/ovl00.bin`, in iov0_015d (3), iov0_0fd9 (1), iov0_1265 (2), iov0_1df3 (3)
  and iov0_23a3 (2); the EOB calls at file offsets 0x146D2 and 0x1478F; `9f 0e` at 0CDA:000C.
- Round 1 should 3 (the step list) is right: each step matches lines 53 to 63. Its new sentence is
  not (see the third must-fix below).

- [ ] must-fix src/I0CDA.ASM:32-36 (callers): "each continues `mov ah,0`" is wrong for three of the
  eleven sites. Eight continue with `mov ah,0` (ovl0:05F5, 0644, 1397, 157A, 2043, 2092, 26F6, 2797).
  ovl0:0650 and ovl0:209E continue with `test al,1; je`, and ovl0:1015 continues with
  `mov ax,[2326h]; or ax,[2328h]`, which is not `mov ah,0` and not a small value either. The forms
  named in the list are also off: the bytes are `test ax,7` at ovl0:0644 and 2092, `test ax,3` at
  157A, `and ax,7` at 26F6 and 2797, `and ax,3` at 1397, `idiv` by 31 at 05F5 and 2043. Say what
  the bytes do, per site or as counts (the ovl0 offsets are the `ovl00.bin` offsets, `make overlays`
  rebuilds that file).
- [ ] must-fix src/I0CDA.ASM:38-41 and re/symbols.toml:10610 (name): the comment says "Purpose
  unknown ... so the placeholder name stays", but the source now names the routine `_RandomByte`
  (line 41) and re/symbols.toml still has `name = "_f0CDA_0006"`. The name itself has evidence: the
  routine is a 24-bit state step that returns a byte (the seed in the image), and src/IOV0.C:87
  already calls it `RandomByte`. So the comment is what is wrong. Fix the comment to say that
  (the step, the seed, and the callers using the byte as a random value), drop "placeholder", and
  run `make rename` so re/symbols.toml carries `_RandomByte` too.
- [ ] must-fix src/I0CDA.ASM:28 (the sentence added for round 1 should 3): "The shifts act on the
  copy in AL, not on DGROUP:2310" is false. The `rcr byte ptr es:[si],1` at line 61 shifts
  DGROUP:2310 itself, and the next sentence of the comment says the rcr writes it. Only the two
  `shr al,1` (lines 54 and 55) act on AL. Reword to "The two shr (lines 54 and 55) act on the copy
  in AL; DGROUP:2310 is only read until the rcr at line 61 writes it."
- [ ] should src/I0CDA.ASM:26: "AX = b0 ^ b1" uses the values after the rcl and the rcr (lines 62 and
  63 read the bytes they wrote); say "new b0 ^ new b1" so the last step is checkable.
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

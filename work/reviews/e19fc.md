# Review: e19fc

Reviewer: rev-sonnet
Commit reviewed: 3fe04f8616ac8534c1e2942a25801bac51e37ffe

Playbook 70. Judge what a matching file can still get wrong: names, types, comments, structure.
The matcher already proved the bytes; do not re-prove them, read for meaning.

`make match FILE=src/E19FC.ASM`: 1 of 1 functions match (420 bytes), unit complete.

## Checked

- Token set against the decoder 1D9A:0013 (tools/re/disasm.py): 80h ends (1D9A:0095), 81h..BFh copies `al & 3Fh` literal bytes (0099..00AA), 00h..7Fh copies `(al >> 4) + 3` bytes from `di - (((al & 0Fh) << 8) | next)` (0066..008D), FEh reads a count word and a fill byte (00C5..00D7), FFh reads a length word and then a distance word (00E0), C0h..FDh has length `(al & 3Fh) + 3` and a distance word (00BB..00BE, falls to 00E3). Each matches what the encoder emits (E19FC.ASM 0105..0180).
- The distance word of the C0h..FFh forms is added to `cs:[0109]`, the decoder's output start (1D9A:00E6); the encoder subtracts `inBase` (019F, the source start). The header's "same index" claim holds.
- Encoder limits: short form only for length 3..0Ah and distance <= 0FFFh (the `cmp cx,2 / 0ah / 0fffh` tests at 0105..0112); C0h..FDh for length up to 40h (0129); FFh beyond; lengths 1 and 2 become literals; literal runs are capped at 3Fh (`cmp byte ptr es:[bx],0bfh`, 0149).
- Normaliser 1BA8:0081 as in src/E1BA8.ASM (AX = offset, DX = segment in; AX = offset & 0Fh, DX = segment + offset >> 4 out); the decoder 1D9A:0013 opens with the same two calls and the same argument offsets, so the frame layout is the same one.
- Caller claim: I searched EOB.EXE for `9a 07 00 fc 19` and `9a 08 00 fc 19` and for the bare far pointers; no hit (overlay code not re-searched by me).

## Findings

- [x] must-fix src/E19FC.ASM:`litCount` (019F..01A1 data, uses at 0053, 0141, 0149): the name says "count" but the word holds the output offset of the current literal run's count byte (`mov litCount,di` before `stosb` of 80h; `mov bx,litCount ... inc byte ptr es:[bx]`). A reader takes it for a number of literals. Rename (for example `litCountPtr` or `litHdr`) and say in the data comment that it is a pointer into the output.
  Fixed in fe3d880: renamed `litCount` to `litHdr`; its data comment now reads "output offset of the open literal run's count byte", and the `newlit` store site carries "litHdr = offset of this run's count byte (the 80h below)" so a reader cannot take it for a count.
- [x] should src/E19FC.ASM header: state the limits the encoder keeps (matches shorter than 3 bytes become literals; short form only for length 3..0Ah and distance <= 0FFFh; C0h..FDh up to 40h bytes; literal runs up to 3Fh bytes). They are in the code but a reader of the header cannot see them, and the header names the token ranges only.
  Fixed in fe3d880: the header now has a paragraph "The limits the encoder keeps: matches shorter than 3 bytes become literals; the short 00h..7Fh form is used only for lengths 3..0Ah and distances <= 0FFFh; C0h..FDh carry lengths up to 40h; longer matches take FFh; literal runs are capped at 3Fh bytes (a full count byte starts a new 80h run)."
- [x] should src/E19FC.ASM:`cmp al,[si+40h]` (006F): comment "a run of equal bytes 40h ahead?" is a cheap precheck, not the test; the run length is only known after `repe scasb` and the `cmp cx,41h` at 0094. Say "precheck: a candidate fill run (byte 40h ahead equal)".
  Fixed in fe3d880: the comment at 006F now reads "precheck: a candidate fill run (byte 40h ahead equal)", so it is no longer read as the test itself.
- [x] should src/E19FC.ASM header: the "eight words of state" are described only by position. Give each a one line role in the data comments (litOpen: a literal run is open; outBase/inBase: the start of the output / input, as normalised offsets; outSeg/outPtr: the output position kept while es holds the source; bestLen/bestPos: the best match so far, length and input offset).
  Fixed in fe3d880: every one of the eight data words now carries a one-line role (litOpen, outBase, inBase, litHdr, outSeg, outPtr, bestLen, bestPos), and the header points at them ("each with its role in the data comments below").
- [x] note The decoder's third argument is the decompressed length (it adds it to the output start), this encoder's third argument is the source length: the same quantity seen from the other side, so this is the inverse of 1D9A:0013 for format 4.
  No change needed: the header already says the distances use "the same index, because the decompressed prefix has the same length", which is this observation.
- [x] note `[si+40h]` (006F..0072) can read up to 40h bytes past the source end near the buffer's tail; harmless for the matching but a reader may wonder.
  No change needed: it is the original's overscan; the comment at 006F now marks it as a precheck only.
- [x] note When a fill run reaches the end of the input exactly, `repe scasb` stops on cx = 0 without a mismatch and the `dec di` at 0083 makes the run one byte short (that last byte follows as a literal). The output stays valid; this is the original's behaviour, not a decompilation error.
  No change needed: original behaviour, kept as it is.
- [x] note A source length of 0 is not handled (the first literal is always written). Real callers do not exist (none found), so the name stays a placeholder; that is fine.
  No change needed: no caller was found, so no evidence for a better name; the placeholder and `Purpose unknown` stand.

## Summary

The unit is understandable from its source: the header describes the algorithm, the token set agrees with the decoder at 1D9A:0013 byte for byte, and the data words and labels are tied to addresses. The one wrong claim is the name `litCount`, which is a pointer, so the review fails on that single item until it is renamed (a header and label edit only, no byte changes). The function name `_f19FC_0007` is a placeholder and fine: nothing calls it, so there is no evidence for a better one beyond "LCW encoder for format 4", which the header already says.

Resolution (dec-deepseek, fe3d880, merged in e57b4c2): the must-fix and the three shoulds are done in that commit, comment and label edits only, no byte changes. `litCount` is `litHdr` and its data comment says it is the output offset of the run's count byte; the header states the encoder's limits; the 006F comment says "precheck"; all eight state words carry a one-line role. Re-verified here: `make match FILE=src/E19FC.ASM` reports 1 of 1 functions match, unit e19fc complete (ours 420 bytes, the unit 420), and `make readability UNIT=e19fc` reports no new findings. No open must-fix.

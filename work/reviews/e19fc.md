# Review: e19fc

Reviewer: rev-haiku
Commit reviewed: af827ef (main); src/E19FC.ASM, re/symbols.toml and re/renames.toml are the same at
9f13115, the review/e19fc worktree HEAD.

Playbook 70. Judge what a matching file can still get wrong: names, types, comments, structure.
The matcher already proved the bytes; do not re-prove them, read for meaning.

This is round 4. The earlier rounds are in git history (round 1 e1429a0, round 2 4f54830, round 3
b7743ea); the round 3 items were fixed in 05b2d7a and are ticked above, and I re-checked each fix
against the current file. This round reads the file as it stands.

What I checked (read-only; no game run; no source edited):

- `make match FILE=src/E19FC.ASM`: 2 of 2 (`_f19FC_0007` 1 byte, `_CompressLcw` 419 bytes).
  `make readability UNIT=e19fc`: 0 findings. Not re-proved here.
- Prologue and epilogue of `_CompressLcw` (src/E19FC.ASM:65-73 and 228-236): the pushes are bp, bx,
  cx, di, ds, dx, es, si and the pops restore si, es, dx, ds, di, cx, bx, bp. The argument offsets
  [bp+6], [bp+8], [bp+10], [bp+12], [bp+14] are right for a FAR function with bp pushed first.
- The far pointer normaliser: `_NormFar` (src/E1BA8.ASM:93-103) does DX += AX >> 4 and AX &= 0Fh,
  as the header says. The name and the address 1BA8:0081 agree with E1BA8.ASM:92.
- The marker: `eob/EOB.EXE` holds `00 55 8B EC 53 51 57 1E 52 06 56 8B 46 06` at file offset 116679,
  so 19FC:0007 is `00` and the first instruction, `push bp`, is at 19FC:0008 (the load module is at
  file offset 0x2800, and 19FC:0000 is file 116672). `build/re/eob.json` segment 28 (para 6652, start
  106439 = 19FC:0007, size 420) agrees.
- The byte search the header cites is real: `9a 07 00 fc 19`, `9a 08 00 fc 19`, `07 00 fc 19` and
  `08 00 fc 19` are absent from `eob/EOB.EXE` and the seven images in `build/re/eob-overlays`.
- The token forms in the header against the code: short reference (lines 176-181: length - 3 in the
  high nibble, distance <= 0FFFh, lengths 3..0Ah), C0h..FDh (lines 184-190: length 3..40h), FFh
  (lines 207-216), FEh (lines 118-129: the count word is xchg'd out of ax, then the byte), literal
  runs (lines 194-205: the count byte starts at 80h and is incremented to at most BFh), and the end
  byte 80h (lines 223-225). Each matches the header's description.
- The state words and their comments (lines 239-248) match the code: gBestPos is the candidate's
  input start (the `dec bx` at line 161, stored at line 162); gInBase is subtracted only on the absolute
  paths (line 214), not on the short form (line 173).
- The candidate test (lines 142-145, 157-159): `ah` is `[si + gBestLen - 1]` and the candidate byte is
  `es:[di + gBestLen - 2]` where di is one past the candidate start, so the comment is right; `jb`
  keeps the earlier match and an equal length replaces it, as the comment says.
- The Unpack4 citation and the other `src/` names: `Unpack4` is declared at src/E1B82.C:39 with
  `@ 1D9A:0013`. `wwfmt.py` `lcw` and whitepaper 4.2 exist as cited.
- The caller search: packet.py reports "(no direct call found)" for a unit with no direct call; the
  byte search above is the stronger check and it is negative.

## Findings

Each finding is a checkbox. `must-fix` blocks `make review-pass` until a decompiler ticks it off
(with the commit that fixes it); `should` and `note` do not block.

- [ ] must-fix src/E19FC.ASM:61-62: the function comment says "on return DS:SI points at the source end
  and ES:DI just past the output". The code does not leave those registers that way: the epilogue pops
  si, es, ds and di (src/E19FC.ASM:228-233), so on return SI, ES, DS and DI are the caller's values
  from entry. Only AX is the result (AX = bytes written, set at lines 226-227). Say that SI, ES, DS and
  DI are preserved and that AX is the only result, and drop the claim about the source end and the output
  end.

- [ ] must-fix src/E19FC.ASM:14 (and the same citation in `re/renames.toml`, the WHY of the
  `19FC:0008=CompressLcw` rename, line 703): "Unpack4 (1D9A:0013, src/E1B82.C:22)". Line 22 of
  src/E1B82.C is the closing `} WorkBuffer;` of the union. Unpack4 is declared at src/E1B82.C:39
  (`extern void Unpack4(char far *src, void far *dst, int len); /*@ 1D9A:0013 */`). Cite :39. The
  registry's WHY goes through `make rename` (or the rename entry in `re/renames.toml`, as the decompiler
  sees fit); the source line goes in src/E19FC.ASM.

- [ ] should src/E19FC.ASM:123: the comment "FEh fill: count, byte" is a little short of what is
  emitted. The code writes FEh (stosb at 124), then the count as a word (xchg/stosw at 125-126), then the
  byte (mov al,ch / stosb at 127-128). The header (lines 18-19) says "a count word then the byte".
  Write "FEh fill: count word, byte" so the comment matches the layout.

- [ ] note src/E19FC.ASM:43-46: "the map marks a code segment's first byte as a function, as in 1BA8:0005
  and 1BF3:000D" states a convention that is not shown here. The evidence that settles 19FC:0007 is in the
  segment table: the unit at 19F3:000E is 137 bytes (segment 27 of `build/re/eob.json`, start 106302),
  so it ends at 19F3:0097, which is linear 106439. That is the byte 19FC:0007 (19FC is para 6652, linear
  106432, so 19FC:0000..0006 are the same linear bytes as 19F3:0090..0096, the 19F3 unit's last bytes).
  So the 00 at 19FC:0007 is the first byte after the 19F3 unit, and it is the 19FC segment's first byte in
  the table. Saying so would tell the next reader why the marker is data and where the segment starts.

- [ ] note src/E19FC.ASM:92-95 and the header: the encoder writes the first literal (81h, then one source
  byte) before it tests the length, so a length of 0 does not give an empty stream; it gives a stream that
  copies bytes at and past the source start. Read from the code, not run. The header says nothing about
  the length (it says a caller keeps both buffers inside 64 KiB). Add "the length must be at least 1" to
  the limits at lines 36-41.

- [ ] note src/E19FC.ASM:157-159 and 116-126: the match length is one short when a run reaches the end of
  the source. After `repe cmpsb` (or `repe scasb` for a fill) runs out of count, DI is one past the last
  compared byte, not on a mismatch, but `ax = di - bx` (and `dec di`) assume a mismatch, so the length is
  n - 1 for a match that runs to bp. The output is still correct, since the last byte becomes a literal or
  a shorter match. The original keeps the same bytes, so this is the original's behaviour, not a
  decompile error; the header says nothing that would contradict it, but a reader who expects a maximal
  match should be told. Read from the code, not run.

## Summary

The unit reads well from its source and header: the token forms, the limits, the state words and the
candidate test are stated, and each address the comments give matches the disassembly and the segment
table. The name `CompressLcw` claims what the code does (an LCW encoder, the inverse of Unpack4) and
nothing about its role in the game; the byte search finds no caller, so that is correctly left open. The
two must-fix items are both wrong claims: the return-state comment at lines 61-62 says registers hold
values the epilogue restores to the caller's, and the Unpack4 citation points at the wrong line (E1B82.C:22
instead of :39). Neither changes a byte. The shoulds and notes are wording and edge cases. The names
agree with `src/` (`_NormFar`, `Unpack4`, the `g` state words) and the whitepaper (4.2). I did not edit
`src/`; the findings are the decompiler's to fix.

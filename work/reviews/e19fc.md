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

- [x] must-fix src/E19FC.ASM:61-62: the function comment says "on return DS:SI points at the source end
  and ES:DI just past the output". The code does not leave those registers that way: the epilogue pops
  si, es, ds and di (src/E19FC.ASM:228-233), so on return SI, ES, DS and DI are the caller's values
  from entry. Only AX is the result (AX = bytes written, set at lines 226-227). Say that SI, ES, DS and
  DI are preserved and that AX is the only result, and drop the claim about the source end and the output
  end. Fixed in 081be14: the comment now says the epilogue restores SI, ES, DS and DI to the caller's
  values, so AX is the only result.

- [x] must-fix src/E19FC.ASM:14 (and the same citation in `re/renames.toml`, the WHY of the
  `19FC:0008=CompressLcw` rename, line 703): "Unpack4 (1D9A:0013, src/E1B82.C:22)". Line 22 of
  src/E1B82.C is the closing `} WorkBuffer;` of the union. Unpack4 is declared at src/E1B82.C:39
  (`extern void Unpack4(char far *src, void far *dst, int len); /*@ 1D9A:0013 */`). Cite :39. The
  registry's WHY goes through `make rename` (or the rename entry in `re/renames.toml`, as the decompiler
  sees fit); the source line goes in src/E19FC.ASM. Fixed in 081be14: src/E19FC.ASM:2 and the
  re/renames.toml WHY now cite src/E1B82.C:39, and the WHY records that :22 was the union's closing
  brace.

- [x] should src/E19FC.ASM:123: the comment "FEh fill: count, byte" is a little short of what is
  emitted. The code writes FEh (stosb at 124), then the count as a word (xchg/stosw at 125-126), then the
  byte (mov al,ch / stosb at 127-128). The header (lines 18-19) says "a count word then the byte".
  Write "FEh fill: count word, byte" so the comment matches the layout. Fixed in 081be14: the comment
  now reads "FEh fill: count word, byte".

- [x] note src/E19FC.ASM:43-46: "the map marks a code segment's first byte as a function, as in 1BA8:0005
  and 1BF3:000D" states a convention that is not shown here. The evidence that settles 19FC:0007 is in the
  segment table: the unit at 19F3:000E is 137 bytes (segment 27 of `build/re/eob.json`, start 106302),
  so it ends at 19F3:0097, which is linear 106439. That is the byte 19FC:0007 (19FC is para 6652, linear
  106432, so 19FC:0000..0006 are the same linear bytes as 19F3:0090..0096, the 19F3 unit's last bytes).
  So the 00 at 19FC:0007 is the first byte after the 19F3 unit, and it is the 19FC segment's first byte in
  the table. Saying so would tell the next reader why the marker is data and where the segment starts.
  Fixed in 081be14: the header now cites segment 27 of build/re/eob.json (start 106302, 137 bytes,
  ends 106439 = 19FC:0007) and drops the unshown convention claim.

- [x] note src/E19FC.ASM:92-95 and the header: the encoder writes the first literal (81h, then one source
  byte) before it tests the length, so a length of 0 does not give an empty stream; it gives a stream that
  copies bytes at and past the source start. Read from the code, not run. The header says nothing about
  the length (it says a caller keeps both buffers inside 64 KiB). Add "the length must be at least 1" to
  the limits at lines 36-41. Fixed in 081be14: the limits paragraph now requires a length of at least 1
  and says a length of 0 copies bytes at and past the source start.

- [x] note src/E19FC.ASM:157-159 and 116-126: the match length is one short when a run reaches the end of
  the source. After `repe cmpsb` (or `repe scasb` for a fill) runs out of count, DI is one past the last
  compared byte, not on a mismatch, but `ax = di - bx` (and `dec di`) assume a mismatch, so the length is
  n - 1 for a match that runs to bp. The output is still correct, since the last byte becomes a literal or
  a shorter match. The original keeps the same bytes, so this is the original's behaviour, not a
  decompile error; the header says nothing that would contradict it, but a reader who expects a maximal
  match should be told. Read from the code, not run. Fixed in 081be14: the header carries QUIRK
  19FC:007E / 19FC:00D7 with the one-short case and why the output stays correct, and the two string
  instructions point at it.

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

---

## Round 5

Reviewer: rev-haiku
Commit reviewed: b46994c (main; the review/e19fc worktree is at the same commit). `src/E19FC.ASM` is
the round 4 version (7c4d343).

Playbook 70. Judge what the file can still get wrong; the bytes were proved by `make match`.

### What I checked (read-only; no game run; no source edited)

- `make match FILE=src/E19FC.ASM`: 2 of 2. `make readability UNIT=e19fc`: 0 findings. Not re-proved.
- The caller, which the header says does not exist. `make packet UNIT=e19fc` lists `ovl3:0FD1 [eov3]`.
  I checked it against the bytes: `ovl3:1033` is `9A 08 00 E0 00` (lcall 0E0h:0008); `build/re/eob.json`
  overlay 3 has a fixup at `ovl3:1036`; `segments[28]` (0E0h = 28 x 8) is para 6652 = 19FC, start 106439.
  The same single instance is in the FBOV copy of overlay 3 appended to `eob/EOB.EXE` (file offset
  0x39D80, identical bytes; the call is at 0x3ADB3). No other `08 00 E0 00` occurs in the overlays or
  the EXE, and no absolute `08 00 FC 19` or `9A 08 00 FC 19` occurs anywhere.
- The caller's arguments (`ovl3:1021` to `1038`): `mov ax,1000h; push ax` (a length of 4096), then
  `push [bp+8]`, `push [bp+6]` (the caller's destination far pointer), `push [ds:DCFE]`, `push [ds:DCFC]`
  (the source). It stores AX at `[bp-0Ah]` (`ovl3:103B`). `ds:DCFC` is `gBufDCFC` (src/E07A9.C:3, :21:
  a 4096-byte start-up allocation).
- Prologue and epilogue: the pushes (E19FC.ASM:77-85) and the pops in reverse (240-247) restore every
  register except AX.
- The segment boundary: the 19F3 unit ends with `cb` (retf) at 19F3:0096 (linear 106438), and the byte
  at 19FC:0007 follows it directly.
- The token forms against the code: the literal count starts at 80h and is incremented to at most BFh
  (207-214); the short form (188-194); the absolute copies (196-202, 219-223); the fill (134-141); the end
  byte (236-237). Each matches the header.
- The Unpack4 citation: `grep -n Unpack4 src/E1B82.C` gives line 40 (the declaration), not 39 (see the
  first finding below).

### Findings

- [ ] must-fix src/E19FC.ASM:2 and re/renames.toml (the WHY of `19FC:0008=CompressLcw`): the Unpack4
  citation "src/E1B82.C:39" is off by one. `grep -n Unpack4 src/E1B82.C` gives line 40, the declaration
  `extern void Unpack4(char far *src, void far *dst, int len); /*@ 1D9A:0013 */`. Line 39 is Unpack2.
  Round 4 changed :22 to :39 and the check did not catch the second slip. Cite :40 in both places. (The
  rename WHY is in `re/renames.toml`; it goes through `make rename` or the registry entry, as the
  decompiler sees fit.)

- [ ] must-fix src/E19FC.ASM:31-34: "No caller was found" and "the packet reports no direct call" are
  wrong. The caller is `ovl3:0FD1` (placeholder `eov3_0fd1`, no src/ file yet): the call at `ovl3:1033`
  is relocated by the overlay fixup at `ovl3:1036` to segment index 28, which is 19FC, so it reaches
  19FC:0008 (evidence: `build/re/eob.json` overlay 3 fixups and segments[28]; the FBOV copy at
  `eob/EOB.EXE` 0x3ADB3). A byte search for `9a 08 00 fc 19` misses it because an overlay stores the
  segment index, not the paragraph. `tools/agent/packet.py` (`Program.target`, lines 60-77) resolves the
  fixup, so the packet's caller list is right. Say this in the header, cite `ovl3:1033` and `eov3_0fd1`,
  and drop the note that the packet checks direct calls only.

- [ ] must-fix src/E19FC.ASM:40-43: "pass a length of at least 1" is wrong; the minimum is 2. The first
  literal is written at 104-107, and then `mainloop` (108) runs with no length test; the only end test is
  at `loopend` (231-234). With length 1, the first search reads the byte past the end (117) and encodes
  it as a second literal, so the stream 82h, src[0], src[1], 80h decodes to 2 bytes. If those two bytes
  are equal, the match scan (162-172) computes `ax = di - bx = -1`, stores gBestLen = FFFFh, and `si`
  wraps back to the source start, so the encoder does not stop at the end test. Length 2 is correct,
  because the first mainloop pass is at start+1, which is below the end. This is from reading the code,
  not a run. Change the limit to "at least 2" and say what a shorter length does (or that the caller must
  not pass one).

- [ ] should src/E19FC.ASM:73-74: "the epilogue restores SI, ES, DS and DI ... so AX is the only result"
  is true but names four registers where the epilogue restores every register except AX (the pops at
  240-247 also restore DX, CX, BX and BP). Say that.

- [ ] should src/E19FC.ASM:1-9: "which the C compiler does not emit for C source" is a claim about
  Turbo C++ 1.00 output made from recall. Ground rule 1 says how the compiler treats an idiom is settled
  by compiling it. Either tag it [H] or compile one sample if the toolchain is installed (`make tc-check`).
  Line 6, "The unit is written in assembly", is the same kind of claim; tag it [H] too.

- [ ] should src/E19FC.ASM:14-29: the token meanings cite `tools/re/wwfmt.py lcw` as [H]. WHITEPAPER 4.2
  says the decoder Unpack4 (1D9A:0013) matches the oracle byte for byte on all 131 format-4 files
  (`tests/unit/test_wwfmt.py`). Cite that for the decoder side of the token set. The encoder side is
  still unverified, and the header already says no round trip has run.

- [ ] should src/E19FC.ASM:39-41: the sentence "All offsets are 16 bits ... and add bp,dx (end of the
  source) wraps at 10000h when offset + length passes FFFFh" is a fragment. It means that bp = start +
  length (98-99) is a 16-bit sum that wraps at 10000h. Rewrite it as that.

- [ ] should src/E19FC.ASM:65-68: "data, not code" is right in that nothing falls through to 19FC:0007 (the
  19F3 unit ends in retf, 19F3:0096), but the header gives no reason. Add "the byte after the 19F3 unit's
  retf, before the encoder's push bp (19FC:0008)".

- [ ] note: the caller is the lead the header's "the game role stays unknown" needs. `ovl3:0FD1` compresses
  one 4096-byte block from `gBufDCFC` (ds:DCFC) per call, and it also calls 1BA8:0006 (Unpack0, src/E1B82.C:36,
  `ovl3:108F`) and an indirect call through `ds:80CA` (`ovl3:106C`, named gDrawText in src/E1AA6.C:7,
  not checked here). Its role is for the eov3 review. The header can state the block size and the source
  without naming the caller's role.

### Summary

The unit is understandable from its source, and the bytes and the token forms check out. The round 5
must-fix items are three wrong claims: the unit has a caller (`ovl3:0FD1` through an overlay fixup), so
"no caller" is false; the stated minimum length of 1 is too small, since the encoder needs at least 2; and
the Unpack4 citation is off by one line (:40, not :39). None changes a byte, and the names stay as they are. The shoulds are wording and evidence
(the epilogue list, the [H] tags, the wwfmt versus Unpack4 citation, a garbled sentence, the marker's
reason). The names agree with `src/` (`_NormFar`, `Unpack4`, the `g` state words, `gBufDCFC`). I did not
edit `src/`; the findings are the decompiler's to fix.

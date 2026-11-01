# Review: e19fc

Reviewer: rev-haiku
Commit reviewed: 7e40b74 (main when I started; src/E19FC.ASM is last changed in 7dda755 and is the same on
29a673f, the review/e19fc worktree HEAD)

Playbook 70. Judge what a matching file can still get wrong: names, types, comments, structure.
The matcher already proved the bytes; do not re-prove them, read for meaning.

This is a third round. The earlier rounds are in git history: round 1 (rev-sonnet, e1429a0) and round 2
(rev-haiku, 4f54830); their findings were fixed in fe3d880 and 7dda755 and ticked in b09f362 and bceb400.
This round reads the current file against the bytes and the other units.

What I checked (read-only; no game run; no source edited):

- Every code label and state word address in the comments matches the disassembly of `eob/EOB.EXE`
  (`tools/re/disasm.py`, segment 19FC): 0053 mainloop, 006F search, 00B4 noRun, 00B6 nextcand, 00F5
  notfound, 0129 absCopy, 0139 litrun, 0141 newlit, 0149 putlit, 0162 longmatch, 016A storeoff, 0173
  emitoff, 0180 loopend, 0187 finish; state words 019B gLitOpen, 019D gOutBase, 019F gInBase, 01A1
  gLitHdr, 01A3 gOutSeg, 01A5 gOutPtr, 01A7 gBestLen, 01A9 gBestPos (the cs: operands in the code agree).
- The byte search the header cites is real: `eob/EOB.EXE` and the seven images in `build/re/eob-overlays`
  contain no `9a 07 00 fc 19`, `9a 08 00 fc 19`, `07 00 fc 19` or `08 00 fc 19`.
- The token forms in the header match `tools/re/wwfmt.py` `lcw` (short back reference, literal run,
  absolute copy `out[pos + k]`, fill, long absolute copy, end at 80h), and that model is checked against
  Unpack4 (1D9A:0013) on all 131 format 4 files by `tests/unit/test_wwfmt.py` (whitepaper 4.2).
- Names agree with `src/`: `_NormFar` (E1BA8.ASM, `re/renames.toml`), `Unpack4` (E1B82.C:37), the state
  words with their `g` prefix. The term LCW is the whitepaper's (4.2, "format 4 (LCW)").
- `make readability UNIT=e19fc`: 0 findings. `make match FILE=src/E19FC.ASM`: 1 of 1, 420 bytes.

## Findings

Each finding is a checkbox. `must-fix` blocks `make review-pass` until a decompiler ticks it off
(with the commit that fixes it); `should` and `note` do not block.

- [ ] must-fix src/E19FC.ASM:44-46: `_CompressLcw PROC FAR` begins with `db 0`, so the entry the name and
  the PUBLIC label give (19FC:0007) is the marker byte, not code. In `eob/EOB.EXE` the bytes at 19FC:0007
  are `00 55 8B` (the disassembly is `add byte ptr [di - 75h], dl`), and the first instruction, `push bp`,
  is at 19FC:0008. A far call to 19FC:0007 would run the marker. The same kind of marker is handled
  differently in the other units: `src/E1BF3.ASM:19-21` (`_f1BF3_000D LABEL BYTE`, a public byte symbol,
  its header lines 8-10 say "data, not code", the functions follow it) and `src/E1BA8.ASM:12`
  (`_f1BA8_0005 LABEL BYTE`, then `_Unpack0 PROC FAR` at 0006). Fix it the same way: the marker is its own
  public byte symbol with a comment that it is data, and `CompressLcw` starts at 19FC:0008 with the same 419
  code bytes (the unit stays 420 bytes; the map's start at 0007 is the marker). The header line 35-36 wording
  "a segment-start byte the map gives to this unit" then says that the byte is data. The registry
  (`make rename`, `re/symbols.toml`, `re/renames.toml` for 19FC:0007) has to follow the new entry address;
  `make match` must stay green.

- [ ] should src/E19FC.ASM:1-3 (unit comment): no whitepaper section. READABILITY.md (Unit) asks for "the
  whitepaper section", and the rename WHY in `re/renames.toml:433` already cites "whitepaper 4.2"
  (`docs/design/WHITEPAPER.md`, "### 4.2 Compressed files"). Add it to the header.

- [ ] should src/E19FC.ASM:14-16: "the decoder adds it to its output base, the same index, because the
  decompressed prefix has the same length" is hard to follow. What holds: the absolute copy word is an
  index from the start of the decompressed output (`tools/re/wwfmt.py` `lcw`, `out[pos + k]`; Unpack4
  agrees on all 131 files in `tests/unit/test_wwfmt.py`), and the encoder measures it from the input start
  (gInBase), so the two agree because the decoded prefix has the same length as the input prefix. Say that.

- [ ] should src/E19FC.ASM:124-127 and 138-139: the quick reject (compare the byte at gBestLen-1 before
  extending) and the `jb search` that drops a shorter candidate carry no reason. Say that a candidate can only
  beat gBestLen if it agrees at offset gBestLen-1, and that an equal length replaces the earlier match
  because the later candidate is nearer (a smaller distance, which can use the 2-byte short form).

- [ ] note src/E19FC.ASM:1: the map gives `kind asm [H]`, and the header does not say why this unit is
  assembly and not C (`src/E1B82.C:2-9` says why its unit is C++). If the reason is known (the cs: overrides
  and the repe/repne string code), say it; if not, say that it is not known.

- [ ] note src/E19FC.ASM:2: `CompressLcw` does not say that the function returns the byte count. The header
  does (line 6, "returning the number of bytes written"); that is enough for a name.

## Summary

The unit is understandable from its source and its header: the token forms, the limits and the state words are
stated, and each address the comments give matches the bytes (checked above). `CompressLcw` is backed by the
tokens it writes and by the decoder it inverts (`Unpack4`, 1D9A:0013); its game role is stated as unknown, and
the byte search finds no caller in `EOB.EXE` or the overlays, so the name says what the code does and nothing
about its role. The one blocker is the entry: the PROC begins with the marker byte at 19FC:0007, and the code
starts at 19FC:0008, so the name sits on data. The fix follows the convention of `E1BF3.ASM` and `E1BA8.ASM`
and changes the registry's address, so it is the decompiler's to make. I did not edit `src/`. Otherwise the
names agree with `src/` and with the whitepaper, and `make readability UNIT=e19fc` passes with 0 findings.

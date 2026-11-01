# Review: e19fc

Reviewer: rev-haiku
Commit reviewed: f0aff11 (main when I started; src/E19FC.ASM is last changed in fe3d880, which is in review/e19fc)

Playbook 70. Judge what a matching file can still get wrong: names, types, comments, structure.
The matcher already proved the bytes; do not re-prove them, read for meaning.

This is a second round. The previous one (rev-sonnet, commit e1429a0; its items ticked in b09f362)
is in git history: `git show b09f362:work/reviews/e19fc.md`. Its must-fix (`litCount` named a pointer)
and its three should items are fixed in fe3d880, and I re-checked them: the state word is `litHdr`
(src/E19FC.ASM:213, "output offset of the open literal run's count byte"), the header has the limits
paragraph (line 21), the comment at line 79 says "precheck", and each of the eight state words has a
role. Its notes stand. I overwrote this file in my first commit without reading it first, which the
history above recovers.

What I checked against the bytes and the other units (read-only, no game run):

- The label and state-word addresses in the comments are right. In `EOB.EXE` at the prologue of
  19FC:0007, 19FC:0053 writes `cs:[01A5]` (outPtr), 19FC:0139 reads `cs:[019B]` (litOpen),
  19FC:00F5 reads `cs:[01A7]` (bestLen), 19FC:016A reads `cs:[01A9]` (bestPos).
- The token forms in the header match the LCW model in `tools/re/wwfmt.py` (`lcw`, the decoder the
  whitepaper and `tests/unit/test_wwfmt.py` check against Unpack4): 00h..7Fh distance counted back
  from the current position; 81h..BFh literal counts; C0h..FDh count (len-3) and an absolute
  offset; FEh count word then byte; FFh length word then absolute offset; 80h ends the stream.
- The normaliser is `NormFar` (1BA8:0081, `src/E1BA8.ASM`, `re/renames.toml`). The decoder at
  1D9A:0013 is `Unpack4` (`src/E1B82.C:22`).

## Findings

Each finding is a checkbox. `must-fix` blocks `make review-pass` until a decompiler ticks it off
(with the commit that fixes it); `should` and `note` do not block.

- [ ] must-fix src/E19FC.ASM:212: the comment on `inBase` says "distances are relative to it". That is
  true only for the absolute forms (C0h..FDh and FFh). The storeoff path subtracts it (19FC:016A..016E,
  `mov ax,cs:[bestPos]` / `sub ax,cs:[inBase]`). The 00h..7Fh form does not use it: its distance is
  `si - bestPos` (19FC:010E..0110, `mov ax,si` / `sub ax,cs:[bestPos]`), counted back from the
  current position, as the header says on line 10. Fix the comment to say what it is for, e.g.
  "input start (normalised offset); the absolute copy offsets (C0h..FDh, FFh) are measured from it".
- [ ] should src/E19FC.ASM:10 and :12 (header): "back reference" for C0h..FDh and FFh. These forms
  copy from an absolute output offset (the distance is from inBase, not from the current position).
  The whitepaper (4.2) calls them "absolute copies" and `tools/re/wwfmt.py` `lcw()` reads
  `out[pos + k]`. Say "absolute copy" so a reader does not take them for relative back references.
  The header's distance sentence (line 13) is right.
- [ ] should src/E19FC.ASM:36 (`_f19FC_0007`): the placeholder has no `Purpose unknown:` in its
  comment, so R2 passes only because the finding is in `work/readability-baseline.txt`. What the bytes
  show is known: an LCW encoder, the inverse of Unpack4 (the token set above). Name what it does, e.g.
  `make rename OLD=_f19FC_0007 NEW=CompressLcw WHY="LCW encoder: writes the tokens Unpack4 (1D9A:0013) reads; wwfmt.py lcw, whitepaper 4.2"`.
  The game role stays unknown (no direct caller, packet: "(no direct call found)"), and the header
  should say so in one sentence. The previous round accepted the placeholder ("LCW encoder for format 4"
  is the only evidence); the bytes now support the verb, so I list it as a should, not a blocker.
- [ ] should src/E19FC.ASM:1-27 and :212: the decoder is named "the loader's format-4 decoder
  (1D9A:0013)". Its name in `src/E1B82.C:22` and in the whitepaper is `Unpack4`. Use that name.
- [ ] should src/E19FC.ASM:210-217: the eight state words have no `g` prefix (litOpen, outBase, inBase,
  litHdr, outSeg, outPtr, bestLen, bestPos). READABILITY.md: globals are `g` + PascalCase
  (`src/E1990.ASM` has `gMemTopSeg`). These are the encoder's state, written and read by the function
  and kept in the code segment. Rename them with `make rename` (gLitOpen, gOutBase, gInBase, gLitHdr,
  gOutSeg, gOutPtr, gBestLen, gBestPos), each with its address as the evidence.
- [ ] should src/E19FC.ASM:154 (label `match2`): the name says nothing about the token. The path is
  taken by `ja match2` on lines 142 and 146 (lengths over 0Ah or distances over 0FFFh) and writes the
  C0h..FDh form (lines 154..161). Name it for that (e.g. `absCopy`); `longmatch` (line 178, the FFh
  form) is already clear.
- [ ] should src/E19FC.ASM:19 (header): "no far pointer anywhere in EOB.EXE or its overlays". The
  previous round searched `EOB.EXE` for the far call bytes `9a 07 00 fc 19` and `9a 08 00 fc 19` and for
  bare far pointers, with no hit, and said the overlay code was not searched. The packet tool itself
  checks direct calls only (`tools/agent/packet.py:149` prints "(no direct call found)"). So the claim
  has evidence for `EOB.EXE` and none for the overlays. Say that, or search the overlays and cite it.
- [ ] should src/E19FC.ASM:59-60 and the limits paragraph (lines 21..23): the encoder uses 16-bit
  offsets from the normalised start, and `add bp,dx` (end of the source) wraps if offset + length passes
  FFFFh. The output has the same limit. The header's "limits" list does not mention this. A caller must
  keep both buffers inside 64 KiB from their normalised starts. Add that line.
- [ ] note src/E19FC.ASM:26-27: "the alignment byte between the previous module (19F3) and this one". In
  `re/units.toml` 19FC:0007 is this unit's start, and 19F3 (137 bytes from 19F3:000E) ends at 19FC:0006.
  The byte is a segment-start byte of the same kind as 1BA8:0005 (`src/E1BA8.ASM`), so the wording "between
  the modules" is loose. Not a claim I could check beyond the map; say "a segment-start byte the map
  gives to this unit" if you touch the header.
- [ ] note No test runs this encoder. `tests/unit/test_wwfmt.py` checks the decoder (Unpack4 in the
  oracle) and `wwfmt.lcw`. The header's claim that the emitted tokens are the ones the decoder reads is
  therefore [H] (read from the bytes and the decoder model), not [V]. A round trip
  (`wwfmt.lcw(encode(x), len(x)) == x` on the pinned data, with the encoder run in the oracle, playbook
  25) would turn it into evidence.

## Summary

The unit is understandable from its source: the header states the token forms and the limits, and I
checked the forms and the label addresses against the bytes and against `wwfmt.py`'s LCW model, and
they hold. What is not yet right is the data comment on `inBase` (wrong for the short form), the
name: `_f19FC_0007` is a placeholder that the bytes support naming `CompressLcw` (what it does;
its game role is unknown and the header should say so), and a few terms and conventions:
"back reference" for absolute copies, `Unpack4` for the decoder, `g` prefixes on the state words,
and the label `match2`. The only must-fix is the `inBase` comment, so the review fails and the
decompiler fixes it. For the name, either `CompressLcw` (the evidence is strong for what it does) or
`Purpose unknown:` with the header sentence on the missing game role would pass a second review.

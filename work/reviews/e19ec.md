# Review: e19ec

Reviewer: rev-sonnet
Commit reviewed: 8afc719b2730588a3f7f5fe83f8ce0e7a71e447e (first pass); re-review at 36d77ba6058a5e4014be8869973523d717099b8b

Playbook 70. Judge what a matching file can still get wrong: names, types, comments, structure.
The matcher already proved the bytes; do not re-prove them, read for meaning.

## Findings

Each finding is a checkbox. `must-fix` blocks `make review-pass` until a decompiler ticks it off
(with the commit that fixes it); `should` and `note` do not block.

- [x] must-fix src/E19EC.C:1-4: the header calls the entries of ds:FC0C "object arena" segments and says ObjAlloc is "selected by 1-based kind". Neither is backed by the bytes. (a) The index is `kind | 1` (19EC:0008 `or bx,1`), so kinds 2k and 2k+1 select the same entry; that is a pair/odd-slot rule, not 1-based indexing (a 1-based kind would use `kind - 1`). (b) The table is filled in `main`, by a switch on the video mode ds:8102 (jump table cs:0580, 07C8:0291..03FD). The entries are screen and off-screen page segments: constants 0xB800 or 0xA000 (video RAM) and 0xA200/0xA400/0xA800, or segments returned by MemAlloc (19BC:0007) of 0xFA00, 0x7D00, 0xCB20 or 0xFDE8 bytes (07C8:02A4..03D9, writes to ds:FC0C..FC1A). In the first mode block both [0] and [1] are 0xB800 and [6], [7] are the same allocation. So the file should say "page/buffer segment table filled per video mode at 07C8:02A4..", and give this as the evidence, not "arena". Fixed in 195b005.
- [x] must-fix src/E19EC.C:13 (name `ObjAlloc`): the function allocates nothing; it reads a segment from the table and returns seg:0 (19EC:0002..0015, no call). "Alloc" and the E07A9.C globals `gObjDCA6`/`gObjDF4E`/`gObjEFC2` ("named by its callers") are circular: the callers took the name from this function and nothing independent supports "object". Per the playbook a confident name without evidence is a must-fix; rename to a placeholder (`f19EC_0002`) or to something the evidence supports (for example `PageSeg`, "far pointer to the screen/buffer page segment for slot kind|1"). Callers in E07A9.C:36 and E1B82.C:15 follow whichever name is chosen. Renamed to `f19EC_0002`; the extern declarations and call sites in E07A9.C and E1B82.C follow. Fixed in 195b005.
- [x] should src/E19EC.C:7-9, 28: `f19EC_0016` is the even twin (`kind & ~1`, 19EC:001C `and bx,0FFFEh`) of the odd lookup; say so, and that it has no direct caller found (it may be reached from overlays or be dead). Comment added. Fixed in 195b005.
- [x] should src/E19EC.C:23-27: "transfers between two arenas" is a guess. What the bytes show: both slots must be non-zero and the two segments different (19EC:0039, 0044, 0057), and then the far pointer at ds:8046 is called with (segA, segB, c, d) (19EC:005D..0073), cdecl, caller pops 8. "Copy/blit between two page segments if they differ" is consistent with the table contents above, but ds:8046 was not traced to its target (its initialiser is not a direct store in the image; probably copied from a per-mode handler table), so keep it tagged [H] and name c, d as unknown. "arena" wording dropped, ds:8046 kept [H], c and d noted unknown. Fixed in 195b005.
- [ ] note ds:8046 sits among the other per-mode far handlers used in src/ (ds:8052 gDrawRect in E19F3.C, ds:80D2 gPrint80D2 in E07A9.C); that supports "indirect call through a per-mode driver table".
- [ ] note The unit's own start 19EC:0002 is preceded by `pop bp; retf` at 19EC:0000, which is not part of the unit (padding or the tail of the previous unit); not an issue.
- [ ] note ds:FC1A (slot 7) is passed to f1B82_0217 (SetUnpackBuffer) with size 0xFC00 in E07A9.C:64, and ObjAlloc(7) is the same segment at offset 0, stored in gObjDCA6: so the unpack work buffer is the slot 6/7 allocation. Useful when naming gObjDCA6.
- [ ] note Types: `unsigned gSegsFC0C[]` is right (16-bit segments, scaled index `shl bx,1`, base ds:FC0C = `[bx - 0x3F4]`); the table has at least 8 entries (index 7 is used). `kind | 1` and `kind & ~1` are the C that compiled to these bytes; `int` is fine.

## Summary

The code is three tiny far functions and matches; the C is clear. The prose is not: the header and the name `ObjAlloc` claim an "object arena" and a 1-based kind, which the bytes contradict. The table at ds:FC0C is a per-video-mode list of screen and off-screen page segments filled in `main` (07C8:02A4..03D9), indexed with the low bit forced on or off. Two must-fix items (header comment, the confident name `ObjAlloc`); both are in the review above, with the evidence. The other two functions are properly placeholder-named. Review failed until the header and name are corrected.

Decompiler response (195b005): both must-fix items and the two should items are addressed. Header now describes the per-video-mode page/buffer segment table with the 07C8 fill addresses; `ObjAlloc` is renamed `f19EC_0002` (placeholders elsewhere unchanged); the extern declarations and call sites in E07A9.C and E1B82.C follow the rename. `make match FILE=src/E19EC.C` exits 0 (3 of 3); E07A9.C and E1B82.C still match.

## Re-review (36d77ba)

Reviewer: rev-sonnet. Rechecked the fixes against the bytes (tools/re/disasm.py eob/EOB.EXE 19EC and 07C8).

- Match: `make match FILE=src/E19EC.C` gives 3 of 3, unit complete (124 bytes). Callers in E07A9.C (60..62) and E1B82.C (42, 43) use `f19EC_0002`; re/symbols.toml names agree; no `ObjAlloc` left in src/.
- Header: slot rule (`or bx,1` at 19EC:0008, `and bx,0FFFEh` at 19EC:001C), the pairs written with one segment (07C8:02A7/02AA, 02C0..02C6 via `mov ax,dx` at 02C4, 02DC..02E2, 02F8..02FE), constants 0xB800/0xA000/0xA200/0xA400/0xA800 and the MemAlloc (19BC:0007) sizes 0xFA00/0x7D00/0xCB20/0xFDE8 all check out. The jump on ds:8102 (cmp bx,4 at 07C8:0295, `jmp cs:[bx+580h]` at 029F) is right.
- f19EC_002A: arguments pushed d, c, seg[b], seg[a], `lcall [8046]`, `add sp,8` (19EC:005D..0077), so the call is handler(seg[a], seg[b], c, d) as written. The [H] tag and the unknown c, d are honest.

- [ ] should src/E19EC.C:5-8: the fill list is incomplete: the mode 4 block also stores the constant 0xAC00 into ds:FC18 (07C8:03E0) and MemAllocs 0xFC00 bytes for ds:FC1A (07C8:03EC; E07A9.C:64 passes gFC1A with 0xFC00 to f1B82_0217). Add them to the header list.
- [ ] note The mode 1 block (07C8:0304..0315) stores `si` (not a constant) into ds:FC10 and ds:FC12; si is not traced, so the header's "screen segments and page segments" is a description of the usual case only.

No open must-fix. Review passed.

# Review: eov4

Reviewer: rev-haiku
Commit reviewed: 467a5b5 (main, when review started; unit complete at e03cf0a)

Playbook 70. Judge what a matching file can still get wrong: names, types, comments, structure.
The matcher already proved the bytes; do not re-prove them, read for meaning.

Evidence used: `make match FILE=src/EOV4.C` (18 of 18), `make readability UNIT=eov4` (0 findings), `build/packets/eov4.md`, the other files of `src/` (grep), and a capstone disassembly of `build/re/eob-overlays/ovl04.bin` (16-bit, offset = ovl4 offset) to check every call-site address cited in a comment. Call sites found: ComputeViewSpan 1E3C (from DrawViewSlots); Ovl4_1091 1E50; Ovl4_0AD0 1E8C; Ovl4_1411 1EA2; Ovl4_0D1B 1EB9; Ovl4_19C4 1EBF; Ovl4_1D20 1ECB; Ovl4_044B 1F5D (from DrawViewFrame); DrawViewRows 202F; DrawViewSlots 1F75; ComputeViewSpan also from Ovl4_0AD0 at 0C41 and 0CFB.

## Findings

Must-fix: a wrong claim, or a name or citation with no evidence.

- [ ] must-fix src/EOV4.C:58: ComputeViewSpan says `left`/`right` receive the span in "2-pixel columns". They are 8-pixel columns: SetRegionRect (:38-41) takes 8-pixel columns and ComputeViewSpan passes `*left`/`*right` straight to it (:117); VIEW_RIGHT (:51) is 0x16 columns = 0xb0 pixels; Ovl4_1411 shifts the ds:7106/ds:7108 pixel bounds right by 3 (:846-847).
- [ ] must-fix src/EOV4.C:61-64: the ComputeViewSpan header ties ADJUST_SKIP to a zero ds:1879 count. The code (:89-92) ignores a record when its ds:1879 count is zero (for any adjustment) and ignores ADJUST_SKIP (for any count). Say both conditions.
- [ ] must-fix src/EOV4.C:68-69: the QUIRK note is wrong. `pair` is a separate local set at :85; `j` is not the pair index before the loop (it is only the gViewSlots index in the `if` at :79-81). Inside the loop `j = (slot + i) * 2` (:103), and `slot` was already multiplied by VIEW_SLOTS (:86), so `j` is not `2 * slot`. Delete or rewrite the note.
- [ ] must-fix src/EOV4.C:66: "Called by Ovl4_1E18 at ovl4:1E23". The DrawViewSlots call is at ovl4:1E3C (ovl4:1E23 is inside the `jmp` at 1E22). The callers also include Ovl4_0AD0 at ovl4:0C41 and ovl4:0CFB, which the comment omits. Use the current name, DrawViewSlots, for the first caller.
- [ ] must-fix src/EOV4.C:254: Ovl4_044B "Called by DrawViewFrame at ovl4:1F6B". The call is at ovl4:1F5D; ovl4:1F6B is the operand of the `lcall` at 1F6A (_DrawTile).
- [ ] must-fix src/EOV4.C:675: Ovl4_1091 "Called by DrawViewSlots at ovl4:1E2E". The call is at ovl4:1E50; 1E2E is inside the `mov ax,5` at 1E2C.
- [ ] must-fix src/EOV4.C:792: Ovl4_1411 "Called by DrawViewSlots at ovl4:1E2E". The call is at ovl4:1EA2. (Same wrong address as the Ovl4_1091 header; both cannot be right.)
- [ ] must-fix src/EOV4.C:1046: Ovl4_1D20 "Called by Ovl4_1E18 at ovl4:1E2A". The call is at ovl4:1ECB; ovl4:1E2A is `mov si,ax`.
- [ ] must-fix src/EOV4.C:402-403: Ovl4_0AD0 says "After the list, when that span slot was set, the slot's own span is recomputed". The `ComputeViewSpan(slotReg, ...)` call (:468-469) is inside the `while` loop (the loop closes at :470), so it runs after each item (call at ovl4:0CFB, after the DrawRegion000F at 0CCF). Say it is per item.
- [ ] must-fix src/EOV4.C:951-954 and :947: Ovl4_19C4 says the cell +2 test is "the cell +2 modulo 8" and that `<< 3 >> 3` is needed. For a signed char in a 16-bit int, `(c << 3) >> 3` is exactly `c` (for -128..127 the shift stays in int), so the test compares the sign-extended cell byte with g725A[frame]. It is not a modulo 8. Fix the wording: the pair is a codegen form that keeps the value unchanged.
- [ ] must-fix src/EOV4.C:1163-1164: DrawViewFrame says that when `full` is set and gF442 is clear the function "blanks the view box and clips a rectangle". The gF442 branch (:1182-1186) only calls ClipRect. The blank (gFillRect over 0,0,0xb0,0x78 with pattern 0xc) is in the gFB45 branch (:1171-1176), which the comment does not describe.
- [ ] must-fix src/EOV4.C:1045: Ovl4_1D20 says "Column 0 is nudged by -4 on both axes". Only idx 0 (column 0, first of its two cells, :1068-1071) is nudged; idx 1 is not. Say "the first cell (idx 0)".
- [ ] must-fix src/EOV4.C:232-233: IsSlotLive's comment says "(the slot is a live map object)". Nothing in the unit or the cited code shows that; ComputeViewSpan reads the same ds:194B bit 3 as "carries ds:6CC6 bounds" (:55, :64, :102), and src/E1414.C:321 and src/EOV3.C:1010 read it with other meanings. Keep the name only with the evidence it has, or state what the function tests: "returns 1 when slot is nonzero and ds:194B bit 3 is clear".

Should: clearer, or a name that differs from the rest of `src/`.

- [ ] should src/EOV4.C:1-4: the file header says "The unit is still being lifted." The unit is complete (make match 18 of 18); drop the sentence.
- [ ] should src/EOV4.C:21-22 and :59: InitViewSlots "reset the slot to -1 in both halves of the view-slot array" and gViewSlots "(two words per slot)". The code (:32) sets all 36 words (words i and i+18 for i < 18), i.e. all 18 pairs; say that.
- [ ] should src/EOV4.C:416 and :468: `spanSlot` is set only inside the drawn-item branch (:437-440 region, `spanSlot = -1` at :437). For a first item that is skipped, the per-item check at :468 reads an indeterminate value; for later skipped items it reads the previous item's value. The original reads the same stack slot (the bytes match). Say so in the Ovl4_0AD0 comment.
- [ ] should src/EOV4.C:397-401: Ovl4_0AD0's "bit 3 of the flags also recomputes the span ... and pulls the object up 4 pixels" omits the condition at :451 (`sprite < 2 && frame != 0`), and the gate at :423-428 (slot 0x10 forces the flags to 0x80; the return needs a nonzero ds:1879 count and bit 7 clear) is only described as "gate the slot".
- [ ] should src/EOV4.C:670-676: Ovl4_1091 calls the face at ds:7018[slot*2] "the second face (slot*2)", but it is processed second (sideA at :715-716) while ds:7018[slot*2+1] is listed first in the same comment and processed first (:693). Name the faces by their index: "the face at slot*2 (drawn second)".
- [ ] should src/EOV4.C:789-790 (Ovl4_1411 header): "nudged by ds:718E/ds:7192 when the heading is below 0x1e". The code uses g718E (with the g714E base) below 0x1e and g7192 with no heading term at or above it (:831-834). Say which table each case uses.
- [ ] should src/EOV4.C:788-790: for actions 4..6 the comment says "then redraws region 5 at the column extent". The code fits region 5 once (:852, with ds:7106/ds:7108 and gF1/gF5); there is no column-extent call in 4..6. The final SetRegionRect (:907) fits the visible span for every case.
- [ ] should src/EOV4.C:1086 (DrawViewSlots header) and :40, :66, :1090, :1130: the comments name functions by placeholder names that no longer exist in this file: "Ovl4_0097" (ComputeViewSpan, :40 and :1086), "Ovl4_1E18" (DrawViewSlots, :66, :1046), "Ovl4_1F51" (DrawViewFrame, :1090, :1130). Use the current names.
- [ ] should src/EOV4.C:1137-1139: DrawViewRows "hides the pointer over the view box". The zone it hides (HidePointerZone 0x46, 0x79, 0xb0, 0xaf, :1138) is x 70 to 176, y 121 to 175, which is below the view box (DrawViewFrame hides 0,0,0xb0,0x78 for the box). Say "below the view box".
- [ ] should src/EOV4.C:761-762: gF1 cites "(src/EOV5.C)" and gF5 "(this unit)". src/E0EF3.C:395-397 names both as "the view region's top/height (src/EOV4.C)", and src/EOV5.C:222 also declares gF1. Use one owner and make the citations agree.
- [ ] should src/EOV4.C:13 and :156 (g1879): src/EOV3.C:60 names ds:1879 `gTable1879` (`unsigned char`, "the four parallel per-kind tables"). Rename to gTable1879 (`make rename`) and agree the signedness (here it is signed char). g1879 is also declared twice in this file (:13 and :156); the same for g194B (:12, :230), gA9C6 (:11, :1083), HidePointerZone and ShowPointerZone (:1124-1125, :1154-1155). Remove the second copies.
- [ ] should src/EOV4.C:1122: gFarA92A names ds:A92A, which src/EOV2.C:122 names `gIconTableA92A` (same type, `void far *[]`). Rename to match.
- [ ] should src/EOV4.C:11 and :1083 (gA9C6): cites "(src/E0ACD.C)", but src/E0ACD.C:78 says "an offset into a gB894 record (src/E0DE8.C)", src/E122E.C:100 says "the member row the mouse is over", and src/E1414.C:101 says "an offset added into gEF6A's targets" (the meaning used here). Cite src/E1414.C:101 and note that the other files disagree.
- [ ] should src/EOV4.C:748-760 and :759 (gFB5E): this file declares `struct SlotSize far *gFB5E[]` (the code dereferences `->width`, so the far pointer type is the one the bytes need). src/EOV2.C:155 and src/EOV3.C:143 declare `long gFB5E[]`. Tell those owners; keep the far-pointer type here.
- [ ] should src/EOV4.C:526-527: g1CD3 and g1CE3 have no comment. They are the two colour rows the colour callback is passed (src/E08E6.C:103-104, :719, :729-731). Add that, with the citation.
- [ ] should src/EOV4.C:915: DDRec m06 "low two bits index ds:7246 and ds:7262". ds:7262 is indexed by `m07 * 4 + m06` with the whole m06 (:1007, :1016), and the g7262 comment (:931) says "per record +6". Say that ds:7246 uses the low two bits and ds:7262 the whole byte.

Notes: for the next reader; they do not block.

- [ ] note src/EOV4.C:10 (gEF6A signedness): this file declares `signed char far *gEF6A[]`; src/E1414.C:100 declares `unsigned char`. The signed read is backed by src/E08E6.C:532 (`(signed char)gB894[...]` before indexing g194B). Also gB894 is `signed char` here, `unsigned char` in src/E08E6.C:85 and `char` in src/E0ACD.C:76.
- [ ] note src/EOV4.C:525 and :1153 (function-pointer types): gFn80D2 is `void (far *)(char far *map)` here, `void far *colors` in src/E07A9.C:249; gFillRect's fifth parameter is "pattern" here and in src/E0BD2.C:113 but "colour" in src/E082C.C:64. Same bytes; pick one set of names.
- [ ] note src/EOV4.C:119-128 (struct Region): the same layout is declared in src/E082C.C:24, src/E16A5.C:16, src/E1A29.C:20, src/E1DE7.C:21 and src/EOV2.C:22 with different owner citations. The struct in this file cites src/E1DE7.C; fine, but the file should be the one place the struct is described.
- [ ] note src/EOV4.C:44 (SetRegionRect): height is scaled by 8 but width (right - left) is not; the header says left and right are in 8-pixel columns, so width is in columns too. Say so.
- [ ] note src/EOV4.C:584 and :572 (Ovl4_0D1B): `page5` holds the g7001 base (`gFBCC * 5`) and is then reused as the loop counter. The reuse matches the bytes; say why in a comment or leave it.
- [ ] note src/EOV4.C:603-605 and :582-583 (Ovl4_0D1B): the `0x12` sprite nudge for `obj->m07` and the `0xfe`/`0xff` overrides of `m0A` (:593-598) have no comment; their meaning is unknown, so "Purpose unknown" is enough.
- [ ] note src/EOV4.C:1046 (Ovl4_1D20): the header says "the 0x34 object" (the heading value DrawViewSlots tests at :1114). That is a description of the value, not a name with evidence; fine as written.
- [ ] note src/EOV4.C:1033 (cross-file, owner EOV2): src/EOV2.C:116 says "src/EOV4.C:955 indexes the table" and src/EOV2.C:155 says "src/EOV4.C:756"; both lines are stale (now 1072 and 759). The owner of src/EOV2.C should cite the addresses instead (ovl4:1D20 and ovl4:0000 region of the SlotSize use). Not a must-fix here.
- [ ] note src/EOV4.C:1192-1199 (NoOp2039): the body has no observable effect; the name says so. Fine as it is.

## Summary

The unit is understandable from its source: the names that carry evidence (ComputeViewSpan, SetRegionRect, CopyViewColumn, ToggleViewColumn, InitViewSlots, DrawViewFrame, DrawViewRows) match what the code does, and the struct layouts agree with the offsets the bytes use. Most of the named placeholders (Ovl4_044B, Ovl4_0AD0, Ovl4_1091, Ovl4_1411, Ovl4_19C4, Ovl4_1D20, Ovl4_0D1B) are still "Purpose unknown" with no evidence beyond the call graph, which is a fine reason to keep them. The problems are in the comments: several call-site addresses are wrong (checked against the disassembly), some behaviour is misdescribed (the ComputeViewSpan adjustment rule and its QUIRK note, the Ovl4_0AD0 per-item recompute, the DrawViewFrame blank, the Ovl4_1D20 nudge, the Ovl4_19C4 `modulo 8`), and two names need evidence or a rename (IsSlotLive's "live" claim, and the `g1879`/`gFarA92A` names that src/ already gives other names). Nothing escalated: no matcher, spec or pin question.

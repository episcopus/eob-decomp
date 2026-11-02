# Review: e1de7

Reviewer: rev-haiku
Commit reviewed: 1a6cbb9f1ac8b45f1aea2766d03dbe6b2e185518 (main, when I started)

Playbook 70. Judge what a matching file can still get wrong: names, types, comments, structure.
The matcher already proved the bytes; do not re-prove them, read for meaning.

## Findings

Each finding is a checkbox. `must-fix` blocks `make review-pass` until a decompiler ticks it off
(with the commit that fixes it); `should` and `note` do not block.

- [ ] must-fix src/E1DE7.C:126: `QUIRK 1DE7:0135` is not an instruction start. The linear disassembly of eob/EOB.EXE puts `add sp, 8` (83 c4 08) at 1DE7:0134, so 0135 is inside it. The two reads of gTextLeft that the QUIRK describes (the second and third edge runs, lines 154 and 155) are the `mov ax, [0xfe64]` at 1DE7:01FE and at 1DE7:021C. Fix the address and the instruction it names. (Evidence: disassembly of 1DE7 in eob/EOB.EXE; the QUIRK at 1DE7:09A6, by contrast, is a real `eb 00` and is correct.)
- [ ] must-fix src/E1DE7.C:125: "Called by many units to draw a decorated box" has no evidence. The packet lists no caller for 1DE7:00C9, and a byte scan of eob/EOB.EXE finds no far call `9a c9 00 e7 1d`. The overlay images show no direct calls to any 1DE7 routine either, so the packet's call scan is the evidence here, and it finds none. Either cite the callers (segment:offset), or say "no caller found", as PrintIntText does (line 280).
- [ ] must-fix src/E1DE7.C:39 and :206: "the colour of the frame" is not what this unit does with gTextAttr2. DrawTextFrame (lines 129 to 157) draws only tiles, through gTile805A, and takes no colour. In this file gTextAttr2 is the fill colour of ClearTextRegion (line 221) and of the DrawTextLine refill (line 247), and the fifth argument of the draw-text hook (lines 248 and 264). re/renames.toml records ds:811B as "the fill colour of ClearTextRegion and of the redrawn scroll line" (default 0x0C at E07C8:148). Make both comments say that.
- [ ] must-fix src/E1DE7.C:68-70: "the role is not established" for ds:8115 is contradicted by the rest of src/, which already has the meaning with evidence: E082C.C:44 and :383 ("g8115, the status-line flag", saved and cleared around the report), E082C.C:296 (set to 2 by LoadOverlayScreen), E1C0E.C:589 (clears ds:8115 while the message box is drawn), E16A5.C:511 (ds:8115 the status-line flag is cleared). Give that meaning and cite one of them.
- [ ] should src/E1DE7.C:10-11: the header says the game meaning of ds:811B, ds:811D and ds:8115 is not established. re/renames.toml already records ds:811D as the foreground colour (default 0x0F at E07C8:147) and ds:811B as the fill colour (see the finding above), and ds:8115 is the status-line flag. Say what is evidenced, so the header does not contradict the rename log.
- [ ] should src/E1DE7.C:57-59: the gTile805A comment says the frame corners are drawn through it. DrawTileRun also draws the edges through it (line 176). The image has five `lcall [805a]` sites: four in DrawTextFrame (1DE7:0130, 0163, 0196, 01CC) and one in DrawTileRun. The "no writer" claim holds: no store to ds:805A in eob/EOB.EXE or build/re/eob-overlays/*.bin. The name is a placeholder; its signature (tile, x, y) matches ds:805E, which src/ calls gDraw805E (E16A5.C:88, E1658.C:12 to 14). Consider `gDraw805A` (make rename), and list the edges in the comment.
- [ ] should src/E1DE7.C:64-66: gScroll8117 puts an address in the name and calls the routine a "scroll hook". What is known: it is read only in DrawTextLine, when the row is past the last full line and gScrollOn is set (line 238). Overlay 5 writes it when it opens windows 0xC and 9: offset 0xAC and segment 0x2D0 (EOV5.C:1114-1115, 1254-1255, 1317-1318; the bytes `c7 06 17 81 ac 00` are in eob/EOB.EXE and build/re/eob-overlays/ovl05.bin). No function at ovl5:00AC is named in src/. Name it for what is known (for example gScrollHook), and say that overlay 5 sets it. EOV5.C:161 and :162 declare the two words as int g8117 and g8119, so give that in the WHY of the rename.
- [ ] should src/E1DE7.C:45: g811F is the record index of the current text window (SetTextWindow stores it as gRegions[g811F], line 88 onwards); E1A29.C:38 says the same. It is a placeholder with evidence, so give it the evidenced name (for example gTextRegion) with make rename. Users: E07A9.C, E1C0E.C, EOV1.ASM, EOV2.C and EOV6.C. E1C0E.C:577 uses a local named saved811F for the same value.
- [ ] should src/E1DE7.C:46: g8102 is the video mode the game initialised (E082C.C:41, "3 is CGA-compatible"). work/reviews/e1ddd.md:15 calls it the graphics mode (WHITEPAPER 111). The comment cites src/E07A9.C, which does not define the value. Either rename it with evidence (gGraphicsMode) or cite E082C.C:41.
- [ ] should src/E1DE7.C:63: the draw-text hook's parameters are named a, b, c, d. E14EF.C:79 and E16A5.C:87 name them x, y, color, extra, which the e14ef review round (commit 1efdacc) settled on. Use one set across src/ (declarations are not checked by R4, so nothing else will catch it).
- [ ] note src/E1DE7.C:62: the rectangle fill's last parameter is `colour` here, `pattern` in src/E1547.C:90 and `attribute` in re/renames.toml. Pick one.
- [ ] note src/E1DE7.C:11 and 13: `inc sp; inc sp` (44 44) is present in the segment (1DE7:00C0, 00C1, 033F and one more), so the byte pattern the header describes is real. The match proves the flags give these bytes; I did not check that -G is the switch that produces them.
- [ ] note src/E1DE7.C:52-55: the `char` declaration of gFontWidth and gFontHeight is justified. ds:8100 is loaded at 1DE7:03EF and followed by `98` (cbw; capstone prints it as cwde in 16-bit mode). The other files declare them unsigned char (src/E1A47.C:13 and 14). The difference is real and documented here, so it can stay.
- [ ] note src/E1DE7.C:126-128: the DrawTextFrame QUIRK is a codegen difference, not a behaviour: both reads give the same value. It is still a fair QUIRK (dead or unused read), but the comment should say so, so a porter does not think the value differs.
- [ ] note src/E1DE7.C:307-308: `line[82]` and `word[82]` are unexplained constants. Say why 82 (a window size, a frame size) or leave a comment that it is a buffer size.
- [ ] note: the caller lists in the comments were checked against the packet and the byte scans (SetTextWindow: 5 far call sites in eob/EOB.EXE plus the overlay callers the packet finds; SetTextWindowAndClear; SetRegionExtents from ovl4; ClearTextRegion; DrawTextLine only from DrawText; PrintIntText none in resident code). All match. The pointer claim for HidePointerZone (E1BBE.ASM:354) and the ClearTextRegion rectangle (pixels, one short on the right and bottom: lines 221 and 246) also check out.

## Summary

The unit is understandable from its source: the names of the nine functions match what they do, the
window model (region record, live globals, SetTextWindow save and load) reads clearly, and every caller
list in the comments matches the packet, except DrawTextFrame, whose "many units" caller claim has no
evidence (the packet lists none). The function names are evidenced; the placeholder globals are
mostly evidenced too but still carry placeholder names: g811F (record index), g8102 (video mode),
gTile805A (a tile routine in the same family as gDraw805E) and gScroll8117 (a far hook that overlay 5
sets; "scroll" is an inference from its one call site). Four must-fix items: a QUIRK address that falls
inside an instruction, an unevidenced caller claim, a "frame colour" that the unit does not use, and a
"role not established" for ds:8115 that the rest of src/ has already established. None of them needs a
matcher or spec change. Nothing is escalated.

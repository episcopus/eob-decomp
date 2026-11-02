# Review: e14ef

Reviewer: rev-haiku
Commit reviewed: 6358509 (main; review/e14ef at the same commit; src/E14EF.C last changed in f8ab4f5; `make match FILE=src/E14EF.C`: 6 of 6, 1410 bytes, exit 0; `make readability UNIT=e14ef`: 0 findings)

Playbook 70. Judge what a matching file can still get wrong: names, types, comments, structure.
The matcher already proved the bytes; do not re-prove them, read for meaning.

Round 5. Round 4's findings are fixed and hold. This round I checked every cross-file cite in the
file by hand: the callers in src/EOV0.C (:1148, :1149, :1189-1193, :1203, :1215, :1241-1244, :1253,
:1257, :1259-1262), src/EOV2.C (:380, :383), src/E1A72.ASM (:60, :63, :297, :356, :380-381),
src/E1AAD.ASM (:337-338, :350-351, the 0B0E button bit at :1157-1162), src/E1C0E.C (:59, :60),
src/E07C8.C (:30) and src/E1A47.C (:13). They are right except the two must-fix items below. The
names still hold. Each finding is a comment or a cite; no code change is asked for.

## Findings

Each finding is a checkbox. `must-fix` blocks `make review-pass` until a decompiler ticks it off
(with the commit that fixes it); `should` and `note` do not block.

- [ ] must-fix src/E14EF.C:69-71 (ds:8134): "the map screen clears it (src/E0ACD.C:119)" cites a declaration, not the clear. E0ACD.C:119 is `extern int g8134;` with the comment "set to 0 by the map screen (src/E14EF.C)", so the two comments cite each other and neither shows the store. The store is src/E0ACD.C:886 (`g8134 = 0;` in RunMapLoop, 0ACD:0E8A). Cite :886. (evidence: src/E0ACD.C:119, :880-886)
- [ ] must-fix src/E14EF.C:132-139 (CountMaskBits comment): "1 << 16 is 0 and bits 16 and up are never counted" is a claim about a shift by 16 on bytes that no caller reaches (src/EOV0.C:1193 and :1244 pass n 15 and 9 with shift 0, so i + shift stays at most 14). It is recall of 8086 shift behaviour, and C leaves a shift by the full width of an int undefined. Tag it [H] or drop it. The bit-15 reading (the int `1 << 15` sign-extended in the long AND) is C's own promotion rule, but it is not exercised on our bytes either; tag it the same way. (evidence: src/EOV0.C:1193, :1244; AGENTS.md ground rule 1)
- [ ] should src/E14EF.C:51-57 (g8140, g8142): "tested by UpdateOptionList" names one reader, but this file's own gFC7A comment (:51-53) says PickAlignment tests them at src/EOV0.C:1262, and others read them: src/EOV5.C:377 and :483, src/E16A5.C:224-225 and :240-241 (into gHitX and gHitY), and the wrapper at src/E082C.C:465. src/EOV0.C:1204 writes them. Say "tested by UpdateOptionList and PickAlignment (src/EOV0.C:1262)" so no reader takes the last click to be UpdateOptionList's alone. (evidence: the greps above)
- [ ] should src/E14EF.C:243-247 (UpdateOptionList locals): `colorA`, `colorB`, `prev`, `row` and `entry` are `char` with no reason given. The header gives one for gFontWidth (:58-59), so these should have one too: if the match needs char, say so at the declarations; if the reason is not known, say `Purpose unknown:` for the type and leave it to make match. (evidence: src/E1A47.C:13 for the gFontWidth case)
- [ ] should src/E14EF.C:42 (struct Panel, `pad` at +e): the name says padding, but the only evidence is "not read in this unit". src/EOV0.C:185 names the same field pad. Keep the placeholder and say what is known (the eighth word of the 16-byte record, unread here), or name it by its offset. (evidence: :42; src/EOV0.C:185)
- [ ] note Callee names differ by unit for one routine: 14EF:00E1 is IsPointInRect here and in src/EOV0.C:205, while 082C:0613 is IsPointInsideRect in src/E082C.C:302 and src/EOV5.C:187, and the comment at src/E082C.C:300-301 says "Same test as PointInRect at 14EF:00E1". They are two addresses, so `make rename` does not force a choice; READABILITY.md puts cross-unit harmonisation in phase 6. Routed, not edited.
- [ ] note make readability still does not check line cites (round 4's note, still open): the two stale cites above passed it with 0 findings.
- [ ] note the routed notes of rounds 1 to 4 still stand: src/EOV5.C:373 and :468 still write the literals 0x8FF and 0x8ff, not KEY_CODE_MASK; src/EOV0.C:143 and src/EOV2.C:181 declare UpdateOptionList with `long counter` and no `extra`; src/EOV0.C:204 declares CountMaskBits with `char shift`; src/EOV0.C:177-186 declares a second struct Panel; src/E082C.C:300-301 "Same test as PointInRect".

## Summary

The unit is understandable from its source alone. The names hold: DrawOptionList, UpdateOptionList,
FindMaskBit, CountMaskBits, BlinkText and IsPointInRect (see the naming note), the shared globals
gFontWidth, gFontHeight, gMouseDisabled, gPanels, g8140 and g8142, and the gFC7A reader. The gMouseDisabled
claim checks out: the INT 33h handler at 1A72:017A tests it at src/E1A72.ASM:356 and returns when it is
set. Two must-fix this round, both about where a comment points or how it is tagged: the g8134 clear
cite and the unverified shift claims in CountMaskBits. Three shoulds: the g8140/g8142 readers, the char
locals and the `pad` name. The placeholders are honest and need no change: gFC78, g5D1A, g8102,
`counter` and `extra`. The matcher (6 of 6) and the readability check (0 findings) were re-run on
6358509. No matcher, spec or pin question; nothing escalated.

---

# Round 4 (kept)

Reviewer: rev-haiku
Commit reviewed: 62c8136 (main; review/e14ef at the same commit; src/E14EF.C last changed in 2b84265; `make match FILE=src/E14EF.C`: 6 of 6, 1410 bytes, exit 0; `make readability UNIT=e14ef`: 0 findings)

Playbook 70. Judge what a matching file can still get wrong: names, types, comments, structure.
The matcher already proved the bytes; do not re-prove them, read for meaning.

Round 4. The names, types and the caller claims still hold: the callers in src/EOV0.C (:1148, :1149,
:1189-1193, :1203, :1215, :1241-1244, :1253, :1257, :1259-1262) and src/EOV2.C (:380, :383) match the
header, and the overlay witnesses for CountMaskBits (build/re/eob-overlays/ovl00.bin: B8 0F 00 at
0x20DD, B8 09 00 at 0x228E) are right. What fails is the line citations. Round 3 wrote its cites
against a file whose header then grew, and nothing re-checked them, so about twenty cites in this
file now point at other lines (the header's field reads, the `picked` lines, the g5D1A, g8102 and
g8134 notes, and two cites into src/E1A72.ASM). `make readability` reports 0 findings, so it does not
catch stale cites. Each finding below is a comment fix; no code changes.

## Findings

Each finding is a checkbox. `must-fix` blocks `make review-pass` until a decompiler ticks it off
(with the commit that fixes it); `should` and `note` do not block.

- [x] must-fix src/E14EF.C:10-12 (header, field offsets): "reads y at :169, x at :170, sel at :171 and height at :172" points at the local declarations. The reads are y at :178, x at :179, sel at :180 (FindMaskBit(panelDef->sel, ...)) and height at :181. "reads width at :260" is wrong: :260 is the x read (xbase); width is read at :269 (the right edge). "colorA/colorB at :252-253" are `counter++` and the panel pointer; they are at :261 and :262. "DrawOptionList also reads colorA/colorB at :177, :179" are at :186 (colorA) and :188 (colorB). (evidence: grep -n of src/E14EF.C, lines :169-172, :178-181, :186, :188, :252-253, :260-262, :269) Fixed in f8ab4f5: the header now reads y :178, x :179, sel :180 and height :181 (DrawOptionList), width :269 and colorA/colorB :261-262 (UpdateOptionList), and colorA :186 / colorB :188 (DrawOptionList). Comments only, no code byte changed.
- [x] must-fix src/E14EF.C:220-221 (UpdateOptionList header): "`picked` holds the selectable row at first (:290, :298) and then ... the string index (:320, ...)". The row is stored in picked at :299 (click) and :307 (confirm); :290 is the `break;` of KEY_FIRST. The string index is assigned at :329 (the line that has the comment); :320 is a gDrawText call. (evidence: E14EF.C:289-290, :298-299, :307, :320, :329) Fixed in f8ab4f5: the header now cites the row stores at :299 (click) and :307 (confirm) and the returned string index at :329.
- [x] must-fix src/E14EF.C:225 (UpdateOptionList header): "(!gMouseDisabled, :262)" points at `colorB = panelDef->colorB;`. The test is at :271. (evidence: E14EF.C:262, :271) Fixed in f8ab4f5: the header now cites the `if (!gMouseDisabled)` at :271.
- [x] must-fix src/E14EF.C:202 (KEY_CONFIRM): "the row is picked (:298)". Line 298 is the click case's `row = (g8142 - top) / lineheight;`. The confirm case that picks is :307 (`picked = row;` under `case KEY_CONFIRM`). (evidence: E14EF.C:298, :304-307) Fixed in f8ab4f5: the KEY_CONFIRM enum comment now cites :307, the `picked = row;` in the confirm case.
- [x] must-fix src/E14EF.C:55 (g5D1A): "makes DrawOptionList draw the selected row in colorB (:178)". The test is at :187 (`entry == selEntry && (g5D1A || gMouseDisabled)`) and the draw is at :188; :178 is `y = gTextTop + panelDef->y;`. (evidence: E14EF.C:187-188) Fixed in f8ab4f5: the g5D1A comment now cites the test at :187 and the draw at :188.
- [x] must-fix src/E14EF.C:61 (g8102): "a zero here makes the normal text colour 1 (:177, :252)". The two tests are :186 (DrawOptionList) and :261 (UpdateOptionList). :177 is `panelDef = &gPanels[panel];` and :252 is `counter++`. (evidence: E14EF.C:186, :261) Fixed in f8ab4f5: the g8102 comment now cites the two tests at :186 and :261.
- [x] must-fix src/E14EF.C:70-71 (g8134): "(or ax,[g8134]; mov [g8134],ax, src/E1A72.ASM:370-371)" points at other lines: the pair is src/E1A72.ASM:376-377. "this file clears it (:184)" is the `g8134 = 0;` at :193. (evidence: src/E1A72.ASM:376-377; E14EF.C:193) Fixed in f8ab4f5: the comment now cites the pair at src/E1A72.ASM:380-381 (E1A72.ASM gained 4 comment lines after this review; re-read and confirmed `or ax,[g8134]` / `mov [g8134],ax`) and this file's clear at :193.
- [x] must-fix src/E14EF.C:66 (gMouseDisabled): "InitMouse sets it to 1 before it installs the handler (1A72:0150, src/E1A72.ASM:295)". src/E1A72.ASM:295 is `mov word ptr [gMouseHandlerInstalled],1`; the store to gMouseDisabled is src/E1A72.ASM:296. The address 1A72:0150 was not checked here. (evidence: src/E1A72.ASM:286-300) Fixed in f8ab4f5: the comment now cites the gMouseDisabled store at src/E1A72.ASM:297 (E1A72.ASM gained 1 comment line before it after this review), and the address 1A72:0150 is kept after checking it: disasm.py shows 1A72:0150 `mov word ptr [0x814a],1` (the instruction after 1A72:014A `mov word ptr [0x8148],1`).
- [x] should src/E14EF.C:46-47 (gTextLeft): "x is in cell columns (eight pixels in DrawOptionList, the font width in UpdateOptionList)" mixes two units. gTextLeft is always in 8-pixel columns (`<< 3` at :260 and :267; src/E1C0E.C:60 "8-pixel columns"). The panel's x is 8-pixel cells in DrawOptionList (:179) and font-width units in UpdateOptionList (:267). Say that once for gTextLeft and once for the panel's x. (evidence: E14EF.C:179, :260, :267; src/E1C0E.C:60) Fixed in f8ab4f5: the gTextLeft comment now says gTextLeft is in 8-pixel columns (<< 3 at :260 and :267, src/E1C0E.C:60) and separately that the panel's x is 8-pixel cells in DrawOptionList (:179) and font-width units in UpdateOptionList (:267).
- [x] should src/E14EF.C:77-78 and :164 (IsKeyPending, FlushKeyBuffer, the DrawOptionList header): "nonzero when a key is waiting", "clear the pending key", "clears the pending key". The callees' comments say events, and the flush drops the whole ring: 1AAD:0471 is "1 when at least one event waits in the ring" (src/E1AAD.ASM:337-338) and 1AAD:0480 is "discarding every buffered event" (src/E1AAD.ASM:350-351). The ring also holds the mouse events 41h and 42h (1AAD:048D), so "event" is the true word. (evidence: src/E1AAD.ASM:337-338, :350-351) Fixed in f8ab4f5: IsKeyPending now says "nonzero when an event waits in the ring (src/E1AAD.ASM:337-338)", FlushKeyBuffer "discard every buffered event (src/E1AAD.ASM:350-351)", and the DrawOptionList header "discards the buffered events (1AAD:0480)".
- [x] should src/E14EF.C:67-68 (gCursorX, gCursorY): "mouse x (E1A72.ASM)" and "mouse y (E1A72.ASM)" give no unit. src/E1A72.ASM:59 and :62 say "mouse x in pixels" and "mouse y in pixels", and UpdateOptionList compares them with the pixel rectangle (E14EF.C:268, :272-273). Say pixels. (evidence: src/E1A72.ASM:59, :62; E14EF.C:268, :272-273) Fixed in f8ab4f5: gCursorX now says "mouse x in pixels (src/E1A72.ASM:60); tested with the pixel rectangle (:272-273)" and gCursorY "mouse y in pixels (src/E1A72.ASM:63)" (E1A72.ASM gained 1 comment line before those EXTRNs after this review).
- [ ] note make readability does not check line citations (this round's ~20 stale cites passed it with 0 findings). A cite check on the citing line (grep the cited text at the cited line) would catch them; that is a tool question for the human, not a decompiler edit.
- [ ] note the rounds 1 to 3 notes still stand and are routed, not edited here: WaitKeyEvent's callers (src/EOV5.C:373, :468 still write the literal 0x8FF, not KEY_CODE_MASK); the EOV0 prototypes src/EOV0.C:143 (UpdateOptionList with five parameters and `long counter`, no `extra`; the bytes agree) and :204 (`char shift`); the struct Panel copy at src/EOV0.C:177-186; src/E082C.C:300-302 ("same test as PointInRect"); and the stale cites of this file in src/E1A72.ASM, src/E1BBE.ASM, src/E1C00.ASM, re/globals.toml and re/renames.toml.

## Summary

The unit is understandable from its source alone. The names hold (DrawOptionList, UpdateOptionList,
IsPointInRect, FindMaskBit, CountMaskBits, BlinkText, and the shared globals gFontWidth, gFontHeight,
g8140, g8142, gMouseDisabled, gPanels), and so do the callers' masks and the gFC7A reader. What fails
this round is the line citations: about twenty in this file's comments point at other lines, because
round 3's header grew and its cites were not rechecked. Eight must-fix items (the citation blocks at
:10-12, :220-221, :225, :202, :55, :61, :70-71, :66), each a comment fix with no code change. Two shoulds:
the gTextLeft units and the event wording of IsKeyPending and FlushKeyBuffer, plus the pixel units of
gCursorX and gCursorY. Placeholders are honest and need no change: gFC78, gFC7A (its reader is now
named), g5D1A, g8102 (meaning not established), g8134, g8140, g8142, `counter`, `extra` and `pad`. The
matcher (6 of 6) and the readability check (0 findings) were re-run on 62c8136. No matcher, spec or pin
question; nothing escalated.

---

# Round 3 (kept)

Reviewer: rev-haiku
Commit reviewed: 98f74ac
Playbook 70. Judge what a matching file can still get wrong: names, types, comments, structure.
The matcher already proved the bytes; do not re-prove them, read for meaning.

Round 3. Rounds 1 and 2 (kept below) are ticked and their fixes hold. This round found that the
callers changed under the unit: src/EOV0.C is complete (b57c9eb), so ovl0:207A PickClass and
ovl0:2238 PickAlignment are no longer stubs, and they pass masks other than all-ones. The header,
the CountMaskBits and UpdateOptionList comments cite the old stub lines, and the reader of ds:FC7A
is now in src/. Four must-fix, all comments; no name or type is wrong.

## Findings

Each finding is a checkbox. `must-fix` blocks `make review-pass` until a decompiler ticks it off
(with the commit that fixes it); `should` and `note` do not block.

- [x] must-fix src/E14EF.C:11-15 (header, the callers paragraph): "Every caller in src/ passes mask 0xffffffff and shift 0 (src/EOV0.C:786, :787; src/EOV2.C:380, :383) ... FindMaskBit always takes its all-ones shortcut (its bit scan is reached by no known caller) and CountMaskBits is reached only from the two stub callers ovl0:207A and ovl0:2238 (src/EOV0.C:809-810)" is no longer true. src/EOV0.C:1180 (PickClass, ovl0:207A) and src/EOV0.C:1233 (PickAlignment, ovl0:2238) are decompiled in src/ and pass masks other than -1: PickClass passes gTable6081[race] & mask to CountMaskBits (:1193), DrawOptionList (:1203) and UpdateOptionList (:1215), with mask -5 when a party member has align >= 6 (:1189-1190); PickAlignment passes gTable6099[classIndex] & mask to CountMaskBits (:1244), DrawOptionList (:1253) and UpdateOptionList (:1257), with mask 0xfe3f when a party member has m20 == 2 (:1241-1242). So the bit scan in FindMaskBit is reached in play. PickOption (src/EOV0.C:1148-1149, 0xffffffffL, shift 0) and src/EOV2.C:380, :383 pass the all-ones mask. src/EOV0.C:786-787 and :809-810 now hold unrelated comments. Fix: name the src/ callers and their masks at these lines, and drop the stub and "no known caller" claims. (evidence: src/EOV0.C:1148-1149, :1193, :1203, :1215, :1244, :1253, :1257, :1189-1190, :1241-1242; b57c9eb) Fixed in 2b84265: the header now names PickOption (src/EOV0.C:1148, :1149) and LoadGamePanel (src/EOV2.C:380, :383) as the all-ones 0xffffffff/shift 0 callers, and PickClass (:1203, :1215) and PickAlignment (:1253, :1257) as the callers of gTable6081[race] & mask and gTable6099[classIndex] & mask (mask -5 at :1189-1190, 0xfe3f at :1241-1242); the stub and "no known caller" claims are gone (the bit scan is reached in play), and CountMaskBits is named with its src/ callers.
- [x] must-fix src/E14EF.C:126-130 (CountMaskBits comment): "the two external callers ovl0:207A and ovl0:2238 pass shift 0 with n 15 and 9 (overlay disassembly at ovl0:20DD and ovl0:228E)". Those callers are in src/ now: CountMaskBits(15, ..., 0) at src/EOV0.C:1193 and CountMaskBits(9, ..., 0) at src/EOV0.C:1244. "External" is stale; cite the source (the disassembly can stay as a second witness). The 16-bit safety claim still holds: with n 15 and shift 0, i + shift is at most 14. (evidence: src/EOV0.C:1193, :1244) Fixed in 2b84265: "external" is gone; the comment now cites PickClass (src/EOV0.C:1193) and PickAlignment (src/EOV0.C:1244) as the src/ callers, keeps the overlay disassembly (ovl0:20DD, ovl0:228E) as the second witness, and adds QUIRK 14EF:0107 for the bit-15 sign extension (see the round 3 should below).
- [x] must-fix src/E14EF.C:218-219 (UpdateOptionList comment): "Both callers pass a long zero (0L) ... (src/EOV0.C:787; src/EOV2.C:383)". src/EOV0.C:787 is a comment about other code now. Every caller passes 0L: src/EOV0.C:1149 (PickOption), :1215 (PickClass), :1257 (PickAlignment) and src/EOV2.C:383 (LoadGamePanel). Say "every caller" and cite those lines. (evidence: src/EOV0.C:1149, :1215, :1257; src/EOV2.C:383) Fixed in 2b84265: the comment now says "Every caller passes a long zero (0L)" and cites src/EOV0.C:1149, :1215, :1257 and src/EOV2.C:383 (the stale :787 cite is gone).
- [x] must-fix src/E14EF.C:47 (ds:FC7A): "written here, its reader is not yet identified" is wrong. The reader is src/EOV0.C:1259 and :1261 in PickAlignment (ovl0:2238): it tests gFC7A == 0x6e or 0x32 (:1259), and 0x41 or 0x42 (:1261), then IsPointInRect on g8140/g8142 (:1262). Name the reader with those lines. (evidence: src/EOV0.C:1259, :1261, :1262; src/EOV0.C:198) Fixed in 2b84265: the gFC7A comment now names PickAlignment (src/EOV0.C:1259, :1261) as the reader, its 0x6e/0x32 and 0x41/0x42 tests, and the IsPointInRect on g8140/g8142 (:1262); "not yet identified" is gone.
- [x] should src/E14EF.C:3: "its size in characters" covers both width and height, but height (+6, struct Panel line 34) is in lines. Say "its width in characters and height in lines". Fixed in 2b84265: the header now says "its width in characters and height in lines".
- [x] should src/E14EF.C:9-10: the header says the stride and field offsets come from DrawOptionList's reads at :169-:172 (x, y, sel, height). The other offsets have no address in the header: width (+4) is read at :260 (UpdateOptionList), colorA and colorB at :252-253 (UpdateOptionList) and :177, :179 (DrawOptionList). Add them, so every offset has an address. Fixed in 2b84265: the header now cites width at :260 and colorA/colorB at :252-253 from UpdateOptionList and colorA/colorB at :177, :179 from DrawOptionList, so every field offset has an address.
- [x] should src/E14EF.C:126-130 (CountMaskBits): for i + shift = 15 the int `1 << 15` is -32768, which sign-extends to 0xFFFF8000 in the long AND, so bit 15 is tested as "any of bits 15 to 31", not as bit 15. No caller reaches it (i + shift is at most 14 with n 15 and shift 0), but the comment says only that bits 16 and up are never counted. Say what bit 15 does, and mark `QUIRK 14EF:0107:` if a port must keep it. Fixed in 2b84265: the CountMaskBits comment now adds QUIRK 14EF:0107 saying the int `1 << 15` sign-extends to 0xFFFF8000 in the long AND, so bit 15 would count as "any of bits 15 to 31", and a port must keep it.
- [ ] note src/E14EF.C:192 (KEY_CODE_MASK, "the callers' filter"): the two other callers of WaitKeyEvent that apply this filter, src/EOV5.C:373 and :468, still write the literal 0x8FF and 0x8ff, not this name. Nothing to change here; the EOV5 owner may name it. (evidence: src/EOV5.C:373, :468)
- [ ] note src/EOV0.C:177-186 declares a second `struct Panel` for the same 16-byte record. Its +8 field is "the selected entry", where src/E14EF.C:35 says the selected row, counted among the selectable entries (the distinction FindMaskBit makes). Routed to the EOV0 owner; not edited here.
- [ ] note src/EOV0.C:143 and :204 declare UpdateOptionList with `long counter` and CountMaskBits with `char shift`, where src/E14EF.C has int counter and extra and int shift. The bytes agree for the callers. Routed to the EOV0 owner.
- [ ] note src/E082C.C:300-302: "Same test as PointInRect at 14EF:00E1" is stale. This file's name is IsPointInRect (:119), used by src/EOV0.C:205 and src/EOV1.ASM:48. E082C's IsPointInsideRect (082C:0613, src/E082C.C:302) is a separate routine at a different address. Routed to the E082C owner.
- [ ] note stale citations of this file in other units, routed and not edited: src/E1A72.ASM:54 "E14EF.C:150" (the g8134 clear is now :184); src/E1BBE.ASM:431 "E14EF.C:44-45" (the LockHidePointer and LockShowPointer externs are now :69-70); src/E1C00.ASM:288 "E14EF.C:227" (the mouse-enabled test is now :262); re/globals.toml:302 and re/renames.toml:775 cite E14EF.C:144 and :227, which have moved too.

## Summary

The unit is understandable from its source alone, and the names still hold: DrawOptionList,
UpdateOptionList, IsPointInRect, FindMaskBit, CountMaskBits and BlinkText say what the code does,
and the round 1 and round 2 fixes hold. What fails is the evidence in the comments, because it was
true when written and has since moved. The stub callers (ovl0:207A and ovl0:2238) are now decompiled
callers in src/ that pass other masks, which changes what FindMaskBit's general path does in play.
The reader of ds:FC7A is now in src/. Four must-fix items, all comments; no name or type is wrong.
The matcher (6 of 6) and the readability check (0 findings) were re-run on 98f74ac. No matcher, spec
or pin question; nothing escalated.

---

# Round 2 (kept)

Reviewer: rev-haiku
Commit reviewed: 2dd86e3 (main; review/e14ef at a5b4f64; src/E14EF.C is identical on both)

Playbook 70. Judge what a matching file can still get wrong: names, types, comments, structure.
The matcher already proved the bytes; do not re-prove them, read for meaning.

Round 2. `make match FILE=src/E14EF.C`: 6 of 6, 1410 bytes; `make readability UNIT=e14ef`: 0 findings.
Round 1 (all items ticked, kept below) fixed the comment problems it found; the fixes hold (ds:5D1A,
ds:814A polarity, the mask, Panel.sel, ds:FC7A, the key count). This round read the file again
against the code and the other units, and found stale line citations and some claims the evidence
does not carry.

## Findings

Each finding is a checkbox. `must-fix` blocks `make review-pass` until a decompiler ticks it off
(with the commit that fixes it); `should` and `note` do not block.

- [x] must-fix src/E14EF.C:10: "the accesses at :134-:137" points at comment lines. The panel fields are read at :153-:157 (DrawOptionList: y :154, x :155, sel :156, height :157). (evidence: E14EF.C:153-157) Fixed in 00b90bf: the header now cites the reads in DrawOptionList by their final lines, y :169, x :170, sel :171, height :172 (the comment grew, so every in-file cite was re-checked).
- [x] must-fix src/E14EF.C:176: KEY_CONFIRM says "the row is picked (:260)". Line 260 is `case KEY_FIRST:`. The pick is `picked = row;` at :280. (evidence: E14EF.C:277-281) Fixed in 00b90bf: the enum comment now cites :298, the `picked = row;` in the confirm case.
- [x] must-fix src/E14EF.C:197: "(!gMouseDisabled, :227)" points at `rows = panelDef->height - 1`. The test is at :244. (evidence: E14EF.C:244) Fixed in 00b90bf: the UpdateOptionList comment now cites :262, the `if (!gMouseDisabled)`.
- [x] must-fix src/E14EF.C:11-12 and :200-201: "src/EOV0.C:475, :490, :491" are not option-list calls. Those lines are in Ovl0_0DDC (EOV0.C:475-491). The calls are EOV0.C:771 (UpdateOptionList), :786 (DrawOptionList) and :787 (UpdateOptionList), all with mask 0xffffffffL and shift 0. (evidence: src/EOV0.C:771, :786, :787, :475) Fixed in 00b90bf: the header now cites src/EOV0.C:786 (DrawOptionList) and :787 (UpdateOptionList) and src/EOV2.C:380, :383; the UpdateOptionList comment cites src/EOV0.C:787 and src/EOV2.C:383. EOV0.C:475/:490/:491 are gone. (EOV0.C:771 is a comment line, not a call, so it is not cited.)
- [x] must-fix src/E14EF.C:12-13: "reached only from callers outside src/ (ovl0:1F66, ovl0:207A, ovl0:2238)" is wrong for ovl0:1F66. PickOption (src/EOV0.C:774) is decompiled in src/ and passes mask 0xffffffffL, shift 0 (EOV0.C:786-787). ovl0:207A and ovl0:2238 are stubs in src/EOV0.C:809-810. Say that the general paths are reached only from stub callers, and cite where the disassembly for their arguments is. (evidence: src/EOV0.C:774, :786, :809-810) Fixed in 00b90bf: the header now says every src/ caller passes 0xffffffff/shift 0 (EOV0.C:786, :787; EOV2.C:380, :383), so FindMaskBit always takes its all-ones shortcut and CountMaskBits is reached only from the two stubs ovl0:207A and ovl0:2238 (EOV0.C:809-810); ovl0:1F66 (PickOption) is named as the src/ caller.
- [x] must-fix src/E14EF.C:114-117: the 16-bit safety claim considers only the shift. The bit tested is i + shift with i < n, so the claim needs the largest n too, and the callers are stubs (src/EOV0.C:809-810), so n is not in src/. Either give n with its source (an address in the disassembly), or mark the behaviour `QUIRK 14EF:0107:` (bits 16 and up are never counted, because 1 << 16 is 0 in a 16-bit int), since a port must keep it. (evidence: E14EF.C:127; src/EOV0.C:809-810) Fixed in 00b90bf: n comes from the stub disassembly, both external callers pass shift 0 with n 15 and 9 (ovl0:20DD `mov ax,0xf`, ovl0:228E `mov ax,9`), so i + shift stays below 16 and no QUIRK is needed; the comment now says this.
- [x] must-fix src/E14EF.C:52: "nonzero disables the mouse (src/E1A72.ASM:282)". Line 282 of E1A72.ASM is `shl cx,1`. The statement that a nonzero ds:814A makes the handler do nothing is E1A72.ASM:300 (the 1A72:017A comment); line 279 sets it to 1 at install. (evidence: src/E1A72.ASM:279, :282, :300) Fixed in 00b90bf: the gMouseDisabled comment now cites the handler at 1A72:017A (it returns at once) and the install store at 1A72:0150 (src/E1A72.ASM:295), not the old line 282.
- [x] must-fix src/E14EF.C:179-180: "left mouse button (src/E1AAD.ASM:363-405)" and "right mouse button". Lines 363-405 show that AL 41h and 42h store the click into ds:8140/ds:8142; they do not say which button. The mapping is at src/E1AAD.ASM:1160-1181 (HandleMouseButtons, 1AAD:0B0E): a changed bit 0 of the button state gives 41h, bit 1 gives 42h. "Left" and "right" are the INT 33h convention [H]. Cite 1AAD:0B0E for the codes and either mark the names [H] or name them by bit. (evidence: src/E1AAD.ASM:1150-1181) Fixed in 00b90bf: the KEY_CLICK/KEY_CLICK2 comments now say mouse button 0/1 changed and cite 1AAD:0B0E for the 0x41/0x42 codes, with the INT 33h left/right names tagged [H]; the enum preamble cites 1AAD:048D for the ds:8140/ds:8142 stores.
- [x] must-fix src/E14EF.C:50-51: g8102 is described as "nonzero: a colour row exists", citing E0BD2.C:100 and E0D1C.C:51. Those are other units' comments, and E0D1C.C:51 cites E07A9.C. Other files give other meanings: src/E07C8.C:27 "graphics choice" (set from the command-line digit at E07C8.C:118), src/E1C0E.C:59 "the video mode the game initialised". This file shows only that a zero g8102 makes the normal colour 1 (:162, :234). Say that, and say the meaning is not established. (evidence: E14EF.C:162, :234; src/E07C8.C:27, :118; src/E1C0E.C:59) Fixed in 00b90bf: the comment now says only that a zero g8102 makes the normal text colour 1 (:177, :252) and that the meaning is not established (E07C8.C:30 calls it the graphics choice, E1C0E.C:59 the video mode).
- [x] should src/E14EF.C:45: g5D1A is documented only as the pointer pick in UpdateOptionList. It also decides whether DrawOptionList draws the selected row in colorB (:163, `g5D1A || gMouseDisabled`). Add that. Fixed in 00b90bf: the g5D1A comment now also says it makes DrawOptionList draw the selected row in colorB (:178).
- [x] should src/E14EF.C:205, :227, :253, :257, :266: `rows` holds the last row index (height - 1), not a count. Name it lastRow; the wrap tests (:253, :257) then read as written. Fixed in 00b90bf: renamed the local to lastRow; make match still 6 of 6.
- [x] should src/E14EF.C:147, :151, :160, :220, :288, :291, :297, :300: `glyph` (and `sel` in DrawOptionList) is the index of the entry in `names`, not a glyph. Name it entry (selEntry for the selected one). Fixed in 00b90bf: renamed the locals to entry (and DrawOptionList's selected index to selEntry).
- [x] should src/E14EF.C:48-49: gFontWidth and gFontHeight are declared `char` here, `unsigned char` in src/E1A47.C:13-14 (the defining file), and `char` in src/EOV0.C:153. Check whether `unsigned char` keeps the match (`make match`, not an edit by the reviewer), and use one type in every file. Fixed in 00b90bf: unsigned char was tried and fails make match (DrawOptionList +005D and UpdateOptionList +0041 differ), so char stays; the declaration comment says why and that E1A47.C declares unsigned char.
- [x] should src/E14EF.C:35: "pad: unused" is not established. Nothing in this unit reads +0Eh, and no other file in src/ names ds:017F; say "not read in this unit". Fixed in 00b90bf: the field comment now says "not read in this unit".
- [x] should src/E14EF.C:133-140: DrawOptionList's comment does not describe its parameters: `names` (the far strings, indexed by entry), `mask` and `shift` (the selectable-entry mask, see the header), `spacing` (the extra pixels between lines, stored in ds:FC78). UpdateOptionList's comment does; match it. Fixed in 00b90bf: the comment now describes names, mask/shift and spacing.
- [x] should src/E14EF.C:67-69: FindMaskBit returns the bit number minus shift (the counter i tests bit i + shift), so the result is an index counted from shift. Say so. "nothing to skip" (:69) is unclear: say that for the mask -1L every entry is selectable, so the n-th entry is n. Fixed in 00b90bf: the comment now says the result is the bit number minus shift (an index counted from shift) and that -1L makes the n-th entry n.
- [x] should src/E14EF.C:190 and :239: 0x8ff is a magic number with a meaning the comment gives (bits 0-7 and bit 11 of the key word). Name it (KEY_CODE_MASK, with its comment) and use the name in both places. Fixed in 00b90bf: added KEY_CODE_MASK to the enum with its comment and used it at :257 (the WaitKeyEvent mask).
- [x] should src/E14EF.C:206, :272, :302, :305: `picked` holds the selectable row (:272, :280) and then, at :302, the string index that is returned. Use a separate variable for the returned index, or say so at the assignment. Fixed in 00b90bf: the comment says picked first holds the row (:290, :298) and then the returned string index (:320), and an inline comment at the reassignment marks it.
- [x] should src/E14EF.C:1-13: the unit header does not cite the whitepaper section. docs/design/WHITEPAPER.md 2.1.1 (lines 47-56) covers this unit's flags and build. Cite it, as READABILITY.md asks. Fixed in 00b90bf: the header now cites whitepaper 2.1.1 for the flags and the assembler build.
- [x] should src/E14EF.C:55: g8134 is "accumulated button changes (E1A72.ASM)". The evidence is src/E1A72.ASM:354-355 (`or ax,[g8134]`, `mov [g8134],ax`), and src/E0ACD.C:119 says the map screen clears it. Cite those. Fixed in 00b90bf: the g8134 comment cites src/E1A72.ASM:370-371 (the or/mov) and src/E0ACD.C:119 (the map screen), and this file's own clear at :184.
- [x] note src/E14EF.C:202 (comment :189-201): `UpdateOptionList` says "update" but its result is the chosen entry and it reads input. The other files use the same name (EOV0.C:125, EOV2.C:181), so a rename needs `make rename` across them; leave it unless a better name is agreed. Left as is in 00b90bf (the name is shared with src/EOV0.C and src/EOV2.C).
- [x] note routed to the E07C8 owner: src/E07C8.C:37 says "ds:814A: 0 disables the mouse handler body", which is the opposite of E07A9.C:204, E1BBE.ASM:42, E1C00.ASM:40, E1A72.ASM:300 and re/globals.toml:302 (nonzero disables). This file follows the majority. Routed to the E07C8 owner; this file follows the majority and was not edited here.
- [x] note routed (other units' stale citations of this file): src/E1BBE.ASM:427 "E14EF.C:44-45" (now :59); src/E1C00.ASM:288 "E14EF.C:227" (now :244); src/E1A72.ASM:52 "E14EF.C:150" (now :169); re/globals.toml:302 "E14EF.C:144 and E14EF.C:227" (now :163 and :244); src/E082C.C:300 still says PointInRect (the name 14EF:00E1 has in this unit is now IsPointInRect); src/EOV1.ASM:2765 says PointInRect in a comment. Routed to the owning units. After this round's edits the cited lines moved again, so those files are still stale (the panel field reads are now :169-:172, DrawOptionList's selected-colour draw :178, the mouse-enabled test :262, the g8134 clear :184) and need the owning unit or the integrator.

## Summary

The unit is understandable from its source alone. DrawOptionList, UpdateOptionList, IsPointInRect,
FindMaskBit, CountMaskBits and BlinkText are named for what the code does, and the round 1 fixes hold.
What still fails is the evidence in the comments: stale line citations (in this file and in the EOV0
calls it cites), the list of callers in the header (one is a decompiled function in src/, two are
stubs), the 16-bit safety claim for CountMaskBits (it covers the shift, not the count), a line cite
for ds:814A that points at `shl cx,1`, the left and right mouse-button names (the cite shows the
codes, not the buttons), and the meaning of ds:8102 (contested across units). The nine must-fix
items fall into those six groups. Placeholders are honest and need no change for now: gFC78 (extra line spacing,
evidenced here), gFC7A (its reader is not yet identified), g5D1A, g8102 (contested), g8134, g8140,
g8142, `counter`, `extra` and `pad`. The shoulds are clarity: the `rows`, `glyph` and `picked` names,
the char type of the font metrics, the DrawOptionList parameters, the 0x8ff constant and the
whitepaper cite. No matcher or spec question; nothing escalated.

---

# Round 1 (kept)

# Review: e14ef

Reviewer: rev-haiku
Commit reviewed: 7b384425e6b41f59ac1980ebdc7f8e5fee1c554e (review/e14ef at start; `make match FILE=src/E14EF.C`: 6 of 6, 1410 bytes; `make readability UNIT=e14ef`: 0 findings)

Playbook 70. Judge what a matching file can still get wrong: names, types, comments, structure.
The matcher already proved the bytes; do not re-prove them, read for meaning.

## Findings

Each finding is a checkbox. `must-fix` blocks `make review-pass` until a decompiler ticks it off
(with the commit that fixes it); `should` and `note` do not block.

- [x] must-fix src/E14EF.C:31: the comment says ds:5D1A "lets the pointer position pick the entry in DrawOptionList". DrawOptionList never reads the pointer. Its only use of ds:5D1A is at :144, where it draws the selected entry in colorB. The pointer pick is in UpdateOptionList at :229 (and only when the mouse is on, :227). Also repeated at :119. (evidence: E14EF.C:144, :227, :229) Fixed: the comment now says UpdateOptionList; DrawOptionList's comment says ds:5D1A only controls the colorB draw.
- [x] must-fix src/E14EF.C:119-120: "when ds:5D1A is set ... or ds:814A is not (the handler runs)" has the polarity of ds:814A backwards. The code at :144 draws the selected entry in colorB when gMouseDisabled is nonzero. ds:814A is nonzero when the mouse is disabled: re/globals.toml:302 ("nonzero disables the mouse"), src/E1A72.ASM:282 ("With DGROUP:814A nonzero it does nothing"), src/E1BBE.ASM:40. (evidence: E14EF.C:144; re/globals.toml:302) Fixed: the comment now says ds:814A is nonzero (the mouse is disabled).
- [x] must-fix src/E14EF.C:48: the WaitKeyEvent declaration says "the low eleven bits are the code". The callers mask the result with 0x8FF (E14EF.C:222, src/EOV5.C:373 and :468). 0x8FF keeps bits 0 to 7 and bit 11 (0x800) and drops bits 8 to 10, so it is not the low eleven bits. WaitKeyEvent itself returns the event unfiltered (src/E1AAD.ASM:529-531 "with no key filter"), so the mask belongs to the callers. Bit 11 is Purpose unknown. (evidence: the 0x8FF masks; src/E1AAD.ASM:529-531) Fixed: the declaration now says returned unfiltered; the callers mask with 0x8FF (bits 0-7 and bit 11).
- [x] must-fix src/E14EF.C:19: the Panel field `sel` is described as "index of the selected entry in the caller's string array". It is not: DrawOptionList passes it to FindMaskBit at :137, which turns a selectable-row number into a string index, and UpdateOptionList stores `row` into it at :287, where `row` is a selectable-row number (its glyph is FindMaskBit(row) at :274). Say: the selected row, counted among the selectable entries. (evidence: E14EF.C:137, :274, :287) Fixed: the field comment now says the selected row, counted among the selectable entries.
- [x] must-fix src/E14EF.C:30: "read by its overlay callers" has no evidence. No file under src/ or tests/ reads ds:FC7A (a case-insensitive grep for fc7a finds only E14EF.C and its negative fixture). The overlay callers in src/ (src/EOV0.C:491, src/EOV2.C:383) read only the return value. Cite the reader by address, or say the global is written here and its reader is not yet identified. (evidence: grep of src/ and tests/) Fixed: the comment now says written here, its reader is not yet identified.
- [x] must-fix src/E14EF.C:152-153: "One game key and one keyboard key share each action" does not match the enum below it. Confirm has three codes (0x2B, 0x3D, 0x61), and src/EOV5.C:421-427 calls them "the three confirm keys"; up and down have one code each. Nothing in the file shows which codes are game keys and which are keyboard keys. Count the codes per action and drop the game/keyboard split unless it is evidenced. (evidence: E14EF.C:155-165; src/EOV5.C:421-427) Fixed: the enum comment now counts the codes per action and drops the game/keyboard split.
- [x] should src/E14EF.C:170-176: the mouse rectangle note leaves out that the pointer pick (:229) also needs the mouse on (`if (!gMouseDisabled)`, :227). Add it. Fixed: the comment now says when ds:5D1A lets the pointer pick and the mouse is enabled (!gMouseDisabled, :227).
- [x] should src/E14EF.C:25-26 and :136 against :223: the Panel x field is documented as "times eight pixels", which is what DrawOptionList does (`<< 3`, :136). The hit rectangle in UpdateOptionList (:223) multiplies x by gFontWidth1A47 instead. The two agree only while the font cell is eight pixels wide. Say which is meant, or say that the rectangle uses the font width. Fixed: the struct and gTextLeft comments now say DrawOptionList uses eight-pixel cells and UpdateOptionList the font width; the UpdateOptionList comment says the rectangle uses the font width.
- [x] should src/E14EF.C:2 and :14: "eight of them" (the panel array) has no evidence in src/, docs/ or re/. The two src callers use panels 1 (src/EOV0.C:475, :490, :491) and 6 (src/EOV2.C:380, :383). Say the array size is not established. Fixed: the header and struct comment now say the array size is not established.
- [x] should src/E14EF.C:115-121: "draws every selectable entry" is not what the loop does. It draws `height` rows (:140), and row i shows the i-th selectable entry (:141). "with the panel colour" also leaves out the rule at :143: when ds:8102 is zero the normal colour is 1, not the panel's colorA. Fixed: the comment now says draws height rows; row i shows the i-th selectable entry, and the normal colour (1 when ds:8102 is zero, else the panel's colorA).
- [x] should src/E14EF.C:36: ds:8102 has no comment, and the colour rule that uses it (:143, :217, "!g8102 ? 1 : colorA") is not explained anywhere in this file. Other units already record it: src/E0BD2.C:100 and src/E0D1C.C:51 say "nonzero: a colour row exists". Other units also call it "the video mode" (src/EOV0.C:101, src/E082C.C:41). That conflict is for whoever renames it; this file should carry the colour-row meaning it uses. Fixed: ds:8102 now carries nonzero: a colour row exists (src/E0BD2.C:100, src/E0D1C.C:51); other units call it the video mode.
- [x] should src/E14EF.C:91 (and the extern at :6-7): PointInRect is a test, and a test reads as a question (READABILITY.md: "a test reads as a question"). Suggested: IsPointInRect. src/E082C.C:300 says its own copy (IsPointInsideRect, 082C:0613, src/E082C.C:465, src/EOV5.C:187) is "the same test as PointInRect at 14EF:00E1". One name for the routine in every file needs `make rename` (a decompiler or integrator step, not a reviewer edit). Fixed: make rename PointInRect -> IsPointInRect (14EF:00E1).
- [x] should src/E14EF.C:34-35: gFontWidth1A47 and gFontHeight1A47 carry a file suffix (the source is src/E1A47.C), not a meaning. They are used in src/E1DE7.C:54-55, src/EOV0.C:121, src/EOV5.C:81. Rename them to gFontWidth and gFontHeight with `make rename`, with the evidence (src/E1A47.C, the "font cell width" comments in E1DE7.C). Fixed: make rename gFontWidth1A47 -> gFontWidth (ds:8100) and gFontHeight1A47 -> gFontHeight (ds:8101).
- [x] should src/E14EF.C:75: the last parameter of BlinkText, `char zero`, names nothing. It is the fifth argument of gDrawText and is always 0 (:283). src/E082C.C:405 and src/E16A5.C:87 call that argument `extra`. Rename it `extra`. Fixed: BlinkText's last parameter is now extra.
- [x] should src/E14EF.C:41 and :43: the gDrawText and DrawShadowText prototypes use a, b, c, d. Use the names the defining files use: src/E16A5.C:87 (`text, x, y, color, extra`) and src/E082C.C:82 (`DrawShadowText(char far *text, int x, int y, int color, int extra)`). Fixed: the prototypes now use text, x, y, color, extra.
- [x] should src/E14EF.C:185-186: UpdateOptionList takes `int counter, int extra` here, but src/EOV0.C:110 and src/EOV2.C:181 declare the third parameter `long counter`. The callers pass 0L (EOV0.C:475, :491; EOV2.C:383), so both views push the same four zero bytes, but the prototypes disagree. Pick one form and say in the comment that a long zero is passed. Fixed: the comment now says Both callers pass a long zero (0L), which fills the same four bytes as this int pair.
- [x] should src/E14EF.C:98-111: CountMaskBits tests the bit with `1 << (i + shift)`, where `1` is an int, while the mask is a long. For a shift of 16 or more the bit is not counted at all (16-bit int). If the original does this (the matched bytes are the 16-bit form), mark it `QUIRK 14EF:0107:` in the comment; the comment says nothing about it. Check the shift values the callers pass before writing the QUIRK; the callers are outside src/ (the packet lists ovl0:207A and ovl0:2238). Fixed: the comment now says the bit test uses the 16-bit 1 << (i + shift); the external callers push shift 0 (disassembly: mov al,0; push ax), so the shift stays below 16 and the 16-bit form is safe. No QUIRK needed.
- [x] should src/E14EF.C:177-184: the build notes (`-B`, TASM with SMART off, the four case tails, the positive form of the KEY_CLICK case) describe the unit's compilation, not what UpdateOptionList does. Move them to the file header or the whitepaper (2.1.1 already covers this, see docs/design/WHITEPAPER.md:47-56), and keep in the function comment only the behaviour. Fixed: the build note moved to the file header after the flags; the function comment now only describes behaviour.
- [x] should src/E14EF.C:156-157 and :164-165: KEY_CONFIRM2, KEY_CONFIRM3, KEY_FIRST2 and KEY_LAST2 name their position in a list, not the key. src/EOV5.C:425-427 uses the same codes with the same suffixes, so keep the confirm names if they are shared, but the enum should say what each code does (confirm, first or last row) and which keys they are (evidence: the switch at :232-268). Fixed: each enum value now carries a comment saying what it does.
- [x] should src/E14EF.C:158-159 and :152-153: "the mouse buttons arrive as KEY_CLICK/KEY_CLICK2" is true, and the evidence is in the event code: src/E1AAD.ASM:363-405 (1AAD:048D: AL 41h and 42h are mouse events that store the click into ds:8140/ds:8142). Cite that address. Fixed: the enum comment now cites src/E1AAD.ASM:363-405.
- [x] should src/E14EF.C:9-10: "The panel records, the mask and the rectangle are all this code's evidence" does not say what evidence is meant. Say that the 16-byte stride and field offsets come from the code at :134-:137, and that the mask and shift come from the callers. Fixed: the header now says the 16-byte stride and field offsets come from :134-:137 and the mask and shift from the callers.
- [x] note src/E14EF.C:137 and :141 and src/EOV0.C:475, :490, :491; src/EOV2.C:380, :383: every src/ caller passes mask 0xffffffff and shift 0. So the general path of FindMaskBit (the bit scan) and CountMaskBits are reached only from callers outside src/ (ovl0:1F66, ovl0:207A, ovl0:2238). Say so in the header. Fixed: the header now says every src/ caller passes mask 0xffffffff and shift 0, so the general path is reached only from outside src/.
- [x] note src/E14EF.C:211: `row = prev = panelDef->sel % (rows + 1)` wraps a stored row into range without saying why it can be out of range. Say why, or that the reason is not established. Fixed: added an inline comment: wrap a stored row into 0..rows; why it can be out of range is not established.
- [x] note src/E14EF.C:208 and :185-186: `counter` is only incremented (:208) and never read, so the name claims a count the code never uses. The comment already says so. Keep the name only as a placeholder, or name it for what is known. Fixed: the comment now calls counter a placeholder.
- [x] note src/EOV0.C:596: the same codes (0x61, 0x2B, 0x3D) are described there as "the enter/left/right keys" and as opening the editor (:639). src/EOV5.C:421-427 and this file call them confirm keys, and this file's code picks the row on them (:260-263). The EOV0.C comment is for the next reader of that file, not this one. Routed to src/EOV0.C; this file's codes are documented in the enum.
- [x] note the packet (build/packets/e14ef.md, "Globals src/ already names") lists ds:80CA as "text (E082C.C)". src/E082C.C:40 and the other files call it gDrawText. The packet is stale on that line; nothing in this file changes. Packet regenerated; still lists ds:80CA as text (E082C.C); no change in this file.

## Summary

The unit is understandable from its source alone, and most names carry their evidence: the panel record layout, the mask scan, the rectangle test, the mouse globals (ds:8134, ds:8138/813A, ds:8140/8142, ds:814A), the text routine ds:80CA, and the calls into 1A16, 1AAD, 1BBE and 082C all agree with the other files of src/. The problems are in the comments, not the names: six must-fix items are comments that say something the code or the other files contradict. They are the ds:5D1A pointer pick (in the wrong function), the ds:814A polarity, the "low eleven bits" mask, the Panel `sel` meaning, the unevidenced reader of ds:FC7A, and the key-count claim. The remaining placeholder names are gFC78, gFC7A, g5D1A, g8102 (its meaning differs between units), g8134, g8140, g8142 and the `counter` parameter. Each of those has either some evidence in the comment or a clear placeholder, so none of them is a must-fix on its own. The shared-name clean-up (gFontWidth1A47, PointInRect versus IsPointInsideRect, the gDrawText parameter names, the counter/extra prototype) needs `make rename` or the integrator, not an edit here.

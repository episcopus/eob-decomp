# Review: e1e8b

# Round 5

Reviewer: rev-haiku
Commit reviewed: 79b728d (main; review/e1e8b was at the same commit when I started)

Playbook 70. Judge what a matching file can still get wrong: names, types, comments, structure.
The matcher already proved the bytes; do not re-prove them, read for meaning.

Checked: `make match FILE=src/E1E8B.ASM` 30 of 30 functions, exit 0 (no source change this round); `make readability UNIT=e1e8b` 0 findings; the 30 function names in re/symbols.toml equal the PUBLIC names in src/E1E8B.ASM. The round 4 fixes hold (02FF carry, 0250 copy, 01F3 file name, 0442 mask, 07D7 second pass, 0580 successor test, 086F entry).
Evidence used: src/E1E8B.ASM (all of it); build/packets/e1e8b.md; eob/EOB.EXE, read only: the bytes at 1E8B:0004, 0025, 011F, 0498, 049E, 04A2, 08C9 and 090B, and the MZ relocation table (entries at 1E8B:0004 and 1E8B:0006); build/re/rtl_compare.json (tc100 symbols: `__OverlayHalt` 20543 = 0x503F, `_abort` 497 = 0x1F1, `_farmalloc` 29406 = 0x72DE); build/re/rtl/tc100/l/r.map (`__OverlayHalt` in the library map); re/symbols.toml.

## Findings

Each finding is a checkbox. `must-fix` blocks `make review-pass` until a decompiler ticks it off
(with the commit that fixes it); `should` and `note` do not block.

### File header and the EXTRN

- [x] must-fix src/E1E8B.ASM:18 (and :786, :794, :903, :911): `_OverlayHalt` is not the runtime's name. The pinned runtime (build/re/rtl_compare.json, tc100 symbols) has `__OverlayHalt` at 20543, which is 0x503F, the address the EXTRN comment cites. build/re/rtl/tc100/l/r.map also lists `__OverlayHalt`. The file's own comments at :786, :794, :903 and :911 use `__OverlayHalt`. Use `__OverlayHalt` in the EXTRN and the comments. If `make match FILE=src/E1E8B.ASM` changes with the EXTRN name, that is a matcher question: escalate, do not force the bytes. [fixed: the EXTRN and the two code jumps now use `__OverlayHalt` (the comments already did); `make match FILE=src/E1E8B.ASM` 30 of 30, exit 0, so the rename did not move a byte.]
- [x] must-fix src/E1E8B.ASM:29-30: "(EOB.EXE loads at paragraph 0)" is wrong. DOS loads an EXE above its PSP, not at paragraph 0. eob/EOB.EXE has MZ relocation entries at 1E8B:0004 and 1E8B:0006, so the loader adds the load paragraph to the words 20B5 and 212E. The file holds the image-relative values, and the map's addresses are image-relative. Say that, with the two relocation entries as the evidence. [fixed: the paragraph now says the file holds the image-relative words and the MZ relocation entries at 1E8B:0004/0006 make the loader add the load paragraph.]

### 000F InitOverlayManager

- [x] must-fix src/E1E8B.ASM:51-52 and :158-163: "the init closes the file with the dead 0FFFBh" names the wrong handle. On that path `init_closefail` closes BX, and BX holds the free paragraph count (`mov bx,ds:[0B6h]`, `sub bx,ds:[0B4h]`, :158-159), not the overlay file handle that 0134 stored at 20B5:00B8 (:223). That handle stays open. Say that the DOS close is on BX and that the handle is left open, and extend the `QUIRK 1E8B:000F` note (:57-60) to say so. The other `init_closefail` paths do close the handle, because BX is set at :95 and kept through the reads. [fixed: the header now says init_closefail closes BX = the free paragraph count on the free-area path and leaves the handle at 20B5:00B8 open, and QUIRK 1E8B:000F says so.]
- [x] should src/E1E8B.ASM:49: "It then swaps the overlay file open again" leaves out the close of the program file just before the swap (`mov ah,3Eh; int 21h`, :147-148) and the clear of 20B5:00B8 (:149). The swap then opens 20B5:001C (the name copied by 01B3, 01A7 or 01F3) and stores the new handle at 20B5:00B8 (:220-223). Say that. [fixed: the header now gives the INT 21h/3Eh close of the program file and the clear of 20B5:00B8 before the swap, and says the swap opens 20B5:001C and stores the new handle at 20B5:00B8.]

### 0442 RelocateOverlayPointer (the loader tail)

- [x] must-fix src/E1E8B.ASM:762: "rel_out2: ; 1E8B:049E: the not-found exit" has the wrong address. The `jmp short rel_out2` at 1E8B:0498 is `EB 06`, which lands on 1E8B:04A0, the second `pop cx; ret`. 1E8B:049E is `rel_out` (:759), the exit for the early mismatches and for a found slot. Write 1E8B:04A0. [fixed: rel_out2 is now labelled 1E8B:04A0.]

### 03FD ApplyOverlayRelocations

- [x] should src/E1E8B.ASM:656-657 and :660: "ES:0008 is the byte offset of the fixup table from the image base" and "(index times 8)" are unclear. ES:0008 is the code size in 077D (:1251) and 0789, and the fixup table follows the code (04A2 reads ES:0008 plus ES:000A bytes, :767-769). Say "ES:0008 is the code size, which is also the offset of the fixup table". At :660 the entry's byte offset is the word with its low three bits cleared (`and di,0FFF8h`, :691): write that, "8 times the entry number", instead of "(index times 8)". [fixed: 03FD now says ES:0008 is the code size, which is also the fixup table's offset, and that the entry's byte offset is 8 times the entry number (`and di,0FFF8h`).]
### 02FF MapOverlayRecords

- [x] should src/E1E8B.ASM:514-518: the walk also stops at a record whose end passes the free area's top: `cmp ax,di; ja chain_tail` (:547-548). The header names only the 0012 test (`jcxz chain_tail`, :538). Say both exits. [fixed: 02FF now gives both walk exits: the zero word 0012 (`jcxz chain_tail`) and the end past the free area's top (`cmp ax,di; ja chain_tail`).]

### 0536 AgeOverlayRecords

- [x] should src/E1E8B.ASM:850-851: "takes the total paragraph count from 0789 in DX" does not say whose total. 0789 reads ES on entry, which is the record 0580 is loading (ES is unchanged from 0580's entry, :923), not the chain head. Say so. [fixed: 0536 now says 0789 reads ES, the record 0580 is loading, not the chain head.]

### 0580 LoadOverlayRecord

- [x] must-fix src/E1E8B.ASM:900-901: "(whose returned top it stores at word 0Eh of the header paragraph just below that top, DS = top - 1; 07A1 reads that back link)". The value stored is the record's segment ES (`mov ds:[0Eh],es`, :927). The returned top only sets DS (`dec ax; mov ds,ax`, :925-926). Say: stores ES at word 0Eh of the header paragraph just below the top, with DS = top - 1. [fixed: 0580 now says the record's segment ES is stored at word 0Eh of the header paragraph just below 0536's returned top, and that the returned top only sets DS.]

### 07D7 DropStaleOverlaySlots

- [x] must-fix src/E1E8B.ASM:1313: "It walks the caller's slot table at SS:BP" is wrong. `push bp; mov bp,sp` (:1326-1327) and `mov bx,bp` (:1334) start the walk at 07D7's own frame. The first link (`ss:[bx]`) is the caller's saved BP, and the first entry's offset and segment are 07D7's own return IP and CS. The caller's BP is not used. Say that, and that the walk follows the saved-BP links up the stack from that frame. [fixed: 07D7 now says the walk starts at its own frame (`push bp; mov bp,sp; mov bx,bp`), the first link is the caller's saved BP, the first entry's offset/segment are 07D7's own return IP/CS, and the caller's BP is not used.]

### 068D ClearOverlayVersionTable

- [x] should src/E1E8B.ASM:1070-1071: "retargets the matching slot and returns the slot's previous offset in CX" leaves out two effects. CX is zero on entry (`xor cx,cx`, :1083), so 072F sets the slot's offset to zero, and 073B sets the slot's segment to ES. Say both. [fixed: 068D now says CX = 0 on entry, so 073B stores ES into the matching slots and 072F swaps zero into the slot's offset, and CX returns the previous offset.]

### 073B FindOverlaySlot and 0711 LinkOverlayRecord (names)

- [x] should src/E1E8B.ASM:1199-1200 (073B): the name says only "find", but the effect that 06C0 and 072F depend on is storing AX into every matching slot (`mov [bp+4],ax`, :1215). The header says so; the name does not. Name it for the store, or accept the name and say in the header that the store is the point. [fixed: 073B now says the store of AX into every matching slot is the point (what 06C0 and 072F depend on).]
- [x] should src/E1E8B.ASM:1158 (0711): the name says only "link", but the routine first adds the record's size to the running top 20B5:00B0 (`add word ptr ds:[0B0h],ax`, :1161). The header says so; the name does not. Same choice as 073B. [fixed: 0711 now says the add to 20B5:00B0 is the point of the first half.]

### 086F OverlayManagerEntry

- [x] should src/E1E8B.ASM:1402: "On the reached exits it stores ES:0010 in [bp+8]" is not true of the id-mismatch exit, because `jne entry_out` (:1423) skips the store. Say "after the id test passes, the load or call path stores ES:0010 in [bp+8]". [fixed: 086F now says the load or call path stores ES:0010 in [bp+8] after the id test passes, and the id-mismatch exit skips it.]

### 01B3 and 01F3 (the path copies)

- [x] should src/E1E8B.ASM:305-313 (01B3 `own_copy`) and :363-371 (01F3 `path_dir`): both copy until NUL or ';' with no length limit into 20B5:001C. The 132 bytes from 20B5:001C up to 20B5:00A0 (the overlay id) are the room, so a path longer than that overruns the manager's words. The 12-byte limit of 0250 does not apply to these copies, and no header says so. Not established whether the original does the same: say that, or mark the case as a QUIRK if the path length is bounded elsewhere. [fixed: 01B3 and 01F3 now say their copies have no length limit and can overrun the 132 bytes from 20B5:001C to 20B5:00A0, and that whether the original bounds them elsewhere is not established.]

### File level: QUIRK marks (READABILITY item 6)

- [x] must-fix: behaviour a port must keep, which looks wrong, is not marked `QUIRK <address>:`. Mark each with what happens: (a) the 0FFFBh close on BX in 000F (:158-163, see above); (b) 02FF: the last record of the 0012 chain is never placed and the last placed record's link is cleared (:514-518, :538, :555-557); (c) 0580: the last chain record is never examined, counted or cleared (header :906-911, code :940-942); (d) 07D7: the second pass reads past CX when the first pass stopped at the table end (:1316-1319, :1369-1371). The existing `QUIRK 1E8B:000F` note covers only the dead CX codes. [fixed: QUIRK 1E8B:02FF, QUIRK 1E8B:0580 and QUIRK 1E8B:07D7 added, and QUIRK 1E8B:000F extended for the close on BX.]

### Notes

- [x] note: with 20B5:00BC zero, the 0536 no-head fallback can call 0613 when DX > AX (CF from 0761 is set on that path, :1243-1245, and the test is :889-890 with the popf at :867). Then 0613 starts with ES = 0000 and reads 0000:001C (:994-1002), and 0580's chain walk does the same from `mov es,ds:[0BCh]` (:939). Whether that state is reachable is not established: 000F's check keeps the record total below the free area, but not the running top 20B5:00B0. Say so in 0613 and 0580, or mark it. [fixed: 0613 and 0580 now note the 00BC-zero case (ES = 0000, reads 0000:001C) and that reachability is not established.]
- [x] note: the 20B5 offsets in the ASM (00A0, 00B0, 00BC, and the rest) are raw constants whose meanings live only in comments. Leave that for phase 6, when the layout can be written as EQUs or a struct. [fixed: no change; left for phase 6, as the review says.]

## Confirmed, no change

- 000F: 1E8B:0025 is the `jmp init_ok` (bytes e9), and 1E8B:011F is the `mov bx,ds:[0B8h]` of the map-failure exit (bytes 8b 1e b8 00), as the header says.
- 0004: the words at 1E8B:0004 and 0006 (b5 20, 2e 21) and "PATH=" at 1E8B:0008 match the header. The relocation point is the finding above.
- 086F: the bootstrap is at 1E8B:08C9 (1e 56 57 b8 b5 20 8e d8: push ds, push si, push di, mov ax,20B5h, mov ds,ax), and the far call at 1E8B:090B (9a 0f 00) is the call to _InitOverlayManager, so the NOSMART note at :1413 is right.
- 04A2: the loader is at 1E8B:04A2 (26 8b 36 08 00, mov si,es:[8]), and the 0442 tail ends at 04D0, as the header says.
- 0580: the successor loop adds nothing when the use counter is non-zero (`xor ax,ax`, :947-950), as the header says.
- 0536: the 077D call is overwritten by 0761, as the round 4 note says.
- 0761, 0789, 077D and 07A1: arithmetic and return values match their headers.
- 03C4 and 03B7 (uncalled): headers match the code.

## Summary

Round 5: the unit reads as the overlay manager it is, and the round 4 fixes hold. Seven must-fix items: the EXTRN name (the runtime's `__OverlayHalt`, not `_OverlayHalt`); the 0004 paragraph's claim that EOB.EXE loads at paragraph 0 (the MZ table relocates the two words); a close on the wrong handle in 000F; a wrong address in the 0442 tail (1E8B:04A0, not 049E); the 0580 stored value (ES, not the top); the 07D7 walk start (its own frame, not the caller's BP); and the QUIRK marks that READABILITY asks for. Nine should items are clarity fixes. The placeholder names are all marked: the only "Purpose unknown" items are 08C9 (marked [H]) and the 0185 pointers (the writer is not in the unit). Nothing is escalated, except that a rename of the EXTRN could change the match, which the decompiler must check. Verdict: review-failed, 7 must-fix items, all comments, names or quirk marks; no code change is asked for unless `make match` moves with the EXTRN name.

# Round 4

Reviewer: rev-haiku
Commit reviewed: 4afb5a6 (main; review/e1e8b was at the same commit when I started)

Playbook 70. Judge what a matching file can still get wrong: names, types, comments, structure.
The matcher already proved the bytes; do not re-prove them, read for meaning.

Checked: `make match FILE=src/E1E8B.ASM` 30 of 30 functions, exit 0; `make readability UNIT=e1e8b` 0 findings; the names in re/symbols.toml for the 30 functions equal the PUBLIC names in src/E1E8B.ASM. The round 3 fixes hold (0789, 03FD, 0536, 05FB, 0580's back link, 000F, 0286, 04D0).
Evidence used: src/E1E8B.ASM (all of it); build/packets/e1e8b.md (callers, from `make packet UNIT=e1e8b`); eob/EOB.EXE, read only: 1E8B:0004 holds B5 20 2E 21 "PATH", and 20C1:0318 holds "eob.exe" (99 entries of 8 bytes end at 0318h); build/toolchain/tc100/STARTUP/C0.ASM:164-165 and :675-677 (_envseg, _psp); build/re/rtl/tc100/l/r.map:1549 and :1640 (__envseg at DGROUP:0075, __psp at DGROUP:0079). Searched src/, re/ and docs/ for any caller of 1E8B:08C9 and for overlay stubs that call 1E8B:086F: none.

## Findings

Each finding is a checkbox. `must-fix` blocks `make review-pass` until a decompiler ticks it off
(with the commit that fixes it); `should` and `note` do not block.

### 02FF MapOverlayRecords

- [x] must-fix src/E1E8B.ASM:515: "Returns carry set on a read or overlay failure" claims an overlay failure that the code never reports. Carry is set only when the image read fails (`jb chain_ret` after `_ReadOverlayImage`, :567). The overlay entry call (:585) is followed by `or ax,ax` (:588), and the normal exit clears carry (`clc`, :592; `jmp chain_out`, :555). Say: carry set when the image read fails (03C4). [fixed in 91d0378: the 02FF header now says "Returns carry set when the image read fails (03C4)" and drops the overlay-failure claim.]
- [x] must-fix src/E1E8B.ASM:506-507: "It takes the free segment 20B5:00B2" names 00B2 as free memory with no evidence. The code reads 20B5:00B2 as the first record (`mov cx,ds:[0B2h]`, :521) and stores it as the chain head 20B5:00BC (:522), then reads that record's fields (:529 to :533). The free area is 20B5:00B4 to 00B6 (:524 to :525), and the image is read into segment 00B4 (AX from :552, the call at :566). Say that 00B2 is the first record of the chain, set outside this unit. [fixed in 91d0378: the 02FF header now reads 00B2 as "the first record of the chain, set outside this unit", stores it as the head 20B5:00BC, and names the free area 20B5:00B4 to 00B6 with the image read into segment 00B4.]
- [x] must-fix src/E1E8B.ASM:509-510: "links each record to its successor ... fitting each" does not match the walk. A record is placed only when its own word 0012 is non-zero (`jcxz chain_tail`, :530), so the last record of the 0012 chain is never placed. The last placed record then has its link cleared (`mov word ptr ds:[1Ch],0`, :549), which drops its link to that unplaced record. Each record's size comes from its successor's offset (:532 to :537), so the unplaced record is the end of the image. That reads as an end marker, but this is not established. Say what the walk places and what it leaves out. [fixed in 91d0378: the 02FF header now says a record is placed only when its own word 0012 is non-zero (jcxz chain_tail), so the last record of the 0012 chain is never placed and the last placed record's own +1Ch is cleared, dropping its link; each placed record's size comes from its successor's offset, and the unplaced record reads as the end of the image but that is not established.]
- [x] should src/E1E8B.ASM:512 and :604: "SI:DI" puts the high word second. The code makes SI the low word and DI the high word (`mov di,si; and di,0Fh; and si,0FFF0h`, :560 to :562; 03C4 takes SI as the count when DI is zero, :626 to :628). Elsewhere the file writes high:low (DX:AX in the 03B7 tail, :595). Say "SI is the low word, DI the high word" in both headers. [fixed in 91d0378: the 02FF header now says "SI is the low word, DI the high word" and 03C4's header now opens "read SI:DI bytes (SI is the low word, DI the high word)".]

### 0250 CopyNameAndOpenOverlay

- [x] must-fix src/E1E8B.ASM:392: "into the 20B5:001C buffer" is true only for 01A7. The copy goes to ES:DI (`stosb`, :403 and :408). 01B3 passes DI = the offset past the last backslash (`mov di,bx`, :315), and 01F3 passes DI = the offset after the directory it has copied (:358 to :379). Only the open always uses 20B5:001C (`lea dx,ds:[1Ch]`, :410). Say the copy goes to ES:DI and the open is of 20B5:001C. [fixed in 91d0378: the 0250 header now says the copy goes to ES:DI (01A7 passes DI = 001C, 01B3 the offset past the last backslash, 01F3 the offset after the copied directory) and that only the open always uses 20B5:001C.]

### 01F3 OverlayFileViaPath

- [x] must-fix src/E1E8B.ASM:325-326: the header says the directory is copied into 20B5:001C and then "calling 0250 to open the file". The 0250 call (:383) also copies the caller's name, through the far pointer at [bp+6] (`lds si,[bp+6]` in 0250, :396), after the directory. The file opened is the directory plus that name. Say so; as written a reader takes the directory itself for the file. [fixed in 91d0378: the 01F3 header now says 0250 also copies the caller's name (far pointer [bp+6]) after the copied directory, so 20B5:001C holds directory plus name and is the file 0250 opens.]
- [x] should src/E1E8B.ASM:323-325: the header paragraph is ragged ("bytes equal the "PATH=" literal" is one short line). Rewrap it to the unit's width. [fixed in 91d0378: the 01F3 header is rewrapped; the short "PATH=" literal line is gone.]

### 0442 RelocateOverlayPointer

- [x] must-fix src/E1E8B.ASM:700-702: "must both be 0B8h" and "the same masked 50h (`push ax`)" are not what the code checks. The mask 0F807h keeps the low three bits, which are the register field of `mov r16,imm16` (B8+r) and `push r16` (50+r). The code accepts any register r, as long as all four bytes use the same one: `cmp al,cl` (:724), `cmp cx,ax` (:730), `cmp ax,dx` (:735). Say: B8+r at ES:BX-1 and ES:BX+3, 50+r at ES:BX+2 and ES:BX+6, one r in all four (`mov ax` and `push ax` are the r = 0 case). [fixed in 91d0378: the 0442 header now says the mask 0F807h keeps the opcode and the low three bits, the register field, and describes B8+r at ES:BX-1/ES:BX+3 and 50+r at ES:BX+2/ES:BX+6 with one r in all four (cmp al,cl / cmp cx,ax / cmp ax,dx; mov ax and push ax are r = 0).]

### 07D7 DropStaleOverlaySlots

- [x] must-fix src/E1E8B.ASM:1299-1300: "When a record was marked, it walks the table again" is wrong about where the second pass starts. `drop_second` (:1348) does not reset BX to SS:BP: it reads SS:[BX] where the first pass stopped (:1323 or :1326). If the first pass stopped on a zero link, the second pass reads that same link and leaves at once (`je drop_out`, :1351). If it stopped at the table end, the second pass reads past CX, since the `cmp bx,cx` test (:1322) is only in the first pass. Say where the second pass starts and what it visits. [fixed in 91d0378: the 07D7 header now says the second pass starts at BX where the first pass stopped (not reset to SS:BP): on a zero link it reads that link and leaves at once, at the table end it reads past CX (the cmp bx,cx test is only in the first pass).]

### 0580 LoadOverlayRecord

- [x] must-fix src/E1E8B.ASM:891-893: "walks the overlay's entry chain ... each record whose use counter ES:001B is zero adds its code paragraphs" is wrong for the last record. `load_chain` tests the successor before the current record: `mov cx,es:[1Ch]` and `jcxz load_out` (:925 to :926) come first, so the record whose ES:001C is zero is never counted or cleared. The walk starts at the head 20B5:00BC (:923). Say that each record with a successor is examined, and that the last one is not (why is not established). [fixed in 91d0378: the 0580 header now says the successor ES:001C is tested before the current record, so only a record whose ES:001C is non-zero is examined and the last record of the chain is never counted or cleared (why is not established).]

### 086F OverlayManagerEntry

- [x] must-fix src/E1E8B.ASM:1376-1377 and :1383-1384: "reached from the overlay stubs" and "08C9 is a second entry of this same PROC, the program's overlay-memory bootstrap" have no evidence in the repo. build/packets/e1e8b.md gives "(no direct call found)" for 1E8B:086F. No file in src/, re/ or docs/ refers to 1E8B:08C9 or to a stub that calls 086F, and nothing in this unit calls or jumps to 08C9 (it has no label). Say "entered from outside this unit; no caller found in the repo", and mark the bootstrap role [H] or Purpose unknown. The body from :1432 to :1476 does match the description (the paragraph count, the farmalloc call, the arguments at [bp+6] to [bp+0Ch]). [fixed in 91d0378: the 086F header now says it is "entered from outside this unit" (the packet finds no direct caller; no file in src/, re/ or docs/ names a stub or 08C9), and the 08C9 bootstrap role is marked "Purpose unknown [H]".]

### Other headers

- [x] should src/E1E8B.ASM:422 (0276): "000F keeps its handle at 20B5:00B8 and moves it to BX before each call" is not what 000F does. It sets BX once from the open handle (`mov bx,ax`, :95), and both calls (:98, :124) rely on BX surviving the seek (INT 21h/4200h does not change BX). Say it once. [fixed in 91d0378: the 0276 header now says 000F sets BX once from the open handle (mov bx,ax) and both calls rely on BX surviving the seek; the "moves it to BX before each call" sentence is gone.]
- [x] should src/E1E8B.ASM:776-782 (04D0): the header does not say the routine receives BX. `pop bx` at :820 takes back the request word pushed at :805 (`push word ptr es:[bx]`), so the call at :826 passes AX = bit 3 and BX = the request word. Say so. [fixed in 91d0378: the 04D0 header now says the 20B5:0016 call passes AX = the bit (cbw) and BX = the request word (pop bx takes back the word pushed at ES:[BX]).]
- [x] should src/E1E8B.ASM:783 (04D0 evidence line): "the ES:001A bit 8 read and clear" names a different bit from the header (:780 to :781: bit 3, value 8). Write "bit 3 (value 8)". [fixed in 91d0378: the 04D0 evidence line now says "the ES:001A bit 3 (value 8) read and clear".]
- [x] should src/E1E8B.ASM:844 (0536): "takes its code paragraphs (077D)" suggests the value is used. The AX from 077D (:867) is overwritten by 0761 in `age_test` (:876), and 077D only computes AX, so this call has no effect. Say so, or drop it from the description. [fixed in 91d0378: the 0536 header now says 077D's AX is overwritten by 0761 at age_test, so the call has no effect.]
- [x] should src/E1E8B.ASM:888 and :894 (0580): "marks the record used or unused (ES:001B/ES:001A)" omits that the loaded path sets ES:001B to 1 (:901) and ES:001A bit 2 (value 4, :902), that the unused path sets bit 3 (value 8, :906), and that ES:001B gets the low two bits of ES:001A added (`add byte ptr es:[1Bh],al`, :919 to :920). "A failed overlay routine jumps to __OverlayHalt" (:894) is the carry of the loader at ES:0018 (`call word ptr es:[18h]; jb load_halt`, :913 to :914), not an overlay routine. [fixed in 91d0378: the 0580 header now gives the loaded path (ES:001B = 1, ES:001A bit 2, value 4), the unused path (bit 3, value 8), the low-two-bits add to ES:001B, and says the carry to __OverlayHalt is the ES:0018 loader's, not an overlay routine's.]
- [x] should src/E1E8B.ASM:1234-1235 (077D): "The 11h is the 10h record header plus one more byte" states a 10h header without evidence. The 10h-byte header this file reads is the FBOV header of 000F (:123 to :124); whether the 11h includes that header is not shown. Mark [H] or say not established. [fixed in 91d0378: the 077D header now marks "why 11h is added is not established [H]" instead of asserting a 10h record header, and notes it is not shown that the 11h includes the FBOV header 000F reads.]
- [x] should src/E1E8B.ASM:651-652 (03FD): "the fixup word" names two different words: the table entry read by `lodsw` (:675), which is the offset of the word to fix, and the value at ES:BX (`mov di,es:[bx]`, :677) whose bit 0 is tested (:685). Name each once ("table entry", "segment-table index word") and use those names in the flag sentence. [fixed in 91d0378: the 03FD header now names "table entry" (the lodsw offset) and "segment-table index word" (the value at ES:BX) and uses those names in the bit-0 sentence.]
- [x] should src/E1E8B.ASM:1178-1179 (073B): "a set low bit only skips that entry's match test" omits that a link word of 0 or 1 ends the walk: `shr cx,1` then `je slot_out` (:1193 to :1194) run before the low-bit test. Say it. [fixed in 91d0378: the 073B header now says a link word of 0 or 1 ends the walk (shr cx,1 then je slot_out, before the low-bit test).]

## Confirmed, no change

- 0185: no function in this unit writes 20B5:0012 or 20B5:0014 (the only word-0012 store, :475, is to ES = 20BF).
- 000F: the 1E8B:0004 data words and the "eob.exe" default name at 20C1:0318 match the header (checked against the bytes of eob/EOB.EXE).
- 0286: _psp at DGROUP:0079 (C0.ASM:677, r.map:1640) and the 99-entry table at 20B5:00C0 (`cmp si,3D8h`) match the header.
- 066F and 068D: the 5-byte stub form (CD 3F at +0, offset at +2, zero at +4) and the far-jump form (0EAh, offset at +1, segment at +3) match the code.
- 06C0 (back link at word 0Eh, the 073B call with AX = new base and DX = old base), 072F, 064E control flow, 0613 relink, 0761, 077D and 0789 arithmetic, 05FB, and the 086F load condition all match their headers.
- 03B7 and 07A1: no caller in src/ or the packet; "uncalled in this unit" stands.

## Summary

Round 4: the unit reads as the overlay manager it is, and the round 3 fixes hold. Nine must-fix items, all header claims: a carry claim in 02FF; the 02FF free-segment name and the record walk that leaves the last 0012 record unplaced; the 0250 copy destination and the 01F3 file name; the 0442 register test; the 07D7 second-pass start; the 0580 walk that skips the last record; and the 086F/08C9 entry claims with no caller evidence. No placeholder name is left without a "Purpose unknown" note. There is no matcher, spec or pin question: the odd behaviour (the unplaced 02FF record, the 0580 last record, the 07D7 second pass) is in the original code and is to be described, not changed, so nothing is escalated. Verdict: review-failed, 9 must-fix items, all comment corrections (no code change is asked for).

# Round 3

Reviewer: rev-haiku
Commit reviewed: 6e0c980 (main; review/e1e8b was at the same commit when I started)

Playbook 70. Judge what a matching file can still get wrong: names, types, comments, structure.
The matcher already proved the bytes; do not re-prove them, read for meaning.

Checked: `make match FILE=src/E1E8B.ASM` 30 of 30 functions, exit 0; `make readability UNIT=e1e8b` 0 findings. The round 2 fixes hold: I checked the 086F, 064E, 068D, 073B, 07A1 and 0761 headers against their code. This round read every function's header against its code and checked the record fields against the layout in the whitepaper.
Evidence used: src/E1E8B.ASM (all of it), docs/design/WHITEPAPER.md (2.2, 2.3 and the stub layout, the fixup note at line 100), build/packets/e1e8b.md (callers per function). 03B7 and 07A1 are not map entries and no src/ file calls them.

## Findings

Each finding is a checkbox. `must-fix` blocks `make review-pass` until a decompiler ticks it off
(with the commit that fixes it); `should` and `note` do not block.

### Record fields ES:0008 to ES:000C

- [x] must-fix src/E1E8B.ASM:1235-1237 (0789): "the data paragraphs (ES:000A plus 0Fh, shifted right by 4)" names ES:000A as data. The overlay record is "code, u16 fixup offsets" (WHITEPAPER 2.3, the +10 row). The stub layout there is code bytes, fixup bytes, entry count, and this unit reads those at ES:0008 (code size, 077D and 0789), ES:000A and ES:000C (entry count, 064E, 066F, 068D). So ES:000A is the byte size of the fixup table. 04A2 loads it with the code (it reads ES:0008+ES:000A bytes, :752-754), and 03FD walks it (`shr cx,1`, :668). Write "the fixup table" for ES:000A and "code plus fixup table" for the total. The 0286 comparison of totals is unchanged. [fixed in dacdf20: the 0789 header now says "DX gets the total, code plus fixup table: the code value plus the fixup table's paragraph count (the fixup byte size ES:000A plus 0Fh, shifted right by 4)"; the 0286 comparison is untouched.]
- [x] must-fix src/E1E8B.ASM:644-646 (03FD): "ES:000A is the count" and "walks the CX words" are wrong about the unit. ES:000A is a byte size: 02FF passes it as CX (:570), and 03FD halves it (`shr cx,1`, :668) before the loop, so CX is the number of fixup words, ES:000A/2. Say so. [fixed in dacdf20: the 03FD header now says it walks CX fixup words, CX being half the byte size at ES:000A (02FF loads it, `shr cx,1` makes CX the number of fixup words, ES:000A/2).]

### The chain: 0536, 0580, 05FB

- [x] must-fix src/E1E8B.ASM:835-836 (0536): "when 0761 signals its fallback (no current overlay), discards the whole chain (0613) first" gives the wrong trigger. The carry from 0761 is set on two paths: no head (20B5:00BC zero) and a borrow when the head's base is below 20B5:00B0 (`sub ax,ds:[0B0h]` then `jae`, :1212-1217; the 0761 header, :1198-1204, says the borrow is the case for a linked record). 0536 tests that carry (`popf`, `jae age_replace`, :848-851), so the discard runs on both paths. Write both. [fixed in dacdf20: the 0536 header now gives both triggers: the carry from 0761 is set on no head (20B5:00BC zero) and on a borrow when the head's base ES:0010 is below the running top 20B5:00B0.]
- [x] should src/E1E8B.ASM:945-946 (05FB): "Called from 0536 when the record's use counter ES:001B reaches zero." 0536 calls 05FB when ES:001B is already zero (`cmp byte ptr es:[1Bh],0`, `jne age_dec`, then the call, :856-858); the counter is decremented only on the other branch (:862). Write "when ES:001B is zero". [fixed in dacdf20: the 05FB header now says "Called from 0536 when ES:001B is zero", matching the `cmp ... 0; jne age_dec` test.]
- [x] should src/E1E8B.ASM:882-883 (0580): "while 0761 reports paragraphs remaining below the 20B5:00A8 limit". The compared value is not 0761's result alone: it is 0761's value plus the code paragraphs already added in this loop (`add ax,cx`, :932; `cmp ax,ds:[0A8h]`, :918). Say "the running paragraph count, starting from 0761's value". [fixed in dacdf20: the 0580 header now says the loop runs while "the running paragraph count" stays below 20B5:00A8, and that the count starts at 0761's value.]
- [x] note src/E1E8B.ASM:899-903 (0580): the store of the record segment at word 0Eh of the header paragraph below the new base (`dec ax; mov ds,ax; mov ds:[0Eh],es`) is not in the header. 07A1 reads that back link (:1264-1266). Add one clause. [fixed in dacdf20: the 0580 header now says the returned top is stored at word 0Eh of the header paragraph just below that top (DS = top - 1) and that 07A1 reads that back link.]

### 02FF, 000F: wording and the free area

- [x] should src/E1E8B.ASM:508 (02FF): "(20B5:001C, named in older text here, is the file-name buffer of 01A7, ...)" refers to the comment's history. Say "20B5:001C is the file-name buffer of 01A7, not part of this walk" and drop "named in older text here". [fixed in dacdf20: the 02FF header now reads "20B5:001C is the file-name buffer of 01A7, not part of this walk", the history phrase is gone.]
- [x] should src/E1E8B.ASM:49-50 (000F): "sizes the free area from the arguments at [bp+0Ah]/[bp+0Ch] (_InitOverlayRecords)" credits _InitOverlayRecords with the free area. 000F sets the free-area base 20B5:00B4 to [bp+0Ch]+1 and the top 20B5:00B6 to [bp+0Ah] (:148-153). _InitOverlayRecords sets 20B5:00AA (:499). The header also omits the check that follows (:155-164): when the free paragraphs are below 00AA the init closes the file with the dead 0FFFBh, and 20B5:00A8 is set to a quarter of the free paragraphs, the limit 0580 uses. Add both. [fixed in dacdf20: the 000F header now gives the free-area stores to 000F itself (base 20B5:00B4 = [bp+0Ch]+1, top 20B5:00B6 = [bp+0Ah]), keeps 00AA with _InitOverlayRecords, and adds the below-00AA close and the quarter-of-free 00A8 limit.]

### 03FD and the fixup flag

- [x] must-fix src/E1E8B.ASM:649 (03FD): "when the index's low bit is set" names the wrong bit. The index is the fixup word with its low three bits cleared (`and di,0FFF8h`, :678), so the index has no low bit of its own. The test is bit 0 of the fixup word itself (`mov ax,di` before the mask, :677; `test ax,1`, :681). Say that, and mark what bit 0 means as not established (the whitepaper does not say). [fixed in dacdf20: the 03FD header now says the test is bit 0 of the fixup word itself (the value read at ES:BX, before `and di,0FFF8h`) and marks what bit 0 means as not established.]
- [x] should src/E1E8B.ASM:439-448 (0286): the header paragraph is ragged ("...while ES", "is set to the 20BF list head)", "(1E8B:04A2), adds the"). Rewrap it to the unit's width. [fixed in dacdf20: the 0286 header is rewrapped to the unit's width, no ragged lines.]

### Other headers

- [x] should src/E1E8B.ASM:773-774 (04D0): "either the single entry at [bp+2] or the two-word form at [bp+6]/[bp+2]" does not say which test picks the path. It is `sub word ptr [bp+2],2` then `jne handler_multi` (:800-801): the single path is the one where the decremented [bp+2] is zero. Say that, and mark what the two-word form means as not shown in this unit. [fixed in dacdf20: the 04D0 header now says `sub word ptr [bp+2],2; jne handler_multi`: a zero [bp+2] is the single entry, a non-zero the two-word form, and marks what that form means as not shown in this unit.]
- [x] should src/E1E8B.ASM:1018 (066F) and :1046 (068D): "fill the CX overlay-version records" and "rewrites the CX version records" read CX as an input. Both routines load the count from ES:000C (:1027, :1059) and overwrite CX; the caller's CX is not used. Name ES:000C (the entry count, WHITEPAPER 2.3 stub layout) in both. [fixed in dacdf20: the 066F header now says ES:000C entries (loaded from ES:000C, overwriting CX), and the 068D header says the ES:000C version records (the count is loaded from ES:000C, overwriting CX).]
- [x] should src/E1E8B.ASM:1168 (073B): "a slot whose link word has its low bit set steps one level down" does not say what happens. The walk follows the link word of every entry (`shl cx,1; mov bp,cx`, :1177-1179); a set low bit only skips the match test (:1184). Say that. [fixed in dacdf20: the 073B header now says the walk follows the link word of every entry (`shl cx,1; mov bp,cx`) and that a set low bit only skips that entry's match test.]
- [x] note src/E1E8B.ASM:227-231 (0185): `push cs` before each of the two indirect calls (:242-245) is not explained: the callee sees CS on the stack. Say what the pushed CS is for, or that its use is not established. [fixed in dacdf20: the 0185 header now says each call pushes CS first, so the callee sees CS on the stack, and that what it uses it for is not established.]

### Confirmed, no change

- 0442: DS:0020 is the record's own version table. The frame paragraph from the 20C1 table is the resident stub segment of the record (WHITEPAPER 2.3 and the fixup note at line 100), and the +2 compare matches the stub form that 068D writes.
- 086F and 08C9: the load condition (:1390-1393) and the bootstrap's argument pushes (:1451-1456) match their headers.
- 03B7 and 07A1 have no caller in src/ and no packet entry; "uncalled" is right.

## Summary

Round 3: the unit reads as the overlay manager it is, and the round 2 fixes hold. Four must-fix items are about meaning: the record's ES:000A is the fixup table's byte size, not data (0789), and 03FD's "count" is the same byte size; the trigger for the whole-chain discard in 0536 is the carry from 0761, which is also set on a borrow; and 03FD's flag is bit 0 of the fixup word, not the index's low bit. The rest are clarity fixes (a history reference in 02FF, the free-area sentence in 000F, a running count in 0580, the CX inputs in 066F and 068D). No placeholder names are left that need evidence. No matcher, spec or pin question, so nothing is escalated. Verdict: review-failed, 4 must-fix items, all comment corrections (no code change is asked for).

# Round 2

Reviewer: rev-haiku
Commit reviewed: b922a421366e324e4af07368ca9c20afdda3b82a (main). src/E1E8B.ASM is the round 1 fixed file (acac24d); review/e1e8b was at the same commit when I started.

Playbook 70. Judge what a matching file can still get wrong: names, types, comments, structure.
The matcher already proved the bytes; do not re-prove them, read for meaning.

Checked: `make match FILE=src/E1E8B.ASM` 30 of 30 functions, exit 0; `make readability UNIT=e1e8b` 0 findings. Round 1's fixes are in the file. The findings below were not raised in round 1: they come from reading the call paths (0580 calls ES:0018, 086F, 0536 and 0613 walks, the 06C0 patch loop) against each comment.
Evidence used: src/E1E8B.ASM (all of it), re/symbols.toml (:7210, :7300), re/renames.toml (:1985-1998), docs/design/WHITEPAPER.md (2.3, the FBOV header and the stub layout).

## Findings

Each finding is a checkbox. `must-fix` blocks `make review-pass` until a decompiler ticks it off
(with the commit that fixes it); `should` and `note` do not block.

### Registry and names

- [x] must-fix re/symbols.toml:7210 and :7300, re/renames.toml:1986-1998: the names of 1E8B:05FB and 1E8B:0761 are recorded without the leading underscore (`ReleaseOverlayRecord`, `OverlayParagraphsRemaining`), but src/E1E8B.ASM:932 and :1180 use `_ReleaseOverlayRecord` and `_OverlayParagraphsRemaining`, and every other function of the unit is recorded with `_` (for example `_InitOverlayManager`, symbols :7048). The registry and the file disagree, so `make rename APPLY=1` would rename the file to a different name. Record the underscore form the file uses (`make rename` again, or edit the two entries). `make readability` does not flag it; check the convention by hand. [fixed: re/symbols.toml now records `_ReleaseOverlayRecord` and `_OverlayParagraphsRemaining`, and `_DropStaleOverlaySlots` for the renamed 07D7, matching the file. re/renames.toml keeps the base names on purpose: rename.py adds the leading underscore for .ASM, so `make rename APPLY=1` would write a double underscore if the log held the underscore form. The registry now agrees with the file. `make match FILE=src/E1E8B.ASM` 30 of 30, exit 0.]
- [x] should src/E1E8B.ASM:1254, :1262-1263 and :931-932: `_ReleaseOverlayRecords` (07D7, uncalled in this unit) and `_ReleaseOverlayRecord` (05FB) differ by one letter and do different jobs: 05FB clears one record's version table and base (:933-939); 07D7 drops slot references that point below 20B5:00B4 (the SS:BP walk, :1280-1297, :1322-1323). Give 07D7 a name that says what it drops. The comment should also say that CX on entry is the table end (`cmp bx,cx`, :1280), since the caller supplies it. [fixed: `make rename OLD=1E8B:07D7 NEW=DropStaleOverlaySlots` (WHY cites the SS:BP walk and the below-00B4 test, logged in re/renames.toml); the header now says "drop stale overlay slots" and that CX on entry is the table end.]

### 01A7 to 01F3: the open path

- [x] must-fix src/E1E8B.ASM:266-267: "copies the program path after it to 20B5:001C up to the last backslash (the leaf name)" is wrong. own_copy (:296-304) copies the whole path into 20B5:001C; the last backslash only sets BX (:303), the offset after the leaf. The open at :400 uses the full path in 001C when the caller's argument is null. A non-null argument is copied at BX by 0250 (:307, :385-399), so the caller's name replaces the leaf; nothing is kept as "the leaf name". Say that. [fixed: the 01B3 header now says the whole path is copied and the backslash only sets BX to the offset past the last one, and that a non-null argument makes 0250 copy the caller's name over the leaf so the full path is opened.]
- [x] must-fix src/E1E8B.ASM:264-265 and :314: "DGROUP:0075 gives the PSP segment" (and "the PSP segment" in 01F3) names the wrong thing. The code loads DS from DGROUP:0075 (:281, :324) and reads strings from offset 0 of it: the double-NUL scan (:284-290), the "PATH=" compare (:327-333) and the count word after the block (:291). That is the environment block, not the PSP (the PSP holds the command tail and the INT 20h bytes, not these strings). Name the word for the environment segment. [H] Borland's startup keeps an `_envseg`-style word; check the startup code before choosing a name. [fixed: checked the startup: `_envseg` is the environment segment in build/toolchain/tc100/STARTUP/C0.ASM:675, and build/re/rtl/tc100/l/r.map:1549 has `__envseg` at DGROUP:0075 (with `__psp` at DGROUP:0079). The 01B3 and 01F3 headers now name DGROUP:0075 the environment segment and cite those. The 0286 comment that read DGROUP:0079 was also updated: it is `_psp`.]

### 0134 and 0185: swap and shutdown

- [x] must-fix src/E1E8B.ASM:183-185: "then closes the file handle at 20B5:00B8 and reopens the file" describes two paths. The code (:208-219) either closes the handle at 20B5:00B8 and clears it, with no reopen (:210-214), or, when none is open, opens the file named at 20B5:001C and stores the new handle (:215-219). Say so. 0185 relies on the close path (:236-238); 000F (:146-147) relies on the open path. [fixed: the 0134 header now describes the two paths: close-and-clear with no reopen when a handle is open, open the named file and store the handle when none is.]
- [x] must-fix src/E1E8B.ASM:227-228: "the two cleanup callbacks kept at 20B5:0014 and 20B5:0012" gives a role the unit does not show. The only reads are the two calls (:241, :243). Nothing in this unit stores 20B5:0012 or 20B5:0014 (the word-0012 store at :464 is to 20BF:0012, and :515 reads a record's word). Write "Purpose unknown: two near pointers the shutdown calls; writer not in this unit" unless a writer is found in src/ or in the overlay stubs. [fixed: the 0185 header now says "Purpose unknown: what those two pointers do; no function in this unit writes 20B5:0012 or 20B5:0014".]

### 02FF MapOverlayRecords

- [x] must-fix src/E1E8B.ASM:496-497: "walks the overlay's own file records (the ES:0012 chain)" and "linking them through 20B5:001C" name the wrong segment and the wrong address. The walk reads DS:0012 of the current record (`mov cx,ds:[12h]`, :514-515; ES is the next record, :517). The link is each record's own word +1Ch (:530, and the terminating 0 at :535), and the chain head is 20B5:00BC (:508). 20B5:001C is the name buffer (see 01A7). [fixed: the 02FF header now says the walk reads the next record from the current record's own word 0012 (DS:0012), keeps the head in 20B5:00BC, and links each record through its own word +1Ch; it notes 20B5:001C is the file-name buffer of 01A7.]

### 0286 InitOverlayRecords and 4A2h

- [x] must-fix src/E1E8B.ASM:687 and :736: the tail at 1E8B:04A2 is called "an uncalled sibling". It is reached indirectly: 0286 stores 4A2h in each record's ES:0018 (:473) and 0580 calls ES:0018 when the record is not loaded (`call word ptr es:[18h]`, :887), testing the carry it returns (`jb load_halt`, :888). So 1E8B:04A2 is the loader of those records (it reads ES:0004:0006 and ES:0008+000A bytes, :737-751). Drop "uncalled" here and in the 0442 header (:687-688). [fixed: the 0442 header and the 04A2 comment now call 04A2 the record loader, reached through each record's ES:0018 (0286 stores its offset, 0580 calls it); "uncalled" is gone.]
- [x] should src/E1E8B.ASM:436, :925, :930: 4A2h is called a "marker" (436) and a "sentinel" (925, 930). It is the offset of the loader routine 1E8B:04A2, which the record's ES:0018 holds. Say "the loader offset 04A2 (1E8B:04A2)", and in 05FB say what the test of that offset decides. [fixed: the 0286 and 05FB headers now say "the loader offset 04A2 (1E8B:04A2)" and 05FB says the test decides whether the near call at 20B5:0010 is made.]
- [x] note src/E1E8B.ASM:434-435, :464: the store `mov es:[12h],ax` (:464) goes to 20BF:0012, the list head, while 02FF walks each record's own word 0012 (:515). The comment is right that this unit does not read 20BF:0012, but a reader will ask what the head word is for. Say "Purpose unknown" or name the reader if one is found. [fixed: the 0286 header now says no function in this unit reads 20BF:0012, so its purpose is unknown.]

### 03FD and 0442: relocation

- [x] should src/E1E8B.ASM:636-637: "fixes the word up through 0442" does not say what is written. The target word is replaced by the frame paragraph from the 20C1 table (`mov es:[bx],dx`, :667), and when the odd flag is set 0442 then rewrites the far pointer's offset. Say the word becomes the frame paragraph. [fixed: the 03FD header now says the word at ES:BX is replaced by the frame paragraph from the 20C1 table, and 0442 rewrites the offset word at ES:BX+4 when the index's low bit is set.]
- [x] should src/E1E8B.ASM:682 and :685-686: "the target word at ES:BX" and "one whose +2 offset equals the target word" use the same name for two different words. The segment word is at ES:BX (the `mov ax,<seg>` operand; the B8 and 50 bytes around it are checked at :695-718). The word searched and replaced is the offset at ES:BX+4 (:721, :729). Name both. [fixed: the 0442 header now names the segment word at ES:BX and the offset word at ES:BX+4, the one searched and replaced.]

### 04D0 OverlayCallHandler

- [x] should src/E1E8B.ASM:755-756: "An odd BP means the call came from outside the overlay manager" is not shown. `push bp; mov bp,sp` makes BP equal SP, so the test (:766) is the parity of the stack pointer. Nothing in this unit says what an odd SP means. Say that the test is on SP parity, and mark the reason [H] or Purpose unknown. [fixed: the 04D0 header now says the test is the parity of the stack pointer and marks what an odd SP means as not shown in this unit [H].]
- [x] should src/E1E8B.ASM:759 and :798-800: "with the ES:001A bit 8 value in AX" leaves out that the bit is cleared in the same step (`and byte ptr es:[1Ah],0F7h`, :800). 086F clears it too (:1373). Say so. [fixed: the 04D0 header now says bit 3 (value 8) of ES:001A is read into AX and cleared in the same step.]

### 05FB ReleaseOverlayRecord

- [x] should src/E1E8B.ASM:925-926: "calls the release hook at 20B5:0010" reads as a far call through that address. The call at :936 is `call word ptr ds:[10h]`: a near call in this unit's code segment to the offset stored at 20B5:0010. Say so. "Release hook" is a guess; the evidence is the clear of ES:0010 at :938. Keep the name, and say which part is evidence. [fixed: the 05FB header now says the call at 20B5:0010 is a near call (`call word ptr ds:[10h]`) and that "release hook" is a guess whose only evidence is the call and the clear of ES:0010.]

### 064E and 068D: the version table

- [x] must-fix src/E1E8B.ASM:981-982: "it first strips the record's entry from the patch table (072F)" has the effect backwards. With CX = ES:0002 (:993), 072F exchanges that saved offset into the matching slot (`xchg ss:[bx+2],cx`, :1138), so 064E restores the saved offset into the slot. Say restores. [fixed: the 064E header now says 072F restores the saved offset into the matching slot.]
- [x] should src/E1E8B.ASM:981: "the version chain ES:0002" is one saved offset (written by 068D at :1039, read by 064E at :993), not a chain. Name it the saved slot offset. [fixed: the 064E header now calls ES:0002 the saved slot offset.]
- [x] must-fix src/E1E8B.ASM:1026: "clears ES:0002" is wrong. 072F returns the slot's old offset in CX, and `mov es:[2],cx` (:1039) stores it in ES:0002. ES:0002 is zero only when no slot matched. Write "saves the slot's previous offset in ES:0002 (zero when no slot matched)". [fixed: the 068D header now says it saves the slot's previous offset in ES:0002 (zero when no slot matched).]
- [x] must-fix src/E1E8B.ASM:1027 and :1045: "the word DGROUP:00A0" is 20B5:00A0. The code reads `ds:[0A0h]` with DS = 20B5: the manager keeps DS on 20B5 (000F loads it from cs:[4], 0580 and 0613 restore it, 086F reads the same word as 20B5:00A0 at :1355). Write 20B5:00A0. The word's value 3FCDh is the bytes CD 3F of the stub form (WHITEPAPER 2.3), so say that, or write Purpose unknown for the word itself. The round 1 wording "DGROUP:00A0" came from this comment and was not corrected. [fixed: the 068D header now says "the word at 20B5:00A0, whose bytes are 0CDh, 3Fh (the Turbo Pascal overlay stub)".]

### 06C0 MoveOverlayRecord

- [x] must-fix src/E1E8B.ASM:1058: "updates ES:000E" is wrong. The write at :1088 goes through DS = base-1 (`dec ax; mov ds,ax; mov ds:[0Eh],es`, :1085-1088), so it stores the record segment at word 0Eh of the header paragraph just below the new base. 07A1 reads that same back link (:1237). Say so. [fixed: the 06C0 header now says it writes the record's segment ES at word 0Eh of the header paragraph just below the new base (DS = new base - 1) and that 07A1 reads that back link.]
- [x] must-fix src/E1E8B.ASM:1058-1059: "strips the record from the patch table (073B)" and "clears the CX version records at ES:0023" are both wrong. 073B is called with AX = the new base and DX = the old base (:1063-1064, :1093), so it retargets the slots whose segment is the old base to the new one. The loop (:1095-1100) stores AX, the new base (not zero), in the segment word of each version record (ES:0023, then +5 each), so the far jumps of 066F's layout point at the new base. The evidence line (:1059) also says "DGROUP:00B0"; the word is 20B5:00B0 (`mov ax,ds:[0B0h]`, :1063). [fixed: the 06C0 header now says 073B is called with AX = the new base and DX = the old base and retargets the slots, and that the loop stores the new base in each version record's segment word; the evidence line now says 20B5:00B0.]

### 073B FindOverlaySlot

- [x] must-fix src/E1E8B.ASM:1143-1144: "walk the overlay table at SS:BP backward" and "BX = the first matching slot whose far offset at SS:[BP+2] is zero (the free one)" are wrong. The walk follows the link word at each slot (`mov bp,cx` at :1155 from `[bp]`, :1157; again at :1167), and it never tests SS:[BP+2]: BX is the first slot whose segment equals DX (:1161-1166). Every matching slot gets AX (:1163), not only the first. 072F's header (:1126-1132) says this correctly; this header does not. [fixed: the 073B header now says the walk follows the link word at each slot, stores AX into every matching slot's SS:[BP+4], sets BX once from the first match, and never tests SS:[BP+2].]

### 07A1 (in the 0789 range)

- [x] must-fix src/E1E8B.ASM:1212-1213: "returns its base segment in AX" is wrong. At :1237 ES becomes the record (from the back link), and at :1242 the code compares AX (the base) with the record's ES:0010; on a match `mov ax,es` (:1243) returns the record segment. Say: "returns the record segment (ES) of the loaded entry whose base is AX, or AX = 0 with carry set; the id test is at :1239-1241". [fixed: the 0789 header now says 07A1 finds the record through the header back link at word 0Eh, tests the id 20B5:00A0 and AX against ES:0010, and returns the record segment in AX on a match, else AX = 0 with carry set.]

### 0761 OverlayParagraphsRemaining

- [x] should src/E1E8B.ASM:1174-1175: "how many paragraphs ... remain before the top of the allocated area at 20B5:00B0" does not describe `ES:0010 minus 00B0` (:1185-1186). That is the record's base minus the running top. The subtraction borrows (CF set, the fallback) whenever the base is below 00B0, which is the case for a record already linked (0711 adds its size to 00B0, :1112). Say what the value measures: the paragraphs from 00B0 up to the current base, or the free paragraphs on the fallback (00B6 minus 00B0, :1189-1190). [fixed: the 0761 header now says AX = the current record's base (ES:0010) minus the running top 00B0 (the paragraphs from 00B0 up to that base), and on borrow the free paragraphs 00B6 minus 00B0 with carry set.]

### 086F OverlayManagerEntry

- [x] must-fix src/E1E8B.ASM:1336-1337: the load condition is inverted. The code loads when ES:0010 is zero (:1358-1359) or when ES:0020 is CDh (:1360-1361: `jne entry_call` skips the load when the marker is not CDh). With ES:0010 set and ES:0020 not CDh it returns ES:0010 without loading or calling. Say that. The call through 20B5:0016 (:1376) is on the load path only. [fixed: the 086F header now says it loads when ES:0010 is zero or ES:0020 is CDh, that a loaded record whose marker is not CDh returns ES:0010 without loading or calling, and that the 20B5:0016 call is on the load path only.]

### Notes

- [x] note src/E1E8B.ASM:581: "converts DX:AX bytes to a segment value" in the 03B7 tail. It returns a paragraph count (the 02FF header, :501, says so). Make the two agree. [fixed: the 03B7 comment now says "DX:AX bytes to a paragraph count", agreeing with the 02FF header.]

## Summary

Round 2: the unit reads as the overlay manager it is, and its names mostly follow from the code. The problems are in the comments on the paths that round 1 did not walk: the record load condition in 086F is inverted; 064E, 068D and 06C0 say the reverse of what their patch and copy loops do ("strips", "clears", and a wrong segment word); 073B's header describes a backward walk and a free-slot test that are not in the code; 07A1 returns the record segment, not the base; 01B3 and 0134 describe more or less than their code does; the 4A2h loader is called uncalled. The registry and the file disagree on the underscore of two names (05FB, 0761). Placeholder names left: 07D7 needs a name that says what it drops; 20B5:0012/0014 have no writer in the unit (Purpose unknown until one is found). Nothing is blocked; there is no matcher, spec or pin question, so nothing is escalated. Verdict: review-failed, 15 must-fix items, all comment, register or naming corrections (no code change is asked for).

# Round 1 (kept)

Reviewer: rev-haiku
Commit reviewed: ae2deb94e5d9bc7e3485367fb47f84bb56b375d7

Playbook 70. Judge what a matching file can still get wrong: names, types, comments, structure.
The matcher already proved the bytes; do not re-prove them, read for meaning.

## Findings

Each finding is a checkbox. `must-fix` blocks `make review-pass` until a decompiler ticks it off
(with the commit that fixes it); `should` and `note` do not block.

Unit: `src/E1E8B.ASM`, the VROOMM overlay manager (31 functions, all L2). Line numbers are
`src/E1E8B.ASM` unless another file is named.

### Unit header and globals

- [x] must-fix src/E1E8B.ASM:5-6: "installs a DOS interrupt handler (04D0) for overlay calls" claims an installation this unit does not perform. 04D0 has no direct caller and no vector store here (packet: "no direct call found"); 0134 only swaps whatever is stored at 20B5:0002/0004. Cite who stores 04D0 in the vector, or write what is known ("entered through an interrupt vector, installer not in this unit"). [fixed in acac24d: the header now says 04D0 is entered through a DOS interrupt vector whose installer is not in this unit (no direct caller, no vector store; 0134 only swaps 20B5:0002/0004)]
- [x] must-fix src/E1E8B.ASM:6: "the MemAlloc call at 086F (0000:72DE, ...)" names the runtime's farmalloc (EXTRN at line 17, `_farmalloc`). MemAlloc is the game's own allocator at 19BC:0007 (src/E1B82.C:66, src/EOV0.C:130). Use farmalloc, as line 17 and the 086F code do. [fixed in acac24d: now reads the farmalloc call at 086F]
- [x] must-fix src/E1E8B.ASM:8: "Many functions are still stubs; names and globals below are placeholders until proven" is stale. All 31 functions are matched (L2) and none is a stub. Remove the sentence or say which names are still placeholders (none is, by the packet). [fixed in acac24d: the stale stubs sentence is removed]
- [x] must-fix src/E1E8B.ASM:14: `_gOverlayIndex` at 20C1:0000 is called "the overlay index table". The code (03FD, lines 635-639) reads the frame paragraph at index times 8 from it for every relocation, which is TLINK's segment table: WHITEPAPER line 76 places that table at load module offset 20C10h (= 20C1:0000) with 99 entries of 8 bytes (frame paragraph, end offset, kind, flags). Rename it to the segment table's name with `make rename` and fix the comment in 03FD. [fixed in acac24d: renamed to _gTlinkSegmentTable (make rename, WHY cites WHITEPAPER 2.2) and the 03FD comment now names TLINK's segment table]
- [x] should src/E1E8B.ASM:15: `_gOverlayList` (20BF:0000, "overlay record list head") is supported by the chain walk in 0711 (20BF:001C). No change needed; listed so the next reader knows the chain fields (1Ch, 12h) are in the record. [fixed in acac24d: no change needed, as the review says]
- [x] note src/E1E8B.ASM:20-25: the QUIRK 0004 paragraph says the bytes are identical, but in this file the data words sit at file offsets 0 and 2 and the "PATH=" literal at 4, so `mov ds,cs:[4]` in this file reads "PA". The code is correct only in the original frame (the unit starts at 1E8B:0004, map). Say so plainly in that paragraph. [fixed in acac24d: the QUIRK now says the data sit at offsets 0 and 2 and PATH= at 4, so cs:[4] reads PA and cs:[6] reads TH here; the code is correct only in the original frame]

### 000F InitOverlayManager

- [x] must-fix src/E1E8B.ASM:36-44 (and the exit at 156): "Returns AX = 0 on success, 0FFFEh for a missing file and 0FFFFh for a bad file" is false on one path. The MapOverlayRecords failure exit (lines 153-157: `jae init_ok`, `mov bx,ds:[0B8h]`, `mov cx,0FFFFh`, `jmp init_fail`) loads 0FFFFh into CX, not AX, so AX keeps whatever the failed call left (a DOS result or a byte count), and the handle in BX is never closed. Document that path and mark it `QUIRK 1E8B:...:` (or fix the code's intent with the matcher's help, not by hand). [fixed in acac24d: the comment now documents the 1E8B:011F path (AX from _MapOverlayRecords, handle still open) and a QUIRK 1E8B:000F for the dead CX codes]
- [x] should src/E1E8B.ASM:84, 90, 112, 117, 147: the codes 0FFFDh, 0FFFCh and 0FFFBh are stored in CX and never returned: every error path reaches init_closefail, which sets AX to 0FFFFh. They are dead stores. Mark `QUIRK 1E8B:...` and say that AX is always 0FFFFh on those paths. [fixed in acac24d: marked QUIRK 1E8B:000F: the codes are dead stores and AX is always 0FFFFh on those paths]
- [x] should src/E1E8B.ASM:56 (first compare of ds:[0A0h]): the early success exit when the overlay id word is zero (`jmp init_ok`, AX = 0) is not described. Say what it means (already initialised, or no overlay id yet) or what is not known. [fixed in acac24d: the comment now describes the 20B5:00A0 == 0 early return and says which of the two meanings is not established]
- [x] should src/E1E8B.ASM:125-129: "stores the overlay-file offset, its base and size at DGROUP:84B2/84B4 and 20B5:00A4/00A6". The code stores the FBOV size (the u32 at FBOV header +4, WHITEPAPER 2.3) at DGROUP:84B2/84B4, and the offset of the FBOV data at 20B5:00A4/00A6. No base is stored. Name what is stored. [fixed in acac24d: the comment now says the FBOV data offset at 20B5:00A4/00A6 and the FBOV size at DGROUP:84B2/84B4; no base is stored]
- [x] note src/E1E8B.ASM:36-44: the fallback name pointer 20C1:0318 (stored into [bp+6]/[bp+8] when the caller gives none, lines 64-66) is not in the comment. Say what it points to, or that it is not known. [fixed in acac24d: the comment now says 20C1:0318 is the eob.exe string just past the 99-entry segment table]

### 0134 SwapAndReopenOverlayFile

- [x] should src/E1E8B.ASM:170-172: "installs the previously saved one from 20B5:0004:0002" uses a notation that reads as one address. The code loads the offset from ds:[2] and the segment from ds:[4]. Write "the vector whose offset is at 20B5:0002 and segment at 20B5:0004". [fixed in acac24d: now says the vector whose offset is at 20B5:0002 and segment at 20B5:0004]

### 0185 ShutdownOverlayManager

- [x] must-fix src/E1E8B.ASM:217: "the second vector 0012 is also used by the exit path of 086F". 086F (lines 1289-1345) never reads ds:[12h]. Remove the sentence or name the real user. (ds:[12h] is the record link read in 02FF, line 493, and written in 0286, line 444.) [fixed in acac24d: the false sentence is removed; the comment now calls 20B5:0012/0014 cleanup callbacks in the manager's own data segment]

### 01A7 OpenNamedOverlayFile

- [x] must-fix src/E1E8B.ASM:238-239: "keeping only the leaf name after the last backslash". 01A7 does not scan for a backslash; it calls 0250, which copies the whole name (up to 12 bytes). Only 01B3 keeps the leaf name. [fixed in acac24d: the 01A7 comment now says it copies the whole name; only 01B3 trims the leaf]

### 01B3 OpenOwnOverlayFile

- [x] must-fix src/E1E8B.ASM:251-253: "takes the following word (the program name in the master PSP tail)". The `lodsw` at line 275 reads the 16-bit count that follows the environment block (DOS 3 layout) and discards it; the program path starts after that word and is copied by own_copy. Fix the comment. [fixed in acac24d: the 01B3 comment now says the word after the environment is a 16-bit count that is discarded, and the path after it is copied]
- [x] should src/E1E8B.ASM:251-253: the call to 0250 at the end copies from the caller's argument [bp+6] when it is not null, so the own-file name is only used when the caller passes null (the boot path does, 086F pushes 0,0). Say so; it is the only reason 01B3 works for the boot call. [fixed in acac24d: the 01B3 comment now says 0250 still copies the caller's non-null [bp+6], so the own name is used only when the caller passes null (086F pushes 0,0)]

### 01F3 OpenOverlayFileViaPath

- [x] note: the comment matches the code (PATH= compare at cs:0008, ';' split, backslash appended unless the entry ends in ':' or '\'). No change needed. [fixed in acac24d: no change needed, as the review says]

### 0250 CopyNameAndOpenOverlay

- [x] note: the comment matches the code (12-byte copy, NUL terminated, null argument skips the copy, carry on failure). No change needed. [fixed in acac24d: no change needed, as the review says]

### 0276 ReadOverlayHeaderBytes

- [x] must-fix src/E1E8B.ASM:394: "(handle in 20B5:00B8)". The routine reads the handle from BX (the `mov ah,3Fh; int 21h` at 0276 uses BX as set by the caller, 000F's `mov bx,ax` before the reads). It does not read 20B5:00B8. Say "handle in BX (the handle 000F keeps at 20B5:00B8)". [fixed in acac24d: now says the handle is in BX, set by the caller; 000F keeps it at 20B5:00B8]

### 0286 InitOverlayRecords

- [x] must-fix src/E1E8B.ASM:413-414: "walks the record table at 20BF:00C0". The table is at DS:00C0 (`lea si,ds:[0C0h]`), and DS is 20B5 (000F loads it from cs:[4]; the 086F bootstrap sets it). 20BF is loaded into ES and is not the table's segment. Write 20B5:00C0 (or DS:00C0). [fixed in acac24d: now the 20B5:00C0 table (DS:00C0); ES is the 20BF list head]
- [x] must-fix src/E1E8B.ASM:416: "points the record at its own segment (ES:0012)". `mov es:[12h],ax` (line 444) runs while ES is still the list head (20BF, loaded at the start of the function), before ES becomes the record. It writes the head's word 0012, not the record's. Fix the comment, and name the head word if it is known what reads it. [fixed in acac24d: now says the store goes to the list head's word 20BF:0012 while ES is still the head, which this unit does not read]
- [x] must-fix src/E1E8B.ASM:413: "DGROUP:0079 + 10h (the start of free memory after the program)". No evidence is given, and nothing else in the repo names DGROUP:0079 (only this unit reads it, line 425). Give the evidence (a runtime symbol, a whitepaper section) or write "Purpose unknown" for the value. [fixed in acac24d: the guess is gone; the comment says what DGROUP:0079 holds is not established and nothing reads cs:000D]
- [x] must-fix src/E1E8B.ASM:418: "the 4A2h marker (also cleared at 05FB)". 05FB clears ES:0010 (line 900) and only reads the marker at ES:0018 (line 896). Nothing in this unit clears 4A2h. [fixed in acac24d: the false clear reference is removed; the 4A2h store is in 0286]

### 02FF MapOverlayRecords

- [x] must-fix src/E1E8B.ASM:477: "computes the 4 KB-rounded size". The code computes (paragraphs minus base) times 16 as SI:DI bytes (`rol si,4`, `and si,0FFF0h`, the chain_load block). There is no 4 KB rounding. Say "the byte count of the image as SI:DI". [fixed in acac24d: now the image's byte count as SI:DI (paragraphs times 16, no rounding)]
- [x] should src/E1E8B.ASM:475-481: "starts from the free segment 20B5:00B2": this unit never writes 00B2 (it is read at line 485 only). Say that it is set outside this unit, or where. [fixed in acac24d: the comment now says 00B2 is set outside this unit]
- [x] note src/E1E8B.ASM:481: the tail at 03B7 converts DX:AX bytes to paragraphs (shifts by 4), not to "a segment value" as such. Fine as written; "paragraphs" would be exact. [fixed in acac24d: the tail is now called a paragraph count]

### 03C4 ReadOverlayImage

- [x] must-fix src/E1E8B.ASM:568-570: "four KB at a time" and "advancing DS by 0FFFh to step 4 KB". Each read is 0FFF0h bytes (line 587) and DS advances by 0FFFh paragraphs (line 584), which is 0FFF0h bytes (64 KB minus 16), not 4 KB. Fix both phrases. [fixed in acac24d: now 0FFF0h bytes (64 KB minus 16) per read and the DS step is 0FFFh paragraphs]
- [x] should src/E1E8B.ASM:569: "seeks to DX:CX". INT 21h/4200h takes CX:DX (CX is the high word); the callers load DX with the low word and CX with the high word. Write CX:DX. [fixed in acac24d: now CX:DX (DX low, CX high)]

### 03FD ApplyOverlayRelocations

- [x] note src/E1E8B.ASM:617-622: the reloc list is read from the image base plus ES:0008 (paragraph part in DS, offset in SI); the comment says "at the record's ES:0008/ES:000A". Accurate enough; say which word is the byte offset and which is the count (CX from ES:0A). [fixed in acac24d: the comment now says ES:0008 is the byte offset of the reloc list and ES:000A is the count (CX from 02FF)]

### 0442 RelocateOverlayPointer

- [x] must-fix src/E1E8B.ASM:652-654: "Given DS:DX (the overlay base)" and "replaces the ES:[BX+4] entry with its 20B5:0020 version slot". DX is the frame paragraph read from the 20C1 table in 03FD (line 639, `mov dx,[di]`), and `mov ds,dx` (line 661) makes it DS. The search (lines 686-690) reads DS:[di+2] from DS:0020, which is that segment's version table (the table 066F and 068D write at ES:0020 of an overlay record). 20B5 is not involved. [fixed in acac24d: the comment now says DX is the 20C1 frame paragraph, mov ds,dx, and the version table searched is DS:0020; 20B5 is not involved]
- [x] should src/E1E8B.ASM:652-654: "the b8 50 pattern" needs a sentence on what it is (a `mov ax,imm` followed by `push ax`, the fixup form the comment assumes). Say which bytes are checked and what the four masked bytes mean. [fixed in acac24d: the comment now spells out the bytes checked (ES:BX-1 and +3 are 0B8h, ES:BX+2 and +6 are 50h) and the 0F807h mask]
- [x] note src/E1E8B.ASM:700: `rel_out2` is a second `pop cx; ret` (the not-found exit). Mark it as such. [fixed in acac24d: rel_out2 now carries a comment marking it the 1E8B:049E not-found exit]

### 04D0 OverlayCallHandler

- [x] note: the comment matches the code (odd BP to __OverlayHalt, the 20B5 segment, the request word, the 0016 call, IRET). No change needed beyond the header claim at line 5-6 above. [fixed in acac24d: no change needed, as the review says; the header 04D0 claim is fixed above]

### 0536 AgeOverlayRecords

- [x] must-fix src/E1E8B.ASM:783-786: the comment describes LoadOverlayRecord's logic (the ES:0010 test, the ES:001B/ES:001A flags, the ES:0018 call, the ES:000E store, the __OverlayHalt jump). None of that is in AgeOverlayRecords. Its code walks the 20B5:00BC chain, calls 0613 when the test fails, calls 05FB or 077D when ES:001B is zero, decrements ES:001B otherwise, moves (06C0) and links (0711) the record, and stores the new top at the caller's ES:0010. Rewrite the comment for the code that is there. [fixed in acac24d: the 0536 comment is rewritten for its own code (00BA, 0789, 0761, 0613, 05FB/077D, 06C0/0711, the [00B0] store)]

### 0580 LoadOverlayRecord

- [x] must-fix src/E1E8B.ASM:828-832: "clears or restores each record's version table (068D and 077D)". The loop calls 068D and 077D only when ES:001B is zero; the other branch adds zero (`xor ax,ax`). Nothing restores a version table in the loop (064E does that, before the loop). Fix the sentence. [fixed in acac24d: the comment now says 064E restores once before the loop and the loop clears (068D) only when ES:001B is zero]
- [x] should src/E1E8B.ASM:828-832: "(0761 says how many bytes are left)". 0761 returns paragraphs (see 0761 below). Say paragraphs. [fixed in acac24d: now paragraphs]

### 05FB ReleaseOverlayFpuState

- [x] must-fix src/E1E8B.ASM:889-892 and the name: the function is called ReleaseOverlayFpuState and the comment says "the numeric coprocessor state" and "the emulator (or a real FPU) installed its own replacement". WHITEPAPER line 49 says no game code uses floating point and there are no emulator interrupts (INT 34h to 3Dh) in any code segment. The 4A2h value has no FPU evidence anywhere in the repo, and the code only tests it and calls the entry at 20B5:0010 (line 897-898). Rename the function (or write "Purpose unknown:" with what is known: the marker test, the call through ds:[10h], the clear of ES:0010). [fixed in acac24d: renamed to _ReleaseOverlayRecord (make rename) with the FPU claim gone; the label fpu_done is now rel_done]
- [x] must-fix src/E1E8B.ASM:892: "matching the save at 02FF:02C9". 02FF has no store of 4A2h. The store is in InitOverlayRecords (line 453, `mov word ptr es:[18h],4A2h`). Fix the cross-reference. [fixed in acac24d: the cross-reference now points at 0286]

### 0613 DiscardOverlayRecords

- [x] must-fix src/E1E8B.ASM:904-907: "unlinking each from the 20B5:00BC head via 06C0". 06C0 moves the record's image (it does not unlink). The relink onto the head is inline (lines 926-927: `mov es:[1Ch],ax`, `mov ds:[0BCh],es`). Say so. [fixed in acac24d: the comment now says the relink onto the head is inline and 06C0 moves the image]
- [x] must-fix src/E1E8B.ASM:907: "resets the running size at 20B5:00B0 to the file size 20B5:00B4". 00B4 is the base of the free area (000F stores [bp+0Ch]+1 there, lines 138-139), not the file size. The code first sets 00B0 to 00B6 (the top of the area, lines 920-921) and sets it to 00B4 at the end (lines 933-934). Name them as free-area top and base. [fixed in acac24d: now 00B0 is set to 00B6 (free area top) first and to 00B4 (free area base) at the end]

### 064E RestoreOverlayVersionTable

- [x] must-fix src/E1E8B.ASM:938-941: "when the record's marker at ES:0020 is 0EAh it falls into _FillOverlayVersionTable at 066F". `je short ver_ret` (line 950) jumps to the RET at the end of 066F, so for 0EAh the fill is skipped. Otherwise: when ES:0002 is zero the code jumps to the fill (line 952), and when it is not zero it calls 072F and then falls into the fill (the proc has no RET after line 953). So the fill runs for every marker except 0EAh. The comment has it the other way round. [fixed in acac24d: the control flow is corrected; the fill runs for every marker except 0EAh]

### 066F FillOverlayVersionTable

- [x] must-fix src/E1E8B.ASM:957-960: "(0EAh, the offset at ES:0010 and the base segment ES)". The code writes 0EAh, then the offset it read from the old entry at +2 (line 968, `mov dx,es:[di+2]`), then the segment ES:[10h] (line 953, `mov ax,es:[10h]` into BX). So the offset is the old entry's, and the segment is the record's base (ES:0010). The comment swaps offset and segment and names ES as the segment. [fixed in acac24d: now 0EAh, the old entry's offset from +2, and the record's base segment ES:0010]
- [x] should src/E1E8B.ASM:957-960: name the two 5-byte entry forms: 0EAh + offset + segment (a live far jump) and the 0CDh form (CD 3F, offset, 0, the Turbo Pascal-style stub entry of WHITEPAPER 2.3). 066F turns the second into the first; 068D turns the first into the second. Without the names the reader cannot see why the offset is read from +2. [fixed in acac24d: the comment names the 0EAh far-jump form and the 0CDh 3Fh stub form (WHITEPAPER 2.3); 068D was updated too]

### 068D ClearOverlayVersionTable

- [x] should src/E1E8B.ASM:980-983: "rewrites the CX version records at ES:0020 with the old entry at ES:0020, the current DGROUP:00A0 and a zero marker". The word written first is DGROUP:00A0 (the 0CDh-form "CD 3F" bytes, see 066F), then the old entry's offset from +1, then zero. Say so, and name the form. [fixed in acac24d: now DGROUP:00A0's CD 3F bytes, then the old offset from +1, then zero, and names the stub form]

### 06C0 MoveOverlayRecord

- [x] note: the comment matches the code (copy of ES:0008 words from the old base ES:0010 to 20B5:00B0, direction by the compare, the 073B call and the clear of the CX version records at ES:0023). No change needed. The record field names (0008, 0010, 0023) are placeholders with evidence from the copy, fine as they are. [fixed in acac24d: no change needed, as the review says]

### 0711 LinkOverlayRecord

- [x] note: the comment matches the code (077D paragraphs added to 00B0, tail link through 20BF:001C). No change needed. [fixed in acac24d: no change needed, as the review says]

### 072F PatchOverlayEntryVector

- [x] must-fix src/E1E8B.ASM:1079-1081: "save the old entry pointer at SS:[BP+4] into the far function pointer 20B5:0010". 072F does not write 20B5:0010 at all: it calls 073B and exchanges SS:[BX+2] with CX when BX is nonzero. The reader of 20B5:0010 is 05FB. "the slot whose ES:0010 equals DX": the compare is inside 073B against the slot's SS:[BP+4]. "BP = CX * 2" is also 073B's loop, not 072F's. Rewrite the comment for 072F's own code and move the rest to 073B. [fixed in acac24d: the 072F comment is rewritten for its own code (073B call, the SS:[BX+2]/CX exchange); the 20B5:0010 claim is gone]
- [x] must-fix src/E1E8B.ASM:1079-1081 (callers): 064E passes AX = ES:[10h] and DX = ES; 068D passes AX = ES and DX = ES:[10h]. The arguments are swapped between the two callers and the comment does not say which is which. Write what each argument is. [fixed in acac24d: the comment now gives both callers' AX/DX]

### 073B FindOverlaySlot

- [x] note: the comment matches the code (backward walk, skips odd entries, the DX compare against SS:[BP+4], the AX store, BX = first free slot). No change needed. [fixed in acac24d: no change needed, as the review says]

### 0761 OverlayBytesRemaining

- [x] must-fix src/E1E8B.ASM:1124-1126: "return in AX how many bytes ... remain". ES:[10h] and 20B5:00B0 are segment (paragraph) values, so AX is a count of paragraphs (line 1134 `mov ax,es:[10h]`, `sub ax,ds:[0B0h]`). Say paragraphs. [fixed in acac24d: now paragraphs; the function is also renamed to _OverlayParagraphsRemaining]
- [x] must-fix src/E1E8B.ASM:1126: "the fallback computed from the file size at 20B5:00B6". 00B6 is the top of the free area (000F stores the caller's [bp+0Ah] there, and the 086F bootstrap passes the end of the block), not the file size. Fix the name of the quantity. [fixed in acac24d: now 00B6 is the free area's top]

### 077D OverlayCodeParagraphs

- [x] must-fix src/E1E8B.ASM:1146: "(size at ES:0008 plus the 10h byte header plus 0Fh rounding)". The code adds 11h (line 1151), not 10h plus 0Fh (which is 1Fh). Fix the constant in the comment and say what the 11h is (10h header and one more byte, if known). [fixed in acac24d: now +11h, the 10h header plus one byte whose reason is not established]

### 0789 OverlayTotalParagraphs

- [x] must-fix src/E1E8B.ASM:1157: "return in DX:AX the total size in paragraphs". The code returns DX = total (code plus data, paragraphs) and AX = code paragraphs only (lines 1165-1171). Callers use DX (0286 compares `bx,dx`). Fix the comment. [fixed in acac24d: now AX = code paragraphs, DX = total; callers use DX]
- [x] must-fix src/E1E8B.ASM:1160: the 07A1 routine (inside this PROC) "returns its base segment in ES". Line 1189 is `mov ax,es`, and ES is popped before the RETF, so the base segment comes back in AX. Fix it. [fixed in acac24d: now returns its base segment in AX (ES is popped before the RETF)]

### 07D7 ReleaseOverlayRecords

- [x] should src/E1E8B.ASM:1200-1204: "This is the manager's flush path." Nothing in this unit calls it (packet: "no direct call found"), so the role is not shown. Write "uncalled in this unit" and what the code does. [fixed in acac24d: now says uncalled in this unit and what the code does]
- [x] note src/E1E8B.ASM:1200-1204: the comment says "not the current entry" for the id match; the code tests the table slot at SS:[BX+2] for zero. Make the condition exact. [fixed in acac24d: the condition is now exact (SS:[bx+2] zero and non-zero, the ES:[0] id and ES:[2] tests)]

### 086F OverlayManagerEntry

- [x] must-fix src/E1E8B.ASM:1284-1285: "takes the larger of 20B5:00AA and DGROUP:3088, allocates that many paragraphs plus one". The code doubles 00AA when it is at least 3088 (`shl bx,1`, line 1343) and otherwise uses 3088 undoubled, then adds one. Describe the doubling, or write the size formula as the code has it. [fixed in acac24d: the comment now gives the size formula with the 00AA doubling]
- [x] should src/E1E8B.ASM:1284-1285: "([bp-style far args 0,0, bx, dx])" is not a readable description. Say that the call pushes DX, BX, then two zero words (the name pointer, null), so the callee's frame is [bp+6]/[bp+8] = 0, [bp+0Ah] = BX, [bp+0Ch] = DX. [fixed in acac24d: now says the call pushes DX, BX, 0, 0 and the callee's frame is [bp+6]/[bp+8] = 0, [bp+0Ah] = BX, [bp+0Ch] = DX]
- [x] should src/E1E8B.ASM:1279-1289: the same PROC has two entries: the record entry (086F) and the bootstrap at 08C9 that allocates the overlay memory and starts the manager. The comment covers both in one paragraph. Split the bootstrap into its own function or say in the comment that 08C9 is a second entry of this PROC. [fixed in acac24d: the comment now says 08C9 is a second entry of this same PROC]
- [x] note: the function's own comment says "(no direct call found)" in the packet. The record entry is reached from overlay stubs, not from this unit; say so. [fixed in acac24d: the 086F comment now says the record entry is reached from the overlay stubs, not from this unit]

## Summary

The unit reads as the VROOMM overlay manager it is: open (000F, 01A7 to 01F3, 0250), map (02FF), load (0580), relocate (03FD, 0442), age (0536, 0613), and the version table (064E to 073B). Each function's code is clear and its name mostly follows from the code. The problems are in the comments, and several are factual errors that a later port would copy: the record table is in 20B5 not 20BF; 0536 describes 0580; the sizes are paragraphs and bytes, not "4 KB"; 066F and 064E describe the wrong offset, segment and control flow; 0761 and 0789 name the wrong units and registers; the bootstrap size in 086F drops a doubling. Two names contradict the evidence: `_ReleaseOverlayFpuState` (the whitepaper says the game uses no floating point) and `_gOverlayIndex` (the evidence says TLINK's segment table). `_farmalloc` is called MemAlloc in the header. No placeholder names are left in the unit. Verdict: review-failed, 32 must-fix items, all comment or naming corrections (no code change is asked for; a QUIRK mark is asked for the dead CX codes and the failing MapOverlayRecords exit in 000F).

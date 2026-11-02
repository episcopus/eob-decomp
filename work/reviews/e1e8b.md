# Review: e1e8b

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

- [ ] must-fix src/E1E8B.ASM:5-6: "installs a DOS interrupt handler (04D0) for overlay calls" claims an installation this unit does not perform. 04D0 has no direct caller and no vector store here (packet: "no direct call found"); 0134 only swaps whatever is stored at 20B5:0002/0004. Cite who stores 04D0 in the vector, or write what is known ("entered through an interrupt vector, installer not in this unit").
- [ ] must-fix src/E1E8B.ASM:6: "the MemAlloc call at 086F (0000:72DE, ...)" names the runtime's farmalloc (EXTRN at line 17, `_farmalloc`). MemAlloc is the game's own allocator at 19BC:0007 (src/E1B82.C:66, src/EOV0.C:130). Use farmalloc, as line 17 and the 086F code do.
- [ ] must-fix src/E1E8B.ASM:8: "Many functions are still stubs; names and globals below are placeholders until proven" is stale. All 31 functions are matched (L2) and none is a stub. Remove the sentence or say which names are still placeholders (none is, by the packet).
- [ ] must-fix src/E1E8B.ASM:14: `_gOverlayIndex` at 20C1:0000 is called "the overlay index table". The code (03FD, lines 635-639) reads the frame paragraph at index times 8 from it for every relocation, which is TLINK's segment table: WHITEPAPER line 76 places that table at load module offset 20C10h (= 20C1:0000) with 99 entries of 8 bytes (frame paragraph, end offset, kind, flags). Rename it to the segment table's name with `make rename` and fix the comment in 03FD.
- [ ] should src/E1E8B.ASM:15: `_gOverlayList` (20BF:0000, "overlay record list head") is supported by the chain walk in 0711 (20BF:001C). No change needed; listed so the next reader knows the chain fields (1Ch, 12h) are in the record.
- [ ] note src/E1E8B.ASM:20-25: the QUIRK 0004 paragraph says the bytes are identical, but in this file the data words sit at file offsets 0 and 2 and the "PATH=" literal at 4, so `mov ds,cs:[4]` in this file reads "PA". The code is correct only in the original frame (the unit starts at 1E8B:0004, map). Say so plainly in that paragraph.

### 000F InitOverlayManager

- [ ] must-fix src/E1E8B.ASM:36-44 (and the exit at 156): "Returns AX = 0 on success, 0FFFEh for a missing file and 0FFFFh for a bad file" is false on one path. The MapOverlayRecords failure exit (lines 153-157: `jae init_ok`, `mov bx,ds:[0B8h]`, `mov cx,0FFFFh`, `jmp init_fail`) loads 0FFFFh into CX, not AX, so AX keeps whatever the failed call left (a DOS result or a byte count), and the handle in BX is never closed. Document that path and mark it `QUIRK 1E8B:...:` (or fix the code's intent with the matcher's help, not by hand).
- [ ] should src/E1E8B.ASM:84, 90, 112, 117, 147: the codes 0FFFDh, 0FFFCh and 0FFFBh are stored in CX and never returned: every error path reaches init_closefail, which sets AX to 0FFFFh. They are dead stores. Mark `QUIRK 1E8B:...` and say that AX is always 0FFFFh on those paths.
- [ ] should src/E1E8B.ASM:56 (first compare of ds:[0A0h]): the early success exit when the overlay id word is zero (`jmp init_ok`, AX = 0) is not described. Say what it means (already initialised, or no overlay id yet) or what is not known.
- [ ] should src/E1E8B.ASM:125-129: "stores the overlay-file offset, its base and size at DGROUP:84B2/84B4 and 20B5:00A4/00A6". The code stores the FBOV size (the u32 at FBOV header +4, WHITEPAPER 2.3) at DGROUP:84B2/84B4, and the offset of the FBOV data at 20B5:00A4/00A6. No base is stored. Name what is stored.
- [ ] note src/E1E8B.ASM:36-44: the fallback name pointer 20C1:0318 (stored into [bp+6]/[bp+8] when the caller gives none, lines 64-66) is not in the comment. Say what it points to, or that it is not known.

### 0134 SwapAndReopenOverlayFile

- [ ] should src/E1E8B.ASM:170-172: "installs the previously saved one from 20B5:0004:0002" uses a notation that reads as one address. The code loads the offset from ds:[2] and the segment from ds:[4]. Write "the vector whose offset is at 20B5:0002 and segment at 20B5:0004".

### 0185 ShutdownOverlayManager

- [ ] must-fix src/E1E8B.ASM:217: "the second vector 0012 is also used by the exit path of 086F". 086F (lines 1289-1345) never reads ds:[12h]. Remove the sentence or name the real user. (ds:[12h] is the record link read in 02FF, line 493, and written in 0286, line 444.)

### 01A7 OpenNamedOverlayFile

- [ ] must-fix src/E1E8B.ASM:238-239: "keeping only the leaf name after the last backslash". 01A7 does not scan for a backslash; it calls 0250, which copies the whole name (up to 12 bytes). Only 01B3 keeps the leaf name.

### 01B3 OpenOwnOverlayFile

- [ ] must-fix src/E1E8B.ASM:251-253: "takes the following word (the program name in the master PSP tail)". The `lodsw` at line 275 reads the 16-bit count that follows the environment block (DOS 3 layout) and discards it; the program path starts after that word and is copied by own_copy. Fix the comment.
- [ ] should src/E1E8B.ASM:251-253: the call to 0250 at the end copies from the caller's argument [bp+6] when it is not null, so the own-file name is only used when the caller passes null (the boot path does, 086F pushes 0,0). Say so; it is the only reason 01B3 works for the boot call.

### 01F3 OpenOverlayFileViaPath

- [ ] note: the comment matches the code (PATH= compare at cs:0008, ';' split, backslash appended unless the entry ends in ':' or '\'). No change needed.

### 0250 CopyNameAndOpenOverlay

- [ ] note: the comment matches the code (12-byte copy, NUL terminated, null argument skips the copy, carry on failure). No change needed.

### 0276 ReadOverlayHeaderBytes

- [ ] must-fix src/E1E8B.ASM:394: "(handle in 20B5:00B8)". The routine reads the handle from BX (the `mov ah,3Fh; int 21h` at 0276 uses BX as set by the caller, 000F's `mov bx,ax` before the reads). It does not read 20B5:00B8. Say "handle in BX (the handle 000F keeps at 20B5:00B8)".

### 0286 InitOverlayRecords

- [ ] must-fix src/E1E8B.ASM:413-414: "walks the record table at 20BF:00C0". The table is at DS:00C0 (`lea si,ds:[0C0h]`), and DS is 20B5 (000F loads it from cs:[4]; the 086F bootstrap sets it). 20BF is loaded into ES and is not the table's segment. Write 20B5:00C0 (or DS:00C0).
- [ ] must-fix src/E1E8B.ASM:416: "points the record at its own segment (ES:0012)". `mov es:[12h],ax` (line 444) runs while ES is still the list head (20BF, loaded at the start of the function), before ES becomes the record. It writes the head's word 0012, not the record's. Fix the comment, and name the head word if it is known what reads it.
- [ ] must-fix src/E1E8B.ASM:413: "DGROUP:0079 + 10h (the start of free memory after the program)". No evidence is given, and nothing else in the repo names DGROUP:0079 (only this unit reads it, line 425). Give the evidence (a runtime symbol, a whitepaper section) or write "Purpose unknown" for the value.
- [ ] must-fix src/E1E8B.ASM:418: "the 4A2h marker (also cleared at 05FB)". 05FB clears ES:0010 (line 900) and only reads the marker at ES:0018 (line 896). Nothing in this unit clears 4A2h.

### 02FF MapOverlayRecords

- [ ] must-fix src/E1E8B.ASM:477: "computes the 4 KB-rounded size". The code computes (paragraphs minus base) times 16 as SI:DI bytes (`rol si,4`, `and si,0FFF0h`, the chain_load block). There is no 4 KB rounding. Say "the byte count of the image as SI:DI".
- [ ] should src/E1E8B.ASM:475-481: "starts from the free segment 20B5:00B2": this unit never writes 00B2 (it is read at line 485 only). Say that it is set outside this unit, or where.
- [ ] note src/E1E8B.ASM:481: the tail at 03B7 converts DX:AX bytes to paragraphs (shifts by 4), not to "a segment value" as such. Fine as written; "paragraphs" would be exact.

### 03C4 ReadOverlayImage

- [ ] must-fix src/E1E8B.ASM:568-570: "four KB at a time" and "advancing DS by 0FFFh to step 4 KB". Each read is 0FFF0h bytes (line 587) and DS advances by 0FFFh paragraphs (line 584), which is 0FFF0h bytes (64 KB minus 16), not 4 KB. Fix both phrases.
- [ ] should src/E1E8B.ASM:569: "seeks to DX:CX". INT 21h/4200h takes CX:DX (CX is the high word); the callers load DX with the low word and CX with the high word. Write CX:DX.

### 03FD ApplyOverlayRelocations

- [ ] note src/E1E8B.ASM:617-622: the reloc list is read from the image base plus ES:0008 (paragraph part in DS, offset in SI); the comment says "at the record's ES:0008/ES:000A". Accurate enough; say which word is the byte offset and which is the count (CX from ES:0A).

### 0442 RelocateOverlayPointer

- [ ] must-fix src/E1E8B.ASM:652-654: "Given DS:DX (the overlay base)" and "replaces the ES:[BX+4] entry with its 20B5:0020 version slot". DX is the frame paragraph read from the 20C1 table in 03FD (line 639, `mov dx,[di]`), and `mov ds,dx` (line 661) makes it DS. The search (lines 686-690) reads DS:[di+2] from DS:0020, which is that segment's version table (the table 066F and 068D write at ES:0020 of an overlay record). 20B5 is not involved.
- [ ] should src/E1E8B.ASM:652-654: "the b8 50 pattern" needs a sentence on what it is (a `mov ax,imm` followed by `push ax`, the fixup form the comment assumes). Say which bytes are checked and what the four masked bytes mean.
- [ ] note src/E1E8B.ASM:700: `rel_out2` is a second `pop cx; ret` (the not-found exit). Mark it as such.

### 04D0 OverlayCallHandler

- [ ] note: the comment matches the code (odd BP to __OverlayHalt, the 20B5 segment, the request word, the 0016 call, IRET). No change needed beyond the header claim at line 5-6 above.

### 0536 AgeOverlayRecords

- [ ] must-fix src/E1E8B.ASM:783-786: the comment describes LoadOverlayRecord's logic (the ES:0010 test, the ES:001B/ES:001A flags, the ES:0018 call, the ES:000E store, the __OverlayHalt jump). None of that is in AgeOverlayRecords. Its code walks the 20B5:00BC chain, calls 0613 when the test fails, calls 05FB or 077D when ES:001B is zero, decrements ES:001B otherwise, moves (06C0) and links (0711) the record, and stores the new top at the caller's ES:0010. Rewrite the comment for the code that is there.

### 0580 LoadOverlayRecord

- [ ] must-fix src/E1E8B.ASM:828-832: "clears or restores each record's version table (068D and 077D)". The loop calls 068D and 077D only when ES:001B is zero; the other branch adds zero (`xor ax,ax`). Nothing restores a version table in the loop (064E does that, before the loop). Fix the sentence.
- [ ] should src/E1E8B.ASM:828-832: "(0761 says how many bytes are left)". 0761 returns paragraphs (see 0761 below). Say paragraphs.

### 05FB ReleaseOverlayFpuState

- [ ] must-fix src/E1E8B.ASM:889-892 and the name: the function is called ReleaseOverlayFpuState and the comment says "the numeric coprocessor state" and "the emulator (or a real FPU) installed its own replacement". WHITEPAPER line 49 says no game code uses floating point and there are no emulator interrupts (INT 34h to 3Dh) in any code segment. The 4A2h value has no FPU evidence anywhere in the repo, and the code only tests it and calls the entry at 20B5:0010 (line 897-898). Rename the function (or write "Purpose unknown:" with what is known: the marker test, the call through ds:[10h], the clear of ES:0010).
- [ ] must-fix src/E1E8B.ASM:892: "matching the save at 02FF:02C9". 02FF has no store of 4A2h. The store is in InitOverlayRecords (line 453, `mov word ptr es:[18h],4A2h`). Fix the cross-reference.

### 0613 DiscardOverlayRecords

- [ ] must-fix src/E1E8B.ASM:904-907: "unlinking each from the 20B5:00BC head via 06C0". 06C0 moves the record's image (it does not unlink). The relink onto the head is inline (lines 926-927: `mov es:[1Ch],ax`, `mov ds:[0BCh],es`). Say so.
- [ ] must-fix src/E1E8B.ASM:907: "resets the running size at 20B5:00B0 to the file size 20B5:00B4". 00B4 is the base of the free area (000F stores [bp+0Ch]+1 there, lines 138-139), not the file size. The code first sets 00B0 to 00B6 (the top of the area, lines 920-921) and sets it to 00B4 at the end (lines 933-934). Name them as free-area top and base.

### 064E RestoreOverlayVersionTable

- [ ] must-fix src/E1E8B.ASM:938-941: "when the record's marker at ES:0020 is 0EAh it falls into _FillOverlayVersionTable at 066F". `je short ver_ret` (line 950) jumps to the RET at the end of 066F, so for 0EAh the fill is skipped. Otherwise: when ES:0002 is zero the code jumps to the fill (line 952), and when it is not zero it calls 072F and then falls into the fill (the proc has no RET after line 953). So the fill runs for every marker except 0EAh. The comment has it the other way round.

### 066F FillOverlayVersionTable

- [ ] must-fix src/E1E8B.ASM:957-960: "(0EAh, the offset at ES:0010 and the base segment ES)". The code writes 0EAh, then the offset it read from the old entry at +2 (line 968, `mov dx,es:[di+2]`), then the segment ES:[10h] (line 953, `mov ax,es:[10h]` into BX). So the offset is the old entry's, and the segment is the record's base (ES:0010). The comment swaps offset and segment and names ES as the segment.
- [ ] should src/E1E8B.ASM:957-960: name the two 5-byte entry forms: 0EAh + offset + segment (a live far jump) and the 0CDh form (CD 3F, offset, 0, the Turbo Pascal-style stub entry of WHITEPAPER 2.3). 066F turns the second into the first; 068D turns the first into the second. Without the names the reader cannot see why the offset is read from +2.

### 068D ClearOverlayVersionTable

- [ ] should src/E1E8B.ASM:980-983: "rewrites the CX version records at ES:0020 with the old entry at ES:0020, the current DGROUP:00A0 and a zero marker". The word written first is DGROUP:00A0 (the 0CDh-form "CD 3F" bytes, see 066F), then the old entry's offset from +1, then zero. Say so, and name the form.

### 06C0 MoveOverlayRecord

- [ ] note: the comment matches the code (copy of ES:0008 words from the old base ES:0010 to 20B5:00B0, direction by the compare, the 073B call and the clear of the CX version records at ES:0023). No change needed. The record field names (0008, 0010, 0023) are placeholders with evidence from the copy, fine as they are.

### 0711 LinkOverlayRecord

- [ ] note: the comment matches the code (077D paragraphs added to 00B0, tail link through 20BF:001C). No change needed.

### 072F PatchOverlayEntryVector

- [ ] must-fix src/E1E8B.ASM:1079-1081: "save the old entry pointer at SS:[BP+4] into the far function pointer 20B5:0010". 072F does not write 20B5:0010 at all: it calls 073B and exchanges SS:[BX+2] with CX when BX is nonzero. The reader of 20B5:0010 is 05FB. "the slot whose ES:0010 equals DX": the compare is inside 073B against the slot's SS:[BP+4]. "BP = CX * 2" is also 073B's loop, not 072F's. Rewrite the comment for 072F's own code and move the rest to 073B.
- [ ] must-fix src/E1E8B.ASM:1079-1081 (callers): 064E passes AX = ES:[10h] and DX = ES; 068D passes AX = ES and DX = ES:[10h]. The arguments are swapped between the two callers and the comment does not say which is which. Write what each argument is.

### 073B FindOverlaySlot

- [ ] note: the comment matches the code (backward walk, skips odd entries, the DX compare against SS:[BP+4], the AX store, BX = first free slot). No change needed.

### 0761 OverlayBytesRemaining

- [ ] must-fix src/E1E8B.ASM:1124-1126: "return in AX how many bytes ... remain". ES:[10h] and 20B5:00B0 are segment (paragraph) values, so AX is a count of paragraphs (line 1134 `mov ax,es:[10h]`, `sub ax,ds:[0B0h]`). Say paragraphs.
- [ ] must-fix src/E1E8B.ASM:1126: "the fallback computed from the file size at 20B5:00B6". 00B6 is the top of the free area (000F stores the caller's [bp+0Ah] there, and the 086F bootstrap passes the end of the block), not the file size. Fix the name of the quantity.

### 077D OverlayCodeParagraphs

- [ ] must-fix src/E1E8B.ASM:1146: "(size at ES:0008 plus the 10h byte header plus 0Fh rounding)". The code adds 11h (line 1151), not 10h plus 0Fh (which is 1Fh). Fix the constant in the comment and say what the 11h is (10h header and one more byte, if known).

### 0789 OverlayTotalParagraphs

- [ ] must-fix src/E1E8B.ASM:1157: "return in DX:AX the total size in paragraphs". The code returns DX = total (code plus data, paragraphs) and AX = code paragraphs only (lines 1165-1171). Callers use DX (0286 compares `bx,dx`). Fix the comment.
- [ ] must-fix src/E1E8B.ASM:1160: the 07A1 routine (inside this PROC) "returns its base segment in ES". Line 1189 is `mov ax,es`, and ES is popped before the RETF, so the base segment comes back in AX. Fix it.

### 07D7 ReleaseOverlayRecords

- [ ] should src/E1E8B.ASM:1200-1204: "This is the manager's flush path." Nothing in this unit calls it (packet: "no direct call found"), so the role is not shown. Write "uncalled in this unit" and what the code does.
- [ ] note src/E1E8B.ASM:1200-1204: the comment says "not the current entry" for the id match; the code tests the table slot at SS:[BX+2] for zero. Make the condition exact.

### 086F OverlayManagerEntry

- [ ] must-fix src/E1E8B.ASM:1284-1285: "takes the larger of 20B5:00AA and DGROUP:3088, allocates that many paragraphs plus one". The code doubles 00AA when it is at least 3088 (`shl bx,1`, line 1343) and otherwise uses 3088 undoubled, then adds one. Describe the doubling, or write the size formula as the code has it.
- [ ] should src/E1E8B.ASM:1284-1285: "([bp-style far args 0,0, bx, dx])" is not a readable description. Say that the call pushes DX, BX, then two zero words (the name pointer, null), so the callee's frame is [bp+6]/[bp+8] = 0, [bp+0Ah] = BX, [bp+0Ch] = DX.
- [ ] should src/E1E8B.ASM:1279-1289: the same PROC has two entries: the record entry (086F) and the bootstrap at 08C9 that allocates the overlay memory and starts the manager. The comment covers both in one paragraph. Split the bootstrap into its own function or say in the comment that 08C9 is a second entry of this PROC.
- [ ] note: the function's own comment says "(no direct call found)" in the packet. The record entry is reached from overlay stubs, not from this unit; say so.

## Summary

The unit reads as the VROOMM overlay manager it is: open (000F, 01A7 to 01F3, 0250), map (02FF), load (0580), relocate (03FD, 0442), age (0536, 0613), and the version table (064E to 073B). Each function's code is clear and its name mostly follows from the code. The problems are in the comments, and several are factual errors that a later port would copy: the record table is in 20B5 not 20BF; 0536 describes 0580; the sizes are paragraphs and bytes, not "4 KB"; 066F and 064E describe the wrong offset, segment and control flow; 0761 and 0789 name the wrong units and registers; the bootstrap size in 086F drops a doubling. Two names contradict the evidence: `_ReleaseOverlayFpuState` (the whitepaper says the game uses no floating point) and `_gOverlayIndex` (the evidence says TLINK's segment table). `_farmalloc` is called MemAlloc in the header. No placeholder names are left in the unit. Verdict: review-failed, 32 must-fix items, all comment or naming corrections (no code change is asked for; a QUIRK mark is asked for the dead CX codes and the failing MapOverlayRecords exit in 000F).

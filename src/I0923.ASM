; Unit at INTRO.EXE 0923:0004 (707 bytes): the DOS memory control block code that EOB.EXE links as
; e1990 (1990:0001, 710 bytes), instruction for instruction. This copy is three bytes shorter
; because its memory top word, gMemTopSeg, is a DGROUP word (ds:28C4, reached as cs:8084h) instead of
; a code segment word. CheckMCB merges free blocks in the MCB chain; CheckMemoryChain walks the
; chain, relocates the environment block and frees the one-paragraph block it allocated;
; ReserveMemoryBelowVideo and TrimMemoryToVideo size the program's memory below a probed segment
; (the "video top" reading is a hypothesis [H], see gVideoTopSeg). Five state words are code segment
; data at 0923:0004 to 000F. INTRO.EXE's own code calls in: the function at 0892:0241 calls
; CheckMemoryChain (0892:026C) and ReserveMemoryBelowVideo (0892:02A6, argument 3, a 64 KB reserve).
; CheckMCB is called only from within this unit (0923:0110, 0923:0271), and no call or far pointer to
; TrimMemoryToVideo exists in INTRO.EXE, so it is dead code here.

        .MODEL  LARGE
        .CODE

gVideoTopSeg  dw 0                      ; 0923:0004: the base of the memory reserved below the
                                        ;   probed segment (the caller at 0892:02A6 takes
                                        ;   ReserveMemoryBelowVideo's result into DI at 0892:02AD
                                        ;   and uses it as memtop (src/I0892.C:303, :332).
                                        ;   "Video top" is a hypothesis [H]: ReserveMemoryBelowVideo
                                        ;   probes the segment whose B800:0000 marker aliases (or A000,
                                        ;   0923:0139 to 0150) and subtracts (n-1) x 32 KB
                                        ;   (0923:01A8 to 01B6), but nothing in src/ or docs/ says
                                        ;   that segment is where video memory ends. It is then
                                        ;   lowered to memtop plus the moved footprint when memtop is
                                        ;   at or below that value less the footprint (0923:01FE to
                                        ;   0207: the add is skipped when memtop is above), so it
                                        ;   never ends above the value the subtraction left it at
                                        ;   (0923:01B6).
gMovedSeg     dw 0                      ; 0923:0006: data segment DOS gave the relocated copy of the
                                        ;   environment block (0923:00E0). ReserveMemoryBelowVideo
                                        ;   copies that copy to its final place and frees it
                                        ;   (0923:0268 to 026D, ES = gMovedSeg, INT 21h 49h).
gMovedParas   dw 0                      ; 0923:0008: the environment block's size plus its own MCB
                                        ;   paragraph (its footprint): CheckMemoryChain loads
                                        ;   es:[3], adds 1 and stores it (0923:00D1); the
                                        ;   allocation then requests the size without the +1
                                        ;   (0923:00D6). ReserveMemoryBelowVideo reads it at
                                        ;   0923:01F9, 0923:0202 and 0923:0212; its copy count is
                                        ;   the copy's own MCB size plus one (0923:023F to 0243),
                                        ;   which equals the footprint.
        db      0,0                     ; 0923:000A: unused (no reader in the unit)
gNewMemTopSeg dw 0                      ; 0923:000C: the program's new top of memory, stored into
                                        ;   PSP:0002 (0923:0264) when a block was moved. The moved
                                        ;   copy's MCB lies exactly here (es at 0923:0236; bx set at
                                        ;   0923:0221 to 0225), so the copy starts at this top and
                                        ;   its gMovedParas paragraphs reach up to gVideoTopSeg.
gMcbSeg       dw 0                      ; 0923:000E: MCB of the one-paragraph block that
                                        ;   CheckMemoryChain allocates (0923:0079 to 007E) and
                                        ;   frees (0923:0120 to 0129); set at 0923:0088

; 0923:8084: the memory top word. e1990 keeps it in the code segment at cs:0002 (it is the `dw` after
; its leading `db 0`); here it is the DGROUP word ds:28C4, and the code reaches it as cs:8084h because
; DGROUP's frame (0E9F) is 057Ch paragraphs above this segment's (0923): 057C0h + 28C4h = 8084h. The
; word has no relocation in the EXE, so the assembler resolved it to a constant. ReserveMemoryBelowVideo
; stores the end of DOS memory here (0923:01F0) and TrimMemoryToVideo reads it back (0923:02B3); i0892
; reads it at 0892:01D8, itoa's it into gMemTopArg (0892:01CF) and passes gMemTopArg as the last execl
; argument (src/I0892.C:271). That EOB.EXE then sets its own gMemTopSeg from atol of the number behind
; gArgv (src/E07C8.C:311) is a hypothesis [H]: no file in src/ reads argv[6] (src/E07C8.C reads argv[1]
; to argv[5], :229 to :235) and nothing in src/ or docs/ writes ds:8894 (src/E1990.ASM:23-26), so the
; link from this number to EOB's word is not shown, and the writer of ds:8894 is unknown.
gMemTopSeg    equ 8084h

; 0923:0010: walk the MCB chain from gMcbSeg, merging free blocks; AX = 0 if it ends with a
; proper 'Z' block, 1 if the chain is broken
        PUBLIC  CheckMCB                ; @ 0923:0010
CheckMCB PROC NEAR
        push    bx
        push    ds
        push    es
        mov     es,cs:gMcbSeg
        cmp     byte ptr es:[0],5Ah
        je      ok
again:  mov     bx,es:[1]
        mov     ax,es
        mov     ds,ax
        add     ax,es:[3]
        inc     ax
        mov     es,ax
        cmp     byte ptr es:[0],4Dh
        je      owner
        cmp     byte ptr es:[0],5Ah
        jne     bad
owner:  or      bx,es:[1]
        jne     next
        mov     ax,es:[3]
        inc     ax
        add     ds:[3],ax
        mov     al,es:[0]
        mov     ds:[0],al
        mov     ax,ds
        mov     es,ax
next:   cmp     byte ptr es:[0],5Ah
        jne     again
ok:     mov     ax,0
        jmp     done
bad:    mov     ax,1
done:   pop     es
        pop     ds
        pop     bx
        ret
CheckMCB ENDP

; 0923:0071: allocate a one-paragraph block to reach the MCB chain and walk it. A free block is
; skipped (owner word zero, 0923:00B4 to 00BA); a block whose owner is not this program is skipped
; (owner must equal ds, which holds the PSP, 0923:00BC to 00C3); and a block whose MCB is not the
; segment at PSP:002C minus one is skipped (0923:00C5 to 00C9). When the current block is the
; environment block (its MCB is PSP:002C minus one) it is moved. The copy is
; requested with the same size (0923:00D6 to 0923:00D9) and DOS chooses where the copy goes (its
; default allocation strategy, which this unit leaves unchanged: there is no INT 21h AH=58h),
; PSP:002C is pointed at the copy (0923:00DD), the bytes are copied (0923:00EC to 0923:00FD)
; and the old block is marked free by zeroing its owner (0923:0101). Then the one-paragraph block is
; freed.
; On the normal path the last step calls CheckMCB (0923:0110), which merges the free blocks as a side
; effect, so AX is CheckMCB's result there. Three error exits set AX = 1 directly, without CheckMCB:
; the one-paragraph allocation fails (0923:0082, jumping past the free to 0923:0116: there is no
; block to free), the walk finds a block type that is neither 'M' nor 'Z' (0923:00B1), and the
; environment copy's allocation fails (0923:00DB, jb); the last two reach 0923:011C, which frees the
; one-paragraph block first. The caller at 0892:0241 tests the result at 0892:0271 and, on 1, prints
; a message (PrintString, 097D:0000; SetVideoMode(7) at 0D2B:0033 runs first) and exits instead of
; reserving memory; 0892:026C is the call itself.
        PUBLIC  CheckMemoryChain        ; @ 0923:0071
CheckMemoryChain PROC FAR
        push    bp
        mov     bp,sp
        push    bx
        push    di
        push    ds
        push    es
        push    si
        mov     bx,1
        mov     ah,48h
        int     21h
        jae     allocOk
        jmp     allocFail
allocOk:
        dec     ax
        mov     es,ax
        mov     cs:gMcbSeg,ax
        mov     ax,es:[1]
        mov     ds,ax
        mov     bx,ds:[2ch]
        dec     bx
walk:   mov     ax,es
        add     ax,es:[3]
        inc     ax
        mov     es,ax
        cmp     byte ptr es:[0],4Dh
        je      checkOwner
        cmp     byte ptr es:[0],5Ah
        je      checkOwner
        jmp     chainError
checkOwner:
        cmp     word ptr es:[1],0
        je      nextBlock
        mov     ax,ds
        cmp     ax,es:[1]
        jne     nextBlock
        mov     ax,es
        cmp     bx,ax
        jne     nextBlock
        mov     bx,es:[3]
        inc     bx
        mov     cs:gMovedParas,bx
        dec     bx
        mov     ah,48h
        int     21h
        jb      chainError
        mov     ds:[2ch],ax
        mov     cs:gMovedSeg,ax
        push    ds
        push    es
        mov     bx,es
        mov     es,ax
        mov     ds,bx
        mov     si,10h
        mov     di,0
        cld
        mov     bx,ds:[3]
copyLoop:
        mov     cx,8
        rep movsw
        dec     bx
        jne     copyLoop
        pop     es
        pop     ds
        mov     word ptr es:[1],0
nextBlock:
        cmp     byte ptr es:[0],5Ah
        jne     walk
        call    CheckMCB
        jmp     freeTiny
allocFail:
        mov     ax,1
        jmp     chainDone
chainError:
        mov     ax,1
freeTiny:
        push    ax
        mov     ax,cs:gMcbSeg
        inc     ax
        mov     es,ax
        mov     ah,49h
        int     21h
        pop     ax
chainDone:
        pop     si
        pop     es
        pop     ds
        pop     di
        pop     bx
        pop     bp
        ret
CheckMemoryChain ENDP

; 0923:0133: find the segment whose byte at offset 0 aliases B800:0000: write the marker there
; (0923:014A) and scan down in 16 KB steps. The first test is A000 minus 16 KB (0x400 paragraphs,
; 0923:0155 to 015D), the scan goes down while the segment is above 3C00h (0923:015A to 015D), and
; A000 is used when nothing matches (the probeDone path, 0923:0197 to 01A1). That segment is read
; as the top of video memory only as a hypothesis [H] (see gVideoTopSeg), and it is not what is
; stored: the stored value is that segment less (n-1) x 32 KB, then lowered. The argument n
; is the first word argument of the far call, at bp+6; it is a count of 32 KB steps, not the
; graphics mode: the code computes (n-1) shifted by 11 paragraphs (0923:01A8 to 0923:01B4), so it
; reserves (n-1) x 32 KB below the probed top. The only caller passes 3 (0892:02A2, the lcall at
; 0892:02A6), a 64 KB reserve. It reaches the chain by allocating a one-paragraph block (INT 21h
; AH=48h at 0923:01C0) but does not test the carry flag, unlike CheckMemoryChain (jae at
; 0923:0080): when the allocation fails AX holds DOS's error code (7 or 8), so the MCB type is read
; at segment 6 or 7; if that byte is not 'M' the one-word-out exit marked below is taken, and if it
; is 'M' the chain is walked from there. Then it shrinks the last block of the chain, the one whose
; type is 'Z' (0923:01DE to 01E4, store at 0923:021D; the owner is not checked), and frees the
; one-paragraph block it allocated (0923:0226 to 0229, INT 21h AH=49h). Only when a block was moved
; (0923:0231) it points PSP:002C at that moved block's data segment (its MCB plus one, 0923:0256 to
; 025B), moves the relocated copy to gNewMemTopSeg and frees the old copy: the copy's MCB sits
; exactly at gNewMemTopSeg (es set at 0923:0236 from bx, set at 0923:0221 to 0225), so the copy
; starts at the program's new top and its gMovedParas paragraphs reach up to gVideoTopSeg. Then it
; calls CheckMCB (0923:0271) to merge the freed block into the chain and stores that new top into
; PSP:0002 (0923:0264). The result is gVideoTopSeg after
; the step at 0923:0202 to 0207 (loaded at 0923:0274), not gMemTopSeg; the caller takes it into DI
; at 0892:02AD and uses it as memtop. The CheckMCB result is overwritten at 0923:0274.
        PUBLIC  ReserveMemoryBelowVideo ; @ 0923:0133
ReserveMemoryBelowVideo PROC FAR
        push    bp
        mov     bp,sp
        push    bx
        push    ds
        push    es
        mov     word ptr cs:gVideoTopSeg,9800h ; overwritten before use (0923:01B6)
        mov     ax,0B800h
        mov     es,ax
        mov     bl,byte ptr es:[0]
        mov     byte ptr es:[0],0C3h
        mov     ax,0A000h
        mov     es,ax
probeDown:
        mov     ax,es
        sub     ax,400h
        cmp     ax,3C00h
        jbe     probeDone
        mov     es,ax
        mov     bh,byte ptr es:[0]
        cmp     bh,0C3h
        jne     probeDown
        push    es
        mov     ax,0B800h
        mov     es,ax
        mov     byte ptr es:[0],3Ch
        pop     es
        mov     bh,byte ptr es:[0]
        push    es
        mov     ax,0B800h
        mov     es,ax
        mov     byte ptr es:[0],0C3h
        pop     es
        cmp     bh,3Ch
        jne     probeDown
        mov     byte ptr es:[0],bl
        jmp     probeEnd
probeDone:
        mov     ax,0B800h
        mov     es,ax
        mov     byte ptr es:[0],bl
        mov     ax,0A000h
        mov     es,ax
probeEnd:
        mov     bx,es
        mov     ax,word ptr [bp+6]
        dec     ax
        xchg    al,ah
        shl     ax,1
        shl     ax,1
        shl     ax,1
        sub     bx,ax
        mov     word ptr cs:gVideoTopSeg,bx
        mov     bx,1
        mov     ah,48h
        int     21h
        push    ax
        dec     ax
        mov     es,ax
        cmp     byte ptr es:[0],4Dh
        je      chainHead
; QUIRK 0923:01CE: the jump to retTop is taken when the byte at the block's MCB is not 'M' (test to
;   branch at 0923:01C6 to 01CC); it leaves the allocated data segment (its MCB plus one, pushed at
;   0923:01C2) on the stack, so the exit pops one word out (the far return takes the wrong words)
;   and the block is not freed. Two ways in: in a valid chain a
;   block that is not 'M' is the 'Z' block, so the path is taken when the one-paragraph block is the
;   last one, and the code does not test for a corrupt chain; and the allocation that finds the block
;   does not test its own result (0923:01C0, no carry check, unlike CheckMemoryChain's jae at
;   0923:0080), so on failure AX is DOS's error code (7 or 8), the MCB type is read at the segment
;   made from it (0923:01C3 to 01C6), and the path is taken when that byte is not 'M'.
        jmp     retTop
chainHead:
        push    ax
        push    bx
lastBlock:
        mov     ax,es:[3]
        inc     ax
        mov     bx,es
        add     ax,bx
        mov     es,ax
        cmp     byte ptr es:[0],5Ah
        jne     lastBlock
        pop     bx
        pop     ax
        mov     ax,es
        add     ax,es:[3]
        inc     ax
        mov     word ptr cs:gMemTopSeg,ax
        mov     bx,cs:gVideoTopSeg
        sub     bx,cs:gMovedParas
        cmp     ax,bx
        ja      noGrow
        add     ax,cs:gMovedParas
        mov     cs:gVideoTopSeg,ax
noGrow:
        mov     bx,es
        inc     bx
        mov     ax,cs:gVideoTopSeg
        sub     ax,cs:gMovedParas
        mov     cs:gNewMemTopSeg,ax
        sub     ax,bx
        mov     es:[3],ax
        mov     bx,es
        add     bx,ax
        inc     bx
        pop     es
        mov     ah,49h
        int     21h
        mov     ax,cs:gMovedSeg
        or      ax,ax
        je      retTop
        dec     ax
        mov     ds,ax
        mov     es,bx
        mov     si,0
        mov     di,0
        cld
        mov     bx,ds:[3]
        inc     bx
copyMoved:
        mov     cx,8
        rep movsw
        dec     bx
        jne     copyMoved
        mov     byte ptr es:[0],5Ah
        mov     ax,es:[1]
        mov     bx,es
        inc     bx
        mov     ds,ax
        mov     ds:[2ch],bx
        mov     bx,cs:gNewMemTopSeg
        mov     ds:[2],bx
        mov     es,cs:gMovedSeg
        mov     ah,49h
        int     21h
        call    CheckMCB
retTop:
        mov     ax,cs:gVideoTopSeg
        pop     es
        pop     ds
        pop     bx
        pop     bp
        ret
ReserveMemoryBelowVideo ENDP

; 0923:027D: shrink the last MCB of the chain down to gMemTopSeg, then free the one-paragraph
; block (0923:02BD to 02C0, INT 21h AH=49h). gMemTopSeg is written by ReserveMemoryBelowVideo
; (0923:01F0) and read at 0923:02B3 and 0892:01D8; nothing else in INTRO.EXE writes it, and no call
; or far pointer to this function exists, so it is dead code here. With gMemTopSeg at its loaded
; value, 0 (its DGROUP word 0E9F:28C4 is 00 00 in eob/INTRO.EXE, image offset 112B4h), the last
; block's size is set to 0 less its data segment (0923:02B0 to 02B9), a wrapped 16 bit value, and
; the walk (0923:0299 to 02AC) only looks for a 'Z' type, so it does not stop at a broken chain. The
; name's claim that this value is the video top is a hypothesis [H]: the bytes show only that the last
; block is set to end at gMemTopSeg (0923:02B0 to 02B9). ReserveMemoryBelowVideo stored the end of
; DOS memory there (0923:01F0). Like ReserveMemoryBelowVideo, it allocates the one-paragraph block
; (INT 21h AH=48h at 0923:0288) without testing the carry flag: on failure AX is DOS's error code
; (7 or 8), the MCB type is read at segment 6 or 7 (0923:028B to 028E), and the one-word-out exit
; marked below is taken when that byte is not 'M'.
        PUBLIC  TrimMemoryToVideo       ; @ 0923:027D
TrimMemoryToVideo PROC FAR
        push    bp
        mov     bp,sp
        push    ax
        push    bx
        push    es
        mov     bx,1
        mov     ah,48h
        int     21h
        push    ax
        dec     ax
        mov     es,ax
        cmp     byte ptr es:[0],4Dh
        je      trimLast
; QUIRK 0923:0296: the jump to trimDone is taken when the byte at the block's MCB is not 'M' (test to
;   branch at 0923:028E to 0296); it leaves the allocated data segment (its MCB plus one, pushed at
;   0923:028A) on the stack, so the exit pops one word out (the far return takes the wrong words)
;   and the block is not freed. Two ways in: in a valid chain a
;   block that is not 'M' is the 'Z' block, so the path is taken when the one-paragraph block is the
;   last one, and the code does not test for a corrupt chain; and the allocation that finds the block
;   does not test its own result (0923:0288, no carry check, unlike CheckMemoryChain's jae at
;   0923:0080), so on failure AX is DOS's error code (7 or 8), the MCB type is read at the segment
;   made from it (0923:028B to 028E), and the path is taken when that byte is not 'M'.
        jmp     trimDone
trimLast:
        push    ax
        push    bx
walkLast:
        mov     ax,es:[3]
        inc     ax
        mov     bx,es
        add     ax,bx
        mov     es,ax
        cmp     byte ptr es:[0],5Ah
        jne     walkLast
        pop     bx
        pop     ax
        mov     bx,es
        inc     bx
        mov     ax,cs:gMemTopSeg
        sub     ax,bx
        mov     es:[3],ax
        pop     es
        mov     ah,49h
        int     21h
trimDone:
        pop     es
        pop     bx
        pop     ax
        pop     bp
        ret
TrimMemoryToVideo ENDP

        END

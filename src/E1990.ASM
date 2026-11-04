; Unit at EOB.EXE 1990:0001 (710 bytes): DOS memory control blocks. Fifteen bytes of data (the
; segment of our own MCB at +0E and five state words), then four functions. CheckMCB merges free
; blocks in the chain; CheckMemoryChain walks the chain, relocates the environment block and frees
; the one-paragraph block it allocated; ReserveMemoryBelowVideo and TrimMemoryToVideo size the
; program's memory below the probed segment (the "video top" reading is a hypothesis [H], see
; gVideoTopSeg). The last two run only for the Tandy graphics mode:
; StartupGame (07C8:0199) calls CheckMemoryChain (07C8:01BB) and ReserveMemoryBelowVideo (07C8:01F2)
; for graphics choice 1, and ShutdownGame (07C8:060D) calls TrimMemoryToVideo when ds:8102 is 1
; (src/E07C8.C:343-344), including after a failed CheckMemoryChain: StartupGame returns 0 before it
; stores gMemTopSeg, so Trim then runs with gMemTopSeg at its loaded 0 (the EXE image has 0 there).
; WHITEPAPER 2.6 reads argv digit 1 as Tandy and 3 as VGA, and START1's menu order is CGA, Tandy,
; EGA, VGA (WHITEPAPER line 105); the B800/A000 probe is in the code at 1990:0139 to 0150. The
; names are supported by the code and the callers, never placeholders.

        .MODEL  LARGE
        .CODE

        db      0                       ; 1990:0001: data byte with no reader (the EXE's byte); the
                                        ;   open question from work/reviews/e1bb1.md applies here too
gMemTopSeg    dw 0                      ; 1990:0002: segment where the program's memory must end.
                                        ;   ReserveMemoryBelowVideo stores the end of DOS memory
                                        ;   here (1990:01F0: the segment just past the last block);
                                        ;   the caller StartupGame then stores atol(gArgv->number)
                                        ;   over it (07C8:0212, src/E07C8.C:189). What that numeric
                                        ;   string holds is Purpose unknown: nothing in src/ or
                                        ;   docs/ writes ds:8894. TrimMemoryToVideo reads it back.
gVideoTopSeg  dw 0                      ; 1990:0004: the base of the memory reserved below the
                                        ;   probed segment (the caller uses it as the base of the
                                        ;   64000 byte Tandy buffer, src/E07C8.C:206 to 207 and
                                        ;   220). "Video top" is a
                                        ;   hypothesis [H]: ReserveMemoryBelowVideo probes the
                                        ;   segment whose B800:0000 marker aliases (or A000,
                                        ;   1990:0139 to 0150) and subtracts (n-1) x 32 KB
                                        ;   (1990:01A8 to 01B6), but nothing in src/ or docs/ says
                                        ;   that segment is where video memory ends. The reading
                                        ;   would be confirmed by a docs source for where video
                                        ;   memory ends, or a source showing the buffer lies in
                                        ;   that memory. It is then lowered to memtop plus the
                                        ;   moved footprint when memtop is at or below that value
                                        ;   less the footprint (1990:01FE to 0207: the add is
                                        ;   skipped when memtop is above), so it never ends above
                                        ;   the value the subtraction left it at (1990:01B6).
gMovedSeg     dw 0                      ; 1990:0006: data segment DOS gave the relocated copy of the
                                        ;   environment block (1990:00E0). ReserveMemoryBelowVideo
                                        ;   copies that copy to its final place and frees it
                                        ;   (1990:0268 to 026D, ES = gMovedSeg, INT 21h 49h).
gMovedParas   dw 0                      ; 1990:0008: the environment block's size plus its own MCB
                                        ;   paragraph (its footprint): CheckMemoryChain loads
                                        ;   es:[3], adds 1 and stores it (1990:00D1); the
                                        ;   allocation then requests the size without the +1
                                        ;   (1990:00D6). ReserveMemoryBelowVideo reads it at
                                        ;   1990:01F9, 1990:0202 and 1990:0212; its copy count is
                                        ;   the copy's own MCB size plus one (1990:023F to 0243),
                                        ;   which equals the footprint.
        db      0,0                     ; 1990:000A: unused (no reader in the unit)
gNewMemTopSeg dw 0                      ; 1990:000C: the program's new top of memory, stored into
                                        ;   PSP:0002 (1990:0264) when a block was moved. The moved
                                        ;   copy's MCB lies exactly here (es at 1990:0236; bx set at
                                        ;   1990:0221 to 0225), so the copy starts at this top and
                                        ;   its gMovedParas paragraphs reach up to gVideoTopSeg.
gMcbSeg dw      0                       ; 1990:000E: MCB of the one-paragraph block that
                                        ;   CheckMemoryChain allocates (1990:0079 to 007E) and
                                        ;   frees (1990:0120 to 0129); set at 1990:0088

; 1990:0010: walk the MCB chain from gMcbSeg, merging free blocks; AX = 0 if it ends with a
; proper 'Z' block, 1 if the chain is broken
        PUBLIC  CheckMCB                ; @ 1990:0010
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

; 1990:0071: allocate a one-paragraph block to reach the MCB chain and walk it. A free block is
; skipped (owner word zero, 1990:00B4 to 00BA); a block whose owner is not this program is skipped
; (owner must equal ds, which holds the PSP, 1990:00BC to 00C3); and a block whose MCB is not the
; segment at PSP:002C minus one is skipped (1990:00C5 to 00C9). When the current block is the
; environment block (its MCB is PSP:002C minus one) it is moved. The copy is
; requested with the same size (1990:00D6 to 1990:00D9) and DOS chooses where the copy goes (its
; default allocation strategy, which this unit leaves unchanged: there is no INT 21h AH=58h),
; PSP:002C is pointed at the copy (1990:00DD), the bytes are copied (1990:00EC to 1990:00FD)
; and the old block is marked free by zeroing its owner (1990:0101). Then the one-paragraph block is
; freed.
; On the normal path the last step calls CheckMCB (1990:0110), which merges the free blocks as a side
; effect, so AX is CheckMCB's result there. Three error exits set AX = 1 directly, without CheckMCB:
; the one-paragraph allocation fails (1990:0082, jumping past the free to 1990:0116: there is no
; block to free), the walk finds a block type that is neither 'M' nor 'Z' (1990:00B1), and the
; environment copy's allocation fails (1990:00DB, jb); the last two reach 1990:011C, which frees the
; one-paragraph block first. The caller StartupGame tests the result at 07C8:01C0
; (src/E07C8.C:181) and prints "Memory chain corrupt!" on 1; 07C8:01BB is the call itself.
        PUBLIC  CheckMemoryChain        ; @ 1990:0071
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

; 1990:0133: find the segment whose byte at offset 0 aliases B800:0000: write the marker there
; (1990:014A) and scan down in 16 KB steps. The first test is A000 minus 16 KB (0x400 paragraphs,
; 1990:0155 to 015D), the scan goes down while the segment is above 3C00h (1990:015A to 015D), and
; A000 is used when nothing matches (the probeDone path, 1990:0197 to 01A1). That segment is read
; as the top of video memory only as a hypothesis [H] (see gVideoTopSeg), and it is not what is
; stored: the stored value is that segment less (n-1) x 32 KB, then lowered. The argument n
; is the first word argument of the far call, at bp+6; it is a count of 32 KB steps, not the
; graphics mode: the code computes (n-1) shifted by 11 paragraphs (1990:01A8 to 1990:01B4), so it
; reserves (n-1) x 32 KB below the probed top. The only caller passes 3 (src/E07C8.C:188, lcall at
; 07C8:01F2), a 64 KB reserve. It reaches the chain by allocating a one-paragraph block (INT 21h
; AH=48h at 1990:01C0) but does not test the carry flag, unlike CheckMemoryChain (jae at
; 1990:0080): when the allocation fails AX holds DOS's error code (7 or 8), so the MCB type is read
; at segment 6 or 7; if that byte is not 'M' the one-word-out exit marked below is taken, and if it
; is 'M' the chain is walked from there. Then it shrinks the last block of the chain, the one whose
; type is 'Z' (1990:01DE to 1990:01E4, store at 1990:021D; the owner is not checked), and frees the
; one-paragraph block it allocated (1990:0226 to 0229, INT 21h AH=49h). Only when a block was moved
; (1990:0231) it points PSP:002C at that moved block's data segment (its MCB plus one, 1990:0256 to
; 025B), moves the relocated copy to gNewMemTopSeg and frees the old copy: the copy's MCB sits
; exactly at gNewMemTopSeg (es set at 1990:0236 from bx, set at 1990:0221 to 0225), so the copy
; starts at the program's new top and its gMovedParas paragraphs reach up to gVideoTopSeg. Then it
; calls CheckMCB (1990:0271) to merge the freed block into the chain and stores that new top into
; PSP:0002 (1990:0264). The result is gVideoTopSeg after
; the step at 1990:0202 to 0207 (loaded at 1990:0274), not gMemTopSeg; the caller uses it as memtop
; (src/E07C8.C:188, slots 2 and 3 at E07C8.C:220). The CheckMCB result is overwritten at 1990:0274.
        PUBLIC  ReserveMemoryBelowVideo ; @ 1990:0133
ReserveMemoryBelowVideo PROC FAR
        push    bp
        mov     bp,sp
        push    bx
        push    ds
        push    es
        mov     word ptr cs:gVideoTopSeg,9800h ; overwritten before use (1990:01B6)
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
; QUIRK 1990:01CE: the jump to retTop is taken when the byte at the block's MCB is not 'M' (test to
;   branch at 1990:01C6 to 01CC); it leaves the allocated data segment (its MCB plus one, pushed at
;   1990:01C2) on the stack, so the exit pops one word out (the far return takes the wrong words)
;   and the block is not freed. Two ways in: in a valid chain a
;   block that is not 'M' is the 'Z' block, so the path is taken when the one-paragraph block is the
;   last one, and the code does not test for a corrupt chain; and the allocation that finds the block
;   does not test its own result (1990:01C0, no carry check, unlike CheckMemoryChain's jae at
;   1990:0080), so on failure AX is DOS's error code (7 or 8), the MCB type is read at the segment
;   made from it (1990:01C3 to 01C6), and the path is taken when that byte is not 'M'.
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
        mov     cs:gMemTopSeg,ax
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

; 1990:027D: shrink the last MCB of the chain down to gMemTopSeg, then free the one-paragraph
; block (1990:02BD to 02C0, INT 21h AH=49h). gMemTopSeg has one of two inputs here. On the path
; through StartupGame it is atol(gArgv->number) (07C8:0212). On the path where CheckMemoryChain
; failed, StartupGame returns 0 (src/E07C8.C:181-185) before that store, and g8102 is already 1
; (src/E07C8.C:178), so ShutdownGame still calls this (src/E07C8.C:343-344) with gMemTopSeg at its
; loaded value, 0 (the EXE image has 0 at 1990:0001 to 000F). With gMemTopSeg 0 the last block's
; size is set to 0 less its data segment (1990:02B0 to 02B9), a wrapped 16 bit value, and the walk
; (1990:0299 to 02AC) only looks for a 'Z' type, so it does not stop at a broken chain. The name's
; claim that this value is the video top is a hypothesis [H]: the bytes show only that the last
; block is set to end at gMemTopSeg (1990:02B0 to 02B9). ReserveMemoryBelowVideo stored the end of
; DOS memory there (1990:01F0), and nothing in src/ or docs/ shows that the atol string holds a
; video-buffer segment (its writer is Purpose unknown). It would be confirmed by a writer of ds:8894
; whose +0x18 numeric string is a video-buffer top (the value ReserveMemoryBelowVideo computes), or
; by a docs source. Like ReserveMemoryBelowVideo, it allocates the one-paragraph block (INT 21h
; AH=48h at 1990:0288) without testing the carry flag: on failure AX is DOS's error code (7 or 8),
; the MCB type is read at segment 6 or 7 (1990:028B to 028E), and the one-word-out exit marked
; below is taken when that byte is not 'M'. Called from ShutdownGame at exit for the Tandy mode,
; when ds:8102 is 1 (07C8:060D, src/E07C8.C:343-344).
        PUBLIC  TrimMemoryToVideo       ; @ 1990:027D
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
; QUIRK 1990:0296: the jump to trimDone is taken when the byte at the block's MCB is not 'M' (test to
;   branch at 1990:028E to 0296); it leaves the allocated data segment (its MCB plus one, pushed at
;   1990:028A) on the stack, so the exit pops one word out (the far return takes the wrong words)
;   and the block is not freed. Two ways in: in a valid chain a
;   block that is not 'M' is the 'Z' block, so the path is taken when the one-paragraph block is the
;   last one, and the code does not test for a corrupt chain; and the allocation that finds the block
;   does not test its own result (1990:0288, no carry check, unlike CheckMemoryChain's jae at
;   1990:0080), so on failure AX is DOS's error code (7 or 8), the MCB type is read at the segment
;   made from it (1990:028B to 028E), and the path is taken when that byte is not 'M'.
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

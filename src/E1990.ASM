; Unit at EOB.EXE 1990:0001 (710 bytes): DOS memory control blocks. Fifteen bytes of data (the
; segment of our own MCB at +0E and five scratch words), then four functions. CheckMCB merges free
; blocks in the chain; CheckMemoryChain walks the chain, relocates the environment block and frees
; the probe block; ReserveMemoryBelowVideo and TrimMemoryToVideo size the program's memory below
; the video buffer. The last two run only for the Tandy graphics mode (main calls them when the
; graphics argument reduces to 1, WHITEPAPER 2.6); the names are placeholders, the Tandy link is
; inferred from that caller and from the B800/A000 probes.

        .MODEL  LARGE
        .CODE

        db      0                       ; 1990:0001
gMemTopSeg    dw 0                      ; 1990:0002: segment where the program's memory must end;
                                        ;   ReserveMemoryBelowVideo computes the end of DOS memory
                                        ;   here, its caller then stores the video top over it
                                        ;   (07C8:0212), TrimMemoryToVideo reads it
gVideoTopSeg  dw 0                      ; 1990:0004: top segment of video memory found by
                                        ;   ReserveMemoryBelowVideo, whose result it is
gMovedSeg     dw 0                      ; 1990:0006: block segment of the environment block that
                                        ;   CheckMemoryChain moved (its copy is released later)
gMovedParas   dw 0                      ; 1990:0008: size in paragraphs of that block
        db      0,0                     ; 1990:000A
gNewMemTopSeg dw 0                      ; 1990:000C: new top of memory stored into PSP:0002
mcbseg  dw      0                       ; 1990:000E: MCB of our block, set by 1990:0071

; 1990:0010: walk the MCB chain from mcbseg, merging free blocks; AX = 0 if it ends with a
; proper 'Z' block, 1 if the chain is broken
        PUBLIC  CheckMCB                ; @ 1990:0010
CheckMCB PROC NEAR
        push    bx
        push    ds
        push    es
        mov     es,cs:mcbseg
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

; 1990:0071: allocate a one-paragraph block to reach the MCB chain, walk it, and when the current
; block is the environment block (the MCB of the segment at PSP:002C) grow and move it into the
; free block that follows; then free the probe block. AX = 0 when the chain ends in a 'Z' block,
; 1 when a block type is wrong or an allocation fails; the caller prints "Memory chain corrupt!"
; on 1 (07C8:01BB).
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
        mov     cs:mcbseg,ax
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
        mov     ax,cs:mcbseg
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

; 1990:0133: find the top of video memory by writing a marker at B800:0000 and scanning down from
; A000:0000, then reserve room below it: walk to the last MCB, shrink the free block so the program
; ends at that top, move any block CheckMemoryChain had relocated and free it. Argument (far, at
; bp+6) is the graphics mode; result is the top segment found (globals gVideoTopSeg, gMemTopSeg).
        PUBLIC  ReserveMemoryBelowVideo ; @ 1990:0133
ReserveMemoryBelowVideo PROC FAR
        push    bp
        mov     bp,sp
        push    bx
        push    ds
        push    es
        mov     word ptr cs:gVideoTopSeg,9800h
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

; 1990:027D: shrink the last MCB of the chain to the segment ReserveMemoryBelowVideo left in
; gMemTopSeg (the video top), then free the probe block. Called at exit for the Tandy mode, when
; ds:8102 is 1 (07C8:060D).
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

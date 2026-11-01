; Unit at EOB.EXE 19FC:0007 (420 bytes, 1 function), kind asm [H] by the map. Placeholder name.
;
; A byte oriented LZ encoder: it reads an input buffer (arg 1, a far pointer) of a given length
; (arg 3, a word) and writes a compressed stream to arg 2 (a far pointer), returning the number of
; bytes written. Both far pointers are first run through the far pointer normaliser 1BA8:0081
; (DX += offset >> 4, offset &= 0Fh). The emitted tokens are the LCW ones the loader's format-4
; decoder (1D9A:0013) reads: 81h..BFh a literal run of (byte & 3Fh) bytes (the run's count
; byte is emitted once as 80h and then incremented in place up to BFh), 00h..7Fh a short back
; reference (high nibble length - 3, low nibble the high 4 bits of the 12-bit distance, next byte
; the low 8, counted back from the current position), C0h..FDh a longer back reference
; ((byte & 3Fh) + 3 bytes, then a distance word), FEh a fill (a count word then the byte), FFh a
; long back reference (a length word then a distance word), and 80h ends the stream. For the two
; word forms the distance is measured from cs:inBase, the source base (the decoder adds it to its
; output base, the same index, because the decompressed prefix has the same length). The first
; literal is written as an explicit 81h plus the byte, which opens a run. Runs of 41h or more equal
; bytes are found by scanning with repe scasb and become FEh fills; shorter matches are searched
; with repne scasb over the input base and extended with repe cmpsb. The eight words of state are
; kept in the code segment. No caller was found (the packet: no direct call, no far pointer
; anywhere in EOB.EXE or its overlays).
;
; The map's segment starts at 19FC:0007, the alignment byte between the previous module (19F3) and
; this one; the frame is 19FC:0008. Reproduced here as the leading db 0.

        .MODEL  LARGE

        EXTRN   _NormFar:FAR            ; @ 1BA8:0081

        .CODE

        PUBLIC  _f19FC_0007             ; @ 19FC:0007
_f19FC_0007 PROC FAR
        db      0                       ; the alignment byte at 19FC:0007
        push    bp
        mov     bp,sp
        push    bx
        push    cx
        push    di
        push    ds
        push    dx
        push    es
        push    si
        mov     ax,[bp+6]               ; arg 1: source
        mov     dx,[bp+8]
        call    far ptr _NormFar
        mov     si,ax
        mov     ds,dx
        mov     ax,[bp+10]              ; arg 2: destination
        mov     dx,[bp+12]
        call    far ptr _NormFar
        mov     di,ax
        mov     es,dx
        mov     dx,[bp+14]              ; arg 3: source length
        cld
        mov     bp,si
        add     bp,dx                   ; bp = end of the source
        mov     word ptr cs:[litOpen],1
        mov     word ptr cs:[outBase],di
        mov     word ptr cs:[inBase],si
        mov     word ptr cs:[litCount],di
        mov     al,81h                  ; open the first literal run
        stosb
        lodsb
        stosb
mainloop:                               ; 19FC:0053
        mov     word ptr cs:[outPtr],di
        mov     di,es
        mov     word ptr cs:[outSeg],di
        mov     di,ds
        mov     es,di                   ; es = the source segment for the searches
        mov     di,cs:[inBase]
        mov     word ptr cs:[bestLen],1
search:                                 ; 19FC:006F
        mov     al,[si]
        cmp     al,[si+40h]             ; a run of equal bytes 40h ahead?
        jne     nextcand
        mov     bx,di
        mov     di,si
        mov     cx,bp
        sub     cx,di
        repe    scasb                   ; scan the equal run
        dec     di
        mov     cx,di
        sub     cx,si
        cmp     cx,41h
        jb      noRun
        mov     word ptr cs:[litOpen],0 ; close the literal run
        mov     si,di
        mov     es,cs:[outSeg]
        mov     di,cs:[outPtr]
        mov     ah,al
        mov     al,0feh                 ; FEh fill: count, byte
        stosb
        xchg    cx,ax
        stosw
        mov     al,ch
        stosb
        mov     word ptr cs:[outPtr],di
        mov     di,ds
        mov     es,di
        mov     di,bx
        jmp     search
noRun:                                  ; 19FC:00B4
        mov     di,bx
nextcand:                               ; 19FC:00B6
        mov     cx,si
        sub     cx,di
        je      notfound
        repne   scasb                   ; scan forward for this byte
        jne     notfound
        mov     bx,cs:[bestLen]
        mov     ah,[bx+si-1]
        cmp     ah,es:[bx+di-2]
        jne     nextcand
        mov     dx,si
        mov     bx,di
        dec     di
        mov     cx,bp
        sub     cx,si
        repe    cmpsb                   ; extend the match
        mov     si,dx
        mov     ax,di
        sub     ax,bx
        mov     di,bx
        cmp     ax,cs:[bestLen]
        jb      search
        mov     cs:[bestLen],ax
        dec     bx
        mov     cs:[bestPos],bx
        jmp     search
notfound:                               ; 19FC:00F5
        mov     cx,cs:[bestLen]
        mov     es,cs:[outSeg]
        mov     di,cs:[outPtr]
        cmp     cx,2
        jbe     litrun
        cmp     cx,0ah
        ja      match2
        mov     ax,si
        sub     ax,cs:[bestPos]
        cmp     ax,0fffh
        ja      match2
        mov     bl,cl                   ; 00h..7Fh: (length - 3) << 4 | distance >> 8, distance
        sub     bl,3
        mov     cl,4
        shl     bl,cl
        add     ah,bl
        xchg    ah,al
        jmp     short emitoff
match2:                                 ; 19FC:0129
        cmp     cx,40h
        ja      longmatch
        sub     cl,3
        or      cl,0c0h                 ; C0h..FDh: (length - 3), then the distance word
        mov     al,cl
        stosb
        jmp     short storeoff
litrun:                                 ; 19FC:0139
        cmp     word ptr cs:[litOpen],0
        jne     putlit
newlit:                                 ; 19FC:0141
        mov     word ptr cs:[litCount],di
        mov     al,80h
        stosb
putlit:                                 ; 19FC:0149
        mov     bx,cs:[litCount]
        cmp     byte ptr es:[bx],0bfh
        je      newlit
        inc     byte ptr es:[bx]
        lodsb
        stosb
        mov     word ptr cs:[litOpen],1
        jmp     short loopend
longmatch:                              ; 19FC:0162
        mov     al,0ffh                 ; FFh: length word, distance word
        stosb
        mov     ax,cs:[bestLen]
        stosw
storeoff:                               ; 19FC:016A
        mov     ax,cs:[bestPos]
        sub     ax,cs:[inBase]
emitoff:                                ; 19FC:0173
        stosw
        add     si,cs:[bestLen]
        mov     word ptr cs:[litOpen],0
loopend:                                ; 19FC:0180
        cmp     si,bp
        jae     finish
        jmp     mainloop
finish:                                 ; 19FC:0187
        mov     ax,80h                  ; end of stream
        stosb
        mov     ax,di
        sub     ax,cs:[outBase]         ; return the number of bytes written
        pop     si
        pop     es
        pop     dx
        pop     ds
        pop     di
        pop     cx
        pop     bx
        pop     bp
        ret
_f19FC_0007 ENDP

litOpen dw      0                       ; 19FC:019B
outBase dw      0                       ; 19FC:019D
inBase  dw      0                       ; 19FC:019F
litCount dw     0                       ; 19FC:01A1
outSeg  dw      0                       ; 19FC:01A3
outPtr  dw      0                       ; 19FC:01A5
bestLen dw      0                       ; 19FC:01A7
bestPos dw      0                       ; 19FC:01A9

        END

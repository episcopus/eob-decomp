; Unit at EOB.EXE 19FC:0007 (420 bytes, 1 function), kind asm [H] by the map. The one function is
; CompressLcw (19FC:0007), the inverse of Unpack4 (1D9A:0013, src/E1B82.C:22): an LCW encoder.
; What the encoder is for in the game is unknown: no caller was found (see the note below).
;
; A byte oriented LZ encoder: it reads an input buffer (arg 1, a far pointer) of a given length
; (arg 3, a word) and writes a compressed stream to arg 2 (a far pointer), returning the number of
; bytes written. Both far pointers are first run through the far pointer normaliser 1BA8:0081
; (DX += offset >> 4, offset &= 0Fh). The emitted tokens are the LCW ones Unpack4 (1D9A:0013), the
; loader's format-4 decoder, reads: 81h..BFh a literal run of (byte & 3Fh) bytes (the run's count
; byte is emitted once as 80h and then incremented in place up to BFh), 00h..7Fh a short back
; reference (high nibble length - 3, low nibble the high 4 bits of the 12-bit distance, next byte
; the low 8, counted back from the current position), C0h..FDh a longer absolute copy
; ((byte & 3Fh) + 3 bytes, then a distance word), FEh a fill (a count word then the byte), FFh a
; long absolute copy (a length word then a distance word), and 80h ends the stream. For the two
; word forms the distance is measured from cs:gInBase, the source base (the decoder adds it to its
; output base, the same index, because the decompressed prefix has the same length). The first
; literal is written as an explicit 81h plus the byte, which opens a run. Runs of 41h or more equal
; bytes are found by scanning with repe scasb and become FEh fills; shorter matches are searched
; with repne scasb over the input base and extended with repe cmpsb. The eight words of state are
; kept in the code segment, each with its role in the data comments below. The token set above is
; the one of the decoder model tools/re/wwfmt.py lcw [H]; no round trip of this encoder has run.
;
; No caller was found: the packet reports no direct call (tools/agent/packet.py:149 checks direct
; calls only), and a byte search of EOB.EXE and the seven overlay images (build/re/eob-overlays)
; finds neither the far call bytes 9a 07 00 fc 19 / 9a 08 00 fc 19 nor a bare far pointer
; 07 00 fc 19 / 08 00 fc 19. The overlays were searched here; the game role stays unknown.
;
; The limits the encoder keeps: matches shorter than 3 bytes become literals; the short 00h..7Fh
; form is used only for lengths 3..0Ah and distances <= 0FFFh; C0h..FDh carry lengths up to 40h;
; longer matches take FFh; literal runs are capped at 3Fh bytes (a full count byte starts a new
; 80h run). All offsets are 16 bits from the normalised starts, and add bp,dx (end of the source)
; wraps at 10000h when offset + length passes FFFFh; the output has the same limit. A caller must
; keep both buffers inside 64 KiB from their normalised starts.
;
; The map's segment starts at 19FC:0007, a segment-start byte the map gives to this unit (the same
; kind as 1BA8:0005 in src/E1BA8.ASM); the frame is 19FC:0008. Reproduced here as the leading db 0.

        .MODEL  LARGE

        EXTRN   _NormFar:FAR            ; @ 1BA8:0081

        .CODE

        PUBLIC  _CompressLcw             ; @ 19FC:0007
_CompressLcw PROC FAR
        db      0                       ; the segment-start byte the map gives to this unit (19FC:0007)
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
        mov     word ptr cs:[gLitOpen],1
        mov     word ptr cs:[gOutBase],di
        mov     word ptr cs:[gInBase],si
        mov     word ptr cs:[gLitHdr],di
        mov     al,81h                  ; open the first literal run
        stosb
        lodsb
        stosb
mainloop:                               ; 19FC:0053
        mov     word ptr cs:[gOutPtr],di
        mov     di,es
        mov     word ptr cs:[gOutSeg],di
        mov     di,ds
        mov     es,di                   ; es = the source segment for the searches
        mov     di,cs:[gInBase]
        mov     word ptr cs:[gBestLen],1
search:                                 ; 19FC:006F
        mov     al,[si]
        cmp     al,[si+40h]             ; precheck: a candidate fill run (byte 40h ahead equal)
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
        mov     word ptr cs:[gLitOpen],0 ; close the literal run
        mov     si,di
        mov     es,cs:[gOutSeg]
        mov     di,cs:[gOutPtr]
        mov     ah,al
        mov     al,0feh                 ; FEh fill: count, byte
        stosb
        xchg    cx,ax
        stosw
        mov     al,ch
        stosb
        mov     word ptr cs:[gOutPtr],di
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
        mov     bx,cs:[gBestLen]
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
        cmp     ax,cs:[gBestLen]
        jb      search
        mov     cs:[gBestLen],ax
        dec     bx
        mov     cs:[gBestPos],bx
        jmp     search
notfound:                               ; 19FC:00F5
        mov     cx,cs:[gBestLen]
        mov     es,cs:[gOutSeg]
        mov     di,cs:[gOutPtr]
        cmp     cx,2
        jbe     litrun
        cmp     cx,0ah
        ja      absCopy
        mov     ax,si
        sub     ax,cs:[gBestPos]
        cmp     ax,0fffh
        ja      absCopy
        mov     bl,cl                   ; 00h..7Fh: (length - 3) << 4 | distance >> 8, distance
        sub     bl,3
        mov     cl,4
        shl     bl,cl
        add     ah,bl
        xchg    ah,al
        jmp     short emitoff
absCopy:                                 ; 19FC:0129
        cmp     cx,40h
        ja      longmatch
        sub     cl,3
        or      cl,0c0h                 ; C0h..FDh: (length - 3), then the distance word
        mov     al,cl
        stosb
        jmp     short storeoff
litrun:                                 ; 19FC:0139
        cmp     word ptr cs:[gLitOpen],0
        jne     putlit
newlit:                                 ; 19FC:0141
        mov     word ptr cs:[gLitHdr],di ; gLitHdr = offset of this run's count byte (the 80h below)
        mov     al,80h
        stosb
putlit:                                 ; 19FC:0149
        mov     bx,cs:[gLitHdr]
        cmp     byte ptr es:[bx],0bfh   ; a full 3Fh run starts a new one
        je      newlit
        inc     byte ptr es:[bx]        ; bump the run's count byte
        lodsb
        stosb
        mov     word ptr cs:[gLitOpen],1
        jmp     short loopend
longmatch:                              ; 19FC:0162
        mov     al,0ffh                 ; FFh: length word, distance word
        stosb
        mov     ax,cs:[gBestLen]
        stosw
storeoff:                               ; 19FC:016A
        mov     ax,cs:[gBestPos]
        sub     ax,cs:[gInBase]
emitoff:                                ; 19FC:0173
        stosw
        add     si,cs:[gBestLen]
        mov     word ptr cs:[gLitOpen],0
loopend:                                ; 19FC:0180
        cmp     si,bp
        jae     finish
        jmp     mainloop
finish:                                 ; 19FC:0187
        mov     ax,80h                  ; end of stream
        stosb
        mov     ax,di
        sub     ax,cs:[gOutBase]         ; return the number of bytes written
        pop     si
        pop     es
        pop     dx
        pop     ds
        pop     di
        pop     cx
        pop     bx
        pop     bp
        ret
_CompressLcw ENDP

gLitOpen dw 0                           ; 19FC:019B  a literal run is open (1) or closed (0)
gOutBase dw 0                           ; 19FC:019D  output start (normalised offset), for the return length
gInBase  dw 0                           ; 19FC:019F  input start (normalised offset); the absolute copy
                                        ;   offsets (C0h..FDh, FFh) are measured from it, the short
                                        ;   00h..7Fh distance is not
gLitHdr  dw 0                           ; 19FC:01A1  output offset of the open literal run's count byte
gOutSeg  dw 0                           ; 19FC:01A3  output segment, saved while es holds the source
gOutPtr  dw 0                           ; 19FC:01A5  output position, saved while es holds the source
gBestLen dw 0                           ; 19FC:01A7  best match length so far
gBestPos dw 0                           ; 19FC:01A9  best match input offset so far

        END

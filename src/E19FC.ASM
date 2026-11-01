; Unit at EOB.EXE 19FC:0007 (420 bytes, 1 function), kind asm [H] by the map. The one function is
; CompressLcw (19FC:0008), the inverse of Unpack4 (1D9A:0013, src/E1B82.C:39): an LCW encoder for
; the format-4 compressed files of whitepaper 4.2. What the encoder is for in the game is unknown:
; no caller was found (see the note below).
;
; The unit is written in assembly, which the map's kind asm says only as a guess [H]: it keeps its
; state in the code segment through cs: overrides and scans with the repe/repne string instructions,
; which the C compiler does not emit for C source. That the source was assembly and not C is
; therefore inferred from those forms, not proven.
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
; word forms the distance is an index from the start of the decompressed output: the encoder
; measures it from cs:gInBase, the start of the input, and the decoder adds it to the start of its
; output, so the two agree because the decoded prefix has the same length as the input prefix it
; came from. The first literal is written as an explicit 81h plus the byte, which opens a run. Runs
; of 41h or more equal bytes are found by scanning with repe scasb and become FEh fills; shorter
; matches are searched with repne scasb over the input base and extended with repe cmpsb. The eight
; words of state are kept in the code segment, each with its role in the data comments below. The
; token set above is the one of the decoder model tools/re/wwfmt.py lcw [H]; no round trip of this
; encoder has run.
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
; keep both buffers inside 64 KiB from their normalised starts, and pass a length of at least 1: the
; first literal (81h plus one byte) is written before any length test, so a length of 0 copies bytes
; at and past the source start instead of producing an empty stream.
;
; QUIRK 19FC:007E and 19FC:00D7: when a run of equal bytes reaches the end of the source, repe scasb
; (the fill scan) or repe cmpsb (the match extension) runs out of count with DI one past the last
; compared byte, not on a mismatch, but the length is computed as DI - start (after the dec di at
; 19FC:0080 for the fill), so such a match is one byte short of the run. The encoded output is still
; correct, because the last byte is then emitted as a literal or a shorter match. This is the
; original's behaviour, not a decompile error.
;
; The map's segment starts at 19FC:0007, a lone 00 byte that is data, not code: the segment table
; (build/re/eob.json) has the 19F3 unit (segment 27, start 106302) at 137 bytes, so it ends at
; 106439, which is 19FC:0007 (19FC is paragraph 6652, linear 106432; 19FC:0000..0006 repeat the
; 19F3 unit's last seven bytes). The 00 is therefore the first byte after the 19F3 unit and the
; 19FC segment's first byte in the table. It is emitted as a public byte symbol (_f19FC_0007) so
; the registry records it at L2, and CompressLcw starts at 19FC:0008 with the same 419 code bytes.
; The unit stays 420 bytes.

        .MODEL  LARGE

        EXTRN   _NormFar:FAR            ; @ 1BA8:0081

        .CODE

        PUBLIC  _f19FC_0007
_f19FC_0007 LABEL BYTE                   ; @ 19FC:0007
        db      0                       ; the segment-start byte: data, not code

; 19FC:0008: encode. Arg 1 at [bp+6]/[bp+8] is the source far pointer, arg 2 at [bp+0Ah]/[bp+0Ch] the
; destination, arg 3 at [bp+0Eh] the source length in bytes. Returns AX = the number of bytes written
; to the destination, the whole stream (the leading 81h byte and the trailing 80h end byte included).
; Reads and writes the eight cs: state words declared below; the epilogue restores SI, ES, DS and DI
; to the values the caller had, so AX is the only result.
        PUBLIC  _CompressLcw             ; @ 19FC:0008
_CompressLcw PROC FAR
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
        repe    scasb                   ; scan the equal run; one short at bp (QUIRK 19FC:007E)
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
        mov     al,0feh                 ; FEh fill: count word, byte
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
        mov     ah,[bx+si-1]            ; a candidate can only beat gBestLen if it agrees at offset
        cmp     ah,es:[bx+di-2]         ;   gBestLen - 1 (else the match cannot reach gBestLen); this
        jne     nextcand                ;   byte is gBestLen - 1 ahead of each start
        mov     dx,si
        mov     bx,di
        dec     di
        mov     cx,bp
        sub     cx,si
        repe    cmpsb                   ; extend the match; one short at bp (QUIRK 19FC:00D7)
        mov     si,dx
        mov     ax,di
        sub     ax,bx
        mov     di,bx
        cmp     ax,cs:[gBestLen]
        jb      search                  ; jb, not jbe: an equal length replaces the earlier match,
                                        ;   because the later candidate is nearer, and a smaller
                                        ;   distance can use the 2-byte short form
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

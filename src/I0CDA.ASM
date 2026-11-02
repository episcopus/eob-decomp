; Unit at INTRO.EXE 0CDA:0005 (51 bytes, 1 function and a lone byte at 0CDA:0005), kind asm [H] by
; the map. The same code is in EOB.EXE at 1D8C:0000 (unit e1d8c, src/E1D8C.ASM), 50 bytes, with the
; DGROUP offsets moved; INTRO.EXE's segment holds one byte more, the lone 0 at 0CDA:0005 before the
; function at the even offset 0CDA:0006. The map took that byte for the function's start (it had no
; call to the function to place it), so its `0CDA:0005` entry is that byte, not the function.

        .MODEL  LARGE
        .CODE

; 0CDA:0005: the lone 0 before the function at the even offset 0CDA:0006 (data, not code) [H]; the
; map counts it in the unit's 51 bytes and its 0CDA:0005 entry covers it.
        db      0

; 0CDA:0006: Purpose unknown: a 24-bit pseudo-random step over the three bytes at DGROUP:2310, seeded
; 34 12 76 by the file image (2310h appears nowhere else, so mov si,2310h is its only access). It shifts
; the byte at DGROUP:2310 right twice (bit 1 into the carry), rotates DGROUP:2312 then DGROUP:2311 left
; through the carry, rotates DGROUP:2310 right through the low bit of its own shifted value, and returns
; the new DGROUP:2310 exclusive-ored with the new DGROUP:2311 in AL (AH 0). ES and SI are saved. The far
; twin is E1D8C.ASM; no near or far call reaches either copy, so its use is not established.
        PUBLIC  _f0CDA_0006              ; @ 0CDA:0006
_f0CDA_0006 PROC FAR
        push    bp
        mov     bp,sp
        push    es
        push    si
        mov     ax,DGROUP                ; the segment to read the state through; the original's word
                                         ;   is a relocation to the group, fixed up at load (compare
                                         ;   src/E1D8C.ASM, which spells the same value 212Eh)
        mov     es,ax
        mov     si,2310h
        xor     ah,ah
        mov     al,es:[si]
        shr     al,1
        shr     al,1
        rcl     byte ptr es:[si+2],1
        rcl     byte ptr es:[si+1],1
        cmc
        sbb     al,es:[si]
        shr     al,1
        rcr     byte ptr es:[si],1
        mov     al,es:[si]
        xor     al,es:[si+1]
        pop     si
        pop     es
        pop     bp
        ret
_f0CDA_0006 ENDP

        END

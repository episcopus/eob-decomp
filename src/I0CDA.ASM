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

; 0CDA:0006: a 24-bit pseudo-random step over the three bytes of a static state at DGROUP:2310 (the
; file image holds the seed 34 12 76 there; 2310h appears nowhere else, so mov si,2310h is its only
; access). With b0 = DGROUP:2310, b1 = DGROUP:2311, b2 = DGROUP:2312 the instructions do, step by
; step:
;
;   al = b0; al = al >> 2                 (shr twice; bit 1 of the old b0 into the carry)
;   b2 = (b2 << 1) | carry                (rcl; carry = bit 7 of the old b2)
;   b1 = (b1 << 1) | carry                (rcl; carry = bit 7 of the old b1)
;   cmc: carry = !(bit 7 of the old b1)
;   al = (b0 >> 2) - b0 - carry           (sbb al,es:[si]; es:[si] is still the old b0)
;   al = al >> 1                          (shr; bit 0 of that result into the carry)
;   b0 = (b0 >> 1) | (carry << 7)         (rcr)
;   AX = b0 ^ b1, AH = 0
;
; The shifts act on the copy in AL, not on DGROUP:2310; the byte there is only read until the rcr
; writes it. Only b2 and b1 shift left (rcl), b0 shifts right (rcr). ES and SI are saved; DS is
; never read. The far twin is E1D8C.ASM (the same code over DGROUP:8484 in EOB.EXE).
;
; Callers: the INTRO copy is reached from the overlay ovl0 by eleven relocatable far calls
; (`9a 06 00 d0 00`, the segment word a fixup to this unit's segment 0CDA) in five functions
; (ovl0:015D, 0FD9, 1265, 1DF3, 23A3); each continues `mov ah,0` and uses the byte as a small value
; (`idiv` by 31, `and ax,7`, `test al,1`, `and ax,3`). The EOB copy is called twice from game unit
; e0ef3 (0EF3:2FA2, 0EF3:305F; src/E1D8C.ASM).
;
; Purpose unknown: those uses support "random byte" and nothing more specific, so the placeholder
; name stays.
        PUBLIC  _RandomByte              ; @ 0CDA:0006
_RandomByte PROC FAR
        push    bp
        mov     bp,sp
        push    es
        push    si
        mov     ax,DGROUP                ; the segment to read the state through; the word (0E9F, at
                                         ;   0CDA:000C) is a relocation to the group, fixed up at
                                         ;   load; the same DGROUP as src/E1D8C.ASM, which spells
                                         ;   212Eh in EOB.EXE
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
_RandomByte ENDP

        END

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
;   AX = new b0 ^ new b1, AH = 0          (the bytes the rcl and rcr just wrote)
;
; The two shr (lines 62 and 63) act on the copy in AL; DGROUP:2310 is only read until the rcr at
; line 69 writes it. Only b2 and b1 shift left (rcl), b0 shifts right (rcr). ES and SI are
; saved; DS is never read. The far twin is E1D8C.ASM (the same code over DGROUP:8484 in EOB.EXE).
;
; Callers: the INTRO copy is reached from the overlay ovl0 by eleven relocatable far calls
; (`9a 06 00 d0 00`, the segment word a fixup to this unit's segment 0CDA) in five functions:
; ovl0:015D (sites 05F5, 0644, 0650), ovl0:0FD9 (1015), ovl0:1265 (1397, 157A),
; ovl0:1DF3 (2043, 2092, 209E) and ovl0:23A3 (26F6, 2797), the sites as ovl00.bin offsets.
; Eight sites continue `mov ah,0` (05F5, 0644, 1397, 157A, 2043, 2092, 26F6, 2797); 0650 and
; 209E test only the low byte (`test al,1`), and 1015 drops the result (its next bytes,
; `mov ax,[2326h]; or ax,[2328h]`, are the caller's own loop test). The byte is used as a small
; value: `idiv` by 31 (05F5, 2043), `test ax,7` (0644, 2092), `and ax,3` then three `shl ax,1`
; (1397), `test ax,3` (157A), `test al,1` (0650, 209E) and `and ax,7; add ax,0x19` (26F6, 2797).
; The EOB copy is called twice from game unit e0ef3 (0EF3:2FA2, 0EF3:305F; src/E1D8C.ASM).
;
; The name _RandomByte has evidence: the routine steps the 24-bit state at DGROUP:2310 (the seed
; 34 12 76 in the image) and returns the XOR of the two bytes it has just written (new b0 ^ new b1),
; and its callers use the byte as a random value (src/IOV0.C declares and calls it RandomByte).
; No caller gives the byte a more specific meaning: the callers mask or take it modulo
; (`% 31`, `& 7`, `& 3`, `& 1`), some then scale or offset the result (`* 8 + 0xb4`,
; `+ 0x19`), and two drop it (src/IOV0.C:545 and ovl0:1015).
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

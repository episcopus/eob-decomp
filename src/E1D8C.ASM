; Unit at EOB.EXE 1D8C:0000 (50 bytes, 1 function), kind asm [H] by the map. A 24-bit
; pseudo-random step over the three bytes of a static state at DGROUP:8484 (the file image
; holds the initial value 34 12 76 there). With b0 = state[0], b1 = state[1], b2 = state[2]:
;
;   c  = bit 1 of b0
;   b2 = (b2 << 1) | c
;   b1 = (b1 << 1) | (bit 7 of the old b2)
;   c  = !(bit 7 of the old b1)
;   b0 = (b0 >> 1) | (((b0 >> 2) - b0 - c) & 1) << 7
;   AX = b0 ^ b1, AH = 0
;
; Only b2 and b1 shift left (rcl); b0 shifts right (rcr). The returned byte mixes only the
; updated b0 and b1; b2 reaches it only through b1. ES and SI are saved; DS is never read.
;
; The far call to it (`9a 00 00 8c 1d`) occurs exactly twice in EOB.EXE, both in game unit
; e0ef3 (segment 0EF3, `re/units.toml`; not the C runtime, which is segment 0000): 0EF3:2FA2
; continues `mov ah,0 / and ax,3` (a 2-bit value), and 0EF3:305F continues `test al,1`. The
; name RandomByte comes from the algorithm above and from its twin, INTRO.EXE's RandomByte
; (0CDA:0006, src/I0CDA.ASM): the same 50-byte generator (re/renames.toml, 1D8C:0000).
;
; 212Eh is DGROUP (whitepaper 2.x), so `mov ax,212Eh` (1D8C:0006) carries a segment
; relocation and is fixed up at load, not a constant. The offset 8484 appears nowhere else
; in the image (`mov si,8484h` is its only occurrence), so the first outputs are
; deterministic unless something seeds the state through another pointer.

        .MODEL  LARGE
        .CODE

        PUBLIC  _RandomByte              ; @ 1D8C:0000
_RandomByte PROC FAR
        push    bp
        mov     bp,sp
        push    es
        push    si
        mov     ax,212Eh
        mov     es,ax
        mov     si,8484h
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

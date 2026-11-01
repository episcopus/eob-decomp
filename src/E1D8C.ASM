; Unit at EOB.EXE 1D8C:0000 (50 bytes, 1 function), kind asm [H] by the map. A 24-bit
; pseudo-random step over three bytes at 212E:8484 (the caller is the C runtime's random
; helper at 0EF3:2EBC): rotate the low two bits out, shift the 3-byte state left, then
; subtract the old low byte and fold the high bytes together; AX = the new byte. Placeholder name.

        .MODEL  LARGE
        .CODE

        PUBLIC  _f1D8C_0000              ; @ 1D8C:0000
_f1D8C_0000 PROC FAR
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
_f1D8C_0000 ENDP

        END

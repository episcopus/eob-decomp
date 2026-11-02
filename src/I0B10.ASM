; Unit at INTRO.EXE 0B10:0000 (139 bytes, 3 functions): far copy with normalized far pointers. It is
; the same module as EOB.EXE's 1BA8:0006 (src/E1BA8.ASM) without that unit's leading data byte, so
; every function here sits six bytes below its EOB twin. Unpack0 (0B10:0000) copies CX bytes and
; then BX blocks of 64 KiB, renormalizing both far pointers after every block; NormalizeFarPointer
; (0B10:006C) normalizes a far pointer passed by value and returns it in DX:AX; NormFar (0B10:007B)
; is the normalizer itself, given AX = offset, DX = segment it returns AX = offset and 0Fh, DX =
; segment plus offset shifted right four. Named through the twin and its callers: i094f (EOB E19BC)
; and i0b5d (EOB E1C0E) call NormalizeFarPointer, and the format-0 unpackers call Unpack0.

        .MODEL  LARGE
        .CODE

        PUBLIC  _Unpack0                ; @ 0B10:0000
_Unpack0 PROC FAR
        push    bp
        mov     bp,sp
        push    ax
        push    bx
        push    cx
        push    di
        push    dx
        push    ds
        push    es
        push    si
        mov     si,[bp+6]
        mov     ds,[bp+8]
        mov     di,[bp+0Ah]
        mov     es,[bp+0Ch]
        mov     ax,di
        mov     dx,es
        push    cs
        call    near ptr _NormFar
        mov     di,ax
        mov     es,dx
        mov     ax,si
        mov     dx,ds
        push    cs
        call    near ptr _NormFar
        mov     si,ax
        mov     ds,dx
        mov     cx,[bp+0Eh]
        mov     bx,[bp+10h]
        shr     cx,1
        je      L0041
        rep     movsw
L0041:  jae     L0044
        movsb
L0044:  or      bx,bx
        je      L0068
L0048:  mov     ax,di
        mov     dx,es
        push    cs
        call    near ptr _NormFar
        mov     di,ax
        mov     es,dx
        mov     ax,si
        mov     dx,ds
        push    cs
        call    near ptr _NormFar
        mov     si,ax
        mov     ds,dx
        mov     cx,8000h
        rep     movsw
        dec     bx
        jne     L0048
L0068:  pop     si
        pop     es
        pop     ds
        pop     dx
        pop     di
        pop     cx
        pop     bx
        pop     ax
        pop     bp
        ret
_Unpack0 ENDP

        PUBLIC  _NormalizeFarPointer    ; @ 0B10:006C
_NormalizeFarPointer PROC FAR
        push    bp
        mov     bp,sp
        mov     ax,[bp+6]
        mov     dx,[bp+8]
        push    cs
        call    near ptr _NormFar
        pop     bp
        ret
_NormalizeFarPointer ENDP

        PUBLIC  _NormFar                ; @ 0B10:007B
_NormFar PROC FAR
        push    ax
        shr     ax,1
        shr     ax,1
        shr     ax,1
        shr     ax,1
        add     dx,ax
        pop     ax
        and     ax,0Fh
        ret
_NormFar ENDP

        END

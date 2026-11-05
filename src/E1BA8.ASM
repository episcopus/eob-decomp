; Unit at EOB.EXE 1BA8:0005 (140 bytes, 4 functions): far copy with normalized far pointers.
; PadBeforeUnpack0 (1BA8:0005) is the lone 00 pad the map calls a function (not code); Unpack0
; (1BA8:0006), the format 0 copy Unpack calls, copies CX bytes then BX blocks of 64 KiB, renormalizing
; the far pointers each block; NormalizePtr (1BA8:0072) normalizes a far pointer passed by value and
; returns it in DX:AX; NormFar (1BA8:0081) is the normalizer itself: given AX = offset, DX = segment,
; it returns AX = offset & 0Fh, DX = segment + offset >> 4. Evidence: re/renames.toml.

        .MODEL  LARGE
        .CODE

        PUBLIC  _PadBeforeUnpack0
_PadBeforeUnpack0 LABEL BYTE                     ; @ 1BA8:0005
        db      0

        PUBLIC  _Unpack0                ; @ 1BA8:0006
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

        PUBLIC  _NormalizePtr                ; @ 1BA8:0072
_NormalizePtr PROC FAR
        push    bp
        mov     bp,sp
        mov     ax,[bp+6]
        mov     dx,[bp+8]
        push    cs
        call    near ptr _NormFar
        pop     bp
        ret
_NormalizePtr ENDP

        PUBLIC  _NormFar                ; @ 1BA8:0081
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

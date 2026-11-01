; Unit at EOB.EXE 1BA8:0005 (140 bytes, 4 functions): far copy with normalized far pointers.
; The segment opens with one byte the map calls a function (a segment-start marker, not code);
; 1BA8:0006 copies CX bytes then BX blocks of 64 KiB, renormalizing the far pointers each block;
; 1BA8:0072 normalizes a far pointer passed by value; 1BA8:0081 is the normalizer itself:
; given AX = offset, DX = segment, it returns AX = offset & 0Fh, DX = segment + offset >> 4.
; Placeholder names.

        .MODEL  LARGE
        .CODE

        PUBLIC  _f1BA8_0005
_f1BA8_0005 LABEL BYTE                     ; @ 1BA8:0005
        db      0

        PUBLIC  _f1BA8_0006                ; @ 1BA8:0006
_f1BA8_0006 PROC FAR
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
        call    near ptr _f1BA8_0081
        mov     di,ax
        mov     es,dx
        mov     ax,si
        mov     dx,ds
        push    cs
        call    near ptr _f1BA8_0081
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
        call    near ptr _f1BA8_0081
        mov     di,ax
        mov     es,dx
        mov     ax,si
        mov     dx,ds
        push    cs
        call    near ptr _f1BA8_0081
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
_f1BA8_0006 ENDP

        PUBLIC  _f1BA8_0072                ; @ 1BA8:0072
_f1BA8_0072 PROC FAR
        push    bp
        mov     bp,sp
        mov     ax,[bp+6]
        mov     dx,[bp+8]
        push    cs
        call    near ptr _f1BA8_0081
        pop     bp
        ret
_f1BA8_0072 ENDP

        PUBLIC  _f1BA8_0081                ; @ 1BA8:0081
_f1BA8_0081 PROC FAR
        push    ax
        shr     ax,1
        shr     ax,1
        shr     ax,1
        shr     ax,1
        add     dx,ax
        pop     ax
        and     ax,0Fh
        ret
_f1BA8_0081 ENDP

        END

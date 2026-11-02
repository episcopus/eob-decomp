; Unit at INTRO.EXE 0921:0000 (36 bytes, 1 function): load the whole VGA palette, 256 colours of
; three 6-bit components, from a far buffer. Only INTRO.EXE links it (EOB.EXE has no twin).

        .MODEL  LARGE
        .CODE

; 0921:0000: SetVgaPalette(far *rgb): writes DAC index 0 to port 3C8h, then the 300h bytes at rgb
; (red, green, blue per colour) to the data port 3C9h, which steps the index itself. Every register
; it uses is saved.
        PUBLIC  _SetVgaPalette          ; @ 0921:0000
_SetVgaPalette PROC FAR
        push    bp
        mov     bp,sp
        push    ax
        push    cx
        push    dx
        push    si
        push    ds
        mov     si,[bp+6]
        mov     ds,[bp+8]
        mov     dx,3C8h                 ; DAC write index
        xor     ax,ax
        out     dx,al
        inc     dl                      ; 3C9h, DAC data
        mov     cx,300h                 ; 256 colours, three bytes each
next:   lodsb
        out     dx,al
        loop    next
        pop     ds
        pop     si
        pop     dx
        pop     cx
        pop     ax
        pop     bp
        ret
_SetVgaPalette ENDP

        END

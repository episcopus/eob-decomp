; Unit at EOB.EXE 19EA:0000 (34 bytes, 1 function): print a NUL-terminated far string with
; DOS function 06h (direct console output), every register it uses saved. Placeholder name.

        .MODEL  LARGE
        .CODE

        PUBLIC  _PrintString            ; @ 19EA:0000
_PrintString PROC FAR
        push    bp
        mov     bp,sp
        push    ax
        push    di
        push    ds
        push    dx
        push    si
        mov     si,[bp+6]
        mov     ds,[bp+8]
next:   lodsb
        or      al,al
        jz      done
        mov     dl,al
        mov     ah,6
        int     21h
        jmp     next
done:   pop     si
        pop     dx
        pop     ds
        pop     di
        pop     ax
        pop     bp
        ret
_PrintString ENDP

        END

; Unit at EOB.EXE 1D8F:0002 (185 bytes, 3 functions), kind asm [H] by the map. INT 1Ch timer
; support. The unit begins with the saved old INT 1Ch vector (offset at cs:[2], segment at
; cs:[4], both zero at load): 1D8F:0010 installs our handler (cs:0055) and saves the old vector,
; 1D8F:0038 restores it, 1D8F:0002 reads the 32-bit tick counter at ds:8496/8498 with interrupts
; off. The handler bumps a bank of 16-bit counters and runs a countdown. Callers: 07C8:00E4
; (install, from e07c8_0001) and 07C8:05DE (uninstall, from e07c8_058A). Placeholder names.

        .MODEL  LARGE
        .CODE

; 1D8F:0002: the saved INT 1Ch vector lives at cs:[2]/cs:[4]; then AX:DX = the 32-bit tick
; counter at ds:8496:8498, read atomically (cli/sti).
        PUBLIC  _f1D8F_0002              ; @ 1D8F:0002
_f1D8F_0002 PROC FAR
        dw      0                       ; cs:[2]: old INT 1Ch offset
        dw      0                       ; cs:[4]: old INT 1Ch segment
        cli
        mov     ax,ds:[8496h]
        mov     dx,ds:[8498h]
        sti
        retf
_f1D8F_0002 ENDP

; 1D8F:0010: save the current INT 1Ch vector into cs:[2]/cs:[4], then point INT 1Ch at cs:0055.
        PUBLIC  _f1D8F_0010              ; @ 1D8F:0010
_f1D8F_0010 PROC FAR
        push    bp
        mov     bp,sp
        push    ax
        push    bx
        push    ds
        push    es
        mov     ah,35h
        mov     al,1Ch
        int     21h
        mov     word ptr cs:[4],es
        mov     word ptr cs:[2],bx
        mov     ah,25h
        mov     al,1Ch
        push    cs
        pop     ds
        mov     dx,55h
        int     21h
        pop     es
        pop     ds
        pop     bx
        pop     ax
        pop     bp
        retf
_f1D8F_0010 ENDP

; 1D8F:0038: restore the saved INT 1Ch vector from cs:[2]/cs:[4]. 1D8F:0055 (inside this
; function's range, reached through the vector) is the handler: switch to DGROUP, bump the
; counters selected by the flags at ds:849E, and run the countdown at ds:849A:849C; if no old
; vector was saved, iret, else chain to it.
        PUBLIC  _f1D8F_0038              ; @ 1D8F:0038
_f1D8F_0038 PROC FAR
        push    bp
        mov     bp,sp
        push    ax
        push    ds
        push    dx
        push    es
        mov     ah,25h
        mov     al,1Ch
        mov     ds,word ptr cs:[4]
        mov     dx,word ptr cs:[2]
        int     21h
        pop     es
        pop     dx
        pop     ds
        pop     ax
        pop     bp
        retf

        push    ax                      ; 1D8F:0055: the INT 1Ch handler
        push    ds
        mov     ax,212Eh
        mov     ds,ax
        test    word ptr ds:[849Eh],1
        je      L6E
        inc     word ptr ds:[848Eh]
        jne     L6E
        inc     word ptr ds:[8490h]
L6E:    test    word ptr ds:[849Eh],2
        je      L80
        inc     word ptr ds:[8492h]
        jne     L80
        inc     word ptr ds:[8494h]
L80:    inc     word ptr ds:[8488h]
        inc     word ptr ds:[848Ah]
        jne     L8E
        inc     word ptr ds:[848Ch]
L8E:    inc     word ptr ds:[8496h]
        jne     L98
        inc     word ptr ds:[8498h]
L98:    mov     ax,ds:[849Ah]
        or      ax,ds:[849Ch]
        je      LAb
        sub     word ptr ds:[849Ah],1
        sbb     word ptr ds:[849Ch],0
LAb:    pop     ds
        pop     ax
        cmp     word ptr cs:[4],0
        je      LBa
        jmp     dword ptr cs:[2]
LBa:    iret
_f1D8F_0038 ENDP

        END

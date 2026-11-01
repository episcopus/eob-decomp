; Unit at EOB.EXE 1990:0001 (710 bytes): DOS memory control blocks. Starts with 15 bytes of data
; (the segment of the program's MCB at +0E, kept in the code segment), then the functions; only
; the first is written so far. Placeholder names.

        .MODEL  LARGE
        .CODE

        db      13 dup (0)              ; 1990:0001
mcbseg  dw      0                       ; 1990:000E: MCB of our block, set by 1990:0071

; 1990:0010: walk the MCB chain from mcbseg, merging free blocks; AX = 0 if it ends with a
; proper 'Z' block, 1 if the chain is broken
        PUBLIC  CheckMCB                ; @ 1990:0010
CheckMCB PROC NEAR
        push    bx
        push    ds
        push    es
        mov     es,cs:mcbseg
        cmp     byte ptr es:[0],5Ah
        je      ok
again:  mov     bx,es:[1]
        mov     ax,es
        mov     ds,ax
        add     ax,es:[3]
        inc     ax
        mov     es,ax
        cmp     byte ptr es:[0],4Dh
        je      owner
        cmp     byte ptr es:[0],5Ah
        jne     bad
owner:  or      bx,es:[1]
        jne     next
        mov     ax,es:[3]
        inc     ax
        add     ds:[3],ax
        mov     al,es:[0]
        mov     ds:[0],al
        mov     ax,ds
        mov     es,ax
next:   cmp     byte ptr es:[0],5Ah
        jne     again
ok:     mov     ax,0
        jmp     done
bad:    mov     ax,1
done:   pop     es
        pop     ds
        pop     bx
        ret
CheckMCB ENDP

        END

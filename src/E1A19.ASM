; Unit at EOB.EXE 1A19:0006 (102 bytes, 2 functions), kind asm [H] by the map. The same code is in
; INTRO.EXE at 0992:0002 (unit i0992, same_as e1a19) with different DGROUP offsets. Placeholder
; names. Function 1 records the current default drive and the highest drive number DOS accepts;
; function 2 tests one drive number with IOCTL 0Eh (get logical drive map).

        .MODEL  LARGE
        .DATA
        EXTRN   gDefaultDrive:BYTE      ; @ ds:FF34
        EXTRN   gLastDrive:BYTE         ; @ ds:FF00
        .CODE

; 1A19:0006: gDefaultDrive (ds:FF34) = the current default drive (INT 21h/19h); then AL = the
; drive count INT 21h/0Eh returns, and the drive number (0 = A) is walked down until IOCTL 0Eh
; (44h/0Eh, get logical drive map) stops failing with AL = 0Fh (invalid drive); gLastDrive
; (ds:FF00) = that drive number. Called once from 1C0E:13E7.
        PUBLIC  _FindLastDrive          ; @ 1A19:0006
_FindLastDrive PROC FAR
        push    bp
        mov     bp,sp
        push    ax
        push    bx
        push    dx
        mov     ah,19h
        int     21h
        mov     byte ptr [gDefaultDrive],al
        mov     dl,al
        mov     ah,0Eh
        int     21h
        dec     al
        xor     ah,ah
        mov     dx,ax
loop:   mov     bl,dl
        inc     bl
        mov     ax,440Eh
        int     21h
        jae     done
        cmp     al,0Fh
        jne     done
        dec     dx
        jmp     loop
done:   mov     ax,dx
        mov     byte ptr [gLastDrive],al
        pop     dx
        pop     bx
        pop     ax
        pop     bp
        ret
_FindLastDrive ENDP

        PUBLIC  _f1A19_003B            ; @ 1A19:003B stub
_f1A19_003B PROC FAR
        ret
_f1A19_003B ENDP

        END

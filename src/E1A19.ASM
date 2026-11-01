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
scan:   mov     bl,dl
        inc     bl
        mov     ax,440Eh
        int     21h
        jae     found
        cmp     al,0Fh
        jne     found
        dec     dx
        jmp     scan
found:  mov     ax,dx
        mov     byte ptr [gLastDrive],al
        pop     dx
        pop     bx
        pop     ax
        pop     bp
        ret
_FindLastDrive ENDP

; 1A19:003B: 1 if drive DL (0 = A) is the last logical drive on its physical device, 0 otherwise
; (or when the drive is invalid). IOCTL 0Eh (44h/0Eh, get logical drive map) with BL = drive + 1:
; on error (CF set) AL = 1 means the call is unsupported (DOS before 3.0) and AL = 0Fh an invalid
; drive. Called in a loop from 1C0E:1369 with drive numbers from gLastDrive down to 0.
        PUBLIC  _IsLastLogicalDrive     ; @ 1A19:003B
_IsLastLogicalDrive PROC FAR
        push    bp
        mov     bp,sp
        push    bx
        push    dx
        mov     dl,byte ptr [bp+6]
        push    bx
        mov     bl,dl
        inc     bl
        mov     ax,440Eh
        int     21h
        pop     bx
        jae     mapped
        cmp     al,1
        je      yes
        jmp     short no
mapped: cmp     al,0
        je      yes
        dec     al
        cmp     al,dl
        je      yes
no:     mov     ax,0
        jmp     short exit
yes:    mov     ax,1
exit:   pop     dx
        pop     bx
        pop     bp
        ret
_IsLastLogicalDrive ENDP

        END

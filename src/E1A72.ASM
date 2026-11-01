; Unit at EOB.EXE 1A72:0006 (841 bytes, 8 functions), kind asm [H] by the map. Low level support
; code: the game's INT 33h mouse driver glue plus a few interrupt and far pointer helpers. MouseInit
; (1A72:00DA) detects the driver, initialises the mouse variables in DGROUP (8130h..8174h), installs
; the event handler at 1A72:017A and returns 1 when a driver is present (0 otherwise). The handler
; at 1A72:017A, which sits inside MouseInit, clamps the position, hands the button state to the
; keyboard glue 1AAD:0B0E and redraws through the far pointers at DGROUP:80AA and DGROUP:80A2 under
; the semaphore at DGROUP:812E. MouseInRect (1A72:02A6) tests the current position against a
; rectangle; MouseDisable (1A72:0285) removes the handler; 1A72:02FA and 1A72:02FC enable and
; disable interrupts; 1A72:02FE and 1A72:033A set and read the far pointer at DGROUP:8172/8174
; (1A72:02FE also copies 16 bytes from the old target to the new one). 1A72:0006 is the INT 3Fh
; handler main installs (07C8:008F), it chains through DGROUP:FC08 (the previous vector) and
; (re)opens EOB.EXE through 1C0E:1310. Placeholder DGROUP names mark the mouse state whose exact
; role is inferred from this code only.

        .MODEL  LARGE

        EXTRN   _access:FAR             ; @ 0000:505D (runtime ACCESS module)
        EXTRN   _open:FAR               ; @ 0000:6443 (runtime OPEN module, E1A47.C)
        EXTRN   _close:FAR              ; @ 0000:50C5 (runtime, E1A47.C)
        EXTRN   _f1C0E_1310:FAR         ; @ 1C0E:1310 (locates and opens the overlay file)
        EXTRN   _f07C8_058A:FAR         ; @ 07C8:058A (cleanup and exit, E19BC.C)
        EXTRN   _f1AAD_0B0E:FAR         ; @ 1AAD:0B0E (keyboard glue, takes the button state)
        EXTRN   _f1BBE_0002:FAR         ; @ 1BBE:0002 (E1BB1.ASM)
        EXTRN   _f1BBE_00ED:FAR         ; @ 1BBE:00ED (E1BB1.ASM)

        .DATA
        EXTRN   g80A2:DWORD             ; @ ds:80A2: indirect far function, no arguments
        EXTRN   g80AA:DWORD             ; @ ds:80AA: indirect far function, called with ds:8172/8174
        EXTRN   g80EC:BYTE              ; @ ds:80EC: compared with 2 before the position update
        EXTRN   g8104:DWORD             ; @ ds:8104: indirect far function, (far string, 2)
        EXTRN   g8111:DWORD             ; @ ds:8111: indirect far function, (far string, 2)
        EXTRN   g812E:WORD              ; @ ds:812E: semaphore, 0 = free (busy-wait acquire)
        EXTRN   g8130:WORD              ; @ ds:8130: mouse buttons seen by the last event
        EXTRN   g8132:WORD              ; @ ds:8132: button bits or-ed into every event
        EXTRN   g8134:WORD              ; @ ds:8134: accumulated button changes
        EXTRN   g8136:WORD              ; @ ds:8136: set to 1 while a button is held
        EXTRN   g8138:WORD              ; @ ds:8138: working mouse x (set to the middle on init)
        EXTRN   g813A:WORD              ; @ ds:813A: working mouse y (set to the middle on init)
        EXTRN   g813C:WORD              ; @ ds:813C: x of the event just handled
        EXTRN   g813E:WORD              ; @ ds:813E: y of the event just handled
        EXTRN   g8144:WORD              ; @ ds:8144: 1 when the driver reports a 640 wide range
        EXTRN   g8146:WORD              ; @ ds:8146: nonzero suppresses the redraw
        EXTRN   g8148:WORD              ; @ ds:8148: 1 when the mouse driver handler is installed
        EXTRN   g814A:WORD              ; @ ds:814A: 0 disables the handler body (E07A9.C)
        EXTRN   g814E:WORD              ; @ ds:814E: screen width minus 1, 13Fh
        EXTRN   g8152:WORD              ; @ ds:8152: screen height minus 1, 0C7h
        EXTRN   g815C:WORD              ; @ ds:815C: last position handed to the game
        EXTRN   g815E:WORD              ; @ ds:815E: last position handed to the game
        EXTRN   g8160:WORD              ; @ ds:8160: status bits, 4000h and 8000h (see the handler)
        EXTRN   g8162:WORD              ; @ ds:8162: tracking rectangle, left
        EXTRN   g8164:WORD              ; @ ds:8164: tracking rectangle, top
        EXTRN   g8166:WORD              ; @ ds:8166: tracking rectangle, right
        EXTRN   g8168:WORD              ; @ ds:8168: tracking rectangle, bottom
        EXTRN   g8172:WORD              ; @ ds:8172: far pointer argument, offset
        EXTRN   g8174:WORD              ; @ ds:8174: far pointer argument, segment
        EXTRN   g8026:WORD              ; @ ds:8026: scratch copy of the reported x (MouseInRect)
        EXTRN   g8028:WORD              ; @ ds:8028: scratch copy of the reported y (MouseInRect)
        EXTRN   gFC08:DWORD             ; @ ds:FC08: previous INT 3Fh vector, chained by 1A72:0006
        .CODE

; 1A72:0006: the INT 3Fh handler main installs (07C8:008F, `setvect(0x3F, 1A72:0006)`; the old
; vector was saved at DGROUP:FC08 by 07C8:0088) and always leaves through that saved vector, so it
; is a hook around the runtime's overlay trap. It saves everything, points DS at DGROUP and calls
; `access("EOB.EXE", 0)`: when that returns zero (the program file is accessible) it goes straight
; to the chain. Otherwise it closes the file handle kept at 20B5h:00B8h, retries opening EOB.EXE
; through 1C0E:1310 (with the indirect helpers at DGROUP:8111 and DGROUP:8104 in between) and
; reopens it with `open("EOB.EXE", O_RDONLY)`, storing the new handle at 20B5h:00B8h; every
; failing path calls 07C8:058A. Which of the two paths is the normal case is not established.
        PUBLIC  _Int3FHandler           ; @ 1A72:0006
_Int3FHandler PROC FAR
        push    ax
        push    bx
        push    cx
        push    dx
        push    es
        push    ds
        push    si
        push    di
        push    bp
        sti
        mov     bp,212Eh
        mov     ds,bp
        mov     bp,sp
        xor     ax,ax
        push    ax
        push    ds
        mov     ax,308Ah
        push    ax
        call    far ptr _access
        add     sp,6
        or      ax,ax
        jne     L002E
        jmp     L00CD
L002E:  mov     ax,20B5h
        mov     es,ax
        push    word ptr es:[0B8h]
        call    far ptr _close
        inc     sp
        inc     sp
        xor     si,si
L003F:  push    ds
        mov     ax,308Ah
        push    ax
        call    far ptr _f1C0E_1310
        add     sp,4
        or      ax,ax
        je      L007C
        jmp     short L0079
L0054:  push    ds
        mov     ax,308Ah
        push    ax
        mov     ax,2
        push    ax
        call    dword ptr [g8111]
        add     sp,6
        or      ax,ax
        je      L0080
        push    ds
        mov     ax,308Ah
        push    ax
        call    far ptr _f1C0E_1310
        add     sp,4
        or      ax,ax
        je      L007C
L0079:  mov     si,1
L007C:  or      si,si
        je      L0054
L0080:  or      si,si
        jne     short L0099
        push    ds
        mov     ax,308Ah
        push    ax
        mov     ax,2
        push    ax
        call    dword ptr [g8104]
        add     sp,6
        call    far ptr _f07C8_058A
L0099:  mov     ax,1
        push    ax
        push    ds
        mov     ax,308Ah
        push    ax
        call    far ptr _open
        add     sp,6
        mov     bx,20B5h
        mov     es,bx
        mov     word ptr es:[0B8h],ax
        cmp     ax,0FFFFh
        jne     L00CD
        push    ds
        mov     ax,308Ah
        push    ax
        mov     ax,2
        push    ax
        call    dword ptr [g8104]
        add     sp,6
        call    far ptr _f07C8_058A
L00CD:  pop     bp
        pop     di
        pop     si
        pop     ds
        pop     es
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        jmp     dword ptr [gFC08]
_Int3FHandler ENDP

; 1A72:00DA: initialise the mouse driver through INT 33h and install the event handler. It reads
; the INT 33h vector (INT 21h/35h) and treats a null vector or one whose first byte is CFh (an
; IRET, no driver) as absent. It resets the driver (function 0, expects FFFFh), reads the position
; (function 3) and, when the driver reports x = 0A0h, records 640 wide mode (DGROUP:8144 = 1) and
; doubles x before setting the position (function 4). It installs 1A72:017A for moves and both
; buttons (function 0Ch, mask 1Fh) and returns 1 in AX; with no driver it returns 0. Called from
; main at 07C8:0199. The DGROUP defaults are the middle of a 320x200 screen and the screen bounds.
        PUBLIC  _MouseInit              ; @ 1A72:00DA
_MouseInit PROC FAR
        push    bp
        mov     bp,sp
        push    bx
        push    cx
        push    dx
        push    es
        mov     ax,3533h
        int     21h
        mov     word ptr [g8138],0A0h
        mov     word ptr [g813A],64h
        mov     word ptr [g8146],1
        mov     word ptr [g8160],0
        mov     word ptr [g814E],13Fh
        mov     word ptr [g8152],0C7h
        mov     ax,es
        or      ax,bx
        je      L0118
        mov     bl,byte ptr es:[bx]
        cmp     bl,0CFh
        jne     L0123
L0118:  mov     ax,0
        mov     word ptr [g8148],0
        jmp     short L0174
L0123:  mov     ax,0
        int     33h
        cmp     ax,0FFFFh
        jne     L0118
        mov     word ptr [g8144],0
        mov     ax,3
        int     33h
        cmp     cx,0A0h
        je      L0144
        mov     word ptr [g8144],1
L0144:  mov     cx,0A0h
        mov     dx,64h
        mov     word ptr [g8148],1
        mov     word ptr [g814A],1
        cmp     word ptr [g8144],1
        jne     L015F
        shl     cx,1
L015F:  mov     ax,4
        int     33h
        mov     ax,0Ch
        mov     cx,1Fh
        push    cs
        pop     es
        mov     dx,offset MouseEventHandler
        int     33h
        mov     ax,1
L0174:  pop     es
        pop     dx
        pop     cx
        pop     bx
        pop     bp
        ret

; 1A72:017A: the INT 33h event handler installed above (entry point inside _MouseInit, not a
; separate function). CX = x, DX = y, BX = buttons. With DGROUP:814A zero it does nothing. In 640
; wide mode it halves x, clamps x to 13Fh and keeps the event position at DGROUP:813C/813E. It
; or-s in the mask at DGROUP:8132, and when the buttons changed calls the keyboard glue 1AAD:0B0E
; and folds the change into DGROUP:8134. With the semaphore free it publishes the position to
; DGROUP:8138/813A (unless DGROUP:80EC is 2), and when the position moved and redraws are not
; suppressed (DGROUP:8146 = 0) either redraws the tracking rectangle (DGROUP:8160 bit 8000h set
; and the position inside DGROUP:8162..8168, which or-s 4000h instead) or asks the far pointers at
; DGROUP:80AA (with DGROUP:8172/8174) and DGROUP:80A2 to redraw. It chains to nothing: the vector
; belongs to the game.
MouseEventHandler:
        push    ds
        push    ax
        push    cx
        push    dx
        pushf
        cld
        mov     ax,212Eh
        mov     ds,ax
        cmp     word ptr [g814A],0
        jne     L01E9
        cmp     word ptr [g8144],1
        jne     L0195
        shr     cx,1
L0195:  cmp     cx,13Fh
        jbe     L019E
        mov     cx,13Fh
L019E:  mov     word ptr [g813C],cx
        mov     word ptr [g813E],dx
        mov     ax,[g8132]
        or      ax,ax
        je      L01B8
        test    bx,4
        je      L01B8
        and     bx,3
        or      bx,ax
L01B8:  cmp     word ptr [g8130],bx
        je      L01D7
        push    bx
        call    far ptr _f1AAD_0B0E
        add     sp,2
        mov     ax,[g8130]
        xor     ax,bx
        or      ax,[g8134]
        mov     [g8134],ax
        mov     [g8130],bx
L01D7:  cmp     bx,0
        je      L01E2
        mov     word ptr [g8136],1
L01E2:  cmp     word ptr [g812E],0
        je      L020C
L01E9:  jmp     L027F
L01EC:  push    word ptr [g8174]
        push    word ptr [g8172]
        call    dword ptr [g80AA]
        add     sp,4
        call    far ptr _f1BBE_0002
        call    dword ptr [g80A2]
        or      word ptr [g8160],4000h
        jmp     short L0279
L020C:  mov     word ptr [g812E],1
        cmp     byte ptr [g80EC],2
        je      L0221
        mov     word ptr [g8138],cx
        mov     word ptr [g813A],dx
L0221:  mov     cx,[g8138]
        cmp     [g815C],cx
        jne     L0235
        mov     dx,[g813A]
        cmp     [g815E],dx
        je      L0279
L0235:  cmp     word ptr [g8146],0
        jne     L0279
        test    word ptr [g8160],8000h
        je      L025C
        cmp     cx,[g8162]
        jl      L0250
        cmp     cx,[g8166]
        jg      L025C
L0250:  cmp     dx,[g8164]
        jl      L025C
        cmp     dx,[g8168]
        jle     L01EC
L025C:  push    word ptr [g8174]
        push    word ptr [g8172]
        call    dword ptr [g80AA]
        add     sp,4
        call    far ptr _f1BBE_0002
        call    far ptr _f1BBE_00ED
        call    dword ptr [g80A2]
L0279:  mov     word ptr [g812E],0
L027F:  popf
        pop     dx
        pop     cx
        pop     ax
        pop     ds
        ret
_MouseInit ENDP

; 1A72:0285: remove the mouse event handler, but only when one was installed (DGROUP:8148 = 1).
; INT 33h function 0Ch with mask 0 clears the handler; ES:DX is set to CS:1A72:017A as INT 33h
; expects even for a null mask. Called from the exit path at 07C8:058A.
        PUBLIC  _MouseDisable           ; @ 1A72:0285
_MouseDisable PROC FAR
        push    bp
        mov     bp,sp
        cmp     word ptr [g8148],0
        je      L02A4
        push    ax
        push    cx
        push    dx
        push    es
        mov     ax,0Ch
        mov     cx,0
        push    cs
        pop     es
        mov     dx,offset MouseEventHandler
        int     33h
        pop     es
        pop     dx
        pop     cx
        pop     ax
L02A4:  pop     bp
        ret
_MouseDisable ENDP

; 1A72:02A6: 1 when the last position handed to the game (DGROUP:815C/815E, copied through
; DGROUP:8026/8028 under the semaphore) lies inside the rectangle [bp+6, bp+0Ah] x [bp+8, bp+0Ch]
; inclusive, 0 otherwise. Arguments are x1, y1, x2, y2 words; called from UI code (122E:0847,
; overlay 0 at 207A, overlay 1 at 1551).
        PUBLIC  _MouseInRect            ; @ 1A72:02A6
_MouseInRect PROC FAR
        push    bp
        mov     bp,sp
        push    cx
        push    di
        push    dx
        push    si
        mov     cx,[bp+6]
        mov     si,[bp+0Ah]
        mov     dx,[bp+8]
        mov     di,[bp+0Ch]
L02B9:  cmp     word ptr [g812E],0
        jne     L02B9
        inc     word ptr [g812E]
        mov     ax,[g815C]
        mov     [g8026],ax
        mov     ax,[g815E]
        mov     [g8028],ax
        cmp     [g8026],cx
        jl      L02ED
        cmp     [g8026],si
        jg      L02ED
        cmp     [g8028],dx
        jl      L02ED
        cmp     [g8028],di
        jg      L02ED
        mov     ax,1
        jmp     short L02F0
L02ED:  mov     ax,0
L02F0:  dec     word ptr [g812E]
        pop     si
        pop     dx
        pop     di
        pop     cx
        pop     bp
        ret
_MouseInRect ENDP

; 1A72:02FA: enable interrupts and return (the `sti` wrapper used around the retrace calibration
; at 1A65:000E).
        PUBLIC  _EnableInterrupts       ; @ 1A72:02FA
_EnableInterrupts PROC FAR
        sti
        ret
_EnableInterrupts ENDP

; 1A72:02FC: disable interrupts and return (the `cli` wrapper used around the retrace calibration
; at 1A65:000E).
        PUBLIC  _DisableInterrupts      ; @ 1A72:02FC
_DisableInterrupts PROC FAR
        cli
        ret
_DisableInterrupts ENDP

; 1A72:02FE: point the far pointer at DGROUP:8172/8174 at the offset in [bp+6] and the segment in
; [bp+8]. When the old segment was nonzero it copies 16 bytes from the old target to the new one,
; with interrupts disabled around the whole sequence. Called from the 17A6 unit at 0459 with
; CS:0304 and at 050D with the buffer whose pointer 17A6:0446 saved through 033A.
        PUBLIC  _SetFarPointer          ; @ 1A72:02FE
_SetFarPointer PROC FAR
        push    bp
        mov     bp,sp
        push    ax
        push    bx
        push    cx
        push    ds
        push    si
        push    es
        push    di
        mov     ax,212Eh
        mov     ds,ax
        mov     ax,[bp+6]
        mov     bx,[bp+8]
        cli
        mov     si,[g8172]
        mov     cx,[g8174]
        mov     [g8172],ax
        mov     [g8174],bx
        jcxz    L0330
        mov     ds,cx
        mov     di,ax
        mov     es,bx
        mov     cx,10h
        rep     movsb
L0330:  sti
        pop     di
        pop     es
        pop     si
        pop     ds
        pop     cx
        pop     bx
        pop     ax
        pop     bp
        ret
_SetFarPointer ENDP

; 1A72:033A: return the far pointer at DGROUP:8172/8174 in DX:AX, with interrupts disabled only
; around the two loads. Called from the 17A6 unit at 0446, which saves the result and passes
; CS:0304 to 02FE.
        PUBLIC  _GetFarPointer          ; @ 1A72:033A
_GetFarPointer PROC FAR
        push    bp
        mov     bp,sp
        push    ds
        mov     ax,212Eh
        mov     ds,ax
        cli
        mov     ax,[g8172]
        mov     dx,[g8174]
        sti
        pop     ds
        pop     bp
        ret
_GetFarPointer ENDP

        END

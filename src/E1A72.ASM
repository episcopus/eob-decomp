; Unit at EOB.EXE 1A72:0006 (841 bytes, 8 functions), kind asm [H] by the map. Low level support
; code: the game's INT 33h mouse driver glue plus a few interrupt and far pointer helpers. InitMouse
; (1A72:00DA) detects the driver, initialises the mouse state the game reads (DGROUP:8138, 813A,
; 8144, 8146, 8148, 814A, 814E, 8152 and 8160), installs
; the event handler at 1A72:017A and returns 1 when a driver is present (0 otherwise). The handler
; at 1A72:017A, which sits inside InitMouse, clamps the position, hands the button state to the
; keyboard glue 1AAD:0B0E and redraws through the far pointers at DGROUP:80AA and DGROUP:80A2 under
; the busy flag at DGROUP:812E. IsMouseInRect (1A72:02A6) tests the position the pointer was last
; drawn at against a rectangle; RemoveMouseHandler (1A72:0285) removes the handler; 1A72:02FA and
; 1A72:02FC enable and disable interrupts; 1A72:02FE and 1A72:033A set and read the far pointer at
; DGROUP:8172/8174 (1A72:02FE also copies 16 bytes from the old target to the new one). 1A72:0006
; is the INT 3Fh handler main installs (07C8:008F), it chains through DGROUP:FC08 (the previous
; EOB.EXE through 1C0E:1310 to find the drive it is on and reopens it with the runtime's _open.
; Placeholder DGROUP names mark the mouse state whose exact role is inferred from this code only.

        .MODEL  LARGE

        EXTRN   _access:FAR             ; @ 0000:505D (runtime ACCESS module)
        EXTRN   _open:FAR               ; @ 0000:6443 (runtime OPEN module, declared in E1A47.C)
        EXTRN   _close:FAR              ; @ 0000:50C5 (runtime, declared in E1A47.C)
        EXTRN   _LocateOverlayFile:FAR         ; @ 1C0E:1310 (1C0E.C: probes `name` by opening it,
        ;                                              closes at once and returns drive plus one, or 0;
        ;                                              it leaves no handle for the caller)
        EXTRN   _ShutdownGame:FAR         ; @ 07C8:058A (cleanup and exit, E07C8.C)
        EXTRN   _HandleMouseButtons:FAR         ; @ 1AAD:0B0E (keyboard glue, takes the button state)
        EXTRN   _HidePointer:FAR         ; @ 1BBE:0002 (E1BBE.ASM)
        EXTRN   _ShowPointer:FAR         ; @ 1BBE:00ED (E1BBE.ASM)

        .DATA
        EXTRN   g80A2:DWORD             ; @ ds:80A2: indirect far function, no arguments
        EXTRN   g80AA:DWORD             ; @ ds:80AA: indirect far function, called with ds:8172/8174
        EXTRN   g80EC:BYTE              ; @ ds:80EC: compared with 2 before the position update
        EXTRN   g8104:DWORD             ; @ ds:8104: the fatal file error handler (E1C0E.C:66),
        ;                                              called as (2, name) on the two give-up exits
        EXTRN   g8111:DWORD             ; @ ds:8111: the retry file error handler (E1C0E.C:67),
        ;                                              called as (2, name) after a failed probe
        EXTRN   _gPointerBusy:WORD              ; @ ds:812E: busy flag, 0 = free; the game takes it with
        ;                                              a test then a store, so it is not a lock
        EXTRN   g8130:WORD              ; @ ds:8130: mouse buttons seen by the last event
        EXTRN   g8132:WORD              ; @ ds:8132: a mask the handler or-s into the low two button
        ;                                              bits when it is nonzero and the middle bit (BX
        ;                                              bit 2, value 4) is set: BX becomes (BX and 3)
        ;                                              or the mask, so the old low two bits are kept and
        ;                                              the middle bit and every other bit are dropped
        EXTRN   g8134:WORD              ; @ ds:8134: accumulated button changes
        EXTRN   g8136:WORD              ; @ ds:8136: latched to 1 once any button has been seen;
        ;                                              this unit never clears it, E1AAD.ASM:1356
        ;                                              sets it too and nothing in src/ clears it
        EXTRN   _gCursorX:WORD              ; @ ds:8138: mouse x in pixels (set to the middle on
        ;                                              init); the handler rewrites it only when the
        ;                                              busy flag is free and DGROUP:80EC is not 2
        EXTRN   _gCursorY:WORD              ; @ ds:813A: mouse y in pixels (set to the middle on
        ;                                              init); the handler rewrites it under the same
        ;                                              conditions as x
        EXTRN   g813C:WORD              ; @ ds:813C: x of the event just handled
        EXTRN   g813E:WORD              ; @ ds:813E: y of the event just handled
        EXTRN   g8144:WORD              ; @ ds:8144: 1 when the x that function 3 reads after the
        ;                                              reset is not 0A0h, which InitMouse reads as a
        ;                                              640 wide mode [H]
        EXTRN   _gHideDepth:WORD              ; @ ds:8146: nonzero suppresses the pointer redraw;
        ;                                              InitMouse sets it to 1, so the pointer starts
        ;                                              hidden until ShowPointer runs
        EXTRN   gMouseHandlerInstalled:WORD              ; @ ds:8148: records that the mouse driver
        ;                                              handler was installed (1 at install, 0 when no
        ;                                              driver is found); RemoveMouseHandler clears
        ;                                              nothing, so it still reads 1 after removal
        EXTRN   gMouseDisabled:WORD              ; @ ds:814A: nonzero disables the mouse: the handler
        ;                                              returns at once and the pointer hide/show do
        ;                                              nothing. InitMouse sets it to 1 before it
        ;                                              installs the handler (1A72:0150); Main then
        ;                                              forces it to 1 when no handler is installed and
        ;                                              otherwise sets it to !mouse (E07C8.C:153-156);
        ;                                              the overlay loader clears it when a handler is
        ;                                              installed (E1C00.ASM:298); E07A9.C tests it
        ;                                              too
        EXTRN   g814E:WORD              ; @ ds:814E: screen width minus 1, 13Fh; the handler clamps
        ;                                              with the literal 13Fh, not with this variable
        EXTRN   g8152:WORD              ; @ ds:8152: screen height minus 1, 0C7h (set at
        ;                                              1A72:0104); this unit never reads it: the
        ;                                              handler clamps no y at all
        EXTRN   _gDrawnX:WORD              ; @ ds:815C: the position the pointer was last drawn at,
        ;                                              copied there by ShowPointer (E1BBE.ASM:214-215)
        EXTRN   _gDrawnY:WORD              ; @ ds:815E: the position the pointer was last drawn at,
        ;                                              copied there by ShowPointer (E1BBE.ASM:219-220)
        EXTRN   _gPointerState:WORD              ; @ ds:8160: pointer state bits: 8000h the pointer is
        ;                                              hidden over the hide zone, 4000h a redraw is
        ;                                              requested, low byte a counter (E1BBE.ASM)
        EXTRN   _gZoneLeft:WORD              ; @ ds:8162: hide zone, left in pixels, stored reduced
        ;                                              (the argument less the pointer width, E1BBE.ASM:341,
        ;                                              store at :329-330);
        ;                                              1A72:0244 compares it, but the QUIRK there
        ;                                              skips the check
        EXTRN   _gZoneTop:WORD              ; @ ds:8164: hide zone, top in pixels, stored reduced
        ;                                              (the argument less the pointer height, E1BBE.ASM:348,
        ;                                              store at :329-330)
        EXTRN   _gZoneRight:WORD              ; @ ds:8166: hide zone, right in pixels
        EXTRN   _gZoneBottom:WORD              ; @ ds:8168: hide zone, bottom in pixels
        EXTRN   g8172:WORD              ; @ ds:8172: far pointer argument, offset
        EXTRN   g8174:WORD              ; @ ds:8174: far pointer argument, segment
        EXTRN   g8026:WORD              ; @ ds:8026: scratch copy of gDrawnX made by IsMouseInRect
        EXTRN   g8028:WORD              ; @ ds:8028: scratch copy of gDrawnY made by IsMouseInRect
        EXTRN   gFC08:DWORD             ; @ ds:FC08: previous INT 3Fh vector, chained by 1A72:0006
        .CODE

; 1A72:0006: the INT 3Fh handler main installs (07C8:008F, `setvect(0x3F, 1A72:0006)`; the old
; vector was saved at DGROUP:FC08 by 07C8:0088) and always leaves through that saved vector, so it
; is a hook around the runtime's overlay trap [H] (nothing in this file proves the trap is the
; runtime's; what the file shows is that it chains to the saved vector and re-finds EOB.EXE when it
; fails). It saves everything, points DS at DGROUP and calls `access("EOB.EXE", 0)`: when that
; returns zero (the program file is accessible) it goes straight to the chain. Otherwise it closes
; the file handle kept at 20B5h:00B8h, probes EOB.EXE through 1C0E:1310 (which opens it only to
; find its drive and closes it at once) and reopens it with `open("EOB.EXE", O_RDONLY)`, storing the
; new handle at 20B5h:00B8h. After each failed probe it asks the retry file error handler at
; DGROUP:8111 (E1C0E.C:67), whose 0 return gives up the loop; on the two give-up exits (1A72:0080
; and 1A72:00B8) it reports to the fatal file error handler at DGROUP:8104 (E1C0E.C:66) with code 2
; and calls 07C8:058A. Which of the two paths is the normal case is not established.
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
        call    far ptr _LocateOverlayFile
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
        call    far ptr _LocateOverlayFile
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
        call    far ptr _ShutdownGame
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
        call    far ptr _ShutdownGame
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
; IRET, no driver) as absent. It resets the driver (function 0, expects FFFFh) and reads the
; position (function 3): when that x is NOT 0A0h it takes the driver to be in 640 wide mode [H], so
; it records that (DGROUP:8144 = 1) and doubles the constant centre 0A0h before setting
; the position (function 4). It installs 1A72:017A for moves and both buttons (function 0Ch, mask
; 1Fh) and returns 1 in AX; with no driver it returns 0. Called from StartupGame (07C8:0199,
; E07C8.C:196). The DGROUP defaults are the middle of a 320x200 screen and the screen bounds.
        PUBLIC  _InitMouse              ; @ 1A72:00DA
_InitMouse PROC FAR
        push    bp
        mov     bp,sp
        push    bx
        push    cx
        push    dx
        push    es
        mov     ax,3533h
        int     21h
        mov     word ptr [_gCursorX],0A0h
        mov     word ptr [_gCursorY],64h
        mov     word ptr [_gHideDepth],1
        mov     word ptr [_gPointerState],0
        mov     word ptr [g814E],13Fh
        mov     word ptr [g8152],0C7h
        mov     ax,es
        or      ax,bx
        je      L0118
        mov     bl,byte ptr es:[bx]
        cmp     bl,0CFh
        jne     L0123
L0118:  mov     ax,0
        mov     word ptr [gMouseHandlerInstalled],0
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
        mov     word ptr [gMouseHandlerInstalled],1
        mov     word ptr [gMouseDisabled],1
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

; 1A72:017A: the INT 33h event handler installed above (entry point inside _InitMouse, not a
; separate function). CX = x, DX = y, BX = buttons. With DGROUP:814A nonzero it does nothing (a
; nonzero word disables the handler). In 640 wide mode it halves x, clamps x to 13Fh and keeps the
; event position at DGROUP:813C/813E. When the mask at DGROUP:8132 is nonzero and the middle button
; bit (BX bit 2, value 4) is set, it sets BX to (BX and 3) or the mask (`and bx,3; or bx,ax`,
; 1A72:01A6): the old low two bits are kept and the middle bit and every other bit are dropped. The
; installed mask 1Fh (INT 33h function 0Ch) requests moves and left and right presses and releases
; [H]; BX carries the full button state on each of those events, so the middle bit (4) reaches the
; handler whenever the middle button is held, though a middle press or release alone never calls it. When the buttons
; changed it calls the keyboard glue 1AAD:0B0E and folds
; the change into DGROUP:8134. With the busy flag free it publishes the position to
; DGROUP:8138/813A (unless DGROUP:80EC is 2), and when the position moved and redraws are not
; suppressed (DGROUP:8146 = 0) it redraws through the far pointers: with gPointerState bit 8000h
; (hidden over the hide zone) set and the position in the zone it calls DGROUP:80AA (with
; DGROUP:8172/8174), HidePointer (1BBE:0002) and DGROUP:80A2, then sets 4000h (1A72:01EC);
; otherwise it calls DGROUP:80AA, HidePointer, ShowPointer (1BBE:00ED) and DGROUP:80A2 (1A72:025C).
; With the busy flag set the event publishes no position and draws nothing, and nothing is kept for
; a later event; the button change above it has already gone through.
; QUIRK 1A72:0248: the `x < gZoneLeft` test jumps into the y test, so the hide zone's left edge is
; never enforced (only x > gZoneRight and y outside [top, bottom] leave the zone). The handler calls
; no earlier mouse handler (none is saved in this unit).
MouseEventHandler:
        push    ds
        push    ax
        push    cx
        push    dx
        pushf
        cld
        mov     ax,212Eh
        mov     ds,ax
        cmp     word ptr [gMouseDisabled],0
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
        call    far ptr _HandleMouseButtons
        add     sp,2
        mov     ax,[g8130]
        xor     ax,bx
        or      ax,[g8134]
        mov     [g8134],ax
        mov     [g8130],bx
L01D7:  cmp     bx,0
        je      L01E2
        mov     word ptr [g8136],1
L01E2:  cmp     word ptr [_gPointerBusy],0
        je      L020C
L01E9:  jmp     L027F
L01EC:  push    word ptr [g8174]
        push    word ptr [g8172]
        call    dword ptr [g80AA]
        add     sp,4
        call    far ptr _HidePointer
        call    dword ptr [g80A2]
        or      word ptr [_gPointerState],4000h
        jmp     short L0279
L020C:  mov     word ptr [_gPointerBusy],1
        cmp     byte ptr [g80EC],2
        je      L0221
        mov     word ptr [_gCursorX],cx
        mov     word ptr [_gCursorY],dx
L0221:  mov     cx,[_gCursorX]
        cmp     [_gDrawnX],cx
        jne     L0235
        mov     dx,[_gCursorY]
        cmp     [_gDrawnY],dx
        je      L0279
L0235:  cmp     word ptr [_gHideDepth],0
        jne     L0279
        test    word ptr [_gPointerState],8000h
        je      L025C
        cmp     cx,[_gZoneLeft]
        jl      L0250                   ; QUIRK 1A72:0248: jumping to the y test skips the left
                                        ; edge check, so the zone has no left bound
        cmp     cx,[_gZoneRight]
        jg      L025C
L0250:  cmp     dx,[_gZoneTop]
        jl      L025C
        cmp     dx,[_gZoneBottom]
        jle     L01EC
L025C:  push    word ptr [g8174]
        push    word ptr [g8172]
        call    dword ptr [g80AA]
        add     sp,4
        call    far ptr _HidePointer
        call    far ptr _ShowPointer
        call    dword ptr [g80A2]
L0279:  mov     word ptr [_gPointerBusy],0
L027F:  popf
        pop     dx
        pop     cx
        pop     ax
        pop     ds
        ret
_InitMouse ENDP

; 1A72:0285: remove the mouse event handler, but only when one was installed (DGROUP:8148 = 1).
; INT 33h function 0Ch with mask 0 clears the handler; ES:DX = CS:1A72:017A with mask 0 (INT 33h
; wants ES:DX set even for a null mask [H]). Called from the exit path at 07C8:058A.
        PUBLIC  _RemoveMouseHandler           ; @ 1A72:0285
_RemoveMouseHandler PROC FAR
        push    bp
        mov     bp,sp
        cmp     word ptr [gMouseHandlerInstalled],0
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
_RemoveMouseHandler ENDP

; 1A72:02A6: 1 when the position the pointer was last drawn at (DGROUP:815C/815E, the copy ShowPointer
; makes of the mouse position at E1BBE.ASM:214-215, :219-220, so it is where the pointer sits after a show)
; lies inside the rectangle [bp+6, bp+0Ah] x [bp+8, bp+0Ch]
; inclusive, 0 otherwise. It spins at 1A72:02B9 while the busy flag at DGROUP:812E is set, takes the
; flag, copies the drawn position through DGROUP:8026/8028, tests it and releases the flag.
; Arguments are x1, y1, x2, y2 words; called from UI code (122E:0847,
; overlay 0 at 207A, overlay 1 at 1551).
        PUBLIC  _IsMouseInRect            ; @ 1A72:02A6
_IsMouseInRect PROC FAR
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
L02B9:  cmp     word ptr [_gPointerBusy],0
        jne     L02B9
        inc     word ptr [_gPointerBusy]
        mov     ax,[_gDrawnX]
        mov     [g8026],ax
        mov     ax,[_gDrawnY]
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
L02F0:  dec     word ptr [_gPointerBusy]
        pop     si
        pop     dx
        pop     di
        pop     cx
        pop     bp
        ret
_IsMouseInRect ENDP

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

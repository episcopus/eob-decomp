; Unit at EOB.EXE 1BBE:0001 (860 bytes, 7 functions): the game's own mouse pointer, its hide and
; show halves and the wrappers that call them. The pointer is drawn by [ds:80C2] (gDraw80C2 in
; src/E1A29.C) from the shape record _gPointerShapeOff/_gPointerShapeSeg, positioned at the mouse
; position gCursorX/gCursorY less the hot spot _gPointerHotX/_gPointerHotY; before it is drawn at
; a new place the screen under it is saved through [ds:80AE] (gSaveRect) into the buffer
; _gPointerBackOff/_gPointerBackSeg, and to erase it the saved
; background is written back through [ds:804E] (gRestoreRect). 1BBE:0002 HidePointer is the hide
; half: restore the background, then gHideDepth++. 1BBE:00ED ShowPointer is the show half:
; gHideDepth is decremented and, only at zero, move the drawn position to the mouse position, save
; and draw.
; 1BBE:01FE HidePointerZone and 1BBE:02C2 ShowPointerZone are the hide/show pair that carry the
; rectangle the pointer was hidden over (gZoneLeft..gZoneBottom, read by the mouse handler
; 1A72:0244) and the gPointerState redraw flag; 1BBE:0307 LockHidePointer and 1BBE:0332
; LockShowPointer hide and show without that state. Every wrapper takes the pointer semaphore
; _gPointerBusy and brackets its work with [ds:80AA] and [ds:80A2]. The unit is hand-written assembly (AX
; is saved with push/pop ax, which the compiler's C functions never do; the far call through a
; DGROUP pointer and the busy-wait on _gPointerBusy are the style of E1BB1 and E1BF3).
;
; Globals src/ already names keep those names: _gPointerBusy, g814A, gPointerHeight, gPointerWidth8,
; gPointerHotX, gPointerHotY, gPointerBackOff, gPointerBackSeg, gPointerShapeOff, gPointerShapeSeg,
; g8172, g8174, g80AA, g80A2 and g80C6 from src/E1BB1.ASM (and g814A from
; src/E07A9.C, gDraw80C2 from src/E1A29.C). The byte at 1BBE:0001 is the unit's segment start, as
; in 1BB1:0001 and 1BF3:000D.

        .MODEL  LARGE

        .DATA
        EXTRN   _gPointerBusy:WORD              ; @ ds:812E: pointer semaphore, 0 = free (busy-wait acquire)
        EXTRN   _gCursorX:WORD          ; @ ds:8138: mouse position, x in pixels (1BF3/1A72 own it)
        EXTRN   _gCursorY:WORD          ; @ ds:813A: mouse position, y in pixels
        EXTRN   _gHideDepth:WORD        ; @ ds:8146: hide depth, 0 = drawn (1BBE:0002/00ED change it)
        EXTRN   g814A:WORD              ; @ ds:814A: non-zero disables the pointer (E07A9 tests it too)
        EXTRN   _gPointerHeight:WORD     ; @ ds:8154: sprite height in pixels (record byte +3, 1BB1)
        EXTRN   _gPointerWidth8:WORD     ; @ ds:8156: sprite width in 8-pixel columns (byte +2 + 1)
        EXTRN   _gPointerHotX:WORD       ; @ ds:8158: hot spot, x in pixels (subtracted from gCursorX)
        EXTRN   _gPointerHotY:WORD       ; @ ds:815A: hot spot, y in pixels
        EXTRN   _gDrawnX:WORD           ; @ ds:815C: position drawn at last, x in pixels
        EXTRN   _gDrawnY:WORD           ; @ ds:815E: position drawn at last, y in pixels
        EXTRN   _gPointerState:WORD     ; @ ds:8160: bit 8000 hidden, bit 4000 redraw, low byte a count
        EXTRN   _gZoneLeft:WORD         ; @ ds:8162: hide zone, left in pixels (1A72:0244 tests it)
        EXTRN   _gZoneTop:WORD          ; @ ds:8164: hide zone, top in pixels
        EXTRN   _gZoneRight:WORD        ; @ ds:8166: hide zone, right in pixels
        EXTRN   _gZoneBottom:WORD       ; @ ds:8168: hide zone, bottom in pixels
        EXTRN   _gPointerBackOff:WORD    ; @ ds:816A: save-under buffer, far offset (1BB1 allocates)
        EXTRN   _gPointerBackSeg:WORD    ; @ ds:816C: save-under buffer, far segment, 0 = none
        EXTRN   _gPointerShapeOff:WORD   ; @ ds:816E: pointer shape record, far offset (1BB1 stores it)
        EXTRN   _gPointerShapeSeg:WORD   ; @ ds:8170: pointer shape record, far segment
        EXTRN   g8172:WORD              ; @ ds:8172: far pointer handed to [ds:80AA], offset
        EXTRN   g8174:WORD              ; @ ds:8174: far pointer handed to [ds:80AA], segment
        EXTRN   _gRectLeft:WORD         ; @ ds:8176: clipped rectangle, left in 8-pixel columns
        EXTRN   _gRectTop:WORD          ; @ ds:8178: clipped rectangle, top in pixel rows
        EXTRN   _gRectRight:WORD        ; @ ds:817A: clipped rectangle, right in 8-pixel columns
        EXTRN   _gRectBottom:WORD       ; @ ds:817C: clipped rectangle, bottom in pixel rows
        EXTRN   _gShapeX:WORD           ; @ ds:817E: shape drawn at, x in pixels, unclipped
        EXTRN   _gShapeY:WORD           ; @ ds:8180: shape drawn at, y in pixels, unclipped
        EXTRN   _gSpriteHeight:WORD     ; @ ds:8182: copy of _gPointerHeight, the upper y clip bound
        EXTRN   _gNegSpriteHeight:WORD  ; @ ds:8184: minus _gPointerHeight, the lower y clip bound
        EXTRN   _gSpriteWidth:WORD      ; @ ds:8186: copy of _gPointerWidth8, the upper x clip bound
        EXTRN   _gNegSpriteWidth:WORD   ; @ ds:8188: minus _gPointerWidth8, the lower x clip bound
        EXTRN   _gRestoreRect:DWORD     ; @ ds:804E: far fn (x, y, w, h, buf): buffer back to screen
        EXTRN   _gSaveRect:DWORD        ; @ ds:80AE: far fn (x, y, w, h, buf): screen into buffer
        EXTRN   g80AA:DWORD             ; @ ds:80AA: far fn (g8172/g8174) before the pointer work
        EXTRN   g80A2:DWORD             ; @ ds:80A2: far fn (no arguments) after the pointer work
        EXTRN   gDraw80C2:DWORD         ; @ ds:80C2: far fn (mode, void far *src, x, y)
        EXTRN   g80C6:DWORD             ; @ ds:80C6: far fn (w, h): the sprite size query (1BB1:0094)
        .CODE

; 1BBE:0001: Purpose unknown: the unit's first byte is 00. It is emitted under a public symbol
; so the registry records it and the unit counts complete, as 1BB1:0001, 1BA8:0005 and 1BF3:000D
; do; it is the unit's segment start, not code and not called.
        PUBLIC  _f1BBE_0001
_f1BBE_0001 LABEL BYTE                    ; @ 1BBE:0001
        db      0

; 1BBE:0002 HidePointer: hide the pointer. Nothing happens when the pointer is disabled (g814A),
; when it is already hidden (gHideDepth non-zero) or when no save-under buffer exists (_gPointerBackSeg zero).
; Otherwise the rectangle the pointer covers is recomputed from the position it was last drawn at
; (gDrawnX/gDrawnY) less the hot spot (_gPointerHotX/_gPointerHotY) and the sprite size (_gPointerHeight/_gPointerWidth8; the x in
; 8-pixel columns, the y in rows, each clipped to the 40x200 screen), the saved background is
; written back to the screen through [ds:804E] (x, y, w, h, _gPointerBackOff/_gPointerBackSeg) and gHideDepth is
; incremented. The height and width used are the ones 1BB1 stored before it calls here, which is
; what keeps the buffer valid while the shape is replaced. Called by 1BB1:0002, 1BF3:000E and
; 1A72, and by the wrappers below. Pushes AX only to preserve the caller's.
        PUBLIC  _HidePointer            ; @ 1BBE:0002
_HidePointer PROC FAR
        push    bp
        mov     bp,sp
        push    ax
        cmp     word ptr [g814A],0
        je      enabled
disabled: jmp     HideDone
enabled: cmp     word ptr [_gHideDepth],0
        jne     L0075
        cmp     word ptr [_gPointerBackSeg],0
        je      disabled
        mov     ax,[_gDrawnX]
        sub     ax,[_gPointerHotX]
        mov     [_gRectLeft],ax
        mov     ax,[_gDrawnY]
        sub     ax,[_gPointerHotY]
        mov     [_gRectTop],ax
        mov     ax,[_gPointerHeight]
        mov     [_gSpriteHeight],ax
        neg     ax
        mov     [_gNegSpriteHeight],ax
        mov     ax,[_gPointerWidth8]
        mov     [_gSpriteWidth],ax
        neg     ax
        mov     [_gNegSpriteWidth],ax
        mov     ax,[_gRectLeft]
        sar     ax,1
        sar     ax,1
        sar     ax,1
        cmp     ax,[_gNegSpriteWidth]
        jle     L0075
        cmp     ax,28h
        jge     L0075
        mov     [_gRectLeft],ax
        add     ax,[_gSpriteWidth]
        mov     [_gRectRight],ax
        cmp     word ptr [_gRectLeft],0
        jge     L0077
        mov     word ptr [_gRectLeft],0
        jmp     short L0084
L0075:  jmp     short L00E0
L0077:  cmp     word ptr [_gRectRight],28h
        jle     L0084
        mov     word ptr [_gRectRight],28h
L0084:  mov     ax,[_gRectTop]
        cmp     ax,[_gNegSpriteHeight]
        jle     L00E0
        cmp     ax,0C8h
        jge     L00E0
        mov     [_gRectTop],ax
        add     ax,[_gSpriteHeight]
        mov     [_gRectBottom],ax
        cmp     word ptr [_gRectTop],0
        jge     L00AB
        mov     word ptr [_gRectTop],0
        jmp     short L00B9
L00AB:  cmp     word ptr [_gRectBottom],0C8h
        jle     L00B9
        mov     word ptr [_gRectBottom],0C8h
L00B9:  push    word ptr [_gPointerBackSeg]
        push    word ptr [_gPointerBackOff]
        mov     ax,[_gRectBottom]
        sub     ax,[_gRectTop]
        push    ax
        mov     ax,[_gRectRight]
        sub     ax,[_gRectLeft]
        push    ax
        mov     ax,[_gRectTop]
        push    ax
        push    word ptr [_gRectLeft]
        call    dword ptr [_gRestoreRect]
        add     sp,0Ch
L00E0:  inc     word ptr [_gHideDepth]
        jne     HideDone
        dec     word ptr [_gHideDepth]
HideDone: pop    ax
        pop     bp
        ret
_HidePointer ENDP

; 1BBE:00ED ShowPointer: show the pointer. Nothing happens when it is disabled (g814A), when it is
; not hidden (gHideDepth zero) or when no save-under buffer exists (_gPointerBackSeg). Otherwise gHideDepth
; is decremented and, only at zero, the current mouse position (gCursorX/gCursorY) is copied to
; the position being drawn at (gDrawnX/gDrawnY), the rectangle is recomputed and clipped exactly
; as in 1BBE:0002, the screen under it is saved through [ds:80AE] (x, y, w, h, _gPointerBackOff/_gPointerBackSeg) and the
; shape record _gPointerShapeOff/_gPointerShapeSeg is drawn at the unclipped position gShapeX/gShapeY through [ds:80C2]
; (mode 0). Called by 1BB1:0002, 1BF3:000E and 1A72, and by the wrappers below. Pushes AX to
; preserve the caller's.
        PUBLIC  _ShowPointer            ; @ 1BBE:00ED
_ShowPointer PROC FAR
        push    bp
        mov     bp,sp
        push    ax
        cmp     word ptr [g814A],0
        jne     L015B
        cmp     word ptr [_gHideDepth],0
        je      L015B
        dec     word ptr [_gHideDepth]
        cmp     word ptr [_gHideDepth],0
        jne     L015B
        cmp     word ptr [_gPointerBackSeg],0
        je      L015B
        mov     ax,[_gCursorX]
        mov     [_gDrawnX],ax
        sub     ax,[_gPointerHotX]
        mov     [_gRectLeft],ax
        mov     [_gShapeX],ax
        mov     ax,[_gCursorY]
        mov     [_gDrawnY],ax
        sub     ax,[_gPointerHotY]
        mov     [_gRectTop],ax
        mov     [_gShapeY],ax
        mov     ax,[_gPointerHeight]
        mov     [_gSpriteHeight],ax
        neg     ax
        mov     [_gNegSpriteHeight],ax
        mov     ax,[_gPointerWidth8]
        mov     [_gSpriteWidth],ax
        neg     ax
        mov     [_gNegSpriteWidth],ax
        mov     ax,[_gRectLeft]
        sar     ax,1
        sar     ax,1
        sar     ax,1
        cmp     ax,[_gNegSpriteWidth]
        jle     L015B
        cmp     ax,28h
        jl      L015E
L015B:  jmp     ShowDone
L015E:  mov     [_gRectLeft],ax
        add     ax,[_gSpriteWidth]
        mov     [_gRectRight],ax
        cmp     word ptr [_gRectLeft],0
        jge     L0177
        mov     word ptr [_gRectLeft],0
        jmp     short L0184
L0177:  cmp     word ptr [_gRectRight],28h
        jle     L0184
        mov     word ptr [_gRectRight],28h
L0184:  mov     ax,[_gRectTop]
        cmp     ax,[_gNegSpriteHeight]
        jle     L015B
        cmp     ax,0C8h
        jge     L015B
        mov     [_gRectTop],ax
        add     ax,[_gSpriteHeight]
        mov     [_gRectBottom],ax
        cmp     word ptr [_gRectTop],0
        jge     L01AB
        mov     word ptr [_gRectTop],0
        jmp     short L01B9
L01AB:  cmp     word ptr [_gRectBottom],0C8h
        jle     L01B9
        mov     word ptr [_gRectBottom],0C8h
L01B9:  push    word ptr [_gPointerBackSeg]
        push    word ptr [_gPointerBackOff]
        mov     ax,[_gRectBottom]
        sub     ax,[_gRectTop]
        push    ax
        mov     ax,[_gRectRight]
        sub     ax,[_gRectLeft]
        push    ax
        mov     ax,[_gRectTop]
        push    ax
        push    word ptr [_gRectLeft]
        call    dword ptr [_gSaveRect]
        add     sp,0Ch
        push    word ptr [_gShapeY]
        push    word ptr [_gShapeX]
        push    word ptr [_gPointerShapeSeg]
        push    word ptr [_gPointerShapeOff]
        mov     ax,0
        push    ax
        call    dword ptr [gDraw80C2]
        add     sp,0Ah
ShowDone: pop    ax
        pop     bp
        ret
_ShowPointer ENDP

; 1BBE:01FE HidePointerZone: hide the pointer over a rectangle (left, top, right, bottom in
; pixels, at [bp+6]..[bp+0Ch]) and remember it. Takes the semaphore. When the pointer is already
; hidden (gPointerState bit 8000) the low byte of gPointerState is advanced (not past zero) and the
; hide is repeated; otherwise the four arguments are stored in gZoneLeft..gZoneBottom (1A72:0244
; reads them to test whether the cursor has entered the zone), the pointer's rectangle is
; recomputed exactly as in 1BBE:0002 but with x rounded down to a multiple of 8 and the left and
; top reduced by the sprite size, gPointerState is set to 8000h (hidden) and, if that rectangle
; overlaps the argument rectangle, bit 4000 (redraw) is set. Either way the hide call [ds:80AA],
; 1BBE:0002, [ds:80A2] runs, then the semaphore is released. Called by 0EF3, 16A5, 1DE7 and the
; overlays, always paired with 1BBE:02C2.
        PUBLIC  _HidePointerZone        ; @ 1BBE:01FE
_HidePointerZone PROC FAR
        push    bp
        mov     bp,sp
        push    ax
        push    bx
        push    cx
        push    di
        push    dx
        push    si
        mov     cx,[bp+6]
        mov     dx,[bp+8]
        mov     si,[bp+0Ah]
        mov     di,[bp+0Ch]
L0213:  cmp     word ptr [_gPointerBusy],0
        jne     L0213
        inc     word ptr [_gPointerBusy]
        test    word ptr [_gPointerState],8000h
        je      L0234
        mov     ax,[_gPointerState]
        inc     al
        jne     L022F
        dec     al
L022F:  mov     [_gPointerState],ax
        jmp     short L029F
L0234:  mov     [_gZoneLeft],cx
        mov     [_gZoneTop],dx
        mov     [_gZoneRight],si
        mov     [_gZoneBottom],di
        mov     ax,[_gDrawnX]
        sub     ax,[_gPointerHotX]
        and     ax,0FFF8h
        mov     [_gRectLeft],ax
        mov     bx,[_gPointerWidth8]
        shl     bx,1
        shl     bx,1
        shl     bx,1
        sub     [_gZoneLeft],bx
        add     ax,bx
        mov     [_gRectRight],ax
        mov     ax,[_gDrawnY]
        sub     ax,[_gPointerHotY]
        mov     [_gRectTop],ax
        mov     bx,[_gPointerHeight]
        sub     [_gZoneTop],bx
        add     ax,bx
        mov     [_gRectBottom],ax
        mov     word ptr [_gPointerState],8000h
        cmp     [_gRectRight],cx
        jl      L02B6
        cmp     [_gRectLeft],si
        jg      L02B6
        cmp     [_gRectBottom],dx
        jle     L02B6
        cmp     [_gRectTop],di
        jg      L02B6
        or      word ptr [_gPointerState],4000h
L029F:  push    word ptr [g8174]
        push    word ptr [g8172]
        call    dword ptr [g80AA]
        add     sp,4
        push    cs
        call    near ptr _HidePointer
        call    dword ptr [g80A2]
L02B6:  dec     word ptr [_gPointerBusy]
        pop     si
        pop     dx
        pop     di
        pop     cx
        pop     bx
        pop     ax
        pop     bp
        ret
_HidePointerZone ENDP

; 1BBE:02C2 ShowPointerZone: show the pointer again after 1BBE:01FE. Takes the semaphore. When the
; low byte of gPointerState is non-zero it is decremented and the pointer is shown; at zero
; gPointerState is cleared and the pointer is shown only when bit 4000 (redraw) was set, otherwise
; the call does nothing. The show call is [ds:80AA], 1BBE:00ED, [ds:80A2], then the semaphore is
; released. Called by the same sites as 1BBE:01FE.
        PUBLIC  _ShowPointerZone        ; @ 1BBE:02C2
_ShowPointerZone PROC FAR
        push    bp
        mov     bp,sp
        push    ax
L02C6:  cmp     word ptr [_gPointerBusy],0
        jne     L02C6
        inc     word ptr [_gPointerBusy]
        mov     ax,[_gPointerState]
        cmp     al,0
        je      L02DE
        dec     word ptr [_gPointerState]
        jmp     short L02E9
L02DE:  mov     word ptr [_gPointerState],0
        test    ax,4000h
        je      L0300
L02E9:  push    word ptr [g8174]
        push    word ptr [g8172]
        call    dword ptr [g80AA]
        add     sp,4
        push    cs
        call    near ptr _ShowPointer
        call    dword ptr [g80A2]
L0300:  dec     word ptr [_gPointerBusy]
        pop     ax
        pop     bp
        ret
_ShowPointerZone ENDP

; 1BBE:0307 LockHidePointer: hide the pointer, taking the semaphore and bracketing the work with
; [ds:80AA] and [ds:80A2]. Like 1BBE:01FE without the rectangle, the zone and the redraw flag.
; Called by many units of the resident code and the overlays, always paired with 1BBE:0332.
        PUBLIC  _LockHidePointer        ; @ 1BBE:0307
_LockHidePointer PROC FAR
        push    bp
        mov     bp,sp
L030A:  cmp     word ptr [_gPointerBusy],0
        jne     L030A
        inc     word ptr [_gPointerBusy]
        push    word ptr [g8174]
        push    word ptr [g8172]
        call    dword ptr [g80AA]
        add     sp,4
        push    cs
        call    near ptr _HidePointer
        call    dword ptr [g80A2]
        dec     word ptr [_gPointerBusy]
        pop     bp
        ret
_LockHidePointer ENDP

; 1BBE:0332 LockShowPointer: show the pointer, taking the semaphore and bracketing the work with
; [ds:80AA] and [ds:80A2]. Like 1BBE:02C2 without the state and redraw tests. Called by many units
; of the resident code and the overlays, always paired with 1BBE:0307.
        PUBLIC  _LockShowPointer        ; @ 1BBE:0332
_LockShowPointer PROC FAR
        push    bp
        mov     bp,sp
L0335:  cmp     word ptr [_gPointerBusy],0
        jne     L0335
        inc     word ptr [_gPointerBusy]
        push    word ptr [g8174]
        push    word ptr [g8172]
        call    dword ptr [g80AA]
        add     sp,4
        push    cs
        call    near ptr _ShowPointer
        call    dword ptr [g80A2]
        dec     word ptr [_gPointerBusy]
        pop     bp
        ret
_LockShowPointer ENDP

        END

; Unit at EOB.EXE 1BBE:0001 (860 bytes, 7 functions): the game's own mouse pointer, its hide and
; show halves and the wrappers that call them. The pointer is drawn by [ds:80C2] (_gDrawShape in
; src/E1A29.C) from the shape record _gPointerShapeOff/_gPointerShapeSeg, positioned at the mouse
; position gCursorX/gCursorY less the hot spot _gPointerHotX/_gPointerHotY; before it is drawn at
; a new place the screen under it is saved through [ds:80AE] (gSaveRect) into the buffer
; _gPointerBackOff/_gPointerBackSeg, and to erase it the saved
; background is written back through [ds:804E] (gRestoreRect). 1BBE:0002 HidePointer is the hide
; half: restore the background, then gHideDepth++. 1BBE:00ED ShowPointer is the show half:
; gHideDepth is decremented and, only at zero with a save-under buffer (1BBE:010F), the drawn
; position is moved to the mouse position (this move happens even when the rectangle is off screen,
; QUIRK 1BBE:0111), then the screen under it is saved and the pointer drawn.
; 1BBE:01FE HidePointerZone and 1BBE:02C2 ShowPointerZone are the hide/show pair that carry the
; zone (the caller's rectangle, stored in gZoneLeft..gZoneBottom with left and top reduced by the
; sprite size, read by the mouse handler 1A72:0244) and the
; gPointerState flag (bit 8000 a zone hide is in force, bit 4000 a redraw is owed at the show);
; 1BBE:0307 LockHidePointer and 1BBE:0332
; LockShowPointer hide and show without that state. Every wrapper that hides or draws takes the pointer
; busy flag _gPointerBusy and brackets its work with [ds:80AA] and [ds:80A2], but two exits skip the
; bracket: the no-redraw exit of ShowPointerZone (zero count and bit 4000h clear, clears the state
; and releases the busy flag, 1BBE:02DE/0300) and the disjoint-rectangle exit of HidePointerZone
; (QUIRK 1BBE:02B6, which skips the bracket at 1BBE:029F-02B5). The unit is hand-written assembly
; ([H]: the map's c_like test, tools/re/map_units.py:42-46, looks only at the instructions after the
; frame and finds no C-like frame here, the same reading src/E1BB1.ASM:57-59 records; AX is saved with
; push/pop ax and the far call goes through a DGROUP pointer, the style of E1BB1 and E1BF3).
;
; Globals src/ already names keep those names: _gPointerBusy, _gMouseDisabled, _gPointerHeight, _gPointerWidth8,
; _gPointerHotX, _gPointerHotY, _gPointerBackOff, _gPointerBackSeg, _gPointerShapeOff, _gPointerShapeSeg,
; gEgaRegShadowOff, gEgaRegShadowSeg, gSaveDrawState and gRestoreDrawState from src/E1BB1.ASM (and _gMouseDisabled from
; src/E07A9.C, _gDrawShape from src/E1A29.C). The byte at 1BBE:0001 is the unit's segment start, as
; in 1BB1:0001 and 1BF3:000D.

        .MODEL  LARGE

        SCREEN_COLUMNS  EQU 28h ; the 40 eight-pixel columns of the 320 pixel screen (320 / 8): the x
                                ; clip bound in HidePointer (1BBE:0057) and ShowPointer (1BBE:0156) and
                                ; the right edge of the pointer rectangle
        SCREEN_ROWS     EQU 0C8h ; the 200 pixel rows: the y clip bound in HidePointer (1BBE:0090) and
                                 ; ShowPointer (1BBE:0190) and the bottom edge of the pointer rectangle
        COLUMN_MASK     EQU 0FFF8h ; rounds a pixel x down to the 8-pixel column that holds it
                                   ; (HidePointerZone, 1BBE:024B)

        .DATA
        EXTRN   _gPointerBusy:WORD              ; @ ds:812E: pointer busy flag, 0 = free; the wrappers below
                                                ; wait while it is set and then increment it (not atomic;
                                                ; src/E1BB1.ASM:53 says the same of its acquire; 1BBE:0213,
                                                ; 02C6, 030A and 0335), while the mouse handler tests it and
                                                ; drops its event when set (src/E1A72.ASM, label L01E2, grep
                                                ; `_gPointerBusy`: the test, then `je L020C` for the free path,
                                                ; and the drop at label L01E9, `jmp L027F`, with no position
                                                ; update), sets it to 1 when free (label L020C) and clears it
                                                ; to 0 (label L0279). It does not increment and does not wait.
        EXTRN   _gCursorX:WORD          ; @ ds:8138: mouse position, x in pixels (written by the handler
                                        ; at src/E1A72.ASM, label L020C, grep `_gCursorX`: it stores cx,
                                        ; skipped by `je L0221` when `cmp byte ptr [gRecordMode],2` is 2, and the
                                        ; reload is at label L0221; set to 0A0h and 64h by InitMouse, grep
                                        ; `_gCursorX`, the 0A0h and 64h stores; and written by
                                        ; src/E1BF3.ASM:35)
        EXTRN   _gCursorY:WORD          ; @ ds:813A: mouse position, y in pixels (written by the handler
                                        ; at src/E1A72.ASM, label L020C, grep `_gCursorY`: it stores dx
                                        ; right after the `_gCursorX` store, and the reload is at label
                                        ; L0221, which the QUIRK 1A72:0229 jump can skip; set to 64h by
                                        ; InitMouse, grep `_gCursorY`, the 64h store)
        EXTRN   _gHideDepth:WORD        ; @ ds:8146: hide depth, 0 = drawn (1BBE:0002/00ED change it);
                                        ; InitMouse starts it at 1 and the state at 0 (src/E1A72.ASM:312-313,
                                        ; InitMouse, grep `_gHideDepth` and `_gPointerState`), so the pointer
                                        ; starts hidden until the first show
        EXTRN   _gMouseDisabled:WORD              ; @ ds:814A: non-zero disables the pointer (E07A9 tests it too)
        EXTRN   _gPointerHeight:WORD     ; @ ds:8154: sprite height in pixels (record byte +3, 1BB1)
        EXTRN   _gPointerWidth8:WORD     ; @ ds:8156: sprite width in 8-pixel columns ([H]: the stored
                                        ; value is record byte +2 plus one, src/E1BB1.ASM:21-25)
        EXTRN   _gPointerHotX:WORD       ; @ ds:8158: hot spot, x in pixels (HidePointer subtracts it
                                        ; from gDrawnX, 1BBE:0021; ShowPointer from gCursorX, 1BBE:0117)
        EXTRN   _gPointerHotY:WORD       ; @ ds:815A: hot spot, y in pixels (HidePointer subtracts it
                                        ; from gDrawnY, 1BBE:002B; ShowPointer from gCursorY, 1BBE:0127)
        EXTRN   _gDrawnX:WORD           ; @ ds:815C: position drawn at last, x in pixels
        EXTRN   _gDrawnY:WORD           ; @ ds:815E: position drawn at last, y in pixels
        EXTRN   _gPointerState:WORD     ; @ ds:8160: bit 8000 a zone hide is in force, bit 4000 a
                                        ; redraw is owed at the show, low byte a repeat count (1BBE:01FE/
                                        ; 02C2; the handler tests 8000h at src/E1A72.ASM, grep
                                        ; `_gPointerState`, before its zone test, and sets 4000h for a hide
                                        ; in the zone on the path labelled L01EC, whose `call far ptr
                                        ; _HidePointer` sits inside the [ds:80AA]/[ds:80A2] bracket)
        EXTRN   _gZoneLeft:WORD         ; @ ds:8162: hide zone, left in pixels, stored as the argument
                                        ; less the sprite width in pixels (_gPointerWidth8 shifted left
                                        ; three times, 1BBE:0234, 025B); the handler's left compare
                                        ; `cmp cx,[_gZoneLeft]; jl L0250` runs, but QUIRK 1A72:0248 sends
                                        ; a left miss past the right-edge test (`cmp cx,[_gZoneRight];
                                        ; jg L025C`) to the y tests at L0250, so the left edge is never
                                        ; enforced: only x past the right edge and y outside top and
                                        ; bottom leave the zone
        EXTRN   _gZoneTop:WORD          ; @ ds:8164: hide zone, top in pixels, stored as the argument
                                        ; less the sprite height (_gPointerHeight, already in pixels, no
                                        ; shift; 1BBE:0238, 0272)
        EXTRN   _gZoneRight:WORD        ; @ ds:8166: hide zone, right in pixels as given (1BBE:023C)
        EXTRN   _gZoneBottom:WORD       ; @ ds:8168: hide zone, bottom in pixels as given (1BBE:0240)
        EXTRN   _gPointerBackOff:WORD    ; @ ds:816A: save-under buffer, far offset (1BB1 allocates)
        EXTRN   _gPointerBackSeg:WORD    ; @ ds:816C: save-under buffer, far segment, 0 = none
        EXTRN   _gPointerShapeOff:WORD   ; @ ds:816E: pointer shape record, far offset (1BB1 stores it)
        EXTRN   _gPointerShapeSeg:WORD   ; @ ds:8170: pointer shape record, far segment
        EXTRN   gEgaRegShadowOff:WORD              ; @ ds:8172: far pointer handed to [ds:80AA], offset; the 17A6
                                        ; unit saves and restores the 16 bytes under it (src/E1A72.ASM:574-582
                                        ; is the _SetFarPointer header, grep `_SetFarPointer`; the sentence
                                        ; is at :565; the saved copy is E17A6.ASM:112-114)
        EXTRN   gEgaRegShadowSeg:WORD              ; @ ds:8174: the same far pointer, segment (pushed first)
        EXTRN   _gRestoreRect:DWORD     ; @ ds:804E: far fn (x, y, w, h, buf): buffer back to screen;
                                        ; x and w are in 8-pixel columns, y and h in pixel rows
                                        ; (HidePointer shifts the left edge right by three, the three
                                        ; `sar ax,1` at 1BBE:004B-0050, after the `mov ax,[_gRectLeft]`
                                        ; at 1BBE:0048-004A)
        EXTRN   _gSaveRect:DWORD        ; @ ds:80AE: far fn (x, y, w, h, buf): screen into buffer;
                                        ; x and w are in 8-pixel columns, y and h in pixel rows
                                        ; (ShowPointer shifts the left edge right by three, the three
                                        ; `sar ax,1` at 1BBE:014A-014F, after the `mov ax,[_gRectLeft]`
                                        ; at 1BBE:0147-0149)
        EXTRN   gSaveDrawState:DWORD             ; @ ds:80AA: far fn (gEgaRegShadowOff/gEgaRegShadowSeg) before the pointer work
        EXTRN   gRestoreDrawState:DWORD             ; @ ds:80A2: far fn (no arguments) after the pointer work
        EXTRN   _gDrawShape:DWORD         ; @ ds:80C2: far fn (mode, void far *src, x, y); the `gDraw`
                                        ; prefix is a family guess (src/E1A29.C:53); the evidence is
                                        ; ShowPointer below, which calls it at 1BBE:01F4 with mode 0 and
                                        ; the shape record _gPointerShapeOff/_gPointerShapeSeg at the
                                        ; unclipped position _gShapeX/_gShapeY (stored at 1BBE:011E and
                                        ; 012E; src/E1BBE.ASM:379-384 gives the push order, call at :385)

; @data
; the data of e1bbe, ds:8176 to ds:8196 (32 bytes, no relocations), the original's bytes in address
; order: the pointer rectangle and sprite bounds this unit's code keeps, then twelve bytes the
; overlay loader uses.

        PUBLIC  _gRectLeft ; @ ds:8176: pointer rectangle, left; in 8-pixel columns and
                                        ; clipped in HidePointer/ShowPointer, in pixels and unclipped
                                        ; in HidePointerZone, which compares it with the zone
_gRectLeft      dw      0

        PUBLIC  _gRectTop ; @ ds:8178: pointer rectangle, top; pixel rows clipped in
                                        ; HidePointer/ShowPointer, pixels unclipped in HidePointerZone
_gRectTop       dw      0

        PUBLIC  _gRectRight ; @ ds:817A: pointer rectangle, right; as _gRectLeft
_gRectRight     dw      0

        PUBLIC  _gRectBottom ; @ ds:817C: pointer rectangle, bottom; as _gRectTop
_gRectBottom    dw      0

        PUBLIC  _gShapeX ; @ ds:817E: shape drawn at, x in pixels, unclipped
_gShapeX        dw      0

        PUBLIC  _gShapeY ; @ ds:8180: shape drawn at, y in pixels, unclipped
_gShapeY        dw      0

        PUBLIC  _gSpriteHeight ; @ ds:8182: copy of _gPointerHeight, the sprite height in pixel
                                        ; rows; added to the top to get the bottom (clip bounds 0 and SCREEN_ROWS)
_gSpriteHeight  dw      0

        PUBLIC  _gNegSpriteHeight ; @ ds:8184: minus _gPointerHeight, the lower y clip bound in rows
_gNegSpriteHeight dw      0

        PUBLIC  _gSpriteWidth ; @ ds:8186: copy of _gPointerWidth8, the sprite width in 8-pixel
                                        ; columns; added to the left to get the right (bounds 0 and SCREEN_COLUMNS)
_gSpriteWidth   dw      0

        PUBLIC  _gNegSpriteWidth ; @ ds:8188: minus _gPointerWidth8, the lower x clip bound in columns
_gNegSpriteWidth dw      0

; the twelve bytes ds:818A to 8196, 0 in the image, are the overlay loader's
; (src/E1C00.ASM reads and writes them by address, with no name): named by their address.

; the INT 21h/4B03 parameter block, two words: the load segment and the relocation factor
; (src/E1C00.ASM:66-68, 1C00:002F and 007C)
g818A label byte ; ds:818A
        dw      0, 0

; the far pointer through which 1C00:0055 calls the overlay's entry, offset then segment
; (src/E1C00.ASM:198-200, 1C00:0081 and 0085)
g818E label byte ; ds:818E
        dd      0

; SP and SS saved across the overlay load (src/E1C00.ASM:101-102 and 110-111)
g8192 label byte ; ds:8192
        dw      0, 0
        .CODE

; 1BBE:0001: Purpose unknown: the unit's first byte is 00. It is emitted under a public symbol
; so the registry records it and the unit counts complete, as 1BB1:0001, 1BA8:0005 and 1BF3:000D
; do; it is the unit's segment start, not code and not called.
        PUBLIC  _PadBeforeHidePointer
_PadBeforeHidePointer LABEL BYTE                    ; @ 1BBE:0001
        db      0

; 1BBE:0002 HidePointer: hide the pointer. Does nothing when the pointer is disabled (_gMouseDisabled).
; A hide while already hidden (gHideDepth non-zero, 1BBE:0015) only increments gHideDepth instead of
; restoring the background, so a nested hide is counted even when no save-under buffer exists; the
; buffer test is reached only when not hidden. When not hidden and no save-under buffer exists
; (_gPointerBackSeg zero) nothing happens. Otherwise the rectangle the pointer covers is recomputed
; from the position it was last drawn at (gDrawnX/gDrawnY) less the hot spot (_gPointerHotX/_gPointerHotY) and the sprite size
; (_gPointerHeight/_gPointerWidth8; the x in 8-pixel columns, the y in rows, each clipped to the 40x200
; screen), the saved background is written back to the screen through [ds:804E] (x, y, w, h,
; _gPointerBackOff/_gPointerBackSeg) and gHideDepth is incremented. When the rectangle is off screen the
; restore is skipped but gHideDepth is still incremented (1BBE:0055 and 005A reach the increment at
; 1BBE:00E0 through the jmp short at 1BBE:0075; 1BBE:008B and 0090 jump there directly). The height
; and width used are the ones still in place, the old shape's: 1BB1 calls here (src/E1BB1.ASM:130)
; before it stores the new sizes (src/E1BB1.ASM:162-164), which is what keeps the old buffer valid
; while the shape is replaced. Called by 1BB1:0002, 1BF3:000E,
; the mouse handler inside _InitMouse (1A72:00DA; the handler entry is 1A72:017A) at
; src/E1A72.ASM, label L01EC (the in-zone hide) and label L025C (the other path),
; and by the wrappers below. Pushes AX only to preserve the caller's.
        PUBLIC  _HidePointer            ; @ 1BBE:0002
_HidePointer PROC FAR
        push    bp
        mov     bp,sp
        push    ax
        cmp     word ptr [_gMouseDisabled],0
        je      enabled
skipHide: jmp    HideDone
enabled: cmp     word ptr [_gHideDepth],0
        jne     L0075
        cmp     word ptr [_gPointerBackSeg],0
        je      skipHide
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
        cmp     ax,SCREEN_COLUMNS
        jge     L0075
        mov     [_gRectLeft],ax
        add     ax,[_gSpriteWidth]
        mov     [_gRectRight],ax
        cmp     word ptr [_gRectLeft],0
        jge     L0077
        mov     word ptr [_gRectLeft],0
        jmp     short L0084
L0075:  jmp     short L00E0
L0077:  cmp     word ptr [_gRectRight],SCREEN_COLUMNS
        jle     L0084
        mov     word ptr [_gRectRight],SCREEN_COLUMNS
L0084:  mov     ax,[_gRectTop]
        cmp     ax,[_gNegSpriteHeight]
        jle     L00E0
        cmp     ax,SCREEN_ROWS
        jge     L00E0
        mov     [_gRectTop],ax
        add     ax,[_gSpriteHeight]
        mov     [_gRectBottom],ax
        cmp     word ptr [_gRectTop],0
        jge     L00AB
        mov     word ptr [_gRectTop],0
        jmp     short L00B9
L00AB:  cmp     word ptr [_gRectBottom],SCREEN_ROWS
        jle     L00B9
        mov     word ptr [_gRectBottom],SCREEN_ROWS
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
; gHideDepth saturates at 0FFFFh: on the wrap to zero the increment is undone, because a zero depth
; would make a hidden pointer look shown.
L00E0:  inc     word ptr [_gHideDepth]
        jne     HideDone
        dec     word ptr [_gHideDepth]
HideDone: pop    ax
        pop     bp
        ret
_HidePointer ENDP

; 1BBE:00ED ShowPointer: show the pointer. Does nothing when it is disabled (_gMouseDisabled) or when it
; is not hidden (gHideDepth zero, 1BBE:00FD). Otherwise gHideDepth is decremented first (1BBE:00FF), so
; the count still changes even when no save-under buffer exists (_gPointerBackSeg, 1BBE:010F) or the
; rectangle is off screen (1BBE:0154, 018B and 0190 reach ShowDone after the decrement, and the
; off-screen fall-through at 1BBE:0159 reaches the jmp ShowDone at 1BBE:015B); at zero with a buffer
; the current mouse position (gCursorX/gCursorY) is copied to the position being drawn at
; (gDrawnX/gDrawnY) before the on-screen test (1BBE:0111-0114 and 0121-0124), so an off-screen show
; still moves the drawn position (QUIRK 1BBE:0111); the on-screen test (1BBE:0150-0159) then gates
; only the rectangle, the save and the draw: with an on-screen rectangle the rectangle is recomputed
; and clipped as in 1BBE:0002, the screen under it is saved through [ds:80AE] (x, y, w, h,
; _gPointerBackOff/_gPointerBackSeg) and the shape record _gPointerShapeOff/_gPointerShapeSeg is drawn
; at the unclipped position gShapeX/gShapeY through [ds:80C2] (mode 0). Called by 1BB1:0002,
; 1BF3:000E and the mouse handler inside _InitMouse (1A72:00DA; the handler entry is 1A72:017A) at
; src/E1A72.ASM, label L025C (after its _HidePointer call), and by the wrappers below. Pushes AX to
; preserve the caller's.
        PUBLIC  _ShowPointer            ; @ 1BBE:00ED
_ShowPointer PROC FAR
        push    bp
        mov     bp,sp
        push    ax
        cmp     word ptr [_gMouseDisabled],0
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
        cmp     ax,SCREEN_COLUMNS
        jl      L015E
L015B:  jmp     ShowDone
L015E:  mov     [_gRectLeft],ax
        add     ax,[_gSpriteWidth]
        mov     [_gRectRight],ax
        cmp     word ptr [_gRectLeft],0
        jge     L0177
        mov     word ptr [_gRectLeft],0
        jmp     short L0184
L0177:  cmp     word ptr [_gRectRight],SCREEN_COLUMNS
        jle     L0184
        mov     word ptr [_gRectRight],SCREEN_COLUMNS
L0184:  mov     ax,[_gRectTop]
        cmp     ax,[_gNegSpriteHeight]
        jle     L015B
        cmp     ax,SCREEN_ROWS
        jge     L015B
        mov     [_gRectTop],ax
        add     ax,[_gSpriteHeight]
        mov     [_gRectBottom],ax
        cmp     word ptr [_gRectTop],0
        jge     L01AB
        mov     word ptr [_gRectTop],0
        jmp     short L01B9
L01AB:  cmp     word ptr [_gRectBottom],SCREEN_ROWS
        jle     L01B9
        mov     word ptr [_gRectBottom],SCREEN_ROWS
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
        call    dword ptr [_gDrawShape]
        add     sp,0Ah
ShowDone: pop    ax
        pop     bp
        ret
_ShowPointer ENDP

; 1BBE:01FE HidePointerZone: hide the pointer over the zone (the caller's rectangle, left, top, right,
; bottom in pixels, at [bp+6]..[bp+0Ch]) and remember it. Takes the busy flag. When a zone hide is
; already in force (gPointerState bit 8000, 1BBE:021E) the low byte of gPointerState is incremented and
; saturates at 0FFh (1BBE:0229 `inc al`, then 1BBE:022D `dec al` undoes it on wrap) and the hide is
; repeated without recomputing the zone or the rectangle, so bit 4000 is decided by the first hide
; alone (1BBE:0232 jmp short L029F); otherwise the four arguments are stored in gZoneLeft..gZoneBottom,
; with right and bottom as given (1BBE:023C, 0240) but left and top reduced by the sprite width and
; height in pixels (1BBE:025B and 0272; 1A72:0244 reads the zone to test whether the cursor is inside
; it), the pointer's rectangle is computed in pixels without clipping (left x rounded down to a multiple
; of 8, right = left + sprite width, top = the drawn y less the hot spot y (no rounding), bottom = top +
; sprite height; 1BBE:0244..0278), and gPointerState is set to 8000h (a zone hide is in force). If that
; rectangle overlaps the zone, bit 4000 (a redraw is owed at the show) is set (1BBE:0299) and the code
; falls into the bracket; a disjoint rectangle jumps to 1BBE:02B6, skipping the hide and the bracket and
; leaving only 8000h (QUIRK 1BBE:02B6: a disjoint hide hides nothing but still sets 8000h, which the
; handler reads; a port that omits it changes the handler's later behaviour). The bracket is [ds:80AA],
; 1BBE:0002, [ds:80A2] (1BBE:029F). The push cs before
; the near call to 1BBE:0002 (1BBE:02AE-02AF) makes that pair a far call: the callee is a FAR proc,
; so its RETF (1BBE:00EC) pops the CS the push put on the stack; 1BBE:02F8, 0324 and 034F are the same
; form. Then the busy flag is released. Called by 0EF3, 16A5, 1DE7 and the overlays; the packet lists
; the same 14 callers for 1BBE:02C2, so the pair is called at the same sites.
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
        and     ax,COLUMN_MASK
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
L029F:  push    word ptr [gEgaRegShadowSeg]
        push    word ptr [gEgaRegShadowOff]
        call    dword ptr [gSaveDrawState]
        add     sp,4
        push    cs                      ; far call: the FAR ret pops this CS (see 1BBE:01FE)
        call    near ptr _HidePointer
        call    dword ptr [gRestoreDrawState]
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

; 1BBE:02C2 ShowPointerZone: show the pointer again after 1BBE:01FE. Takes the busy flag. When the
; low byte of gPointerState is non-zero it is decremented and the pointer is shown (regardless of bit
; 4000); at zero gPointerState is cleared (1BBE:02DE) and the pointer is shown only when bit 4000
; (redraw) was set, otherwise it clears the state and draws nothing. The show call is [ds:80AA],
; 1BBE:00ED, [ds:80A2]; the busy flag is released either way (1BBE:0300). Called by the same sites as
; 1BBE:01FE.
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
L02E9:  push    word ptr [gEgaRegShadowSeg]
        push    word ptr [gEgaRegShadowOff]
        call    dword ptr [gSaveDrawState]
        add     sp,4
        push    cs                      ; far call: the FAR ret pops this CS (see 1BBE:01FE)
        call    near ptr _ShowPointer
        call    dword ptr [gRestoreDrawState]
L0300:  dec     word ptr [_gPointerBusy]
        pop     ax
        pop     bp
        ret
_ShowPointerZone ENDP

; 1BBE:0307 LockHidePointer: hide the pointer, taking the busy flag and bracketing the work with
; [ds:80AA] and [ds:80A2]. "Lock" is the callers' name for the frame bracket, not mutual exclusion:
; the bracket rests on the code itself, [ds:80AA] and [ds:80A2] around the hide (1BBE:0307-0332), and
; the busy flag is a test then an increment (1BBE:030A), not atomic (src/E1BB1.ASM:53 says the same
; of its acquire). Like 1BBE:01FE without the rectangle, the zone and the redraw flag.
; Called by many units of the resident code and the overlays, usually paired with 1BBE:0332 (the
; call-site lists differ: 0ACD:0E8A, 0ACD:0F72, ovl0:2547 and ovl5:17C8 call only the show, ovl0:28F9
; only the hide).
        PUBLIC  _LockHidePointer        ; @ 1BBE:0307
_LockHidePointer PROC FAR
        push    bp
        mov     bp,sp
L030A:  cmp     word ptr [_gPointerBusy],0
        jne     L030A
        inc     word ptr [_gPointerBusy]
        push    word ptr [gEgaRegShadowSeg]
        push    word ptr [gEgaRegShadowOff]
        call    dword ptr [gSaveDrawState]
        add     sp,4
        push    cs                      ; far call: the FAR ret pops this CS (see 1BBE:01FE)
        call    near ptr _HidePointer
        call    dword ptr [gRestoreDrawState]
        dec     word ptr [_gPointerBusy]
        pop     bp
        ret
_LockHidePointer ENDP

; 1BBE:0332 LockShowPointer: show the pointer, taking the busy flag and bracketing the work with
; [ds:80AA] and [ds:80A2] ("Lock" is the callers' name for the frame bracket, not mutual exclusion;
; see the note on 1BBE:0307). Like 1BBE:02C2 without the state and redraw tests. Called by many units
; of the resident code and the overlays, usually paired with 1BBE:0307.
        PUBLIC  _LockShowPointer        ; @ 1BBE:0332
_LockShowPointer PROC FAR
        push    bp
        mov     bp,sp
L0335:  cmp     word ptr [_gPointerBusy],0
        jne     L0335
        inc     word ptr [_gPointerBusy]
        push    word ptr [gEgaRegShadowSeg]
        push    word ptr [gEgaRegShadowOff]
        call    dword ptr [gSaveDrawState]
        add     sp,4
        push    cs                      ; far call: the FAR ret pops this CS (see 1BBE:01FE)
        call    near ptr _ShowPointer
        call    dword ptr [gRestoreDrawState]
        dec     word ptr [_gPointerBusy]
        pop     bp
        ret
_LockShowPointer ENDP

        END

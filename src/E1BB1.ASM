; Unit at EOB.EXE 1BB1:0001 (208 bytes, 2 functions), kind asm [H] by the map. 1BB1:0001 is a
; single 00 byte with `segment` evidence (the unit's start marker, like 1990:0001); the one real
; function follows at 1BB1:0002.
;
; 1BB1:0002 sets the mouse pointer's shape. Called from f07A9_010C (07A9:010C) with a hot-spot
; offset (8,8) when n > 0 or (0,0) otherwise, and a pointer shape record gIconsDDE4[n] (E07A9.C:92-94;
; the table holds item icon images, nothing here says "level" or "name"). Unit 1BBE shows what the
; globals are: the hot spot ds:8158/815A is subtracted from the last drawn pointer position
; ds:815C/815E (1BBE:001E..002F, 1BBE:0111..012E), and the shape record ds:816E/8170 is what
; 1BBE:00ED draws through [ds:80C2]. The save-under buffer ds:816A/816C is the screen behind the
; pointer: 1BBE:0002 restores it through [ds:804E] and 1BBE:00ED saves the background into it
; through [ds:80AE].
;
; The size comes from the shape record: byte SHAPE_HEIGHT_BYTE (+3) is the height in pixels,
; taken unchanged (1BB1:0034 stores it at ds:8154; 1BBE:0095 adds it to the pointer's y and clips
; to 200), and byte SHAPE_WIDTH_BYTE (+2), plus 1, is the width in 8-pixel units (1BB1:003E
; `inc dx` stores dx at ds:8156; 1BBE:005F adds it to x>>3 and clips to 40, the same unit as the
; region records of 1A29). The width is an unsigned char count, so a stored width of 0 means 1
; column; a stored height of 0 is 0 pixels. If the size changed, the old buffer is freed and the
; size query [ds:80C6] is called as (width, height); its result is the new buffer size. The check
; needs LargestFreeBlock (19BC:023D) to return at least that result plus 16 (1BB1:0076
; `add ax,16`), and that function already subtracts 16 from the raw largest free block (19BC:0142
; `max -= 0x10`, its header "less one 16 byte block"), so a raw free block of result + 32 bytes
; is what passes. MemAlloc then allocates the result at 1BB1:0091..0094 (si minus the same 16).
;
; The order is what keeps the old buffer valid: 1BBE:0002 hides the pointer first, while the old
; buffer and sizes are still in place, then the globals are overwritten, the buffer is resized,
; and 1BBE:00ED shows the pointer again and draws the new shape. 1BBE:0002 does nothing when
; ds:814A is set; when the hide count ds:8146 is nonzero it only increments it (a nested hide);
; when the count is 0 and ds:816C (no buffer) is 0 it does nothing; otherwise it restores the
; background and increments ds:8146. 1BBE:00ED decrements the count and, at zero, copies the
; position, saves the background and draws. The pair [ds:80AA] on the far pointer ds:8172/8174
; and [ds:80A2] brackets only the show call at 1BB1:00B8 (1BB1:00B1 and 1BB1:00BD); nobody in
; this unit or in E1BF3 writes ds:8172/8174, so the writer is unknown.
;
; The function acquires the busy flag at ds:812E (a test then an increment, not a lock), returns 1,
; or 0 when no block can be allocated.
;
; The unit is hand-written assembly ([H]: the map's c_like test, tools/re/map_units.py:42-46, looks
; only at the instructions after the frame and finds no C-like frame here), so the module uses
; explicit segments like the runtime startup; `ASSUME CS:NOTHING` around each direct far call
; keeps TASM from turning `lcall` into `push cs; call` (which it does when it assumes the far
; callee is in the same code segment).

        SHAPE_WIDTH_BYTE  EQU 2  ; shape record byte +2: width in 8-pixel units (1BB1:003A)
        SHAPE_HEIGHT_BYTE EQU 3  ; shape record byte +3: height in pixels (1BB1:0034)

        _DATA   SEGMENT WORD PUBLIC 'DATA'
        EXTRN   _gPointerBusy:WORD      ; @ ds:812E: busy flag, 0 = free; a test then an
                                        ; increment, not a lock, acquired by busy-wait
        EXTRN   _gPointerHeight:WORD    ; @ ds:8154: pointer height in pixels, from shape byte +3
        EXTRN   _gPointerWidth8:WORD    ; @ ds:8156: pointer width in 8-pixel units, shape byte +2 + 1
        EXTRN   _gPointerHotX:WORD      ; @ ds:8158: pointer hot spot, x (argument 1)
        EXTRN   _gPointerHotY:WORD      ; @ ds:815A: pointer hot spot, y (argument 2)
        EXTRN   _gPointerBackOff:WORD   ; @ ds:816A: save-under buffer, far offset
        EXTRN   _gPointerBackSeg:WORD   ; @ ds:816C: save-under buffer, far segment (0 = none)
        EXTRN   _gPointerShapeOff:WORD  ; @ ds:816E: pointer shape record, far offset
        EXTRN   _gPointerShapeSeg:WORD  ; @ ds:8170: pointer shape record, far segment
        EXTRN   g8172:WORD              ; @ ds:8172: argument of the [ds:80AA] call, offset (writer unknown)
        EXTRN   g8174:WORD              ; @ ds:8174: argument of the [ds:80AA] call, segment (writer unknown)
        EXTRN   g80C6:DWORD             ; @ ds:80C6: indirect far function, buffer size (width, height)
        EXTRN   g80AA:DWORD             ; @ ds:80AA: indirect far function, called with ds:8172/8174
        EXTRN   g80A2:DWORD             ; @ ds:80A2: indirect far function, no arguments
        _DATA   ENDS
        DGROUP  GROUP   _DATA

        _TEXT   SEGMENT WORD PUBLIC 'CODE'
        ASSUME  CS:_TEXT, DS:DGROUP
        db      0                       ; 1BB1:0001: the unit's start marker byte (segment evidence)
        EXTRN   _HidePointer:FAR        ; @ 1BBE:0002: hide the pointer (restore background, ++gHideDepth)
        EXTRN   _ShowPointer:FAR        ; @ 1BBE:00ED: show the pointer (--gHideDepth, save, draw)
        EXTRN   _MemAlloc:FAR           ; @ 19BC:0007
        EXTRN   _MemFree:FAR            ; @ 19BC:0137
        EXTRN   _LargestFreeBlock:FAR         ; @ 19BC:023D: returns the largest free block (DX:AX)

; 1BB1:0002: set the mouse pointer shape to the record at (argument 3, argument 4) and the hot
; spot to (argument 1, argument 2), resizing the save-under buffer when the sprite size changed.
; Hides the pointer before overwriting the globals and shows it after, all under the busy flag at
; ds:812E.
        PUBLIC  _SetMousePointerShape             ; @ 1BB1:0002
_SetMousePointerShape PROC FAR
        push    bp
        mov     bp,sp
        push    dx
        push    si
        push    es
acquire:
        cmp     word ptr [_gPointerBusy],0
        jne     acquire
        inc     word ptr [_gPointerBusy]
        ASSUME  CS:NOTHING
        call    far ptr _HidePointer
        ASSUME  CS:_TEXT
        mov     ax,[bp+6]
        mov     word ptr [_gPointerHotX],ax
        mov     ax,[bp+8]
        mov     word ptr [_gPointerHotY],ax
        mov     si,[bp+0Ah]
        mov     word ptr [_gPointerShapeOff],si
        mov     es,[bp+0Ch]
        mov     word ptr [_gPointerShapeSeg],es
        xor     ah,ah
        mov     al,byte ptr es:[si+SHAPE_HEIGHT_BYTE]   ; height in pixels, stored unchanged
        xor     dh,dh
        mov     dl,byte ptr es:[si+SHAPE_WIDTH_BYTE]    ; width in 8-pixel units
        inc     dx
        cmp     word ptr [_gPointerBackSeg],0
        je      setsize
        cmp     ax,word ptr [_gPointerHeight]
        jne     resize
        cmp     dx,word ptr [_gPointerWidth8]
        je      ready
resize: push    ax
        push    dx
        push    word ptr [_gPointerBackSeg]
        push    word ptr [_gPointerBackOff]
        ASSUME  CS:NOTHING
        call    far ptr _MemFree
        ASSUME  CS:_TEXT
        add     sp,4
        pop     dx
        pop     ax
setsize:
        mov     word ptr [_gPointerHeight],ax
        push    ax
        mov     word ptr [_gPointerWidth8],dx
        push    dx
        call    dword ptr [g80C6]
        add     sp,4
        add     ax,16
        mov     si,ax
        ASSUME  CS:NOTHING
        call    far ptr _LargestFreeBlock
        ASSUME  CS:_TEXT
        or      dx,dx
        jne     alloc
        cmp     ax,si
        jb      failed
alloc:  mov     ax,0
        push    ax
        xor     ax,ax
        push    ax
        mov     ax,si
        sub     ax,16
        push    ax
        ASSUME  CS:NOTHING
        call    far ptr _MemAlloc
        ASSUME  CS:_TEXT
        add     sp,6
        cmp     dx,0
        je      failed
        mov     word ptr [_gPointerBackOff],ax
        mov     word ptr [_gPointerBackSeg],dx
ready:  push    word ptr [g8174]
        push    word ptr [g8172]
        call    dword ptr [g80AA]
        add     sp,4
        ASSUME  CS:NOTHING
        call    far ptr _ShowPointer
        ASSUME  CS:_TEXT
        call    dword ptr [g80A2]
        mov     ax,1
        jmp     short release
; QUIRK 1BB1:00C6: on failure the old buffer was already freed at resize and the new size is
; already stored, but ds:816A/816C still hold the freed pointer, so a later call with the same
; size passes `je ready` with a dangling buffer. A first-time failure (ds:816C == 0) leaves it
; zero and the next call retries.
;
; QUIRK 1BB1:00C6: the failure path also skips the 1BBE:00ED call above, so when ds:816C was
; nonzero when 1BBE:0002 was called (1BB1:0013) the hide it did (incrementing ds:8146) is never
; undone and the pointer stays hidden until something else balances the count. When ds:816C was 0
; on entry, 1BBE:0002 returned before incrementing (1BBE:0017), so the first-time failure above
; leaks no hide. Both paths still release the busy flag ds:812E at `release`.
failed: xor     ax,ax
release:
        dec     word ptr [_gPointerBusy]
        pop     es
        pop     si
        pop     dx
        pop     bp
        ret
_SetMousePointerShape ENDP

        _TEXT   ENDS
        END
